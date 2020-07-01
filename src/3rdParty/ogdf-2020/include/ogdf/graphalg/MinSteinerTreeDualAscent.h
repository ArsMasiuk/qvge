/** \file
 * \brief Implementation of an approxmiation algorithm for
 * Steiner tree problems given by Richard T. Wong.
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

#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>
#include <ogdf/graphalg/MinSteinerTreeModule.h>

// enable this to print log
//#define OGDF_DUAL_ASCENT_LOGGING

namespace ogdf {

/**
 * @brief Dual ascent heuristic for the minimum Steiner tree problem.
 *
 * The algorithm is implemented following the paper by Richard T. Wong,
 * "A Dual Ascent Approach for Steiner Tree Problems on a Directed Graph",
 * Mathematical Programming 28, pages 271-287, 1984.
 *
 * @ingroup ga-steiner
 */
template<typename T>
class MinSteinerTreeDualAscent : public MinSteinerTreeModule<T> {
private:
	const T MAX_VALUE = std::numeric_limits<T>::max();

	const EdgeWeightedGraph<T> *m_pOrigGraph; //!< original graph passed to the module
	const List<node> *m_pTerminals; //!< list of terminals passed to the module
	const NodeArray<bool> *m_pIsTerminal; //!< terminal incidence vector passed to the module

	GraphCopy m_diGraph; //!< the directed graph
	GraphCopy m_steinerGraph; //!< the to-be constructed "almost" Steiner tree
	EdgeArray<edge> m_origMapping; //!< maps each directed edge to its undirected original

	EdgeArray<bool> m_edgeInclusions; //!< stores the resulting Steiner tree
	EdgeArray<T> m_edgeSlacks; //!< slack variables for each directed edge representing the adjusted weight

	node m_rootTerminal; //!< root node

	// components for the Steiner graph
	NodeArray<int> m_componentMapping; //!< maps each node to its component

	/**
	 * Intializes all relevant variables.
	 * Creates the respectiv graph copies.
	 */
	void init();

	/**
	 * Returns whether this node is a terminal.
	 * If v is the root terminal, the second parameter will be returned.
	 *
	 * @param v the node to be tested
	 * @param rootIsTerminal whether the root terminal should be considered as terminal
	 * @return true if v is a terminal, false otherwise
	 */
	bool isTerminal(const node v, bool rootIsTerminal) const;

	/**
	 * Searches for the next active component.
	 *
	 * @param terminal Will hold an arbitrary terminal of the returned component.
	 * @return the index of the found componennt or -1 if none is found
	 */
	int findActiveComponent(node *terminal) const;

	/**
	 * Returns the component of node v.
	 *
	 * @param v the node of the component
	 * @return the found component
	 */
	int findComponent(const node v) const;

	/**
	 * Returns all incoming cut edges of the component of v.
	 *
	 * @param v representative of the component
	 * @return a list of edges containing each incoming cut edge once
	 */
	List<edge> *computeCutSet(const node v) const;

	/**
	 * Determines whether a strongly connected component
	 * is active (paper says "is root component").
	 * A component is active if and only if it has no dangling terminal node
	 * and includes at least one component.
	 *
	 * @param v representative of the component
	 * @return true if active, false otherwise
	 */
	bool isActiveComponent(const node v) const;

	/**
	 * Re-establishes all strongly connected components
	 * for the Steiner graph.
	 */
	void updateComponents();

protected:

