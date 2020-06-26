/** \file
 * \brief Declaration of extended graph algorithms
 *
 * \author Sebastian Leipert, Karsten Klein, Markus Chimani
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

#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/basic/PriorityQueue.h>
#include <ogdf/basic/DisjointSets.h>
#include <ogdf/planarity/BoyerMyrvold.h>


namespace ogdf {

//! \name Methods for induced subgraphs
//! @{

//! Computes the subgraph induced by a list of nodes.
/**
 * @ingroup ga-induced
 *
 * @tparam NODELISTITERATOR is the type of iterators for the input list of nodes.
 * @param G        is the input graph.
 * @param start    is a list iterator pointing to the first element in a list of nodes, for which
 *                 an induced subgraph shall be computed.
 * @param subGraph is assigned the computed subgraph.
 */
template<class LISTITERATOR>
void inducedSubGraph(const Graph &G, LISTITERATOR start, Graph &subGraph)
{
	NodeArray<node> nodeTableOrig2New;
	inducedSubGraph(G,start,subGraph,nodeTableOrig2New);
}

//! Computes the subgraph induced by a list of nodes (plus a mapping from original nodes to new copies).
/**
 * @ingroup ga-induced
 *
 * @tparam NODELISTITERATOR is the type of iterators for the input list of nodes.
 * @param G        is the input graph.
 * @param start    is a list iterator pointing to the first element in a list of nodes, for which
 *                 an induced subgraph shall be computed.
 * @param subGraph is assigned the computed subgraph.
 * @param nodeTableOrig2New is assigned a mapping from the nodes in \p G to the nodes in \p subGraph.
 */
template<class LISTITERATOR>
void inducedSubGraph(
	const Graph &G,
	LISTITERATOR start,
	Graph &subGraph,
	NodeArray<node> &nodeTableOrig2New)
{
	subGraph.clear();
	nodeTableOrig2New.init(G,nullptr);

	EdgeArray<bool> mark(G,false);

	LISTITERATOR its;
	for (its = start; its.valid(); its++)
	{
		node w = (*its);
		OGDF_ASSERT(w != nullptr);
		OGDF_ASSERT(w->graphOf() == &G);
		nodeTableOrig2New[w] = subGraph.newNode();

		for(adjEntry adj : w->adjEntries)
		{
			edge e = adj->theEdge();
			if (nodeTableOrig2New[e->source()] && nodeTableOrig2New[e->target()] && !mark[e])
			{
				subGraph.newEdge(nodeTableOrig2New[e->source()],nodeTableOrig2New[e->target()]);
				mark[e] = true;
			}
		}
	}
}


//! Computes the subgraph induced by a list of nodes (plus mappings from original nodes and edges to new copies).
/**
 * @ingroup ga-induced
 *
 * @tparam NODELISTITERATOR is the type of iterators for the input list of nodes.
 * @param G        is the input graph.
 * @param start    is a list iterator pointing to the first element in a list of nodes, for which
 *                 an induced subgraph shall be computed.
 * @param subGraph is assigned the computed subgraph.
 * @param nodeTableOrig2New is assigned a mapping from the nodes in \p G to the nodes in \p subGraph.
 * @param edgeTableOrig2New is assigned a mapping from the edges in \p G to the egdes in \p subGraph.
 */
template<class LISTITERATOR>
void inducedSubGraph(
	const Graph &G,
	LISTITERATOR start,
	Graph &subGraph,
	NodeArray<node> &nodeTableOrig2New,
	EdgeArray<edge> &edgeTableOrig2New)
{
	subGraph.clear();
	nodeTableOrig2New.init(G,nullptr);
	edgeTableOrig2New.init(G,nullptr);

	EdgeArray<bool> mark(G,false);

	LISTITERATOR its;
	for (its = start; its.valid(); its++)
	{
		node w = (*its);
		OGDF_ASSERT(w != nullptr);
		OGDF_ASSERT(w->graphOf() == &G);
		nodeTableOrig2New[w] = subGraph.newNode();

		for(adjEntry adj : w->adjEntries)
		{
			edge e = adj->theEdge();
			if (nodeTableOrig2New[e->source()] &&
				nodeTableOrig2New[e->target()] &&
				!mark[e])
			{
				edgeTableOrig2New[e] =
					subGraph.newEdge(
						nodeTableOrig2New[e->source()],
						nodeTableOrig2New[e->target()]);
				mark[e] = true;
			}
		}
	}
}


//! Computes the edges in a node-induced subgraph.
/**
 * @ingroup ga-induced
 *
 * @tparam NODELISTITERATOR is the type of iterators for the input list of nodes.
 * @tparam EDGELIST         is the type of the returned edge list.
 * @param  G  is the input graph.
 * @param  it is a list iterator pointing to the first element in a list of nodes, whose
 *            induced subgraph is considered.
 * @param  E  is assigned the list of edges in the node-induced subgraph.
 */
