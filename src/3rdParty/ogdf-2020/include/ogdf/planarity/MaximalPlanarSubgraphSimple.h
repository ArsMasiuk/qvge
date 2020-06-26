/** \file
 * \brief Declaration of class MaximalPlanarSubgraphSimple.
 *
 * \author Tilo Wiedera, Ivo Hedtke
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

#include <ogdf/planarity/PlanarSubgraphEmpty.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/DisjointSets.h>
#include <ogdf/basic/Math.h>
#include <type_traits>

namespace ogdf {

//! @cond

template<typename TCost, class Enable = void>
class MaximalPlanarSubgraphSimple {};

//! @endcond

//! Naive maximal planar subgraph approach that extends a configurable non-maximal subgraph heuristic.
/**
 * @ingroup ga-plansub
 *
 * A (possibly non-maximal) planar subgraph is first computed by the set heuristic (default: ogdf::PlanarSubgraphEmpty).
 * Secondly, we iterate over all non-inserted edges performing one planarity test each.
 * Each edge is inserted if planarity can be maintained and discarded otherwise.
 */
template<typename TCost>
class MaximalPlanarSubgraphSimple<TCost, typename std::enable_if<std::is_integral<TCost>::value>::type> : public PlanarSubgraphModule<TCost> {

public:
	//! Constructor
	MaximalPlanarSubgraphSimple()
	: m_heuristic(*(new PlanarSubgraphEmpty<TCost>))
	, m_deleteHeuristic(true) {}

	/**
	 * \brief Constructor with user given heuristic that is executed before extending the solution to maximality.
	 * @param heuristic
	 */
	explicit MaximalPlanarSubgraphSimple(PlanarSubgraphModule<TCost> &heuristic)
	: m_heuristic(heuristic)
	, m_deleteHeuristic(false) { }

	//!Desctructor
	virtual ~MaximalPlanarSubgraphSimple() {
		if(m_deleteHeuristic) {
			delete &m_heuristic;
		}
	}

	//! Clone method
	virtual MaximalPlanarSubgraphSimple *clone() const override {
		auto result = new MaximalPlanarSubgraphSimple(*(m_heuristic.clone()));
		result->m_deleteHeuristic = true; // normally a given heuristic is not deleted by the destructor
		return result;
	}

protected:

	/**
	 * \brief Computes the set of edges \p delEdges which have to be deleted to obtain the planar subgraph.
	 * @param graph is the input graph.
	 * @param preferredEdges are edges that should be contained in the planar subgraph.
	 * @param delEdges is the set of edges that need to be deleted to obtain the planar subgraph.
	 * @param pCost is apointer to an edge array containing the edge costs; this pointer
	 *        can be 0 if no costs are given (all edges have cost 1).
	 * @param preferredImplyPlanar indicates that the edges \p preferredEdges induce a planar graph.
	 */
	virtual Module::ReturnType
	doCall(
		const Graph &graph,
		const List<edge> &preferredEdges,
		List<edge> &delEdges,
		const EdgeArray<TCost>  *pCost,
		bool preferredImplyPlanar) override
	{
		Module::ReturnType result;
		delEdges.clear();
		List<edge> heuDelEdges;
		if(pCost == nullptr) {
			result = m_heuristic.call(graph, preferredEdges, heuDelEdges, preferredImplyPlanar);
		} else {
			result = m_heuristic.call(graph, *pCost, preferredEdges, heuDelEdges, preferredImplyPlanar);
			heuDelEdges.quicksort(GenericComparer<edge, TCost>(*pCost));
		}
		if(Module::isSolution(result)) {
			GraphCopy copy(graph);
			for (edge e : heuDelEdges) {
				copy.delEdge(copy.copy(e));
			}
			for (edge e : heuDelEdges) {
				edge f = copy.newEdge(e);

				if (!isPlanar(copy)) {
					delEdges.pushBack(e);
					copy.delEdge(f);
				}
			}
		}
		return result;
	}


private:
	PlanarSubgraphModule<TCost> &m_heuristic; //!< user given heuristic
	bool m_deleteHeuristic; //!< flag to store we have to delete a self created heuristic
};



template<typename TCost>
class MaximalPlanarSubgraphSimple<TCost, typename std::enable_if<std::is_floating_point<TCost>::value>::type> : public PlanarSubgraphModule<TCost> {

public:
	/**
	 * \brief Constructor
	 *
	 * Default constructor with PlanarSubgraphEmpty as pre-executed heuristic before the extension to maximality.
	 * randomness is set to 0 and runs is set to 1
	 */
	MaximalPlanarSubgraphSimple()
		: m_heuristic(*(new PlanarSubgraphEmpty<TCost>))
		, m_deleteHeuristic(true)
		, m_randomness(0.0)
		, m_randomGenerator()
		, m_runs(1) { }

	/**
	 * \brief Constructor
	 * @param heuristic user given instance of PlanarSubgraphModule that is executed before extending the solution to maximality.
	 * @param randomness randomness of the process: use 0 to compute everything based on the costs, use 1 for completely randomness
	 * @param runs number of runs when randomness > 0, the best solution is returned by call
	 */
	explicit MaximalPlanarSubgraphSimple(PlanarSubgraphModule<TCost> &heuristic, double randomness = 0.0, unsigned int runs = 1)
		: m_heuristic(heuristic)
		, m_deleteHeuristic(false)
		, m_randomness(randomness)
		, m_randomGenerator()
		, m_runs(runs)
	{
		OGDF_ASSERT( runs > 0 );
	}

