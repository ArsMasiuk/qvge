/** \file
 * \brief Implementation of OrderComparer and LayerBasedUPRLayout classes.
 *
 * \author Hoi-Ming Wong
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

#include <ogdf/upward/LayerBasedUPRLayout.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Queue.h>

namespace ogdf {

OrderComparer::OrderComparer(const UpwardPlanRep &_UPR, Hierarchy &_H) : m_UPR(_UPR), H(_H)
{
	m_dfsNum.init(m_UPR, -1);
	crossed.init(m_UPR, false);

	//compute dfs number
	node start;
	hasSingleSource(m_UPR, start);
	NodeArray<bool> visited(m_UPR, false);
	adjEntry rightAdj = m_UPR.getAdjEntry(m_UPR.getEmbedding(), start, m_UPR.getEmbedding().externalFace());
	int num = 0;
	m_dfsNum[start] = num++;
	adjEntry run = rightAdj;
	do {
		run = run->cyclicSucc();
		if (!visited[run->theEdge()->target()])
			dfs_LR(run->theEdge(), visited, m_dfsNum, num);
	} while(run != rightAdj);
}

bool OrderComparer::left(edge e1UPR, edge e2UPR) const
{
	OGDF_ASSERT(e1UPR->source() == e2UPR->source() || e1UPR->target() == e2UPR->target());
	OGDF_ASSERT(e1UPR != e2UPR);

	node v = e1UPR->source();
	if (e2UPR->source() != v)
		v = e1UPR->target();

	adjEntry inLeft = nullptr, outLeft = nullptr;
	//compute left in and left out edge of the common node v if exist
	if (v->indeg() != 0) {
		for(adjEntry run : v->adjEntries) {
			if (run->cyclicSucc()->theEdge()->source() == v) {
				inLeft = run;
				break;
			}
		}
	}
	if (v->outdeg() != 0) {
		for(adjEntry run : v->adjEntries) {
			if (run->cyclicPred()->theEdge()->target() == v || m_UPR.getEmbedding().leftFace(run) == m_UPR.getEmbedding().externalFace()) {
				outLeft = run;
				break;
			}
		}
	}

	//same source;
	if (v == e2UPR->source()) {
		do {
			if (outLeft->theEdge() == e1UPR)
				return false;
			if (outLeft->theEdge() == e2UPR)
				return true;
			outLeft = outLeft->cyclicSucc();
		} while (true);
	}
	//same target
	else {
		do {
			if (inLeft->theEdge() == e1UPR)
				return false;
			if (inLeft->theEdge() == e2UPR)
				return true;
			inLeft = inLeft->cyclicPred();
		} while (true);
	}
}

bool OrderComparer::left(node v1UPR, const List<edge> &chain1, node v2UPR , const List<edge> &chain2) const
{
	//mark the edges an nodes of chain2 list
	NodeArray<bool> visitedNode(m_UPR, false);
	EdgeArray<bool> visitedEdge(m_UPR, false);
	for(edge e : chain2) {
		visitedNode[e->source()] =  visitedNode[e->target()] = true;
		visitedEdge[e] = true;
	}

	// traverse from vUPR2 to the super source using left path p and marks it.
	visitedNode[v2UPR] = true;
	for(adjEntry run = m_UPR.leftInEdge(v2UPR); run != nullptr; run = m_UPR.leftInEdge(run->theEdge()->source())) {
		visitedNode[run->theEdge()->source()] = visitedNode[run->theEdge()->target()] = true;
		visitedEdge[run->theEdge()] = true;
	}

	//is one of the node of chain1 marked?
	for(edge e : reverse(chain1)) {
		node u = e->source();
		if (visitedNode[u]) {
			for(adjEntry run : u->adjEntries) {
				if (visitedEdge[run->theEdge()] && run->theEdge()->source() == run->theNode()) // outgoing edges only
					return left(e, run->theEdge()); //(outEdgeOrder[e] > outEdgeOrder[run->theEdge()]);
			}
		}
	}

	// traverse from vUPR1 to a node of path p (using left path).
	adjEntry adj_v1 = nullptr, adj_v2 = nullptr;
	for (adjEntry run = m_UPR.leftInEdge(v1UPR); run != nullptr; run = m_UPR.leftInEdge(run->theEdge()->source())) {
		if (visitedNode[run->theEdge()->source()]) {
			adj_v1 = run->twin(); //reached a marked node
			break;
		}
	}

	OGDF_ASSERT(adj_v1 != nullptr);

	for(adjEntry run : adj_v1->theNode()->adjEntries) {
		if (visitedEdge[run->theEdge()] && run->theEdge()->source() == run->theNode()){ // outgoing edges only
			adj_v2 = run;
			break;
		}
	}

	OGDF_ASSERT(adj_v2 != nullptr);

	return left(adj_v1->theEdge(), adj_v2->theEdge());
}

bool OrderComparer::checkUp(node vUPR, int level) const
{
	const GraphCopy &GC = H;

	//traverse from vUPR (going up)
	NodeArray<bool> inList(m_UPR, false);
	List<node> list;
	list.pushBack(vUPR);
	inList[vUPR] = true;
	while (!list.empty()) {
		node v = list.popFrontRet();
		node vOrig = m_UPR.original(v);
		if (vOrig != nullptr && H.rank(GC.copy(vOrig)) <= level)
			return true;
		List<edge> outEdges;
		v->outEdges(outEdges);
		for(edge e : outEdges) {
			node tgt = e->target();
			if (!inList[tgt]) {
				list.pushBack(tgt);
				inList[tgt] = true;
			}
		}
	}
	return false;
}

bool OrderComparer::left(List<edge> &chain1, List<edge> &chain2, int level) const
{
	//mark the source nodes of the edges of chain1
	NodeArray<bool> markedNodes(m_UPR, false);
	EdgeArray<bool> markedEdges(m_UPR, false);
	for(edge e : chain1) {
		node v = e->source();
		markedNodes[v] = true;
		markedEdges[e] = true;
	}

	//compute the list of common nodes of chain1 and chain2
	List< Tuple2<node, bool> > commonNodeList; // first: common node; second: true if vH1 (associated with chain1) is on the left hand side
	for(edge e : chain2) {
		node v = e->source();
		if (markedNodes[v]) {
			//edge e = *iter;
			bool value = true;
			adjEntry adj = e->adjSource();
			while (true) {
				adj = adj->cyclicSucc();
				if (adj->theEdge()->target() == v) {
					value = false;
					break;
				}
				if (markedEdges[adj->theEdge()]) {
					break;
				}

			}
			Tuple2<node, bool> tulp(v, value);
			commonNodeList.pushFront(tulp);
		}
	}

	//no crossings between the associated edges
	if (commonNodeList.empty()) {
		if (chain1.front()->source() == chain2.front()->source()) {
			return left(chain1.front(), chain2.front());
		} else {
			return left(chain1.front()->source(), chain1, chain2.front()->source(), chain2);
		}
	}

	// there is a least one crossing
	ListIterator< Tuple2<node, bool> > it = commonNodeList.begin();
	while(it.valid()) {
		Tuple2<node, bool> tulp = *it;
		// is there a node above which level is lower or equal the given level?
		// if so, then return the value
		if (checkUp(tulp.x1(), level)) {
			// there is a node above, which is lower or equal the given level
			return tulp.x2();
		}
		it = it.succ();
	}

	// the both edges are on the "first segment" of the crossing
	Tuple2<node, bool> tulp = *(commonNodeList.rbegin());
	return !tulp.x2();
}

bool OrderComparer::less(node vH1, node vH2) const
{
	if (vH1 == vH2)
		return false;

	/*
	case:vH1 and vH2 are not long-edge dummies.
	*/
	const GraphCopy &GC = H;
	if (!H.isLongEdgeDummy(vH1) && !H.isLongEdgeDummy(vH2)) {
		node v1 = m_UPR.copy(GC.original(vH1));
		node v2 = m_UPR.copy(GC.original(vH2));
		return m_dfsNum[v1] > m_dfsNum[v2];
	}

	/*
	vH1 and vH2 are long-edge-dummies
	*/
	if (H.isLongEdgeDummy(vH1) && H.isLongEdgeDummy(vH2)) {
		List<edge> chain1 = m_UPR.chain(GC.original(vH1->firstAdj()->theEdge()));
		List<edge> chain2 = m_UPR.chain(GC.original(vH2->firstAdj()->theEdge()));

		OGDF_ASSERT(!chain1.empty());
		OGDF_ASSERT(!chain2.empty());

		int level = H.rank(vH1);
		return left(chain1, chain2, level);
	}

	/*
	only vH1 or vH2 is a long-edge dummy
	*/
	node v;
	List<edge> chain1, chain2;
	if (H.isLongEdgeDummy(vH1)) {
		chain1 = m_UPR.chain(GC.original(vH1->firstAdj()->theEdge()));
		v = m_UPR.copy(GC.original(vH2));

		OGDF_ASSERT(!chain1.empty());

		return left(chain1.front()->source(), chain1, v, chain2);
	}
	else {
		chain2 = m_UPR.chain(GC.original(vH2->firstAdj()->theEdge()));
		v = m_UPR.copy(GC.original(vH1));

		OGDF_ASSERT(!chain2.empty());

		return left(v, chain1, chain2.front()->source(), chain2);
	}
}

