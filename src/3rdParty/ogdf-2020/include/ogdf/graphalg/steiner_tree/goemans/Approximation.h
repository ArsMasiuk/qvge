/** \file
 * \brief Definition of ogdf::steiner_tree::goemans::Approximation class template
 *
 * \author Stephan Beyer
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

#include <memory>
#include <ogdf/graphalg/MaxFlowGoldbergTarjan.h>
#include <ogdf/graphalg/MinCostFlowReinelt.h>
#include <ogdf/graphalg/steiner_tree/goemans/BlowupComponents.h>
#include <ogdf/graphalg/steiner_tree/goemans/CoreEdgeRandomSpanningTree.h>

//#define OGDF_STEINER_TREE_GOEMANS_APPROXIMATION_LOGGING

namespace ogdf {
namespace steiner_tree {
namespace goemans {

//! The actual 1.39-approximation algorithm by Goemans et al. with a set of terminalized nodes as result
template<typename T>
class Approximation
{
	const EdgeWeightedGraph<T> &m_G;
	const NodeArray<bool> &m_isTerminal;
	const List<node> &m_terminals;
	const FullComponentWithExtraStore<T, double> &m_fullCompStore; //!< all enumerated full components, with solution

	const double m_eps; //!< epsilon for double operations

	std::minstd_rand m_rng;

	//! Add edges into a blowup graph and delete them on destruction
	struct TemporaryEdges : ArrayBuffer<edge> {
		//! Construct object for a specific \p blowupGraph
		TemporaryEdges(BlowupGraph<T>& blowupGraph) : m_blowupGraph(blowupGraph) {}

		//! Add a temporary edge to the blowup graph
		edge add(node v, node w, T cost, int capacity) {
			edge e = m_blowupGraph.newEdge(v, w, cost, capacity);
			this->push(e);
			return e;
		}

		//! Remove the edges again
		~TemporaryEdges() {
			m_blowupGraph.delEdges(*this);
		}

	private:
		BlowupGraph<T>& m_blowupGraph;
	};

	//! Computes the rank of the gammoid (given by the blowup graph)
	int gammoidGetRank(const BlowupGraph<T> &blowupGraph) const
	{
		MaxFlowGoldbergTarjan<int> maxFlow(blowupGraph.getGraph());
		return maxFlow.computeValue(blowupGraph.capacities(), blowupGraph.getSource(), blowupGraph.getTarget());
	}

	//! Finds the best component and its maximum-weight basis in the given blowup graph with given core and witness set
	//! \return The component id of the best component
	int findComponentAndMaxBasis(std::unique_ptr<ArrayBuffer<std::pair<node,int>>> &maxBasis,
	                             BlowupGraph<T> &blowupGraph,
	                             const BlowupComponents<T> &gamma)
	{
		// there should always be saturated flow to the component roots
		// (contracted matroid)
		EdgeArray<int> lB(blowupGraph.getGraph(), 0);
		for (adjEntry adj : blowupGraph.getSource()->adjEntries) {
			const edge e = adj->theEdge();
			lB[e] = blowupGraph.getCapacity(e);
		}

		// compute weights of core edges and add source->core edges
		TemporaryEdges sourceCoreEdges(blowupGraph);
		EdgeArray<double> cost(blowupGraph.getGraph(), 0);
		for (node v : blowupGraph.core()) {
			// compute weight of core edge
			double weight = blowupGraph.computeCoreWeight(v);

			// add edges from source to core edges v
			edge e = sourceCoreEdges.add(blowupGraph.getSource(), v, 0, blowupGraph.getCoreCapacity(v));
			cost[e] = -weight;
		}

		NodeArray<int> supply(blowupGraph.getGraph(), 0);
		EdgeArray<int> flow(blowupGraph.getGraph());
		MinCostFlowReinelt<double> mcf;

		for (int id = 1; id <= gamma.size(); ++id) {
			/* We want to find maximum-weight basis B \in B^K_Q
			 * B_Q = minimal edge set to remove after contracting Q to obtain feasible solution (blowup graph)
			 *     = bases of gammoid M_Q
			 * B^K_Q = { B \in B_Q | B \subseteq K }  [K is the set of core edges]
			 *       = bases of gammoid M^K_Q
			 * M^K_Q is gammoid "obtained by restricting M_Q to K"
			 * M_Q = M'_Q / X'  (M'_Q contracted by X') is a gammoid
			 * M'_Q = gammoid from   X \cup X'  to  Y  in  D'  with arc-disjointness instead of vertex-disjointness
			 *    D' is D like for separation but without source node and with interim node v_e for each edge e
			 *    X  = inserted nodes v_e in each edge in blowup graph
			 *    X' = the (arbitrary) roots of each component
			 *    Y  = Q \cup {t}
			 * that means:
			 *   I (subset of X) is an independent set of M_Q
			 *  <=> (if X' is always an independent set in M'_Q ... which we assume here)
			 *   I \cup X'  is an independent set of M'_Q
			 * Restricting to K means:
			 *   only put v_e in X with e \in K (that is, we only need to *generate* v_e if e \in K)
			 * Hence:
			 * - we generate D'^K (this is blowupGraph)
			 * - compute the max flow from X^K \cup X' to Q \cup {t}
			 * - ASSERT that X' is "saturated"!
			 * - check which subset of X is saturated -> these are the nodes representing the edge set we need
			 */
			// add edges from component's terminals to target
			TemporaryEdges Q_to_target(blowupGraph);
			for (node t : gamma.terminals(id)) {
				Q_to_target.add(t, blowupGraph.getTarget(), 0, blowupGraph.getLCM() * blowupGraph.getY());
				// the last value is an upper bound for the capacity
			}
			// TODO: we could also use static edges from all terminals to the target
			// and just change their capacities each time; needs extra data structures

			std::unique_ptr<ArrayBuffer<std::pair<node,int>>> basis(new ArrayBuffer<std::pair<node,int>>);

			int rank = gammoidGetRank(blowupGraph);
			OGDF_ASSERT(rank >= blowupGraph.getY() + blowupGraph.getLCM());
			supply[blowupGraph.getSource()] = rank;
			supply[blowupGraph.getTarget()] = -rank;

