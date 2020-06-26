/** \file
 * \brief Declaration of a constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * These constraints represent the planarity-constraints belonging to the
 * ILP formulation. These constraints are dynamically separated.
 * For the separation the planarity test algorithm by Boyer and Myrvold is used.
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

#include <ogdf/cluster/internal/ClusterKuratowskiConstraint.h>

using namespace ogdf;
using namespace ogdf::cluster_planarity;
using namespace abacus;

ClusterKuratowskiConstraint::ClusterKuratowskiConstraint(Master *master, int nEdges, SListPure<NodePair> &ks) :
	Constraint(master, nullptr, CSense::Less, nEdges-1, true, false, true)
{
	for (const NodePair &np : ks) {
		m_subdivision.pushBack(np);
	}
}


ClusterKuratowskiConstraint::~ClusterKuratowskiConstraint() {}


double ClusterKuratowskiConstraint::coeff(const Variable *v) const {
	const EdgeVar *e = static_cast<const EdgeVar*>(v);
	for (const NodePair &np : m_subdivision) {
		if( (np.source == e->sourceNode() && np.target == e->targetNode()) ||
			(np.source == e->targetNode() && np.target == e->sourceNode()) )
		{
			return 1.0;
		}
	}
	return 0.0;
}
