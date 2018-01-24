/** \file
 * \brief Template of base class of min-st-cut algorithms.
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

#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/CombinatorialEmbedding.h>

namespace ogdf {

template<typename TCost>
class MinSTCutModule {
public:
	/**
	 * default constructor (empty)
	 */
	MinSTCutModule() { }

	/**
	 * The actual algorithm call.
	 *
	 * @param graph The graph on which the min-st-cut is to be calculated.
	 * @param weight Provides a weight for every edge.
	 * @param s The source node.
	 * @param t The target node.
	 * @param edgeList This list is filled with the edges which are part of the mincut. If the graph is st-planarly
	 * embedded, this list is correctly ordered along the cut.
	 * @param e_st An edge between \p s and \p t which is used to determine where the cut should start,
	 * nullptr elsewise.
	 * @return Indicates success.
	 */
	virtual bool call(const Graph &graph, const EdgeArray<TCost> &weight, node s, node t, List<edge> &edgeList,
	                  edge e_st = nullptr) = 0;

	/**
	 * The actual algorithm call.
	 *
	 * @param graph The graph on which the min-st-cut is to be calculated.
	 * @param s The source node.
	 * @param t The target node.
	 * @param edgeList This list is filled with the edges which are part of the mincut. If the graph is st-planarly
	 * embedded, this list is correctly ordered along the cut.
	 * @param e_st An edge between \p s and \p t which is used to determine where the cut should start,
	 * nullptr elsewise.
	 * @return Indicates success.
	 */
	virtual bool call(const Graph &graph, node s, node t, List<edge> &edgeList, edge e_st = nullptr) = 0;

	virtual ~MinSTCutModule() {
		delete m_gc;
	}

	/**
	 * Returns the direction of \p e in the cut.
	 *
	 * @pre \p e is part of the cut calculated last.
	 * @param e An edge in the graph for which the min-st-cut was calculated last.
	 * @return true, iff the source of \p e is in one component with \a s, if all edges of the cut are deleted.
	 */
	virtual bool direction(edge e) {
		OGDF_ASSERT(m_gc->numberOfEdges() != 0);
		OGDF_ASSERT(m_direction[e] != -1);
		return m_direction[e];
	}

protected:
	EdgeArray<int> m_direction;
	GraphCopy *m_gc = nullptr;

	/**
	 * This method preprocesses \p gc for minstcut calculations, by adding an st-edge if needed and embedding \p gc.
	 *
	 * @param graph The original graph of \p gc.
	 * @param gc The input graph.
	 * @param CE Holds the embedding of \p gc.
	 * @param source \a s
	 * @param target \a t
	 * @param e_st If not nullptr, this edge is meant to split the external face of the embedded \p gc.
	 * @return
	 */
	bool preprocessingDual(const Graph &graph,
	                       GraphCopy &gc,
	                       CombinatorialEmbedding &CE,
	                       node source,
	                       node target,
	                       edge e_st) {
		node gcS(gc.copy(source));
		node gcT(gc.copy(target));
		adjEntry adjT, adjS;
		bool isSTplanarEmbeded(false);
		if (gc.representsCombEmbedding()) {
			CE.init(gc);
			adjS = CE.findCommonFace(gcS, gcT, adjT, false);
			isSTplanarEmbeded = (adjS != nullptr);
		}
		if(isSTplanarEmbeded) {
			if (e_st == nullptr) {
				CE.splitFace(adjS, adjT);
			}
		} else {
			if (e_st == nullptr) {
				gc.newEdge(gcS, gcT);
			}
			if(!planarSTEmbed(gc, gcS, gcT)) {
				return false;
			}
			CE.init(gc);
		}
		return true;
	}
};
}
