/** \file
 * \brief implementation of constraint class for the Branch&Cut algorithm
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

#include <ogdf/cluster/internal/MinimalClusterConnection.h>

using namespace ogdf;
using namespace ogdf::cluster_planarity;
using namespace abacus;

MinimalClusterConnection::MinimalClusterConnection(Master *master, List<NodePair> &edges) :
	Constraint(master, nullptr, CSense::Less, 1.0, false, false, true)
{
	for (const NodePair &np : edges) {
		m_edges.pushBack(np);
	}
}

MinimalClusterConnection::~MinimalClusterConnection() {}

double MinimalClusterConnection::coeff(const Variable *v) const {
	//TODO: speedup, we know between which nodepairs edges exist...
	const EdgeVar *e = static_cast<const EdgeVar *>(v);
	for (const NodePair &np : m_edges) {
		if ( (np.source == e->sourceNode() && np.target == e->targetNode()) ||
			 (np.target == e->sourceNode() && np.source == e->targetNode()) )
		{
			return 1.0;
		}
	}
	return 0.0;
}
