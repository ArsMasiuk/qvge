/** \file
 * \brief Implementation of the 1.55-approximation algorithm for the Minimum
 * 		  Steiner Tree problem by Robins and Zelikovsky
 *
 * \author Matthias Woste, Stephan Beyer
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
#include <set>
#include <ogdf/graphalg/steiner_tree/FullComponentStore.h>
#include <ogdf/graphalg/steiner_tree/FullComponentGeneratorDreyfusWagner.h>
#include <ogdf/graphalg/steiner_tree/FullComponentGeneratorDreyfusWagnerWithoutMatrix.h>
#include <ogdf/graphalg/steiner_tree/Full3ComponentGeneratorVoronoi.h>
#include <ogdf/graphalg/steiner_tree/FullComponentGeneratorCaller.h>
#include <ogdf/graphalg/steiner_tree/SaveStatic.h>
#include <ogdf/graphalg/MinSteinerTreeMehlhorn.h>

#define OGDF_STEINERTREE_RZLOSS_REDUCE_ON

namespace ogdf {

/*!
 * \brief This class implements the loss-contracting (1.55+epsilon)-approximation algorithm
 * for the Steiner tree problem by Robins and Zelikovsky.
 *
 * @ingroup ga-steiner
 *
 * This implementation is based on:
 *
 * (G. Robins, A. Zelikovsky, Improved Steiner Tree Approximation in Graphs,
 * SODA 2000, pages 770-779, SIAM, 2000)
 */
template<typename T>
class MinSteinerTreeRZLoss : public MinSteinerTreeModule<T> {
	int m_restricted;

	class Main;
	std::unique_ptr<Main> m_alg;

public:
	MinSteinerTreeRZLoss()
	{
		setMaxComponentSize(3);
	}

	MinSteinerTreeRZLoss(int v)
	{
		setMaxComponentSize(v);
	}

	virtual ~MinSteinerTreeRZLoss() { }

	/*!
	 * \brief Sets the maximal number of terminals in a full component
	 * @param k the maximal number of terminals in a full component
	 */
	void setMaxComponentSize(int k)
	{
		m_restricted = k;
	}

	//! Returns the number of generated components
	long numberOfGeneratedComponents() {
		if (m_alg == nullptr) {
			return 0;
		}
		return m_alg->numberOfGeneratedComponents();
	}

	//! Returns the number of contracted components
	long numberOfContractedComponents() {
		if (m_alg == nullptr) {
			return 0;
		}
		return m_alg->numberOfContractedComponents();
	}

	//! Returns the number of components lookups during execution time
	long numberOfComponentLookUps() {
		if (m_alg == nullptr) {
			return 0;
		}
		return m_alg->numberOfComponentLookUps();
	}

protected:
	/*!
	 * \brief Builds a minimum Steiner tree given a weighted graph and a list of terminals \see MinSteinerTreeModule::call
	 * @param G The weighted input graph
	 * @param terminals The list of terminal nodes
	 * @param isTerminal A bool array of terminals
	 * @param finalSteinerTree The final Steiner tree
	 * @return The objective value (sum of edge costs) of the final Steiner tree
	 */
	virtual T computeSteinerTree(
		const EdgeWeightedGraph<T> &G,
		const List<node> &terminals,
		const NodeArray<bool> &isTerminal,
		EdgeWeightedGraphCopy<T> *&finalSteinerTree) override
	{
		m_alg.reset(new Main(G, terminals, isTerminal, m_restricted));
		return m_alg->getApproximation(finalSteinerTree);
	}
};

template<typename T>
class MinSteinerTreeRZLoss<T>::Main {
	template<typename TYPE> using SaveStatic = steiner_tree::SaveStatic<TYPE>;

	//! \name Finding full components
	//! @{

	/**
	 * Find full components using algorithm by Dreyfus-Wagner
	 * @param tree Current minimal terminal spanning tree
	 * @param distance The distance matrix
	 * @param pred The predecessor matrix for shortest paths
	 */
	void findFullComponentsDW(const EdgeWeightedGraphCopy<T> &tree, const NodeArray<NodeArray<T>>& distance, const NodeArray<NodeArray<edge>>& pred);
	//! Find full components using algorithm by Erickson et al
	void findFullComponentsEMV(const EdgeWeightedGraphCopy<T> &tree);
	//! Find full components of size 3
	void findFull3Components(const EdgeWeightedGraphCopy<T> &tree, const NodeArray<NodeArray<T>>& distance, const NodeArray<NodeArray<edge>>& pred);
	//! Auxiliary function to retrieve components by Dreyfus-Wagner/Erickson et al implementation
	template<typename FCG>
	void retrieveComponents(const FCG& fcg, const EdgeWeightedGraphCopy<T> &tree);

