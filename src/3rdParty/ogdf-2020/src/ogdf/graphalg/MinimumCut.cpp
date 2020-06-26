/** \file
 * \brief Declares & Implements a minimum-cut algorithmn according
 * to an approach of Stoer and Wagner 1997
 *
 * \author Mathias Jansen
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

#include <ogdf/basic/PriorityQueue.h>
#include <ogdf/graphalg/MinimumCut.h>
#include <ogdf/basic/Math.h>

//used solely for efficiency and correctness checks of priority
//queue usage
//#define USE_PRIO


namespace ogdf {


MinCut::MinCut(Graph &G, EdgeArray<double> &w) : m_GC(G) {

	// Due to the node contraction (which destroys the Graph step by step),
	// we have to create a GraphCopy.
#if 0
	m_GC = new GraphCopy(G);
#endif

	// Edge weights are initialized.
	m_w.init(m_GC);
	for(edge e : m_GC.edges) {
		m_w[e] = w[(m_GC).original(e)];
	}
	m_contractedNodes.init(m_GC);
	m_minCut = 1e20;
}


MinCut::~MinCut() {}


void MinCut::contraction(node t, node s) {

	/*
	 * The contraction of the two nodes \p s and \p t is performed as follows:
	 * in the first step, all edges between \p s and \p t are deleted, and all edges incident
	 * to \p s are redirected to \p t. Then, node \p s is deleted and the adjacency list of \p t
	 * is checked for parallel edges. If k parallel edges are found, k-1 of them are deleted
	 * and their weights are added to the edge that is left.
	 */

	// Step 1: redirecting edges and deleting node \p s
	adjEntry adj = s->firstAdj();
	while (adj != nullptr)
	{
		adjEntry succ = adj->succ();
		edge e = adj->theEdge();
		if (e->source() == t || e->target() == t) {
			m_GC.delEdge(e);
		}
		else if (e->source() == s) {
			m_GC.moveSource(e,t);
		}
		else {
			m_GC.moveTarget(e,t);
		}
		adj = succ;
	}
	m_GC.delNode(s);

	/*
	 * Because of technical problems that occur when deleting edges and thus adjacency entries in a loop,
	 * a NodeArray is filled with the edges incident to node \p t.
	 * This NodeArray is checked for entries with more than one edge, which corresponds
	 * to parallel edges.
	 */

	// NodeArray containing parallel edges
	NodeArray<List<edge> > adjNodes(m_GC);

	for(adjEntry adjT : t->adjEntries) {
		adjNodes[adjT->twinNode()].pushBack(adjT->theEdge());
	}

	// Step 2: deleting parallel edges and adding their weights
	node v = m_GC.firstNode();
	while (v!=nullptr) {
		if (adjNodes[v].size() > 1) {
			edge e = adjNodes[v].front();
			adjNodes[v].popFront();
			for (edge ei : adjNodes[v]) {

				// Add weight of current edge to \a e.
				m_w[e] += m_w[ei];
				m_GC.delEdge(ei);
			}
		}
		v = v->succ();
	}
}


