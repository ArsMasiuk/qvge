/** \file
 * \brief Auxiliary functions for FMMM to reduce code duplication
 *
 * \author Stephan Beyer
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

#include <ogdf/energybased/fmmm/NodeAttributes.h>
#include <ogdf/energybased/fmmm/numexcept.h>

namespace ogdf {
namespace energybased {
namespace fmmm {

inline void calculate_forces_inside_contained_nodes(NodeArray<DPoint> &F_rep, const NodeArray<NodeAttributes> &A, const List<node> &contained_nodes) {
	int length = contained_nodes.size();
	Array<node> numbered_nodes(length+1);
	int i = 1;
	for (node v : contained_nodes) {
		numbered_nodes[i] = v;
		++i;
	}

	for (i = 1; i < length; i++) {
		for (int j = i + 1; j <= length; j++) {
			node u = numbered_nodes[i];
			node v = numbered_nodes[j];
			DPoint f_rep_u_on_v = numexcept::f_rep_u_on_v(A[u].get_position(), A[v].get_position());
			F_rep[v] += f_rep_u_on_v;
			F_rep[u] -= f_rep_u_on_v;
		}
	}
}

}
}
}
