/** \file
 * \brief Implementation of an LP-based 1.39+epsilon Steiner tree
 * approximation algorithm by Goemans et al.
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

#include <ogdf/graphalg/steiner_tree/FullComponentGeneratorDreyfusWagner.h>
#include <ogdf/graphalg/steiner_tree/Full2ComponentGenerator.h>
#include <ogdf/graphalg/steiner_tree/Full3ComponentGeneratorVoronoi.h>
#include <ogdf/graphalg/steiner_tree/common_algorithms.h>
#include <ogdf/graphalg/steiner_tree/LPRelaxationSER.h>
#include <ogdf/graphalg/steiner_tree/goemans/Approximation.h>

namespace ogdf {

/*!
 * \brief This class implements the (1.39+epsilon)-approximation algorithm
 * for the Steiner tree problem by Goemans et. al.
 *
 * @ingroup ga-steiner
 *
 * This implementation is based on:
 *
 * M.X. Goemans, N. Olver, T. Rothvoß, R. Zenklusen:
 * Matroids and Integrality Gaps for Hypergraphic Steiner Tree Relaxations.
 * STOC 2012, pages 1161-1176, 2012
 *
 * and
 *
 * S. Beyer, M. Chimani: Steiner Tree 1.39-Approximation in Practice.
 * MEMICS 2014, LNCS 8934, 60-72, Springer, 2014
 */
template<typename T>
class MinSteinerTreeGoemans139 : public MinSteinerTreeModule<T>
{
private:
	class Main;

protected:
	int m_restricted;
	bool m_use2approx;
	bool m_forceAPSP;
	bool m_separateCycles;
	int m_seed;

public:
	MinSteinerTreeGoemans139()
	  : m_restricted(3)
	  , m_use2approx(false)
	  , m_forceAPSP(false)
	  , m_separateCycles(false)
	  , m_seed(1337)
	{
	}

	virtual ~MinSteinerTreeGoemans139() { }

	/*!
	 * \brief Sets the maximal number of terminals in a full component
	 * @param k the maximal number of terminals in a full component
	 */
	void setMaxComponentSize(int k)
	{
		m_restricted = k;
	}

	/*!
	 * \brief Set seed for the random number generation.
	 * @param seed The seed
	 */
	void setSeed(int seed)
	{
		m_seed = seed;
	}

	/*!
	 * \brief Use Takahashi-Matsuyama 2-approximation as upper bounds
	 * \note not recommended to use in general
	 * @param use2approx True to apply the bound
	 */
	void use2Approximation(bool use2approx = true)
	{
		m_use2approx = use2approx;
	}

	/*! \brief Force full APSP algorithm even if consecutive SSSP algorithms may work
	 *
	 * For the 3-restricted case, it is sufficient to compute an SSSP from every terminal
	 *  instead of doing a full APSP. In case a full APSP is faster, use this method.
	 * @param force True to force APSP instead of SSSP.
	 */
	void forceAPSP(bool force = true)
	{
		m_forceAPSP = force;
	}