void OrderComparer::dfs_LR(
	edge e,
	NodeArray<bool> &visited,
	NodeArray<int> &dfsNum,
	int &num)
{
	node v = e->target();
	//outEdgeOrder[e] = num++;
	dfsNum[v] = num++;
	if (e->target()->outdeg() > 0) {
		// compute left out edge
		adjEntry run = nullptr;
		for(adjEntry adj : v->adjEntries) {
			adjEntry adj_pred = adj->cyclicPred();
			if (adj_pred->theEdge()->target() == v && adj->theEdge()->source() == v) {
				run = adj;
				break; // run is the left out-edge
			}
		}

		do {
			if (!visited[run->theEdge()->target()]) {
				dfs_LR(run->theEdge(), visited, dfsNum, num);
			}
			run = run->cyclicSucc();
		} while(run->theEdge()->target() != e->target());
	}
	visited[v] = true;
}

void LayerBasedUPRLayout::doCall(const UpwardPlanRep &UPR, GraphAttributes &AG)
{
	OGDF_ASSERT(UPR.augmented());

	numberOfLevels = 0;
	m_numLevels = 0;
	m_crossings = 0;

	const Graph &G = UPR.original();
	NodeArray<int> rank_G(G);
	computeRanking(UPR, rank_G);
	Hierarchy H(G, rank_G);
	HierarchyLevels levels(H);
	const GraphCopy &GC = H;

#if 0
	// m_UPR.outputFaces(m_UPR.getEmbedding());
	for(node x : G.nodes) {
		std::cout << "vOrig " << x << ";   vUPR " << m_UPR.copy(x) << std::endl;
	}
	for(node x : m_UPR.nodes) {
		std::cout << "m_UPR edge order:" << std::endl;
		adjEntry adj = x->firstAdj();
		std::cout << "node " << x << std::endl;
		do {
			std::cout << " edge : " << adj->theEdge() << std::endl;
			adj = adj->cyclicSucc();
		} while (adj != x->firstAdj());
	}
#endif

	//adjust order
	OrderComparer oComparer(UPR, H);
	for(int i = 0; i < levels.size(); ++i) {
		Level &level = levels[i];
		level.sortOrder(oComparer);
	}

	// postprocessing
	List<node> sources;
	for(node vTmp : GC.nodes) {
		if (vTmp->indeg() == 0)
			sources.pushBack(vTmp);
	}

	sources.quicksort(GenericComparer<node, int>([&](node v) { return -H.rank(v); }));

	postProcessing_reduceLED(H, levels, sources);
	levels.buildAdjNodes();

#if 0
	std::cout << std::endl << std::endl;
	for(int i = 0; i <= levels.high(); i++) {
		Level &lvl = levels[i];
		std::cout << "level : " << lvl.index() << std::endl;
		std::cout << "nodes : ";
		for(int j = 0; j <= lvl.high(); j++) {
			std::cout << lvl[j] << " ";
		}
		std::cout << std::endl;
	}
#endif

	postProcessing_sourceReorder(levels, sources);
	m_crossings = levels.calculateCrossings();

	while(!m_dummies.empty()) {
		H.m_GC.delNode(m_dummies.popRet());
	}

	OGDF_ASSERT(m_crossings <= UPR.numberOfCrossings());
	OGDF_ASSERT(m_layout);

	m_layout->call(levels,AG);
	// end postprocessing

	numberOfLevels = levels.size();
	m_maxLevelSize = 0;
	for(int i = 0; i <= levels.high(); i++) {
		Level &level = levels[i];
		if (m_maxLevelSize < level.size())
			m_maxLevelSize = level.size();
	}
}