	//! Destructor
	virtual ~MaximalPlanarSubgraphSimple() {
		if(m_deleteHeuristic) {
			delete &m_heuristic;
		}
	}

	//! Clone method
	virtual MaximalPlanarSubgraphSimple *clone() const override {
		auto result = new MaximalPlanarSubgraphSimple(*(m_heuristic.clone()), m_randomness, m_runs);
		result->m_deleteHeuristic = true; // normally a given heuristic is not deleted by the destructor
		return result;
	}

	/**
	 * @brief set seed of std::default_random_engine generator to use when randomness > 0
	 * @param seed user given seed value
	 */
	void setSeed(unsigned seed) {
		m_randomGenerator.seed(seed);
	}


protected:

	/**
	 * \brief Computes the set of edges \p delEdges which have to be deleted to obtain the planar subgraph.
	 * @param graph is the input graph.
	 * @param preferredEdges are edges that should be contained in the planar subgraph.
	 * @param delEdges is the set of edges that need to be deleted to obtain the planar subgraph.
	 * @param pCost is apointer to an edge array containing the edge costs; this pointer
	 *        can be 0 if no costs are given (all edges have cost 1).
	 * @param preferredImplyPlanar indicates that the edges \p preferredEdges induce a planar graph.
	 */
	virtual Module::ReturnType
	doCall(
		const Graph &graph,
		const List<edge> &preferredEdges,
		List<edge> &delEdges,
		const EdgeArray<TCost>  *pCost,
		bool preferredImplyPlanar) override
	{
		Module::ReturnType result = Module::ReturnType::Error;
		delEdges.clear();

		// scale the costs and do multiple runs (if needed)
		List<edge> delEdgesCurrentBest;

		// normalize costs to [0,1] and apply randomness
		EdgeArray<TCost> normalizedCost(graph);

		for (auto i=0u; i < m_runs; i++) {

			List<edge> heuDelEdges;

			if(pCost == nullptr) {
				result = m_heuristic.call(graph, preferredEdges, heuDelEdges, preferredImplyPlanar);
			} else {
				std::uniform_real_distribution<TCost> distribution (0.0,1.0);
				edge firstEdge = graph.firstEdge();
				TCost maxCost = firstEdge == nullptr ? 0 : (*pCost)[firstEdge];
				TCost minCost = firstEdge == nullptr ? 0 : (*pCost)[firstEdge];
				for (edge e: graph.edges) {
					Math::updateMax(maxCost, (*pCost)[e]);
					Math::updateMin(minCost, (*pCost)[e]);
				}
				for (edge e: graph.edges) {
					// do not merge with first FOR !
					// normalized = pCost transformed to [0,1]
					TCost normalized = 1;
					if (maxCost > minCost) {
						normalized = ((*pCost)[e] - minCost) / (maxCost - minCost);
					}
					// now use randomness
					normalizedCost[e] = (1.0 - m_randomness)*normalized + m_randomness*distribution(m_randomGenerator);
				}
				result = m_heuristic.call(graph, normalizedCost, preferredEdges, heuDelEdges, preferredImplyPlanar);
			}

			if(Module::isSolution(result)) {
				GraphCopy copy(graph);

				if (pCost != nullptr) {
					GenericComparer<edge, TCost> cmp(normalizedCost);
					heuDelEdges.quicksort(cmp);
				}

				for (edge e : heuDelEdges) {
					copy.delEdge(copy.copy(e));
				}

				delEdgesCurrentBest.clear();
				for (edge e : heuDelEdges) {
					edge f = copy.newEdge(e);

					if (!isPlanar(copy)) {
						delEdgesCurrentBest.pushBack(e);
						copy.delEdge(f);
					}
				}

				if (pCost == nullptr) {
					if (i == 0 || delEdgesCurrentBest.size() < delEdges.size()) {
						// better solution: copy to delEdges
						delEdges.clear();
						for (auto e: delEdgesCurrentBest) { delEdges.pushBack(e); };
					}
				} else {
					if (i == 0 || weightOfList(delEdgesCurrentBest,normalizedCost) < weightOfList(delEdges,normalizedCost)) {
						// better solution: copy to delEdges
						delEdges.clear();
						for (auto e: delEdgesCurrentBest) { delEdges.pushBack(e); };
					}
				}
			}
		}

		return result;
	}

private:
	PlanarSubgraphModule<TCost> &m_heuristic; //!< user given heuristic
	bool m_deleteHeuristic; //!< flag to store we have to delete a self created heuristic
	double m_randomness; //!< randomness of the process: use 0 to compute everything based on the costs, use 1 for completely randomness
	std::default_random_engine m_randomGenerator; //!< random generator to use with std::uniform_real_distribution
	unsigned int m_runs; //!< number of runs when algorithms is randomized

	/**
	 * @param list list of edges to sum over
	 * @param weights EdgeArray with weights
	 * @return sum the values in \p weights over the given list \p list
	 */
	TCost weightOfList(const List<edge>& list, const EdgeArray<TCost>& weights) {
		TCost result = 0.0;
		for (auto e: list) {
			result += weights[e];
		}
		return result;
	}
};

}
