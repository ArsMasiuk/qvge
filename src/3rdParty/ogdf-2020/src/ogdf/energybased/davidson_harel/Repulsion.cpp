/** \file
 * \brief Implementation of class Repulsion
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

#include <ogdf/energybased/davidson_harel/Repulsion.h>

namespace ogdf {
namespace davidson_harel {

Repulsion::Repulsion(GraphAttributes &AG) : NodePairEnergy("Repulsion",AG) { }

double Repulsion::computeCoordEnergy(
	node v1,
	node v2,
	const DPoint &p1,
	const DPoint &p2)
	const
{
	double energy = 0;
	if(!adjacent(v1,v2)) {
		DIntersectableRect i1 = shape(v1);
		DIntersectableRect i2 = shape(v2);
		i1.move(p1);
		i2.move(p2);
		double dist = i1.distance(i2);
		OGDF_ASSERT(dist >= 0.0);
		double div = (dist+1.0)*(dist+1.0);
		energy = 1.0/div;
	}
	return energy;
}

}}
