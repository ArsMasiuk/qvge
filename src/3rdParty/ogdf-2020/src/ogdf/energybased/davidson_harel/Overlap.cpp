/** \file
 * \brief Implementation of class Overlap
 *
 * \author Rene Weiskircher
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


#include <ogdf/energybased/davidson_harel/Overlap.h>

namespace ogdf {
namespace davidson_harel {

Overlap::Overlap(GraphAttributes &AG) : NodePairEnergy("Overlap",AG){}

double Overlap::computeCoordEnergy(node v1, node v2, const DPoint &p1, const DPoint &p2)
	const
{
	DIntersectableRect i1(shape(v1)), i2(shape(v2));
	i1.move(p1);
	i2.move(p2);
	DIntersectableRect intersection = i1.intersection(i2);
	double area = intersection.area();
	if(area < 0.0) {
		OGDF_ASSERT(area > -0.00001);
		area = 0.0;
	}
	double minArea = min(i1.area(),i2.area());
	double energy = area / minArea;
	return energy;
}

}}
