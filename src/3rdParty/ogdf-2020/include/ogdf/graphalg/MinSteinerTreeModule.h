/** \file
 * \brief Declaration of ogdf::MinSteinerTreeModule
 *
 * \author Matthias Woste, Stephan Beyer
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

#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/PriorityQueue.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/energybased/FMMMLayout.h>
#include <ogdf/graphalg/Dijkstra.h>
#include <ogdf/graphalg/steiner_tree/Triple.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/graphalg/AStarSearch.h>
#include <sstream>

namespace ogdf {

/**
 * Serves as an interface for various methods to
 * compute or approximate minimum Steiner trees
 * on undirected graphs with edge costs.
 *
 * Furthermore it supplies some auxiliary methods.
 *
 * @tparam T The type of the edge costs of the Steiner tree instance
 */
template<typename T>
class MinSteinerTreeModule {
public:
	//! Do nothing on destruction
	virtual ~MinSteinerTreeModule() { }

	/**
	 * Calls the Steiner tree algorithm for nontrivial cases
	 * but handles trivial cases directly.
	 *
	 * @param G The weighted input graph
	 * @param terminals The list of terminal nodes
	 * @param isTerminal A bool array of terminals
	 * @param finalSteinerTree The final Steiner tree
	 * @return The total cost of the final Steiner tree
	 */
	virtual T call(const EdgeWeightedGraph<T> &G,
			const List<node> &terminals,
			const NodeArray<bool> &isTerminal,
			EdgeWeightedGraphCopy<T> *&finalSteinerTree
			);

	//! @name Auxiliary post-processing functions
	//! @{

	/**
	 * Prunes nonterminal leaves and their paths to terminal or branching nodes
	 *
	 * @param steinerTree The given Steiner tree
	 * @param isTerminal Incidence vector indicating terminal nodes
	 * @return The total cost of the removed edges (achieved improvement)
	 */
	static T pruneAllDanglingSteinerPaths(EdgeWeightedGraphCopy<T> &steinerTree, const NodeArray<bool> &isTerminal);

	/**
	 * Prunes the dangling Steiner path beginning at a given nonterminal leaf only
	 *
	 * @param steinerTree The given Steiner tree
	 * @param isTerminal Incidence vector indicating terminals
	 * @param start A nonterminal leaf to start pruning at
	 * @return The total cost of the removed edges (achieved improvement)
	 */
	static T pruneDanglingSteinerPathFrom(EdgeWeightedGraphCopy<T> &steinerTree, const NodeArray<bool> &isTerminal, node start);

	/**
	 * Prunes dangling Steiner paths beginning at given nonterminal leaves only
	 *
	 * @see pruneDanglingSteinerPathFrom(EdgeWeightedGraphCopy<T> &steinerTree, const NodeArray<bool> &isTerminal, node start)
	 */
	static T pruneDanglingSteinerPathsFrom(EdgeWeightedGraphCopy<T> &steinerTree, const NodeArray<bool> &isTerminal, const List<node> &start);

	/**
	 * Remove remaining cycles from a Steiner "almost" tree
	 *
	 * @return The edge weights of the removed edges (achieved improvement)
	 */
	static T removeCyclesFrom(EdgeWeightedGraphCopy<T> &steinerTree, const NodeArray<bool> &isTerminal);

	//! @}
	//! @name Special SSSP and APSP algorithms used in component-based approximation algorithms
	//! @{

	/**
	 * Modified single-source-shortest-paths (%Dijkstra)
	 * with heuristic to prefer paths over terminals
	 *
	 * A shortest path over a terminal will mark the nodes that
	 * come after that terminal as unreachable by setting the predecessor
	 * to \c nullptr.
	 * Nevertheless, the distance will be set correctly.
	 *
	 * @param G Input graph
	 * @param source Start terminal
	 * @param isTerminal Incidence vector indicating terminal nodes
	 * @param distance Distance matrix result
	 * @param pred Resulting shortest path such that \p pred[s][t] contains last edge of an s-t-path
	 */
	static void singleSourceShortestPathsPreferringTerminals(const EdgeWeightedGraph<T> &G, node source, const NodeArray<bool> &isTerminal, NodeArray<T> &distance, NodeArray<edge> &pred);

