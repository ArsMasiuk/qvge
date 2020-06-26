/** \file
 * \brief Max-Flow on s-t-planar graphs (s and t lie on the boundary of the
 * outer face) via shortest paths in the dual. See [Ahuja, Magnanti, Orlin:
 * Network Flows. Section 8.4]. Runtime: O(V log V).
 *
 * \author Ivo Hedtke
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

#include <ogdf/graphalg/MaxFlowModule.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/DualGraph.h>
#include <ogdf/graphalg/Dijkstra.h>

namespace ogdf {

//! Computes a max flow in s-t-planar network via dual shortest paths.
/**
 * @ingroup ga-flow
 */
template <typename TCap>
class MaxFlowSTPlanarDigraph : public MaxFlowModule<TCap> {
private:

	//! Create back arcs for all edges and set the backedges' \p new_costs to zero.
	void createBackArcs(Graph &gr, EdgeArray<TCap> &new_costs) {
		// gr = dg.
		for (edge e = gr.lastEdge(); e != nullptr; e = e->pred()) {
			edge e_new_dg = gr.newEdge(e->target(), e->source());
			new_costs[e_new_dg] = 0;
		}
	}

public:
	/**
	 * @copybrief ogdf::MaxFlowModule<TCap>::computeValue
	 *
	 * After this first phase, the flow itself is already computed!
	 *
	 * @copydetails ogdf::MaxFlowModule<TCap>::computeValue
	 */
	TCap computeValue(const EdgeArray<TCap> &cap, const node &s, const node &t) override {
		// clear old flow
		(*this->m_flow).fill(0);
		// store capacity, source and sink
		this->m_cap = &cap;
		this->m_s = &s;
		this->m_t = &t;
		OGDF_ASSERT(this->isFeasibleInstance());

		OGDF_ASSERT(isSTPlanar(*this->m_G, s, t));
		GraphCopy copyG(*this->m_G);
		node copyS = copyG.copy(s);
		node copyT = copyG.copy(t);
		CombinatorialEmbedding ce(copyG);
		adjEntry adjAtTarget;
		adjEntry adjAtSource = ce.findCommonFace(copyS, copyT, adjAtTarget, false);
		face f_infty = ce.rightFace(adjAtSource);
		ce.setExternalFace(f_infty);

		// split the external face
		edge ts_edge = ce.splitFace(adjAtTarget, adjAtSource);
		DualGraph dg(ce);

		EdgeArray<TCap> costs(dg, 0);
		for (edge e : (*this->m_G).edges) {
			costs[dg.dualEdge(copyG.copy(e))] = cap[e];
		}
		createBackArcs(dg, costs);
		costs[dg.dualEdge(ts_edge)] = std::numeric_limits<TCap>::max();

		Dijkstra<TCap> dij;
		NodeArray<edge> preds(dg, nullptr);
		NodeArray<TCap> dists(dg, 0);
		dij.call(dg, costs, dg.dualNode(f_infty), preds, dists, true);  // directed

		for (edge e : (*this->m_G).edges) {
			(*this->m_flow)[e] =
				dists[dg.dualNode(ce.leftFace(copyG.copy(e)->adjSource()))] -
				dists[dg.dualNode(ce.rightFace(copyG.copy(e)->adjSource()))];
		}

		// compute flow value
		TCap flowValue = 0;
		for (adjEntry adj : s->adjEntries) {
			edge e = adj->theEdge();
			if (e->source() == s) {
				flowValue += (*this->m_flow)[e];
			} else {
				flowValue -= (*this->m_flow)[e];
			}
		}
		return flowValue;
	}

	//! Implementation of computeFlowAfterValue from the super class. This does
	//! nothing, because the algorithm is finished after computeValue.
	void computeFlowAfterValue() override { /* nothing to do here */
	}

	void init(const Graph &graph, EdgeArray<TCap> *flow = nullptr) override {
		OGDF_ASSERT(isConnected(graph));
		OGDF_ASSERT(isPlanar(graph));
		MaxFlowModule<TCap>::init(graph, flow);
	}

	// use methods from super class
	using MaxFlowModule<TCap>::useEpsilonTest;
	using MaxFlowModule<TCap>::computeFlow;
	using MaxFlowModule<TCap>::computeFlowAfterValue;
	using MaxFlowModule<TCap>::MaxFlowModule;
};

}
