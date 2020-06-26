/** \file
 * \brief Implementation of an approxmiation algorithm for
 * Steiner tree problems provided by Michel X. Goemans and
 * David P. Williamson.
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

#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>
#include <ogdf/graphalg/MinSteinerTreeModule.h>
#include <ogdf/basic/DisjointSets.h>

//#define OGDF_MINSTEINERTREE_PRIMAL_DUAL_LOGGING

namespace ogdf {

/**
 * @brief Primal-Dual approximation algorithm for Steiner tree problems.
 * Yields a guaranteed approximation factor of two.
 *
 * This algorithm was first described by Michel X. Goemans and David P. Williamson in
 * "A General Approximation Technique for Constrained Forest Problems",
 * SIAM Journal on Computing, 24:296-317, 1995.
 *
 * @ingroup ga-steiner
 */
template<typename T>
class MinSteinerTreePrimalDual : public MinSteinerTreeModule<T> {
private:
	const EdgeWeightedGraph<T> *m_pGraph;
	const List<node> *m_pTerminals;
	const NodeArray<bool> *m_pIsTerminal;
	const T MAX_VALUE = std::numeric_limits<T>::max();

	NodeArray<int> m_componentMapping;
	DisjointSets<> *m_pComponents;
	HashArray<int, ListIterator<int>> m_activeComponentIterators;
	List<int> m_activeComponents;
	double m_lowerBound;
	NodeArray<double> m_priorities;

	/**
	 * Merges two disjoint components
	 *
	 * @param v representative of the first component
	 * @param w representative of the second component
	 */
	void mergeComponents(const node v, const node w);

	/**
	 * Marks the specified component as active.
	 *
	 * @param component the component to be activated.
	 * @return
	 */
	void makeActive(int component);

	/**
	 * Returns whether the given component is active.
	 *
	 * @return true if the component is active, false otherwise
	 */
	bool isActive(int component) const;

	/**
	 * Finds the biggest set including node v.
	 *
	 * @param v the representative of the set to find
	 */
	int getComponent(const node v) const;

	/**
	 * Idendifies the next edge with a tight-to-be packing constraint.
	 *
	 * @param nextEdge the found edge
	 * @return the adjusted weight (aka epsilon) for the found edge
	 */
	double getNextEdge(edge *nextEdge);

	/**
	 * Must be called after merging any two components.
	 * Will update all the priorities of all active edges by epsilon.
	 *
	 * @param eps the value of the last tight edge
	 */
	void updatePriorities(double eps);

	/**
	 * Initializes all required datastructes.
	 */
	void init();

protected:
	/**
	 * Builds a minimum Steiner tree given a weighted graph and a list of terminals
	 *
	 * \param G
	 * 	The weighted input graph
	 * \param terminals
	 * 	The list of terminal nodes
	 * \param isTerminal
	 * 	A bool array of terminals
	 * \param finalSteinerTree
	 * 	The final Steiner tree
	 *
	 * \return
	 * 	The objective value (sum of edge costs) of the final Steiner tree
	 */
	virtual T computeSteinerTree(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal,
			EdgeWeightedGraphCopy<T> *&finalSteinerTree) override;

public:
	virtual T call(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, EdgeWeightedGraphCopy<T> *&finalSteinerTree) override
	{
		m_lowerBound = 0;
		return MinSteinerTreeModule<T>::call(G, terminals, isTerminal, finalSteinerTree);
	}

	/**
	 * Returns the lower bound calculated while
	 * solving the last problem. Will return 0 if no problem
	 * was solved before.
	 */
	double getLastLowerBound() const;
};

template<typename T>
void MinSteinerTreePrimalDual<T>::init()
{
	m_activeComponentIterators.clear();
	m_activeComponents.clear();
	m_componentMapping.init(*m_pGraph);
	m_priorities.init(*m_pGraph, 0);
}

template<typename T>
int MinSteinerTreePrimalDual<T>::getComponent(const node v) const
{
	return m_pComponents->find(m_componentMapping[v]);
}

template<typename T>
bool MinSteinerTreePrimalDual<T>::isActive(int component) const
{
	return m_activeComponentIterators[component].valid();
}

template<typename T>
void MinSteinerTreePrimalDual<T>::makeActive(int comp)
{
	m_activeComponentIterators[comp] = m_activeComponents.pushBack(comp);
}

