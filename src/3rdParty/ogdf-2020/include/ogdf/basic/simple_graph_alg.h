/** \file
 * \brief Declaration of simple graph algorithms.
 *
 * \author Carsten Gutwenger and Sebastian Leipert
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

#pragma once

#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/SList.h>

namespace ogdf {

//! \name Methods for loops
//! @{

/**
 * Removes all self-loops for a given node \p v in \p graph
 *
 * @ingroup ga-multi
 */
OGDF_EXPORT void removeSelfLoops(Graph &graph, node v);

//! Returns true iff \p G contains no self-loop.
/**
 * @ingroup ga-multi
 *
 * @param G is the input graph.
 * @return true if \p G contains no self-loops (edges whose two endpoints are the same), false otherwise.
 */
OGDF_EXPORT bool isLoopFree(const Graph &G);

//! Removes all self-loops from \p G and returns all nodes with self-loops in \p L.
/**
 * @ingroup ga-multi
 *
 * @tparam NODELIST is the type of the node list for returning the nodes with self-loops.
 * @param  G is the input graph.
 * @param  L is assigned the list of nodes with self-loops.
 */
template<class NODELIST>
void makeLoopFree(Graph &G, NODELIST &L)
{
	L.clear();

	safeForEach(G.edges, [&](edge e) {
		if (e->isSelfLoop()) {
			L.pushBack(e->source());
			G.delEdge(e);
		}
	});
}

//! Returns whether \p G has edges which are not self-loops.
OGDF_EXPORT bool hasNonSelfLoopEdges(const Graph &G);

//! Removes all self-loops from \p G.
/**
 * @ingroup ga-multi
 *
 * @param  G is the input graph.
 */
OGDF_EXPORT void makeLoopFree(Graph &G);


//! @}
//! \name Methods for parallel edges
//! @{

//! Sorts the edges of \p G such that parallel edges come after each other in the list.
/**
 * @ingroup ga-multi
 *
 * @param G is the input graph.
 * @param edges is assigned the list of sorted edges.
 */
OGDF_EXPORT void parallelFreeSort(const Graph &G, SListPure<edge> &edges);


//! Returns true iff \p G contains no parallel edges.
/**
 * @ingroup ga-multi
 *
 * A parallel edge is an edge e1=(v,w) such that there exists another edge e2=(v,w) in
 * the graph. Reversal edges (e.g. (v,w) and (w,v)) are not parallel edges. If you want to
 * test if a graph contains no undirected parallel edges, use isParallelFreeUndirected().
 *
 * @param G is the input graph.
 * @return true if \p G contains no multi-edges (edges with the same source and target).
 */
OGDF_EXPORT bool isParallelFree(const Graph &G);


//! Returns the number of parallel edges in \p G.
/**
 * @ingroup ga-multi
 *
 * A parallel edge is an edge e1=(v,w) such that there exists another edge e2=(v,w) in
 * the graph. Reversal edges (e.g. (v,w) and (w,v)) are not parallel edges. If you want to
 * also take reversal edges into account, use numParallelEdgesUndirected().
 *
 * @param G is the input graph.
 * @tparam ONLY_ONCE Whether the searching for multi-edges should be stopped
 * once a single multi-edge is found.
 * @return is the number of parallel edges: for each bundle of parallel edges between two nodes
 *         v and w, all but one are counted.
 */
template <bool ONLY_ONCE = false>
int numParallelEdges(const Graph &G) {
	if (G.numberOfEdges() <= 1) return 0;

	SListPure<edge> edges;
	parallelFreeSort(G,edges);

	int num = 0;
	SListConstIterator<edge> it = edges.begin();
	edge ePrev = *it, e;
	for(it = ++it; it.valid(); ++it, ePrev = e) {
		e = *it;
		if (ePrev->isParallelDirected(e)) {
			++num;
			if (ONLY_ONCE) {
				return num;
			}
		}
	}

	return num;
}

