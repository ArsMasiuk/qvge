/** \file
 * \brief Implementation of Sugiyama algorithm (classes Hierarchy,
 * Level, SugiyamaLayout)
 *
 * \author Carsten Gutwenger
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


#include <ogdf/layered/Hierarchy.h>
#include <ogdf/layered/SugiyamaLayout.h>
#include <ogdf/layered/LongestPathRanking.h>
#include <ogdf/layered/BarycenterHeuristic.h>
#include <ogdf/layered/SplitHeuristic.h>
#include <ogdf/layered/FastHierarchyLayout.h>
#include <ogdf/layered/OptimalHierarchyClusterLayout.h>
#include <ogdf/packing/TileToRowsCCPacker.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Thread.h>

#include <atomic>

using std::atomic;
using std::mutex;
using std::lock_guard;
using std::minstd_rand;

namespace ogdf {

void ClusterGraphCopyAttributes::transform()
{
	for(node v : m_pH->nodes)
	{
		node vG = m_pH->origNode(v);
		if(vG) {
			m_pACG->x(vG) = m_x[v];
			m_pACG->y(vG) = m_y[v];
		}
	}

	for(edge e : m_pH->edges)
	{
		edge eG = m_pH->origEdge(e);
		if(eG == nullptr || e != m_pH->chain(eG).front())
			continue;
		// current implementation does not layout self-loops;
		// they are simply ignored
#if 0
		if (e->isSelfLoop()) continue;
#endif

		DPolyline &dpl = m_pACG->bends(eG);
		dpl.clear();

		ListConstIterator<edge> itE = m_pH->chain(eG).begin();
		node v      = (*itE)->source();
		node vAfter = (*itE)->target();

		for (++itE; itE.valid(); ++itE)
		{
			node vBefore = v;
			v      = vAfter;
			vAfter = (*itE)->target();

			// filter real bend points
			if((m_x[v] != m_x[vBefore] || m_x[v] != m_x[vAfter]) &&
				(m_y[v] != m_y[vBefore] || m_y[v] != m_y[vAfter]))
				dpl.pushBack(DPoint(m_x[v],m_y[v]));
		}

		if (m_pH->isReversed(eG))
			dpl.reverse();
	}
}


const Array<node> &Level::adjNodes(node v) const
{
	return m_pLevels->adjNodes(v);
}


void Level::swap(int i, int j)
{
	m_nodes.swap(i,j);
	m_pLevels->m_pos[m_nodes[i]] = i;
	m_pLevels->m_pos[m_nodes[j]] = j;
}


void Level::recalcPos()
{
	NodeArray<int> &pos = m_pLevels->m_pos;

	for(int i = 0; i <= high(); ++i)
		pos[m_nodes[i]] = i;

	m_pLevels->buildAdjNodes(m_index);
}


void Level::getIsolatedNodes(SListPure<Tuple2<node,int> > &isolated) const
{
	for (int i = 0; i <= high(); ++i)
		if (adjNodes(m_nodes[i]).high() < 0)
			isolated.pushBack(Tuple2<node,int>(m_nodes[i],i));
}


void Level::setIsolatedNodes(SListPure<Tuple2<node,int> > &isolated)
{
	const int sizeL = size();
	Array<node> sortedNodes(sizeL);
	isolated.pushBack(Tuple2<node,int>(nullptr,sizeL));
	SListConstIterator<Tuple2<node,int> > itIsolated = isolated.begin();

	int nextPos = (*itIsolated).x2();
	for( int iNodes = 0, iSortedNodes = 0; nextPos <= sizeL; ) {
		if( iSortedNodes == nextPos ) {
			if( iSortedNodes == sizeL )
				break;
			sortedNodes[iSortedNodes++] = (*itIsolated).x1();
			nextPos = (*(++itIsolated)).x2();
		} else {
			node v = m_nodes[iNodes++];
			if( adjNodes(v).size() > 0 )
				sortedNodes[iSortedNodes++] = v;
		}
	}

	for( int i = 0; i < sizeL; ++i)
		m_nodes[i] = sortedNodes[i];
}


class WeightBucket : public BucketFunc<node> {
	const NodeArray<int> *m_pWeight;

public:
	explicit WeightBucket(const NodeArray<int> *pWeight) : m_pWeight(pWeight) { }

	int getBucket(const node &v) override { return (*m_pWeight)[v]; }
};


void Level::sort(NodeArray<double> &weight)
{
	SListPure<Tuple2<node,int> > isolated;
	getIsolatedNodes(isolated);

	WeightComparer<> cmp(&weight);
	std::stable_sort(&m_nodes[0], &m_nodes[0]+m_nodes.size(), cmp);

	if (!isolated.empty()) setIsolatedNodes(isolated);
	recalcPos();
}


void Level::sortByWeightOnly(NodeArray<double> &weight)
{
	WeightComparer<> cmp(&weight);
	std::stable_sort(&m_nodes[0], &m_nodes[0]+m_nodes.size(), cmp);
	recalcPos();
}


void Level::sort(NodeArray<int> &weight, int minBucket, int maxBucket)
{
	SListPure<Tuple2<node,int> > isolated;
	getIsolatedNodes(isolated);

	WeightBucket bucketFunc(&weight);
	bucketSort(m_nodes,minBucket,maxBucket,bucketFunc);

	if (!isolated.empty()) setIsolatedNodes(isolated);
	recalcPos();
}



Hierarchy::Hierarchy(const Graph &G, const NodeArray<int> &rank) :
	m_GC(G), m_rank(m_GC)
{
	doInit(rank);
}


void Hierarchy::createEmpty(const Graph &G)
{
	m_GC.createEmpty(G);
	m_rank.init(m_GC);
}


void Hierarchy::initByNodes(const List<node> &nodes,
	EdgeArray<edge> &eCopy,
	const NodeArray<int> &rank)
{
	m_GC.initByNodes(nodes,eCopy);

	doInit(rank);
}


void Hierarchy::doInit(const NodeArray<int> &rank)
{
	makeLoopFree(m_GC);

	int maxRank = 0;

	for(node v : m_GC.nodes) {
		int r = m_rank[v] = rank[m_GC.original(v)];
		OGDF_ASSERT(r >= 0);
		if (r > maxRank) maxRank = r;
	}

	SListPure<edge> edges;
	m_GC.allEdges(edges);
	for(edge e : edges)
	{
		int rankSrc = m_rank[e->source()], rankTgt = m_rank[e->target()];

		if (rankSrc > rankTgt) {
			m_GC.reverseEdge(e); std::swap(rankSrc,rankTgt);
		}

		if (rankSrc == rankTgt) {
			e = m_GC.split(e);
			m_GC.reverseEdge(e);
			if ((m_rank[e->target()] = rankSrc+1) > maxRank)
				maxRank = rankSrc+1;

		} else {
			for(++rankSrc; rankSrc < rankTgt; ++rankSrc)
				m_rank[(e = m_GC.split(e))->source()] = rankSrc;
		}
	}

	m_size.init(0,maxRank,0);
	for(node v : m_GC.nodes)
		m_size[m_rank[v]]++;
}


HierarchyLevels::HierarchyLevels(const Hierarchy &H) : m_H(H), m_pLevel(0,H.maxRank()), m_pos(H), m_lowerAdjNodes(H), m_upperAdjNodes(H), m_nSet(H,0)
{
	const GraphCopy &GC = m_H;
	int maxRank = H.maxRank();

	for(int i = 0; i <= maxRank; ++i)
		m_pLevel[i] = new Level(this,i,H.size(i));

	Array<int> next(0,maxRank,0);

	for(node v : GC.nodes) {
		int r = H.rank(v), pos = next[r]++;
		m_pos[(*m_pLevel[r])[pos] = v] = pos;

		m_lowerAdjNodes[v].init(v->indeg());
		m_upperAdjNodes[v].init(v->outdeg());
	}

	buildAdjNodes();
}


HierarchyLevels::~HierarchyLevels()
{
	for(int i = 0; i <= high(); ++i)
		delete m_pLevel[i];
}


void HierarchyLevels::buildAdjNodes()
{
	for(int i = 0; i <= high(); ++i)
		buildAdjNodes(i);
}


void HierarchyLevels::buildAdjNodes(int i)
{
	if (i > 0) {
		const Level &lowerLevel = *m_pLevel[i-1];

		for(int j = 0; j <= lowerLevel.high(); ++j)
			m_nSet[lowerLevel[j]] = 0;
	}

	if (i < high()) {
		const Level &upperLevel = *m_pLevel[i+1];

		for(int j = 0; j <= upperLevel.high(); ++j)
			m_nSet[upperLevel[j]] = 0;
	}

	const Level &level = *m_pLevel[i];

	for(int j = 0; j <= level.high(); ++j) {
		node v = level[j];
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			if (e->source() == v) {
				(m_lowerAdjNodes[e->target()])[m_nSet[e->target()]++] = v;
			} else {
				(m_upperAdjNodes[e->source()])[m_nSet[e->source()]++] = v;
			}
		}
	}
}


void HierarchyLevels::storePos (NodeArray<int> &oldPos) const
{
	oldPos = m_pos;
}


void HierarchyLevels::restorePos (const NodeArray<int> &newPos)
{
	const GraphCopy &GC = m_H;

	m_pos = newPos;

	for(node v : GC.nodes) {
		(*m_pLevel[m_H.rank(v)])[m_pos[v]] = v;
	}

#if 0
	check();
#endif

	buildAdjNodes();
}


void HierarchyLevels::permute()
{
	for(int i = 0; i < m_pLevel.high(); ++i) {
		Level &level = *m_pLevel[i];
		level.m_nodes.permute();
		for(int j = 0; j <= level.high(); ++j)
			m_pos[level[j]] = j;
	}

#if 0
	check();
#endif

	buildAdjNodes();
}


void HierarchyLevels::separateCCs(int numCC, const NodeArray<int> &component)
{
	Array<SListPure<node> > table(numCC);

	for(int i = 0; i < m_pLevel.high(); ++i) {
		Level &level = *m_pLevel[i];
		for(int j = 0; j <= level.high(); ++j) {
			node v = level[j];
			table[component[v]].pushBack(v);
		}
	}

	Array<int> count(0, m_pLevel.high(), 0);
	for(int c = 0; c < numCC; ++c) {
		for(node v : table[c])
			m_pos[v] = count[m_H.rank(v)]++;
	}

	const GraphCopy &GC = m_H;

	for(node v : GC.nodes) {
		(*m_pLevel[m_H.rank(v)])[m_pos[v]] = v;
	}

#if 0
	check();
#endif

	buildAdjNodes();
}


int HierarchyLevels::calculateCrossingsSimDraw(const EdgeArray<uint32_t> *edgeSubGraphs) const
{
	int nCrossings = 0;

	for(int i = 0; i < m_pLevel.high(); ++i) {
		nCrossings += calculateCrossingsSimDraw(i, edgeSubGraphs);
	}

	return nCrossings;
}


// naive calculation of edge crossings between level i and i+1
// for SimDraw-calculation by Michael Schulz

int HierarchyLevels::calculateCrossingsSimDraw(int i, const EdgeArray<uint32_t> *edgeSubGraphs) const
{
	const int maxGraphs = 32;

	const Level &level = *m_pLevel[i];             // level i
	const GraphCopy &GC = m_H;

	int nc = 0; // number of crossings

	for(int j = 0; j < level.size(); ++j)
	{
		node v = level[j];
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			if (e->source() == v){
				int pos_adj_e = pos(e->target());
				for (int k = j+1; k < level.size(); k++) {
					node w = level[k];
					for(adjEntry adjW : w->adjEntries) {
						edge f = adjW->theEdge();
						if (f->source() == w) {
							int pos_adj_f = pos(f->target());
							if(pos_adj_f < pos_adj_e)
							{
								int graphCounter = 0;
								for(int numGraphs = 0; numGraphs < maxGraphs; numGraphs++)
									if((1 << numGraphs) & (*edgeSubGraphs)[GC.original(e)] & (*edgeSubGraphs)[GC.original(f)])
										graphCounter++;
								nc += graphCounter;
							}
						}
					}
				}
			}
		}
	}

	return nc;
}


int HierarchyLevels::transposePart(
	const Array<node> &adjV,
	const Array<node> &adjW)
{
	const int vSize = adjV.size();
	int iW = 0, iV = 0, sum = 0;

	for(; iW <= adjW.high(); ++iW) {
		int p = m_pos[adjW[iW]];
		while(iV < vSize && m_pos[adjV[iV]] <= p) ++iV;
		sum += vSize - iV;
	}

	return sum;
}


bool HierarchyLevels::transpose(node v)
{
	int rankV = m_H.rank(v), posV = m_pos[v];
	node w = (*m_pLevel[rankV])[posV+1];

	int d = 0;
	d += transposePart(m_upperAdjNodes[v],m_upperAdjNodes[w]);
	d -= transposePart(m_upperAdjNodes[w],m_upperAdjNodes[v]);
	d += transposePart(m_lowerAdjNodes[v],m_lowerAdjNodes[w]);
	d -= transposePart(m_lowerAdjNodes[w],m_lowerAdjNodes[v]);

	if (d > 0) {
		m_pLevel[rankV]->swap(posV,posV+1);
		return true;
	}

	return false;
}


void HierarchyLevels::print(std::ostream &os) const
{
	for(int i = 0; i <= m_pLevel.high(); ++i) {
		os << i << ": ";
		const Level &level = *m_pLevel[i];
		for(int j = 0; j < level.size(); ++j)
			os << level[j] << " ";
		os << std::endl;
	}

	os << std::endl;

	const GraphCopy &GC = m_H;

	for(node v : GC.nodes) {
		os << v << ": lower: " << (m_lowerAdjNodes[v]) <<
			", upper: " << (m_upperAdjNodes[v]) << std::endl;
	}

}


void HierarchyLevels::check() const
{
	int i, j;
	for(i = 0; i <= high(); ++i) {
		Level &level = *m_pLevel[i];
		for(j = 0; j <= level.high(); ++j) {
			if (m_pos[level[j]] != j) {
				std::cerr << "m_pos[" << level[j] << "] wrong!" << std::endl;
			}
			if (m_H.rank(level[j]) != i) {
				std::cerr << "m_rank[" << level[j] << "] wrong!" << std::endl;
			}
		}
	}
}


class LayerByLayerSweep::CrossMinMaster {

	NodeArray<int>  *m_pBestPos;
	int              m_bestCR;

	const SugiyamaLayout &m_sugi;
	const Hierarchy      &m_H;

	atomic<int>  m_runs;
	mutex        m_mutex;

public:
	CrossMinMaster(
		const SugiyamaLayout &sugi,
		const Hierarchy &H,
		int runs);

	const Hierarchy &hierarchy() const { return m_H; }

	void restore(HierarchyLevels &levels, int &cr);

	void doWorkHelper(
		LayerByLayerSweep        *pCrossMin,
		TwoLayerCrossMinSimDraw *pCrossMinSimDraw,
		HierarchyLevels         &levels,
		NodeArray<int>          &bestPos,
		bool                     permuteFirst,
		std::minstd_rand        &rng);

private:
	const EdgeArray<uint32_t> *subgraphs() const { return m_sugi.subgraphs(); }
	int fails() const { return m_sugi.fails(); }
	bool transpose() { return m_sugi.transpose(); }

	bool arrangeCCs() const { return m_sugi.arrangeCCs(); }
	int arrange_numCC() const { return m_sugi.numCC(); }
	const NodeArray<int> &arrange_compGC() const { return m_sugi.compGC(); }

	bool transposeLevel(int i, HierarchyLevels &levels, Array<bool> &levelChanged);
	void doTranspose(HierarchyLevels &levels, Array<bool> &levelChanged);
	void doTransposeRev(HierarchyLevels &levels, Array<bool> &levelChanged);

	int traverseTopDown(
		HierarchyLevels &levels,
		LayerByLayerSweep *pCrossMin,
		TwoLayerCrossMinSimDraw *pCrossMinSimDraw,
		Array<bool>             *pLevelChanged);

	int traverseBottomUp(
		HierarchyLevels &levels,
		LayerByLayerSweep *pCrossMin,
		TwoLayerCrossMinSimDraw *pCrossMinSimDraw,
		Array<bool>             *pLevelChanged);

	int queryBestKnown() const { return m_bestCR; }
	bool postNewResult(int cr, NodeArray<int> *pPos);
	bool getNextRun();
};



LayerByLayerSweep::CrossMinMaster::CrossMinMaster(
	const SugiyamaLayout &sugi,
	const Hierarchy &H,
	int runs)
	: m_pBestPos(nullptr), m_bestCR(std::numeric_limits<int>::max()), m_sugi(sugi), m_H(H), m_runs(runs) { }


bool LayerByLayerSweep::CrossMinMaster::postNewResult(int cr, NodeArray<int> *pPos)
{
	bool storeResult = false;

	lock_guard<mutex> guard(m_mutex);

	if(cr < m_bestCR) {
		m_bestCR = cr;
		m_pBestPos = pPos;
		storeResult = true;

		if(cr == 0)
			m_runs = 0;
	}

	return storeResult;
}


bool LayerByLayerSweep::CrossMinMaster::getNextRun()
{
	return --m_runs >= 0;
}


void LayerByLayerSweep::CrossMinMaster::restore(HierarchyLevels &levels, int &cr)
{
	levels.restorePos(*m_pBestPos);
	cr = m_bestCR;
}


bool LayerByLayerSweep::CrossMinMaster::transposeLevel(int i, HierarchyLevels &levels, Array<bool> &levelChanged)
{
	bool improved = false;

	if (levelChanged[i] || levelChanged[i-1] || levelChanged[i+1]) {
		Level &level = levels[i];

		for (int j = 0; j < level.high(); j++) {
			if (levels.transpose(level[j])) improved = true;
		}
	}

	if (improved) levels.buildAdjNodes(i);
	return (levelChanged[i] = improved);
}


void LayerByLayerSweep::CrossMinMaster::doTranspose(HierarchyLevels &levels, Array<bool> &levelChanged)
{
	levelChanged.fill(true);

	bool improved;
	do {
		improved = false;

		for (int i = 0; i <= levels.high(); ++i)
			improved |= transposeLevel(i,levels,levelChanged);
	} while (improved);
}


void LayerByLayerSweep::CrossMinMaster::doTransposeRev(HierarchyLevels &levels, Array<bool> &levelChanged)
{
	levelChanged.fill(true);

	bool improved;
	do {
		improved = false;

		for (int i = levels.high(); i >= 0 ; --i)
			improved |= transposeLevel(i,levels,levelChanged);
	} while (improved);
}


int LayerByLayerSweep::CrossMinMaster::traverseTopDown(
	HierarchyLevels           &levels,
	LayerByLayerSweep          *pCrossMin,
	TwoLayerCrossMinSimDraw   *pCrossMinSimDraw,
	Array<bool>               *pLevelChanged)
{
	levels.direction(HierarchyLevels::TraversingDir::downward);

	for (int i = 1; i <= levels.high(); ++i) {
		if(pCrossMin != nullptr)
			pCrossMin->call(levels[i]);
		else
			pCrossMinSimDraw->call(levels[i], subgraphs());
	}

	if(pLevelChanged != nullptr)
		doTranspose(levels, *pLevelChanged);
	if(!arrangeCCs())
		levels.separateCCs(arrange_numCC(), arrange_compGC());

	return (pCrossMin != nullptr) ? levels.calculateCrossings() : levels.calculateCrossingsSimDraw(subgraphs());
}


int LayerByLayerSweep::CrossMinMaster::traverseBottomUp(
	HierarchyLevels           &levels,
	LayerByLayerSweep          *pCrossMin,
	TwoLayerCrossMinSimDraw   *pCrossMinSimDraw,
	Array<bool>               *pLevelChanged)
{
	levels.direction(HierarchyLevels::TraversingDir::upward);

	for (int i = levels.high()-1; i >= 0; i--) {
		if(pCrossMin != nullptr)
			pCrossMin->call(levels[i]);
		else
			pCrossMinSimDraw->call(levels[i], subgraphs());
	}

	if (pLevelChanged != nullptr)
		doTransposeRev(levels, *pLevelChanged);
	if (!arrangeCCs())
		levels.separateCCs(arrange_numCC(), arrange_compGC());

	return (pCrossMin != nullptr) ? levels.calculateCrossings() : levels.calculateCrossingsSimDraw(subgraphs());
}


void LayerByLayerSweep::CrossMinMaster::doWorkHelper(
	LayerByLayerSweep        *pCrossMin,
	TwoLayerCrossMinSimDraw *pCrossMinSimDraw,
	HierarchyLevels         &levels,
	NodeArray<int>          &bestPos,
	bool                     permuteFirst,
	minstd_rand             &rng)
{
	if(permuteFirst)
		levels.permute(rng);

	int nCrossingsOld = (pCrossMin != nullptr) ? levels.calculateCrossings() : levels.calculateCrossingsSimDraw(subgraphs());
	if(postNewResult(nCrossingsOld, &bestPos))
		levels.storePos(bestPos);

	if(queryBestKnown() == 0)
		return;

	if(pCrossMin != nullptr)
		pCrossMin->init(levels);
	else
		pCrossMinSimDraw->init(levels);

	Array<bool> *pLevelChanged = nullptr;
	if(transpose()) {
		pLevelChanged = new Array<bool>(-1,levels.size());
		(*pLevelChanged)[-1] = (*pLevelChanged)[levels.size()] = false;
	}

	int maxFails = fails();
	for( ; ; ) {

		int nFails = maxFails+1;
		do {

			// top-down traversal
			int nCrossingsNew = traverseTopDown(levels, pCrossMin, pCrossMinSimDraw, pLevelChanged);
			if(nCrossingsNew < nCrossingsOld) {
				if(nCrossingsNew < queryBestKnown() && postNewResult(nCrossingsNew, &bestPos))
					levels.storePos(bestPos);

				nCrossingsOld = nCrossingsNew;
				nFails = maxFails+1;
			} else
				--nFails;

			// bottom-up traversal
			nCrossingsNew = traverseBottomUp(levels, pCrossMin, pCrossMinSimDraw, pLevelChanged);
			if(nCrossingsNew < nCrossingsOld) {
				if(nCrossingsNew < queryBestKnown() && postNewResult(nCrossingsNew, &bestPos))
					levels.storePos(bestPos);

				nCrossingsOld = nCrossingsNew;
				nFails = maxFails+1;
			} else
				--nFails;

		} while(nFails > 0);

		if(!getNextRun())
			break;

		levels.permute(rng);

		nCrossingsOld = (pCrossMin != nullptr) ? levels.calculateCrossings() : levels.calculateCrossingsSimDraw(subgraphs());
		if(nCrossingsOld < queryBestKnown() && postNewResult(nCrossingsOld, &bestPos))
			levels.storePos(bestPos);
	}

	delete pLevelChanged;

	if(pCrossMin != nullptr)
		pCrossMin->cleanup();
	else
		pCrossMinSimDraw->cleanup();
}


// LayerByLayerSweep::CrossMinWorker

class LayerByLayerSweep::CrossMinWorker : public Thread {

	LayerByLayerSweep::CrossMinMaster &m_master;
	LayerByLayerSweep        *m_pCrossMin;
	TwoLayerCrossMinSimDraw *m_pCrossMinSimDraw;

	NodeArray<int>   m_bestPos;

public:
	CrossMinWorker(LayerByLayerSweep::CrossMinMaster &master, LayerByLayerSweep *pCrossMin, TwoLayerCrossMinSimDraw *pCrossMinSimDraw)
		: m_master(master), m_pCrossMin(pCrossMin), m_pCrossMinSimDraw(pCrossMinSimDraw)
	{
		OGDF_ASSERT( (pCrossMin != nullptr && pCrossMinSimDraw == nullptr) || (pCrossMin == nullptr && pCrossMinSimDraw != nullptr));
	}

	~CrossMinWorker() { delete m_pCrossMin; }

	void operator()();

private:
	CrossMinWorker(const CrossMinWorker &); // = delete
	CrossMinWorker &operator=(const CrossMinWorker &); // = delete
};

void LayerByLayerSweep::CrossMinWorker::operator()()
{
	HierarchyLevels levels(m_master.hierarchy());

	minstd_rand rng(randomSeed()); // different seeds per worker
	m_master.doWorkHelper(m_pCrossMin, m_pCrossMinSimDraw, levels, m_bestPos, true, rng);
}


SugiyamaLayout::SugiyamaLayout()
{
	m_ranking        .reset(new LongestPathRanking);
	m_crossMin       .reset(new BarycenterHeuristic);
	m_crossMinSimDraw.reset(new SplitHeuristic);
	m_layout         .reset(new FastHierarchyLayout);
	m_clusterLayout  .reset(new OptimalHierarchyClusterLayout);
	m_packer         .reset(new TileToRowsCCPacker);

	m_fails = 4;
	m_runs = 15;
	m_transpose = true;
	m_permuteFirst = false;

	m_arrangeCCs = true;
	m_minDistCC = LayoutStandards::defaultCCSeparation();
	m_pageRatio = 1.0;

#ifdef OGDF_MEMORY_POOL_NTS
	m_maxThreads = 1u;
#else
	m_maxThreads = max(1u, Thread::hardware_concurrency());
#endif

	m_alignBaseClasses = false;
	m_alignSiblings = false;

	m_subgraphs = nullptr;

	m_maxLevelSize = -1;
	m_numLevels = -1;
	m_timeReduceCrossings = 0.0;
}


void SugiyamaLayout::call(GraphAttributes &AG)
{
	doCall(AG,false);
}


void SugiyamaLayout::call(GraphAttributes &AG, NodeArray<int> &rank)
{
	doCall(AG,false,rank);
}


void SugiyamaLayout::doCall(GraphAttributes &AG, bool umlCall)
{
	NodeArray<int> rank;
	doCall(AG, umlCall, rank);
}


void SugiyamaLayout::doCall(GraphAttributes &AG, bool umlCall, NodeArray<int> &rank)
{
	const Graph &G = AG.constGraph();
	if (G.numberOfNodes() == 0)
		return;

	// compute connected component of G
	NodeArray<int> component(G);
	m_numCC = connectedComponents(G,component);

	const bool optimizeHorizEdges = (umlCall || rank.valid());
	if(!rank.valid())
	{
		if(umlCall)
		{
			LongestPathRanking ranking;
			ranking.alignBaseClasses(m_alignBaseClasses);
			ranking.alignSiblings(m_alignSiblings);

			ranking.callUML(AG,rank);

		} else {
			m_ranking->call(AG.constGraph(),rank);
		}
	}

	if(m_arrangeCCs) {
		// intialize the array of lists of nodes contained in a CC
		Array<List<node> > nodesInCC(m_numCC);

		for(node v : G.nodes)
			nodesInCC[component[v]].pushBack(v);

		Hierarchy H;
		H.createEmpty(G);

		EdgeArray<edge> auxCopy(G);
		Array<DPoint> boundingBox(m_numCC);
		Array<DPoint> offset1(m_numCC);

		m_numLevels = m_maxLevelSize = 0;

		int totalCrossings = 0;
		for(int i = 0; i < m_numCC; ++i)
		{
			// adjust ranks in cc to start with 0
			int minRank = std::numeric_limits<int>::max();
			for(node v : nodesInCC[i])
				if(rank[v] < minRank)
					minRank = rank[v];

			if(minRank != 0) {
				for(node v : nodesInCC[i])
					rank[v] -= minRank;
			}
			H.createEmpty(G);
			H.initByNodes(nodesInCC[i],auxCopy,rank);
			//HierarchyLevels levels(H);
			//reduceCrossings(levels);
			const HierarchyLevelsBase *pLevels = reduceCrossings(H);
			const HierarchyLevelsBase &levels = *pLevels;
			totalCrossings += m_nCrossings;

			const GraphCopy &GC = H;
			NodeArray<bool> mark(GC);

			m_layout->call(levels,AG);

			double
				minX = std::numeric_limits<double>::max(),
				maxX = std::numeric_limits<double>::lowest(),
				minY = std::numeric_limits<double>::max(),
				maxY = std::numeric_limits<double>::lowest();

			for(node vCopy : GC.nodes)
			{
				mark[vCopy] = false;
				node v = GC.original(vCopy);
				if(v == nullptr) continue;

				if(AG.x(v)-AG.width (v)/2 < minX) minX = AG.x(v)-AG.width(v) /2;
				if(AG.x(v)+AG.width (v)/2 > maxX) maxX = AG.x(v)+AG.width(v) /2;
				if(AG.y(v)-AG.height(v)/2 < minY) minY = AG.y(v)-AG.height(v)/2;
				if(AG.y(v)+AG.height(v)/2 > maxY) maxY = AG.y(v)+AG.height(v)/2;
			}

			if(optimizeHorizEdges)
			{
				for(int k = 0; k < levels.size(); ++k) {
					const LevelBase &level = levels[k];
					for(int j = 0; j < level.size(); ++j) {
						node v = level[j];
						if(!GC.isDummy(v)) continue;
						edge e = GC.original(v->firstAdj()->theEdge());
						if(e == nullptr) continue;
						node src = GC.copy(e->source());
						node tgt = GC.copy(e->target());

						if(H.rank(src) == H.rank(tgt)) {
							int minPos = levels.pos(src), maxPos = levels.pos(tgt);
							if(minPos > maxPos) std::swap(minPos,maxPos);

							bool straight = true;
							const LevelBase &L_e = levels[H.rank(src)];
							for(int p = minPos+1; p < maxPos; ++p) {
								if(!H.isLongEdgeDummy(L_e[p]) && !mark[L_e[p]]) {
									straight = false;
									break;
								}
							}
							if(straight) {
								AG.bends(e).clear();
								mark[v] = true;
							}
						}
					}
				}
			}

			for(edge eCopy : GC.edges)
			{
				edge e = GC.original(eCopy);
				if(e == nullptr || eCopy != GC.chain(e).front()) continue;

				const DPolyline &dpl = AG.bends(e);
				for(const DPoint &dp : dpl)
				{
					if(dp.m_x < minX) minX = dp.m_x;
					if(dp.m_x > maxX) maxX = dp.m_x;
					if(dp.m_y < minY) minY = dp.m_y;
					if(dp.m_y > maxY) maxY = dp.m_y;
				}
			}

			minX -= m_minDistCC;
			minY -= m_minDistCC;

			boundingBox[i] = DPoint(maxX - minX, maxY - minY);
			offset1    [i] = DPoint(minX,minY);

			Math::updateMax(m_numLevels, levels.size());
			for(int iter = 0; iter <= levels.high(); iter++) {
				const LevelBase &level = levels[iter];
				Math::updateMax(m_maxLevelSize, level.size());
			}
			delete pLevels;
		}

		m_nCrossings = totalCrossings;

		// call packer
		Array<DPoint> offset(m_numCC);
		m_packer->call(boundingBox,offset,m_pageRatio);

		// The arrangement is given by offset to the origin of the coordinate
		// system. We still have to shift each node and edge by the offset
		// of its connected component.

		for(int i = 0; i < m_numCC; ++i)
		{
			const List<node> &nodes = nodesInCC[i];

			const double dx = offset[i].m_x - offset1[i].m_x;
			const double dy = offset[i].m_y - offset1[i].m_y;

			// iterate over all nodes in ith CC
			for(node v : nodes)
			{
				AG.x(v) += dx;
				AG.y(v) += dy;

				for(adjEntry adj : v->adjEntries) {
					edge e = adj->theEdge();
					if(e->isSelfLoop() || e->source() != v) continue;

					DPolyline &dpl = AG.bends(e);
					for(DPoint &dp : dpl)
					{
						dp.m_x += dx;
						dp.m_y += dy;
					}
				}
			}
		}

	} else {
		int minRank = std::numeric_limits<int>::max();
		for(node v : G.nodes)
			if(rank[v] < minRank)
				minRank = rank[v];

		if(minRank != 0) {
			for(node v : G.nodes)
				rank[v] -= minRank;
		}

		Hierarchy H(G,rank);

		{  // GC's scope is limited to allow reassignment after crossing reduction phase
			const GraphCopy &GC = H;

			m_compGC.init(GC);
			for(node v : GC.nodes) {
				node vOrig = GC.original(v);
				if(vOrig == nullptr)
					vOrig = GC.original(v->firstAdj()->theEdge())->source();

				m_compGC[v] = component[vOrig];
			}
		}

		const HierarchyLevelsBase *pLevels = reduceCrossings(H);
		const HierarchyLevelsBase &levels = *pLevels;
		//HierarchyLevels levels(H);
		//reduceCrossings(levels);
		m_compGC.init();

		const GraphCopy &GC = H;

		m_layout->call(levels,AG);

		if(optimizeHorizEdges)
		{
			NodeArray<bool> mark(GC,false);
			for(int i = 0; i < levels.size(); ++i) {
				const LevelBase &level = levels[i];
				for(int j = 0; j < level.size(); ++j) {
					node v = level[j];
					if(!GC.isDummy(v)) continue;
					edge e = GC.original(v->firstAdj()->theEdge());
					if(e == nullptr) continue;
					node src = GC.copy(e->source());
					node tgt = GC.copy(e->target());

					if(H.rank(src) == H.rank(tgt)) {
						int minPos = levels.pos(src), maxPos = levels.pos(tgt);
						if(minPos > maxPos) std::swap(minPos,maxPos);

						bool straight = true;
						const LevelBase &L_e = levels[H.rank(src)];
						for(int p = minPos+1; p < maxPos; ++p) {
							if(!H.isLongEdgeDummy(L_e[p]) && !mark[L_e[p]]) {
								straight = false;
								break;
							}
						}
						if(straight) {
							AG.bends(e).clear();
							mark[v] = true;
						}
					}
				}
			}
		}

		m_numLevels = levels.size();
		m_maxLevelSize = 0;
		for(int i = 0; i <= levels.high(); i++) {
			const LevelBase &level = levels[i];
			if (level.size() > m_maxLevelSize)
				m_maxLevelSize = level.size();
		}
		delete pLevels;
	}

	for(edge e : G.edges) {
		AG.bends(e).normalize();
	}
}


void SugiyamaLayout::callUML(GraphAttributes &AG)
{
	doCall(AG,true);
}

#if 0
void SugiyamaLayout::reduceCrossings(HierarchyLevels &levels)
{
	OGDF_ASSERT(m_runs >= 1);


	int64_t t;
	System::usedRealTime(t);

	LayerByLayerSweep          *pCrossMin = 0;
	TwoLayerCrossMinSimDraw   *pCrossMinSimDraw = 0;

	if(!useSubgraphs())
		pCrossMin = &m_crossMin.get();
	else
		pCrossMinSimDraw = &m_crossMinSimDraw.get();

	unsigned int nThreads = min(m_maxThreads, (unsigned int)m_runs);

	minstd_rand rng(randomSeed());

	LayerByLayerSweep::CrossMinMaster master(*this, levels.hierarchy(), seed, m_runs - nThreads);

	Array<LayerByLayerSweep::CrossMinWorker *> thread(nThreads-1);
	for(int i = 0; i < nThreads-1; ++i) {
		thread[i] = new LayerByLayerSweep::CrossMinWorker(master,
			(pCrossMin        != 0) ? pCrossMin       ->clone() : 0,
			(pCrossMinSimDraw != 0) ? pCrossMinSimDraw->clone() : 0);
		thread[i] = Thread(*worker[i]);
	}

	NodeArray<int> bestPos;
	master.doWorkHelper(pCrossMin, pCrossMinSimDraw, levels, bestPos, m_permuteFirst, rng);

	for(unsigned int i = 0; i < nThreads-1; ++i)
		thread[i].join();

	master.restore(levels, m_nCrossings);

	for(unsigned int i = 0; i < nThreads-1; ++i)
		delete worker[i];

	t = System::usedRealTime(t);
	m_timeReduceCrossings = double(t) / 1000;
}
#endif

const HierarchyLevels *LayerByLayerSweep::reduceCrossings(const SugiyamaLayout &sugi, const Hierarchy &H, int &nCrossings)
{
	HierarchyLevels *levels = new HierarchyLevels(H);

	OGDF_ASSERT(sugi.runs() >= 1);

	unsigned int nThreads = min(sugi.maxThreads(), (unsigned int) sugi.runs());

	minstd_rand rng(randomSeed());

	LayerByLayerSweep::CrossMinMaster master(sugi, levels->hierarchy(), sugi.runs() - nThreads);

	Array<LayerByLayerSweep::CrossMinWorker *> worker(nThreads-1);
	Array<Thread>                              thread(nThreads - 1);
	for (unsigned int i = 0; i < nThreads - 1; ++i) {
		worker[i] = new LayerByLayerSweep::CrossMinWorker(master, clone(), nullptr);
		thread[i] = Thread(*worker[i]);
	}

	NodeArray<int> bestPos;
	master.doWorkHelper(this, nullptr, *levels, bestPos, sugi.permuteFirst(), rng);

	for (unsigned int i = 0; i < nThreads - 1; ++i )
		thread[i].join();

	master.restore(*levels, nCrossings);

	for ( unsigned int i = 0; i < nThreads - 1; ++i )
		delete worker[i];

	return levels;
}


const HierarchyLevelsBase *SugiyamaLayout::reduceCrossings(Hierarchy &H)
{
	OGDF_ASSERT(m_runs >= 1);

	if (!useSubgraphs()) {
		int64_t t;
		System::usedRealTime(t);
		const HierarchyLevelsBase *levels = m_crossMin->reduceCrossings( *this, H, m_nCrossings);
		t = System::usedRealTime(t);
		m_timeReduceCrossings = double(t) / 1000;
		m_nCrossings = levels -> calculateCrossings();
		return levels;
	}


	//unchanged crossing reduction of subgraphs
	HierarchyLevels *pLevels = new HierarchyLevels(H);
	HierarchyLevels levels = *pLevels;

	int64_t t;
	System::usedRealTime(t);

	LayerByLayerSweep          *pCrossMin = nullptr;
	TwoLayerCrossMinSimDraw   *pCrossMinSimDraw = nullptr;

	pCrossMinSimDraw = m_crossMinSimDraw.get();

	unsigned int nThreads = min(m_maxThreads, (unsigned int)m_runs);

	int seed = rand();
	minstd_rand rng(seed);

	LayerByLayerSweep::CrossMinMaster master(*this, levels.hierarchy(), m_runs - nThreads);

	Array<LayerByLayerSweep::CrossMinWorker *> worker(nThreads - 1);
	Array<Thread>                              thread(nThreads - 1);
	for (unsigned int i = 0; i < nThreads - 1; ++i) {
		worker[i] = new LayerByLayerSweep::CrossMinWorker(master,
			(pCrossMin        != nullptr) ? pCrossMin       ->clone() : nullptr,
			(pCrossMinSimDraw != nullptr) ? pCrossMinSimDraw->clone() : nullptr);
		thread[i] = Thread(*worker[i]);
	}

	NodeArray<int> bestPos;
	master.doWorkHelper(pCrossMin, pCrossMinSimDraw, levels, bestPos, m_permuteFirst, rng);

	for (unsigned int i = 0; i < nThreads - 1; ++i)
		thread[i].join();

	master.restore(levels, m_nCrossings);

	for (unsigned int i = 0; i < nThreads - 1; ++i)
		delete worker[i];

	t = System::usedRealTime(t);
	m_timeReduceCrossings = double(t) / 1000;

	return pLevels;
}

void SugiyamaLayout::call(ClusterGraphAttributes &AG)
{
#if 0
	std::ofstream os("C:\\temp\\sugi.txt");
	freopen("c:\\work\\GDE\\std::cout.txt", "w", stdout);

	const Graph &G = AG.constGraph();
#endif
	const ClusterGraph &CG = AG.constClusterGraph();
#if 0
	if (G.numberOfNodes() == 0) {
		os << "Empty graph." << std::endl;
		return;
	}
#endif

	// 1. Phase: Edge Orientation and Layer Assignment

	// Build extended nesting hierarchy H
	ExtendedNestingGraph H(CG);


#if 0
	os << "Cluster Hierarchy:\n";
	for(node v : G.nodes)
		os << "V " << v << ": parent = " << CG.clusterOf(v)->index() << "\n";

	for(cluster c : CG.clusters)
		if(c != CG.rootCluster())
			os << "C " << c->index() << ": parent = " << c->parent()->index() << "\n";

	os << "\nExtended Nesting Graph:\n";
	os << "  nodes: " << H.numberOfNodes() << "\n";
	os << "  edges: " << H.numberOfEdges() << "\n\n";

	for(node v : H.nodes) {
		os << v->index() << ": ";
		switch(H.type(v)) {
			case ExtendedNestingGraph::NodeType::Node:
				os << "[V  " << H.origNode(v);
				break;
			case ExtendedNestingGraph::NodeType::ClusterTop:
				os << "[CT " << H.originalCluster(v)->index();
				break;
			case ExtendedNestingGraph::NodeType::ClusterBottom:
				os << "[CB " << H.originalCluster(v)->index();
				break;
		}
		os << ", rank = " << H.rank(v) << "]  ";

		edge e;
		forall_adj_edges(e, v)
			if(e->target() != v)
				os << e->target() << " ";
		os << "\n";
	}


	os << "\nLong Edges:\n";
	for(edge e : G.edges) {
		os << e << ": ";
		ListConstIterator<edge> it;
		for(edge ei : H.chain(e))
			os << " " << ei;
		os << "\n";
	}

	os << "\nLevel:\n";
	int maxLevel = 0;
	for(node v : H.nodes)
		if(H.rank(v) > maxLevel)
			maxLevel = H.rank(v);
#endif

	Array<List<node> > level(H.numberOfLayers());
	for(node v : H.nodes)
		level[H.rank(v)].pushBack(v);
#if 0
	for(int i = 0; i <= maxLevel; ++i) {
		os << i << ": ";
		ListConstIterator<node> it;
		for(node v : level[i]) {
			switch(H.type(v)) {
				case ExtendedNestingGraph::NodeType::Node:
					os << "(V" << H.origNode(v);
					break;
				case ExtendedNestingGraph::NodeType::ClusterTop:
					os << "(CT" << H.originalCluster(v)->index();
					break;
				case ExtendedNestingGraph::NodeType::ClusterBottom:
					os << "(CB" << H.originalCluster(v)->index();
					break;
				case ExtendedNestingGraph::NodeType::Dummy:
					os << "(D" << v;
					break;
			}

			os << ",C" << H.originalCluster(v) << ")  ";
		}
		os << "\n";
	}


	os << "\nLayer Hierarchy Trees:\n";
	for(int i = 0; i <= maxLevel; ++i) {
		os << i << ": " << H.layerHierarchyTree(i) << "\n";
	}

	os << "\nCompound Nodes Adjacencies:\n";
	for(int i = 0; i <= maxLevel; ++i) {
		os << "Layer " << i << ":\n";

		Queue<const LHTreeNode*> Q;
		Q.append(H.layerHierarchyTree(i));

		while(!Q.empty()) {
			const LHTreeNode *p = Q.pop();
			os << "  C" << p->originalCluster() << ": ";

			for(const LHTreeNode::Adjacency &adj : p->m_upperAdj) {
				node        u      = adj.m_u;
				LHTreeNode *vNode  = adj.m_v;
				int         weight = adj.m_weight;

				os << " (" << u << ",";
				if(vNode->isCompound())
					os << "C" << vNode->originalCluster();
				else
					os << "N" << vNode->getNode();
				os << "," << weight << ") ";
			}
			os << "\n";

			for(int i = 0; i < p->numberOfChildren(); ++i)
				if(p->child(i)->isCompound())
					Q.append(p->child(i));
		}
	}
#endif

	// 2. Phase: Crossing Reduction
	reduceCrossings(H);
#if 0
	os << "\nLayers:\n";
	for(int i = 0; i < H.numberOfLayers(); ++i) {
		os << i << ": ";
		const List<node> &L = level[i];
		Array<node> order(0,L.size()-1,0);
		for(node v : L)
			order[H.pos(v)] = v;

		for(int j = 0; j < L.size(); ++j)
			os << " " << order[j];
		os << "\n";
	}

	os << "\nnumber of crossings: " << m_nCrossingsCluster << "\n";
#endif

	// 3. Phase: Coordinate Assignment
	H.removeTopBottomEdges();
	m_clusterLayout->callCluster(H, AG);
}


RCCrossings SugiyamaLayout::traverseTopDown(ExtendedNestingGraph &H)
{
	RCCrossings numCrossings;

	for(int i = 1; i < H.numberOfLayers(); ++i)
		numCrossings += H.reduceCrossings(i,true);

	return numCrossings;
}


RCCrossings SugiyamaLayout::traverseBottomUp(ExtendedNestingGraph &H)
{
	RCCrossings numCrossings;

	for(int i = H.numberOfLayers()-2; i >= 0; --i)
		numCrossings += H.reduceCrossings(i,false);

	return numCrossings;
}


void SugiyamaLayout::reduceCrossings(ExtendedNestingGraph &H)
{
	RCCrossings nCrossingsOld, nCrossingsNew;
	m_nCrossingsCluster = nCrossingsOld.setInfinity();

	for(int i = 1; ; ++i)
	{
		int nFails = m_fails+1;
		int counter = 0;

		do {
			counter++;
			// top-down traversal
			nCrossingsNew = traverseTopDown(H);

			if(nCrossingsNew < nCrossingsOld) {
				if(nCrossingsNew < m_nCrossingsCluster) {
					H.storeCurrentPos();

					if((m_nCrossingsCluster = nCrossingsNew).isZero())
						break;
				}
				nCrossingsOld = nCrossingsNew;
				nFails = m_fails+1;
			} else
				--nFails;

			// bottom-up traversal
			nCrossingsNew = traverseBottomUp(H);

			if(nCrossingsNew < nCrossingsOld) {
				if(nCrossingsNew < m_nCrossingsCluster) {
					H.storeCurrentPos();

					if((m_nCrossingsCluster = nCrossingsNew).isZero())
						break;
				}
				nCrossingsOld = nCrossingsNew;
				nFails = m_fails+1;
			} else
				--nFails;

		} while(nFails > 0);

		if(m_nCrossingsCluster.isZero() || i >= m_runs)
			break;

		H.permute();
		nCrossingsOld.setInfinity();
	}

	H.restorePos();
	m_nCrossings = m_nCrossingsCluster.m_cnEdges;
}

}