template<typename T>
void MinSteinerTreePrimalDual<T>::mergeComponents(const node v, const node w)
{

	int compV = getComponent(v);
	int compW = getComponent(w);

	// remove former components
	if(isActive(compV)) {
		m_activeComponents.del(m_activeComponentIterators[compV]);
	}
	if(isActive(compW)) {
		m_activeComponents.del(m_activeComponentIterators[compW]);
	}

	// craete new component
	int compNew = m_pComponents->link(compV, compW);
	if(!m_activeComponents.empty()) {
		makeActive(compNew);
	}
}

template<typename T>
void MinSteinerTreePrimalDual<T>::updatePriorities(double eps)
{
	List<node> nodes;
	m_pGraph->allNodes(nodes);
	for(node v : nodes) {
		if(isActive(getComponent(v))) {
			m_priorities(v) += eps;
		}
	}
}

template<typename T>
double MinSteinerTreePrimalDual<T>::getNextEdge(edge *nextEdge)
{
	double result = MAX_VALUE;
	*nextEdge = nullptr;

	List<edge> edges;
	m_pGraph->allEdges(edges);
	for(edge e : edges) {
		node v = e->source();
		node w = e->target();
		int compV = getComponent(v);
		int compW = getComponent(w);
		if(compV != compW) { // spanning different components ?
			double value = m_pGraph->weight(e) - m_priorities(v) - m_priorities(w);
			int divisor = isActive(compV) + isActive(compW);
			if(divisor == 0) {
				value = MAX_VALUE;
			} else {
				value /= divisor;
			}
			if(*nextEdge == nullptr || value < result) {
				*nextEdge = e;
				result = value;
			}
		}
	}
	return result;
}

template<typename T>
T MinSteinerTreePrimalDual<T>::computeSteinerTree(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, EdgeWeightedGraphCopy<T> *&finalSteinerTree)
{
	m_pGraph = &G;
	m_pTerminals = &terminals;
	m_pIsTerminal = &isTerminal;
	DisjointSets<> components;
	m_pComponents = &components;

	finalSteinerTree = new EdgeWeightedGraphCopy<T>();
	finalSteinerTree->createEmpty(*m_pGraph);

	init();

	// initialize components
	List<node> nodes;
	m_pGraph->allNodes(nodes);
	for(node v : nodes) {
		int comp = m_pComponents->makeSet();
		m_componentMapping[v] = comp;
		if(isTerminal(v)) {
			makeActive(comp);
		}
	}

#ifdef OGDF_MINSTEINERTREE_PRIMAL_DUAL_LOGGING
	std::cout << "Goemans primal-dual starting..." << std::endl;
	std::cout << "terminals:";
	for(node v : *m_pTerminals) {
		std::cout << " " << v;
	}
	std::cout << std::endl;

	std::cout << "loop starting... " << std::endl;
#endif

	T result = 0;
	while(!m_activeComponents.empty()) {
#ifdef OGDF_MINSTEINERTREE_PRIMAL_DUAL_LOGGING
		std::cout << "active component exists" << std::endl;
#endif
		// idendify next edge
		edge minEdge = nullptr;
		double minValue = getNextEdge(&minEdge);
		OGDF_ASSERT(minEdge != nullptr);

#ifdef OGDF_MINSTEINERTREE_PRIMAL_DUAL_LOGGING
		std::cout << "minEdge found: " << minEdge << ", weight is " << m_pGraph->weight(minEdge) << ", adjusted weight is " << minValue << std::endl;
#endif
		node v = minEdge->source();
		node w = minEdge->target();

		// include nodes in Steiner tree
		if(finalSteinerTree->copy(v) == nullptr) {
			finalSteinerTree->newNode(v);
		}
		if(finalSteinerTree->copy(w) == nullptr) {
			finalSteinerTree->newNode(w);
		}

		// include edge in Steiner tree
		T weight = m_pGraph->weight(minEdge);
		result += weight;
		finalSteinerTree->newEdge(minEdge, weight);

		m_lowerBound += m_activeComponents.size() * minValue;

		mergeComponents(v, w);

		updatePriorities(minValue);
	}
	result -= this->pruneAllDanglingSteinerPaths(*finalSteinerTree, *m_pIsTerminal);

#ifdef OGDF_MINSTEINERTREE_PRIMAL_DUAL_LOGGING
	std::cout << "calculation finished!" << std::endl;
#endif
	return result;
}

template<typename T>
double MinSteinerTreePrimalDual<T>::getLastLowerBound() const
{
	return m_lowerBound;
}

}
