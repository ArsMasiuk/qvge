/** \file
 * \brief Declaration of class PlanarSubgraphTriangles.
 *
 * \author Jöran Schierbaum
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

namespace ogdf {

//! Maximum planar subgraph approximation algorithms by Chalermsook/Schmid and Calinescu et al.
/**
 * @ingroup ga-plansub
 *
 * This planarity module supports two algorithms.
 * - A greedy one by Calinescu et all with an approximation factor of 7/18
 * - A greedy one by Chalermsook and Schmid with an approximation factor of 13/33
 *
 * The default selection is Chalermsook and Schmid.
 *
 * Setting preferred edges is not supported.
 * Weighted edges are heuristically respected but there is no approximation guarantee in the weighted case.
 */
template<typename TCost>
class PlanarSubgraphTriangles : public PlanarSubgraphModule<TCost>
{
public:

	//! Creates a planarization module based on triangle or diamond matching
	/**
	 * @param onlyTriangles If true, only search for triangles. If false (default), search for diamonds
	 *        first and then match triangles.
	 */
	PlanarSubgraphTriangles(bool onlyTriangles = false) : m_onlyTriangles(onlyTriangles) { }

	//! Returns a new instance of the planarization module with the same settings
	virtual PlanarSubgraphTriangles *clone() const override {
		return new PlanarSubgraphTriangles(m_onlyTriangles);
	}

protected:
	virtual Module::ReturnType doCall(const Graph &graph,
		const List<edge> &,
		List<edge> &delEdges,
		const EdgeArray<TCost> *pCost,
		bool preferredImplyPlanar = false) override
	{
		OGDF_ASSERT(isConnected(graph));
		OGDF_ASSERT(isSimpleUndirected(graph));

		delEdges.clear();
		GraphCopy copy(graph);
		List<edge> edges;
		copy.allEdges(edges);
		EdgeArray<bool> includeEdges(copy, false);

		// sort weighted edges
		if(pCost != nullptr) {
			GenericComparer<edge, TCost, false> edgeCmp([&](edge e) { return (*pCost)[copy.original(e)]; });
			GenericComparer<adjEntry, TCost, false> adjCmp([&](adjEntry a) { return (*pCost)[copy.original(a->theEdge())]; });
			edges.quicksort(edgeCmp);

			for(node v : copy.nodes) {
				List<adjEntry> newOrder;
				v->allAdjEntries(newOrder);
				newOrder.quicksort(adjCmp);
				copy.sort(v, newOrder);
			}
		}

		DisjointSets<> components(copy.numberOfNodes());
		NodeArray<int> set(copy);
		for (node v : copy.nodes) {
			set[v] = components.makeSet();
		}

		if (!m_onlyTriangles) {
			// First step: Find as many diamonds as we can.
			for (edge currentEdge : edges) {

				// Skip if we already include this edge
				if (includeEdges[currentEdge]) continue;

				// We assume this edge to be the cord of our diamond. This means we have to find two
				// distinct triangles to make a diamond, and we will try to prefer ones with higher
				// weights given by our pCost parameter

				node source = currentEdge->source();
				node target = currentEdge->target();

				edge triangleEdge1 = nullptr, triangleEdge2 = nullptr;
				node triangleNode = nullptr;
				int triangleSet = -1;

				findTriangle(copy, currentEdge, pCost, components, set, [&](node v, edge e1, edge e2) {
					// Only use the found triangle if none of the nodes are in the same component.
					// We know that each individually found triangle does not have two nodes in the
					// same component, so we only have to check the opposite ones.
					int potentialSet = components.find(set[v]);
					if (potentialSet == triangleSet) return false; // This is not a triangle we can use, keep looking!

					if (triangleNode == nullptr) {
						// We don't have a triangle yet, mark this one down as the best we can find from this edge.
						triangleNode = v;
						triangleEdge1 = e1;
						triangleEdge2 = e2;
						triangleSet = potentialSet;
						return false; // continue searching for a second triangle to make our diamond
					}
					else {
						// We already found a triangle before, so this is the second-best triangle we can find,
						// making a diamond.
						includeEdges[currentEdge] = includeEdges[triangleEdge1] = includeEdges[triangleEdge2]
						                          = includeEdges[e1] = includeEdges[e2] = true;

						// Link up diamond nodes' components. These cannot be on the same connected subgraph yet.
						int sourceSet = components.find(set[source]);
						int targetSet = components.find(set[target]);
						components.link(components.link(components.link(sourceSet, targetSet), triangleSet), potentialSet);
						return true; // stop looking
					}
				});
			}
		}

		// Second step: Find as many triangles as we can.
		for (edge currentEdge : edges) {

			if (includeEdges[currentEdge]) continue;

			node source = currentEdge->source();
			node target = currentEdge->target();

			findTriangle(copy, currentEdge, pCost, components, set,[&](node v, edge e1, edge e2) {
				includeEdges[currentEdge] = includeEdges[e1] = includeEdges[e2] = true;
				int potentialSet = components.find(set[v]);
				int sourceSet = components.find(set[source]);
				int targetSet = components.find(set[target]);
				components.link(components.link(sourceSet, targetSet), potentialSet);
				return true;
			});

		}

		// Third step: Link unconnected sub graphs
		for (edge currentEdge : edges) {
			node source = currentEdge->source();
			node target = currentEdge->target();
			int sourceSet = components.find(set[source]);
			int targetSet = components.find(set[target]);
			if (sourceSet != targetSet) {
				includeEdges[currentEdge] = true;
				components.link(sourceSet, targetSet);
			}

			if (!includeEdges[currentEdge]) delEdges.pushBack(copy.original(currentEdge));
		}

		return Module::ReturnType::Feasible;
	}


private:
	bool m_onlyTriangles; //!< Whether we want to only check for triangles