	/*!
	 * \brief Use stronger LP relaxation (not recommended in general)
	 * @param separateCycles True to turn the stronger LP relaxation on
	 */
	void separateCycles(bool separateCycles = true)
	{
		m_separateCycles = separateCycles;
	}

protected:
	/*!
	 * \brief Builds a minimum Steiner tree for a given weighted graph with terminals \see MinSteinerTreeModule::computeSteinerTree
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
};

template<typename T>
T MinSteinerTreeGoemans139<T>::computeSteinerTree(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, EdgeWeightedGraphCopy<T> *&finalSteinerTree)
{
	std::minstd_rand rng(m_seed);
	List<node> sortedTerminals(terminals);
	MinSteinerTreeModule<T>::sortTerminals(sortedTerminals);
	Main main(G, sortedTerminals, isTerminal, m_restricted, m_use2approx, m_separateCycles, !m_forceAPSP);
	return main.getApproximation(finalSteinerTree, rng, true);
}

//! \brief Class managing LP-based approximation
//! \todo should be refactored, done this way for historical reasons
template<typename T>
class MinSteinerTreeGoemans139<T>::Main
{
	const EdgeWeightedGraph<T> &m_G;
	const NodeArray<bool> &m_isTerminal;
	const List<node> &m_terminals; //!< List of terminals
	steiner_tree::FullComponentWithExtraStore<T, double> m_fullCompStore; //!< all enumerated full components, with solution

	NodeArray<NodeArray<T>> m_distance;
	NodeArray<NodeArray<edge>> m_predAPSP;

	int m_restricted;
	enum class Approx2State {
		Off,
		On,
		JustUseIt,
	};
	Approx2State m_use2approx;
	bool m_ssspDistances;

	const double m_eps; //!< epsilon for double operations

	EdgeWeightedGraphCopy<T> *m_approx2SteinerTree;
	T m_approx2Weight;

	//! \name Finding full components
	//! @{

	//! Compute m_distance and m_predAPSP
	void computeDistanceMatrix();

	//! Find full components of size 2
	void findFull2Components();
	//! Find full components of size 3
	void findFull3Components();
	//! Find full components
	void findFullComponents();

	//! @}
	//! \name Preliminaries and preprocessing for the approximation algorithm
	//! @{

	//! Remove inactive components from m_fullCompStore (since we do not need them any longer)
	void removeInactiveComponents()
	{
		// XXX: is it faster to do this backwards? (less copying)
		int k = 0;
		while (k < m_fullCompStore.size()) {
			if (m_fullCompStore.extra(k) > m_eps) {
				++k;
			} else {
				m_fullCompStore.remove(k);
			}
		}
	}

	//! Remove the full components with the given ids
	void removeComponents(ArrayBuffer<int> &ids)
	{
		ids.quicksort();
		for (int i = ids.size() - 1; i >= 0; --i) {
			m_fullCompStore.remove(ids[i]);
		}
	}

	//! Add a full component to the final solution (by changing nonterminals to terminals)
	void addComponent(NodeArray<bool> &isNewTerminal, int id)
	{
		m_fullCompStore.foreachNode(id, m_predAPSP, [&](node v) {
			isNewTerminal[v] = true;
		});
	}

	//! \brief Preprocess LP solution
	//! \pre every terminal is covered with >= 1
	void preprocess(NodeArray<bool> &isNewTerminal)
	{
		Graph H; // a graph where each component is a star
		NodeArray<int> id(H); // ids each center of the star to the component id
		NodeArray<node> copy(m_G, nullptr); // ids orig in m_G -> copy in H

		List<node> centers; // all centers
		for (int i = 0; i < m_fullCompStore.size(); ++i) {
			const node center = H.newNode();
			centers.pushBack(center);
			id[center] = i;

			for (node vG : m_fullCompStore.terminals(i)) {
				node vH = copy[vG];
				if (!vH) {
					vH = H.newNode();
					copy[vG] = vH;
				}
				H.newEdge(vH, center); // target is always center
			}
		}

		// find components to be inserted into the steinerTree and insert them
		ArrayBuffer<int> inactive; // ids of components we insert; we have to remove them from the set of active components afterwards
		bool changed;
		do {
			changed = false;
			ListIterator<node> it2;
			for (ListIterator<node> it = centers.begin(); it.valid(); it = it2) {
				it2 = it.succ();
				node c = *it;
				int innerNodes = 0; // count inner nodes
				for (adjEntry adj : c->adjEntries) {
					innerNodes += (adj->twinNode()->degree() != 1);
				}
				if (innerNodes <= 1) { // this center represents a component to add to steinerTree
					// insert component into steinerTree
					addComponent(isNewTerminal, id[c]);

					// remove center from H (adjacent leaves can remain being isolated nodes)
					inactive.push(id[c]);
					H.delNode(c);
					centers.del(it);

					changed = true;
				}
			}
		} while (changed);

		removeComponents(inactive);
	}

	//! @}

public:
	//! Initialize all attributes, sort the terminal list
	Main(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal,
	     int restricted, bool use2approx, bool separateCycles, bool useSSSPfor3Restricted, double eps = 1e-8)
	  : m_G(G)
	  , m_isTerminal(isTerminal)
	  , m_terminals(terminals)
	  , m_fullCompStore(G, m_terminals, isTerminal)
	  , m_distance()
	  , m_predAPSP()
	  , m_restricted(restricted)
	  , m_use2approx(use2approx ? Approx2State::On : Approx2State::Off)
	  , m_ssspDistances(useSSSPfor3Restricted)
	  , m_eps(eps)
	  , m_approx2SteinerTree(nullptr)
	  , m_approx2Weight(0)
	{
		if (m_use2approx != Approx2State::Off) { // add upper bound by 2-approximation
			MinSteinerTreeTakahashi<T> mstT;
			m_approx2Weight = mstT.call(m_G, m_terminals, m_isTerminal, m_approx2SteinerTree, m_G.firstNode());
		}

		if (m_restricted > m_terminals.size()) {
			m_restricted = m_terminals.size();
		}

		computeDistanceMatrix();
		findFullComponents();

		steiner_tree::LPRelaxationSER<T> lp(m_G, m_terminals, m_isTerminal, m_fullCompStore, m_approx2Weight, m_restricted + 1, m_eps);
		if (!lp.solve()) {
			OGDF_ASSERT(m_use2approx == Approx2State::On);
			m_use2approx = Approx2State::JustUseIt;
		}
	}

	~Main()
	{
	}

	//! Obtain an (1.39+epsilon)-approximation based on the LP solution
	T getApproximation(EdgeWeightedGraphCopy<T> *&finalSteinerTree, const std::minstd_rand &rng, const bool doPreprocessing = true);
};

template<typename T>
void MinSteinerTreeGoemans139<T>::Main::findFull2Components()
{
	steiner_tree::Full2ComponentGenerator<T> fcg;
	fcg.call(m_G, m_terminals, m_distance, m_predAPSP,
	  [&](node s, node t, T cost) {
		EdgeWeightedGraphCopy<T> minComp;
		minComp.createEmpty(m_G);
		minComp.newEdge(minComp.newNode(s), minComp.newNode(t), m_distance[s][t]);
		m_fullCompStore.insert(minComp);
	});
}

template<typename T>
void MinSteinerTreeGoemans139<T>::Main::findFull3Components()
{
	steiner_tree::Full3ComponentGeneratorVoronoi<T> fcg;
	fcg.call(m_G, m_terminals, m_isTerminal, m_distance, m_predAPSP,
	  [&](node t0, node t1, node t2, node minCenter, T minCost) {
		// create a full 3-component
		EdgeWeightedGraphCopy<T> minComp;
		minComp.createEmpty(m_G);
		node minCenterC = minComp.newNode(minCenter);
		minComp.newEdge(minComp.newNode(t0), minCenterC, m_distance[t0][minCenter]);
		minComp.newEdge(minComp.newNode(t1), minCenterC, m_distance[t1][minCenter]);
		minComp.newEdge(minComp.newNode(t2), minCenterC, m_distance[t2][minCenter]);
		m_fullCompStore.insert(minComp);
	});
}

template<typename T>
void MinSteinerTreeGoemans139<T>::Main::computeDistanceMatrix()
{
	if (m_ssspDistances
	 && m_restricted <= 3) {
		// for 2- and 3-restricted computations, it is ok to use SSSP from all terminals
#ifndef OGDF_MINSTEINERTREEGOEMANS139_DETOUR
		MinSteinerTreeModule<T>::allTerminalShortestPathsStrict
#else
		MinSteinerTreeModule<T>::allTerminalShortestPathsDetour
#endif
		  (m_G, m_terminals, m_isTerminal, m_distance, m_predAPSP);
	} else {
		m_ssspDistances = false;
#ifndef OGDF_MINSTEINERTREEGOEMANS139_DETOUR
		MinSteinerTreeModule<T>::allPairShortestPathsStrict
#else
		MinSteinerTreeModule<T>::allPairShortestPathsDetour
#endif
		  (m_G, m_isTerminal, m_distance, m_predAPSP);
	}
}

template<typename T>
void MinSteinerTreeGoemans139<T>::Main::findFullComponents()
{
	if (m_restricted >= 4) { // use Dreyfus-Wagner based full component generation
		SubsetEnumerator<node> terminalSubset(m_terminals);
		steiner_tree::FullComponentGeneratorDreyfusWagner<T> fcg(m_G, m_terminals, m_distance);
		fcg.call(m_restricted);
		for (terminalSubset.begin(2, m_restricted); terminalSubset.valid(); terminalSubset.next()) {
			EdgeWeightedGraphCopy<T> component;
			List<node> terminals;
			terminalSubset.list(terminals);
			fcg.getSteinerTreeFor(terminals, component);
			if (steiner_tree::FullComponentGeneratorDreyfusWagner<T>::isValidComponent(component, m_predAPSP, m_isTerminal)) {
				m_fullCompStore.insert(component);
			}
		}
	} else {
		findFull2Components();
		if (m_restricted == 3) {
			findFull3Components();
		}
	}
}

template<typename T>
T
MinSteinerTreeGoemans139<T>::Main::getApproximation(EdgeWeightedGraphCopy<T> *&finalSteinerTree, const std::minstd_rand &rng, const bool doPreprocessing)
{
	if (m_use2approx == Approx2State::JustUseIt) {
		// no remaining components
		finalSteinerTree = m_approx2SteinerTree;
		return m_approx2Weight;
	}

	removeInactiveComponents();

	NodeArray<bool> isNewTerminal(m_G, false);
	for (node v : m_terminals) {
		isNewTerminal[v] = true;
	}

	if (doPreprocessing) {
		preprocess(isNewTerminal);
	}

	if (!m_fullCompStore.isEmpty()) {
		steiner_tree::goemans::Approximation<T> approx(m_G, m_terminals, m_isTerminal, m_fullCompStore, rng, m_eps);
		approx.solve(isNewTerminal);
	}

	T cost = steiner_tree::obtainFinalSteinerTree(m_G, isNewTerminal, m_isTerminal, finalSteinerTree);
	if (m_use2approx != Approx2State::Off) {
		if (m_approx2Weight < cost) {
			delete finalSteinerTree;
			finalSteinerTree = m_approx2SteinerTree;
		} else {
			delete m_approx2SteinerTree;
		}
	}

	return cost;
}

}
