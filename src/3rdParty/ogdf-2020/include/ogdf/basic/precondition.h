/** \file
 * \brief Declaration of functions for drawing module precondition
 *        handling.
 *
 * \author Karsten Klein
 *
 * \attention This is legacy code from UML class diagram handling,
 * and it should be checked if it is still required.
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

#include <ogdf/orthogonal/EdgeRouter.h>
#include <ogdf/uml/UMLGraph.h>


namespace ogdf {

//descent the hierarchy tree at "sink" v recursively
bool dfsGenTreeRec(
	UMLGraph& UG,
	EdgeArray<bool> &used,
	NodeArray<int> &hierNumber, //number of hierarchy tree
	// A node is visited if its hierNumber != 0
	int hierNum,
	node v,
	List<edge>& fakedGens, //temporary
	bool fakeTree)
{
	OGDF_ASSERT(hierNumber[v] == 0);
	hierNumber[v] = hierNum;

	bool returnValue = true;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if (e->source() == v) continue;
		if (!(UG.type(e) == Graph::EdgeType::generalization)) continue;
		if (used[e]) continue; //error ??
		used[e] = true;

		node w = e->opposite(v);

		if (hierNumber[w]) {
			//temporarily fake trees
#if 0
			if (hierNumber[w] == hierNum) //forward search edge
#endif
			if (fakeTree)
			{
#if 0
				UG.type(e) = Graph::association;
#endif
				fakedGens.pushBack(e);
				continue;
			}
			else return false;//reached w over unused edge => no tree
		}

		returnValue = dfsGenTreeRec(UG, used, hierNumber, hierNum, w, fakedGens, fakeTree);
		//shortcut
		if (!returnValue) return false;
	}

	return returnValue;
}

edge firstOutGen(UMLGraph& UG, node v, EdgeArray<bool>& /* used */)
{
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if (e->target() == v) continue;
		if (UG.type(e) == Graph::EdgeType::generalization)
		{
#if 0
			OGDF_ASSERT(!used[e]);
#endif
			return e;
		}
		else continue;
	}
	return nullptr;
}

bool dfsGenTree(
	UMLGraph& UG,
	List<edge>& fakedGens,
	bool fakeTree)
{
	EdgeArray<bool> used(UG.constGraph(), false);
#if 0
	NodeArray<bool> visited(UG,false);
#endif
	NodeArray<int>  hierNumber(UG.constGraph(), 0);

	int hierNum = 0; //number of hierarchy tree

	const Graph& G = UG.constGraph();
	for(edge e : G.edges)
	{
		//descent in the hierarchy containing e
		if (!used[e] && UG.type(e) == Graph::EdgeType::generalization)
		{
			hierNum++; //current hierarchy tree
			//first we search for the sink
			node sink = e->target();
			edge sinkPath = firstOutGen(UG, e->target(), used);
			int cycleCounter = 0;
			while (sinkPath)
			{
				sink = sinkPath->target();
				sinkPath = firstOutGen(UG, sinkPath->target(), used);
				cycleCounter++;
				//if there is no sink, convert Generalizations to Associations and draw
				if (cycleCounter > G.numberOfEdges())
				{
					UG.type(sinkPath) = Graph::EdgeType::association;
					fakedGens.pushBack(sinkPath);
					sink = sinkPath->source();
					sinkPath = nullptr;
				}
			}

			//now sink is the hierarchy sink

			//used is set in dfsGenTreeRec
			bool isTree = dfsGenTreeRec(UG, used, hierNumber, hierNum, sink, fakedGens, fakeTree);
			if (!isTree) return false;
		}

	}

	return true;
}

}