//! Removes all but one of each bundle of parallel edges.
/**
 * @ingroup ga-multi
 *
 * A parallel edge is an edge e1=(v,w) such that there exists another edge e2=(v,w) in
 * the graph. Reversal edges (e.g. (v,w) and (w,v)) are not multi-edges. If you want to
 * remove parallel and reversal edges, use ogdf::makeParallelFreeUndirected().
 *
 * @tparam EDGELIST      is the type of edge list that will be assigned the list of parallel edges.
 * @param  G             is the input graph.
 * @param  parallelEdges is assigned the list of remaining edges in \p G that were part of a
 *                       bundle of parallel edges in the input graph.
 */
template <class EDGELIST>
void makeParallelFree(Graph &G, EDGELIST &parallelEdges)
{
	parallelEdges.clear();
	if (G.numberOfEdges() <= 1) return;

	SListPure<edge> edges;
	parallelFreeSort(G,edges);

	SListConstIterator<edge> it = edges.begin();
	edge ePrev = *it++, e;
	bool bAppend = true;
	while(it.valid()) {
		e = *it++;
		if (e->isParallelDirected(ePrev)) {
			G.delEdge(e);
			if (bAppend) { parallelEdges.pushBack(ePrev); bAppend = false; }
		} else {
			ePrev = e; bAppend = true;
		}
	}
}


//! Removes all but one edge of each bundle of parallel edges in \p G.
/**
 * @ingroup ga-multi
 *
 * A parallel edge is an edge e1=(v,w) such that there exists another edge e2=(v,w) in
 * the graph. Reversal edges (e.g. (v,w) and (w,v)) are not parallel edges. If you want to
 * remove parallel and reversal edges, use ogdf::makeParallelFreeUndirected().
 *
 * @param G is the input graph.
 */
inline void makeParallelFree(Graph &G) {
	List<edge> parallelEdges;
	makeParallelFree(G,parallelEdges);
}



//! Sorts the edges of \p G such that undirected parallel edges come after each other in the list.
/**
 * @ingroup ga-multi
 *
 * An undirected parallel edges is an edge e1=(v,w) such that there exists another edge e2=(v,w) or (w,v)
 * in the graph.
 *
 * @param G is the input graph.
 * @param edges is assigned the list of sorted edges.
 * @param minIndex is assigned for each edge (v,w) the index min(index(v),index(w)).
 * @param maxIndex is assigned for each edge (v,w) the index max(index(v),index(w)).
 */
OGDF_EXPORT void parallelFreeSortUndirected(
	const Graph &G,
	SListPure<edge> &edges,
	EdgeArray<int> &minIndex,
	EdgeArray<int> &maxIndex);


//! Returns true iff \p G contains no undirected parallel edges.
/**
 * @ingroup ga-multi
 *
 * An undirected parallel edges is an edge e1=(v,w) such that there exists another edge e2=(v,w) or (w,v)
 * in the graph.
 *
 * @param G is the input graph.
 * @return true if \p G contains no undirected parallel edges.
 */
OGDF_EXPORT bool isParallelFreeUndirected(const Graph &G);


//! Returns the number of undirected parallel edges in \p G.
/**
 * @ingroup ga-multi
 *
 * An undirected parallel edges is an edge e1=(v,w) such that there exists another edge e2=(v,w) or (w,v)
 * in the graph.
 *
 * @param G is the input graph.
 * @tparam ONLY_ONCE Whether the searching for multi-edges should be stopped
 * once a single multi-edge is found.
 * @return the number of undirected parallel edges; for each unordered pair {v,w} of nodes, all
 *         but one of the edges with endpoints v and w (in any order) are counted.
 */
template <bool ONLY_ONCE = false>
int numParallelEdgesUndirected(const Graph &G)
{
	if (G.numberOfEdges() <= 1) return 0;

	SListPure<edge> edges;
	EdgeArray<int> minIndex(G), maxIndex(G);
	parallelFreeSortUndirected(G,edges,minIndex,maxIndex);

	int num = 0;
	SListConstIterator<edge> it = edges.begin();
	edge ePrev = *it, e;
	for(it = ++it; it.valid(); ++it, ePrev = e) {
		e = *it;
		if (minIndex[ePrev] == minIndex[e] && maxIndex[ePrev] == maxIndex[e]) {
			++num;
			if (ONLY_ONCE) {
				return num;
			}
		}
	}

	return num;
}


