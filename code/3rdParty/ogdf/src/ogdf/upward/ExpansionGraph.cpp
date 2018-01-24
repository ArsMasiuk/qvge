/** \file
 * \brief Implements class ExpansionGraph
 *
 * \author Carsten Gutwenger
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


#include <ogdf/upward/ExpansionGraph.h>
#include <ogdf/basic/simple_graph_alg.h>


namespace ogdf {


// constructor
// computes biconnected componets of original graph
// does not create a copy graph
ExpansionGraph::ExpansionGraph(const Graph &G) :
	m_compNum(G), m_adjComponents(G), m_vCopy(G,nullptr)
{
	m_vOrig.init(*this,nullptr);
	m_vRep .init(*this,nullptr);
	m_eOrig.init(*this,nullptr);

	// compute biconnected components
	int numComp = biconnectedComponents(G,m_compNum);

	// for each component, build list of contained edges
	m_component.init(numComp);

	for(edge e : G.edges)
		m_component[m_compNum[e]].pushBack(e);

	// for each vertex v, build list of components containing v
	for(int i = 0; i < numComp; ++i)
	{
		NodeArray<bool> isContained(G, false);

		for(edge e : m_component[i])
		{
			node v = e->source();
			if (!isContained[v]) {
				isContained[v] = true;
				m_adjComponents[v].pushBack(i);
			}

			v = e->target();
			if (!isContained[v]) {
				isContained[v];
				m_adjComponents[v].pushBack(i);
			}
		}
	}
}


// builds expansion graph of i-th biconnected component of the original graph
void ExpansionGraph::init(int i)
{
	OGDF_ASSERT(0 <= i);
	OGDF_ASSERT(i <= m_component.high());

	// remove previous component
	for(node v : nodes) {
		node vOrig = m_vOrig[v];
		if (vOrig)
			m_vCopy[vOrig] = nullptr;
	}
	clear();


	// create new component
	for (edge e: m_component[i]) {
		edge eCopy = newEdge(getCopy(e->source()),getCopy(e->target()));
		m_eOrig[eCopy] = e;
	}

	// expand vertices
	for(node v : nodes)
	{
		if (original(v) && v->indeg() >= 1 && v->outdeg() >= 1) {
			node vPrime = newNode();
			m_vRep[vPrime] = m_vOrig[v];

			SListPure<edge> edgeList;
			v->outEdges(edgeList);

			for (edge e: edgeList) {
				moveSource(e,vPrime);
			}

			newEdge(v,vPrime);
		}
	}
}


// builds expansion graph of graph G
// for debugging purposes only
void ExpansionGraph::init(const Graph &G)
{
	// remove previous component
	for(node v : nodes) {
		node vOrig = m_vOrig[v];
		if (vOrig)
			m_vCopy[vOrig] = nullptr;
	}
	clear();


	// create new component
	for(node v : G.nodes)
		getCopy(v);

	for(edge e : G.edges)
	{
		edge eCopy = newEdge(getCopy(e->source()),getCopy(e->target()));
		m_eOrig[eCopy] = e;
	}

	// expand vertices
	for(node v : nodes)
	{
		if (original(v) && v->indeg() >= 1 && v->outdeg() >= 1) {
			node vPrime = newNode();

			SListPure<edge> edgeList;
			v->outEdges(edgeList);

			SListConstIterator<edge> it;
			for(it = edgeList.begin(); it.valid(); ++it)
				moveSource(*it,vPrime);

			newEdge(v,vPrime);
		}
	}
}

}
