/** \file
 * \brief Handling of clique replacement in planarization layout.
 *
 * \author Carsten Gutwenger, Karsten Klein
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

#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/SList.h>

namespace ogdf {
namespace planarization_layout {

class OGDF_EXPORT CliqueReplacer {

	Graph &m_G;
	GraphAttributes &m_ga;
	Graph::HiddenEdgeSet m_hiddenEdges;

	double m_cliqueCenterSize; //default size of inserted clique replacement center nodes
	SListPure<node> m_centerNodes; //center nodes introduced at clique replacement

	EdgeArray<bool> m_replacementEdge; //used to mark clique replacement edges may be we can join this with edge type

	NodeArray<DRect> m_cliqueCircleSize; //save the bounding box size of the circular drawing of the clique at center

	NodeArray<DPoint> m_cliqueCirclePos; //save the position of the node in the circular drawing of the clique

public:
	CliqueReplacer(GraphAttributes &ga, Graph &G);

	// replace (dense) subgraphs given in list clique by
	// inserting a center node connected to each node (=>star)
	// and deleting all edges between nodes in clique
	// returns center node
	void replaceByStar(List<List<node>*> &cliques);

	// undo clique replacements
	void undoStars();

	// boolean switches restore of all hidden edges in single clique call
	void undoStar(node center, bool restoreAllEdges);

	//returns the size of a circular drawing for a clique around center v
	DRect cliqueRect(node v) const
	{
		return m_cliqueCircleSize[v];
	}
	DPoint cliquePos(node v) const
	{
		return m_cliqueCirclePos[v];
	}

	//compute circle positions for all nodes around center
	//using the ordering given in this UMLGraph, calls
	//ccP(List...)
	//rectMin is a temporary solution until compaction with constraints allows stretching
	//of rect to clique size, it gives the min(w,h) of the given fixed size rect around the clique
#if 1
	void computeCliquePosition(node center, double rectMin);
#else
	void computeCliquePosition(node center, double rectMin, const adjEntry &startAdj);
#endif

	//compute positions for the nodes in adjNodes on a circle
	//tries to keep the relative placement of the nodes in the clique
	//rectangle (left, right,...) to avoid clique crossings of outgoing edges
	void computeCliquePosition(List<node> &adjNodes, node center, double rectMin = -1.0);

	const SListPure<node> &centerNodes() { return m_centerNodes; }

	//default size of inserted clique replacement center nodes
	void setDefaultCliqueCenterSize(double i) { m_cliqueCenterSize = max(i, 1.0); }

	double getDefaultCliqueCenterSize() { return m_cliqueCenterSize; }

	//! returns true if edge was inserted during clique replacement
	bool isReplacement(edge e)
	{
		// TODO: check here how to guarantee that value is defined,
		// edgearray is only valid if there are cliques replaced
		return m_replacementEdge[e];
	}

private:
	node replaceByStar(List<node> &clique, NodeArray<int> &cliqueNum);
	DRect circularBound(node center);
};

}
}