	//! @}

	/*!
	 * \brief Builds a minimum terminal spanning tree (via a MST call on the complete terminal graph)
	 * @param steinerTree The resulting minimum terminal spanning tree
	 * @param distance The distance matrix
	 * @param pred The predecessor matrix for shortest paths
	 */
	void generateInitialTerminalSpanningTree(EdgeWeightedGraphCopy<T> &steinerTree, const NodeArray<NodeArray<T>>& distance, const NodeArray<NodeArray<edge>>& pred);

	//! Setup (build initial terminal-spanning tree, its save data structure, and find all components)
	void setup(EdgeWeightedGraphCopy<T> &tree) {
		tree.createEmpty(m_G);

		if (m_restricted >= 4 &&
		    steiner_tree::FullComponentDecisions::shouldUseErickson(m_G.numberOfNodes(), m_G.numberOfEdges())) {
			steiner_tree::constructTerminalSpanningTreeUsingVoronoiRegions(tree, m_G, m_terminals);
			m_save.reset(new steiner_tree::SaveStatic<T>(tree));
			findFullComponentsEMV(tree);
		} else {
			NodeArray<NodeArray<T>> distance;
			NodeArray<NodeArray<edge>> pred;

			steiner_tree::FullComponentGeneratorCaller<T>::computeDistanceMatrix(distance, pred, m_G, m_terminals, m_isTerminal, m_restricted);

			generateInitialTerminalSpanningTree(tree, distance, pred);
			m_save.reset(new steiner_tree::SaveStatic<T>(tree));

			if (m_restricted >= 4) { // use Dreyfus-Wagner based full component generation
				findFullComponentsDW(tree, distance, pred);
			} else {
				findFull3Components(tree, distance, pred);
			}
		}

		m_fullCompStore.computeAllLosses();
		m_componentsGenerated = m_fullCompStore.size();
	}

	/*!
	 * \brief Contraction phase of the algorithm
	 * @param steinerTree the terminal-spanning tree to be modified
	 */
	void multiPass(EdgeWeightedGraphCopy<T> &steinerTree);

	/*!
	 * \brief Traverses over all full components and finds the one with the highest win-objective.
	 * @param steinerTree Current Steiner tree
	 * @param maxCompId The index of the full component with highest win-objective
	 * @return the win-objective of the returned full component
	 */
	double extractMaxComponent(const EdgeWeightedGraphCopy<T> &steinerTree, int &maxCompId);

	/*!
	 * \brief Calculates the gain for a full component
+	 * @param terminals The terminals of the full component
	 * @param steinerTree Current Steiner tree
	 * @return the gain (cost of save edges) of the returned full component
	 */
	template<typename TERMINAL_CONTAINER>
	T gain(const TERMINAL_CONTAINER &terminals, const EdgeWeightedGraphCopy<T> &steinerTree);

	/*!
	 * \brief Contracts the loss of a full component and integrates it into the given terminal-spanning tree
	 * @param steinerTree The Steiner tree into which the full component is integrated
	 * @param compId The full component to be contracted and inserted
	 */
	void contractLoss(EdgeWeightedGraphCopy<T> &steinerTree, int compId);

	const EdgeWeightedGraph<T>& m_G; //!< The original edge-weighted graph
	const NodeArray<bool>& m_isTerminal; //!< Incidence vector for terminal nodes
	List<node> m_terminals; //!< List of terminal nodes (will be copied and sorted)
	int m_restricted; //!< Parameter for the number of terminals in a full component
	std::unique_ptr<steiner_tree::SaveStatic<T>> m_save; //!< The save data structure
	steiner_tree::FullComponentWithLossStore<T> m_fullCompStore; //!< All generated full components
	NodeArray<bool> m_isNewTerminal; //!< Incidence vector for nonterminal nodes marked as terminals for improvement

	long m_componentsGenerated; //!< Number of generated components
	long m_componentsContracted; //!< Number of contracted components
	long m_componentsLookUps; //!< Number of components lookups

public:
	Main(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, int restricted);

	T getApproximation(EdgeWeightedGraphCopy<T> *&finalSteinerTree) const {
		// obtain final Steiner Tree using (MST-based) Steiner tree approximation algorithm
		return steiner_tree::obtainFinalSteinerTree(m_G, m_isNewTerminal, m_isTerminal, finalSteinerTree);
	}

