/** \file
 * \brief Declaration of an initial constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * If some cluster has no connection to some other cluster,
 * the optimal solution might insert a new connection-edge between theese two clusters,
 * to obtain connectivity. Since the objective function minimizes the number
 * of new connection-edges, at most one new egde will be inserted between
 * two clusters that are not connected.
 * This behaviour of the LP-solution is guaranteed from the beginning, by creating
 * an initial constraint for each pair of non-connected clusters,
 * which is implemented by this constraint class.
 *
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
#include <ogdf/cluster/internal/MaxCPlanarMaster.h>
#include <ogdf/lib/abacus/constraint.h>

namespace ogdf {
namespace cluster_planarity {

class MinimalClusterConnection : public abacus::Constraint {
public:
	MinimalClusterConnection(abacus::Master *master, List<NodePair> &edges);

	virtual ~MinimalClusterConnection();

	// Computes and returns the coefficient for the given variable
	virtual double coeff(const abacus::Variable *v) const override;

private:
	// The node pairs corresponding to the constraint
	List<NodePair> m_edges;
};

}
}
