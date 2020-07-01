/** \file
 * \brief Declaration of class PlanarSubgraphTree.
 *
 * \author Tilo Wiedera
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

#include <ogdf/planarity/PlanarSubgraphModule.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/DisjointSets.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/Math.h>

namespace ogdf {

//! Maximum planar subgraph heuristic that yields a spanning tree.
//! @ingroup ga-plansub
template<typename TCost>
class PlanarSubgraphTree : public PlanarSubgraphModule<TCost>
{
public:
	virtual PlanarSubgraphTree *clone() const override {
		return new PlanarSubgraphTree();
	}

protected:
	virtual Module::ReturnType doCall(
			const Graph &graph,
			const List<edge> &preferredEdges,
			List<edge> &delEdges,
			const EdgeArray<TCost> *pCost,
			bool preferedImplyPlanar) override {
		delEdges.clear();

		if (pCost) {
			GraphCopy copy(graph);
			EdgeArray<TCost> weight(copy);
			TCost maxCost = std::numeric_limits<TCost>::min();

			for (edge e : graph.edges) {
				Math::updateMax(maxCost, (*pCost)[e]);
			}

			for (edge e : copy.edges) {
				weight[e] = maxCost - (*pCost)[copy.original(e)];
			}

			makeMinimumSpanningTree(copy, weight);

			for (edge e : graph.edges) {
				if (copy.copy(e) == nullptr) {
					delEdges.pushBack(e);
				}
			}
		} else if (!graph.empty()) {
			// Will contain the parent of each node in the computed forest.
			// parent[v] == v iff v is a root node.
			// parent[v] == nullptr iff v wasn't visited yet.
			NodeArray<node> parent(graph, nullptr);
			ArrayBuffer<node> nodes(graph.numberOfNodes());

			for (node v : graph.nodes) {
				if (parent[v] == nullptr) {
					parent[v] = v;
					nodes.push(v);

					while (!nodes.empty()) {
						node u = nodes.popRet();

						for (adjEntry adj : u->adjEntries) {
							node w = adj->twinNode();

							if (parent[w] == nullptr) {
								parent[w] = u;
								nodes.push(w);
							}
						}
					}
				}
			}

			for (edge e : graph.edges) {
				node v = e->source();
				node w = e->target();

				bool vIsParent = v == parent[w];
				bool wIsParent = w == parent[v];

				// Delete edges that are not in the tree.
				// In particular, do not pick parallel edges or self-loops.
				if (e->isSelfLoop() || (!vIsParent && !wIsParent)) {
					delEdges.pushBack(e);
				} else if (vIsParent) {
					parent[w] = nullptr;
				} else if (wIsParent) {
					parent[v] = nullptr;
				}
			}
		}

#ifdef OGDF_DEBUG
		NodeArray<int> tmp(graph);
		int numberOfComponents = connectedComponents(graph, tmp);
		int numberOfEdgesInForest = graph.numberOfEdges() - delEdges.size();
		// Euler characteristic for forests
		OGDF_ASSERT(numberOfEdgesInForest == graph.numberOfNodes() - numberOfComponents);
#endif

		return Module::ReturnType::Feasible;
	}
};

}