void LayerBasedUPRLayout::computeRanking(const UpwardPlanRep &UPR, NodeArray<int> &rank)
{
	OGDF_ASSERT(UPR.augmented());

	GraphCopy GC(UPR.original());
	for(edge e : UPR.original().edges) {
		if (UPR.isReversed(e)) {
			GC.reverseEdge(GC.copy(e));
		}
	}

	// compute auxiliary edges
	EdgeArray<int> cost(GC,1);
	List< Tuple2<node, node> > auxEdges;
	NodeArray<int> inL(UPR, -1);
	int num = -1;
	for(node v : UPR.nodes) {

		if (UPR.isDummy(v) || v->indeg()==0)
			continue;

		num = num +1;
		//compute all "adjacent" non dummy nodes
		List<node> toDo, srcNodes;
		toDo.pushBack(v);
		inL[v] = num;
		while (!toDo.empty()) {
			node u = toDo.popFrontRet();
			List<edge> inEdges;
			u->inEdges(inEdges);
			for(edge eIn : inEdges) {
				node w = eIn->source();
				if (UPR.isDummy(w)) {
					if (inL[w] != num) {
						toDo.pushBack(w);
						inL[w] = num;
					}
				}
				else {

					OGDF_ASSERT(UPR.original(w) != nullptr);
					OGDF_ASSERT(UPR.original(v) != nullptr);

					node wGC = GC.copy(UPR.original(w));
					node vGC = GC.copy(UPR.original(v));
					edge eNew = GC.newEdge(wGC, vGC);
					cost[eNew] = 0;
				}
			}
		}
	}

	makeSimple(GC);

	OGDF_ASSERT(isAcyclic(GC));

#if 0
	GraphAttributes GA(GC, GraphAttributes::nodeGraphics |
						GraphAttributes::edgeGraphics |
						GraphAttributes::nodeStyle |
						GraphAttributes::edgeStyle |
						GraphAttributes::nodeLabel |
						GraphAttributes::edgeLabel);
	// label the nodes with their index
	for(node vTmp : GC.nodes) {
		node w = GC.original(vTmp);
		GA.label(vTmp) = to_string(w->index());
	}
	GA.writeGML("c:/temp/ranking_graph.gml");
#endif

	NodeArray<int> ranking(GC, 0);
	EdgeArray<int> length(GC,1);

	m_ranking->call(GC, length, cost, ranking);

	// adjust ranking
	int minRank = std::numeric_limits<int>::max();
	for(node v : GC.nodes) {
		if(ranking[v] < minRank)
			minRank = ranking[v];
	}

	if(minRank != 0) {
		for(node v : GC.nodes)
			ranking[v] -= minRank;
	}

	for(node v : GC.nodes) {
		node vOrig = GC.original(v);
		rank[vOrig] = ranking[v];
	}

#if 0
	std::cout << "Ranking GOrig: " << std::endl;
	for(node v : m_UPR.original().nodes)
		std::cout << "node :" << v << " ranking : " << rank[v] << std::endl;
#endif
}

