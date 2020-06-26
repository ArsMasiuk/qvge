/** \file
 * \brief Declaration of doubly linked lists and iterators
 *
 * \author Gereon Bartel
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
#include <ogdf/basic/geometry.h>
#include <ogdf/energybased/multilevel_mixer/MultilevelGraph.h>
#include <vector>

namespace ogdf {

//! Computes the convex hull of a set of points or a layout.
/**
 * @ingroup graph-algs
 *
 * All returned polygons are clockwise (cw).
 */
class OGDF_EXPORT ConvexHull {
private:
	bool sameDirection(const DPoint &start, const DPoint &end, const DPoint &s, const DPoint &e) const;

	// calculates a convex hull very quickly but only works with cross-free Polygons!
	DPolygon conv(const DPolygon &poly) const;

	// Calculates the Part of the convex hull that is left of line start-end
	// /a points should only contain points that really are left of the line.
	void leftHull(std::vector<DPoint> points, DPoint &start, DPoint &end, DPolygon &hullPoly) const;


public:
	ConvexHull();
	~ConvexHull();

	DPoint calcNormal(const DPoint &start, const DPoint &end) const;
	double leftOfLine(const DPoint &normal, const DPoint &point, const DPoint &pointOnLine) const;

	DPolygon call(std::vector<DPoint> points) const;
	DPolygon call(GraphAttributes &GA) const;
	DPolygon call(MultilevelGraph &MLG) const;
};

}
