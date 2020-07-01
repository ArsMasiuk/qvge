/** \file
 * \brief Declaration of the variable class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem
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

#include <ogdf/basic/Graph_d.h>
#include <ogdf/basic/Logger.h>
#include <ogdf/lib/abacus/variable.h>

namespace ogdf {
namespace cluster_planarity {

class EdgeVar : public abacus::Variable {
	friend class MaxCPlanarSub;
public:
	enum class EdgeType {Original, Connect};

	EdgeVar(abacus::Master *master, double obj, EdgeType eType, node source, node target);
	//! Simple version for cplanarity testing (only connect edges allowed)
	EdgeVar(abacus::Master *master, double obj, node source, node target);
	//! Simple version for cplanarity testing (only connect edges allowed, lower bound given)
	EdgeVar(abacus::Master *master, double obj, double lbound, node source, node target);

	virtual ~EdgeVar();

	edge theEdge() const {return m_edge;}
	node sourceNode() const {return m_source;}
	node targetNode() const {return m_target;}
	EdgeType theEdgeType() const {return m_eType;}
#if 0
	double objCoeff() const {return m_objCoeff;}
#endif

	virtual void printMe(std::ostream& out) {
		out << "[Var: " << sourceNode() << "->" << targetNode() << " (" << ((theEdgeType()==EdgeVar::EdgeType::Original)?"original":"connect") << ") ZF=" << obj() << "]";
	}

private:
	// The edge type of the variable
	EdgeType m_eType;

	// The corresponding nodes and edge
	node m_source;
	node m_target;
	edge m_edge;
};

}
}