	//! Standard single-source-shortest-paths algoritm (%Dijkstra)
	static void singleSourceShortestPathsStandard(const EdgeWeightedGraph<T>& G, node source, const NodeArray<bool>&, NodeArray<T>& distance, NodeArray<edge>& pred) {
		Dijkstra<T> sssp;
		sssp.call(G, G.edgeWeights(), source, pred, distance);
	}

	//! The default single-source-shortest-paths algorithm
	//! @sa #singleSourceShortestPathsPreferringTerminals, #singleSourceShortestPathsStandard
	static inline void singleSourceShortestPaths(const EdgeWeightedGraph<T>& G, node source,
			const NodeArray<bool>& isTerminal, NodeArray<T>& distance, NodeArray<edge>& pred) {
#ifdef OGDF_MINSTEINERTREEMODULE_SHORTEST_PATHS_STANDARD
		singleSourceShortestPathsStandard(G, source, isTerminal, distance, pred);
#else
		singleSourceShortestPathsPreferringTerminals(G, source, isTerminal, distance, pred);
#endif
	}

	//! Runs #singleSourceShortestPathsStandard from all terminals
	static void allTerminalShortestPathsStandard(
			const EdgeWeightedGraph<T> &G,
			const List<node> &terminals,
			const NodeArray<bool> &isTerminal,
			NodeArray<NodeArray<T>> &distance,
			NodeArray<NodeArray<edge>> &pred)
	{
		allTerminalShortestPaths(G, terminals, isTerminal, distance, pred, singleSourceShortestPathsStandard);
	}

	//! Runs #singleSourceShortestPathsPreferringTerminals from all terminals
	static void allTerminalShortestPathsPreferringTerminals(
			const EdgeWeightedGraph<T> &G,
			const List<node> &terminals,
			const NodeArray<bool> &isTerminal,
			NodeArray<NodeArray<T>> &distance,
			NodeArray<NodeArray<edge>> &pred)
	{
		allTerminalShortestPaths(G, terminals, isTerminal, distance, pred, singleSourceShortestPathsPreferringTerminals);
	}

	//! Runs a given (or the default) single-source-shortest-paths function from all terminals
	static void allTerminalShortestPaths(
			const EdgeWeightedGraph<T> &G,
			const List<node> &terminals,
			const NodeArray<bool> &isTerminal,
			NodeArray<NodeArray<T>> &distance,
			NodeArray<NodeArray<edge>> &pred,
			std::function<void(const EdgeWeightedGraph<T>&, node, const NodeArray<bool>&, NodeArray<T>&, NodeArray<edge>&)> ssspFunc = singleSourceShortestPaths) {
		allNodesByListShortestPaths(G, terminals, isTerminal, terminals, distance, pred, ssspFunc);
	}

	//! Runs #singleSourceShortestPathsStandard from all nodes
	static void allNodeShortestPathsStandard(
			const EdgeWeightedGraph<T> &G,
			const List<node> &terminals,
			const NodeArray<bool> &isTerminal,
			NodeArray<NodeArray<T>> &distance,
			NodeArray<NodeArray<edge>> &pred) {
		allNodeShortestPaths(G, terminals, isTerminal, distance, pred, singleSourceShortestPathsStandard);
	}

	//! Runs #singleSourceShortestPathsPreferringTerminals from all nodes
	static void allNodeShortestPathsPreferringTerminals(
			const EdgeWeightedGraph<T> &G,
			const List<node> &terminals,
			const NodeArray<bool> &isTerminal,
			NodeArray<NodeArray<T>> &distance,
			NodeArray<NodeArray<edge>> &pred) {
		allNodeShortestPaths(G, terminals, isTerminal, distance, pred, singleSourceShortestPathsPreferringTerminals);
	}

	//! Runs a given (or the default) single-source-shortest-paths function from all nodes
	static void allNodeShortestPaths(
			const EdgeWeightedGraph<T> &G,
			const List<node> &terminals,
			const NodeArray<bool> &isTerminal,
			NodeArray<NodeArray<T>> &distance,
			NodeArray<NodeArray<edge>> &pred,
			std::function<void(const EdgeWeightedGraph<T>&, node, const NodeArray<bool>&, NodeArray<T>&, NodeArray<edge>&)> ssspFunc = singleSourceShortestPaths) {
		allNodesByListShortestPaths(G, terminals, isTerminal, G.nodes, distance, pred, ssspFunc);
	}

