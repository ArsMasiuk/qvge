/** \file
 * \brief Definition of ogdf::steiner_tree::Full3ComponentGeneratorModule class template
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
 * Interface for full 3-component generation including auxiliary functions
 *
 * A full 3-component is basically a tree with *exactly* three terminal leaves
 * but no inner terminals. There must be exactly one nonterminal of degree 3,
 * the so-called center.
 */
template<typename T>
class Full3ComponentGeneratorModule
{
public:
	Full3ComponentGeneratorModule() = default;
	virtual ~Full3ComponentGeneratorModule() = default;

	//! Generate full components and call \p generateFunction for each full component
	virtual void call(
	  const EdgeWeightedGraph<T> &G,
	  const List<node> &terminals,
	  const NodeArray<bool> &isTerminal,
	  const NodeArray<NodeArray<T>> &distance,
	  const NodeArray<NodeArray<edge>> &pred,
	  std::function<void(node, node, node, node, T)> generateFunction) const = 0;

protected:
	/*!
	 * \brief Update center node if it is the best so far. (Just a helper to avoid code duplication.)
	 * @param x The node to test
	 * @param center The returned best center node
	 * @param minCost The returned cost of the component with that node
	 * @param dist1 SSSP distance vector of the first terminal
	 * @param dist2 SSSP distance vector of the second terminal
	 * @param dist3 SSSP distance vector of the third terminal
	 */
	inline void updateBestCenter(node x, node &center, T &minCost, const NodeArray<T> &dist1, const NodeArray<T> &dist2, const NodeArray<T> &dist3) const
	{
#ifdef OGDF_FULL_COMPONENT_GENERATION_ALWAYS_SAFE
		if (true) {
#else
		if (dist1[x] < std::numeric_limits<T>::max()
		 && dist2[x] < std::numeric_limits<T>::max()
		 && dist3[x] < std::numeric_limits<T>::max()) {
#endif
			const T tmp = dist1[x] + dist2[x] + dist3[x];
			if (tmp < minCost) {
				center = x;
				minCost = tmp;
			}
		}
	}

	inline void forAllTerminalTriples(
	  const List<node> &terminals,
	  const NodeArray<NodeArray<T>> &distance,
	  std::function<void(node, node, node, const NodeArray<T> &, const NodeArray<T> &, const NodeArray<T> &)> func) const
	{
		for (ListConstIterator<node> it_u = terminals.begin(); it_u.valid(); ++it_u) {
			for (ListConstIterator<node> it_v = it_u.succ(); it_v.valid(); ++it_v) {
				for (ListConstIterator<node> it_w = it_v.succ(); it_w.valid(); ++it_w) {
					func(*it_u, *it_v, *it_w, distance[*it_u], distance[*it_v], distance[*it_w]);
				}
			}
		}
	}

	inline void checkAndGenerateFunction(node u, node v, node w, node center, T minCost,
	                                     const NodeArray<NodeArray<edge>> &pred,
	                                     const NodeArray<bool> &isTerminal,
	                                     std::function<void(node, node, node, node, T)> generateFunction) const
	{
		if (center
		 && !isTerminal[center]
		 && pred[u][center]
		 && pred[v][center]
		 && pred[w][center]) {
			generateFunction(u, v, w, center, minCost);
		}
	}
};

}
}
