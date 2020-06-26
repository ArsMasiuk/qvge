/** \file
 * \brief Implementation of Edmonds-Karp max-flow algorithm.
 * Runtime O( |E|^2 * |V| )
 *
 * \author Stephan Beyer, Ivo Hedtke
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

namespace ogdf {

//! Computes a max flow via Edmonds-Karp.
/**
 * @ingroup ga-flow
 */
template<typename TCap>
class MaxFlowEdmondsKarp : public MaxFlowModule<TCap>
{
private:

	//! If there is an augumenting path: do an interation step. Otherwise return
	//! false so that the algorithm can stop.
	bool augmentShortestSourceSinkPath()
	{
		NodeArray<adjEntry> pred(*this->m_G, nullptr); // edges from the predecessor
		List<node> queue;
		queue.pushBack(*this->m_s);

		while (!queue.empty()) {
			node v = queue.popFrontRet();
			if (v == *this->m_t) {
				// find minimum residual capacity value on s-t-path
				TCap augmentVal = std::numeric_limits<TCap>::max();
				for (node w = v; pred[w]; w = pred[w]->theNode()) {
					const adjEntry adj = pred[w];
					const edge e = adj->theEdge();
					if (e->target() == w) { // real edge e = vw
						if ((*this->m_cap)[e] - (*this->m_flow)[e] < augmentVal) {
							augmentVal = (*this->m_cap)[e] - (*this->m_flow)[e];
						}
					} else { // virtual edge e = wv
						if ((*this->m_flow)[e] < augmentVal) {
							augmentVal = (*this->m_flow)[e];
						}
					}
				}
				// augment s-t-path by this value
				for (node w = v; pred[w]; w = pred[w]->theNode()) {
					const adjEntry adj = pred[w];
					const edge e = adj->theEdge();
					if (e->target() == w) { // real edge e = vw
						(*this->m_flow)[e] += augmentVal;
					} else { // virtual edge e = wv
						(*this->m_flow)[e] -= augmentVal;
					}
				}
				return true;
			}
			for(adjEntry adj : v->adjEntries) {
				const node w = adj->twinNode();
				if (w != (*this->m_s)
					&& !pred[w]) // if not already visited
				{
					const edge e = adj->theEdge();
					if (e->source() == v) { // real edge e = vw
						if ( (*this->m_et).greater((*this->m_cap)[e],(*this->m_flow)[e]) ) { // reachable in residual graph
							queue.pushBack(w);
							pred[w] = adj;
						}
					} else { // virtual edge (adj) wv
						if ( (*this->m_et).greater( (*this->m_flow)[e] ,  (TCap) 0 )) { // reachable in residual graph
							queue.pushBack(w);
							pred[w] = adj;
						}
					}
				}
			}
		}

		return false;
	}

public:

	//! Implementation of computeValue from the super class. The flow array is
	//! cleared, \p cap, \p s and \p t are stored and Edmonds&Karp starts. After
	//! this first phase, the flow itself is already computed!
	//! Returns 0 if source and sink are identical.
	/**
	 * @return The value of the flow.
	 * @param cap is the EdgeArray of capacities.
	 * @param s is the source.
	 * @param t is the sink.
	 */
	TCap computeValue(const EdgeArray<TCap> &cap, const node &s, const node &t)
	{
		// clear old flow
		this->m_flow->fill((TCap) 0);
		// store capacity, source and sink
		this->m_cap = &cap;
		this->m_s = &s;
		this->m_t = &t;
		OGDF_ASSERT(this->isFeasibleInstance());

		if (*this->m_s == *this->m_t) {
			return (TCap) 0;
		}

		while (augmentShortestSourceSinkPath());
		TCap flowValue = 0;
		for(adjEntry adj : s->adjEntries) {
			edge e = adj->theEdge();
			if(e->source() == s) {
				flowValue += (*this->m_flow)[e];
			} else {
				flowValue -= (*this->m_flow)[e];
			}
		}
		return flowValue;
	}

	//! Implementation of computeFlowAfterValue from the super class. This does
	//! nothing, because the algorithm is finished after computeValue.
	void computeFlowAfterValue() { /* nothing to do here */ }

	// use methods from super class
	using MaxFlowModule<TCap>::useEpsilonTest;
	using MaxFlowModule<TCap>::computeFlow;
	using MaxFlowModule<TCap>::computeFlowAfterValue;
	using MaxFlowModule<TCap>::MaxFlowModule;
	using MaxFlowModule<TCap>::init;
};

}
