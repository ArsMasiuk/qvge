/** \file
 * \brief Implementation of class CompactionConstraintGraphBase.
 *
 * Represents base class for CompactionConstraintGraph<ATYPE>
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


#include <ogdf/orthogonal/CompactionConstraintGraph.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>


namespace ogdf {


// constructor for orthogonal representation
// builds constraint graph with basic arcs
CompactionConstraintGraphBase::CompactionConstraintGraphBase(
	const OrthoRep &OR,
	const PlanRep &PG,
	OrthoDir arcDir,
	int costGen,
	int costAssoc,
	bool align
) : CommonCompactionConstraintGraphBase(OR, PG, arcDir, costAssoc)
{
	m_verticalGen .init(PG, false);
	m_verticalArc .init(*this, false);
	m_alignmentArc.init(*this, false);
	m_pathToEdge  .init(*this, nullptr);

	m_align     = align;
	m_edgeCost[static_cast<int>(Graph::EdgeType::generalization)] = costGen;
	m_edgeCost[static_cast<int>(Graph::EdgeType::association)   ] = costAssoc;

	for(edge e : PG.edges)
	{
		if ((PG.typeOf(e) == Graph::EdgeType::generalization) && (!PG.isExpansionEdge(e)))
			m_verticalGen[e] = true;
	}

	insertPathVertices(PG);
	insertBasicArcs(PG);
}

// insert vertex for each segment
void CompactionConstraintGraphBase::insertPathVertices(const PlanRep &PG)
{
	NodeArray<node> genOpposite(PG,nullptr);

	for(node v : PG.nodes)
	{
		const OrthoRep::VertexInfoUML *vi = m_pOR->cageInfo(v);
		if (vi == nullptr || PG.typeOf(v) == Graph::NodeType::generalizationMerger) continue;

		adjEntry adjGen = vi->m_side[static_cast<int>(m_arcDir)   ].m_adjGen;
		adjEntry adjOpp = vi->m_side[static_cast<int>(m_oppArcDir)].m_adjGen;
		if (adjGen != nullptr && adjOpp != nullptr)
		{
			node v1 = adjGen->theNode();
			node v2 = adjOpp->theNode();
			genOpposite[genOpposite[v1] = v2] = v1;
		}
	}

	//TODO: hier abfragen, ob Kantensegment einer Originalkante
	//und diese hat Multikantenstatus => man muss sich die Abstaende am Rand merken
	//und auf alle Segmente anwenden

	NodeArray<bool> visited(PG,false);

	for(node v : PG.nodes)
	{
		if (!visited[v]) {
			node pathVertex = newNode();

			dfsInsertPathVertex(v, pathVertex, visited, genOpposite);

			//test for multi sep
			//if exact two nodes in path, edge between is original edge segment,
			//save this to recall the minsep for all segments later over original
			if (m_path[pathVertex].size() == 2 && m_pathToEdge[pathVertex]) {
				// XXX: muss man hier originaledge als rueckgabe reintun???
			} else {
				m_pathToEdge[pathVertex] = nullptr;
			}
		}
	}
}

// insert all graph vertices into segment pathVertex
void CompactionConstraintGraphBase::dfsInsertPathVertex(
	node v,
	node pathVertex,
	NodeArray<bool> &visited,
	const NodeArray<node> &genOpposite)
{
	visited[v] = true;
	m_path[pathVertex].pushFront(v);
	m_pathNode[v] = pathVertex;

	for(adjEntry adj : v->adjEntries)
	{
		OrthoDir dirAdj = m_pOR->direction(adj);
		OGDF_ASSERT(dirAdj != OrthoDir::Undefined);

		if (dirAdj != m_arcDir && dirAdj != m_oppArcDir) {
			//for multiedges, only useful if only one edge considered on path
			//maybe zero if no original edge exists
			if (!m_pathToEdge[pathVertex])
			{
				//only reset later for multi edge segments
				m_pathToEdge[pathVertex] = m_pPR->original(adj->theEdge());
				//used for all vertices

			}

			node w = adj->theEdge()->opposite(v);
			if (!visited[w])
				dfsInsertPathVertex(w, pathVertex, visited, genOpposite);
		}
	}

	node w = genOpposite[v];
	if (w != nullptr && !visited[w])
		dfsInsertPathVertex(w, pathVertex, visited, genOpposite);
}



//
// insert an arc for each edge with direction m_arcDir
void CompactionConstraintGraphBase::insertBasicArcs(const PlanRep &PG)
{
	const Graph &G = *m_pOR;

	for(node v : G.nodes)
	{
		node start = m_pathNode[v];

		for(adjEntry adj : v->adjEntries) {
			if (m_pOR->direction(adj) == m_arcDir) {
				edge e = newEdge(start, m_pathNode[adj->theEdge()->opposite(v)]);
				m_edgeToBasicArc[adj] = e;

				m_cost[e] = m_edgeCost[static_cast<int>(PG.typeOf(adj->theEdge()))];

				//try to pull nodes up in hierarchies
				if ( (PG.typeOf(adj->theEdge()) == Graph::EdgeType::generalization) &&
					(PG.typeOf(adj->theEdge()->target()) == Graph::NodeType::generalizationExpander) &&
					!(PG.isExpansionEdge(adj->theEdge()))
					)
				{
					if (m_align)
					{
						//got to be higher than vertexarccost*doublebendfactor
						m_cost[e] = 4000*m_cost[e]; //use parameter later corresponding
						m_alignmentArc[e] = true;
					}
					//to compconsgraph::doublebendfactor
					else m_cost[e] = 2*m_cost[e];
				}

				//set generalization type
				if (verticalGen(adj->theEdge())) m_verticalArc[e] = true;
				//set onborder
				if (PG.isDegreeExpansionEdge(adj->theEdge()))
				{
					edge borderE = adj->theEdge();
					node v1 = borderE->source();
					node v2 = borderE->target();
					m_border[e] = ((v1->degree()>2) && (v2->degree()>2) ? 2 : 1);
				}

			}
		}
	}
}

}
