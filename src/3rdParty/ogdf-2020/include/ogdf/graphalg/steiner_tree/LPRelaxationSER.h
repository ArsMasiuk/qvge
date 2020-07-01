/** \file
 * \brief Definition of ogdf::steiner_tree::LPRelaxationSER class template
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

#include <ogdf/external/coin.h>
#include <ogdf/basic/SubsetEnumerator.h>
#include <ogdf/graphalg/MinSTCutMaxFlow.h>
#include <ogdf/graphalg/steiner_tree/FullComponentStore.h>

#include <coin/CoinPackedMatrix.hpp>

//#define OGDF_STEINERTREE_LPRELAXATIONSER_LOGGING
//#define OGDF_STEINERTREE_LPRELAXATIONSER_OUTPUT_LP
#define OGDF_STEINERTREE_LPRELAXATIONSER_SEPARATE_CONNECTED_COMPONENTS // this is faster
#define OGDF_STEINERTREE_LPRELAXATIONSER_SEPARATE_YVAR_CONSTRAINTS // if not defined: generate all yvar constraints in the beginning

namespace ogdf {
namespace steiner_tree {

//! Class managing the component-based subtour elimination LP relaxation
//! for the Steiner tree problem and its solving
template<typename T>
class LPRelaxationSER
{
	const EdgeWeightedGraph<T> &m_G;
	const NodeArray<bool> &m_isTerminal;
	const List<node> &m_terminals; //!< List of terminals
	FullComponentWithExtraStore<T, double> &m_fullCompStore; //!< all enumerated full components, with solution

	OsiSolverInterface *m_osiSolver;
	CoinPackedMatrix *m_matrix;
	double *m_objective;
	double *m_lowerBounds;
	double *m_upperBounds;

	const T m_upperBound;
	const int m_separateCliqueSize;
#ifdef OGDF_STEINERTREE_LPRELAXATIONSER_SEPARATE_CONNECTED_COMPONENTS
	int m_separationStage;
#endif

	const double m_eps; //!< epsilon for double operations

	//! Generate the basic LP model
	void generateProblem();
	//! Add terminal cover constraints to the LP
	void addTerminalCoverConstraint();
	//! Add subset cover constraints to the LP for a given subset of terminals
	bool addSubsetCoverConstraint(const ArrayBuffer<node> &subset);
	//! Add constraint that the sum of x_C over all components C spanning terminal \p t is at least 1 to ensure y_t >= 0
	void addYConstraint(const node t);

	//! Separate to ensure that the solution is connected
	//! @return True iff new constraints have been introduced
	bool separateConnected(const ArrayBuffer<int> &activeComponents);

	//! Perform the general cut-based separation algorithm
	//! @return True iff new constraints have been introduced
	bool separateMinCut(const ArrayBuffer<int> &activeComponents);

	//! Perform the separation algorithm for cycle constraints (to obtain stronger LP solution)
	//! @return True iff new constraints have been introduced
	bool separateCycles(const ArrayBuffer<int> &activeComponents);

	//! Perform all available separation algorithms
	//! @return True iff new constraints have been introduced
	bool separate();

	//! Generates an auxiliary multi-graph for separation (during LP solving):
	//!  directed, with special source and target, without Steiner vertices of degree 2
	double generateMinCutSeparationGraph(const ArrayBuffer<int> &activeComponents,
	                                     node &source, node &target,
	                                     GraphCopy &G,
	                                     EdgeArray<double> &capacity,
	                                     int &cutsFound)
	{
		G.createEmpty(m_G);
		capacity.init(G);
		source = G.newNode();
		for (node t : m_terminals) { // generate all terminals
			G.newNode(t);
		}
		target = G.newNode();
		for (int j = 0; j < activeComponents.size(); ++j) {
			const int i = activeComponents[j];
			const double cap = m_fullCompStore.extra(i);
			const Array<node> &terminals = m_fullCompStore.terminals(i);
			// take the first terminal as root
			// XXX: we may generate parallel edges but it's ok
			const auto it0 = terminals.begin();
			const node rC = G.copy(*it0);
			capacity[G.newEdge(source, rC)] = cap;
			if (terminals.size() > 2) {
				const node v = G.newNode();
				capacity[G.newEdge(rC, v)] = cap;
				for (auto it = it0 + 1; it != terminals.end(); ++it) {
					const node w = G.copy(*it);
					capacity[G.newEdge(v, w)] = cap;
				}
			} else { // exactly two terminals: we do not need the inner Steiner node
				capacity[G.newEdge(rC, G.copy(*terminals.rbegin()))] = cap;
			}
		}
		double y_R = 0;
		// TODO: perhaps better to compute y_v before
		// add edges to target and compute y_R
		for (node t : m_terminals) {
			const node v = G.copy(t);
			OGDF_ASSERT(v);
			// compute y_v, simply the sum of all x_C where C contains v and then - 1
			double y_v(-1);
			for (adjEntry adj : v->adjEntries) {
				if (adj->twinNode() != source) {
					y_v += capacity[adj->theEdge()];
				}
			}

#ifdef OGDF_STEINERTREE_LPRELAXATIONSER_SEPARATE_YVAR_CONSTRAINTS
			if (y_v < -m_eps) {
				addYConstraint(t);
				++cutsFound;
			}
			else
#endif
			if (y_v > 0) {
				capacity[G.newEdge(v, target)] = y_v;
				y_R += y_v;
			}
		}
#if 0
		// just for output of blow-up graph
		for (edge e : G.edges) {
			if (G.original(e->source())) {
				std::cout << " T:" << G.original(e->source());
			} else {
				std::cout << " " << e->source();
			}
			std::cout << " -> ";
			if (G.original(e->target())) {
				std::cout << "T:" << G.original(e->target());
			} else {
				std::cout << e->target();
			}
			std::cout << " " << capacity[e] << "\n";
		}
#endif

		return y_R;
	}

public:
	/**
	 * Initialize the LP
	 * @param G edge-weighted input graph
	 * @param terminals terminals of the Steiner instance
	 * @param isTerminal incidence vector of terminals
	 * @param fullCompStore the set of full components variables should be constructed for, augmented with extra for solution value
	 * @param upperBound an upper bound to be applied during the LP solving (or 0 if no upper bound should be applied)
	 * @param cliqueSize the maximal clique size for stronger LP constraints (or 0 if the original LP should be solved)
	 * @param eps epsilon used for comparisons
	 */
	LPRelaxationSER(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal,
	                FullComponentWithExtraStore<T, double> &fullCompStore,
	                T upperBound = 0, int cliqueSize = 0, double eps = 1e-8)
	  : m_G(G)
	  , m_isTerminal(isTerminal)
	  , m_terminals(terminals)
	  , m_fullCompStore(fullCompStore)
	  , m_osiSolver(CoinManager::createCorrectOsiSolverInterface())
	  , m_matrix(new CoinPackedMatrix)
	  , m_objective(new double[m_fullCompStore.size()])
	  , m_lowerBounds(new double[m_fullCompStore.size()])
	  , m_upperBounds(new double[m_fullCompStore.size()])
	  , m_upperBound(upperBound)
	  , m_separateCliqueSize(cliqueSize)