template<class NODELISTITERATOR, class EDGELIST>
void inducedSubgraph(Graph &G, NODELISTITERATOR &it, EDGELIST &E)
{
	NODELISTITERATOR itBegin = it;
	NodeArray<bool>  mark(G,false);

	for (;it.valid();it++)
		mark[*it] = true;
	it = itBegin;
	for (;it.valid();it++)
	{
		node v = (*it);
		for(adjEntry adj : v->adjEntries)
		{
			edge e = adj->theEdge();
			if (mark[e->source()] && mark[e->target()])
				E.pushBack(e);
		}
	}
}


//! @}
//! \name Methods for clustered graphs
//! @{

//! Returns true iff cluster graph \p C is c-connected.
/**
 * @ingroup ga-connectivity
 */
OGDF_EXPORT bool isCConnected(const ClusterGraph &C);

//! Makes a cluster graph c-connected by adding edges.
/**
 * @ingroup ga-connectivity
 *
 * @param C is the input cluster graph.
 * @param G is the graph associated with the cluster graph \p C; the function adds new edges to this graph.
 * @param addedEdges is assigned the list of newly created edges.
 * @param simple selects the method used: If set to true, a simple variant that does not guarantee to preserve
 *        planarity is used.
 */
OGDF_EXPORT void makeCConnected(
	ClusterGraph& C,
	Graph& G,
	List<edge>& addedEdges,
	bool simple = true);


//! @}
//! \name Methods for minimum spanning tree computation
//! @{

//! Computes a minimum spanning tree using Prim's algorithm
/**
 * @ingroup ga-mst
 *
 * @tparam T        is the numeric type for edge weights.
 * @param  G        is the input graph.
 * @param  weight   is an edge array with the edge weights.
 * @param  isInTree is assigned the result, i.e. \a isInTree[\a e] is true iff edge \a e is in the computed MST.
 * @return the sum of the edge weights in the computed tree.
 **/
template<typename T>
inline T computeMinST(const Graph &G, const EdgeArray<T> &weight, EdgeArray<bool> &isInTree)
{
	NodeArray<edge> pred(G, nullptr);
	return computeMinST(G.firstNode(), G, weight, pred, isInTree);
}

//! Computes a minimum spanning tree (MST) using Prim's algorithm
/**
 * @ingroup ga-mst
 *
 * @tparam T        is the numeric type for edge weights.
 * @param  G        is the input graph.
 * @param  weight   is an edge array with the edge weights.
 * @param  isInTree is assigned the result, i.e. \a isInTree[\a e] is true iff edge \a e is in the computed MST.
 * @param  pred     is assigned for each node the edge from its parent in the MST.
 * @return the sum of the edge weights in the computed tree.
 **/
template<typename T>
inline T computeMinST(const Graph &G, const EdgeArray<T> &weight, NodeArray<edge> &pred, EdgeArray<bool> &isInTree)
{
	return computeMinST(G.firstNode(), G, weight, pred, isInTree);
}

//! Computes a minimum spanning tree (MST) using Prim's algorithm
/**
 * @tparam T        is the numeric type for edge weights.
 * @param  G        is the input graph.
 * @param  weight   is an edge array with the edge weights.
 * @param  pred     is assigned for each node the edge from its parent in the MST.
 **/
template<typename T>
inline void computeMinST(const Graph &G, const EdgeArray<T> &weight, NodeArray<edge> &pred)
{
	computeMinST(G.firstNode(), G, weight, pred);
}

//! Computes a minimum spanning tree (MST) using Prim's algorithm
/**
 * @ingroup ga-mst
 *
 * @tparam T        is the numeric type for edge weights.
 * @param  s        is the start node for Prim's algorithm and will be the root of the MST.
 * @param  G        is the input graph.
 * @param  weight   is an edge array with the edge weights.
 * @param  pred     is assigned for each node the edge from its parent in the MST.
 **/
template<typename T>
void computeMinST(node s, const Graph &G, const EdgeArray<T> &weight, NodeArray<edge> &pred)
{
	PrioritizedMapQueue<node, T> pq(G); // priority queue of front vertices

	// insert start node
	T tmp(0);
	pq.push(s, tmp);

	// extract the nodes again along a minimum ST
	NodeArray<bool> processed(G, false);
	pred.init(G, nullptr);

	while (!pq.empty()) {
		const node v = pq.topElement();
		pq.pop();
		processed[v] = true;
		for (adjEntry adj = v->firstAdj(); adj; adj = adj->succ()) {
			const node w = adj->twinNode();
			const edge e = adj->theEdge();
			if (pred[w] == nullptr && w != s) {
				tmp = weight[e];
				pq.push(w, tmp);
				pred[w] = e;
			} else
			if (!processed[w]
			 && weight[e] < pq.priority(w)) {
				pq.decrease(w, weight[e]);
				pred[w] = e;
			}
		}
	}
}

