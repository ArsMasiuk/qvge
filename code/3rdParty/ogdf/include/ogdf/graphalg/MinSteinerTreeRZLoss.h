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

#include <set>
#include <ogdf/basic/Queue.h>
#include <ogdf/basic/SubsetEnumerator.h>
#include <ogdf/graphalg/steiner_tree/FullComponentStore.h>
#include <ogdf/graphalg/steiner_tree/FullComponentGeneratorDreyfusWagner.h>
#include <ogdf/graphalg/steiner_tree/Full3ComponentGeneratorVoronoi.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>
#include <ogdf/graphalg/steiner_tree/common_algorithms.h>
#include <ogdf/graphalg/steiner_tree/SaveStatic.h>
#include <ogdf/module/MinSteinerTreeModule.h>

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
public:
	template<typename TYPE> using SaveStatic = steiner_tree::SaveStatic<TYPE>;

	MinSteinerTreeRZLoss()
	 : m_ssspDistances(true)
	{
		setMaxComponentSize(3);
	}

	MinSteinerTreeRZLoss(int v)
	 : m_ssspDistances(true)
	{
		setMaxComponentSize(v);
	}

	virtual ~MinSteinerTreeRZLoss() { }

	virtual T call(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, EdgeWeightedGraphCopy<T> *&finalSteinerTree) override
	{
		m_componentsLookUps = 0;
		m_componentsContracted = 0;
		m_componentsGenerated = 0;
		return MinSteinerTreeModule<T>::call(G, terminals, isTerminal, finalSteinerTree);
	}

	/*!
	 * \brief Sets the maximal number of terminals in a full component
	 * @param k the maximal number of terminals in a full component
	 */
	void setMaxComponentSize(int k)
	{
		m_restricted = k;
	}

	/*!
	 * \brief For the 3-restricted case, it is sufficient to compute an SSSP from every terminal
	 *  instead of doing a full APSP. In case a full APSP is faster, use this method.
	 * @param force True to force APSP instead of SSSP.
	 */
	void forceAPSP(bool force = true)
	{
		m_ssspDistances = !force;
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
		EdgeWeightedGraphCopy<T> *&finalSteinerTree) override;

private:
	//! Compute m_distance and m_pred
	void computeDistanceMatrix();

	/*!
	 * \brief Finds all k-restricted full components (except 2-components).
	 * @param tree Current minimal terminal spanning tree
	 * @param save Data structure for calculating save edges
	 */
	void findFullComponents(const EdgeWeightedGraphCopy<T> &tree, const SaveStatic<T> &save);
	void findFull3Components(const EdgeWeightedGraphCopy<T> &tree, const SaveStatic<T> &save);

	/*!
	 * \brief Builds a minimum terminal spanning tree (via a MST call on the complete terminal graph)
	 * @param steinerTree The resulting minimum terminal spanning tree
	 */
	void generateInitialTerminalSpanningTree(EdgeWeightedGraphCopy<T> &steinerTree);

	/*!
	 * \brief Contraction phase of the algorithm
	 * @param save save data structure
	 * @param steinerTree the terminal-spanning tree to be modified
	 * @param isNewTerminal true for nodes to be interpreted as terminals
	 */
	void multiPass(SaveStatic<T> &save, EdgeWeightedGraphCopy<T> &steinerTree, NodeArray<bool> &isNewTerminal);

	/*!
	 * \brief Traverses over all full components and finds the one with the highest win-objective.
	 * @param steinerTree Current Steiner tree
	 * @param maxCompId The index of the full component with highest win-objective
	 * @param save Data structure for calculating save values
	 * @return the win-objective of the returned full component
	 */
	double extractMaxComponent(const EdgeWeightedGraphCopy<T> &steinerTree, int &maxCompId, SaveStatic<T> &save);

	/*!
	 * \brief Calculates the gain for a full component
+	 * @param terminals The terminals of the full component
	 * @param steinerTree Current Steiner tree
	 * @param save Data structure for calculating save values
	 * @return the gain (cost of save edges) of the returned full component
	 */
	template<typename TERMINAL_CONTAINER>
	T gain(const TERMINAL_CONTAINER &terminals, const EdgeWeightedGraphCopy<T> &steinerTree, const SaveStatic<T> &save);

	/*!
	 * \brief Contracts the loss of a full component and integrates it into the given terminal-spanning tree
	 * @param steinerTree The Steiner tree into which the full component is integrated
	 * @param compId The full component to be contracted and inserted
	 */
	void contractLoss(EdgeWeightedGraphCopy<T> &steinerTree, int compId);

	const EdgeWeightedGraph<T> *m_originalGraph; //!< The original edge-weighted graph
	const NodeArray<bool> *m_isTerminal; //!< Incidence vector for terminal nodes
	List<node> m_terminals; //!< List of terminal nodes (will be copied and sorted)
	ArrayBuffer<node> m_nonterminals; //!< Container of Steiner (non-terminal) nodes
	int m_restricted; //!< Parameter for the number of terminals in a full component
	steiner_tree::FullComponentWithLossStore<T> *m_fullCompStore; //!< All generated full components
	bool m_ssspDistances; //!< True iff we only compute SSSP from terminals instead of APSP for full component construction
	NodeArray<NodeArray<T>> m_distance;
	NodeArray<NodeArray<edge>> m_pred;

	long m_componentsGenerated; //!< Number of generated components
	long m_componentsContracted; //!< Number of contracted components
	long m_componentsLookUps; //!< Number of components lookups
};

