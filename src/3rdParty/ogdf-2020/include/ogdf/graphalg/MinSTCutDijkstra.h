/** \file
 * \brief MinSTCutDijkstra class template
 *
 * \author Mirko Wagner
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

#include <ogdf/basic/GraphCopy.h>
#include <ogdf/graphalg/Dijkstra.h>
#include <ogdf/basic/DualGraph.h>
#include <ogdf/graphalg/MinSTCutModule.h>
#include <ogdf/basic/extended_graph_alg.h>

namespace ogdf {

/**
 * Min-st-cut algorithm, that calculates the cut by calculating the shortest path between the faces adjacent
 * to an edge between s and t, via the algorithm by %Dijkstra on the dual graph.
 *
 * @pre The input graph is st-planar.
 * @tparam TCost The type in which the weight of the edges is given.
 */
template<typename TCost>
class MinSTCutDijkstra : public MinSTCutModule<TCost> {
public:
	MinSTCutDijkstra() { }

	/**
	 * @copydoc ogdf::MinSTCutModule<TCost>::call(const Graph&,const EdgeArray<TCost>&,node,node,List<edge>&,edge)
	 */
	virtual bool
	call(const Graph &graph, const EdgeArray<TCost> &weight, node s, node t, List<edge> &edgeList, edge e_st = nullptr) override;

	/**
	 * @copydoc ogdf::MinSTCutModule<TCost>::call(const Graph&,node,node,List<edge>&,edge)
	 */
	virtual bool call(const Graph &graph, node s, node t, List<edge> &edgeList, edge e_st = nullptr) override {
		EdgeArray<TCost> weight(graph, 1);
		return call(graph, weight, s, t, edgeList, e_st);
	}

	using MinSTCutModule<TCost>::m_gc;
};

template<typename TCost>
bool MinSTCutDijkstra<TCost>::call(const Graph &graph, const EdgeArray<TCost> &weight,
                                   node s, node t, List<edge> &edgeList, edge e_st) {
	delete m_gc;
	m_gc = new GraphCopy(graph);
	CombinatorialEmbedding CE;

	MinSTCutModule<TCost>::preprocessingDual(graph, *m_gc, CE, s, t, e_st);

	MinSTCutModule<TCost>::m_direction.init(graph);

	DualGraph dual(CE);
	if (e_st != nullptr) {
		e_st = m_gc->copy(e_st);
	} else {
		e_st = m_gc->searchEdge(m_gc->copy(s), m_gc->copy(t));
	}
	edgeList.clear();
	node source = dual.dualNode(CE.rightFace(e_st->adjSource()));
	node target = dual.dualNode(CE.leftFace(e_st->adjSource()));


	EdgeArray<TCost> weightDual(dual, 0);
	TCost sumOfWeights(0);
	for (edge e : m_gc->edges) {
		if (e != e_st) {
			weightDual[dual.dualEdge(e)] = weight[m_gc->original(e)];
			sumOfWeights += weightDual[dual.dualEdge(e)];
			OGDF_ASSERT(sumOfWeights > sumOfWeights - weightDual[dual.dualEdge(e)]);
		}
	}
	weightDual[dual.dualEdge(e_st)] = sumOfWeights + 1;
	List<node> sourceNodeList;
	sourceNodeList.pushFront(source);
	NodeArray<edge> prevEdge(dual);
	NodeArray<TCost> distance(dual);
	Dijkstra<TCost> D;
	D.call(dual, weightDual, sourceNodeList, prevEdge, distance);

	node v = target;
	do {
		edge eDual = prevEdge[v];
		OGDF_ASSERT(eDual != nullptr);
		edge eG = dual.primalEdge(eDual);
		OGDF_ASSERT(eG != nullptr);
		edgeList.pushBack(m_gc->original(eG));
		MinSTCutModule<TCost>::m_direction[m_gc->original(eG)] = eDual->target() != v;
		v = (v == eDual->target() ? eDual->source() : eDual->target());
	} while (v != source);
	return true;
}
}
