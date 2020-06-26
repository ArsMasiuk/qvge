/** \file
 * \brief Implementation of Shore, Foulds and Gibbons' branch
 * and bound algorithm for solving minimum Steiner tree problems.
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

// enable this to print messages
// for all branches (edge inclusion or exclusion)
// additionally a SVG image is generated for each
// recursion depth
//#define OGDF_MINSTEINERTREE_SHORE_LOGGING

#include <memory>

#include <ogdf/basic/List.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/NodeSet.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>
#include <ogdf/graphalg/MinSteinerTreeModule.h>

namespace ogdf {

/**
 * @brief Implementation of Shore, Foulds and Gibbons exact branch and bound
 * algorithm for solving Steiner tree problems.
 *
 * (Shore M.L., Foulds, L.R., and Gibbons, P.B.
 * An Algorithm for the Steiner Problem in Graphs
 * Networks 10:323-333, 1982.)
 *
 * @ingroup ga-steiner
 */
template<typename T>
class MinSteinerTreeShore: public MinSteinerTreeModule<T> {
public:

	MinSteinerTreeShore() : MAX_WEIGHT(std::numeric_limits<T>::max()) {};
	virtual ~MinSteinerTreeShore() {};

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

	const T MAX_WEIGHT;
private:
	const EdgeWeightedGraph<T> *m_originalGraph;
	const List<node> *m_originalTerminals;

	Graph m_graph;
	std::unique_ptr<NodeSet<>> m_terminals;
	EdgeArray<edge> m_mapping;
	T m_upperBound;
	Array2D<edge> m_edges;
	List<edge> m_chosenEdges;
	int m_recursionDepth;

	/**
	 * Used to validate the current mapping of edges to orignal edges
	 * Used solely for debugging.
	 * The mapping is validated by using OGDF_ASSERT.
	 *
	 * \return
	 * 	always returns true
	 */
	bool validateMapping() const;

	/**
	 * Returns the cost of the specified edge.
	 * Looks up the corresponding edge in the original graph
	 * and retrieves its weight.
	 *
	 * \return
	 *   weight of e
	 */
	T weightOf(edge e) const;

	/**
	 * Calculates the optimal Steinter tree recursivly.
	 * Should not be called directly but by STPSolver::solve.
	 * Each edge is either included or excluded,
	 * which gives rise to up to two new branches in each step.
	 *
	 * \param prevCost
	 *	the cost accumulated in previous recursion steps (previously included edges)
	 * \param currentEdges
	 *	the current edges
	 *
	 * \return
	 *	the total weight of the optimal solution (including prevCost)
	 *	note: This might be higher then the actual solution if no solution
	 *	satisfying the upper bound can be found.
	 */
	T bnbInternal(T prevCost, List<edge> &currentEdges);

	/**
	 * Removes the specified edge from the graph.
	 * The corresponding original edge is returned.
	 *
	 * \return
	 *	the original edge, according to m_mapping
	 */
	edge deleteEdge(edge e);

	/**
	 * Creates a new edge.
	 *
	 * \param source
	 *	the source node of the new edge
	 * \param target
	 *	the target node of the new edge
	 * \param originalEdge
	 *	the corresponding edge in the original graph
	 */
	edge newEdge(node source, node target, const edge originalEdge);

	/**
	 * Moves the source of the edge to the specified node.
	 *
	 * \param e
	 *	the edge to be moved
	 * \param v
	 * 	the new source of e
	 */
	void moveSource(edge e, node v);

	/**
	 * Moves the target of the edge to the specified node.
	 *
	 * \param e
	 *	the edge to be moved
	 * \param v
	 * 	the new target of e
	 */
	void moveTarget(edge e, node v);

	/**
	 * Updates the status of the given node to
	 * either terminal or Steiner node.
	 * No side-effects occur even if status is already correct.
	 *
	 * \param v
	 *	the node to be updated
	 * \param makeTerminal
	 * 	true to set it to terminal
	 *	false to set it to nonterminal
	 */
	void setTerminal(const node v, bool makeTerminal);

	/**
	 * Returns whether this node is a terminal or
	 * a Steiner node.
	 *
	 * \param v
	 *	the node to check
	 *
	 * \return
	 *	true if v is a terminal
	 * 	false otherwise
	 *
	 */
	bool isTerminal(const node v) const;