#ifdef OGDF_STEINER_TREE_GOEMANS_APPROXIMATION_LOGGING
			std::cout << "Computing min-cost flow for blowup component " << id << " of " << gamma.size() << std::endl;
			std::cout << " * terminals of component are " << gamma.terminals(id) << std::endl;
			for (node v : blowupGraph.getGraph().nodes) {
				if (supply[v] > 0) {
					std::cout << " * supply node " << v << " with supply " << supply[v] << std::endl;
				}
				if (supply[v] < 0) {
					std::cout << " * demand node " << v << " with demand " << -supply[v] << std::endl;
				}
			}
			for (edge e : blowupGraph.getGraph().edges) {
				std::cout
				  << " * edge " << e
				  << " with cost " << blowupGraph.getCost(e)
				  << " and flow bounds [" << lB[e]
				  << ", " << blowupGraph.getCapacity(e)
				  << "]" << std::endl;
			}
#endif

			// find maximum weight basis
#ifdef OGDF_DEBUG
			bool feasible =
#endif
			  mcf.call(blowupGraph.getGraph(), lB, blowupGraph.capacities(), cost, supply, flow);
			OGDF_ASSERT(feasible);
			OGDF_ASSERT(mcf.checkComputedFlow(blowupGraph.getGraph(), lB, blowupGraph.capacities(), cost, supply, flow));

			double weight(0);
			for (edge e : sourceCoreEdges) {
				if (flow[e] > 0) {
					basis->push(std::make_pair(e->target(), flow[e]));
					weight -= flow[e] * cost[e];
				}
			}

