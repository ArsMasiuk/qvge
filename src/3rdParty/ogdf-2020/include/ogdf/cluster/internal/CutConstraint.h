/** \file
 * \brief Declaration of a constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * This class represents the cut-constraints belonging to the ILP formulation.
 * Cut-constraints are dynamically separated be means of cutting plane methods.
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

class CutConstraint : public BaseConstraint {
public:
	CutConstraint(abacus::Master *master, abacus::Sub *sub, List<NodePair> &edges);

	virtual ~CutConstraint();

	// Computes and returns the coefficient for the given variable
	virtual double coeff(const abacus::Variable *v) const override {
		const EdgeVar *ev = static_cast<const EdgeVar *>(v);
		return static_cast<double>(coeff(ev->sourceNode(), ev->targetNode()));
	}
	inline int coeff(const NodePair& n) const override { return coeff(n.source,n.target); }
	int coeff(node n1, node n2) const;

	void printMe(std::ostream& out) const {
		out << "[CutCon: ";
		for(const NodePair &p : m_cutEdges) {
			out << p << ",";
		}
		out << "]";
	}

private:
	// The list containing the node pairs corresponding to the cut edges
	List<NodePair> m_cutEdges;

};

}
}