void LayerBasedUPRLayout::postProcessing_sourceReorder(HierarchyLevels &levels, List<node> &sources)
{
	const Hierarchy &H = levels.hierarchy();

	//reorder the sources;
	for(node s : sources) {
		Level &level = levels[H.rank(s)];

		//compute the desire position (heuristic)
		int wantedPos = 0;
		GenericComparer<node, int> comp([&](node v) { return H.rank(v); });

		if (s->outdeg() == 1) {
			node tgt =  s->firstAdj()->theEdge()->target();
			List<node> nodes;

			for(adjEntry adj : tgt->adjEntries) {
				if (adj->theEdge()->target() == tgt)
					nodes.pushBack(adj->theEdge()->source());
			}

			nodes.quicksort(comp);

			//postion of the median
			node v = *nodes.get(nodes.size()/2);
			wantedPos = levels.pos(v);
		} else {
			List<node> nodes;

			for(adjEntry adj : s->adjEntries)
				nodes.pushBack(adj->theEdge()->source());

			nodes.quicksort(comp);

			//postion of the median
			node v = *nodes.get(nodes.size()/2);
			wantedPos = levels.pos(v);
		}

		//move s to front of the array
		int pos = levels.pos(s);
		while (pos != 0) {
			level.swap(pos-1, pos);
			pos--;
		}

		// compute the position of s, which cause min. crossing
		int minPos = pos;
		int oldCr = levels.calculateCrossings(level.index());
		while(pos != level.size()-1) {
			level.swap(pos, pos+1);
			int newCr = levels.calculateCrossings(level.index());
			if (newCr <= oldCr) {
				if (newCr < oldCr) {
					minPos = levels.pos(s);
					oldCr = newCr;
				}
				else {
					if (abs(minPos - wantedPos) > abs(pos+1 - wantedPos)) {
						minPos = levels.pos(s);
						oldCr = newCr;
					}

				}
			}
			pos++;
		}

		//move s to minPos
		while (pos != minPos) {
			if (minPos > pos) {
				level.swap(pos, pos+1);
				pos++;
			}
			if (minPos < pos) {
				level.swap(pos, pos-1);
				pos--;
			}
		}
	}
}

void LayerBasedUPRLayout::postProcessing_markUp(HierarchyLevels &levels, node s, NodeArray<bool> &markedNodes)
{
	const GraphCopy &GC = levels.hierarchy();
	NodeArray<bool> inQueue(GC, false);
	Queue<node> nodesToDo;
	nodesToDo.append(s);

	while(!nodesToDo.empty()) {
		node w = nodesToDo.pop();
		markedNodes[w] = true;
		List<edge> outEdges;
		w->outEdges(outEdges);
		ListIterator <edge> it;
		for (it = outEdges.begin(); it.valid(); ++it) {
			edge e = *it;
			if (!inQueue[e->target()] && !markedNodes[e->target()]) { // put the next node in queue if it is not already in queue
				nodesToDo.append( e->target() );
				inQueue[e->target()] = true;
			}
		}
	}
}

