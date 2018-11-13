/** \file
 * \brief Definition of ogdf::steiner_tree::goemans::CoreEdgeModule class template
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

#include <ogdf/basic/EdgeArray.h>

namespace ogdf {
namespace steiner_tree {
namespace goemans {

//! Interface for core edge finder algorithms
template<typename T>
class CoreEdgeModule
{
public:
	//! Compute a set of core edges
	//! @param graph The input graph
	//! @param terminals The terminals of the given graph
	//! @param isInTree The resulting EdgeArray where an edge is true iff it is not a core edge
	virtual void call(const Graph &graph, const List<node> &terminals, EdgeArray<bool> &isInTree) const = 0;
};

}
}
}
