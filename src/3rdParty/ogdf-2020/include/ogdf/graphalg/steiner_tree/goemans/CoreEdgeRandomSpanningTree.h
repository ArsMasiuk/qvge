/** \file
 * \brief Definition of ogdf::steiner_tree::goemans::CoreEdgeRandomSpanningTree class template
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

#include <ogdf/basic/DisjointSets.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/graphalg/steiner_tree/goemans/CoreEdgeModule.h>

namespace ogdf {
namespace steiner_tree {
namespace goemans {

//! Computes a random set of core edges
template<typename T>
class CoreEdgeRandomSpanningTree
  : public CoreEdgeModule<T>
{
	std::minstd_rand &m_rng;

public:
	CoreEdgeRandomSpanningTree(std::minstd_rand &rng)
	  : m_rng(rng)
	{
	}

	void call(const Graph &graph, const List<node> &terminals, EdgeArray<bool> &isInTree) const override
	{
		// Let's do Kruskal's algorithm without weights but on a randomly permuted edge list.
		// We virtually contract all terminals in the union-find data structure.
		NodeArray<int> setID(graph, -1);
		isInTree.init(graph, false);
		DisjointSets<> uf(graph.numberOfNodes() - terminals.size() + 1);

		int contractedID = uf.makeSet();
		OGDF_ASSERT(contractedID >= 0);
		for (node t : terminals) {
			setID[t] = contractedID;
		}
		for (node v : graph.nodes) {
			if (setID[v] < 0) {
				setID[v] = uf.makeSet();
			}
		}

		// obtain a random edge permutation
		ArrayBuffer<edge> edgePermutation;
		for (edge e : graph.edges) {
			edgePermutation.push(e);
		}
		edgePermutation.permute(m_rng);

		// add edges if we do not close a cycle
		for (edge e : edgePermutation) {
			const int v = setID[e->source()];
			const int w = setID[e->target()];
			if (uf.find(v) != uf.find(w)) {
				isInTree[e] = true;
				uf.link(uf.find(v), uf.find(w));
			}
		}
	}
};

}
}
}