void LayerBasedUPRLayout::postProcessing_reduceLED(Hierarchy &H, HierarchyLevels &levels, node s)
{
	const GraphCopy &GC = H;

	NodeArray<bool> markedNodes(GC, false);

	// mark all nodes dominated by s, we call the graph induced by the marked node G*
	// note that not necessary all nodes are marked.
	postProcessing_markUp(levels, s, markedNodes);


	for (int i = H.rank(s) + 1; i <= levels.high(); i++) {
		const Level &lvl = levels[i];

		// Compute the start and end index of the marked graph on this level.
		int minIdx = std::numeric_limits<int>::max();
		int maxIdx = -1;
		List<node> sList;

		int numEdges = 0;
		int sumInDeg = 0;
		int numMarkedNodes = 0;
		int numDummies = 0;
		for(int j = 0; j <= lvl.high(); j++) {
			node u = lvl[j];

			if (markedNodes[u]) {
				numMarkedNodes++;

				if (H.isLongEdgeDummy(u))
					numDummies++;

				if (levels.pos(u)< minIdx)
					minIdx = levels.pos(u);
				if (levels.pos(u) > maxIdx)
					maxIdx = levels.pos(u);

				sumInDeg += u->indeg();
				for(adjEntry adj : u->adjEntries) {
					if (adj->theEdge()->target()==u && markedNodes[adj->theEdge()->source()])
						numEdges++;
				}
			}
		}
		if (numEdges!=sumInDeg || maxIdx-minIdx+1!=numMarkedNodes )
			return;

		if (numDummies!=numMarkedNodes)
			continue;

		//delete long edge dummies
		for (int k = minIdx; k <= maxIdx; k++) {
			node u = lvl[k];

			OGDF_ASSERT(H.isLongEdgeDummy(u));

			edge inEdge = u->firstAdj()->theEdge();
			edge outEdge = u->lastAdj()->theEdge();
			if (inEdge->target() != u)
				std::swap(inEdge, outEdge);

			edge eOrig = H.m_GC.original(inEdge);
			OGDF_ASSERT(eOrig == H.m_GC.original(outEdge));

			node x = H.m_GC.newNode();
			H.m_GC.moveSource(outEdge, x);
			H.m_GC.moveTarget(inEdge, x);
			H.m_GC.unsplit(inEdge, outEdge);
			m_dummies.push(u);
		}

#if 0
		std::cout << std::endl << std::endl;
		std::cout << "vor :					" << std::endl;
		for(int ii = 0; ii <= H.high(); ii++) {
			Level &lvl = H[ii];
			std::cout << std::endl;
			std::cout << "level : " << lvl.index() << std::endl;
			std::cout << "nodes : ";
			for(int jj = 0; jj <= lvl.high(); jj++) {
				std::cout << lvl[jj] << "/" << H.pos(lvl[jj]) << "  ";
			}
			std::cout << std::endl;
		}
#endif

		post_processing_reduce(H, levels, i, s, minIdx, maxIdx, markedNodes);

#if 0
		std::cout << std::endl << std::endl;
		std::cout << std::endl << std::endl;
		std::cout << "nach :					" << std::endl;
		for(int ii = 0; ii <= H.high(); ii++) {
			Level &lvl = H[ii];
			std::cout << std::endl;
			std::cout << "level : " << lvl.index() << std::endl;
			std::cout << "nodes : ";
			for(int jj = 0; jj <= lvl.high(); jj++) {
				std::cout << lvl[jj] << "/" << H.pos(lvl[jj]) << "  ";
			}
			std::cout << std::endl;
		}
#endif
	}
}


