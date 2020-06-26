/** \file
 * \brief Implementation of the maximum flow algorithm for
 * s-t-planar graphs by Alon Itai and Yossi Shiloach
 * (See "Maximum Flow in Planar Networks", p.135,
 * 1979, Society for Industrial and Applied Mathematics).
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

#include <limits>
#include <ogdf/graphalg/MaxFlowModule.h>
#include <ogdf/basic/PriorityQueue.h>
#include <ogdf/basic/CombinatorialEmbedding.h>

namespace ogdf {

//! Computes a max flow in s-t-planar network via uppermost paths.
/**
 * @ingroup ga-flow
 */
template<typename TCap>
class MaxFlowSTPlanarItaiShiloach : public MaxFlowModule<TCap>
{
private:

	/**
	 * \brief To work with unsigned, we need a priority shifting of the elements in the priority queue.
	 * @param e edge in #m_prioritizedEdges
	 * @return unshifted priority
	 */
	TCap unshiftedPriority(edge e) {
		return m_prioritizedEdges->priority(e) - TCap(1);
	}

	/**
	 * see #unshiftedPriority
	 * @return unshifted priority of top element
	 */
	TCap unshiftedTopPriority() {
		return m_prioritizedEdges->topPriority() - TCap(1);
	}

	/**
	 * see #unshiftedPriority
	 * @param priority original priority
	 * @return shifted priority
	 */
	TCap shiftPriority(TCap priority) {
		OGDF_ASSERT(priority < std::numeric_limits<TCap>::max());
		return priority + TCap(1);
	}

	/**
	 * Each node has a certain type depending on its participation in any path.
	 */
	enum class NodeType {
		New,
		Path,
		Done
	};

	/**
	 * Each edge may be part of the source or target path.
	 * We do not store this information. It is only a temporary
	 * variable in two routines.
	 */
	enum class EdgePathType {
		NoPath,
		SourcePath,
		TargetPath,
		Unknown
	};

	adjEntry m_commonFaceAdj;

	/** Whether each edge has was visited */
	EdgeArray<bool> m_visited;

	/** The number of edges visited from each node */
	NodeArray<int> m_edgeCounter;

	/** The predecessor of each node in the currently uppermost path */
	NodeArray<edge> m_pred;

	/** The status of each node */
	NodeArray<NodeType> m_status;

	/** A priority queue for storing all edges currently in a path */
	PrioritizedMapQueue<edge, TCap> *m_prioritizedEdges = nullptr;

	/** The flow reached thus far (monotonically increasing). */
	TCap m_partialFlow;

public:
	/**
	 * Free allocated ressources.
	 */
	~MaxFlowSTPlanarItaiShiloach() {
		delete m_prioritizedEdges;
	}

	/**
	 * Computes the maximal flow from source to sink.
	 *
	 * Assumes the graph to be s-t-planary embedded.
	 *
	 * @param originalCapacities the positive capacity of each edge
	 * @param source the source node
	 * @param target the sink node
	 */
	TCap computeValue(const EdgeArray<TCap> &originalCapacities, const node &source, const node &target)
	{
		OGDF_ASSERT(source != target);
		OGDF_ASSERT(isSTPlanar(*this->m_G, source, target));

		// initialize auxiliary structures

		m_partialFlow = (TCap) 0;

		this->m_s = &source;
		this->m_t = &target;
		this->m_cap = &originalCapacities;
		this->m_flow->init(*this->m_G, (TCap) 0);
		OGDF_ASSERT(this->isFeasibleInstance());

		// establish s-t-planarity
		ConstCombinatorialEmbedding embedding(*this->m_G);
#ifdef OGDF_DEBUG
		embedding.consistencyCheck();
#endif
		adjEntry secondCommonAdj;
		m_commonFaceAdj = embedding.findCommonFace(*this->m_s, *this->m_t, secondCommonAdj);
		OGDF_ASSERT(m_commonFaceAdj != nullptr);

		m_pred.init(*this->m_G, nullptr);
		m_status.init(*this->m_G, NodeType::New);
		m_visited.init(*this->m_G, false);
		m_edgeCounter.init(*this->m_G, 0);
		m_status[*this->m_s] = NodeType::Path;
		m_status[*this->m_t] = NodeType::Path;

		delete m_prioritizedEdges;
		m_prioritizedEdges = new PrioritizedMapQueue<edge, TCap>(*this->m_G);

		// saturate all paths

		edge lastSaturated = nullptr;
		while(findUppermostPath(lastSaturated)) {

			lastSaturated = m_prioritizedEdges->topElement();
			m_partialFlow = unshiftedTopPriority();
			m_prioritizedEdges->pop();

			(*this->m_flow)[lastSaturated] = (*this->m_cap)[lastSaturated];

			m_pred[lastSaturated->target()] = nullptr;
			OGDF_ASSERT(m_status[lastSaturated->target()] == NodeType::Path);
			OGDF_ASSERT(m_status[lastSaturated->source()] == NodeType::Path);
		}

		return m_partialFlow;
	}

	/**
	 * Computes the actual flow on each edge.
	 *
	 * Most edges have already been assigned their respective flow values.
	 * However, there might be some edges remaining (as part of tangling paths
	 * from source and sink).
	 */
	void computeFlowAfterValue()
	{
		// the flow value is only computed if the edge is deleted
		while (!m_prioritizedEdges->empty()) {
			edge e = m_prioritizedEdges->topElement();
			dropEdge(e);
		}
	}

