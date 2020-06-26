/** \file
 * \brief Implements planarization with grid layout.
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

#include <ogdf/planarity/PlanarizationGridLayout.h>
#include <ogdf/planarity/SubgraphPlanarizer.h>
#include <ogdf/planarlayout/MixedModelLayout.h>
#include <ogdf/packing/TileToRowsCCPacker.h>

namespace ogdf {

PlanarizationGridLayout::PlanarizationGridLayout()
{
	m_crossMin      .reset(new SubgraphPlanarizer);
	m_planarLayouter.reset(new MixedModelLayout);
	m_packer        .reset(new TileToRowsCCPacker);

	m_pageRatio = 1.0;
}


void PlanarizationGridLayout::doCall(
	const Graph &G,
	GridLayout &gridLayout,
	IPoint &bb)
{
	m_nCrossings = 0;
	if(G.empty()) return;

	PlanRep pr(G);

	const int numCC = pr.numberOfCCs();
	// (width,height) of the layout of each connected component
	Array<IPoint> boundingBox(numCC);

	for(int cc = 0; cc < numCC; ++cc)
	{
		// 1. crossing minimization
		int cr;
		m_crossMin->call(pr, cc, cr);
		m_nCrossings += cr;
		OGDF_ASSERT(isPlanar(pr));

		GridLayout gridLayoutPG(pr);
		m_planarLayouter->callGrid(pr,gridLayoutPG);

		// copy grid layout of PG into grid layout of G
		for(int j = pr.startNode(); j < pr.stopNode(); ++j)
		{
			node vG = pr.v(j);

			gridLayout.x(vG) = gridLayoutPG.x(pr.copy(vG));
			gridLayout.y(vG) = gridLayoutPG.y(pr.copy(vG));

			for(adjEntry adj : vG->adjEntries) {
				if ((adj->index() & 1) == 0) continue;
				edge eG = adj->theEdge();
				IPolyline &ipl = gridLayout.bends(eG);
				ipl.clear();

				bool firstTime = true;
				for(edge e : pr.chain(eG)) {
					if(!firstTime) {
						node v = e->source();
						ipl.pushBack(IPoint(gridLayoutPG.x(v),gridLayoutPG.y(v)));
					} else
						firstTime = false;
					ipl.conc(gridLayoutPG.bends(e));
				}
			}
		}

		boundingBox[cc] = m_planarLayouter->gridBoundingBox();
		boundingBox[cc].m_x += 1; // one row/column space between components
		boundingBox[cc].m_y += 1;
	}

	Array<IPoint> offset(numCC);
	m_packer->call(boundingBox,offset,m_pageRatio);

	bb.m_x = bb.m_y = 0;
	for(int cc = 0; cc < numCC; ++cc)
	{
		const int dx = offset[cc].m_x;
		const int dy = offset[cc].m_y;

		if(boundingBox[cc].m_x + dx > bb.m_x)
			bb.m_x = boundingBox[cc].m_x + dx;
		if(boundingBox[cc].m_y + dy > bb.m_y)
			bb.m_y = boundingBox[cc].m_y + dy;

		// iterate over all nodes in i-th cc
		for(int j = pr.startNode(cc); j < pr.stopNode(cc); ++j)
		{
			node vG = pr.v(j);

			gridLayout.x(vG) += dx;
			gridLayout.y(vG) += dy;

			for(adjEntry adj : vG->adjEntries) {
				if ((adj->index() & 1) == 0) continue;
				edge eG = adj->theEdge();

				for(IPoint &ip : gridLayout.bends(eG)) {
					ip.m_x += dx;
					ip.m_y += dy;
				}
			}
		}
	}

	bb.m_x -= 1; // remove margin of topmost/rightmost box
	bb.m_y -= 1;
}

}