//! Computes the bundles of undirected parallel edges in \p G.
/**
 * @ingroup ga-multi
 *
 * Stores for one (arbitrarily chosen) reference edge all edges belonging to the same bundle of
 * undirected parallel edges; no edge is removed from the graph.
 *
 * @tparam EDGELIST      is the type of edge list that is assigned the list of edges.
 * @param  G             is the input graph.
 * @param  parallelEdges is assigned for each reference edge the list of edges belonging to the
 *                       bundle of undirected parallel edges.
 */
template <class EDGELIST>
void getParallelFreeUndirected(const Graph &G, EdgeArray<EDGELIST> &parallelEdges)
{
	if (G.numberOfEdges() <= 1) {
		return;
	}

	SListPure<edge> edges;
	EdgeArray<int> minIndex(G), maxIndex(G);
	parallelFreeSortUndirected(G,edges,minIndex,maxIndex);

	SListConstIterator<edge> it = edges.begin();
	edge ePrev = *it++, e;
	while (it.valid()) {
		e = *it++;
		if (minIndex[ePrev] == minIndex[e] && maxIndex[ePrev] == maxIndex[e]) {
			parallelEdges[ePrev].pushBack(e);
		} else {
			ePrev = e;
		}
	}
}


//! Removes all but one edge of each bundle of undirected parallel edges.
/**
 * @ingroup ga-multi
 *
 * An undirected parallel edge is an edge e1=(v,w) such that there exists
 * another edge e2=(v,w) or (w,v) in the graph. This function removes all but
 * one of the edges with endpoints v and w for each unordered pair of nodes
 * {v,w}.
 *
 * @tparam EDGELIST      is the type of edge list that will be assigned the list of edges.
 * @param  G             is the input graph.
 * @param  parallelEdges is assigned the list of remaining edges that were part of a bundle
 *                       of undirected parallel edges in the input graph.
 * @param  cardPositive  contains for each edge the number of removed undirected parallel edges
 *                       pointing in the same direction.
 * @param  cardNegative  contains for each edge the number of removed undirected parallel edges
 *                       pointing in the opposite direction.
 */
template <class EDGELIST = SListPure<edge>>
void makeParallelFreeUndirected(
	Graph &G,
	EDGELIST *parallelEdges = nullptr,
	EdgeArray<int> *cardPositive = nullptr,
	EdgeArray<int> *cardNegative = nullptr)
{
	if (parallelEdges != nullptr) { parallelEdges->clear(); }
	if (cardPositive  != nullptr) { cardPositive->fill(0);  }
	if (cardNegative  != nullptr) { cardNegative->fill(0);  }

	if (G.numberOfEdges() <= 1) {
		return;
	}

	EdgeArray<SListPure<edge>> parEdges(G);
	getParallelFreeUndirected(G, parEdges);

	for (edge e : G.edges) {
		for (edge parEdge : parEdges(e)) {
			if (cardPositive != nullptr && e->source() == parEdge->source()) {
				(*cardPositive)[e]++;
			}
			if (cardNegative != nullptr && e->source() == parEdge->target()) {
				(*cardNegative)[e]++;
			}
			G.delEdge(parEdge);
			if (parallelEdges != nullptr) {
				parallelEdges->pushBack(e);
			}
		}
	}
}


/**
 * @ingroup ga-multi
 */
template <class EDGELIST>
OGDF_DEPRECATED("The pointer-based makeParallelFreeUndirected() should be used instead.")
void makeParallelFreeUndirected(Graph &G, EDGELIST &parallelEdges) {
	makeParallelFreeUndirected(G, &parallelEdges);
}

/**
 * @ingroup ga-multi
 */
template <class EDGELIST>
OGDF_DEPRECATED("The pointer-based makeParallelFreeUndirected() should be used instead.")
void makeParallelFreeUndirected(Graph &G,
		EDGELIST &parallelEdges,
		EdgeArray<int> &cardPositive,
		EdgeArray<int> &cardNegative) {
	makeParallelFreeUndirected(G, &parallelEdges, &cardPositive, &cardNegative);
}

//! @}
//! \name Methods for simple graphs
//! @{

