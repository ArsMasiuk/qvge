/** \file
 * \brief Implementation of dual graph
 *
 * \author Michael Schulz
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

#include <ogdf/basic/DualGraph.h>

namespace ogdf{

// Computes combinatorial embedding of dual graph
// Precondition: CE must be combinatorial embedding of connected planar graph
DualGraph::DualGraph(const ConstCombinatorialEmbedding &CE) : m_primalEmbedding(CE)
{
	const Graph &primalGraph = CE.getGraph();
	init(*(new Graph));
	Graph &dualGraph = getGraph();

	m_dualNode.init(CE);
	m_dualEdge.init(primalGraph);
	m_dualFace.init(primalGraph);
#ifdef OGDF_DEBUG
	m_primalNode.init(*this, nullptr);
#else
	m_primalNode.init(*this);
#endif
	m_primalFace.init(dualGraph);
	m_primalEdge.init(dualGraph);

	// create dual nodes
	for(face f : CE.faces)
	{
		node vDual = dualGraph.newNode();
		m_dualNode[f] = vDual;
		m_primalFace[vDual] = f;
	}

	// create dual edges
	for(edge e : primalGraph.edges)
	{
		adjEntry aE = e->adjSource();
		node vDualSource = m_dualNode[CE.rightFace(aE)];
		node vDualTarget = m_dualNode[CE.leftFace(aE)];
		edge eDual = dualGraph.newEdge(vDualSource, vDualTarget);
		m_primalEdge[eDual] = e;
		m_dualEdge[e] = eDual;
	}

	// sort adjElements of every dual node corresponding to dual embedding
	for(face f : CE.faces)
	{
		node vDual = m_dualNode[f];
		List<adjEntry> newOrder;

		for(adjEntry adj : f->entries) {
			edge e = adj->theEdge();
			edge eDual = m_dualEdge[e];
			bool isSource = adj == e->adjSource();
			adjEntry adjDual = isSource ? eDual->adjSource() : eDual->adjTarget();
			newOrder.pushBack(adjDual);
		}

		dualGraph.sort(vDual, newOrder);
	}

	// calculate dual faces and links to corresponding primal nodes
	computeFaces();

	for(node v : primalGraph.nodes)
	{
		edge ePrimal = v->firstAdj()->theEdge();
		edge eDual = m_dualEdge[ePrimal];
		face fDual = rightFace(eDual->adjSource());
		if(ePrimal->source()==v)
			fDual = leftFace(eDual->adjSource());

		OGDF_ASSERT(m_primalNode[fDual] == nullptr);

		m_dualFace[v] = fDual;
		m_primalNode[fDual] = v;
	}
}

// Destructor
DualGraph::~DualGraph()
{
	clear();
	delete m_cpGraph;
}

}