#ifdef OGDF_STEINER_TREE_GOEMANS_APPROXIMATION_LOGGING
			std::cout
			  << "Basis weight is " << weight << std::endl
			  << "Checking if "
			  << gamma.cost(id) << "(component cost) * " << blowupGraph.getLCM()
			  << "(lcm) <= " << weight
			  << "(basis weight)" << std::endl;
#endif

			// we choose the component with max cost*N <= weight
			if (gamma.cost(id) * blowupGraph.getLCM() <= weight + m_eps) {
#ifdef OGDF_STEINER_TREE_GOEMANS_APPROXIMATION_LOGGING
				std::cout << "Using basis because it is feasible." << std::endl;
#endif
				maxBasis.swap(basis);
				return id;
			}
		}
		return 0; // no component chosen, fail
	}

	//! For the end of the algorithm: find cheapest component and choose all remaining core edges as basis
	//! \return The component id of the cheapest component
	int findCheapestComponentAndRemainingBasis(std::unique_ptr<ArrayBuffer<std::pair<node,int>>> &maxBasis,
	                                           const BlowupGraph<T> &blowupGraph,
	                                           const BlowupComponents<T> &gamma)
	{
		// find cheapest component
		int compId = 0;
		double cost = 0;
		for (int id = 1; id <= gamma.size(); ++id) {
			if (gamma.cost(id) > cost) {
				cost = gamma.cost(id);
				compId = id;
			}
		}
		// use all core edges as basis
		maxBasis.reset(new ArrayBuffer<std::pair<node,int>>());
		for (node v : blowupGraph.core()) {
			maxBasis->push(std::make_pair(v, blowupGraph.getCoreCapacity(v)));
		}
		return compId;
	}

	//! Add a component of the blowup graph to the final solution (by changing nonterminals to terminals)
	void addComponent(NodeArray<bool> &isNewTerminal, const BlowupGraph<T> &blowupGraph, const edge rootEdge)
	{
		OGDF_ASSERT(blowupGraph.isTerminal(rootEdge->source()));
		ArrayBuffer<node> stack;
		stack.push(rootEdge->target());
		while (!stack.empty()) {
			const node v = stack.popRet();
			if (blowupGraph.isTerminal(v)) {
				continue;
			}
			const node vO = blowupGraph.getOriginal(v);
			if (vO) {
				isNewTerminal[vO] = true;
			}
			for (adjEntry adj : v->adjEntries) {
				const node w = adj->theEdge()->target();
				if (v != w) { // outgoing edge
					stack.push(w);
				}
			}
		}
	}

	//! Remove a given basis and cleanup, the basis may be given fractionally
	void removeFractionalBasisAndCleanup(ArrayBuffer<std::pair<node,int>> &basis,
	                                     BlowupGraph<T> &blowupGraph,
	                                     BlowupComponents<T> &gamma)
	{
#ifdef OGDF_STEINER_TREE_GOEMANS_APPROXIMATION_LOGGING
		std::cout << "Remove basis from blowup graph" << std::endl;
#endif
		// remove B from K (K := K \ B) and from blowup graph (X := X - B)
		// and, while at it, remove cleanup edges from blowup graph (X := X - F)
		// and fix components that have no incoming edges
		ArrayBuffer<Prioritized<node, int>> fractionalCoreEdges; // we defer fractional basis elements
		for (auto p : basis) {
			const node v = p.first;
			const int count = p.second;
			int origCap = blowupGraph.getCoreCapacity(v);
			OGDF_ASSERT(count <= origCap);
#ifdef OGDF_STEINER_TREE_GOEMANS_APPROXIMATION_LOGGING
			std::cout << " * node " << v << " with count " << count << " (of " << origCap << ")" << std::endl;
#endif
			if (count < origCap) { // only remove a fraction?
				fractionalCoreEdges.push(Prioritized<node,int>(v, -count));
#ifdef OGDF_STEINER_TREE_GOEMANS_APPROXIMATION_LOGGING
				std::cout << "   -> deferred because fractional" << std::endl;
#endif
			} else {
				// we are deleting the core edge from the whole component
				blowupGraph.delCore(v);
				blowupGraph.removeBasis(v);
#ifdef OGDF_STEINER_TREE_GOEMANS_APPROXIMATION_LOGGING
				std::cout << "   -> done" << std::endl;
#endif
			}
		}
		fractionalCoreEdges.quicksort(); // sort decreasing by flow value
#ifdef OGDF_STEINER_TREE_GOEMANS_APPROXIMATION_LOGGING
		if (!fractionalCoreEdges.empty()) {
			std::cout << "Deferred core edges:" << std::endl;
		}
#endif
		for (auto p : fractionalCoreEdges) {
			const node v = p.item();
			const int count = -p.priority();
			int origCap = blowupGraph.getCoreCapacity(v);
#ifdef OGDF_STEINER_TREE_GOEMANS_APPROXIMATION_LOGGING
			std::cout << " * node " << v << " with count " << count << " of " << origCap << std::endl;
#endif
			OGDF_ASSERT(count <= origCap);
			// copy (split) the component
			blowupGraph.copyComponent(blowupGraph.findRootEdge(v), count, origCap - count);
			// we are deleting the core edge from the whole component
			blowupGraph.delCore(v);
			blowupGraph.removeBasis(v);
		}
	}