//! Returns true iff \p G contains neither self-loops nor parallel edges.
/**
 * @ingroup ga-multi
 *
 * @param G is the input graph.
 * @return true if \p G is simple, i.e. contains neither self-loops nor parallel edges, false otherwise.
 */
inline bool isSimple(const Graph &G) {
	return isLoopFree(G) && isParallelFree(G);
}


//! Removes all self-loops and all but one edge of each bundle of parallel edges.
/**
 * @ingroup ga-multi
 *
 * @param G is the input graph.
 */
inline void makeSimple(Graph &G) {
	makeLoopFree(G);
	makeParallelFree(G);
}


//! Returns true iff \p G contains neither self-loops nor undirected parallel edges.
/**
 * @ingroup ga-multi
 *
 * @param G is the input graph.
 * @return true if \p G is (undirected) simple, i.e. contains neither self-loops
 *         nor undirected parallel edges, false otherwise.
 */
inline bool isSimpleUndirected(const Graph &G) {
	return isLoopFree(G) && isParallelFreeUndirected(G);
}


//! Removes all self-loops and all but one edge of each bundle of undirected parallel edges.
/**
 * @ingroup ga-multi
 *
 * @param G is the input graph.
 */
inline void makeSimpleUndirected(Graph &G) {
	makeLoopFree(G);
	makeParallelFreeUndirected(G);
}

//! @}
//! \name Methods for connectivity
//! @{

//! Returns true iff \p G is connected.
/**
 * @ingroup ga-connectivity
 *
 * @param G is the input graph.
 * @return true if \p G is connected, false otherwise.
 */
OGDF_EXPORT bool isConnected(const Graph &G);


//! Makes \p G connected by adding a minimum number of edges.
/**
 * @ingroup ga-connectivity
 *
 * @param G     is the input graph.
 * @param added is assigned the added edges.
 */
OGDF_EXPORT void makeConnected(Graph &G, List<edge> &added);


//! makes \p G connected by adding a minimum number of edges.
/**
 * @ingroup ga-connectivity
 *
 * @param G is the input graph.
 */
inline void makeConnected(Graph &G) {
	List<edge> added;
	makeConnected(G,added);
}


//! Computes the connected components of \p G and optionally generates a list of
//! isolated nodes.
/**
 * @ingroup ga-connectivity
 *
 * Assigns component numbers (0, 1, ...) to the nodes of \p G. The component
 * number of each node is stored in the node array \p component.
 *
 * @param G         is the input graph.
 * @param component is assigned a mapping from nodes to component numbers.
 * @param isolated  is assigned the list of isolated nodes. An isolated node is
 *                  a node without incident edges.
 * @return the number of connected components.
 */
OGDF_EXPORT int connectedComponents(const Graph &G,
		NodeArray<int> &component,
		List<node> *isolated = nullptr);


OGDF_DEPRECATED("connectedComponents() should be used instead.")
/**
 * @ingroup ga-connectivity
 * @copydoc ogdf::connectedComponents(const Graph&, NodeArray<int>&, List<node>*);
 */
inline int connectedIsolatedComponents(const Graph &G,
		List<node> &isolated,
		NodeArray<int> &component) {
	return connectedComponents(G, component, &isolated);
}


//! Returns true iff \p G is biconnected.
/**
 * @ingroup ga-connectivity
 *
 * @param G is the input graph.
 * @param cutVertex If false is returned and \p G is connected, \p cutVertex is
 *                  assigned a cut vertex in \p G, else it is assigned nullptr.
 */
OGDF_EXPORT bool isBiconnected(const Graph &G, node &cutVertex);


//! Returns true iff \p G is biconnected.
/**
 * @ingroup ga-connectivity
 *
 * @param G is the input graph.
 */
inline bool isBiconnected(const Graph &G) {
	node cutVertex;
	return isBiconnected(G,cutVertex);
}


//! Makes \p G biconnected by adding edges.
/**
 * @ingroup ga-connectivity
 *
 * @param G     is the input graph.
 * @param added is assigned the list of inserted edges.
 */
OGDF_EXPORT void makeBiconnected(Graph &G, List<edge> &added);