	/**
	 * Modified all-pair-shortest-paths algorithm (%Floyd-Warshall)
	 * with heuristic to prefer paths over terminals
	 *
	 * @param G Input graph
	 * @param isTerminal Incidence vector indicating terminal nodes
	 * @param distance Distance matrix result
	 * @param pred Resulting shortest path such that \p pred[s][t] contains last edge of an s-t-path
	 */
	static void allPairShortestPathsPreferringTerminals(const EdgeWeightedGraph<T> &G, const NodeArray<bool> &isTerminal, NodeArray<NodeArray<T>> &distance, NodeArray<NodeArray<edge>> &pred);

	//! Standard all-pair-shortest-paths algorithm (%Floyd-Warshall)
	static void allPairShortestPathsStandard(const EdgeWeightedGraph<T>& G, const NodeArray<bool>&, NodeArray<NodeArray<T>>& distance, NodeArray<NodeArray<edge>>& pred);

	//! The default all-pair-shortest-paths algorithm
	//! @sa #allPairShortestPathsPreferringTerminals, #allPairShortestPathsStandard
	static void allPairShortestPaths(const EdgeWeightedGraph<T>& G, const NodeArray<bool>& isTerminal, NodeArray<NodeArray<T>>& distance, NodeArray<NodeArray<edge>>& pred) {
#ifdef OGDF_MINSTEINERTREEMODULE_SHORTEST_PATHS_STANDARD
		allPairShortestPathsStandard(G, isTerminal, distance, pred);
#else
		allPairShortestPathsPreferringTerminals(G, isTerminal, distance, pred);
#endif
	}

	//! @}
	//! @name Drawings for debugging
	//! @{

	/**
	 * Writes an SVG file of a minimum Steiner tree in the original graph
	 *
	 * @param G The original weighted graph
	 * @param isTerminal Incidence vector indicating terminal nodes
	 * @param steinerTree The Steiner tree of the given graph
	 * @param filename The name of the output file
	 */
	static void drawSVG(const EdgeWeightedGraph<T> &G, const NodeArray<bool> &isTerminal, const EdgeWeightedGraphCopy<T> &steinerTree, const char *filename);

	/**
	 * Writes an SVG file of the instance graph
	 *
	 * @param G The weighted graph instance
	 * @param isTerminal Incidence vector indicating terminal nodes
	 * @param filename The name of the output file
	 */
	static void drawSVG(const EdgeWeightedGraph<T> &G, const NodeArray<bool> &isTerminal, const char *filename) {
		EdgeWeightedGraphCopy<T> emptySteinerTree;
		emptySteinerTree.createEmpty(G);
		drawSVG(G, isTerminal, emptySteinerTree, filename);
	}

	/**
	 * Writes a SVG that shows only the given Steiner tree
	 *
	 * @param steinerTree The Steiner tree to be drawn
	 * @param isTerminal Incidence vector indicating terminal nodes
	 * @param filename The name of the output file
	 */
	static void drawSteinerTreeSVG(const EdgeWeightedGraphCopy<T> &steinerTree, const NodeArray<bool> &isTerminal, const char *filename);

	//! @}

	/**
	 * Checks in O(n) time if a given tree is acually a Steiner Tree
	 *
	 * @param G The original graph
	 * @param terminals The list of terminal nodes
	 * @param isTerminal A bool array of terminals
	 * @param steinerTree The Steiner tree to be checked
	 * @return true iff the given Steiner tree is actually one, false otherwise
	 */
	static bool isSteinerTree(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, const EdgeWeightedGraphCopy<T> &steinerTree);

	/**
	 * Checks in O(n + m) time if a given Steiner tree problem instance is quasi-bipartite
	 *
	 * @param G The original graph
	 * @param isTerminal A bool array of terminals
	 * @return true iff the given Steiner tree problem instance is quasi-bipartite
	 */
	static bool isQuasiBipartite(const EdgeWeightedGraph<T> &G, const NodeArray<bool> &isTerminal);

