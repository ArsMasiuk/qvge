/** \file
 * \brief Declaration of class Repulsion which implements an enrgy
 *        function, where the energy of the layout grows with the
 *        proximity of the vertices.
 *
 * Thus, a layout has lower energy if all vertices are far appart.
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

#pragma once

#include <ogdf/energybased/davidson_harel/NodePairEnergy.h>

namespace ogdf {
namespace davidson_harel {

class Repulsion: public NodePairEnergy {
public:
	//Initializes data structures to speed up later computations
	explicit Repulsion(GraphAttributes &AG);
private:
	//computes for two vertices an the given positions the repulsive energy
	double computeCoordEnergy(node, node, const DPoint&, const DPoint&) const override;
};

}}