double MinCut::minimumCutPhase() {

	/*
	 * This function computes the mincut of the current phase.
	 * First, nodes are added successively in descending order of the sum of
	 * their incident edge weights to the list \a markedNodes.
	 * Afterwards, the current mincut value \a cutOfThePhase is computed, which corresponds to the
	 * sum of the weights of those edges incident to node \a t, which is the node that has been
	 * added to list \a markedNodes at last.
	 * At the end, the two last added nodes (\a s and \a t) are contracted and the \cutOfThePhase
	 * is returned.
	 */

	// Contains the mincut value according to the current phase.
	double cutOfThePhase;

	List<node> markedNodes;
	List<node> leftoverNodes;

	// Contains for each node the sum of the edge weights of those edges
	// incident to nodes in list \a markedNodes
	NodeArray<double> nodePrio(m_GC);
#ifdef USE_PRIO
	PrioritizedMapQueue<node, double> pq(m_GC);
#endif
	// The two nodes that have been added last to the list \a markedNodes.
	// These are the two nodes that have to be contracted at the end of the function.
	node s,t;

	// Initialization of data structures
	for(node v : m_GC.nodes) {
		leftoverNodes.pushBack(v);
#ifdef USE_PRIO
		pq.push(v, 0.0);
#endif
	}
	nodePrio.fill(0.0); //should do this in constructor init above

	// The start-node can be chosen arbitrarily. It has no effect on the correctness of the algorithm.
	// Here, always the first node in the list \a leftoverNodes is chosen.
	node v = leftoverNodes.popFrontRet(); markedNodes.pushBack(v);
	//assumes that no multiedges exist
	for(adjEntry adj : v->adjEntries) {
		nodePrio[adj->twinNode()] = m_w[adj->theEdge()];
#ifdef USE_PRIO
		pq.decrease(adj->twinNode(), -m_w[adj->theEdge()]);
#endif
	}

	// Temporary variables
	ListIterator<node> maxWeightNodeIt;
	//replaces line above
#ifdef USE_PRIO
	node maxWeightNodePq;
#endif

	// Successively adding the most tightly connected node.
	while (markedNodes.size() != m_GC.numberOfNodes()) {

		double mostTightly = 0.0;
		node maxWeightNode = nullptr;
#ifdef USE_PRIO
		//Find the most tightly connected node
		maxWeightNodePq = nullptr;
		if (pq.topPriority() < mostTightly)
		{
			maxWeightNodePq = pq.topElement();
			mostTightly = pq.topPriority();
			pq.pop();
		}
#endif
		// The loop computing the most tightly connected node to the current set \a markedNodes.
		// For better performance, this should be done using PriorityQueues! Since this algorithmn
		// is only used for the Cut-separation within the Branch&Cut-algorithmn for MCPSP, only small
		// and moderate Graph sizes are considered. Thus, the total running time is hardly affected.
		ListIterator<node> it1;
		for(it1=leftoverNodes.begin(); it1.valid(); ++it1) {

			if(nodePrio[*it1] > mostTightly) {
				maxWeightNode = *it1;
				maxWeightNodeIt = it1;
				mostTightly = nodePrio[*it1];
			}
		}
#ifdef USE_PRIO
		OGDF_ASSERT(maxWeightNode == maxWeightNodePq);
#endif

		// If the graph is not connected, maxWeightNode might not be updated in each iteration.
		// Todo: Why not? Just because priority is zero? Then we can simplify this...
		// Hence, in this case we simply choose one of the leftoverNodes (the first one).
		if (maxWeightNode == nullptr) {
			maxWeightNode = leftoverNodes.front();
			maxWeightNodeIt = leftoverNodes.begin();
		}

		// Adding \a maxWeightNode to the list \a markedNodes
		markedNodes.pushBack(maxWeightNode);

		// Deleting \a maxWeightNode from list \a leftoverNodes
		leftoverNodes.del(maxWeightNodeIt);

		// Updating the node priorities
		for(adjEntry a : maxWeightNode->adjEntries) {
			nodePrio[a->twinNode()] += m_w[a->theEdge()];
		}
#ifdef USE_PRIO
		//replaces loop above
		for(adjEntry a : maxWeightNodePq->adjEntries) {
			//should have some decreasePriorityBy instead...
			pq.decrease(a->twinNode(), pq.priority(a->twinNode()) - m_w[a->theEdge()]);
		}
#endif
	}

	// Computing value \a cutOfThePhase
	cutOfThePhase = 0.0;
	ListConstReverseIterator<node> last = markedNodes.rbegin();
	t = (*last); s = *(last.succ());
	for(adjEntry t_adj : t->adjEntries) {
		cutOfThePhase += m_w[t_adj->theEdge()];
	}

	// If the current \a cutOfThePhase is strictly smaller than the global mincut value,
	// the partition defining the mincut has to be updated.
	if(cutOfThePhase < m_minCut) {
		m_partition.clear();
		m_partition.pushBack(m_GC.original(t));
		for(node vi : m_contractedNodes[t]) {
			m_partition.pushBack(vi);
		}
	}

	// Since nodes in #m_GC correspond to sets of nodes (due to the node contraction),
	// the NodeArray #m_contractedNodes has to be updated.
	m_contractedNodes[t].pushBack(m_GC.original(s));
	for (node vi : m_contractedNodes[s]) {
		m_contractedNodes[t].pushBack(vi);
	}

	// Performing the node contraction of nodes \p s and \p t.
	contraction(t,s);

	return cutOfThePhase;
}


double MinCut::minimumCut() {

	/*
	 * Main loop of the algorithm
	 * As long as GraphCopy #m_GC contains at least two nodes,
	 * function minimumCutPhase() is invoked and #m_minCut is updated
	 */

	for (int i=m_GC.numberOfNodes(); i>1; --i) {
		Math::updateMin(m_minCut, minimumCutPhase());
		if (m_minCut == 0.0) return m_minCut;
	}
	return m_minCut;
}


void MinCut::partition(List<node> &nodes) {

	nodes.clear();
	for (node v : m_partition) {
		nodes.pushBack(v);
	}
}


void MinCut::cutEdges(List<edge> &edges, Graph &G) {

	edges.clear();
	NodeArray<bool> inPartition(G);
	inPartition.fill(false);

	for (node v : m_partition) {
		inPartition[v] = true;
	}

	for (node v : m_partition) {
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			if(e->source() == v) {
				if(!inPartition[e->target()]) {
					edges.pushBack(e);
				}
			} else {
				if(!inPartition[e->source()]) {
					edges.pushBack(e);
				}
			}
		}
	}
}

}
