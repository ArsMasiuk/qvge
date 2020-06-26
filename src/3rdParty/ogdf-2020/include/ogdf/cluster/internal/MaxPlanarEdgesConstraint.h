/** \file
 * \brief Declaration of a constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * These constraint do not necessarily belong to the ILP formulation, but
 * have the purpose to strengthen the LP-relaxations in the case of very dense
 * Graphs, by restricting the maximum number of edges that can occur in any optimal
 * solution according to Euler's formula for planar Graphs: |E| <= 3|V|-6
 *
 * \author Mathias Jansen
 *
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

#include <ogdf/cluster/internal/EdgeVar.h>
#include <ogdf/cluster/internal/basics.h>

#include <ogdf/lib/abacus/constraint.h>

namespace ogdf {
namespace cluster_planarity {

class MaxPlanarEdgesConstraint : public abacus::Constraint {
#ifdef OGDF_DEBUG
	friend class MaxCPlanarSub;
	friend class CPlanarSub;
#endif
public:
	//construction
	MaxPlanarEdgesConstraint(abacus::Master *master, int edgeBound, List<NodePair> &edges);
	MaxPlanarEdgesConstraint(abacus::Master *master, int edgeBound);

	//destruction
	virtual ~MaxPlanarEdgesConstraint();

	//computes and returns the coefficient for the given variable
	virtual double coeff(const abacus::Variable *v) const override;

private:
	List<NodePair> m_edges;
	bool m_graphCons;
};

}
}
