/** \file
 * \brief implementation of class Layout
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

#include <ogdf/basic/Layout.h>
#include <ogdf/planarity/PlanRep.h>


namespace ogdf {


void Layout::computePolyline(GraphCopy &GC, edge eOrig, DPolyline &dpl) const
{
	dpl.clear();

	const List<edge> &edgePath = GC.chain(eOrig);

	// The corresponding edge path in the copy must contain at least 1 edge!
	OGDF_ASSERT(edgePath.size() >= 1);

	// iterate over all edges in the corresponding edge path in the copy
	bool firstTime = true;
	for (edge e : edgePath) {
		node v = e->source();

		// append point of source node of e ...
		if (!firstTime)
			dpl.pushBack(DPoint(m_x[v], m_y[v]));
		else
			firstTime = false;

		// ... and polyline of e
		const DPolyline &segment = m_bends[e];

		for (const DPoint &dp : segment)
			dpl.pushBack(dp);
	}
}


// faster version of computePolylineClear
// clears the list of bend points  of all edges in the edge path
// in the copy corresponding to eOrig!
void Layout::computePolylineClear(PlanRep &PG, edge eOrig, DPolyline &dpl)
{
	dpl.clear();

	const List<edge> &edgePath = PG.chain(eOrig);

	// The corresponding edge path in the copy must contain at least 1 edge!
	OGDF_ASSERT(edgePath.size() >= 1);

	// iterate over all edges in the corresponding edge path in the copy
	bool firstTime = true;
	for (edge e : edgePath) {
		node v = e->source();

		// append point of source node of e ...
		if (!firstTime)
			dpl.pushBack(DPoint(m_x[v], m_y[v]));
		else
			firstTime = false;

		// ... and polyline of e
		dpl.conc(m_bends[e]);
	}
	node w = edgePath.back()->target();
	if (PG.typeOf(w) == Graph::NodeType::generalizationExpander)
		dpl.pushBack(DPoint(m_x[w], m_y[w]));
}


DPoint Layout::computeBoundingBox(PlanRep &PG) const
{
	if (PG.numberOfNodes() == 0) {
		return DPoint(0,0);
	}

	double maxX = std::numeric_limits<double>::lowest();
	double maxY = std::numeric_limits<double>::lowest();
	double minX = std::numeric_limits<double>::max();
	double minY = std::numeric_limits<double>::max();

	// check rightmost and uppermost extension of all (original) nodes
	for(int i = PG.startNode(); i < PG.stopNode(); ++i) {
		node vG = PG.v(i);
		Math::updateMax(maxX, x(PG.copy(vG)) + PG.widthOrig(vG)/2);
		Math::updateMax(maxY, y(PG.copy(vG)) + PG.heightOrig(vG)/2);
		Math::updateMin(minX, x(PG.copy(vG)) - PG.widthOrig(vG)/2);
		Math::updateMin(minY, y(PG.copy(vG)) - PG.heightOrig(vG)/2);

		// check polylines of all (original) edges
		for(adjEntry adj : vG->adjEntries) {
			if ((adj->index() & 1) == 0) continue;
			edge eG = adj->theEdge();

			for(edge e : PG.chain(eG)) {
				// we have to check (only) all interior points, i.e., we can
				// omitt the first and last point since it lies in the box of
				// the source or target node.
				// This version checks also the first for simplicity of the loop.
				node v = e->source();
				Math::updateMax(maxX, x(v));
				Math::updateMax(maxY, y(v));
				Math::updateMin(minX, x(v));
				Math::updateMin(minY, y(v));

				for(const DPoint &dp : bends(e)) {
					Math::updateMax(maxX, dp.m_x);
					Math::updateMax(maxY, dp.m_y);
					Math::updateMin(minX, dp.m_x);
					Math::updateMin(minY, dp.m_y);
				}
			}
		}
	}

	return DPoint(maxX - minX, maxY - minY);
}

}