template<typename T>
T MinSteinerTreeRZLoss<T>::computeSteinerTree(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, EdgeWeightedGraphCopy<T> *&finalSteinerTree)
{
	m_originalGraph = &G;
	Math::updateMin(m_restricted, terminals.size());
	m_terminals = terminals; // copy
	MinSteinerTreeModule<T>::sortTerminals(m_terminals);
	m_isTerminal = &isTerminal;
	m_fullCompStore = new steiner_tree::FullComponentWithLossStore<T>(G, m_terminals, isTerminal);

	MinSteinerTreeModule<T>::getNonterminals(m_nonterminals, G, *m_isTerminal);

	computeDistanceMatrix();

	NodeArray<bool> isNewTerminal(G, false);
	for (node v : terminals) {
		isNewTerminal[v] = true;
	}

	// init terminal-spanning tree and its save-edge data structure
	EdgeWeightedGraphCopy<T> steinerTree; // the terminal-spanning tree to be modified
	steinerTree.createEmpty(*m_originalGraph);
	generateInitialTerminalSpanningTree(steinerTree);

	SaveStatic<T> save(steinerTree);

	// component enumeration phase
	if (m_restricted >= 3) {
		findFullComponents(steinerTree, save);
	}
	m_fullCompStore->computeAllLosses();
	m_componentsGenerated = m_fullCompStore->size();

	// contraction phase
	multiPass(save, steinerTree, isNewTerminal);

	// cleanup
	delete m_fullCompStore;
	m_nonterminals.clear();

	// obtain final Steiner Tree using (MST-based) Steiner tree approximation algorithm
	return steiner_tree::obtainFinalSteinerTree(*m_originalGraph, isNewTerminal, *m_isTerminal, finalSteinerTree);
}

template<typename T>
void MinSteinerTreeRZLoss<T>::generateInitialTerminalSpanningTree(EdgeWeightedGraphCopy<T> &steinerTree)
{
	// generate complete graph
	for (node v : m_terminals) {
		steinerTree.newNode(v);
	}
	for (node u : steinerTree.nodes) {
		const node uO = steinerTree.original(u);
		const NodeArray<T> &dist = m_distance[uO];
		const NodeArray<edge> &pred = m_pred[uO];
		for (node v = u->succ(); v; v = v->succ()) {
			const node vO = steinerTree.original(v);
			if (pred[vO] != nullptr) {
				steinerTree.newEdge(u, v, dist[vO]);
			}
		}
	}
	// compute MST
	makeMinimumSpanningTree(steinerTree, steinerTree.edgeWeights());
	OGDF_ASSERT(steinerTree.numberOfNodes() == steinerTree.numberOfEdges() + 1);
}

template<typename T>
void MinSteinerTreeRZLoss<T>::computeDistanceMatrix()
{
	if (m_ssspDistances
	 && m_restricted <= 3) {
		// for 2- and 3-restricted computations, it is ok to use SSSP from all terminals
		MinSteinerTreeModule<T>::allTerminalShortestPathsStrict(*m_originalGraph, m_terminals, *m_isTerminal, m_distance, m_pred);
	} else {
		m_ssspDistances = false;
		MinSteinerTreeModule<T>::allPairShortestPathsStrict(*m_originalGraph, *m_isTerminal, m_distance, m_pred);
	}
}

template<typename T>
void MinSteinerTreeRZLoss<T>::findFull3Components(const EdgeWeightedGraphCopy<T> &tree, const SaveStatic<T> &save)
{
	steiner_tree::Full3ComponentGeneratorVoronoi<T> fcg;
	fcg.call(*m_originalGraph, m_terminals, *m_isTerminal, m_distance, m_pred,
	  [&](node t0, node t1, node t2, node minCenter, T minCost) {
		// create a full 3-component
		EdgeWeightedGraphCopy<T> minComp;
		minComp.createEmpty(*m_originalGraph);
		node minCenterC = minComp.newNode(minCenter);
		minComp.newEdge(minComp.newNode(t0), minCenterC, m_distance[t0][minCenter]);
		minComp.newEdge(minComp.newNode(t1), minCenterC, m_distance[t1][minCenter]);
		minComp.newEdge(minComp.newNode(t2), minCenterC, m_distance[t2][minCenter]);
		OGDF_ASSERT(isTree(minComp));
#ifdef OGDF_STEINERTREE_RZLOSS_REDUCE_ON
		if (gain(std::vector<node>{t0, t1, t2}, tree, save) > minCost) { // reduction
			m_fullCompStore->insert(minComp);
		}
#else
		m_fullCompStore->insert(minComp);
#endif
	});
}

