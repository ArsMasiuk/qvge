/** \file
 * \brief Definition of ogdf::steiner_tree::Full2ComponentGenerator class template
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

#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraph.h>

namespace ogdf {
namespace steiner_tree {

/**
 * Trivial full 2-component generation by lookups of shortest paths between terminal pairs
 *
 * A full 2-component is a path that starts and ends with a terminal
 * but has no terminal in between.
 */
template<typename T>
class Full2ComponentGenerator
{
public:
	//! Generate full 2-components and call \p generateFunction for each full 2-component
	inline void call(
	  const EdgeWeightedGraph<T> &G,
	  const List<node> &terminals,
	  const NodeArray<NodeArray<T>> &distance,
	  const NodeArray<NodeArray<edge>> &pred,
	  std::function<void(node, node, T)> generateFunction) const
	{
		for (ListConstIterator<node> it_u = terminals.begin(); it_u.valid(); ++it_u) {
			const node u = *it_u;
			for (ListConstIterator<node> it_v = it_u.succ(); it_v.valid(); ++it_v) {
				const node v = *it_v;
				if (pred[u][v]) {
					generateFunction(u, v, distance[u][v]);
				}
			}
		}
	}
};

}
}
