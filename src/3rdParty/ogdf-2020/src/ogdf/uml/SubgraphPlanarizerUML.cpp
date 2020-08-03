/** \file
 * \brief Implements class SubgraphPlanarizerUML.
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

#include <ogdf/uml/SubgraphPlanarizerUML.h>
#include <ogdf/uml/VariableEmbeddingInserterUML.h>
#include <ogdf/planarity/MaximalPlanarSubgraphSimple.h>
#include <ogdf/planarity/embedder/CrossingStructure.h>

#include <atomic>

using std::atomic;
using std::mutex;
using std::lock_guard;
using std::minstd_rand;

namespace ogdf {

using embedder::CrossingStructure;

class SubgraphPlanarizerUML::ThreadMaster {
	CrossingStructure *m_pCS;
	int                m_bestCR;

	const PlanRep     &m_pr;
	int                m_cc;

	const EdgeArray<int> *m_pCost;
	const List<edge>     &m_delEdges;

	int m_seed;
	atomic<int>      m_perms;
	int64_t          m_stopTime;
	mutex       m_mutex;

public:
	ThreadMaster(
		const PlanRep &pr,
		int cc,
		const EdgeArray<int>  *pCost,
		const List<edge> &delEdges,
		int seed,
		int perms,
		int64_t stopTime);

	~ThreadMaster() { delete m_pCS; }

	const PlanRep &planRep() const { return m_pr; }
	int currentCC() const { return m_cc; }

	const EdgeArray<int> *cost() const { return m_pCost; }
	const List<edge> &delEdges() const { return m_delEdges; }

	int rseed(long id) const { return (int)id * m_seed; }

	int queryBestKnown() const { return m_bestCR; }
	CrossingStructure *postNewResult(CrossingStructure *pCS);
	bool getNextPerm();

	void restore(PlanRep &PG, int &cr);
};


class SubgraphPlanarizerUML::Worker {

	int                  m_id;
	ThreadMaster *m_pMaster;
	UMLEdgeInsertionModule *m_pInserter;

public:
	Worker(int id, ThreadMaster *pMaster, UMLEdgeInsertionModule *pInserter) : m_id(id), m_pMaster(pMaster), m_pInserter(pInserter) { }
	~Worker() { delete m_pInserter; }

	void operator()();

private:
	Worker(const Worker &other) = delete;
	Worker &operator=(const Worker &other) = delete;
};


SubgraphPlanarizerUML::ThreadMaster::ThreadMaster(
	const PlanRep &pr,
	int cc,
	const EdgeArray<int>  *pCost,
	const List<edge> &delEdges,
	int seed,
	int perms,
	int64_t stopTime)
	:
	m_pCS(nullptr), m_bestCR(std::numeric_limits<int>::max()), m_pr(pr), m_cc(cc),
	m_pCost(pCost),
	m_delEdges(delEdges), m_seed(seed), m_perms(perms), m_stopTime(stopTime)
{ }


CrossingStructure *SubgraphPlanarizerUML::ThreadMaster::postNewResult(CrossingStructure *pCS)
{
	int newCR = pCS->numberOfCrossings();

	lock_guard<mutex> guard(m_mutex);

	if(newCR < m_bestCR) {
		std::swap(pCS, m_pCS);
		m_bestCR = newCR;
	}

	return pCS;
}


bool SubgraphPlanarizerUML::ThreadMaster::getNextPerm()
{
	if(m_stopTime >= 0 && System::realTime() >= m_stopTime)
		return false;

	return --m_perms >= 0;
}


void SubgraphPlanarizerUML::ThreadMaster::restore(PlanRep &PG, int &cr)
{
	m_pCS->restore(PG, m_cc);
	cr = m_bestCR;
}


bool SubgraphPlanarizerUML::doSinglePermutation(
	PlanRepLight &PG,
	int cc,
	const EdgeArray<int>  *pCost,
	Array<edge> &deletedEdges,
	UMLEdgeInsertionModule &inserter,
	minstd_rand &rng,
	int &crossingNumber)
{
	PG.initCC(cc);

	const int nG = PG.numberOfNodes();
	const int high = deletedEdges.high();

	for(int j = 0; j <= high; ++j)
		PG.delEdge(PG.copy(deletedEdges[j]));

	deletedEdges.permute(rng);

	ReturnType ret = inserter.callEx(PG, deletedEdges, pCost, nullptr);

	if(!isSolution(ret))
		return false; // no solution found, that's bad...

	if(pCost == nullptr)
		crossingNumber = PG.numberOfNodes() - nG;
	else {
		crossingNumber = 0;
		for(node n : PG.nodes) {
			if(PG.original(n) == nullptr) { // dummy found -> calc cost
				edge e1 = PG.original(n->firstAdj()->theEdge());
				edge e2 = PG.original(n->lastAdj()->theEdge());
				crossingNumber += (*pCost)[e1] * (*pCost)[e2];
			}
		}
	}

	return true;
}

void SubgraphPlanarizerUML::doWorkHelper(ThreadMaster &master, UMLEdgeInsertionModule &inserter, minstd_rand &rng)
{
	const List<edge> &delEdges = master.delEdges();

	const int m = delEdges.size();
	Array<edge> deletedEdges(m);
	int j = 0;
	for(ListConstIterator<edge> it = delEdges.begin(); it.valid(); ++it)
		deletedEdges[j++] = *it;

	PlanRepLight PG(master.planRep());
	int cc = master.currentCC();

	const EdgeArray<int>  *pCost = master.cost();

	do {
		int crossingNumber;
		if(doSinglePermutation(PG, cc, pCost, deletedEdges, inserter, rng, crossingNumber)
			&& crossingNumber < master.queryBestKnown())
		{
			CrossingStructure *pCS = new CrossingStructure;
			pCS->init(PG, crossingNumber);
			pCS = master.postNewResult(pCS);
			delete pCS;
		}

	} while(master.getNextPerm());
}


void SubgraphPlanarizerUML::Worker::operator()()
{
	minstd_rand rng(m_pMaster->rseed(11+7*m_id)); // different seeds per thread
	doWorkHelper(*m_pMaster, *m_pInserter, rng);
}


// default constructor
SubgraphPlanarizerUML::SubgraphPlanarizerUML()
{
	m_subgraph.reset(new MaximalPlanarSubgraphSimple<int>);
	m_inserter.reset(new VariableEmbeddingInserterUML);

	m_permutations = 1;
	m_setTimeout = true;

#ifdef OGDF_MEMORY_POOL_NTS
	m_maxThreads = 1;
#else
	m_maxThreads = System::numberOfProcessors();
#endif
}


// copy constructor
SubgraphPlanarizerUML::SubgraphPlanarizerUML(const SubgraphPlanarizerUML &planarizer)
	: UMLCrossingMinimizationModule(planarizer), Logger()
{
	m_subgraph.reset(planarizer.m_subgraph->clone());
	m_inserter.reset(planarizer.m_inserter->clone());

	m_permutations = planarizer.m_permutations;
	m_setTimeout   = planarizer.m_setTimeout;
	m_maxThreads   = planarizer.m_maxThreads;
}


// clone method
UMLCrossingMinimizationModule *SubgraphPlanarizerUML::clone() const {
	return new SubgraphPlanarizerUML(*this);
}


// assignment operator
SubgraphPlanarizerUML &SubgraphPlanarizerUML::operator=(const SubgraphPlanarizerUML &planarizer)
{
	m_timeLimit = planarizer.m_timeLimit;
	m_subgraph.reset(planarizer.m_subgraph->clone());
	m_inserter.reset(planarizer.m_inserter->clone());

	m_permutations = planarizer.m_permutations;
	m_setTimeout   = planarizer.m_setTimeout;
	m_maxThreads   = planarizer.m_maxThreads;

	return *this;
}


Module::ReturnType SubgraphPlanarizerUML::doCall(
	PlanRepUML           &pr,
	int                   cc,
	const EdgeArray<int> *pCostOrig,
	int                  &crossingNumber)
{
	OGDF_ASSERT(m_permutations >= 1);

	PlanarSubgraphModule<int>   &subgraph = *m_subgraph;
	UMLEdgeInsertionModule &inserter = *m_inserter;

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

	// gather generalization edges, which should all be in the planar subgraph
	List<edge> preferedEdges;
	for(edge e : pr.edges) {
		if (pr.typeOf(e) == Graph::EdgeType::generalization)
			preferedEdges.pushBack(e);
	}

	List<edge> delEdges;
	ReturnType retValue;

	if(pCostOrig) {
		EdgeArray<int> costPG(pr);
		for(edge e : pr.edges)
			costPG[e] = (*pCostOrig)[pr.original(e)];

		retValue = subgraph.call(pr, costPG, preferedEdges, delEdges);

	} else
		retValue = subgraph.call(pr, preferedEdges, delEdges);

	if(!isSolution(retValue))
		return retValue;

	const int m = delEdges.size();
	for(ListIterator<edge> it = delEdges.begin(); it.valid(); ++it)
		*it = pr.original(*it);

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
			pCostOrig,
			delEdges,
			seed,
			m_permutations - nThreads,
			stopTime);

		Array<Worker *> worker(nThreads-1);
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
		for(ListIterator<edge> it = delEdges.begin(); it.valid(); ++it)
			deletedEdges[j++] = *it;

		bool foundSolution = false;
		CrossingStructure cs;
		for(int i = 1; i <= m_permutations; ++i)
		{
			int cr;
			bool ok = doSinglePermutation(prl, cc, pCostOrig, deletedEdges, inserter, rng, cr);

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

		cs.restore(pr,cc); // restore best solution in PG
		crossingNumber = cs.weightedCrossingNumber();

		OGDF_ASSERT(isPlanar(pr));
	}

	return ReturnType::Feasible;
}

}
