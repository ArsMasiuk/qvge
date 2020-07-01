/** \file
 * \brief Implements class SubgraphPlanarizer.
 *
 * \author Markus Chimani, Carsten Gutwenger
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <ogdf/planarity/SubgraphPlanarizer.h>
#include <ogdf/planarity/VariableEmbeddingInserter.h>
#include <ogdf/planarity/PlanarSubgraphFast.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/planarity/embedder/CrossingStructure.h>

using std::atomic;
using std::mutex;
using std::lock_guard;
using std::minstd_rand;

namespace ogdf {

using embedder::CrossingStructure;

class SubgraphPlanarizer::ThreadMaster {
	CrossingStructure *m_pCS;
	int                m_bestCR;

	const PlanRep     &m_pr;
	int                m_cc;

	const EdgeArray<int>      *m_pCost;
	const EdgeArray<bool>     *m_pForbid;
	const EdgeArray<uint32_t> *m_pEdgeSubGraph;
	const List<edge>          &m_delEdges;

	int         m_seed;
	atomic<int> m_perms;
	int64_t     m_stopTime;
	mutex       m_mutex;

public:
	ThreadMaster(
		const PlanRep &pr,
		int cc,
		const EdgeArray<int>  *pCost,
		const EdgeArray<bool> *pForbid,
		const EdgeArray<uint32_t> *pEdgeSubGraphs,
		const List<edge> &delEdges,
		int seed,
		int perms,
		int64_t stopTime);

	~ThreadMaster() { delete m_pCS; }

	const PlanRep &planRep() const { return m_pr; }
	int currentCC() const { return m_cc; }

	const EdgeArray<int> *cost() const { return m_pCost; }
	const EdgeArray<bool> *forbid() const { return m_pForbid; }
	const EdgeArray<uint32_t> *edgeSubGraphs() const { return m_pEdgeSubGraph; }
	const List<edge> &delEdges() const { return m_delEdges; }

	int rseed(long id) const { return (int)id * m_seed; }

	int queryBestKnown() const { return m_bestCR; }
	CrossingStructure *postNewResult(CrossingStructure *pCS);
	bool getNextPerm();

	void restore(PlanRep &pr, int &cr);
};


class SubgraphPlanarizer::Worker {

	int                  m_id;
	ThreadMaster        *m_pMaster;
	EdgeInsertionModule *m_pInserter;

public:
	Worker(int id, ThreadMaster *pMaster, EdgeInsertionModule *pInserter) : m_id(id), m_pMaster(pMaster), m_pInserter(pInserter) { }
	~Worker() { delete m_pInserter; }

	void operator()();

private:
	Worker(const Worker &other); // = delete
	Worker &operator=(const Worker &other); // = delete
};


SubgraphPlanarizer::ThreadMaster::ThreadMaster(
	const PlanRep &pr,
	int cc,
	const EdgeArray<int>  *pCost,
	const EdgeArray<bool> *pForbid,
	const EdgeArray<uint32_t> *pEdgeSubGraphs,
	const List<edge> &delEdges,
	int seed,
	int perms,
	int64_t stopTime)
	:
	m_pCS(nullptr), m_bestCR(std::numeric_limits<int>::max()), m_pr(pr), m_cc(cc),
	m_pCost(pCost), m_pForbid(pForbid), m_pEdgeSubGraph(pEdgeSubGraphs),
	m_delEdges(delEdges), m_seed(seed), m_perms(perms), m_stopTime(stopTime)
{ }


CrossingStructure *SubgraphPlanarizer::ThreadMaster::postNewResult(CrossingStructure *pCS)
{
	int newCR = pCS->weightedCrossingNumber();

	lock_guard<mutex> guard(m_mutex);

	if(newCR < m_bestCR) {
		std::swap(pCS, m_pCS);
		m_bestCR = newCR;
	}

	return pCS;
}


bool SubgraphPlanarizer::ThreadMaster::getNextPerm()
{
	if(m_stopTime >= 0 && System::realTime() >= m_stopTime)
		return false;

	return --m_perms >= 0;
}


void SubgraphPlanarizer::ThreadMaster::restore(PlanRep &pr, int &cr)
{
	m_pCS->restore(pr, m_cc);
	cr = m_bestCR;
}


bool SubgraphPlanarizer::doSinglePermutation(
	PlanRepLight &prl,
	int cc,
	const EdgeArray<int>  *pCost,
	const EdgeArray<bool> *pForbid,
	const EdgeArray<uint32_t> *pEdgeSubGraphs,
	Array<edge> &deletedEdges,
	EdgeInsertionModule &inserter,
	minstd_rand &rng,
	int &crossingNumber)
{
	prl.initCC(cc);

	const int nG = prl.numberOfNodes();
	const int high = deletedEdges.high();

	for(int j = 0; j <= high; ++j)
		prl.delEdge(prl.copy(deletedEdges[j]));

	deletedEdges.permute(rng);

	ReturnType ret = inserter.callEx(prl, deletedEdges, pCost, pForbid, pEdgeSubGraphs);

	if(!isSolution(ret))
		return false; // no solution found, that's bad...

	if(pCost == nullptr)
		crossingNumber = prl.numberOfNodes() - nG;
	else {
		crossingNumber = 0;
		for(node n : prl.nodes) {
			if(prl.original(n) == nullptr) { // dummy found -> calc cost
				edge e1 = prl.original(n->firstAdj()->theEdge());
				edge e2 = prl.original(n->lastAdj()->theEdge());
				if(pEdgeSubGraphs != nullptr) {
					int subgraphCounter = 0;
					for(int i = 0; i < 32; i++) {
						if((((*pEdgeSubGraphs)[e1] & (1<<i))!=0) && (((*pEdgeSubGraphs)[e2] & (1<<i)) != 0))
							subgraphCounter++;
					}
					crossingNumber += (subgraphCounter * (*pCost)[e1] * (*pCost)[e2]);
				} else
					crossingNumber += (*pCost)[e1] * (*pCost)[e2];
			}
		}
	}

	return true;
}

void SubgraphPlanarizer::doWorkHelper(ThreadMaster &master, EdgeInsertionModule &inserter, minstd_rand &rng)
{
	const List<edge> &delEdges = master.delEdges();

	const int m = delEdges.size();
	Array<edge> deletedEdges(m);
	int j = 0;
	for(edge e : delEdges)
		deletedEdges[j++] = e;

	PlanRepLight prl(master.planRep());
	int cc = master.currentCC();

	const EdgeArray<int>  *pCost = master.cost();
	const EdgeArray<bool> *pForbid = master.forbid();
	const EdgeArray<uint32_t> *pEdgeSubGraphs = master.edgeSubGraphs();

	do {
		int crossingNumber;
		if(doSinglePermutation(prl, cc, pCost, pForbid, pEdgeSubGraphs, deletedEdges, inserter, rng, crossingNumber)
			&& crossingNumber < master.queryBestKnown())
		{
			CrossingStructure *pCS = new CrossingStructure;
			pCS->init(prl, crossingNumber);
			pCS = master.postNewResult(pCS);
			delete pCS;
		}

	} while(master.getNextPerm());
}


void SubgraphPlanarizer::Worker::operator()()
{
	minstd_rand rng(m_pMaster->rseed(11+7*m_id)); // different seeds per thread
	doWorkHelper(*m_pMaster, *m_pInserter, rng);
}


// default constructor
SubgraphPlanarizer::SubgraphPlanarizer()
{
	auto* s = new PlanarSubgraphFast<int>();
	s->runs(64);
	m_subgraph.reset(s);

	VariableEmbeddingInserter *pInserter = new VariableEmbeddingInserter();
	pInserter->removeReinsert(RemoveReinsertType::All);
	m_inserter.reset(pInserter);

	m_permutations = 1;
	m_setTimeout = true;

#ifdef OGDF_MEMORY_POOL_NTS
	m_maxThreads = 1u;
#else
	m_maxThreads = max(1u, Thread::hardware_concurrency());
#endif
}


// copy constructor
SubgraphPlanarizer::SubgraphPlanarizer(const SubgraphPlanarizer &planarizer)
	: CrossingMinimizationModule(planarizer), Logger()
{
	m_subgraph.reset(planarizer.m_subgraph->clone());
	m_inserter.reset(planarizer.m_inserter->clone());

	m_permutations = planarizer.m_permutations;
	m_setTimeout   = planarizer.m_setTimeout;
	m_maxThreads   = planarizer.m_maxThreads;
}


// clone method
CrossingMinimizationModule *SubgraphPlanarizer::clone() const {
	return new SubgraphPlanarizer(*this);
}


// assignment operator
SubgraphPlanarizer &SubgraphPlanarizer::operator=(const SubgraphPlanarizer &planarizer)
{
	m_timeLimit = planarizer.m_timeLimit;
	m_subgraph.reset(planarizer.m_subgraph->clone());
	m_inserter.reset(planarizer.m_inserter->clone());

	m_permutations = planarizer.m_permutations;
	m_setTimeout   = planarizer.m_setTimeout;
	m_maxThreads   = planarizer.m_maxThreads;

	return *this;
}


Module::ReturnType SubgraphPlanarizer::doCall(
	PlanRep &pr,
	int      cc,
	const EdgeArray<int>      *pCostOrig,
	const EdgeArray<bool>     *pForbiddenOrig,
	const EdgeArray<uint32_t> *pEdgeSubGraphs,
	int                       &crossingNumber)
{
	OGDF_ASSERT(m_permutations >= 1);
	crossingNumber = 0;

	PlanarSubgraphModule<int> &subgraph = *m_subgraph;
	EdgeInsertionModule  &inserter = *m_inserter;

	unsigned int nThreads = min(m_maxThreads, (unsigned int)m_permutations);

	int64_t startTime;
	System::usedRealTime(startTime);
	int64_t stopTime = m_timeLimit >= 0 ? startTime + int64_t(1000.0*m_timeLimit) : -1;

	//
	// Compute subgraph
	//
	if(m_setTimeout)
		subgraph.timeLimit(m_timeLimit);

	pr.initCC(cc);

	List<edge> delEdges;
	ReturnType retValue;

	if(pCostOrig) {
		EdgeArray<int> costPG(pr);
		for(edge e : pr.edges)
			costPG[e] = (*pCostOrig)[pr.original(e)];

		retValue = subgraph.call(pr, costPG, delEdges);
	} else
		retValue = subgraph.call(pr, delEdges);

	if(!isSolution(retValue))
		return retValue;

	const int m = delEdges.size();
	if(m == 0)
		return ReturnType::Optimal;  // graph is planar

	for(edge &eDel : delEdges)
		eDel = pr.original(eDel);

	//
	// Permutation phase
	//

	int seed = rand();
	minstd_rand rng(seed);

	if(nThreads > 1) {
		//
		// Parallel implementation
		//
		ThreadMaster master(
			pr, cc,
			pCostOrig, pForbiddenOrig, pEdgeSubGraphs,
			delEdges,
			seed,
			m_permutations - nThreads,
			stopTime);

		Array<Worker *>    worker(nThreads-1);
		Array<Thread> thread(nThreads-1);
		for(unsigned int i = 0; i < nThreads-1; ++i) {
			worker[i] = new Worker(i, &master, inserter.clone());
			thread[i] = Thread(*worker[i]);
		}

		doWorkHelper(master, inserter, rng);

		for(unsigned int i = 0; i < nThreads-1; ++i) {
			thread[i].join();
			delete worker[i];
		}

		master.restore(pr, crossingNumber);

	} else {
		//
		// Sequential implementation
		//
		PlanRepLight prl(pr);

		Array<edge> deletedEdges(m);
		int j = 0;
		for(edge eDel : delEdges)
			deletedEdges[j++] = eDel;

		bool foundSolution = false;
		CrossingStructure cs;
		for(int i = 1; i <= m_permutations; ++i)
		{
			int cr;
			bool ok = doSinglePermutation(prl, cc, pCostOrig, pForbiddenOrig, pEdgeSubGraphs, deletedEdges, inserter, rng, cr);

			if(ok && (!foundSolution || cr < cs.weightedCrossingNumber())) {
				foundSolution = true;
				cs.init(prl, cr);
			}

			if(stopTime >= 0 && System::realTime() >= stopTime) {
				if(!foundSolution)
					return ReturnType::TimeoutInfeasible; // not able to find a solution...
				break;
			}
		}

		cs.restore(pr,cc); // restore best solution in pr
		crossingNumber = cs.weightedCrossingNumber();

		OGDF_ASSERT(isPlanar(pr));
	}

	return ReturnType::Feasible;
}

}
