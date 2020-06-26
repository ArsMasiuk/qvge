/** \file
 * \brief Implementation of node ranking algorithms
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

#include <ogdf/layered/LongestPathRanking.h>
#include <ogdf/layered/DfsAcyclicSubgraph.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Queue.h>

namespace ogdf {

LongestPathRanking::LongestPathRanking()
{
	m_subgraph.reset(new DfsAcyclicSubgraph);
	m_sepDeg0 = true;
	m_separateMultiEdges = true;
	m_optimizeEdgeLength = true;
	m_alignBaseClasses = false;
	m_alignSiblings = false;
}


void LongestPathRanking::call(const Graph &G, const EdgeArray<int> &length, NodeArray<int> &rank)
{
	List<edge> R;

	m_subgraph->call(G,R);

	EdgeArray<bool> reversed(G,false);
	for (edge e : R)
		reversed[e] = true;
	R.clear();

	doCall(G, rank, reversed, length);
}


void LongestPathRanking::call (const Graph& G, NodeArray<int> &rank)
{
	List<edge> R;

	m_subgraph->call(G,R);

	EdgeArray<bool> reversed(G,false);
	for (edge e : R)
		reversed[e] = true;
	R.clear();

	EdgeArray<int> length(G,1);

	if(m_separateMultiEdges) {
		SListPure<edge> edges;
		EdgeArray<int> minIndex(G), maxIndex(G);
		parallelFreeSortUndirected(G, edges, minIndex, maxIndex);

		SListConstIterator<edge> it = edges.begin();
		if(it.valid())
		{
			int prevSrc = minIndex[*it];
			int prevTgt = maxIndex[*it];

			for(it = it.succ(); it.valid(); ++it) {
				edge e = *it;
				if (minIndex[e] == prevSrc && maxIndex[e] == prevTgt)
					length[e] = 2;
				else {
					prevSrc = minIndex[e];
					prevTgt = maxIndex[e];
				}
			}
		}
	}

	doCall(G, rank, reversed, length);
}


void LongestPathRanking::callUML(const GraphAttributes &AG, NodeArray<int> &rank)
{
	const Graph &G = AG.constGraph();

	// find base classes
	List<node> baseClasses;
	for(node v : G.nodes) {
		bool isBase = false;
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			if(AG.type(e) != Graph::EdgeType::generalization)
				continue;

			if(e->target() == v) {
				// incoming edge
				isBase = true; // possible base of a hierarchy
			}
			if(e->source() == v) {
				// outgoing edge
				isBase = false;
				break;
			}
		}
		if(isBase)
			baseClasses.pushBack(v);
	}

	// insert a super sink
	GraphCopySimple GC(G);
	makeLoopFree(GC);
	GraphAttributes AGC(GC,GraphAttributes::edgeType);

	node superSink = GC.newNode();

	for(node v : baseClasses) {
		edge ec = GC.newEdge(GC.copy(v), superSink);
		AGC.type(ec) = Graph::EdgeType::generalization;
	}

	for(edge e : G.edges)
		AGC.type(GC.copy(e)) = AG.type(e);

	// compute length of edges
	EdgeArray<int> length(GC,1);

	if(m_separateMultiEdges) {
		SListPure<edge> edges;
		EdgeArray<int> minIndex(G), maxIndex(G);
		parallelFreeSortUndirected(G, edges, minIndex, maxIndex);

		SListConstIterator<edge> it = edges.begin();
		if(it.valid())
		{
			int prevSrc = minIndex[*it];
			int prevTgt = maxIndex[*it];

			for(it = it.succ(); it.valid(); ++it) {
				edge e = *it;
				if (minIndex[e] == prevSrc && maxIndex[e] == prevTgt)
					length[GC.copy(e)] = 2;
				else {
					prevSrc = minIndex[e];
					prevTgt = maxIndex[e];
				}
			}
		}
	}

	// compute spanning tree
	// marked edges belong to tree
	NodeArray<int> outdeg(GC,0);
	for(node v : GC.nodes) {
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			if(!e->isSelfLoop() && e->source() == v &&
				AGC.type(e) == Graph::EdgeType::generalization /*&& reversed[e] */)
				++outdeg[v];
		}
	}

	Queue<node> Q;
	Q.append(superSink);
	EdgeArray<bool> marked(GC,false);
	while(!Q.empty()) {
		node v = Q.pop();
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node u = e->source();
			if(u == v || AGC.type(e) != Graph::EdgeType::generalization/* || !reversed[e] */)
				continue;

			--outdeg[u];
			if(outdeg[u] == 0) {
				marked[e] = true;
				Q.append(u);
			}
		}
	}

	outdeg.init();

	// build super graph on which we will compute the ranking
	// we join nodes that have to be placed on the same level to super nodes
	NodeArray<node> superNode(G,nullptr);
	NodeArray<SListPure<node> > joinedNodes(GC);

	// initially, there is a single node in GC for every node in G
	for(node v : G.nodes) {
		node vc = GC.copy(v);
		superNode[v] = vc;
		joinedNodes[vc].pushBack(v);
	}

	if(m_alignBaseClasses && baseClasses.size() >= 2) {
		ListConstIterator<node> it = baseClasses.begin();
		node v1 = superNode[*it++];
		for(; it.valid(); ++it)
			join(GC,superNode,joinedNodes,v1,superNode[*it]);
	}

	// not needed anymore
	GC.delNode(superSink);
	baseClasses.clear();

	if(m_alignSiblings) {
		NodeArray<SListPure<node> > toJoin(GC);

		for(node v : GC.nodes) {
			node v1 = nullptr;
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();
				if(!marked[e] || e->source() == v)
					continue;

				node u = e->source();
				if(v1 == nullptr)
					v1 = u;
				else
					toJoin[v1].pushBack(u);
			}
		}

		for(node v : GC.nodes) {
			for(node u : toJoin[v])
				join(GC,superNode,joinedNodes,v,u);
		}
	}

	marked.init();
	joinedNodes.init();

	// don't want self-loops
	makeLoopFree(GC);

	// determine reversed edges
	DfsAcyclicSubgraph sub;
	List<edge> R;
	sub.callUML(AGC,R);

	EdgeArray<bool> reversed(GC,true);
	for (edge e : R)
		reversed[e] = false;
	R.clear();

	// compute ranking of GC
	NodeArray<int> rankGC;
	doCall(GC, rankGC, reversed, length);

	// transfer to ranking of G
	rank.init(G);
	for(node v : G.nodes)
		rank[v] = rankGC[superNode[v]];
}


