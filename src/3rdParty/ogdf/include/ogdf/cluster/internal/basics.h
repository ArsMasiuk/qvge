/** \file
 * \brief Declaration of the master class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * Basic classes for c-planarity computation.
 *
 * \author Karsten Klein
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

#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/constraint.h>
#include <ogdf/basic/Graph_d.h>
#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>

namespace ogdf {
namespace cluster_planarity {

class ChunkConnection;

//! Struct for attaching the current lp-value to the corresponding edge.
//! Used in the primal heuristic.
struct edgeValue {
	node src;
	node trg;
	double lpValue;
	bool original;
	edge e;
};

//! Basic constraint type
class BaseConstraint : public abacus::Constraint {
public:
	BaseConstraint(abacus::Master *master, const abacus::Sub *sub, abacus::CSense::SENSE sense, double rhs, bool dynamic, bool local, bool liftable) :
		abacus::Constraint(master, sub, sense, rhs, dynamic, local, liftable) { }

	virtual ~BaseConstraint() { }

	virtual int coeff(const NodePair& n) const = 0;
	virtual double coeff(const abacus::Variable *v) const = 0;
};

}
}