#ifdef OGDF_STEINERTREE_LPRELAXATIONSER_SEPARATE_CONNECTED_COMPONENTS
	  , m_separationStage(0)
#endif
	  , m_eps(eps)
	{
		generateProblem();
		addTerminalCoverConstraint();

#ifndef OGDF_STEINERTREE_LPRELAXATIONSER_SEPARATE_YVAR_CONSTRAINTS
		for (node t : m_terminals) {
			addYConstraint(t);
		}
#endif
	}

	~LPRelaxationSER()
	{
		delete[] m_objective;
		delete m_matrix;
		delete[] m_lowerBounds;
		delete[] m_upperBounds;
		delete m_osiSolver;
	}

	//! Solve the LP. The solution will be written to the extra data of the full component store.
	//! \return true iff it has found a solution (always true if given upper bound is zero)
	bool solve();
};

template<typename T>
void
LPRelaxationSER<T>::generateProblem()
{
	const int n = m_fullCompStore.size();

	m_matrix->setDimensions(0, n);
	for (int i = 0; i < n; ++i) {
		m_lowerBounds[i] = 0;
		m_upperBounds[i] = 1;
	}
	for (int i = 0; i < n; ++i) {
		m_objective[i] = m_fullCompStore.cost(i);
	}
	m_osiSolver->loadProblem(*m_matrix, m_lowerBounds, m_upperBounds, m_objective, nullptr, nullptr);

	if (m_upperBound > 0) { // add upper bound
		CoinPackedVector row;
		row.setFull(m_fullCompStore.size(), m_objective);
		m_osiSolver->addRow(row, 0, m_upperBound);
	}
}

template<typename T>
void
LPRelaxationSER<T>::addYConstraint(const node t)
{
	CoinPackedVector row;

	for (int i = 0; i < m_fullCompStore.size(); ++i) {
		if (m_fullCompStore.isTerminal(i, t)) { // component spans terminal
			row.insert(i, 1);
		}
	}

	m_osiSolver->addRow(row, 1, m_osiSolver->getInfinity());
}