	/**
	 * Generates a list (as List<node>) of all terminals
	 *
	 * @param terminals The returned list (terminals are appended)
	 * @param G The weighted input graph
	 * @param isTerminal A bool array of terminals
	 */
	static inline void getTerminals(List<node> &terminals, const EdgeWeightedGraph<T> &G, const NodeArray<bool> &isTerminal)
	{
		for (node v : G.nodes) {
			if (isTerminal[v]) {
				terminals.pushBack(v);
			}
		}
	}

	//! Sort terminals by index
	static inline void sortTerminals(List<node> &terminals)
	{
		terminals.quicksort(GenericComparer<node, int>([](node v) { return v->index(); }));
	}

	/**
	 * Generates a list (as ArrayBuffer<node>) of all nonterminals
	 *
	 * @param nonterminals The returned list (nonterminals are appended)
	 * @param G The weighted input graph
	 * @param isTerminal A bool array of terminals
	 */
	static inline void getNonterminals(ArrayBuffer<node> &nonterminals, const EdgeWeightedGraph<T> &G, const NodeArray<bool> &isTerminal)
	{
		for (node v : G.nodes) {
			if (!isTerminal[v]) {
				nonterminals.push(v);
			}
		}
	}

protected:
	/**
	 * Computes the actual Steiner tree
	 *
	 * @return The total cost of the final Steiner tree
	 */
	virtual T computeSteinerTree(const EdgeWeightedGraph<T> &G,
	    const List<node> &terminals,
	    const NodeArray<bool> &isTerminal,
	    EdgeWeightedGraphCopy<T> *&finalSteinerTree
	  ) = 0;

private:
	//! Common initialization routine for APSP algorithms
	static void apspInit(const EdgeWeightedGraph<T> &G, NodeArray<NodeArray<T>> &distance, NodeArray<NodeArray<edge>> &pred);
	//! Common initialization routine for SSSP algorithms
	static void ssspInit(const EdgeWeightedGraph<T> &G, node source, PrioritizedMapQueue<node, T> &queue, NodeArray<T> &distance, NodeArray<edge> &pred);

	//! The inner loop for APSP algorithm to avoid code duplication
	inline static void apspInnerLoop(node v, const EdgeWeightedGraph<T> &G, NodeArray<NodeArray<T>> &distance, std::function<void(node, node, T)> func)
	{
		for (node u : G.nodes) {
			const T duv = distance[u][v];
			if (duv < std::numeric_limits<T>::max()) {
				for (node w = u->succ(); w != nullptr; w = w->succ()) {
					if (distance[v][w] < std::numeric_limits<T>::max()) {
						func(u, w, duv + distance[v][w]);
					}
				}
			}
		}
	}

	//! Runs a given (or the default) single-source-shortest-paths function from all \p nodes
	template<typename NODELIST>
	inline static void allNodesByListShortestPaths(
			const EdgeWeightedGraph<T> &G,
			const List<node> &terminals,
			const NodeArray<bool> &isTerminal,
			const NODELIST& nodes,
			NodeArray<NodeArray<T>> &distance,
			NodeArray<NodeArray<edge>> &pred,
			std::function<void(const EdgeWeightedGraph<T>&, node, const NodeArray<bool>&, NodeArray<T>&, NodeArray<edge>&)> ssspFunc) {
		distance.init(G);
		pred.init(G);
		for (node u : nodes) {
			ssspFunc(G, u, isTerminal, distance[u], pred[u]);
		}
	}
};

template<typename T>
T MinSteinerTreeModule<T>::call(const EdgeWeightedGraph<T> &G,
			const List<node> &terminals,
			const NodeArray<bool> &isTerminal,
			EdgeWeightedGraphCopy<T> *&finalSteinerTree
			)
{
	OGDF_ASSERT(isConnected(G));

	if (terminals.size() > 2) {
		return this->computeSteinerTree(G, terminals, isTerminal, finalSteinerTree);
	}

	finalSteinerTree = new EdgeWeightedGraphCopy<T>();
	finalSteinerTree->createEmpty(G);
	if (!terminals.empty()) {
		finalSteinerTree->newNode(terminals.back());
	}
	if (terminals.size() <= 1) {
		return 0;
	}

	OGDF_ASSERT(terminals.size() == 2);
	T cost(0);
	AStarSearch<T> astar;
	NodeArray<edge> pred;
	NodeArray<T> dist;
	astar.call(G, G.edgeWeights(), terminals.front(), terminals.back(), pred);
	OGDF_ASSERT(pred[terminals.back()] != nullptr); // connected
	for (node t = terminals.back(); t != terminals.front(); t = pred[t]->opposite(t)) {
		const edge e = pred[t];
		finalSteinerTree->newNode(e->opposite(t));
		finalSteinerTree->newEdge(e, G.weight(e));
		cost += G.weight(e);
	}
	return cost;
}

