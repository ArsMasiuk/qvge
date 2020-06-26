/** \file
 * \brief Declaration of the variable class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem
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

#pragma once

#include <ogdf/basic/Graph_d.h>
#include <ogdf/basic/Logger.h>
#include <ogdf/cluster/internal/EdgeVar.h>
#include <ogdf/lib/abacus/variable.h>

namespace ogdf {
namespace cluster_planarity {

class CPlanarEdgeVar : public EdgeVar {
	friend class MaxCPlanarSub;
public:
	CPlanarEdgeVar(abacus::Master *master, double obj, node source, node target) :
		EdgeVar(master, obj, source, target)
	{

	}
	CPlanarEdgeVar(abacus::Master *master, double obj, double lbound, node source, node target) :
			EdgeVar(master, obj, lbound, source, target)
	{

	}

	virtual ~CPlanarEdgeVar() {}

	void printMe(std::ostream& out) override {
		out << "[Var: " << sourceNode() << "->" << targetNode() << " (" << "connect" << ") ZF=" << obj() << "]";
	}

};

}
}
