/** \file
 * \brief Definition of the ogdf::SteinerTreeLowerBoundDualAscent class template
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

#include <ogdf/basic/Math.h>
#include <ogdf/basic/EpsilonTest.h>
#include <ogdf/graphalg/Dijkstra.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraph.h>

namespace ogdf {
namespace steiner_tree {

//! Computes lower bounds for minimum Steiner tree instances
template<typename T>
class LowerBoundDualAscent {
	struct TerminalData;
	struct TerminalDataReference {
		node v;
		ListIterator<ListIterator<TerminalData>> it;
	};

	struct TerminalData {
		node terminal;
		List<adjEntry> cut;
		AdjEntryArray<ListIterator<adjEntry>> cutIterators;
		NodeArray<bool> inCut;
		ArrayBuffer<TerminalDataReference> references;

		TerminalData(const EdgeWeightedGraph<T>& graph, node t)
		  : terminal(t), cutIterators(graph, nullptr), inCut(graph, false) {}
	};

	EpsilonTest m_eps;
	T m_lower;
	const EdgeWeightedGraph<T>& m_graph;
	List<TerminalData> m_terminals;
	node m_root;
	AdjEntryArray<T> m_reducedCost;
	NodeArray<List<ListIterator<TerminalData>>> m_inTerminalCut; //!< Mapping of nodes to the cuts they are in

	//! Finds the terminal with the smallest cut arc set (of the last iteration)
	ListIterator<TerminalData> chooseTerminal() {
		auto it = m_terminals.begin();
		auto bestIt = it;
		for (; it != m_terminals.end(); ++it) {
			if ((*it).cut.size() < (*bestIt).cut.size()) {
				bestIt = it;
			}
		}

		return bestIt;
	}

	//! Adds a node to the cut and add neighbors recursively
	//! @return False if #m_root is reached
	bool addNode(ListIterator<TerminalData>& it, node t) {
		if (t == m_root) {
			return false;
		}
		TerminalData& td = *it;
		td.inCut[t] = true;
		td.references.push({t, m_inTerminalCut[t].pushBack(it)});
		for (adjEntry adj : t->adjEntries) {
			node w = adj->twinNode();
			if (!td.inCut[w]) {
				// not in cut, so check (recursively) if we should add nodes
				if (m_eps.equal(m_reducedCost[adj], T(0))) {
					if (!addNode(it, w)) { // recurse
						return false;
					}
				} else {
					td.cutIterators[adj] = td.cut.pushBack(adj);
				}
			}
		}

		// delete arcs that are inside the cut
		for (adjEntry adj : t->adjEntries) {
			node w = adj->twinNode();
			if (td.inCut[w]) {
				auto& cutAdjIt = td.cutIterators[adj->twin()];
				if (cutAdjIt != td.cut.end()) {
					td.cut.del(cutAdjIt);
					cutAdjIt = nullptr;
				}
			}
		}

		return true;
	}

	bool addNodeChecked(ListIterator<TerminalData>& it, node w) {
		if (!(*it).inCut[w]) {
			if (!addNode(it, w)) {
				return false;
			}
		}
		return true;
	}

	void removeTerminalData(ListIterator<TerminalData> it) {
		for (auto ref : (*it).references) {
			m_inTerminalCut[ref.v].del(ref.it);
		}

		m_terminals.del(it);
	}

	/**
	 * Assumes that the reduced cost of arc \p adj is zeroed and hence
	 * updates (in relevant cuts) the set of arcs coming into W (where W is the smallest node set
	 * containing a given terminal but not #m_root such that all these arcs have reduced cost
	 * greater than zero; this is done for all terminals affected by \p adj).
	 */
	void extendCut(adjEntry adj) {
		OGDF_ASSERT(m_eps.equal(m_reducedCost[adj], T(0)));
		node v = adj->theNode();
		node w = adj->twinNode();
		for (auto it = m_inTerminalCut[v].begin(); it.valid();) {
			if (!addNodeChecked(*it, w)) {
				auto nextIt = it;
				++nextIt;
				removeTerminalData(*it);
				it = nextIt;
			} else {
				++it;
			}
		}
	}

	//! Finds the cheapest arc in \p cut and returns its cost
	T findCheapestCutArcCost(const TerminalData& td) const {
		OGDF_ASSERT(!td.cut.empty());
		T cost = std::numeric_limits<T>::max();
		for (adjEntry adj : td.cut) {
			Math::updateMin(cost, m_reducedCost[adj]);
		}
		OGDF_ASSERT(cost > 0);
		return cost;
	}

	//! Updates reduced costs and cut data
	void update(TerminalData& td, T delta) {
		// reduce costs
		ArrayBuffer<adjEntry> zeroed;
		for (adjEntry adj : td.cut) {
			m_reducedCost[adj] -= delta;
			OGDF_ASSERT(m_eps.geq(m_reducedCost[adj], T(0)));
			if (m_eps.leq(m_reducedCost[adj], T(0))) {
				zeroed.push(adj);
			}
		}
		m_lower += delta;

		// extend
		for (adjEntry adj : zeroed) {
			extendCut(adj);
		}
	}