template<typename T>
bool MinSteinerTreeModule<T>::isSteinerTree(
	const EdgeWeightedGraph<T> &G,
	const List<node> &terminals,
	const NodeArray<bool> &isTerminal,
	const EdgeWeightedGraphCopy<T> &steinerTree)
{
	// the Steiner tree is actually a tree
	if (!isTree(steinerTree)) {
		return false;
	}

	// all terminal nodes are in the graph and have degree >= 1
	for(node v : terminals) {
		const node u = steinerTree.copy(v);
		if (!u || (terminals.size() > 1 && u->degree() < 1)) {
			return false;
		}
	}

	// all Steiner nodes are inner nodes
	for(node u : steinerTree.nodes) {
		if (!isTerminal[steinerTree.original(u)]
		 && u->degree() <= 1) {
			return false;
		}
	}

	return true;
}

template<typename T>
bool MinSteinerTreeModule<T>::isQuasiBipartite(const EdgeWeightedGraph<T> &G, const NodeArray<bool> &isTerminal)
{
	for (node v = G.firstNode(); v; v = v->succ()) {
		if (!isTerminal[v]) {
			for (adjEntry adj = v->firstAdj(); adj; adj = adj->succ()) {
				if (!isTerminal[adj->twinNode()]) {
					return false;
				}
			}
		}
	}
	return true;
}

template<typename T>
T MinSteinerTreeModule<T>::pruneDanglingSteinerPathFrom(EdgeWeightedGraphCopy<T> &steinerTree, const NodeArray<bool> &isTerminal, const node start)
{
	OGDF_ASSERT(isConnected(steinerTree));
	T delWeights(0);
	node u = start;
	while (u->degree() == 1
	    && !isTerminal[steinerTree.original(u)]) {
		const adjEntry adj = u->firstAdj();
		const node v = adj->twinNode();
		delWeights += steinerTree.weight(adj->theEdge());
		steinerTree.delNode(u);
		u = v;
	}
	return delWeights;
}

template<typename T>
T MinSteinerTreeModule<T>::pruneDanglingSteinerPathsFrom(EdgeWeightedGraphCopy<T> &steinerTree, const NodeArray<bool> &isTerminal, const List<node> &start)
{
	T delWeights(0);
	for (node v : start) {
		delWeights += pruneDanglingSteinerPathFrom(steinerTree, isTerminal, v);
	}
	return delWeights;
}

template<typename T>
T MinSteinerTreeModule<T>::pruneAllDanglingSteinerPaths(EdgeWeightedGraphCopy<T> &steinerTree, const NodeArray<bool> &isTerminal)
{
	List<node> start;
	for (node u : steinerTree.nodes) {
		if (u->degree() == 1
		 && !isTerminal[steinerTree.original(u)]) {
			start.pushBack(u);
		}
	}

	return pruneDanglingSteinerPathsFrom(steinerTree, isTerminal, start);
}

template<typename T>
T MinSteinerTreeModule<T>::removeCyclesFrom(EdgeWeightedGraphCopy<T> &steinerTree, const NodeArray<bool> &isTerminal)
{
	if (steinerTree.numberOfEdges() > steinerTree.numberOfNodes() - 1) {
		EdgeArray<bool> isInTree(steinerTree);
		T oldCost(0);
		T newCost(computeMinST(steinerTree, steinerTree.edgeWeights(), isInTree));

		List<node> pendant; // collect resulting pendant edges
		for (edge nextEdge, e = steinerTree.firstEdge(); e; e = nextEdge) {
			oldCost += steinerTree.weight(e);
			nextEdge = e->succ();
			if (!isInTree[e]) {
				if (e->source()->degree() == 2) {
					pendant.pushBack(e->source());
				}
				if (e->target()->degree() == 2) {
					pendant.pushBack(e->target());
				}
				steinerTree.delEdge(e);
			}
		}
		newCost -= MinSteinerTreeModule<T>::pruneDanglingSteinerPathsFrom(steinerTree, isTerminal, pendant);
		return oldCost - newCost;
	}
	return 0;
}

