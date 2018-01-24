/** \file
 * \brief Implementation of the variable class for the Branch&Cut algorithm
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

#include <ogdf/basic/basic.h>
#include <ogdf/cluster/internal/EdgeVar.h>
#include <ogdf/cluster/internal/MaxCPlanarMaster.h>

using namespace ogdf;
using namespace ogdf::cluster_planarity;
using namespace abacus;

EdgeVar::EdgeVar(Master *master, double obj, EdgeType eType, node source, node target)
  : Variable(master, nullptr, false, false, obj,
             eType == EdgeType::Connect
               ? 0.0
               : (static_cast<MaxCPlanarMaster*>(master)->getCheckCPlanar() ? 1.0 : 0.0),
             1.0,
             eType == EdgeType::Connect
               ? VarType::Binary
               : (static_cast<MaxCPlanarMaster*>(master)->getCheckCPlanar() ? VarType::Continuous : VarType::Binary))
{
	m_eType = eType;
	m_source = source;
	m_target = target;
//	m_objCoeff = obj; // not necc.
//TODO no searchedge!
	if (eType == EdgeType::Original) m_edge = static_cast<MaxCPlanarMaster*>(master)->getGraph()->searchEdge(source,target);
	else m_edge = nullptr;
}

EdgeVar::EdgeVar(Master *master, double obj, node source, node target) :
	Variable (master, nullptr, false, false, obj, 0.0, 1.0, VarType::Binary)
{
	m_eType = EdgeType::Connect;
	m_source = source;
	m_target = target;
	m_edge = nullptr;
}

EdgeVar::EdgeVar(Master *master, double obj, double lbound, node source, node target) :
	Variable (master, nullptr, false, false, obj, lbound, 1.0, VarType::Binary)
{
	m_eType = EdgeType::Connect;
	m_source = source;
	m_target = target;
	m_edge = nullptr;
}

EdgeVar::~EdgeVar() {}
