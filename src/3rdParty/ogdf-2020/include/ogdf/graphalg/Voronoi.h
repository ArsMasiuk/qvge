/** \file
 * \brief Definition of ogdf::Voronoi class template
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

#include <ogdf/basic/ArrayBuffer.h>
#include <ogdf/graphalg/Dijkstra.h>
#include <map>

namespace ogdf {

//! Computes %Voronoi regions in an edge-weighted graph.
/**
 * @ingroup graph-algs
 */
template<typename T>
class Voronoi {
protected:
	NodeArray<edge> m_predecessor;
	NodeArray<T> m_distance;
	NodeArray<node> m_seedOfNode;
	std::map<node, ArrayBuffer<node>> m_nodeList;
	const Graph &m_graph;
	const List<node> &m_seeds;

	void computeVoronoiRegions(const Graph &G, const EdgeArray<T> &weights, const List<node> &seeds);

public:
	//! Build data structure to query %Voronoi regions of edge-weighted graph \p G with given %Voronoi seeds
	//! @param G the input graph
	//! @param weights edge weights
	//! @param seeds a list of %Voronoi seed nodes, the centers of the %Voronoi regions
	Voronoi(const Graph &G, const EdgeArray<T> &weights, const List<node> &seeds)
	 : m_seedOfNode(G)
	 , m_nodeList()
	 , m_graph(G)
	 , m_seeds(seeds)
	{
		computeVoronoiRegions(G, weights, seeds);
	}

	//! Returns a reference to the graph the %Voronoi region has been computed for.
	const Graph &getGraph() const
	{
		return m_graph;
	}

	//! Returns a reference to the list of seeds the %Voronoi region has been computed for.
	const List<node> &getSeeds() const
	{
		return m_seeds;
	}

	//! Returns the edge incident to \p v and its predecessor.
	//! Note that the predecessor of a terminal is \c nullptr.
	edge predecessorEdge(node v) const
	{
		return m_predecessor[v];
	}

	//! Returns the nearest node to \p v on the shortest path to its %Voronoi seed.
	node predecessor(node v) const
	{
		edge tmp = predecessorEdge(v);
		return tmp ? tmp->opposite(v) : nullptr;
	}

	//! Returns the distance between \p v and its %Voronoi seed.
	T distance(node v) const
	{
		return m_distance[v];
	}

	//! Returns the %Voronoi seed of node \p v.
	node seed(node v) const
	{
		return m_seedOfNode[v];
	}

	//! Returns the list of nodes in the %Voronoi region of node \p v.
	const ArrayBuffer<node> &nodesInRegion(node v) const
	{
		return m_nodeList.find(seed(v))->second;
	}
};

//// implementation

template<typename T>
void Voronoi<T>::computeVoronoiRegions(const Graph &G, const EdgeArray<T> &weights, const List<node> &seeds)
{
	Dijkstra<T> sssp;
	sssp.call(G, weights, seeds, m_predecessor, m_distance);

	// extract Voronoi seeds for each node and Voronoi regions for each seed
	NodeArray<bool> processed(G, false);
	for (node seed : seeds) {
		processed[seed] = true;
		m_seedOfNode[seed] = seed;
		m_nodeList[seed].push(seed);
	}

	for (node u : G.nodes) {
		ArrayBuffer<node> foundNodes;
		node v;
		for (v = u; !processed[v]; v = predecessor(v)) {
			processed[v] = true;
			foundNodes.push(v);
		}

		for (node passedNode : foundNodes) {
			m_seedOfNode[passedNode] = m_seedOfNode[v];
			m_nodeList[m_seedOfNode[v]].push(passedNode);
		}
	}
}

}