template<typename T>
void MinSteinerTreeModule<T>::ssspInit(const EdgeWeightedGraph<T> &G, node source, PrioritizedMapQueue<node, T> &queue, NodeArray<T> &distance, NodeArray<edge> &pred)
{
	distance.init(G, std::numeric_limits<T>::max());
	distance[source] = 0;

	for (node v : G.nodes) {
		queue.push(v, distance[v]);
	}

	pred.init(G, nullptr);
}

template<typename T>
void MinSteinerTreeModule<T>::singleSourceShortestPathsPreferringTerminals(const EdgeWeightedGraph<T> &G, node source,
		const NodeArray<bool> &isTerminal, NodeArray<T> &distance, NodeArray<edge> &pred)
{
	PrioritizedMapQueue<node, T> queue(G);
	ssspInit(G, source, queue, distance, pred);

	// we must handle the source explicitly because it is a terminal
	node v = queue.topElement();
	queue.pop();
	OGDF_ASSERT(v == source);
	for (adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		node w = adj->twinNode();
		if (distance[w] > G.weight(e)) { // this check is only necessary for multigraphs, otherwise this is always true
			queue.decrease(w, (distance[w] = G.weight(e)));
			pred[w] = e;
		}
	}

	auto setPredecessor = [&](node w, edge e) {
		bool wOnPathWithTerminal = isTerminal[v] || pred[v] == nullptr;
		pred[w] = wOnPathWithTerminal ? nullptr : e;
	};
	while (!queue.empty()) {
		v = queue.topElement();
		queue.pop();

		if (distance[v] == std::numeric_limits<T>::max()) { // min is unreachable, finished
			break;
		}
		for (adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node w = adj->twinNode();
			T dist = distance[v] + G.weight(e);
			if (distance[w] > dist) {
				queue.decrease(w, (distance[w] = dist));
				setPredecessor(w, e);
			} else
			if (distance[w] == dist
			 && pred[w] != nullptr) { // tie
				setPredecessor(w, e);
			}
		}
	}
}

template<typename T>
void MinSteinerTreeModule<T>::allPairShortestPathsPreferringTerminals(const EdgeWeightedGraph<T>& G,
		const NodeArray<bool>& isTerminal, NodeArray<NodeArray<T>>& distance, NodeArray<NodeArray<edge>>& pred) {
	apspInit(G, distance, pred);

	for (node v : G.nodes) {
		if (isTerminal[v]) { // v is a terminal
			apspInnerLoop(v, G, distance, [&distance, &pred](node u, node w, T duvw) {
				if (duvw <= distance[u][w]) { // prefer terminals
					distance[w][u] = distance[u][w] = duvw;
					pred[w][u] = pred[u][w] = nullptr;
				}
			});
		} else { // v is not a terminal
			apspInnerLoop(v, G, distance, [&v, &distance, &pred](node u, node w, T duvw) {
				if (duvw < distance[u][w]) { // do not prefer nonterminals
					distance[w][u] = distance[u][w] = duvw;
					pred[u][w] = (pred[u][v] ? pred[v][w] : nullptr);
					pred[w][u] = (pred[w][v] ? pred[v][u] : nullptr);
				}
			});
		}
	}
	for (node u : G.nodes) {
		distance[u][u] = 0;
	}
}

template<typename T>
void MinSteinerTreeModule<T>::apspInit(const EdgeWeightedGraph<T> &G, NodeArray<NodeArray<T>> &distance, NodeArray<NodeArray<edge>> &pred)
{
	distance.init(G);
	pred.init(G);
	for (node u : G.nodes) {
		distance[u].init(G, std::numeric_limits<T>::max());
		pred[u].init(G, nullptr);
	}
	for (edge e : G.edges) {
		const node u = e->source(), v = e->target();
		distance[u][v] = distance[v][u] = G.weight(e);
		pred[u][v] = pred[v][u] = e;
	}
}

