/** \file
 * \brief Declaration of a constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * This class represents the cut-constraints belonging to the ILP formulation.
 * Cut-constraints are dynamically separated be means of cutting plane methods.
 *
 * \author Mathias Jansen
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

#include <ogdf/basic/basic.h>
#include <ogdf/cluster/internal/CutConstraint.h>

using namespace ogdf;
using namespace ogdf::cluster_planarity;
using namespace abacus;

CutConstraint::CutConstraint(Master *master, Sub *sub, List<NodePair> &edges) :
	BaseConstraint(master, sub, CSense::Greater, 1.0, true, true, true)
{
	for (const NodePair &p : edges) {
		m_cutEdges.pushBack(p);
	}
}

CutConstraint::~CutConstraint() {}

int CutConstraint::coeff(node n1, node n2) const
{
	for (const NodePair &p : m_cutEdges) {
		if ( (p.source == n1 && p.target == n2) || (p.target == n1 && p.source == n2) )
			return 1;
	}
	return 0;
}
