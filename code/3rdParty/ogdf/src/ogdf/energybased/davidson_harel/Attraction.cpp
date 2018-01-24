/** \file
 * \brief Implements class Attraction.
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

#include <ogdf/energybased/davidson_harel/Attraction.h>

namespace ogdf {
namespace davidson_harel {

const double Attraction::MULTIPLIER = 2.0;


//initializes internal data, like name and layout
Attraction::Attraction(GraphAttributes &AG) : NodePairEnergy("Attraction", AG) {

	reinitializeEdgeLength(MULTIPLIER);
}


//computes preferred edge length as the average of all widths and heights of the vertices
//multiplied by the multiplier
void Attraction::reinitializeEdgeLength(double multi)
{
	double lengthSum(0.0);
	for (node v : m_G.nodes) {
		const DIntersectableRect &i = shape(v);
		lengthSum += i.width();
		lengthSum += i.height();
	}
	lengthSum /= (2 * m_G.numberOfNodes());
	// lengthSum is now the average of all lengths and widths
	m_preferredEdgeLength = multi * lengthSum;

}

//the energy of a pair of vertices is computed as the square of the difference between the
//actual distance and the preferred edge length
double Attraction::computeCoordEnergy(node v1, node v2, const DPoint &p1, const DPoint &p2)
const
{
	double energy = 0.0;
	if(adjacent(v1,v2)) {
		DIntersectableRect i1(shape(v1)), i2(shape(v2));
		i1.move(p1);
		i2.move(p2);
		energy = i1.distance(i2) - m_preferredEdgeLength;
		energy *= energy;
	}
	return energy;
}


#ifdef OGDF_DEBUG
void Attraction::printInternalData() const {
	NodePairEnergy::printInternalData();
	std::cout << "\nPreferred edge length: " << m_preferredEdgeLength;
}
#endif

}}