// add constraint if necessary and return if necessary
template<typename T>
bool
LPRelaxationSER<T>::addSubsetCoverConstraint(const ArrayBuffer<node> &subset)
{
	CoinPackedVector row;
	double test = 0;

	for (int i = 0; i < m_fullCompStore.size(); ++i) {
		// compute the intersection cardinality (linear time because terminal sets are sorted by index)
		int intersectionCard = 0;
		auto terminals = m_fullCompStore.terminals(i);
		node *it1 = terminals.begin();
		auto it2 = subset.begin();
		while (it1 != terminals.end()
		    && it2 != subset.end()) {
			if ((*it1)->index() < (*it2)->index()) {
				++it1;
			} else
			if ((*it1)->index() > (*it2)->index()) {
				++it2;
			} else { // ==
				++intersectionCard;
				++it1;
				++it2;
			}
		}
		// and use it as a coefficient
		if (intersectionCard > 1) {
			row.insert(i, intersectionCard - 1);
			test += (intersectionCard - 1) * m_fullCompStore.extra(i);
		}
	}
	if (test > subset.size() - 1) {
		m_osiSolver->addRow(row, 0, subset.size() - 1);
		return true;
	}
	return false;
}

template<typename T>
void
LPRelaxationSER<T>::addTerminalCoverConstraint()
{
	// we use the sum over all components C of (|C| - 1) * x_C = |R| - 1 constraint
	// from the paper
	CoinPackedVector row;

	for (int i = 0; i < m_fullCompStore.size(); ++i) {
		row.insert(i, m_fullCompStore.terminals(i).size() - 1);
	}

	int value = m_terminals.size() - 1;
	m_osiSolver->addRow(row, value, value);
}

template<typename T>
bool
LPRelaxationSER<T>::solve()
{
	bool initialIteration = true;

	do {
		if (initialIteration) {
			m_osiSolver->initialSolve();
#ifdef OGDF_STEINERTREE_LPRELAXATIONSER_LOGGING
			std::cout << "Objective value " << m_osiSolver->getObjValue() << " of initial solution." << std::endl;
#endif
			initialIteration = false;
		} else {
			m_osiSolver->resolve();
#ifdef OGDF_STEINERTREE_LPRELAXATIONSER_LOGGING
			std::cout << "Objective value " << m_osiSolver->getObjValue() << " after resolve." << std::endl;
#endif
		}

		if (!m_osiSolver->isProvenOptimal()) {
			if (m_upperBound > 0) { // failed due to better upper bound
				return false;
			} else { // failed due to infeasibility
				std::cerr << "Failed to optimize LP!" << std::endl;
				throw(-1);
			}
		}
	} while (separate());

	const double *constSol = m_osiSolver->getColSolution();
	const int numberOfColumns = m_osiSolver->getNumCols();

	for (int i = 0; i < numberOfColumns; ++i) {
		m_fullCompStore.extra(i) = constSol[i];
	}

#ifdef OGDF_STEINERTREE_LPRELAXATIONSER_OUTPUT_LP
	m_osiSolver->writeLp(stderr);
#endif

	return true;
}

template<typename T>
bool
LPRelaxationSER<T>::separate()
{
	const double *constSol = m_osiSolver->getColSolution();

	ArrayBuffer<int> activeComponents;
	for (int i = 0; i < m_fullCompStore.size(); ++i) {
		m_fullCompStore.extra(i) = constSol[i];
		if (m_fullCompStore.extra(i) > m_eps) {
			activeComponents.push(i);
		}
	}

#ifdef OGDF_STEINERTREE_LPRELAXATIONSER_SEPARATE_CONNECTED_COMPONENTS
	if (!m_separationStage) {
		if (separateConnected(activeComponents)) {
			return true;
		}
		m_separationStage = 1;
	}
#endif
	if (separateMinCut(activeComponents)) {
		return true;
	}
	return m_separateCliqueSize > 2 ? separateCycles(activeComponents) : false;
}

template<typename T>
bool
LPRelaxationSER<T>::separateConnected(const ArrayBuffer<int> &activeComponents)
{
	NodeArray<int> setID(m_G, -1); // XXX: NodeArray over terminals only would be better
	DisjointSets<> uf(m_terminals.size());
	for (node t : m_terminals) {
		setID[t] = uf.makeSet();
	}

	// union all nodes within one component
	for (int j = 0; j < activeComponents.size(); ++j) {
		auto terminals = m_fullCompStore.terminals(activeComponents[j]);
		auto it = terminals.begin();
		const int s1 = setID[*it];
		for (++it; it != terminals.end(); ++it) {
			uf.link(uf.find(s1), uf.find(setID[*it]));
		}
	}

	if (uf.getNumberOfSets() == 1) { // solution is connected
		return false;
	}

	Array<ArrayBuffer<node>> components(m_terminals.size());
	ArrayBuffer<int> usedComp;
	for (node t : m_terminals) {
		const int k = uf.find(setID[t]);
		if (components[k].empty()) {
			usedComp.push(k);
		}
		components[k].push(t);
	}
	int cutsFound = 0;
	for (const int k : usedComp) {
		cutsFound += addSubsetCoverConstraint(components[k]);
	}
	return true;
}