//! Makes \p G biconnected by adding edges.
/**
 * @ingroup ga-connectivity
 *
 * @param G is the input graph.
 */
inline void makeBiconnected(Graph &G) {
	List<edge> added;
	makeBiconnected(G,added);
}


/**
 * @ingroup ga-connectivity
 * @copydoc ogdf::biconnectedComponents(const Graph&, EdgeArray<int>&)
 * @param nonEmptyComponents is the number of non-empty components.
 * The indices of \p component range from 0 to \p nonEmptyComponents - 1.
 */
OGDF_EXPORT int biconnectedComponents(const Graph &G, EdgeArray<int> &component, int &nonEmptyComponents);

//! Computes the biconnected components of \p G.
/**
 * @ingroup ga-connectivity
 *
 * Assigns component numbers (0, 1, ...) to the edges of \p G. The component
 * number of each edge is stored in the edge array \p component. Each self-loop
 * is counted as one biconnected component and has its own component number.
 *
 * @param G         is the input graph.
 * @param component is assigned a mapping from edges to component numbers.
 * @return the number of biconnected components (including self-loops) + the
 * number of nodes without neighbours (that is, the number of nodes who have no
 * incident edges or whose incident edges are all self-loops).
 */
inline int biconnectedComponents(const Graph &G, EdgeArray<int> &component) {
	int doNotNeedTheValue;
	return biconnectedComponents(G, component, doNotNeedTheValue);
}


/**
 * @copydoc ogdf::isTwoEdgeConnected(const Graph&)
 * @param bridge If false is returned and \p graph is connected, \p bridge is assigned a bridge in \p graph,
 * else it is assigned \c nullptr
 */
OGDF_EXPORT bool isTwoEdgeConnected(const Graph &graph, edge &bridge);

/**
 * Returns true iff \p graph is 2-edge-connected.
 * @ingroup ga-connectivity
 *
 * Implementation of the algorithm to determine 2-edge-connectivity from the following publication:
 *
 * Jens M. Schmidt: <i>A Simple Test on 2-Vertex- and 2-Edge-Connectivity</i>.
 * Information Processing Letters (2013)
 *
 * It runs in O(|E|+|V|) as it relies on two DFS.
 *
 * @param graph is the input graph.
 */
inline bool isTwoEdgeConnected(const Graph &graph) {
	edge bridge;
	return isTwoEdgeConnected(graph, bridge);
}


//! Returns true iff \p G is triconnected.
/**
 * @ingroup ga-connectivity
 *
 * If \p G is not triconnected then
 *   - \p s1 and \p s2 are both \c nullptr if \p G is not connected.
 *   - \p s1 is a cut vertex and \p s2 is \c nullptr if \p G is connected but not biconnected.
 *   - \p s1 and \p s2 are a separation pair if \p G is bi- but not triconnected.
 *
 * @param G is the input graph.
 * @param s1 is assigned a cut vertex or one node of a separation pair, if \p G is not triconnected (see above).
 * @param s2 is assigned one node of a separation pair, if \p G is not triconnected (see above).
 * @return true if \p G is triconnected, false otherwise.
 */
OGDF_EXPORT bool isTriconnected(const Graph &G, node &s1, node &s2);


//! Returns true iff \p G is triconnected.
/**
 * @ingroup ga-connectivity
 *
 * @param G is the input graph.
 * @return true if \p G is triconnected, false otherwise.
 */
inline bool isTriconnected(const Graph &G) {
	node s1, s2;
	return isTriconnected(G,s1,s2);
}


//! Returns true iff \p G is triconnected (using a quadratic time algorithm!).
/**
 * @ingroup ga-connectivity
 *
 * If \p G is not triconnected then
 *   - \p s1 and \p s2 are both \c nullptr if \p G is not connected.
 *   - \p s1 is a cut vertex and \p s2 is \c nullptr if \p G is connected but not biconnected.
 *   - \p s1 and \p s2 are a separation pair if \p G is bi- but not triconnected.
 *
 * \warning This method has quadratic running time. An efficient linear time
 *          version is provided by isTriconnected().
 *
 * @param G is the input graph.
 * @param s1 is assigned a cut vertex of one node of a separation pair, if \p G is not triconnected (see above).
 * @param s2 is assigned one node of a separation pair, if \p G is not triconnected (see above).
 * @return true if \p G is triconnected, false otherwise.
 */