	// use methods from super class
	using MaxFlowModule<TCap>::useEpsilonTest;
	using MaxFlowModule<TCap>::computeFlow;
	using MaxFlowModule<TCap>::computeFlowAfterValue;
	using MaxFlowModule<TCap>::MaxFlowModule;
	using MaxFlowModule<TCap>::init;

private:

	/**
	 * Establishes the next uppermost path.
	 *
	 * @param saturatedEdge The edge saturated most recently
	 */
	bool findUppermostPath(const edge saturatedEdge)
	{
		node v;
		adjEntry initialAdj;

		if (saturatedEdge == nullptr) {
			v = *this->m_s;
			initialAdj = m_commonFaceAdj;
		} else {
			OGDF_ASSERT(!saturatedEdge->isSelfLoop());
			OGDF_ASSERT(saturatedEdge->target() != *this->m_s);
			v = saturatedEdge->source();
			initialAdj = saturatedEdge->adjSource()->cyclicSucc();
		}

		OGDF_ASSERT(v != *this->m_t);

		for(adjEntry adj = initialAdj;
			m_edgeCounter[v] < v->degree();
			adj = adj->cyclicSucc())
		{
			m_edgeCounter[v]++;
			edge e = adj->theEdge();

			bool visited = m_visited[e];
			m_visited[e] = true;

			if(!visited
				&& e->target() != v
				&& m_status[e->target()] != NodeType::Done)
			{
				EdgePathType pathType = getPathType(e);
				OGDF_ASSERT(pathType != EdgePathType::Unknown);

				if(pathType == EdgePathType::NoPath) {
					// extend the path
					appendEdge(e);
					adj = e->adjTarget();
					v = e->target();
				} else if(pathType == EdgePathType::TargetPath) {
					// merge path
					node w = e->target();

					// remove tangling target path
					edge f;
					while(m_pred[w] != nullptr) {
						f = m_pred[w];
						w = f->source();
						dropEdge(f);
					}

					m_status[w] = NodeType::Done;
					appendEdge(e);

					return true;
				} else {
					// remove source path cycle
					OGDF_ASSERT(pathType == EdgePathType::SourcePath);
					node w = e->target();

					node formerSource;
					while(e->source() != w) {
						formerSource = e->source();
						if(e->target() != w) {
							dropEdge(e);
						}
						e = m_pred[formerSource]->adjSource()->theEdge();
					}

					dropEdge(e);
					adj = e->adjSource();
					v = e->source();
				}
			}
		}

		// v is a deadend
		if(v == *this->m_s) {
			return false;
		} else {
			edge e = m_pred[v];
			dropEdge(e);
			return findUppermostPath(e);
		}
	}

	/**
	 * Appends an edge to the current path.
	 *
	 * @param e The edge to be added, must not be part of any path so far
	 */
	void appendEdge(const edge e) {
		node v = e->target();
		OGDF_ASSERT(m_pred[v] == nullptr);

		// update path predecessor
		m_pred[v] = e;
		m_status[v] = NodeType::Path;

		// insert into priority queue while
		// taking into account the partial flow
		TCap value = m_partialFlow + (*this->m_cap)[e];
		m_prioritizedEdges->push(e, shiftPriority(value));
	}

	/**
	 * Removes a single edge from the current path.
	 * Note that the edge is not actually removed from the graph.
	 *
	 * @param e The edge to be removed, must be part of a path
	 */
	void dropEdge(const edge e) {
		node v = e->target();
		OGDF_ASSERT(m_pred[v] == e);
		OGDF_ASSERT(m_status[v] == NodeType::Path);

		// update path predecessor
		m_pred[v] = nullptr;
		m_status[v] = v == *this->m_t ? NodeType::Path : NodeType::Done;

		// remove edge from priority queue
		TCap modCap = unshiftedPriority(e);
		m_prioritizedEdges->decrease(e, std::numeric_limits<TCap>::min());
#ifdef OGDF_DEBUG
		edge f = m_prioritizedEdges->topElement();
#endif
		m_prioritizedEdges->pop();

		// determine the flow on this edge
		(*this->m_flow)[e] = m_partialFlow - (modCap - (*this->m_cap)[e]);

		OGDF_ASSERT(e == f);
	}

	/**
	 * Performs an alternating backtracking from source and target to determine
	 * whether the new node is part of the source or target path.
	 *
	 * @param e The newly encountered edge from the old node to a new one.
	 */
	EdgePathType getPathType(const edge e) const {
		node v = e->source();
		node w = e->target();

		EdgePathType result = m_status[w] == NodeType::Path ? EdgePathType::Unknown : EdgePathType::NoPath;

		while(result == EdgePathType::Unknown) {
			if(v == e->target() || w == *this->m_s) {
				result = EdgePathType::SourcePath;
			} else if(m_pred[v] == nullptr || m_pred[w] == nullptr) {
				result = EdgePathType::TargetPath;
			} else if(m_pred[w]->source() == *this->m_s) {
				result = EdgePathType::SourcePath;
			} else {
				OGDF_ASSERT(w != m_pred[w]->source());
				OGDF_ASSERT(v != m_pred[v]->source());

				w = m_pred[w]->source();
				v = m_pred[v]->source();
			}
		}

		return result;
	}
};

}