template<typename T>
void MinSteinerTreeModule<T>::allPairShortestPathsStandard(const EdgeWeightedGraph<T>& G, const NodeArray<bool>&, NodeArray<NodeArray<T>>& distance, NodeArray<NodeArray<edge>>& pred)
{
	apspInit(G, distance, pred);

	for (node v : G.nodes) {
		apspInnerLoop(v, G, distance, [&v, &distance, &pred](node u, node w, T duvw) {
			if (duvw < distance[u][w]) {
				distance[w][u] = distance[u][w] = duvw;
				pred[u][w] = pred[v][w];
				pred[w][u] = pred[v][u];
			}
		});
	}
	for (node u : G.nodes) {
		distance[u][u] = 0;
	}
}

template<typename T>
void MinSteinerTreeModule<T>::drawSteinerTreeSVG(const EdgeWeightedGraphCopy<T> &steinerTree, const NodeArray<bool> &isTerminal, const char *filename)
{
	GraphAttributes GA(steinerTree,
	  GraphAttributes::nodeGraphics |
	  GraphAttributes::nodeStyle |
	  GraphAttributes::nodeLabel |
	  GraphAttributes::edgeGraphics |
	  GraphAttributes::edgeStyle |
	  GraphAttributes::edgeLabel);

	GA.directed() = false;

	string s;

	for (node v : steinerTree.nodes) {
		std::stringstream out;
		GA.width(v) = GA.height(v) = 25.0;
		if (isTerminal[steinerTree.original(v)]) {
			out << "T";
			GA.shape(v) = Shape::Rect;
			GA.fillColor(v) = Color::Name::Red;
		} else {
			out << "S";
			GA.shape(v) = Shape::Ellipse;
			GA.fillColor(v) = Color::Name::Gray;
		}
		out << steinerTree.original(v);
		GA.label(v) = out.str();
	}

	FMMMLayout fmmm;

	fmmm.useHighLevelOptions(true);
	fmmm.unitEdgeLength(44.0);
	fmmm.newInitialPlacement(true);
	fmmm.qualityVersusSpeed(FMMMOptions::QualityVsSpeed::GorgeousAndEfficient);

	fmmm.call(GA);
	std::ofstream writeStream(filename, std::ofstream::out);
	GraphIO::drawSVG(GA, writeStream);
}

template<typename T>
void MinSteinerTreeModule<T>::drawSVG(const EdgeWeightedGraph<T> &G, const NodeArray<bool> &isTerminal, const EdgeWeightedGraphCopy<T> &steinerTree, const char *filename)
{
	GraphAttributes GA(G,
	  GraphAttributes::nodeGraphics |
	  GraphAttributes::nodeStyle |
	  GraphAttributes::nodeLabel |
	  GraphAttributes::edgeGraphics |
	  GraphAttributes::edgeStyle |
	  GraphAttributes::edgeLabel);

	GA.directed() = false;

	for (edge e : G.edges) {
		GA.strokeColor(e) = Color::Name::Black;
		GA.label(e) = to_string(G.weight(e));
		GA.strokeWidth(e) = 1;
	}
	for (edge e : steinerTree.edges) {
		GA.strokeColor(steinerTree.original(e)) = Color::Name::Red;
		GA.strokeWidth(steinerTree.original(e)) = 2;
	}

	for (node v : G.nodes) {
		std::stringstream out;
		GA.width(v) = GA.height(v) = 25.0;
		GA.strokeColor(v) = Color::Name::Black;
		if (isTerminal[v]) {
			out << "T" << v;
			GA.shape(v) = Shape::Rect;
			GA.fillColor(v) = Color::Name::Red;
			GA.strokeWidth(v) = 2;
		} else {
			out << "S" << v;
			GA.shape(v) = Shape::Ellipse;
			if (steinerTree.copy(v)) {
				GA.fillColor(v) = Color::Name::Gray;
				GA.strokeWidth(v) = 2;
			} else {
				GA.fillColor(v) = Color::Name::White;
				GA.strokeWidth(v) = 1;
			}
		}
		GA.label(v) = out.str();
	}

	FMMMLayout fmmm;

	fmmm.useHighLevelOptions(true);
	fmmm.unitEdgeLength(44.0);
	fmmm.newInitialPlacement(true);
	fmmm.qualityVersusSpeed(FMMMOptions::QualityVsSpeed::GorgeousAndEfficient);

	fmmm.call(GA);

	std::ofstream writeStream(filename, std::ofstream::out);
	GraphIO::drawSVG(GA, writeStream);
}

}