void LayerBasedUPRLayout::post_processing_reduce(
	Hierarchy &H,
	HierarchyLevels &levels,
	int &i,
	node s,
	int minIdx,
	int maxIdx,
	NodeArray<bool> &markedNodes)
{
	const Level &lvl = levels[i];

	if (maxIdx-minIdx+1 == lvl.size()) {
		post_processing_deleteLvl(H, levels, i);
		i--;
		return;
	}

	// delete the dummies in interval[minIdx,maxIdx] and copy the nodes in lvl i-1 to lvl i for i={0,..,i-1}
	int startLvl = H.rank(s);
	for (int j = i; j > startLvl; j--) {

		int idxl1 = std::numeric_limits<int>::max();
		int idxl2 = std::numeric_limits<int>::max();
		int idxh1 = -1;
		int idxh2 = -1;
		for (int k = 0; k <= levels[j].high(); k++) {
			node u = levels[j][k];

			if (markedNodes[u]) {
				if (k < idxl1)
					idxl1 = k;
				if (k > idxh1)
					idxh1 = k;
			}
		}

		for (int k = 0; k <= levels[j-1].high(); k++) {
			node u = levels[j-1][k];

			if (markedNodes[u]) {
				if (k < idxl2)
					idxl2 = k;
				if (k > idxh2)
					idxh2 = k;
			}
		}

		int jTmp = j;
		post_processing_deleteInterval(H, levels, idxl1, idxh1, j);
		if (jTmp!=j) {
			i--;
			return; //a level was deleted, we are done
		}

#if 0
		std::cout << std::endl << std::endl;
		std::cout << "nach delete :					" << std::endl;
		for(int ii = 0; ii <= H.high(); ii++) {
			Level &lvl = H[ii];
			std::cout << std::endl;
			std::cout << "level : " << lvl.index() << std::endl;
			std::cout << "nodes : ";
			for(int jj = 0; jj <= lvl.high(); jj++) {
				std::cout << lvl[jj] << "/" << H.pos(lvl[jj]) << "  ";
			}
			std::cout << std::endl;
		}
#endif

		post_processing_CopyInterval(H, levels, j, idxl2, idxh2, idxl1);

#if 0
		std::cout << std::endl << std::endl;
		std::cout << "nach copy :					" << std::endl;
		for(int ii = 0; ii <= H.high(); ii++) {
			Level &lvl = H[ii];
			std::cout << std::endl;
			std::cout << "level : " << lvl.index() << std::endl;
			std::cout << "nodes : ";
			for(int jj = 0; jj <= lvl.high(); jj++) {
				std::cout << lvl[jj] << "/" << H.pos(lvl[jj]) << "  ";
			}
			std::cout << std::endl;
		}
#endif

	}

	int idxl1 = std::numeric_limits<int>::max();
	int idxh1 = -1;
	for (int k = 0; k <= levels[startLvl].high(); k++) {
		node u = levels[startLvl][k];

		if (markedNodes[u]) {
			if (k < idxl1)
				idxl1 = k;
			if (k > idxh1)
				idxh1 = k;
		}
	}
	int tmp = startLvl;
	post_processing_deleteInterval(H, levels, idxl1, idxh1, startLvl);
	if (tmp!=startLvl)
		i--;
}


void LayerBasedUPRLayout::post_processing_CopyInterval(Hierarchy &H, HierarchyLevels &levels, int i, int beginIdx, int endIdx, int pos)
{
	Level &lvl_cur = levels[i];
	int intervalSize = endIdx - beginIdx +1;
	int lastIdx = lvl_cur.high();

	OGDF_ASSERT(intervalSize > 0);

	// grow array
	lvl_cur.m_nodes.grow(intervalSize);
	//move all the data block [pos,lvl_cur.high()] to the end of the array
	for (int k = 0; k < (lastIdx - pos + 1) ; k++) {
		//update position
		levels.m_pos[lvl_cur[lastIdx - k]] = lvl_cur.high() - k;
		lvl_cur[lvl_cur.high() - k] = lvl_cur[lastIdx - k];
	}

#if 0
	std::cout << std::endl << std::endl;
	std::cout << "level after shift block to end of array : " << lvl.index() << std::endl;
	std::cout << "nodes : ";
	for(int j = 0; j <= lvl.high(); j++) {
		std::cout << lvl[j] << " ; pos() " << pos(lvl[j]) << "  ";
	}
	std::cout << std::endl;
#endif

	//copy the nodes of nodeList into the array
	Level &lvl_low = levels[i-1];
	int idx = pos;
	for (int k = beginIdx; k <= endIdx; k++) {
		node u = lvl_low[k];
		lvl_cur[idx] = u;
		// update member data
		levels.m_pos[u] = idx;
		H.m_rank[u] = lvl_cur.index();
		idx++;
	}
}

void LayerBasedUPRLayout::post_processing_deleteInterval(Hierarchy &H, HierarchyLevels &levels, int beginIdx, int endIdx, int &j)
{
	Level &lvl = levels[j];

	int i = 0;
	while ((endIdx + i) < lvl.high()) {
		lvl[beginIdx + i] = lvl[endIdx + i +1];
		levels.m_pos[lvl[endIdx + i +1]] = beginIdx + i;
		i++;
	}

	int blockSize = endIdx - beginIdx + 1;

	if (lvl.m_nodes.size()==blockSize) {
		int level = lvl.index();
		post_processing_deleteLvl(H, levels, level); //delete the lvl
		j--;
	}
	else
		lvl.m_nodes.grow(-blockSize); // reduce the size of the lvl
}

void LayerBasedUPRLayout::post_processing_deleteLvl(Hierarchy &H, HierarchyLevels &levels, int i)
{
	//move the pointer to end, then delete the lvl
	int curPos = i;
	while (curPos < levels.high()) {
		std::swap(levels.m_pLevel[curPos], levels.m_pLevel[curPos+1]);
		Level &lvlTmp = levels[curPos];
		lvlTmp.m_index = curPos;
		//update rank
		for(int iter = 0; iter <= lvlTmp.high(); iter++) {
			H.m_rank[lvlTmp[iter]] = curPos;
		}
		curPos++;
	}
	//delete
	delete levels.m_pLevel[levels.high()];
	levels.m_pLevel.grow(-1);
}