	//! Returns the number of generated components
	long numberOfGeneratedComponents() {
		return m_componentsGenerated;
	}

	//! Returns the number of contracted components
	long numberOfContractedComponents() {
		return m_componentsContracted;
	}

	//! Returns the number of components lookups during execution time
	long numberOfComponentLookUps() {
		return m_componentsLookUps;
	}
};

template<typename T>
MinSteinerTreeRZLoss<T>::Main::Main(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, int restricted) :
		m_G(G),
		m_isTerminal(isTerminal),
		m_terminals(terminals), // copy
		m_restricted(min(restricted, terminals.size())),
		m_fullCompStore(m_G, m_terminals, m_isTerminal),
		m_isNewTerminal(m_G, false),
		m_componentsGenerated(0),
		m_componentsContracted(0),
		m_componentsLookUps(0) {
	MinSteinerTreeModule<T>::sortTerminals(m_terminals);

	for (node v : terminals) {
		m_isNewTerminal[v] = true;
	}

	// init terminal-spanning tree and its save-edge data structure
	EdgeWeightedGraphCopy<T> steinerTree; // the terminal-spanning tree to be modified

	setup(steinerTree);

	// contraction phase
	multiPass(steinerTree);

	m_save.reset();
}

template<typename T>
void MinSteinerTreeRZLoss<T>::Main::generateInitialTerminalSpanningTree(EdgeWeightedGraphCopy<T> &steinerTree,
		const NodeArray<NodeArray<T>>& distance, const NodeArray<NodeArray<edge>>& pred) {
	// generate complete graph
	for (node v : m_terminals) {
		steinerTree.newNode(v);
	}
	for (node u : steinerTree.nodes) {
		const node uO = steinerTree.original(u);
		const NodeArray<T> &dist = distance[uO];
		const NodeArray<edge> &p = pred[uO];
		for (node v = u->succ(); v; v = v->succ()) {
			const node vO = steinerTree.original(v);
			if (p[vO] != nullptr) {
				steinerTree.newEdge(u, v, dist[vO]);
			}
		}
	}
	// compute MST
	makeMinimumSpanningTree(steinerTree, steinerTree.edgeWeights());
	OGDF_ASSERT(steinerTree.numberOfNodes() == steinerTree.numberOfEdges() + 1);
}

template<typename T>
void MinSteinerTreeRZLoss<T>::Main::findFull3Components(const EdgeWeightedGraphCopy<T> &tree,
		const NodeArray<NodeArray<T>>& distance, const NodeArray<NodeArray<edge>>& pred) {
	steiner_tree::Full3ComponentGeneratorVoronoi<T> fcg;
	fcg.call(m_G, m_terminals, m_isTerminal, distance, pred,
	  [&](node t0, node t1, node t2, node minCenter, T minCost) {
		// create a full 3-component
		EdgeWeightedGraphCopy<T> minComp;
		minComp.createEmpty(m_G);
		node minCenterC = minComp.newNode(minCenter);
		minComp.newEdge(minComp.newNode(t0), minCenterC, distance[t0][minCenter]);
		minComp.newEdge(minComp.newNode(t1), minCenterC, distance[t1][minCenter]);
		minComp.newEdge(minComp.newNode(t2), minCenterC, distance[t2][minCenter]);
		OGDF_ASSERT(isTree(minComp));
#ifdef OGDF_STEINERTREE_RZLOSS_REDUCE_ON
		if (gain(std::vector<node>{t0, t1, t2}, tree) > minCost) { // reduction
			m_fullCompStore.insert(minComp);
		}
#else
		m_fullCompStore.insert(minComp);
#endif
	});
}

template<typename T>
template<typename FCG>
void MinSteinerTreeRZLoss<T>::Main::retrieveComponents(const FCG& fcg, const EdgeWeightedGraphCopy<T> &tree) {
	SubsetEnumerator<node> terminalSubset(m_terminals);
	for (terminalSubset.begin(3, m_restricted); terminalSubset.valid(); terminalSubset.next()) {
		EdgeWeightedGraphCopy<T> component;
		List<node> terminals;
		terminalSubset.list(terminals);
#ifdef OGDF_STEINERTREE_RZLOSS_REDUCE_ON
		T cost =
#endif
		    fcg.getSteinerTreeFor(terminals, component);
		if (
#ifdef OGDF_STEINERTREE_RZLOSS_REDUCE_ON
		    gain(terminals, tree) > cost &&
#endif
		    fcg.isValidComponent(component)) {
			m_fullCompStore.insert(component);
		}
	}
}