OGDF_EXPORT bool isTriconnectedPrimitive(const Graph &G, node &s1, node &s2);


//! Returns true iff \p G is triconnected (using a quadratic time algorithm!).
/**
 * @ingroup ga-connectivity
 *
 * \warning This method has quadratic running time. An efficient linear time
 *          version is provided by isTriconnected().
 *
 * @param G is the input graph.
 * @return true if \p G is triconnected, false otherwise.
 */
inline bool isTriconnectedPrimitive(const Graph &G) {
	node s1, s2;
	return isTriconnectedPrimitive(G,s1,s2);
}


//! Triangulates a planarly embedded graph \p G by adding edges.
/**
 * @ingroup ga-connectivity
 *
 * The result of this function is that \p G is made maximally planar by adding new edges.
 * \p G will also be planarly embedded such that each face is a triangle.
 *
 * \pre \p G is planar, simple and represents a combinatorial embedding (i.e. \p G is planarly embedded).
 *
 * @param G is the input graph to which edges will be added.
 */
void triangulate(Graph &G);


//! @}
//! \name Methods for directed graphs
//! @{

//! Returns true iff the digraph \p G is acyclic.
/**
 * @ingroup ga-digraph
 *
 * @param G         is the input graph
 * @param backedges is assigned the backedges of a DFS-tree.
 * @return true if \p G contains no directed cycle, false otherwise.
 */
OGDF_EXPORT bool isAcyclic(const Graph &G, List<edge> &backedges);


//! Returns true iff the digraph \p G is acyclic.
/**
 * @ingroup ga-digraph
 *
 * @param G is the input graph
 * @return true if \p G contains no directed cycle, false otherwise.
 */
inline bool isAcyclic(const Graph &G) {
	List<edge> backedges;
	return isAcyclic(G,backedges);
}


//! Returns true iff the undirected graph \p G is acyclic.
/**
 * @ingroup ga-digraph
 *
 * @param G         is the input graph
 * @param backedges is assigned the backedges of a DFS-tree.
 * @return true if \p G contains no undirected cycle, false otherwise.
 */
OGDF_EXPORT bool isAcyclicUndirected(const Graph &G, List<edge> &backedges);


//! Returns true iff the undirected graph \p G is acyclic.
/**
 * @ingroup ga-digraph
 *
 * @param G is the input graph
 * @return true if \p G contains no undirected cycle, false otherwise.
 */
inline bool isAcyclicUndirected(const Graph &G) {
	List<edge> backedges;
	return isAcyclicUndirected(G,backedges);
}


//! Makes the digraph \p G acyclic by removing edges.
/**
 * @ingroup ga-digraph
 *
 * The implementation removes all backedges of a DFS tree.
 *
 * @param G is the input graph
 */
OGDF_EXPORT void makeAcyclic(Graph &G);


//! Makes the digraph G acyclic by reversing edges.
/**
 * @ingroup ga-digraph
 *
 * \remark The implementation ignores self-loops and reverses the backedges of a DFS-tree.
 *
 * @param G is the input graph
 */
OGDF_EXPORT void makeAcyclicByReverse(Graph &G);


//! Returns true iff the digraph \p G contains exactly one source node (or is empty).
/**
 * @ingroup ga-digraph
 *
 * @param G      is the input graph.
 * @param source is assigned the single source if true is returned, or 0 otherwise.
 * @return true if \p G has a single source, false otherwise.
 */
OGDF_EXPORT bool hasSingleSource(const Graph &G, node &source);


//! Returns true iff the digraph \p G contains exactly one source node (or is empty).
/**
 * @ingroup ga-digraph
 *
 * @param G is the input graph.
 * @return true if \p G has a single source, false otherwise.
 */
inline bool hasSingleSource(const Graph &G) {
	node source;
	return hasSingleSource(G,source);
}


//! Returns true iff the digraph \p G contains exactly one sink node (or is empty).
/**
 * @ingroup ga-digraph
 *
 * @param G is the input graph.
 * @param sink is assigned the single sink if true is returned, or 0 otherwise.
 * @return true if \p G has a single sink, false otherwise.
 */