public:
	//! Initializes the algorithm
	LowerBoundDualAscent(const EdgeWeightedGraph<T>& graph, const List<node>& terminals, node root, double eps = 1e-6)
	  : m_eps(eps),
	    m_lower(0),
	    m_graph(graph),
	    m_root(nullptr),
	    m_reducedCost(m_graph),
	    m_inTerminalCut(m_graph) {
		for (edge e : graph.edges) {
			m_reducedCost[e->adjSource()] =
			  m_reducedCost[e->adjTarget()] = graph.weight(e);
		}

		for (node t : terminals) {
			if (t == root) {
				m_root = root;
			} else {
				auto it = m_terminals.pushBack(TerminalData(m_graph, t));
				if (!addNode(it, t)) {
					removeTerminalData(it);
				}
			}
		}
		OGDF_ASSERT(m_root != nullptr);
	}

	//! Initializes the algorithm (and takes the first terminal as root)
	LowerBoundDualAscent(const EdgeWeightedGraph<T>& graph, const List<node>& terminals, double eps = 1e-6)
	  : LowerBoundDualAscent(graph, terminals, terminals.front(), eps) {}

	//! Computes the lower bound
	void compute() {
		while (!m_terminals.empty()) {
			auto it = chooseTerminal();
			TerminalData& td = *it;
			update(td, findCheapestCutArcCost(td));
		}
	}

	//! Returns the reduced cost of the arc given by \p adj
	//! @param adj is the adjacency entry of an edge at a node, represents the arc coming into that node
	T reducedCost(adjEntry adj) const {
		return m_reducedCost[adj];
	}

	//! Returns the lower bound
	T get() const {
		return m_lower;
	}
};
}

/**
 * Implementation of a dual-ascent-based lower bound heuristic for Steiner tree problems.
 *
 * This implementation is based on the paper
 * Tobias Polzin, Siavash Vahdati Daneshmand:
 * Improved algorithms for the Steiner problem in networks.
 * Discrete Applied Mathematics 112(1-3): 263-300 (2001)
 */
template<typename T>
class SteinerTreeLowerBoundDualAscent {
	int m_repetitions = 1;

	T compute(const EdgeWeightedGraph<T>& graph, const List<node>& terminals, node root) {
		steiner_tree::LowerBoundDualAscent<T> alg(graph, terminals, root);
		alg.compute();
		return alg.get();
	}

	void compute(const EdgeWeightedGraph<T>& graph, const List<node>& terminals, node root, NodeArray<T>& lbNodes, EdgeArray<T>& lbEdges) {
		OGDF_ASSERT(isConnected(graph));

		// compute first
		steiner_tree::LowerBoundDualAscent<T> alg(graph, terminals, root);
		alg.compute();

		// generate the auxiliary network
		Graph network;
		NodeArray<node> copy(graph);
		EdgeArray<T> weights(network);
		EdgeArray<edge> orig(network);

		for (node v : graph.nodes) {
			copy[v] = network.newNode();
		}
		for (edge e : graph.edges) {
			edge uv = network.newEdge(copy[e->source()], copy[e->target()]);
			edge vu = network.newEdge(copy[e->target()], copy[e->source()]);
			weights[uv] = alg.reducedCost(e->adjTarget());
			weights[vu] = alg.reducedCost(e->adjSource());
			orig[uv] = orig[vu] = e;
		}

		// compute shortest path tree on network starting from root
		Dijkstra<T> sssp;
		NodeArray<edge> pred;
		NodeArray<T> distance;
		sssp.call(network, weights, copy[root], pred, distance, true);

		// set all lower bounds to global lower bound
		lbNodes.init(graph, alg.get());
		lbEdges.init(graph, alg.get());
		EdgeArray<T> lbArcs(network, alg.get());

		// add cost of path root -> v
		for (node v : graph.nodes) {
			lbNodes[v] += distance[copy[v]];
		}
		// add cost of path root -> e
		for (edge a : network.edges) {
			lbArcs[a] += distance[a->source()] + weights[a];
		}

		// to find the (reduced) distance from v / e to any (non-root) terminal,
		// hence compute (directed) Voronoi regions
		network.reverseAllEdges();

		List<node> nonRootTerminals;
		for (node t : terminals) {
			if (t != root) {
				nonRootTerminals.pushBack(copy[t]);
			}
		}
		sssp.call(network, weights, nonRootTerminals, pred, distance, true);

		// add cost of path v -> any terminal
		for (node v : graph.nodes) {
			lbNodes[v] += distance[copy[v]];
		}
		// add cost of path e -> any terminal
		for (edge a : network.edges) {
			lbArcs[a] += distance[a->source()]; // the former target is now the source

			// both lower bounds must be larger than the upper bound, so take the minimum
			Math::updateMin(lbEdges[orig[a]], lbArcs[a]);
		}
	}

public:
	//! Sets the number of repeated calls to the lower bound algorithm (runs with different roots)
	void setRepetitions(int num) {
		m_repetitions = num;
	}

	//! Calls the algorithm and returns the lower bound
	T call(const EdgeWeightedGraph<T>& graph, const List<node>& terminals) {
		T lb(0);
		int i = 0;
		for (node root : terminals) {
			Math::updateMax(lb, compute(graph, terminals, root));
			if (++i >= m_repetitions) {
				break;
			}
		}
		return lb;
	}

	/**
	 * Compute the lower bounds under the assumption nodes or edges are included in the solution.
	 * The parameter \p lbNodes and \p lbEdges are filled with the lower bound for each node and edge, respectively.
	 */
	void call(const EdgeWeightedGraph<T>& graph, const List<node>& terminals, NodeArray<T>& lbNodes, EdgeArray<T>& lbEdges) {
		if (m_repetitions <= 1) {
			// catch this special case to avoid copying
			compute(graph, terminals, terminals.front(), lbNodes, lbEdges);
		} else {
			lbNodes.init(graph, 0);
			lbEdges.init(graph, 0);
			int i = 0;
			for (node root : terminals) {
				NodeArray<T> nodes;
				EdgeArray<T> edges;
				compute(graph, terminals, root, nodes, edges);
				for (node v : graph.nodes) {
					Math::updateMax(lbNodes[v], nodes[v]);
				}
				for (edge e : graph.edges) {
					Math::updateMax(lbEdges[e], edges[e]);
				}
				if (++i >= m_repetitions) {
					break;
				}
			}
		}
	}
};
}