	//! Finds an edge, starting at a given iterator
	/**
	 * Takes a node and its adjacency list iterator and, from the iterator's current position,
	 * tries to find an edge that leads to a target.
	 * @param target the node to connect to
	 * @param source the node to connect from
	 * @param connectionIterator an adjacency list iterator of \p source
	 * @return edge between \p target and \p source if found, \c nullptr otherwise
	 */
	edge searchEdge(node target, node source, internal::GraphIterator<adjEntry> connectionIterator) {
		while (connectionIterator != source->adjEntries.end()) {
			if ((*connectionIterator)->twinNode() == target) {
				return (*connectionIterator)->theEdge();
			}
			connectionIterator++;
		}
		return nullptr;
	}

	//! Finds all triangles from a given edge and calls a callback function on them
	/**
	 * Private implementation function. Takes an edge and attempts to find a node that is
	 * adjacent to the edge's incident nodes.
	 *
	 * @param copy a copy of the graph to look in
	 * @param currentEdge the edge to start the search with
	 * @param pCost for every edge in \p copy, a weight that influences how likely an edge is
	 * chosen, with higher cost being more likely.
	 * @param components a set of components in the graph. Triangles cannot have two nodes in
	 * the same component to keep planarity promises.
	 * @param set a mapping of nodes to components
	 * @param callback a function that takes a node and two of its incident edges, creating a
	 * triangle with \p currentEdge. The calls to this function are roughly ordered by weight
	 * as defined by \p pCost. If the callback returns true, stop the search, otherwise a new
	 * triangle will be searched for.
	 */
	void findTriangle(GraphCopy& copy, edge currentEdge, const EdgeArray<TCost> *pCost,
	                  DisjointSets<>& components, NodeArray<int>& set,
	                  std::function<bool(node, edge, edge)> callback) {
		node source = currentEdge->source();
		node target = currentEdge->target();
		auto sourceIt = source->adjEntries.begin();
		auto targetIt = target->adjEntries.begin();
		int sourceSet = components.find(set[source]);
		int targetSet = components.find(set[target]);
		// Our nodes cannot be in the same set.
		if (sourceSet == targetSet) return;

		while (sourceIt != source->adjEntries.end() && targetIt != target->adjEntries.end()) {
			if ((*sourceIt)->theEdge() == currentEdge) {
				sourceIt++;
				continue; // re-check loop condition
			}
			if ((*targetIt)->theEdge() == currentEdge) {
				targetIt++;
				continue; // re-check loop condition
			}

			// Look for a triangle, starting with the edge with next highest weight
			// If no weights are given, it does not matter which edge we start with
			adjEntry potentialConnector;
			node potentialConnection;
			internal::GraphIterator<adjEntry> potentialConnectionIterator;
			if (pCost == nullptr || (*pCost)[copy.original((*sourceIt)->theEdge())] > (*pCost)[copy.original((*targetIt)->theEdge())]) {
				potentialConnector = *sourceIt;
				potentialConnection = target;
				potentialConnectionIterator = targetIt;
				sourceIt++;
			}
			else {
				potentialConnector = *targetIt;
				potentialConnection = source;
				potentialConnectionIterator = sourceIt;
				targetIt++;
			}
			// Note: sourceIt and targetIt may be invalid until next loop

			node potentialNode = potentialConnector->twinNode();
			int potentialSet = components.find(set[potentialNode]);

			// Only use this edge if it doesn't connect back to one of the components
			if (potentialSet != sourceSet && potentialSet != targetSet) {
				edge potentialEdge = searchEdge(potentialNode, potentialConnection, potentialConnectionIterator);
				if (potentialEdge != nullptr) {
					// We found a triangle.
					// If our callback returns true, it's signalling that we're done and don't
					// want to look for another triangle.
					// If it returns false, we continue looking.
					if (callback(potentialNode, potentialEdge, potentialConnector->theEdge())) {
						return;
					}
				}
			}
		}
	}
};

}
