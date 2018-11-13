/** \file
 * \brief Implementation of class CrossingStructure.
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


#include <ogdf/planarity/embedder/CrossingStructure.h>

namespace ogdf {
namespace embedder {

void CrossingStructure::init(PlanRepLight &PG, int weightedCrossingNumber)
{
	m_weightedCrossingNumber = weightedCrossingNumber;
	m_crossings.init(PG.original());

	m_numCrossings = 0;
	NodeArray<int> index(PG,-1);
	for(node v : PG.nodes)
		if(PG.isDummy(v))
			index[v] = m_numCrossings++;

	for(edge ePG : PG.edges)
	{
		if(PG.original(ePG->source()) != nullptr) {
			edge e = PG.original(ePG);
			ListConstIterator<edge> it = PG.chain(e).begin();
			for(++it; it.valid(); ++it) {
				m_crossings[e].pushBack(index[(*it)->source()]);
			}
		}
	}
}

void CrossingStructure::restore(PlanRep &PG, int cc)
{
	Array<node> id2Node(0,m_numCrossings-1,nullptr);

	SListPure<edge> edges;
	PG.allEdges(edges);

	for(edge ePG : edges)
	{
		edge e = PG.original(ePG);

		for(int i : m_crossings[e])
		{
			node x = id2Node[i];
			edge ePGOld = ePG;
			ePG = PG.split(ePG);
			node y = ePG->source();

			if(x == nullptr) {
				id2Node[i] = y;
			} else {
				PG.moveTarget(ePGOld, x);
				PG.moveSource(ePG, x);
				PG.delNode(y);
			}
		}
	}
}

}
}
