/** \file
 * \brief Declaration of several shortest path algorithms.
 *
 * \author Mark Ortmann, University of Konstanz
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

#include <ogdf/basic/SList.h>
#include <ogdf/basic/Graph_d.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/Array.h>
#include <ogdf/graphalg/Dijkstra.h>


namespace ogdf {

//! Computes all-pairs shortest paths in \p G using breadth-first serach (BFS).
/**
 * @ingroup ga-sp
 *
 * The cost of each edge are \p edgeCost and the result is stored in \p distance.
 */
template<typename TCost>
void bfs_SPAP(const Graph& G, NodeArray<NodeArray<TCost>>& distance, TCost edgeCosts) {
	for (node v : G.nodes) {
		bfs_SPSS(v, G, distance[v], edgeCosts);
	}
}


//! Computes single-source shortest paths from \p s in \p G using breadth-first search (BFS).
/**
 * @ingroup ga-sp
 *
 * The cost of each edge are \p edgeCost and the result is stored in \p distanceArray.
 */
template<typename TCost>
void bfs_SPSS(node s, const Graph& G, NodeArray<TCost> & distanceArray, TCost edgeCosts) {
	NodeArray<bool> mark(G, false);
	SListPure<node> bfs;
	bfs.pushBack(s);
	// mark s and set distance to itself 0
	mark[s] = true;
	distanceArray[s] = TCost(0);
	while (!bfs.empty()) {
		node w = bfs.popFrontRet();
		TCost d = distanceArray[w] + edgeCosts;
		for(adjEntry adj : w->adjEntries) {
			node v = adj->twinNode();
			if (!mark[v]) {
				mark[v] = true;
				bfs.pushBack(v);
				distanceArray[v] = d;
			}
		}
	}
}


//! Computes all-pairs shortest paths in \p GA using %Dijkstra's algorithm.
/**
 * @ingroup ga-sp
 *
 * The cost of an edge \a e are given by GA.doubleWeight(\a e) and the result is stored in \p shortestPathMatrix.
 *
 * @return returns the average edge cost
 */
template<typename TCost>
double dijkstra_SPAP(const GraphAttributes& GA, NodeArray<NodeArray<TCost>>& shortestPathMatrix) {
	const Graph& G = GA.constGraph();
	EdgeArray<TCost> edgeCosts(G);
	double avgCosts = 0;
	for (edge e : G.edges) {
		edgeCosts[e] = GA.doubleWeight(e);
		avgCosts += edgeCosts[e];
	}
	dijkstra_SPAP(G, shortestPathMatrix, edgeCosts);
	return avgCosts / G.numberOfEdges();
}


//! Computes all-pairs shortest paths in graph \p G using %Dijkstra's algorithm.
/**
 * @ingroup ga-sp
 *
 * The cost of an edge are given by \p edgeCosts and the result is stored in \p shortestPathMatrix.
 */
template<typename TCost>
void dijkstra_SPAP(
	const Graph& G,
	NodeArray<NodeArray<TCost>>& shortestPathMatrix,
	const EdgeArray<TCost>& edgeCosts)
{
	for (node v : G.nodes) {
		dijkstra_SPSS(v, G, shortestPathMatrix[v], edgeCosts);
	}
}


//! Computes single-source shortest paths from node \p s in \p G using Disjkstra's algorithm.
/**
 * @ingroup ga-sp
 *
 * The cost of an edge are given by \p edgeCosts and the result is stored in \p shortestPathMatrix.
 * Note this algorithm equals Dijkstra<T>::call, though it does not
 * compute the predecessors on the path and is not inlined.
 */
template<typename TCost>
void dijkstra_SPSS(
	node s,
	const Graph& G,
	NodeArray<TCost>& shortestPathMatrix,
	const EdgeArray<TCost>& edgeCosts)
{
	NodeArray<edge> predecessor;
	Dijkstra<TCost> sssp;
	sssp.call(G, edgeCosts, s, predecessor, shortestPathMatrix);
}


//! Computes all-pairs shortest paths in graph \p G using Floyd-Warshall's algorithm.
/**
 * @ingroup ga-sp
 *
 * Note that the \p shortestPathMatrix has to be initialized and all entries must be positive.
 * The costs of non-adjacent nodes should be set to std::numeric_limits<TCost>::infinity().
 */
template<typename TCost>
void floydWarshall_SPAP(NodeArray<NodeArray<TCost>>& shortestPathMatrix, const Graph& G) {
	for (node u : G.nodes) {
		for (node v : G.nodes) {
			for (node w : G.nodes) {
				Math::updateMin(shortestPathMatrix[v][w], shortestPathMatrix[u][v] + shortestPathMatrix[u][w]);
			}
		}
	}
}

}