template<typename T>
void MinSteinerTreeRZLoss<T>::Main::findFullComponentsDW(const EdgeWeightedGraphCopy<T> &tree,
		const NodeArray<NodeArray<T>>& distance, const NodeArray<NodeArray<edge>>& pred) {
	steiner_tree::FullComponentGeneratorDreyfusWagner<T> fcg(m_G, m_terminals, m_isTerminal, distance, pred);
	fcg.call(m_restricted);
	retrieveComponents(fcg, tree);
}

template<typename T>
void MinSteinerTreeRZLoss<T>::Main::findFullComponentsEMV(const EdgeWeightedGraphCopy<T> &tree) {
	steiner_tree::FullComponentGeneratorDreyfusWagnerWithoutMatrix<T> fcg(m_G, m_terminals, m_isTerminal);
	fcg.call(m_restricted);
	retrieveComponents(fcg, tree);
}

template<typename T>
void MinSteinerTreeRZLoss<T>::Main::multiPass(EdgeWeightedGraphCopy<T> &steinerTree) {
	while (!m_fullCompStore.isEmpty()) {
		int maxCompId;
		double r = extractMaxComponent(steinerTree, maxCompId);
		if (r > 1e-9) {
			++m_componentsContracted;

			// convert nodes of component to terminals
			m_fullCompStore.foreachNode(maxCompId, [&](node v) {
				m_isNewTerminal[v] = true;
			});

			contractLoss(steinerTree, maxCompId);
			m_fullCompStore.remove(maxCompId);

			if (!m_fullCompStore.isEmpty()) {
				m_save->rebuild();
			}
		} else {
			return;
		}
	}
}

template<typename T>
double MinSteinerTreeRZLoss<T>::Main::extractMaxComponent(
		const EdgeWeightedGraphCopy<T> &steinerTree,
		int &maxCompId) {
	maxCompId = -1;
	double max(0);
	for (int i = 0; i < m_fullCompStore.size();) {
		++m_componentsLookUps;
		const double winAbs = gain(m_fullCompStore.terminals(i), steinerTree) - m_fullCompStore.cost(i);
		if (winAbs > 1e-9) {
			const double r = winAbs / m_fullCompStore.loss(i);
			if (r > max) {
				max = r;
				maxCompId = i;
			}
			++i;
		}
#ifdef OGDF_STEINERTREE_RZLOSS_REDUCE_ON
		else {
			// reduction
			m_fullCompStore.remove(i);
		}
#endif
	}
	return max;
}

template<typename T>
template<typename TERMINAL_CONTAINER>
T MinSteinerTreeRZLoss<T>::Main::gain(const TERMINAL_CONTAINER &terminals, const EdgeWeightedGraphCopy<T> &steinerTree) {
	std::set<edge> saveEdges;
	T result(0);

	// extract edges and compute their sum (result value)
	for (auto it1 = terminals.begin(); it1 != terminals.end(); ++it1) {
		auto next = it1;
		++next;
		for (auto it2 = next; it2 != terminals.end(); ++it2) {
			const edge e = m_save->saveEdge(*it1, *it2);
			saveEdges.insert(e);
		}
	}

	for (edge e : saveEdges) {
		result += steinerTree.weight(e);
	}
	return result;
}

template<typename T>
void MinSteinerTreeRZLoss<T>::Main::contractLoss(EdgeWeightedGraphCopy<T> &steinerTree, int compId)
{
	// for every non-loss edge {st} in the component,
	// where s belongs to the loss component of terminal u
	//   and t belongs to the loss component of terminal v,
	// we insert edges from u to v in the terminal-spanning tree
	for (edge e : m_fullCompStore.lossBridges(compId)) {
		const node u = m_fullCompStore.lossTerminal(e->source());
		OGDF_ASSERT(u);
		const node v = m_fullCompStore.lossTerminal(e->target());
		OGDF_ASSERT(v);
		steinerTree.newEdge(steinerTree.copy(u), steinerTree.copy(v), m_fullCompStore.graph().weight(e));
		// parallel edges are ok, will be removed by MST
	}
	if (steinerTree.numberOfNodes() != steinerTree.numberOfEdges() + 1) {
		makeMinimumSpanningTree(steinerTree, steinerTree.edgeWeights());
	}
}

}