OGDF_EXPORT bool hasSingleSink(const Graph &G, node &sink);


//! Returns true iff the digraph \p G contains exactly one sink node (or is empty).
/**
 * @ingroup ga-digraph
 *
 * @param G is the input graph.
 * @return true if \p G has a single sink, false otherwise.
 */
inline bool hasSingleSink(const Graph &G) {
	node sink;
	return hasSingleSink(G,sink);
}


//! Returns true iff \p G is an st-digraph.
/**
 * @ingroup ga-digraph
 *
 * A directed graph is an st-digraph if it is acyclic, contains exactly one source s
 * and one sink t, and the edge (s,t).
 *
 * @param G  is the input graph.
 * @param s  is assigned the single source (if true is returned).
 * @param t  is assigned the single sink (if true is returned).
 * @param st is assigned the edge (s,t) (if true is returned).
 * @return true if \p G is an st-digraph, false otherwise.
 */
OGDF_EXPORT bool isStGraph(const Graph &G, node &s, node &t, edge &st);


//! Returns true if \p G is an st-digraph.
/**
 * @ingroup ga-digraph
 *
 * A directed graph is an st-digraph if it is acyclic, contains exactly one source s
 * and one sink t, and the edge (s,t).
 * @param G  is the input graph.
 * @return true if \p G is an st-digraph, false otherwise.
 */
inline bool isStGraph(const Graph &G) {
	node s, t;
	edge st;
	return isStGraph(G,s,t,st);
}


//! Computes a topological numbering of an acyclic digraph \p G.
/**
 * @ingroup ga-digraph
 *
 * \pre \p G is an acyclic directed graph.
 *
 * @param G   is the input graph.
 * @param num is assigned the topological numbering (0, 1, ...).
 */
OGDF_EXPORT void topologicalNumbering(const Graph &G, NodeArray<int> &num);


//! Computes the strongly connected components of the digraph \p G.
/**
 * @ingroup ga-connectivity
 *
 * The function implements the algorithm by Tarjan.
 *
 * @param G         is the input graph.
 * @param component is assigned a mapping from nodes to component numbers (0, 1, ...).
 * @return the number of strongly connected components.
 */
OGDF_EXPORT int strongComponents(const Graph& G, NodeArray<int>& component);


//! Makes the digraph \p G bimodal.
/**
 * @ingroup ga-digraph
 *
 * The implementation splits all non-bimodal vertices into two vertices.
 *
 * @param G is the input graph.
 * @param newEdges is the list containing the new edges.
 *
 */
OGDF_EXPORT void makeBimodal(Graph &G, List<edge> &newEdges);


//! Makes the digraph \p G bimodal.
/**
 * @ingroup ga-digraph
 *
 * The implementation splits all non-bimodal vertices into two vertices.
 *
 * @param G is the input graph.
 */
inline void makeBimodal(Graph &G) {
	List<edge> dummy;
	makeBimodal(G, dummy);
}


//! @}
//! \name Methods for trees and forests
//! @{

OGDF_DEPRECATED("isAcyclicUndirected() should be used instead.")
/**
 * @ingroup ga-tree
 * @copydoc ogdf::isAcyclicUndirected(const Graph &G)
 */
inline bool isFreeForest(const Graph &G) {
	return isAcyclicUndirected(G);
}


//! Returns true iff \p G is a tree, i.e. contains no undirected cycle and is connected
/**
 * @ingroup ga-tree
 *
 * @param G is the input graph.
 * @return true if \p G is a tree, false otherwise.
 */
inline bool isTree(const Graph &G)
{
	return G.empty() || ((G.numberOfNodes() == G.numberOfEdges() + 1) && isConnected(G));
}


//! Returns true iff \p G is a forest consisting only of arborescences.
/**
 * @ingroup ga-tree
 *
 * @param G is the input graph.
 * @param roots is assigned the list of root nodes of the arborescences in the forest.
 * If false is returned, \p roots is undefined.
 * @return true if \p G represents an arborescence forest, false otherwise.
 */