public:
	//! Initialize everything
	Approximation(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal,
	              const FullComponentWithExtraStore<T, double> &fullCompStore,
	              const std::minstd_rand &rng, double eps = 1e-8)
	  : m_G(G)
	  , m_isTerminal(isTerminal)
	  , m_terminals(terminals)
	  , m_fullCompStore(fullCompStore)
	  , m_eps(eps)
	  , m_rng(rng)
	{
	}

	//! Perform the actual approximation algorithm on the LP solution
	//! @param isNewTerminal is an input/output parameter where new terminals are set to true
	void solve(NodeArray<bool> &isNewTerminal);
};

template<typename T>
void
Approximation<T>::solve(NodeArray<bool> &isNewTerminal)
{
#ifdef OGDF_STEINER_TREE_GOEMANS_APPROXIMATION_LOGGING
	std::cout << "Start solving based on LP solution" << std::endl;
	int iteration = 0;
#endif
	CoreEdgeRandomSpanningTree<T> cer(m_rng);
	BlowupGraph<T> blowupGraph(m_G, m_terminals, m_fullCompStore, cer, m_eps);

	while (blowupGraph.terminals().size() > 1) { // T is not a Steiner tree
#ifdef OGDF_STEINER_TREE_GOEMANS_APPROXIMATION_LOGGING
		std::cout << "Iteration " << ++iteration << " with " << blowupGraph.terminals().size() << " terminals" << std::endl;
#endif
		BlowupComponents<T> gamma(blowupGraph); // Gamma(X)

		OGDF_ASSERT(isLoopFree(blowupGraph.getGraph()));

		// take a component Q in Gamma(X)
		std::unique_ptr<ArrayBuffer<std::pair<node,int>>> maxBasis;
		int compId = blowupGraph.getY() > 0
		     ? findComponentAndMaxBasis(maxBasis, blowupGraph, gamma)
		     : findCheapestComponentAndRemainingBasis(maxBasis, blowupGraph, gamma);
		OGDF_ASSERT(compId != 0);

		// add component Q to T
		addComponent(isNewTerminal, blowupGraph, gamma.rootEdge(compId));

		// remove (maybe fractional) basis and do all the small things necessary for update
		removeFractionalBasisAndCleanup(*maxBasis, blowupGraph, gamma);

		// contract (X := X / Q)
		blowupGraph.contract(gamma.terminals(compId));

		if (blowupGraph.terminals().size() > 1) {
			blowupGraph.updateSpecialCapacities();
		}
	}
}

}
}
}
