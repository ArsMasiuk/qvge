/** \file
 * \brief implementation of class EdgeInsertionModuleOld
 *
 * \author Carsten Gutwenger
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

#include <ogdf/uml/UMLCrossingMinimizationModule.h>

namespace ogdf {


bool UMLCrossingMinimizationModule::checkCrossingGens(const PlanRepUML &prUML)
{
	for(edge e : prUML.edges) {
		Graph::EdgeType et = prUML.typeOf(e);
		if (et != Graph::EdgeType::generalization && et != Graph::EdgeType::association)
			return false;
	}

	for(node v : prUML.nodes)
	{
		if (prUML.typeOf(v) == PlanRepUML::NodeType::dummy && v->degree() == 4) {
			adjEntry adj = v->firstAdj();

			edge e1 = adj->theEdge();
			edge e2 = adj->succ()->theEdge();

			if (prUML.typeOf(e1) == Graph::EdgeType::generalization &&
				prUML.typeOf(e2) == Graph::EdgeType::generalization)
				return false;
		}
	}

	return true;
}

}