template<typename T>
bool
LPRelaxationSER<T>::separateMinCut(const ArrayBuffer<int> &activeComponents)
{
	int cutsFound = 0;
	node source;
	node pseudotarget;
	GraphCopy auxG;
	EdgeArray<double> capacity;
	double y_R = generateMinCutSeparationGraph(activeComponents, source, pseudotarget, auxG, capacity, cutsFound);

#ifdef OGDF_STEINERTREE_LPRELAXATIONSER_SEPARATE_YVAR_CONSTRAINTS
	if (cutsFound > 0) {
		return true;
	}
#endif

	node target = auxG.newNode();
	capacity[auxG.newEdge(pseudotarget, target)] = y_R;

	EdgeArray<double> flow;
	MaxFlowGoldbergTarjan<double> maxFlow;
	MinSTCutMaxFlow<double> minSTCut;
	for (node t : m_terminals) {
		const node v = auxG.copy(t);

		edge v_to_target = auxG.newEdge(v, target);
		capacity[v_to_target] = std::numeric_limits<double>::max(); // XXX: smaller is better

		maxFlow.init(auxG, &flow);

		const double cutVal = maxFlow.computeValue(capacity, source, target);
		if (cutVal - y_R < 1 - m_eps) {
			minSTCut.call(auxG, capacity, flow, source, target);
			ArrayBuffer<node> subset;
			for (node tOrig : m_terminals) {
				const node tCopy = auxG.copy(tOrig);
				if (tCopy && minSTCut.isInBackCut(tCopy)) {
					subset.push(tOrig);
				}
			}

			cutsFound += addSubsetCoverConstraint(subset);
		}

		auxG.delEdge(v_to_target);
	}
	return cutsFound != 0;
}

template<typename T>
bool
LPRelaxationSER<T>::separateCycles(const ArrayBuffer<int> &activeComponents)
{
	int count = 0;

	// generate auxiliary graph
	Graph G;
	NodeArray<int> id(G);
	for (int i : activeComponents) {
		id[G.newNode()] = i;
	}
	for (node u1 : G.nodes) {
		const int i1 = id[u1];
		const Array<node> &terminals1 = m_fullCompStore.terminals(i1);
		for (node u2 = u1->succ(); u2; u2 = u2->succ()) {
			const int i2 = id[u2];
			const Array<node> &terminals2 = m_fullCompStore.terminals(i2);
			// compute intersection cardinality (linear time because terminal sets are sorted by index)
			int intersectionCard = 0;
			const node *it1 = terminals1.begin();
			const node *it2 = terminals2.begin();
			while (it1 != terminals1.end()
			    && it2 != terminals2.end()) {
				if ((*it1)->index() < (*it2)->index()) {
					++it1;
				} else
				if ((*it1)->index() > (*it2)->index()) {
					++it2;
				} else { // ==
					++intersectionCard;
					if (intersectionCard == 2) {
						G.newEdge(u1, u2);
						break;
					}
					++it1;
					++it2;
				}
			}
		}
	}

	if (G.numberOfEdges() == 0) {
		return false;
	}

	// now find cliques
	Array<List<node>> degrees(m_separateCliqueSize);
	for (node v : G.nodes) {
		int idx = v->degree();
		if (idx == 0) { // ignore isolated nodes
			continue;
		}
		if (idx >= m_separateCliqueSize) {
			idx = m_separateCliqueSize - 1;
		}
		--idx;
		degrees[idx].pushBack(v);
	}
	NodeArray<bool> test(G, false);
	for (int k = degrees.size(); k >= 2; --k) {
		degrees[k-2].conc(degrees[k-1]);
		if (degrees[k-2].size() >= k) {
			SubsetEnumerator<node> nodeSubset(degrees[k-2]);
			for (nodeSubset.begin(k); nodeSubset.valid(); nodeSubset.next()) {
				int countEdges = (k * (k-1)) / 2;
				for (int j = 0; j < nodeSubset.size(); ++j) {
					test[nodeSubset[j]] = true;
				}
				for (edge e : G.edges) {
					if (test[e->source()]
					 && test[e->target()]) {
						countEdges -= 1;
					}
				}
				OGDF_ASSERT(countEdges >= 0);
				if (countEdges == 0) {
					// found clique, add constraint
					double val(0);
					CoinPackedVector row;

					for (int j = 0; j < nodeSubset.size(); ++j) {
						int i = id[nodeSubset[j]];
						val += m_fullCompStore.extra(i);
						row.insert(i, 1);
					}
					if (val >= 1 + m_eps) {
						m_osiSolver->addRow(row, 0, 1);
						++count;
					}
				}
				for (int j = 0; j < nodeSubset.size(); ++j) {
					test[nodeSubset[j]] = false;
				}
			}
		}
	}
	return count > 0;
}

}
}
