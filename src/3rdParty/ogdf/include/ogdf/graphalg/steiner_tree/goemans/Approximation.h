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

#include <ogdf/graphalg/MaxFlowGoldbergTarjan.h>
#include <ogdf/graphalg/MinCostFlowReinelt.h>
#include <ogdf/graphalg/steiner_tree/goemans/BlowupComponents.h>
#include <ogdf/graphalg/steiner_tree/goemans/CoreEdgeRandomSpanningTree.h>

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

	//! Computes the rank of the gammoid (given by the blowup graph)
	int gammoidGetRank(const BlowupGraph<T> &blowupGraph) const
	{
		MaxFlowGoldbergTarjan<int> maxFlow(blowupGraph.getGraph());
		return maxFlow.computeValue(blowupGraph.capacities(), blowupGraph.getSource(), blowupGraph.getTarget());
	}

	//! Finds the best component and its maximum-weight basis in the given blowup graph with given core and witness set
	//! \return The component id of the best component
	int findComponentAndMaxBasis(ArrayBuffer<std::pair<node,int>> *&maxBasis,
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
		ArrayBuffer<edge> sourceCoreEdges;
		EdgeArray<double> cost(blowupGraph.getGraph(), 0);
		for (ListConstIterator<node> it = blowupGraph.core().rbegin(); it.valid(); --it) { // XXX: why backwards?
			// compute weight of core edge
			const node v = *it;
			edge tmp = v->firstAdj()->theEdge();
			double weight = (double)blowupGraph.getCost(tmp);
			for (edge e : blowupGraph.witnessList(v)) {
				OGDF_ASSERT(blowupGraph.numberOfWitnesses(e) > 0);
				weight += (double)blowupGraph.getCost(e) / blowupGraph.numberOfWitnesses(e);
			}

			// add edges from source to core edges v
			edge e = blowupGraph.newEdge(blowupGraph.getSource(), v, 0, blowupGraph.getCapacity(tmp));
			cost[e] = -weight;
			sourceCoreEdges.push(e);
		}

		maxBasis = nullptr;
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
			ArrayBuffer<edge> Q_to_target;
			for (node t : gamma.terminals(id)) {
				const edge e = blowupGraph.newEdge(t, blowupGraph.getTarget(), 0,
				  blowupGraph.getLCM() * blowupGraph.getY());
				// the last value is an upper bound for the capacity
				Q_to_target.push(e);
			}

			ArrayBuffer<std::pair<node,int>> *basis = new ArrayBuffer<std::pair<node,int>>;

			int rank = gammoidGetRank(blowupGraph);
			supply[blowupGraph.getSource()] = rank;
			supply[blowupGraph.getTarget()] = -rank;

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

			// remove temporary edges for multi-target max-flow again
			blowupGraph.delEdges(Q_to_target);
			// XXX/TODO: we could also keep target edges from all terminals to the target
			// and just change the capacity

			// we choose the component with max cost*N <= weight
			if (gamma.cost(id) * blowupGraph.getLCM() <= weight + m_eps) {
				maxBasis = basis;
				blowupGraph.delEdges(sourceCoreEdges); // clean up (XXX: check if necessary)
				return id;
			}
			delete basis;
		}
		return 0; // no component chosen, fail
	}

	//! For the end of the algorithm: find cheapest component and choose all remaining core edges as basis
	//! \return The component id of the cheapest component
	int findCheapestComponentAndRemainingBasis(ArrayBuffer<std::pair<node,int>> *&maxBasis,
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
		maxBasis = new ArrayBuffer<std::pair<node,int>>();
		for (node v : blowupGraph.core()) {
			edge tmp = v->lastAdj()->theEdge();
			maxBasis->push(std::make_pair(v, blowupGraph.getCapacity(tmp)));
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

	//! Remove basis (given by \p v, a core edge node of the maximum basis) and cleanup
	void removeBasisAndCleanup(BlowupGraph<T> &blowupGraph, BlowupComponents<T> &gamma, node v)
	{
		blowupGraph.removeBasis(v, [&](edge e) {
			// in this case we have just fixed the direction of the component,
			// so e->source() is the new root of the component -> update gamma
			gamma.setRootEdge(gamma.id(e->target()), e);
		});
	}

	//! Remove a given basis and cleanup, the basis may be given fractionally
	void removeFractionalBasisAndCleanup(ArrayBuffer<std::pair<node,int>> &basis,
	                                     BlowupGraph<T> &blowupGraph,
	                                     BlowupComponents<T> &gamma)
	{
		// remove B from K (K := K \ B) and from blowup graph (X := X - B)
		// and, while at it, remove cleanup edges from blowup graph (X := X - F)
		// and fix components that have no incoming edges
		ArrayBuffer<Prioritized<node, int>> fractionalCoreEdges; // we defer fractional basis elements
		for (auto p : basis) {
			const node v = p.first;
			const int count = p.second;
			OGDF_ASSERT(v->degree() == 2);
			int origCap = blowupGraph.getCapacity(v->firstAdj()->theEdge());
			OGDF_ASSERT(count <= origCap);
			if (count < origCap) { // only remove a fraction?
				fractionalCoreEdges.push(Prioritized<node,int>(v, -count));
			} else {
				// we are deleting the core edge from the whole component
				blowupGraph.delCore(v);
				removeBasisAndCleanup(blowupGraph, gamma, v);
			}
		}
		fractionalCoreEdges.quicksort(); // sort decreasing by flow value
		for (auto p : fractionalCoreEdges) {
			const node v = p.item();
			const int count = -p.priority();
			OGDF_ASSERT(v->degree() == 2);
			int origCap = blowupGraph.getCapacity(v->firstAdj()->theEdge());
			OGDF_ASSERT(count <= origCap);
			// copy (split) the component
			blowupGraph.copyComponent(gamma.rootEdge(gamma.id(v)), count, origCap - count);
			// we are deleting the core edge from the whole component
			blowupGraph.delCore(v);
			removeBasisAndCleanup(blowupGraph, gamma, v);
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
	CoreEdgeRandomSpanningTree<T> cer(m_rng);
	BlowupGraph<T> blowupGraph(m_G, m_terminals, m_fullCompStore, cer, m_eps);

	while (blowupGraph.terminals().size() > 1) { // T is not a Steiner tree
		// TODO: maybe we should initially compute the blowup components when we *build*
		//       the gammoid graph and update it on each delEdge/delNode
		BlowupComponents<T> gamma(blowupGraph); // Gamma(X)

		OGDF_ASSERT(isLoopFree(blowupGraph.getGraph()));

		// take a component Q in Gamma(X)
		ArrayBuffer<std::pair<node,int>> *maxBasis;
		int compId = blowupGraph.getY() > 0
		     ? findComponentAndMaxBasis(maxBasis, blowupGraph, gamma)
		     : findCheapestComponentAndRemainingBasis(maxBasis, blowupGraph, gamma);
		OGDF_ASSERT(compId);

		// add component Q to T
		addComponent(isNewTerminal, blowupGraph, gamma.rootEdge(compId));

		// remove (maybe fractional) basis and do all the small things necessary for update
		removeFractionalBasisAndCleanup(*maxBasis, blowupGraph, gamma);
		delete maxBasis;

		// contract (X := X / Q)
		auto it = gamma.terminals(compId).begin();
		node v = *it;
		for (++it; it != gamma.terminals(compId).end(); ++it) {
			blowupGraph.contract(v, *it);
		}

		if (blowupGraph.terminals().size() > 1) {
			blowupGraph.updateSpecialCapacities();
		}
	}
}

}
}
}