//! Computes a minimum spanning tree (MST) using Prim's algorithm
/**
 * @tparam T        is the numeric type for edge weights.
 * @param  s        is the start node for Prim's algorithm and will be the root of the MST.
 * @param  G        is the input graph.
 * @param  weight   is an edge array with the edge weights.
 * @param  pred     is assigned for each node the edge from its parent in the MST.
 * @param  isInTree is assigned the result, i.e. \a isInTree[\a e] is true iff edge \a e is in the computed MST.
 * @return the sum of the edge weights in the computed tree.
 **/
template<typename T>
T computeMinST(node s, const Graph &G, const EdgeArray<T> &weight, NodeArray<edge> &pred, EdgeArray<bool> &isInTree)
{
	computeMinST(s, G, weight, pred);

	// now just compute isInTree and total weight
	int rootcount = 0;
	T treeWeight = 0;
	isInTree.init(G, false);
	for (node v = G.firstNode(); v; v = v->succ()) {
		if (!pred[v]) {
			++rootcount;
		} else {
			isInTree[pred[v]] = true;
			treeWeight += weight[pred[v]];
		}
	}
	OGDF_ASSERT(rootcount == 1); // is connected

	return treeWeight;
}

//! Reduce a graph to its minimum spanning tree (MST) using Kruskal's algorithm
/**
 * @ingroup ga-mst
 *
 * @tparam T        is the numeric type for edge weights.
 * @param  G        is the input graph.
 * @param  weight   is an edge array with the edge weights.
 * @return the sum of the edge weights in the computed tree.
 **/
template<typename T>
T makeMinimumSpanningTree(Graph &G, const EdgeArray<T> &weight)
{
	T total(0);
	Array<Prioritized<edge, T>> sortEdges(G.numberOfEdges());
	int i = 0;
	for (edge e : G.edges) {
		sortEdges[i++] = Prioritized<edge,T>(e, weight[e]);
	}
	sortEdges.quicksort();

	// now let's do Kruskal's algorithm
	NodeArray<int> setID(G);
	DisjointSets<> uf(G.numberOfNodes());
	for (node v : G.nodes) {
		setID[v] = uf.makeSet();
	}

	for (auto prioEdge : sortEdges) {
		const edge e = prioEdge.item();
		const int v = setID[e->source()];
		const int w = setID[e->target()];
		if (uf.find(v) != uf.find(w)) {
			uf.link(uf.find(v), uf.find(w));
			total += weight[e];
		} else {
			G.delEdge(e);
		}
	}
	return total;
}

//! @}

//! Returns true, if G is planar, false otherwise.
/**
 * @ingroup ga-planembed
 *
 * This is a shortcut for BoyerMyrvold::isPlanar().
 *
 * @param G is the input graph.
 * @return true if \p G is planar, false otherwise.
 */
inline bool isPlanar(const Graph &G) {
	return BoyerMyrvold().isPlanar(G);
}

/**
 * Returns whether G is s-t-planar (i.e. it can be planarly embedded with s and t sharing a face).
 *
 * @param graph The graph to be tested
 * @param s The node to be incident to the same face as t nodes
 * @param t The other node
 *
 * @return true iff the graph is s-t-planar
 */
inline bool isSTPlanar(
  const Graph &graph,
  const node s,
  const node t)
{
	OGDF_ASSERT(s != nullptr);
	OGDF_ASSERT(t != nullptr);
	OGDF_ASSERT(s->graphOf() == &graph);
	OGDF_ASSERT(t->graphOf() == &graph);

	GraphCopy copy(graph);
	copy.newEdge(copy.copy(s), copy.copy(t));

	return isPlanar(copy);
}

//! Returns true, if G is planar, false otherwise. If true is returned, G will be planarly embedded.
/**
 * @ingroup ga-planembed
 *
 * This is a shortcut for BoyerMyrvold::planarEmbed
 *
 * @param G is the input graph.
 * @return true if \p G is planar, false otherwise.
 */
inline bool planarEmbed(Graph &G) {
	return BoyerMyrvold().planarEmbed(G);
}

/**
 * s-t-planarly embeds a graph.
 *
 * @param graph The graph to be embedded
 * @param s The node to be incident to the same face as t nodes
 * @param t The other node
 *
 * @return true iff the graph was successfully embedded
 */
inline bool planarSTEmbed(Graph &graph, node s, node t)
{
	edge e = graph.newEdge(s, t);
	bool result = planarEmbed(graph);
	graph.delEdge(e);

	return result;
}


//! Constructs a planar embedding of G. It assumes that \p G is planar!
/**
 * @ingroup ga-planembed
 *
 * This routine is slightly faster than planarEmbed(), but requires \p G to be planar.
 * If \p G is not planar, the graph will be destroyed while trying to embed it!
 *
 * This is a shortcut for BoyerMyrvold::planarEmbedPlanarGraph().
 *
 * @param G is the input graph.
 * @return true if the embedding was successful; false, if the given graph was non-planar (in this case
 *         the graph will be left in an at least partially deleted state).
 *
 */
inline bool planarEmbedPlanarGraph(Graph &G) {
	return BoyerMyrvold().planarEmbedPlanarGraph(G);
}

}