	/**
	 * Sets the edge incident to both node u and v.
	 * Used for faster lookup.
	 *
	 * \param u
	 *	one of the nodes of the undirected edge
	 * \param v
	 *	the opposite node of u
	 * \param e
	 *	the incident edge
	 *
	 */
	void setEdgeLookup(const node u, const node v, const edge e);

	/**
	 * Retrieves the edge incident to both node u and v.
	 *
	 * \param u
	 *	one of the nodes of the undirected edge
	 * \param v
	 *	the opposite node of u
	 *
	 * \return
	 * 	the edge between u and v
	 *	nullptr if it does not exist
	 */
	edge lookupEdge(const node u, const node v) const;

	/**
	 * Decides which edge to branch on.
	 * Might return nullptr if current upper bound can not be reached
	 * with this graph.
	 *
	 * \param prevCost
	 *	the cost of previously chosen edges
	 *	used for comparing to current upper bound
	 *
	 * \return
	 *	edge to branch on next
	 *	might be nullptr if current upper bound is not reachable
	 */
	edge determineBranchingEdge(T prevCost) const;

	/**
	 * Solves the current STP instance.
	 * Will return the total cost of the optimal solution.
	 *
	 * \param chosenEdges
	 * 	will hold the included edges
	 */
	T solve(List<edge> &chosenEdges);

	/**
	 * Prints the current recursion status
	 * as a SVG image of the current reduced STP.
	 */
	void printSVG();
};

template<typename T>
T MinSteinerTreeShore<T>::computeSteinerTree(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, EdgeWeightedGraphCopy<T> *&finalSteinerTree)
{
	m_originalGraph = &G;
	m_originalTerminals = &terminals;

	m_upperBound = MAX_WEIGHT;
	m_graph = Graph();
	m_mapping.init(m_graph);
	m_terminals.reset(new NodeSet<>(m_graph));
	int nodeCount = m_originalGraph->numberOfNodes();
	m_edges = Array2D<edge>(0, nodeCount, 0, nodeCount, nullptr);

	NodeArray<node> copiedNodes(*m_originalGraph);

	for(node v : m_originalGraph->nodes) {
		node copiedNode = m_graph.newNode();
		copiedNodes[v] = copiedNode;
	}

	for(edge e : m_originalGraph->edges) {
		node source = copiedNodes[e->source()],
		     target = copiedNodes[e->target()];

		newEdge(source, target, e);
	}

	for(node v : *m_originalTerminals) {
		setTerminal(copiedNodes[v], true);
	}

	List<edge> chosenEdges;
	T result = solve(chosenEdges);

	finalSteinerTree = new EdgeWeightedGraphCopy<T>();
	finalSteinerTree->createEmpty(*m_originalGraph);

	for(edge e : chosenEdges) {
		node v = e->source();
		node w = e->target();

		OGDF_ASSERT(v != nullptr);
		OGDF_ASSERT(w != nullptr);
		OGDF_ASSERT(e->graphOf() == m_originalGraph);
		OGDF_ASSERT(v->graphOf() == m_originalGraph);
		OGDF_ASSERT(w->graphOf() == m_originalGraph);

		if(finalSteinerTree->copy(v) == nullptr) {
			finalSteinerTree->newNode(v);
		}
		if(finalSteinerTree->copy(w) == nullptr) {
			finalSteinerTree->newNode(w);
		}
		finalSteinerTree->newEdge(e, m_originalGraph->weight(e));
	}

	return result;
}

template<typename T>
T MinSteinerTreeShore<T>::weightOf(const edge e) const
{
	T result = MAX_WEIGHT;

	if(e != nullptr) {
		OGDF_ASSERT(e->graphOf() == &m_graph);
		OGDF_ASSERT(m_mapping[e] != nullptr);
		OGDF_ASSERT(m_mapping[e]->graphOf() == m_originalGraph);
		result = m_originalGraph->weight(m_mapping[e]);
	}

	return result;
}

template<typename T>
bool MinSteinerTreeShore<T>::validateMapping() const
{
	for (edge e : m_graph.edges) {
		OGDF_ASSERT(m_mapping[e] != nullptr);
		OGDF_ASSERT(m_mapping[e]->graphOf() == m_originalGraph);
	}
	return true;
}