void LayerBasedUPRLayout::UPRLayoutSimple(const UpwardPlanRep &UPR, GraphAttributes &GA)
{
	//clear some data
	for(edge e : GA.constGraph().edges) {
		GA.bends(e).clear();
	}

	// layout the representation
	GraphAttributes GA_UPR(UPR);
	for(node v : GA.constGraph().nodes) {
		node vUPR = UPR.copy(v);
		GA_UPR.height(vUPR) = GA.height(v);
		GA_UPR.width(vUPR) = GA.width(v);
	}


	//compute the left edge
	adjEntry adjLeft = nullptr;
	for(adjEntry adj : UPR.getSuperSource()->adjEntries) {
		if (UPR.getEmbedding().rightFace(adj) == UPR.getEmbedding().externalFace()) {
			adjLeft = adj;
			break;
		}
	}
	adjLeft = adjLeft->cyclicSucc();
	callSimple(GA_UPR, adjLeft);

	//map to AG
	for(node v : GA.constGraph().nodes) {
		double vX = GA_UPR.x(UPR.copy(v));
		double vY = GA_UPR.y(UPR.copy(v));
		GA.x(v) = vX;
		GA.y(v) = vY;
	}

	// add bends to original edges
	for(edge e : GA.constGraph().edges) {
		const List<edge> &chain = UPR.chain(e);
		for(edge eUPR : chain) {
			node tgtUPR = eUPR->target();

			//add bend point of eUPR to original edge
			ListIterator<DPoint> iter;
			DPolyline &line = GA_UPR.bends(eUPR);
			for(iter = line.begin(); iter.valid(); ++iter) {
				double x2 = (*iter).m_x;
				double y2 = (*iter).m_y;
				DPoint p(x2, y2);
				GA.bends(e).pushBack(p);
			}
			//add target node of a edge segment as bend point
			if (tgtUPR != chain.back()->target()) {
				double pX = GA_UPR.x(tgtUPR);
				double pY = GA_UPR.y(tgtUPR);
				DPoint p(pX, pY);
				GA.bends(e).pushBack(p);
			}
		}

		DPolyline &poly = GA.bends(e);
		DPoint pSrc(GA.x(e->source()), GA.y(e->source()));
		DPoint pTgt(GA.x(e->target()), GA.y(e->target()));
		poly.normalize(pSrc, pTgt);
	}

	//layers and max. layer size
}


