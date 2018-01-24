/** \file
 * \brief Offers variety of possible algorithm calls for simultaneous
 * drawing.
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

#include<ogdf/simultaneous/SimDrawCaller.h>
#include<ogdf/layered/SugiyamaLayout.h>
#include<ogdf/planarity/PlanarizationLayout.h>
#include<ogdf/planarity/SubgraphPlanarizer.h>
#include<ogdf/planarity/VariableEmbeddingInserter.h>


namespace ogdf {

// refreshes m_esg
void SimDrawCaller::updateESG()
{
	for(edge e : m_G->edges)
		(*m_esg)[e] = m_GA->subGraphBits(e);
}

// Constructor
SimDrawCaller::SimDrawCaller(SimDraw &SD) : SimDrawManipulatorModule(SD)
{
	m_esg = new EdgeArray<uint32_t>(*m_G);
	updateESG();
}

// call for SugiyamaLayout
void SimDrawCaller::callSugiyamaLayout()
{
	m_SD->addAttribute(GraphAttributes::nodeGraphics);
	m_SD->addAttribute(GraphAttributes::edgeGraphics);

	// nodes get default size
	for(node v : m_G->nodes)
		m_GA->height(v) = m_GA->width(v) = 5.0;

	// actual call of SugiyamaLayout
	updateESG();
	SugiyamaLayout SL;
	SL.setSubgraphs(m_esg); // needed to call SimDraw mode
	SL.call(*m_GA);
}

// call for PlanarizationLayoutUML
void SimDrawCaller::callPlanarizationLayout()
{
	m_SD->addAttribute(GraphAttributes::nodeGraphics);
	m_SD->addAttribute(GraphAttributes::edgeGraphics);

	// nodes get default size
	for(node v : m_G->nodes)
		m_GA->height(v) = m_GA->width(v) = 5.0;

	// actual call on PlanarizationLayout
	PlanarizationLayout PL;
	PL.callSimDraw(*m_GA);
}

// call for SubgraphPlanarizer
// returns crossing number
int SimDrawCaller::callSubgraphPlanarizer(int cc, int numberOfPermutations)
{
	// transfer edge costs if existent
	EdgeArray<int> ec(*m_G, 1);
	if(m_GA->has(GraphAttributes::edgeIntWeight))
	{
		for(edge e : m_G->edges)
			ec[e] = m_GA->intWeight(e);
	}

	// initialize
	updateESG();
	int crossNum = 0;
	PlanRep PR(*m_G);

	// actual call for connected component cc
	SubgraphPlanarizer SP;
	VariableEmbeddingInserter* vei = new VariableEmbeddingInserter;
	vei->removeReinsert(RemoveReinsertType::Incremental);
	SP.setInserter(vei);
	SP.permutations(numberOfPermutations);
	SP.call(PR, cc, crossNum, &ec, nullptr, m_esg);

	// insert all dummy nodes into original graph *m_G
	NodeArray<node> newOrigNode(PR);
	for(node vPR : PR.nodes)
	{
		if(PR.isDummy(vPR))
		{
			node vOrig = m_G->newNode();
			newOrigNode[vPR] = vOrig;
			m_SD->isDummy(vOrig) = true;
		}
		else
			newOrigNode[vPR] = PR.original(vPR);
		//original nodes are saved
	}

	// insert all edges incident to dummy nodes into *m_G
	EdgeArray<bool> toBeDeleted(*m_G, false);
	EdgeArray<bool> visited(PR, false);
	for(node vPR : PR.nodes)
	{
		if(PR.isDummy(vPR))
		{
			node vNewOrig = newOrigNode[vPR]; //lebt in *m_G
			//lebt in PR
			for(adjEntry adj : vPR->adjEntries) {
				edge e = adj->theEdge();
				if(!visited[e])
				{
					node w = e->opposite(vPR); //lebt in PR
					node wNewOrig = newOrigNode[w]; //lebt in *m_G
					edge eNewOrig = m_G->newEdge(vNewOrig,wNewOrig);
					m_GA->subGraphBits(eNewOrig) = m_GA->subGraphBits(PR.original(e));
					toBeDeleted[PR.original(e)] = true;
					visited[e] = true;
				}
			}
		}
	}

	// delete all old edges in *m_G that are replaced by dummy node edges
	List<edge> LE;
	m_G->allEdges(LE);
	for(edge e : LE)
	{
		if(toBeDeleted[e])
			m_G->delEdge(e);
	}

	return crossNum;
}

}