template<typename T>
edge MinSteinerTreeShore<T>::lookupEdge(const node u, const node v) const
{
	return m_edges(u->index(), v->index());
}

template<typename T>
void MinSteinerTreeShore<T>::setEdgeLookup(const node u, const node v, const edge e)
{
	m_edges(u->index(), v->index()) =
	  m_edges(v->index(), u->index()) = e;
}

template<typename T>
edge MinSteinerTreeShore<T>::deleteEdge(edge e)
{
	edge result = m_mapping[e];
	setEdgeLookup(e->source(), e->target(), nullptr);
	m_graph.delEdge(e);
	e->~EdgeElement();

	return result;
}

template<typename T>
edge MinSteinerTreeShore<T>::newEdge(node source, node target, edge e)
{
	edge result = m_graph.newEdge(source, target);
	m_mapping[result] = e;
	setEdgeLookup(source, target, result);

	return result;
}

template<typename T>
void MinSteinerTreeShore<T>::moveSource(edge e, node v)
{
	OGDF_ASSERT(e != nullptr);
	setEdgeLookup(e->source(), e->target(), nullptr);
	setEdgeLookup(v, e->target(), e);
	m_graph.moveSource(e, v);
}

template<typename T>
void MinSteinerTreeShore<T>::moveTarget(edge e, node v)
{
	OGDF_ASSERT(e != nullptr);
	setEdgeLookup(e->source(), e->target(), nullptr);
	setEdgeLookup(e->source(), v, e);
	m_graph.moveTarget(e, v);
}

template<typename T>
bool MinSteinerTreeShore<T>::isTerminal(const node v) const
{
	return m_terminals->isMember(v);
}

template<typename T>
void MinSteinerTreeShore<T>::setTerminal(const node v, bool makeTerminal)
{
	if (makeTerminal) {
		m_terminals->insert(v);
	} else {
		m_terminals->remove(v);
	}
}

template<typename T>
T MinSteinerTreeShore<T>::solve(List<edge> &chosenEdges)
{
	m_chosenEdges.clear();

	m_recursionDepth = 0;
	List<edge> tmp;
	T result = bnbInternal(0, tmp);

	for(edge e : m_chosenEdges) {
		chosenEdges.pushFront(e);
	}

	return result;
}

template<typename T>
edge MinSteinerTreeShore<T>::determineBranchingEdge(T prevCost) const
{
	edge result = nullptr;
	T maxPenalty = -1;

	// calculate penalties for nodes
	T sumOfMinWeights = 0; // b
	T sumOfMinTermWeights = 0; // c
	T absoluteMinTermWeight = MAX_WEIGHT;
	for(ListConstIterator<node> it = m_terminals->nodes().begin();
	  sumOfMinWeights < MAX_WEIGHT && it.valid();
	  ++it) {
		const node t = *it;
		T minTermWeight = MAX_WEIGHT,
		  minWeight = MAX_WEIGHT,
		  secondMinWeight = MAX_WEIGHT;
		edge minEdge = nullptr;

		// investigate all edges of each terminal
		// calculate lower boundary and find branching edge
		for(adjEntry adj = t->firstAdj(); adj; adj = adj->succ()) {
			edge e = adj->theEdge();
			if(weightOf(e) < minWeight) {
				secondMinWeight = minWeight;
				minWeight = weightOf(e);
				minEdge = e;
			}
			else {
				if(weightOf(e) < secondMinWeight) {
					secondMinWeight = weightOf(e);
				}
			}

			if(isTerminal(adj->twinNode()) && weightOf(e) < minTermWeight) {
				minTermWeight = weightOf(e);
				if(minTermWeight < absoluteMinTermWeight) {
					absoluteMinTermWeight = minTermWeight;
				}
			}
		}

		if(sumOfMinTermWeights < MAX_WEIGHT && minTermWeight < MAX_WEIGHT) {
			sumOfMinTermWeights += minTermWeight;
		}
		else {
			sumOfMinTermWeights = MAX_WEIGHT;
		}
		OGDF_ASSERT(absoluteMinTermWeight <= sumOfMinTermWeights);

		// is terminal isolated or has only one edge?
		// if so we can break here
		if(minWeight == MAX_WEIGHT ||
		   secondMinWeight == MAX_WEIGHT) {
			result = minEdge;
			if(minWeight == MAX_WEIGHT) {
				sumOfMinWeights = MAX_WEIGHT;
			}
		} else {
			sumOfMinWeights += minWeight;
			// update branching edge if need be
			T penalty = secondMinWeight - minWeight;
			if(penalty > maxPenalty) {
				maxPenalty = penalty;
				result = minEdge;
			}
		}
	}

	// compare bounds for this graph
	if(result != nullptr) {
		T maxCost = m_upperBound - prevCost;
		if(sumOfMinTermWeights < MAX_WEIGHT) {
			sumOfMinTermWeights -= absoluteMinTermWeight;
		}
		if(maxCost <= sumOfMinWeights && maxCost <= sumOfMinTermWeights) {
			result = nullptr;
		}
	}

	return result;
}

