/** \file
 * \brief Implementation of the A* informed search algorithm.
 *
 * \author Tilo Wiedera
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

//! A-Star informed search algorithm.
/**
 * The algorithm is a generalization the the shortest path algorithm by %Dijkstra.
 * It was first described in "A Formal Basis for the Heuristic Determination of Minimum Cost Paths" by Hart, Nilsson and Raphael in 1968.
 *
 * The algorithm yields an optimal solution to the single pair shortest pair problem.
 * A heuristic for calculating a lower bound on the distance from any node to the target is optional.
 * The algorithm can also be used to compute approximate solutions at a faster pace.
 *
 * @tparam T The type of edge cost
 */
template<typename T>
class AStarSearch {
private:
	using NodeQueue = PrioritizedMapQueue<node, T>;

	bool m_directed;
	double m_maxGap;
	EpsilonTest m_et;

	NodeArray<bool> m_folded;
	const EdgeArray<T> *m_cost = nullptr;
	std::function<T(node)> m_heuristic;
	NodeArray<edge> *m_predecessor = nullptr;
	NodeArray<T> m_distance;
	NodeQueue *m_queue = nullptr;

public:

	/**
	 * Initializes a new A* search algorithm.
	 *
	 * @param directed Whether to traverse edges in both directions
	 * @param maxGap The maximal gap between the computed path costs and the optimal solution.
	 *               The default of 1 leads to an optimal solution.
	 * @param et The ::ogdf::EpsilonTest to be used for comparing edge costs
	 */
	explicit AStarSearch(const bool directed = false, const double maxGap = 1, const EpsilonTest &et = EpsilonTest())
	: m_directed(directed)
	, m_maxGap(maxGap)
	, m_et(et)
	{
		OGDF_ASSERT(m_et.geq(maxGap, 1.0));
	}

	/**
	 * Computes the shortests path between \c source and \c target.
	 *
	 * @param graph The graph to investigate
	 * @param cost The positive cost of each edge
	 * @param source The start of the path to compute
	 * @param target The end of the path to compute
	 * @param predecessor Will contain the preceding edge of each node in the path
	 *        \c predecessor[target] will be \c nullptr if no path could be found
	 * @param heuristic The heuristic to be used.
	 *                  Note that the type ::ogdf::NodeArray is implicitly applicable here.
	 *                  The default heuristic will always return the trivial lower bound of zero.
	 * @return The total length of the found path
	 */
	T call(const Graph &graph,
	       const EdgeArray<T> &cost,
	       const node source,
	       const node target,
	       NodeArray<edge> &predecessor,
	       std::function<T(node)> heuristic = [](node) {
#ifdef _MSC_VER
			return 0;
#else
			return T(0);
#endif
		})
	{
		// initialize auxiliary structures
		m_cost = &cost;
		m_distance.init(graph);
		m_predecessor = &predecessor;
		m_predecessor->init(graph);
		m_heuristic = heuristic;

		m_folded.init(graph, false);
		NodeQueue queue(graph);
		m_queue = &queue;

		m_distance[source] = 0;
		(*m_predecessor)[target] = nullptr;
		m_queue->push(source, 0);

		// investigate each node
		while(!m_queue->empty()) {
			node v = queue.topElement();
			queue.pop();
			m_folded[v] = true;

			if(v == target) {
				queue.clear();
			} else {
				investigateNode(v);
			}
		}

		OGDF_ASSERT((*m_predecessor)[target] == nullptr || validatePath(source, target));

		return m_distance[target];
	}

private:

#ifdef OGDF_DEBUG
	bool validatePath(const node source, const node target) const {
		NodeArray<bool> visited(*m_predecessor->graphOf(), false);

		OGDF_ASSERT(m_et.equal(m_distance[source], T(0)));

		for(node v = target; v != source;) {
			OGDF_ASSERT(!visited[v]);

			visited[v] = true;
			edge e = (*m_predecessor)[v];

			OGDF_ASSERT(e != nullptr);

			node w = e->opposite(v);

			OGDF_ASSERT(m_et.equal(m_distance[w] + (*m_cost)[e], m_distance[v]));

			v = w;
		}

		return true;
	}
#endif

	void investigateNode(const node v) {
		for(adjEntry adj = v->firstAdj(); adj != nullptr; adj = adj->succ()) {
			edge e = adj->theEdge();
			if(!m_directed || e->target() != v) {
				node w = e->opposite(v);
				T distanceW = m_distance[v] + (*m_cost)[e];
				if(!m_folded(w) && (!m_queue->contains(w) || m_et.less(distanceW, m_distance[w]))) {
					m_distance[w] = distanceW;
					(*m_predecessor)[w] = e;
					T priority = (T)(m_distance[w] + m_maxGap * m_heuristic(w));

					if(!m_queue->contains(w)) {
						m_queue->push(w, priority);
					} else {
						m_queue->decrease(w, priority);
					}
				}
			}
		}
	}
};

}