void LayerBasedUPRLayout::callSimple(GraphAttributes &GA, adjEntry adj)
{
	m_numLevels = -1;	//not implemented yet!
	m_maxLevelSize = -1; //not implemented yet!

	const Graph &G = GA.constGraph();

	OGDF_ASSERT(adj->graphOf() == &G);

	// We make a copy stGraph of the input graph G

	GraphCopySimple stGraph(G);

	// determine single source s, single sink t and edge (s,t)
	node s, t;
	hasSingleSource(G, s);
	hasSingleSink(G, t);
	s = stGraph.copy(s);
	t = stGraph.copy(t);

	adjEntry adjCopy = stGraph.copy(adj->theEdge())->adjSource();

#if 0
	std::cout << "stGraph:" << std::endl;
	for(node x : stGraph.nodes) {
		std::cout << x << ":";
		for(adjEntry adj : x->adjEntries) {
			edge e = adj->theEdge();
			std::cout << " " << e;
		}
		std::cout << std::endl;
	}
#endif

	// For the st-graph, we compute a longest path ranking. Since the graph
	// is st-planar, it is also level planar for the computed rank assignment.
	NodeArray<int> stRank(stGraph);
	longestPathRanking(stGraph,stRank);

#ifdef OGDF_DEBUG
	for (edge e : stGraph.edges)
		OGDF_ASSERT(stRank[e->source()] < stRank[e->target()]);
#endif

	// We translate the rank assignment for stGraph to a rank assignment of G
	// a compute a proper hierarchy for G with this ranking. Since G is a
	// subgraph of stGraph, G is level planar with this ranking.
	NodeArray<int> rank(G);

	for(node vG : G.nodes)
		rank[vG] = stRank[stGraph.copy(vG)];


#if 0
	std::cout << "rank assignment G:" << std::endl;
	for(node vG : G.nodes) {
		std::cout << vG << ": " << rank[vG] << std::endl;
	}
#endif

	Hierarchy H(G,rank);
	HierarchyLevels levels(H);

	// GC is the underlying graph of the proper hierarchy H.
	const GraphCopy &GC = H;


	// We compute the positions of the nodes of GC on each level. It is
	// important to determine also the positions of the dummy nodes which
	// resulted from splitting edges. The node array st2GC maps the nodes in
	// stGraph to the nodes in GC.
	NodeArray<node> st2GC(stGraph,nullptr);

	// For nodes representing real nodes in G this is simple.
	for(node vG : G.nodes) {
		OGDF_ASSERT(H.rank(GC.copy(vG)) == stRank[stGraph.copy(vG)]);
		st2GC[stGraph.copy(vG)] = GC.copy(vG);
	}

	// For the dummy nodes, we first have to split edges in stGraph.
	// For an edge e=(v,w), we have to split e stRank[w]-stRank[v]-1 times.
	for(edge eG : G.edges) {
		edge eSt = stGraph.copy(eG);
		const List<edge> &pathGC = GC.chain(eG);

		ListConstIterator<edge> it;
		int r = stRank[eSt->source()];
		for(it = pathGC.begin().succ(); it.valid(); ++it) {
			eSt = stGraph.split(eSt);
			node v = eSt->source();
			node vGC = (*it)->source();
			stRank[v] = ++r;
			st2GC[v] = vGC;

			OGDF_ASSERT(stRank[v] == H.rank(vGC));
		}
	}

#ifdef OGDF_DEBUG
	for(node v : stGraph.nodes) {
		node vGC = st2GC[v];
		OGDF_ASSERT(vGC == nullptr || stRank[v] == H.rank(vGC));
	}
#endif

#if 0
	std::cout << "mapping stGraph -> GC -> G:" << std::endl;
	for(node v : stGraph.nodes)
		std::cout << v << ": " << st2GC[v] << " " << stGraph.original(v) << std::endl;
#endif


	// The array nodes contains the sorted nodes of stGraph on each level.
	Array<SListPure<node> > nodes(stRank[s],stRank[t]);

	dfsSortLevels(adjCopy,stRank,nodes);

#if 0
	for(int i = stRank[s]; i <= stRank[t]; ++i) {
		std::cout << i << ": ";
		SListPure<node> &L = nodes[i];
		SListConstIterator<node> it;
		for(it = L.begin(); it.valid(); ++it) {
			std::cout << stGraph.original(*it) << " ";
			node vGC = st2GC[*it];
			OGDF_ASSERT(vGC == 0 || H.rank(vGC) == i);
		}
		std::cout << std::endl;
	}
#endif

	// We translate the node lists to node lists of nodes in GC using node
	// array st2GC. Note that there are also nodes in stGraph which have
	// no counterpart in GC (these are face nodes of the face-sink graph
	// introduced by the augmentation). We can simply ignore such nodes.
	int i;
	for (i = 0; i <= levels.high(); ++i) {
		Level &level = levels[i];

#if 0
		std::cout << i << std::endl;
		std::cout << level << std::endl;
#endif

		int j = 0;
		SListConstIterator<node> itSt;
		for(itSt = nodes[i].begin(); itSt.valid(); ++itSt) {
			node vGC = st2GC[*itSt];
			if(vGC != nullptr)
				level[j++] = vGC;
		}

		//std::cout << level << std::endl;
		//std::cout << std::endl;

		level.recalcPos(); // Recalculate some internal data structures in H
	}

	levels.check();


	//std::cout << "crossings: " << H.calculateCrossings() << std::endl;
	OGDF_ASSERT(levels.calculateCrossings() == 0);

	// Finally, we draw the computed hierarchy applying a hierarchy layout
	// module.
	m_layout->call(levels,GA);
}


// This procedure computes the sorted nodes lists on each level of an st-graph.
// adj1 corresponds to the leftmost outgoing edge of v = adj1->theNode().
// Levels are build from left to right.
void LayerBasedUPRLayout::dfsSortLevels(
	adjEntry adj1,                   // leftmost outgoing edge
	const NodeArray<int> &rank,      // ranking
	Array<SListPure<node> > &nodes)  // sorted nodes on levels
{
	node v = adj1->theNode();

	nodes[rank[v]].pushBack(v);

	// iterate over all outgoing edges from left to right
	adjEntry adj = adj1;
	do {
		node w = adj->theEdge()->target();
		OGDF_ASSERT(v != w);

		// Is adjW the leftmost outgoing edge of w ?
		adjEntry adjW = adj->twin()->cyclicSucc();
		if(adjW->theEdge()->source() == w)
			dfsSortLevels(adjW,rank,nodes);

		adj = adj->cyclicSucc();
	} while (adj != adj1 && adj->theEdge()->source() == v);
}


// for UPRLayoutSimple
void LayerBasedUPRLayout::longestPathRanking(
	const Graph &G,
	NodeArray<int> &rank)
{
	ArrayBuffer<node> sources;
	NodeArray<int> indeg(G);

	for(node v : G.nodes) {
		indeg[v] = v->indeg();
		rank[v] = 0;
		if(indeg[v] == 0) {
			sources.push(v);
		}
	}

	while(!sources.empty())
	{
		node v = sources.popRet();

		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node w = e->target();
			if (w == v) continue;

			if(rank[w] < rank[v]+1)
				rank[w] = rank[v]+1;

			if(--indeg[w] == 0) {
				sources.push(w);
			}
		}
	}
}

}
