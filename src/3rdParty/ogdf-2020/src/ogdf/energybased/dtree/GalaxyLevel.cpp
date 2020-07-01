/** \file
 * \brief DTreeStuff
 *
 * \author Martin Gronemann
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

#include <ogdf/energybased/dtree/GalaxyLevel.h>
#include <ogdf/basic/simple_graph_alg.h>

using namespace ogdf;
using namespace ogdf::energybased::dtree;

// public constructor for creating a coarser level
GalaxyLevel::GalaxyLevel(const Graph& graph)
{
	// sets the input graph by some not so nice cast
	// but we are not changing it anyways so it is fine
	m_pGraph = const_cast<Graph*>(&graph);

	// set the next finer to the input
	m_pNextFiner = nullptr;

	// this is the coarsest so far
	m_pNextCoarser = nullptr;

	// init the node data
	m_nodeWeight.init(*m_pGraph, 1.0);

	// init the parent pointer node
	m_parent.init(*m_pGraph, nullptr);

	// init the edge weight with 1.0
	m_edgeWeight.init(*m_pGraph, 1.0);
}

// private constructor for creating a coarser level
GalaxyLevel::GalaxyLevel(GalaxyLevel* pNextFiner)
{
	// create a new empty graph
	m_pGraph = new Graph();

	// set the next finer to the input
	m_pNextFiner = pNextFiner;

	// this is the coarsest so far
	m_pNextCoarser = nullptr;

	// and link the other way around
	m_pNextFiner->m_pNextCoarser = this;

	// init the node data
	m_nodeWeight.init(*m_pGraph, 0.0);

	// init the parent pointer node
	m_parent.init(*m_pGraph, nullptr);

	// init the edge weight with 0.0 for summming up
	m_edgeWeight.init(*m_pGraph, 0.0);
}

GalaxyLevel::~GalaxyLevel()
{
	// delete the rest of the chain
	delete m_pNextCoarser;

	// if this is not the original graph
	if (m_pNextFiner) {
		// set this pointer to 0 in case someone
		// is deleting in the middle of the chain
		m_pNextFiner->m_pNextCoarser = nullptr;

		// delete the graph
		delete m_pGraph;
	}
}

struct SunWeightComparer
{
	// constructor with a node array for the weight
	explicit SunWeightComparer(const NodeArray<double>& weight) : m_weight(weight) { }

	// compares the two weights
	bool operator()(node a, node b) const { return m_weight[a] < m_weight[b]; }

	// the node array
	const NodeArray<double>& m_weight;
};

GalaxyLevel* GalaxyLevel::buildNextCoarserLevel(int numLabels)
{
	// make sure the graph is connected
	OGDF_ASSERT(isConnected(*m_pGraph));

	// keeps for each node the estimated sunweight
	NodeArray<double> sunWeight(*m_pGraph, 0.0);

	// iterate over all nodes
	for (node v = m_pGraph->firstNode(); v; v = v->succ()) {
		// init with the nodes weight
		sunWeight[v] = m_nodeWeight[v];

		// and add all adjacent weights
		for (adjEntry adj = v->firstAdj(); adj; adj = adj->succ()) {
			// sum up the weight
			sunWeight[v] += m_nodeWeight[adj->twinNode()];
		}
	}

	// array of nodes for sorting them by sunweight
	Array<node> sortedOrder(m_pGraph->numberOfNodes());
	m_pGraph->allNodes(sortedOrder);

	// shuffle them to avoid artifacts from the initial order
	std::random_shuffle(sortedOrder.begin(), sortedOrder.end());

	// sort them, we want the suns with low weight
	std::sort(sortedOrder.begin(), sortedOrder.end(), SunWeightComparer(sunWeight));

	// the labels
	NodeArray<int> label(*m_pGraph, numLabels);

	// the sun of a node in the same graph
	NodeArray<node> sun(*m_pGraph, nullptr);

	// list of suns
	List<node> sunList;

	// while there is something to do
	for (int i  = 0; i < m_pGraph->numberOfNodes(); i++) {
		// get a node
		node s = sortedOrder[i];

		// check if this is labeled
		if (label[s] < numLabels)
			continue;

		// a nice queue
		List<node> Q;

		// mark it as sun
		label[s] = 0;

		// the sun of s is s
		sun[s] = s;

		// enqueue it
		Q.pushBack(s);

		// save it as a sun
		sunList.pushBack(s);

		// while there are nodes to label
		while (!Q.empty()) {
			// pop a labeled node from the queue
			node u = Q.popFrontRet();

			// the label for the next nodes
			int newLabel = label[u] + 1;

			// if we reached the limit do nothing
			if (newLabel >= numLabels)
				continue;

			// label all adjacent nodes that are closer
			for (adjEntry adj = u->firstAdj(); adj; adj = adj->succ()) {
				// the adjacent one
				node v = adj->twinNode();

				// check if we can relabel
				if (label[v] > newLabel) {
					// set the new label
					label[v] = newLabel;

					// copy sun pointer
					sun[v] = sun[u];

					// enquee the new node
					Q.pushBack(v);
				}
			}
		}
	};

	// the new level to create
	GalaxyLevel* pNewLevel = new GalaxyLevel(this);

	// for all suns
	for (node v : sunList) {
		// create a copy on the next level
		node v_new = pNewLevel->m_pGraph->newNode();

		// set the parent of this sun
		m_parent[v] = v_new;
	}

	// now for all nodes (notice that we will visit suns too)
	for (node v = m_pGraph->firstNode(); v; v = v->succ()) {
		// the sun of v
		node s = sun[v];

		// the parent of the sun
		node s_parent = parent(s);

		// add the weight to the new node
		pNewLevel->m_nodeWeight[s_parent] += m_nodeWeight[v];

		// the parent is the parent of the sun
		m_parent[v] = s_parent;
	}

	// for all edges
	for (edge e = m_pGraph->firstEdge(); e; e = e->succ()) {
		// get the parents of the two endpoints
		node s_new = parent(e->source());
		node t_new = parent(e->target());

		// check if parents are the same node
		if (s_new == t_new)
			continue;

		// create a new edge
		edge e_new = pNewLevel->m_pGraph->newEdge(s_new, t_new);

		// save the old edge weight
		pNewLevel->m_edgeWeight[e_new] = m_edgeWeight[e];
	}

	// removes all parallel edges and sum up weights
	pNewLevel->removeParEdgesWithWeight();

	// returns the new level
	return pNewLevel;
}

void GalaxyLevel::removeParEdgesWithWeight()
{
	// keeps for each node the adj element from where we visited it
	NodeArray<adjEntry> visitedFrom(*m_pGraph, nullptr);

	// loop over all nodes
	for (node v = m_pGraph->firstNode(); v; v = v->succ()) {
		// the edges we have to del at the end
		List<edge> toDel;

		// for all adjacent nodes
		for (adjEntry adj = v->firstAdj(); adj; adj = adj->succ()) {
			// the adjacent node
			node w = adj->twinNode();

			// check if we visited and from where
			if (visitedFrom[w] && (visitedFrom[w]->theNode() == v)) {
				// we visited from v already, its a parallel edge
				edge e = visitedFrom[w]->theEdge();

				// add the edge weight to the parallel edge
				m_edgeWeight[e] += m_edgeWeight[adj->theEdge()];

				// and mark so we remove it later
				toDel.pushBack(adj->theEdge());
			} else {
				// not visited from v, save the info from where we found w
				visitedFrom[w] = adj;
			}
		}

		// remove edges that we marked
		while (!toDel.empty()) {
			m_pGraph->delEdge(toDel.popFrontRet());
		}
	}
}

GalaxyLevel* GalaxyLevel::buildLevelsUntil(int maxNumNodes)
{
	// we start with this
	GalaxyLevel* pLevel = this;

	// fwd in case there are already levels created
	while (pLevel->nextCoarser()) {
		// go to the next
		pLevel = pLevel->nextCoarser();
	}

	// while the curr level is too big
	while (pLevel->graph().numberOfNodes() > maxNumNodes) {
		// build a coarser one
		pLevel = pLevel->buildNextCoarserLevel();
	}

	// return the coarsest level
	return pLevel;
}

// returns the graph
const Graph& GalaxyLevel::graph() const
{
	return *m_pGraph;
}

// returns the parent node of a node on the coarser level
node GalaxyLevel::parent(node v) const
{
	return m_parent[v];
}

// returns the weight of a node
double GalaxyLevel::weight(node v) const
{
	return m_nodeWeight[v];
}

// returns the weight of a node
double GalaxyLevel::edgeWeight(edge e) const
{
	return m_edgeWeight[e];
}

// sets the weight of a node
void GalaxyLevel::setWeight(node v, double weight)
{
	m_nodeWeight[v] = weight;
}

// sets the edge weight of e
void GalaxyLevel::setEdgeWeight(edge e, double weight)
{
	m_edgeWeight[e] = weight;
}

// returns true if this is the level of the original graph
bool GalaxyLevel::isFinestLevel() const
{
	return m_pNextFiner == nullptr;
}

// returns true if this is coarsest level in the chain
bool GalaxyLevel::isCoarsestLevel() const
{
	return m_pNextCoarser == nullptr;
}

// return the next coarser one
GalaxyLevel* GalaxyLevel::nextCoarser()
{
	return m_pNextCoarser;
}

// return the next finer one
GalaxyLevel* GalaxyLevel::nextFiner()
{
	return m_pNextFiner;
}
