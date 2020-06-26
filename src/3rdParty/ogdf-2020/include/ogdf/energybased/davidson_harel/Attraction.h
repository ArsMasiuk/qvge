/** \file
 * \brief Declares class Attraction.
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

//! Energy function for attraction between two adjacent vertices.
/**
 * Implements an energy function that simulates
 * attraction between two adjacent vertices. There is an optimum
 * distance where the energy is zero. The energy grows quadratic
 * with the difference to the optimum distance. The optimum
 * distance between two adjacent vertices depends on the size of
 * the two vertices.
 */
class Attraction: public NodePairEnergy {
public:
	//Initializes data structures to speed up later computations
	explicit Attraction(GraphAttributes &AG);
	~Attraction() {}
	//! set the preferred edge length
	void setPreferredEdgelength(double length) {m_preferredEdgeLength = length;}
	//! set multiplier for the edge length with repspect to node size to multi
	void reinitializeEdgeLength(double multi);
#ifdef OGDF_DEBUG
	void printInternalData() const override;
#endif
private:
	//! Average length and height of nodes is multiplied by this factor to get preferred edge length
	static const double MULTIPLIER;
	//! the length that that all edges should ideally have
	double m_preferredEdgeLength;
	//! computes the energy contributed by the two nodes if they are placed at the two given positions
	double computeCoordEnergy(node,node, const DPoint&, const DPoint &) const override;
};

}}