void LongestPathRanking::join(
	GraphCopySimple &GC,
	NodeArray<node> &superNode,
	NodeArray<SListPure<node> > &joinedNodes,
	node v, node w)
{
	OGDF_ASSERT(v != w);

	for(node vi : joinedNodes[w])
		superNode[vi] = v;

	joinedNodes[v].conc(joinedNodes[w]);

	SListPure<edge> edges;
	w->adjEdges(edges);
	for(edge e : edges) {
		if(e->source() == w)
			GC.moveSource(e, v);
		else
			GC.moveTarget(e, v);
	}

	GC.delNode(w);
}


void LongestPathRanking::doCall(
	const Graph& G,
	NodeArray<int> &rank,
	EdgeArray<bool> &reversed,
	const EdgeArray<int> &length)
{
	rank.init(G,0);

	m_isSource.init(G,true);
	m_adjacent.init(G);

	for(edge e : G.edges) {
		if (e->isSelfLoop()) continue;

		if (!reversed[e]) {
			m_adjacent[e->source()].pushBack(Tuple2<node,int>(e->target(),length[e]));
			m_isSource[e->target()] = false;
		} else {
			m_adjacent[e->target()].pushBack(Tuple2<node,int>(e->source(),length[e]));
			m_isSource[e->source()] = false;
		}
	}

	m_ingoing.init(G,0);

	if(m_optimizeEdgeLength) {
		m_finished.init(G,false);

		int min = 0, max = 0;
		m_maxN = G.numberOfNodes();

		for(node v : G.nodes)
			if (m_isSource[v]) {
				dfs(v);
				getTmpRank(v,rank);
				dfsAdd(v,rank);

				if (rank[v] < min) min = rank[v];
			}

		for(node v : G.nodes) {
			if ((rank[v] -= min) > max) max = rank[v];
		}

		if (max > 0 && separateDeg0Layer()) {
			max++;
			for(node v : G.nodes)
				if (v->degree() == 0) rank[v] = max;
		}

		m_finished.init();

	} else {
		SListPure<node> sources;

		for(node v : G.nodes) {
			if(m_isSource[v])
				sources.pushBack(v);
			for(const Tuple2<node,int> &p : m_adjacent[v])
				++m_ingoing[p.x1()];
		}

		while(!sources.empty()) {
			node v = sources.popFrontRet();

			for (const Tuple2<node, int> &p : m_adjacent[v]) {
				node u = p.x1();
				int r = rank[v]+p.x2();
				if(r > rank[u])
					rank[u] = r;

				if (--m_ingoing[u] == 0)
					sources.pushBack(u);
			}
		}
	}

	m_isSource.init();
	m_adjacent.init();
	m_ingoing .init();
}


void LongestPathRanking::dfs(node v)
{
	m_ingoing[v]++;
	if (m_ingoing[v] == 1 && !m_finished[v]) {
		for (const Tuple2<node, int> &p : m_adjacent[v])
			dfs(p.x1());
	}
}


void LongestPathRanking::getTmpRank(node v, NodeArray<int> &rank)
{
	List<node> N;

	m_offset = m_maxN;
	N.pushBack(v);
	rank[v] = 0;

	while (!N.empty()) {
		node w = N.front(); N.popFront();

		for (const Tuple2<node, int> &p : m_adjacent[w]) {
			node u = p.x1();

			int r = max(rank[u],rank[w]+p.x2());

			m_ingoing[u]--;
			if (m_finished[u])
				Math::updateMin(m_offset, rank[u] - rank[w] - p.x2());

			else {
				if (m_ingoing[u] == 0) {
					N.pushBack(u);
				}
				rank[u] = r;
			}
		}
	}
	if (m_offset == m_maxN) m_offset = 0;
}


void LongestPathRanking::dfsAdd(node v, NodeArray<int> &rank)
{
	if (!m_finished[v]) {
		m_finished[v] = true;
		rank[v] += m_offset;

		for (const Tuple2<node, int> &p : m_adjacent[v])
			dfsAdd(p.x1(),rank);
	}
}

}