template<typename T>
void MinSteinerTreeRZLoss<T>::findFullComponents(const EdgeWeightedGraphCopy<T> &tree, const SaveStatic<T> &save)
{
	if (m_restricted >= 4) { // use Dreyfus-Wagner based full component generation
		SubsetEnumerator<node> terminalSubset(m_terminals);
		steiner_tree::FullComponentGeneratorDreyfusWagner<T> fcg(*m_originalGraph, m_terminals, m_distance);
		fcg.call(m_restricted);
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
			    gain(terminals, tree, save) > cost &&
#endif
			    steiner_tree::FullComponentGeneratorDreyfusWagner<T>::isValidComponent(component, m_pred, *m_isTerminal)) {
				m_fullCompStore->insert(component);
			}
		}
	} else {
		findFull3Components(tree, save);
	}
}

template<typename T>
void MinSteinerTreeRZLoss<T>::multiPass(SaveStatic<T> &save, EdgeWeightedGraphCopy<T> &steinerTree, NodeArray<bool> &isNewTerminal)
{
	while (!m_fullCompStore->isEmpty()) {
		int maxCompId;
		double r = extractMaxComponent(steinerTree, maxCompId, save);
		if (r > 1e-9) {
			++m_componentsContracted;

			// convert nodes of component to terminals
			m_fullCompStore->foreachNode(maxCompId, m_pred, [&](node v) {
				isNewTerminal[v] = true;
			});

			contractLoss(steinerTree, maxCompId);
			m_fullCompStore->remove(maxCompId);

			if (!m_fullCompStore->isEmpty()) {
				save.rebuild();
			}
		} else {
			return;
		}
	}
}

template<typename T>
double MinSteinerTreeRZLoss<T>::extractMaxComponent(
		const EdgeWeightedGraphCopy<T> &steinerTree,
		int &maxCompId,
		SaveStatic<T> &save)
{
	maxCompId = -1;
	double max(0);
	for (int i = 0; i < m_fullCompStore->size();) {
		++m_componentsLookUps;
		const double winAbs = gain(m_fullCompStore->terminals(i), steinerTree, save) - m_fullCompStore->cost(i);
		if (winAbs > 1e-9) {
			const double r = winAbs / m_fullCompStore->loss(i);
			if (r > max) {
				max = r;
				maxCompId = i;
			}
			++i;
		}
#ifdef OGDF_STEINERTREE_RZLOSS_REDUCE_ON
		else {
			// reduction
			m_fullCompStore->remove(i);
		}
#endif
	}
	return max;
}


template<typename T>
template<typename TERMINAL_CONTAINER>
T MinSteinerTreeRZLoss<T>::gain(const TERMINAL_CONTAINER &terminals, const EdgeWeightedGraphCopy<T> &steinerTree, const SaveStatic<T> &save)
{
	std::set<edge> saveEdges;
	T result(0);

	// extract edges and compute their sum (result value)
	for (auto it1 = terminals.begin(); it1 != terminals.end(); ++it1) {
		auto next = it1;
		++next;
		for (auto it2 = next; it2 != terminals.end(); ++it2) {
			const edge e = save.saveEdge(*it1, *it2);
			saveEdges.insert(e);
		}
	}

	for (edge e : saveEdges) {
		result += steinerTree.weight(e);
	}
	return result;
}

template<typename T>
void MinSteinerTreeRZLoss<T>::contractLoss(EdgeWeightedGraphCopy<T> &steinerTree, int compId)
{
	// for every non-loss edge {st} in the component,
	// where s belongs to the loss component of terminal u
	//   and t belongs to the loss component of terminal v,
	// we insert edges from u to v in the terminal-spanning tree
	for (edge e : m_fullCompStore->lossBridges(compId)) {
		const node u = m_fullCompStore->lossTerminal(e->source());
		OGDF_ASSERT(u);
		const node v = m_fullCompStore->lossTerminal(e->target());
		OGDF_ASSERT(v);
		steinerTree.newEdge(steinerTree.copy(u), steinerTree.copy(v), m_fullCompStore->graph().weight(e));
		// parallel edges are ok, will be removed by MST
	}
	if (steinerTree.numberOfNodes() != steinerTree.numberOfEdges() + 1) {
		makeMinimumSpanningTree(steinerTree, steinerTree.edgeWeights());
	}
}

}