	/**
	 * Creates a minimum Steiner tree given a weighted graph and a list of terminals
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
	T computeSteinerTree(
	  const EdgeWeightedGraph<T> &G,
	  const List<node> &terminals,
	  const NodeArray<bool> &isTerminal,
	  EdgeWeightedGraphCopy<T> *&finalSteinerTree);
};

template<typename T>
void MinSteinerTreeDualAscent<T>::init()
{
	// local auxilary lists
	List<node> nodes;
	m_pOrigGraph->allNodes(nodes);
	List<edge> edges;
	m_pOrigGraph->allEdges(edges);

	// create directed graph
	// and initialize slack variables
	m_diGraph.createEmpty(*m_pOrigGraph);
	m_diGraph.clear();
	m_edgeSlacks.init(m_diGraph);
	m_origMapping.init(m_diGraph);

	// create resulting Steiner tree
	m_steinerGraph.createEmpty(m_diGraph);
	m_steinerGraph.clear();
	m_componentMapping.init(m_steinerGraph);

	// TODO: Better choices possible?
	m_rootTerminal = m_pTerminals->chooseElement();

	for(node v : nodes) {
		node w = m_diGraph.newNode(v);
		m_steinerGraph.newNode(w);
	}

	for(edge e : edges) {
		node source = m_diGraph.copy(e->source());
		node target = m_diGraph.copy(e->target());
		edge copiedEdgeS = m_diGraph.newEdge(source, target);
		edge copiedEdgeT = m_diGraph.newEdge(target, source);
		m_edgeSlacks[copiedEdgeS] = m_edgeSlacks[copiedEdgeT] = m_pOrigGraph->weight(e);
		m_origMapping[copiedEdgeS] = m_origMapping[copiedEdgeT] = e;
	}
	updateComponents();

#ifdef OGDF_DUAL_ASCENT_LOGGING
	std::cout << "directed graph has " << m_diGraph.numberOfNodes() << " nodes "
	     << "and " << m_diGraph.numberOfEdges() << " edges." << std::endl
	     << "root terminal is node " << m_rootTerminal << "." << std::endl;
#endif
}

template<typename T>
int MinSteinerTreeDualAscent<T>::findComponent(const node v) const
{
	OGDF_ASSERT(v->graphOf() == &m_steinerGraph);
	OGDF_ASSERT(m_componentMapping[v] > -1);
	return m_componentMapping[v];
}

template<typename T>
void MinSteinerTreeDualAscent<T>::updateComponents()
{
	strongComponents(m_steinerGraph, m_componentMapping);
}

template<typename T>
bool MinSteinerTreeDualAscent<T>::isTerminal(const node v, bool rootIsTerminal) const
{
	OGDF_ASSERT(v->graphOf() == &m_steinerGraph);

	node w = m_diGraph.original(m_steinerGraph.original(v));
	return (m_rootTerminal != w || rootIsTerminal) && (*m_pIsTerminal)[w];
}

template<typename T>
int MinSteinerTreeDualAscent<T>::findActiveComponent(node *terminal) const
{
	int result = -1;

#ifdef OGDF_DUAL_ASCENT_LOGGING
	std::cout << "  searching for active component.." << std::endl;
#endif

	HashArray<int, bool> checked(false);
	for(auto it = m_pTerminals->begin(); it != m_pTerminals->end() && result == -1; it++) {
		if(*it != m_rootTerminal) {
			node v = m_steinerGraph.copy(m_diGraph.copy(*it));
			int candidate = findComponent(v);
			if(!checked[candidate]) {
				checked[candidate] = true;
				bool isRoot = isActiveComponent(v);

				if(isRoot) {
					result = candidate;
					*terminal = *it;
#ifdef OGDF_DUAL_ASCENT_LOGGING
					std::cout << "  active component found: component " << result << ", terminal " << *terminal << std::endl;
#endif
				}
			}
		}
	}

#ifdef OGDF_DUAL_ASCENT_LOGGING
	if(result == -1) {
		std::cout << "  could not find an active component" << std::endl;
	}
#endif

	return result;
}

template<typename T>
List<edge> *MinSteinerTreeDualAscent<T>::computeCutSet(const node root) const
{
	OGDF_ASSERT(root->graphOf() == &m_steinerGraph);

	// establish "weakly connected" component of v (non-standard definition, see paper for details)
	NodeArray<bool> visited(m_steinerGraph, false);
	List<node> weakComp;
	visited[root] = true;

	List<node> queue;
	queue.pushBack(root);

	// determine all nodes connected to root
	// meaning all nodes from which a directed path to root exists
	while(!queue.empty()) {
		node v = queue.popFrontRet();
		weakComp.pushBack(v);
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node w = e->source();
			if(!visited[w]) {
				visited[w] = true;
				queue.pushBack(w);
			}
		}
	}

	// identify (incoming) cut edges
	List<edge> *result = new List<edge>();

	for(node v : weakComp) {
		node w = m_steinerGraph.original(v);
		for(adjEntry adj : w->adjEntries) {
			edge e = adj->theEdge();
			if(!visited[m_steinerGraph.copy(e->source())]) {
				result->pushBack(e);
				OGDF_ASSERT(m_steinerGraph.copy(e) == nullptr);
			}
		}
	}
	return result;
}

template<typename T>
bool MinSteinerTreeDualAscent<T>::isActiveComponent(const node source) const
{
	OGDF_ASSERT(source->graphOf() == &m_steinerGraph);

	int comp = findComponent(source);
	bool danglingTerminalFound = false;
	bool hasTerminal = false;

#ifdef OGDF_DUAL_ASCENT_LOGGING
	std::cout << "    checking whether component of node " << source << " is active.. " << std::endl;
	std::cout << "      component has id " << comp << std::endl;
#endif

	List<node> queue;
	NodeArray<bool> visited(m_steinerGraph, false);
	queue.pushBack(source);
	visited[source] = true;
#ifdef OGDF_DUAL_ASCENT_LOGGING
	while(!queue.empty()) {
#else
	while(!queue.empty() && !danglingTerminalFound) {
#endif
	node v = queue.popFrontRet();
		hasTerminal |= isTerminal(v, false) && findComponent(v) == comp;
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node w = e->source();
			if(!visited[w]) {
				danglingTerminalFound |= isTerminal(w, true) && findComponent(w) != comp;
				visited[w] = true;
				queue.pushBack(w);
			}
		}
	}

#ifdef OGDF_DUAL_ASCENT_LOGGING
	if(hasTerminal) { std::cout << "      component includes a terminal" << std::endl; }
	if(danglingTerminalFound) { std::cout << "      component has dangling terminal" << std::endl; }
	if(hasTerminal && !danglingTerminalFound) { std::cout << "      component is active!" << std::endl; }
#endif
	return hasTerminal && !danglingTerminalFound;
}

template<typename T>
T MinSteinerTreeDualAscent<T>::computeSteinerTree(
	const EdgeWeightedGraph<T> &G,
	const List<node> &terminals,
	const NodeArray<bool> &isTerminal,
	EdgeWeightedGraphCopy<T> *&finalSteinerTree)
{
#ifdef OGDF_DUAL_ASCENT_LOGGING
	std::cout << "MinSteinerTreeDualAscent called." << std::endl;
	std::cout << "terminals are: ";

	for(node v : terminals) {
		std::cout << v << " ";
	}
	std::cout << std::endl;
#endif
	m_pOrigGraph = &G;
	m_pTerminals = &terminals;
	m_pIsTerminal = &isTerminal;

	init();

	// create resulting Steiner tree
	finalSteinerTree = new EdgeWeightedGraphCopy<T>();
	finalSteinerTree->createEmpty(*m_pOrigGraph);
	T result = 0;

	int comp = -1;
	node terminal = nullptr;

#ifdef OGDF_DUAL_ASCENT_LOGGING
	std::cout << "main loop starting.." << std::endl;
#endif
	while((comp = findActiveComponent(&terminal)) != -1) {
		// if active comonents exists we
		// have one of its terminals
		OGDF_ASSERT(terminal != nullptr);

		// find minimal cut edge
		List<edge> *cutEdges = computeCutSet(m_steinerGraph.copy(m_diGraph.copy(terminal)));
		edge minEdge = nullptr;
		T minSlack = MAX_VALUE;
#ifdef OGDF_DUAL_ASCENT_LOGGING
		std::cout << "  cut edges:";
#endif
		for(edge e : *cutEdges) {
#ifdef OGDF_DUAL_ASCENT_LOGGING
			std::cout << " " << e;
#endif
			if(m_edgeSlacks[e] < MAX_VALUE || minEdge == nullptr) {
				minEdge = e;
				minSlack = m_edgeSlacks[e];
			}
		}
#ifdef OGDF_DUAL_ASCENT_LOGGING
		std::cout << std::endl << "  next edge: " << minEdge << std::endl;
#endif
		OGDF_ASSERT(minEdge != nullptr);

		// update slack variables
		for(edge e : *cutEdges) {
			m_edgeSlacks[e] -= minSlack;
		}
		delete cutEdges;

		// insert edge
		m_steinerGraph.newEdge(minEdge);
		updateComponents();

		// insert edge into final Steiner tree
		edge origEdge = m_origMapping[minEdge];
		T cost = m_pOrigGraph->weight(origEdge);
		if (finalSteinerTree->copy(origEdge->source()) == nullptr) {
			finalSteinerTree->newNode(origEdge->source());
		}
		if (finalSteinerTree->copy(origEdge->target()) == nullptr) {
			finalSteinerTree->newNode(origEdge->target());
		}
		if (finalSteinerTree->copy(origEdge) == nullptr) {
			finalSteinerTree->newEdge(origEdge, cost);
			result += cost;
		}
	}

#ifdef OGDF_DUAL_ASCENT_LOGGING
	std::cout << "removing expendable edges" << std::endl;
#endif
	result -= this->pruneAllDanglingSteinerPaths(*finalSteinerTree, *m_pIsTerminal);
	result -= this->removeCyclesFrom(*finalSteinerTree, *m_pIsTerminal);

#ifdef OGDF_DUAL_ASCENT_LOGGING
	std::cout << "algorithm terminated." << std::endl;
#endif
	return result;
}

}
