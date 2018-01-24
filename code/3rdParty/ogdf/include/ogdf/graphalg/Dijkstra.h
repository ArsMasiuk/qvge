/** \file
 * \brief Implementation of Dijkstra's single source shortest path algorithm
 *
 * \author Matthias Woste
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

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/EpsilonTest.h>
#include <ogdf/basic/PriorityQueue.h>


namespace ogdf {

/*!
 * \brief %Dijkstra's single source shortest path algorithm.
 *
 * @ingroup ga-sp
 *
 * This class implements Dijkstra's algorithm for computing single source shortest path
 * in (undirected or directed) graphs with proper, positive edge weights.
 * It returns a predecessor array as well as the shortest distances from the source node
 * to all others.
 */
template<typename T, template<typename P, class C> class H = PairingHeap>
class Dijkstra {
protected:
	EpsilonTest m_eps; //!< For floating point comparisons (if floating point is used)

public:
	/*!
	 * \brief Calculates, based on the graph G with corresponding edge costs and source nodes,
	 * the shortest paths and distances to all other nodes by Dijkstra's algorithm.
	 */
	void call(const Graph &G, //!< The original input graph
		  const EdgeArray<T> &weight, //!< The edge weights
		  const List<node> &sources, //!< A list of source nodes
		  NodeArray<edge> &predecessor, //!< The resulting predecessor relation
		  NodeArray<T> &distance, //!< The resulting distances to all other nodes
		  bool directed = false) //!< True iff G should be interpreted as directed graph
	{
		PrioritizedMapQueue<node, T, std::less<T>, H> queue(G);
		distance.init(G, std::numeric_limits<T>::max());
		predecessor.init(G, nullptr);

		// initialization
		for (node v : G.nodes) {
			queue.push(v, distance[v]);
		}
		for (node s : sources) {
			queue.decrease(s, (distance[s] = 0));
		}

#ifdef OGDF_DEBUG
		for (edge de : G.edges) {
			OGDF_ASSERT(weight[de] >= 0);
		}
#endif

		while (!queue.empty()) {
			node v = queue.topElement();
			queue.pop();
			if (!predecessor[v]
			 && m_eps.greater(distance[v], static_cast<T>(0))) { // v is unreachable, ignore
				continue;
			}
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();
				node w = adj->twinNode();
				if (directed && e->target() == v) { // edge is in wrong direction
					continue;
				}
				if (m_eps.greater(distance[w], distance[v] + weight[e])) {
					OGDF_ASSERT(std::numeric_limits<T>::max() - weight[e] >= distance[v]);
					queue.decrease(w, (distance[w] = distance[v] + weight[e]));
					predecessor[w] = e;
				}
			}
		}
	}

	/*!
	 * \brief Calculates, based on the graph G with corresponding edge costs and a source node s,
	 * the shortest paths and distances to all other nodes by Dijkstra's algorithm.
	 */
	void call(const Graph &G, //!< The original input graph
		  const EdgeArray<T> &weight, //!< The edge weights
		  node s, //!< The source node
		  NodeArray<edge> &predecessor, //!< The resulting predecessor relation
		  NodeArray<T> &distance, //!< The resulting distances to all other nodes
		  bool directed = false) //!< True iff G should be interpreted as directed graph
	{
		List<node> sources;
		sources.pushBack(s);
		call(G, weight, sources, predecessor, distance, directed);
	}
};

}
