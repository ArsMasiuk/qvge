/** \file
 * \brief Definition of ogdf::steiner_tree::Full3ComponentGeneratorEnumeration class template
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

#include <ogdf/graphalg/steiner_tree/Full3ComponentGeneratorModule.h>

namespace ogdf {
namespace steiner_tree {

//! Full 3-component generation using enumeration
template<typename T>
class Full3ComponentGeneratorEnumeration : public Full3ComponentGeneratorModule<T>
{
public:
	void call(const EdgeWeightedGraph<T> &G,
	          const List<node> &terminals,
	          const NodeArray<bool> &isTerminal,
	          const NodeArray<NodeArray<T>> &distance,
	          const NodeArray<NodeArray<edge>> &pred,
	          std::function<void(node, node, node, node, T)> generateFunction) const
	{
		this->forAllTerminalTriples(terminals, distance,
		  [&](node u, node v, node w, const NodeArray<T> &uDistance, const NodeArray<T> &vDistance, const NodeArray<T> &wDistance) {
			node center = nullptr;
			T minCost = std::numeric_limits<T>::max();
			for (node x : G.nodes) {
				this->updateBestCenter(x, center, minCost, uDistance, vDistance, wDistance);
			}
			this->checkAndGenerateFunction(u, v, w, center, minCost, pred, isTerminal, generateFunction);
		});
	}
};

}
}
