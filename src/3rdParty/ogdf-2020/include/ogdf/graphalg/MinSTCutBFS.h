/** \file
 * \brief Declaration of min-st-cut algorithm which calculates the min-st-cut of an st-planar graph
 *        by doing a BFS on the dual graph (class MinSTCutDFS)
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
#include <ogdf/basic/Queue.h>

namespace ogdf {

/**
 * Min-st-cut algorithm, that calculates the cut by doing a depth first search over the dual graph of of an st-planar
 * input graph.
 *
 * @pre The input graph is st-planar.
 * @tparam TCost The type in which the weight of the edges is given.
 */
template<typename TCost>
class MinSTCutBFS : public MinSTCutModule<TCost> {
public:
	MinSTCutBFS() {	}

	/**
	 * @copydoc ogdf::MinSTCutModule<TCost>::call(const Graph&,node,node,List<edge>&,edge)
	 */
	virtual bool
	call(const Graph &graph, node s, node t, List <edge> &edgeList, edge e_st = nullptr) override {
		return call(graph, nullptr, s, t, edgeList, e_st);
	}

	/**
	 * @copydoc ogdf::MinSTCutModule<TCost>::call(const Graph&,const EdgeArray<TCost>&,node,node,List<edge>&,edge)
	 */
	virtual bool call(const Graph &graph, const EdgeArray <TCost> &weight, node s, node t,
	                  List <edge> &edgeList, edge e_st = nullptr) override {
		return call(graph, &weight, s, t, edgeList, e_st);
	}

	using MinSTCutModule<TCost>::m_gc;

private:
	/**
	 * This internal call uses a pointer to the \p weight instead of a reference.
	 *
	 * @copydoc ogdf::MinSTCutModule<TCost>::call(const Graph&,const EdgeArray<TCost>&,node,node,List<edge>&,edge)
	 */
	bool call(const Graph &graph, const EdgeArray <TCost> *weight, node s, node t, List <edge> &edgeList, edge e_st);

};

template<typename TCost>
bool MinSTCutBFS<TCost>::call(const Graph &graph, const EdgeArray <TCost> *weight, node s, node t,
                              List <edge> &edgeList, edge e_st) {
	bool weighted = (weight != nullptr);
	delete m_gc;
	m_gc = new GraphCopy(graph);
	CombinatorialEmbedding CE;
	GraphCopy m_weightedGc;
	EdgeArray<edge> mapE(m_weightedGc, nullptr);

	std::function<edge(edge)> orig = [&](edge e) -> edge { return m_gc->original(e);};

	if(weighted) {
		m_weightedGc.init(graph);
		if(e_st != nullptr) {
			e_st = m_weightedGc.copy(e_st);
		}
		s = m_weightedGc.copy(s);
		t = m_weightedGc.copy(t);
		List<edge> edges;
		graph.allEdges(edges);
		for(edge e : edges) {
			mapE[m_weightedGc.copy(e)] = e;
			if(m_weightedGc.copy(e) == e_st) {
				continue;
			}
			OGDF_ASSERT((*weight)[e] >= 1);
			TCost i = 1;
			for(; i < (*weight)[e]; i++) {
				edge copyEdge = m_weightedGc.copy(e);
				edge newEdge = m_weightedGc.newEdge(copyEdge->source(), copyEdge->target());
				m_weightedGc.move(newEdge, copyEdge->adjSource(), ogdf::Direction::before, copyEdge->adjTarget(), ogdf::Direction::after);
				mapE[newEdge] = e;
			}
			OGDF_ASSERT((*weight)[e] == i);
		}
		// we can't reinit m_gc, because it's possible that the original graph of m_gc
		// doesn't exist anymore.
		delete m_gc;
		m_gc = new GraphCopy();
		m_gc->init(m_weightedGc);
		orig = [&](edge e) -> edge { return mapE[m_gc->original(e)];};
		MinSTCutModule<TCost>::preprocessingDual(m_weightedGc, *m_gc, CE, s, t, e_st);
	} else {
		MinSTCutModule<TCost>::preprocessingDual(graph, *m_gc, CE, s, t, e_st);
	}

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

	NodeArray<edge> spPred(dual, nullptr);
	EdgeArray<node> prev(dual, nullptr);
	EdgeArray<bool> direction(dual, true);
	QueuePure<edge> queue;
	for (adjEntry adj : source->adjEntries) {
		if (dual.primalEdge(adj->theEdge()) != e_st) {
			queue.append(adj->theEdge());
			prev[adj->theEdge()] = source;
		}
	}
	// actual search (using bfs on directed dual)
	for (;;) {
		// next candidate edge
		edge eCand = queue.pop();
		bool dir = (eCand->source() == prev[eCand]);
		node v = (dir ? eCand->target() : eCand->source());

		// leads to an unvisited node?
		if (spPred[v] == nullptr) {
			// yes, then we set v's predecessor in search tree
			spPred[v] = eCand;
			direction[eCand] = dir;

			// have we reached t ...
			if (v == target) {
				// ... then search is done.
				// constructed list of used edges (translated to crossed
				// edges entries in G) from t back to s (including first
				// and last!)

				do {
					edge eDual = spPred[v];
					OGDF_ASSERT(eDual != nullptr);
					// this should be the right way round
					edge eG = dual.primalEdge(eDual);
					OGDF_ASSERT(eG != nullptr);
					edgeList.pushBack(orig(eG));
					MinSTCutModule<TCost>::m_direction[orig(eG)] = !direction[eDual];
					v = prev[eDual];
				} while (v != source);

				break;
			}

			// append next candidate edges to queue
			// (all edges leaving v)
			for (adjEntry adj : v->adjEntries) {
				if (prev[adj->theEdge()] == nullptr) {
					queue.append(adj->theEdge());
					prev[adj->theEdge()] = v;
				}
			}
		}
	}
	if(weighted) {
		auto prevIt = edgeList.begin();
		for(auto it = prevIt.succ(); it != edgeList.end(); prevIt = it++) {
			if(*prevIt == *it) {
				edgeList.del(prevIt);
			}
		}
	}
	return true;
}
}