template<typename T>
T MinSteinerTreeShore<T>::bnbInternal(T prevCost, List<edge> &currentEdges)
{
	T result = MAX_WEIGHT;
	m_recursionDepth++;

#ifdef OGDF_MINSTEINERTREE_SHORE_LOGGING
	printSVG();
#endif

	if(prevCost <= m_upperBound) {
		if(m_terminals->size() < 2) {
			// update currently chosen edges
			if(prevCost != m_upperBound || m_chosenEdges.empty()) {

				m_chosenEdges = List<edge>(currentEdges);
			}
			// all terminals are connected
			m_upperBound = prevCost;
			result = prevCost;
		}
		else {
			edge branchingEdge = determineBranchingEdge(prevCost);
			T branchingEdgeWeight = weightOf(branchingEdge);

#ifdef OGDF_MINSTEINERTREE_SHORE_LOGGING
			for(int i = 0; i < m_recursionDepth; i++) std::cout << " ";
			std::cout << "branching on edge: " << branchingEdge << std::endl;
#endif
			// branching edge has been found or there is no feasible solution
			if(branchingEdgeWeight < MAX_WEIGHT) {
				// chose node to remove
				node nodeToRemove = branchingEdge->source();
				node targetNode = branchingEdge->target();

				// This seems to speed up things.
				if(nodeToRemove->degree() < targetNode->degree()) {
					nodeToRemove = branchingEdge->target();
					targetNode = branchingEdge->source();
				}

				// remove branching edge
				edge origBranchingEdge = deleteEdge(branchingEdge);

				List<node> delEdges, movedEdges;
				List<edge> origDelEdges;

				// first branch: Inclusion of the edge
				// remove source node of edge and calculate new edge weights
				OGDF_ASSERT(targetNode != nullptr);
				OGDF_ASSERT(nodeToRemove != nullptr);

				// remove edges in case of multigraph
				while(m_graph.searchEdge(targetNode, nodeToRemove) != nullptr) {
					edge e = m_graph.searchEdge(targetNode, nodeToRemove);
					delEdges.pushFront(e->target());
					delEdges.pushFront(e->source());
					origDelEdges.pushFront(m_mapping[e]);
					deleteEdge(e);
				}
				while(m_graph.searchEdge(nodeToRemove, targetNode) != nullptr) {
					edge e = m_graph.searchEdge(targetNode, nodeToRemove);
					delEdges.pushFront(e->target());
					delEdges.pushFront(e->source());
					origDelEdges.pushFront(m_mapping[e]);
					deleteEdge(e);
				}

				OGDF_ASSERT(m_graph.searchEdge(targetNode, nodeToRemove) == nullptr);
				OGDF_ASSERT(m_graph.searchEdge(nodeToRemove, targetNode) == nullptr);

				adjEntry adjNext;
				for(adjEntry adj = nodeToRemove->firstAdj(); adj; adj = adjNext) {
					adjNext = adj->succ();
					edge e = adj->theEdge();

					OGDF_ASSERT(e != branchingEdge);
					OGDF_ASSERT(e->target() == nodeToRemove || e->source() == nodeToRemove);
					OGDF_ASSERT(adj->twinNode() != targetNode);

					edge f = lookupEdge(targetNode, adj->twinNode());
					bool deletedEdgeE = false;
					if(f != nullptr) {
						if(weightOf(f) < weightOf(e)) {
							delEdges.pushFront(e->target());
							delEdges.pushFront(e->source());
							origDelEdges.pushFront(m_mapping[e]);
							deleteEdge(e);

							deletedEdgeE = true;
						}
						else {
							delEdges.pushFront(f->target());
							delEdges.pushFront(f->source());
							origDelEdges.pushFront(m_mapping[f]);
							deleteEdge(f);
						}
					}
					if(!deletedEdgeE) {
						if(e->target() == nodeToRemove) {
							OGDF_ASSERT(e->source() != targetNode);
							movedEdges.pushFront(e->source());
							moveTarget(e, targetNode);
						}
						else {
							OGDF_ASSERT(e->source() == nodeToRemove);
							OGDF_ASSERT(e->target() != targetNode);
							movedEdges.pushFront(e->target());
							moveSource(e, targetNode);
						}
					}
				}
				// nodeToRemove is isolated at this point
				// thus no need to actually remove it
				// (easier to keep track of CopyGraph mapping)

				// remove node from terminals too
				bool targetNodeIsTerminal = isTerminal(targetNode),
				     nodeToRemoveIsTerminal = isTerminal(nodeToRemove);

				OGDF_ASSERT(targetNodeIsTerminal || nodeToRemoveIsTerminal);
				setTerminal(nodeToRemove, false);
				setTerminal(targetNode, true);

#ifdef OGDF_MINSTEINERTREE_SHORE_LOGGING
				for(int i = 0; i < m_recursionDepth; i++) std::cout << " ";
				std::cout << "inclusion branch"  << std::endl;
#endif
				// calculate result on modified graph
				currentEdges.pushFront(origBranchingEdge);
				result = bnbInternal(branchingEdgeWeight + prevCost, currentEdges);
				OGDF_ASSERT(currentEdges.front() == origBranchingEdge);
				currentEdges.popFront();

				// restore previous graph

				// restore terminals
				setTerminal(nodeToRemove, nodeToRemoveIsTerminal);
				setTerminal(targetNode, targetNodeIsTerminal);

				// restore moved edges
				while(!movedEdges.empty()) {
					node v = movedEdges.popFrontRet();

					edge e = lookupEdge(v, targetNode);
					OGDF_ASSERT(e != nullptr);
					OGDF_ASSERT(e->opposite(targetNode) != nodeToRemove);

					if(e->source() == v) {
						moveTarget(e, nodeToRemove);
					}
					else {
						moveSource(e, nodeToRemove);
					}
				}

				// restore deleted edges
				while(!delEdges.empty()) {
					OGDF_ASSERT(!origDelEdges.empty());

					node source = delEdges.popFrontRet();
					node target = delEdges.popFrontRet();

					newEdge(source, target, origDelEdges.popFrontRet());
				}
				OGDF_ASSERT(origDelEdges.empty());

#ifdef OGDF_MINSTEINERTREE_SHORE_LOGGING
				for(int i = 0; i < m_recursionDepth; i++) std::cout << " ";
				std::cout << "exclusion branch"  << std::endl;
#endif
				// sencond branch: Exclusion of the edge
				T exEdgeResult = bnbInternal(prevCost, currentEdges);

				// decide which branch returned best result
				if(exEdgeResult < result) {
					result = exEdgeResult;
				}

				// finally: restore the branching edge
				newEdge(nodeToRemove, targetNode, origBranchingEdge);
			}
		}
		OGDF_ASSERT(validateMapping());
	}
	m_recursionDepth--;
	return result;
}

template<typename T>
void MinSteinerTreeShore<T>::printSVG() {
	EdgeWeightedGraphCopy<T> copiedGraph;
	copiedGraph.createEmpty(m_graph);
	List<node> nodes;
	m_graph.allNodes(nodes);
	NodeArray<bool> copiedIsTerminal(m_graph);
	for(node v : nodes) {
		copiedGraph.newNode(v);
		copiedIsTerminal[v] = isTerminal(v);
	}
	List<edge> edges;
	m_graph.allEdges(edges);
	for(edge e : edges) {
		copiedGraph.newEdge(e, weightOf(e));
	}
	std::stringstream filename;
	filename << "bnb_internal_" << m_recursionDepth << ".svg";
	this->drawSteinerTreeSVG(copiedGraph, copiedIsTerminal, filename.str().c_str());
}

}