OGDF_EXPORT bool isArborescenceForest(const Graph& G, List<node> &roots);


//! Returns true iff \p G is a forest consisting only of arborescences.
/**
 * @ingroup ga-tree
 *
 * @param G is the input graph.
 * @return true if \p G represents an arborescence forest, false otherwise.
 */
inline bool isArborescenceForest(const Graph &G) {
	List<node> roots;
	return isArborescenceForest(G,roots);
}


OGDF_DEPRECATED("isArborescenceForest() should be used instead.")
/**
 * @ingroup ga-tree
 * @copydoc ogdf::isArborescenceForest(const Graph& G, List<node> &roots)
 */
inline bool isForest(const Graph& G, List<node> &roots) {
	return isArborescenceForest(G, roots);
}


OGDF_DEPRECATED("isArborescenceForest() should be used instead.")
/**
 * @ingroup ga-tree
 * @copydoc ogdf::isArborescenceForest(const Graph& G)
 */
inline bool isForest(const Graph &G) {
	return isArborescenceForest(G);
}


//! Returns true iff \p G represents an arborescence.
/**
 * @ingroup ga-tree
 *
 * @param G    is the input graph.
 * @param root is assigned the root node (if true is returned).
 * @return true if \p G represents an arborescence, false otherwise.
 */
OGDF_EXPORT bool isArborescence(const Graph& G, node &root);


//! Returns true iff \p G represents an arborescence.
/**
 * @ingroup ga-tree
 *
 * @param G  is the input graph.
 * @return true if \p G represents an arborescence, false otherwise.
 */
inline bool isArborescence(const Graph &G) {
	node root;
	return isArborescence(G,root);
}

//! @}

//! Checks if a graph is regular
/**
 * @param G is the input graph.
 * @return true if \p G is regular, false otherwise.
 */
OGDF_EXPORT bool isRegular(const Graph& G);


//! Checks if a graph is d-regular
/**
 * @param G is the input graph.
 * @param d is the vertex degree.
 * @return true if \p G is d-regular, false otherwise.
 */
OGDF_EXPORT bool isRegular(const Graph& G, int d);


//! Checks whether a graph is bipartite.
/**
 * @param G is the input graph.
 * @param color is assigned the color for each node, i.e. the partition it
 * belongs to, if G is bipartite. Otherwise its contents are undefined.
 * @return true if \p G is bipartite, false otherwise.
 */
OGDF_EXPORT bool isBipartite(const Graph &G, NodeArray<bool> &color);


//! Checks whether a graph is bipartite.
/**
 * @param G is the input graph.
 * @return true if \p G is bipartite, false otherwise.
 */
inline bool isBipartite(const Graph &G) {
	NodeArray<bool> color(G);
	return isBipartite(G, color);
}

/**
 * Fills \p dist with the distribution given by a function \p func
 * in graph \p G.
 *
 * The array \p dist is initialized such that
 * \c dist.low() represents the minimum function value, and
 * \c dist.high() represents the maximum function value.
 *
 * The resulting \p dist array contains for each function value \a x
 * the number \a n of nodes that yield this function value \a x.
 * In that case, the value at index \a x of \p dist is \a n.
 * Also note that because \p dist is an array, all intermediate
 * values are 0.
 *
 * Examples:
 *   - Getting the in-degree distribution:
 *     \code
 *       Array<int> indegDist;
 *       nodeDistribution(G, indegDist, [](node v) { return v->indeg(); });
 *     \endcode
 *   - Getting the number of nodes belonging to specific connected components:
 *     \code
 *       NodeArray<int> component(G);
 *       Array<int> compDist;
 *       connectedComponents(G, component);
 *       nodeDistribution(G, compDist, component);
 *     \endcode
 *
 * @see ogdf::degreeDistribution
 */
OGDF_EXPORT void nodeDistribution(const Graph& G, Array<int> &degdist, std::function<int(node)> func);

/**
 * Fills \p degdist with the degree distribution of graph \p G.
 *
 * @see ogdf::nodeDistribution
 */
inline void degreeDistribution(const Graph& G, Array<int> &degdist) {
	nodeDistribution(G, degdist, [](node v) {
		return v->degree();
	});
}

}
