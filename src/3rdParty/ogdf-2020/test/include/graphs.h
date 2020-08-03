/** \file
 * \brief Graph collection for tests
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

#include <set>

#include <ogdf/basic/graph_generators.h>
#include <ogdf/basic/simple_graph_alg.h>

#include <resources.h>

class GraphSizes {
	const int m_min;
	const int m_max;
	const int m_step;
public:
	//! Creates feasible graph sizes ranging from \p min to \p max
	//! with a step size of \p step.
	GraphSizes(int min, int max, int step) :
			m_min(min), m_max(max), m_step(step) {
		OGDF_ASSERT(m_min <= m_max);
		OGDF_ASSERT(m_step > 0);
	}

	//! Default graphs sizes result in 3 iterations
	//! over graphs with at most 100 nodes
	GraphSizes() : GraphSizes(16, 100, 42) {}

	//! Creates just one feasible size that is \p n
	GraphSizes(int n) : GraphSizes(n, n, 1) {}

	//! calls \p func for each feasible graph size
	void forEachSize(std::function<void(int size)> func) const {
		for(int n = m_min; n <= m_max; n += m_step) {
			func(n);
		}
	}
};

enum class GraphProperty {
	//! Indicates graphs that are (directed!) acyclic.
	acyclic,

	arborescenceForest,
	connected,
	biconnected,
	nonPlanar,
	maxDeg4,
	planar,
	triconnected,

	//! Indicates graphs that are (undirected!) simple.
	simple,
	loopFree,

	//! Indicates instances that have a reasonably low number of edges.
	//! These graphs can, e.g., be used for planarization layouts without raising runtime too much.
	sparse
};

//! Randomly adds loops and parallel edges to \p G.
/**
 * For each edge, we add parallel edges until an event with probability 1 - \p p is encountered.
 * For each node, we do the same creating loops.
 */
inline void addMultiEdges(Graph& G, double p) {
	OGDF_ASSERT(p >= 0);
	OGDF_ASSERT(p < 1);

	auto byChance = [&]() -> bool { return randomDouble(0, 1) < p; };

	List<edge> edges;
	G.allEdges(edges);

	for (node v : G.nodes) {
		while (byChance()) {
			G.newEdge(v, v);
		}
	}

	for (edge e : edges) {
		node v = e->source();
		node w = e->target();

		while (byChance()) {
			G.newEdge(v, w);

			if (byChance()) {
				std::swap(v, w);
			}
		}
	}
}

//! Creates gaps in the indices of nodes and edges of \p G.
/**
 * The gaps are created by reinserting nodes and incident edges.
 *
 * \p p Probability with which each node and its incident edges are reinserted.
 * It also determines whether the last node and edge indices are incremented
 * before reinsertion. A greater \p p leads to greater gaps in the indices.
 */
inline void makeIndicesNonContinuous(Graph& G, double p) {
	OGDF_ASSERT(p >= 0);
	OGDF_ASSERT(p < 1);

	auto byChance = [&]() -> bool { return randomDouble(0, 1) < p; };

#ifdef OGDF_DEBUG
	int n = G.numberOfNodes();
	int m = G.numberOfEdges();
#endif
	List<node> nodes;
	G.allNodes(nodes);

	for (node v : nodes) {
		if (byChance()) {
			// Create gaps before indices for newly inserted node/edges.
			while (byChance()) {
				G.delNode(G.newNode());
			}
			while (byChance()) {
				G.delEdge(G.newEdge(v,v));
			}

			// Create replacement for v.
			node newV = G.newNode();

			// Remember old neighbors.
			ArrayBuffer<node> outNeighbors;
			ArrayBuffer<node> inNeighbors;
			for (adjEntry adj : v->adjEntries) {
				if (adj->theEdge()->isSelfLoop()) {
					// Remember self-loops only once.
					if (adj->isSource()) {
						outNeighbors.push(newV);
					}
				} else {
					if (adj->isSource()) {
						outNeighbors.push(adj->twinNode());
					} else {
						inNeighbors.push(adj->twinNode());
					}
				}
			}

			// Delete v and reinsert incident edges.
			G.delNode(v);
			for (node neighbor : outNeighbors) {
				G.newEdge(newV, neighbor);
			}
			for (node neighbor : inNeighbors) {
				G.newEdge(neighbor, newV);
			}
		}
	}

	OGDF_ASSERT(n == G.numberOfNodes());
	OGDF_ASSERT(m == G.numberOfEdges());
}

/**
 * Creates a planar disconnected graphs that contains cut vertices.
 *
 * @param G Input graph.
 * @param nMax Approximate maximum number of nodes,
 * @param densityMin Approximate minimum edge density (relative to actual number of nodes).
 * @param densityMax Approximate maximum edge density (relative to actual number of nodes).
 * @param cc Number of connected components to create.
 * @param bc Number of biconnected components to create per connected component.
 */
inline void createDisconnectedGraph(
	Graph &G,
	int nMax,
	double densityMin,
	double densityMax,
	int cc,
	int bc)
{
	OGDF_ASSERT(cc > 0);
	OGDF_ASSERT(bc > 0);
	OGDF_ASSERT(densityMin > 0);
	OGDF_ASSERT(densityMax >= densityMin);
	OGDF_ASSERT(densityMax < 3);

	G.clear();

	int nBcMax = ceil(nMax / (cc*bc));
	nBcMax = std::max(nBcMax, 2);

	for(int i = 0; i < cc; i++) {
		int m = ceil(randomDouble(densityMin*nBcMax, densityMax*nBcMax));
		Graph H;
		randomPlanarCNBGraph(H, nBcMax, m, bc);
		G.insert(H);
	}
}

inline void createAlmostPlanarGraph(Graph &G, int n, int m, int add_m)
{
	randomPlanarBiconnectedGraph(G, n, m);

	Array<node> table(n);

	int i = 0;
	for(node v : G.nodes) {
		OGDF_ASSERT(i < n);
		table[i++] = v;
	}

	for(i = 0; i < add_m; ++i) {
		G.newEdge(table[randomNumber(0, n-1)], table[randomNumber(0, n-1)]);
	}

	makeSimpleUndirected(G);
}

//! Returns true if \p subset is a subset of \p superset
inline bool doesInclude(const std::set<GraphProperty>& subset, const std::set<GraphProperty>& superset) {
	for (auto r : subset) {
		if(superset.find(r) == superset.end()) {
			return false;
		}
	}

	return true;
}

inline void imply(std::set<GraphProperty>& props, GraphProperty conclusion, GraphProperty premise) {
	if (doesInclude({premise}, props)) {
		props.insert(conclusion);
	}
};

inline void performImplications(std::set<GraphProperty>& props) {
	imply(props, GraphProperty::biconnected, GraphProperty::triconnected);
	imply(props, GraphProperty::connected, GraphProperty::biconnected);
	imply(props, GraphProperty::planar, GraphProperty::arborescenceForest);
	imply(props, GraphProperty::acyclic, GraphProperty::arborescenceForest);
	imply(props, GraphProperty::loopFree, GraphProperty::simple);

	if (doesInclude({GraphProperty::simple}, props) &&
			(doesInclude({GraphProperty::maxDeg4}, props) || doesInclude({GraphProperty::planar}, props))) {
		props.insert(GraphProperty::sparse);
	}

	OGDF_ASSERT(!doesInclude({GraphProperty::nonPlanar, GraphProperty::planar}, props));
};

//! Make \p G (undirected) simple by splitting parallel edges.
//! Compared to ogdf::makeSimpleUndirected, this maintains biconnectivity.
inline void splitParallelEdges(Graph& G) {
	List<edge> edges;
	G.allEdges(edges);

	for(edge e : edges) {
		for (adjEntry adj : e->source()->adjEntries) {
			if (adj->twinNode() == e->target() && adj->theEdge() != e) {
				G.split(e);
			}
		}
	}
}

/**
 * Performs tests on a diverse set of graphs.
 *
 * @param requirements Required properties that feasible graphs must have.
 * @param doTest Actual test routine for given graph.
 * @param sizes Approximate number of nodes (and number of instances) for randomly generated graphs.
 * @param minSize Minimum number of nodes as a requirement for tested instances.
 * @param maxSize Maximum number of nodes as a requirement for tested instances.
 * @param describable Whether bandit::describe() should be used on \p doTest instead of bandit::it().
 */
inline void forEachGraphItWorks(std::set<GraphProperty> requirements,
		std::function<void(Graph&, const std::string &graphName, const std::set<GraphProperty>& props)> doTest,
		GraphSizes sizes = GraphSizes(),
		int minSize = 0,
		int maxSize = std::numeric_limits<int>::max(),
		bool describable = false) {
	auto testInstance = [&](const string& desc, std::set<GraphProperty> props, std::function<void(Graph&)> generateGraph) {
		performImplications(props);

		if (doesInclude(requirements, props)) {
			Graph G;
			generateGraph(G);
			makeIndicesNonContinuous(G, 0.5);

			if (G.numberOfNodes() >= minSize && G.numberOfNodes() <= maxSize) {
				auto func = [&] {
					doTest(G, desc, props);
				};

				if (describable) {
					bandit::describe("on a " + desc, func);
				} else {
					bandit::it("works on a " + desc, func);
				}
			}
		}
	};

	auto testInstances = [&](const string& desc, std::set<GraphProperty> props, std::function<void(Graph&, int)> generateGraph) {
		sizes.forEachSize([&](int n) {
			testInstance(desc + " [n≈" + to_string(n) + "]", props, [&](Graph& G) { generateGraph(G, n); });
		});
	};

	// Single test instances
	testInstance("graph without any nodes",
		{ GraphProperty::arborescenceForest,
		  GraphProperty::triconnected,
		  GraphProperty::maxDeg4,
		  GraphProperty::acyclic,
		  GraphProperty::simple },
		[](Graph &G) { emptyGraph(G, 0); });

	testInstance("graph with a single node",
		{ GraphProperty::arborescenceForest,
		  GraphProperty::triconnected,
		  GraphProperty::maxDeg4,
		  GraphProperty::acyclic,
		  GraphProperty::simple },
		[](Graph &G) { emptyGraph(G, 1); });

	testInstance("graph with a single node and one self-loop",
		{ GraphProperty::planar,
		  GraphProperty::triconnected,
		  GraphProperty::maxDeg4,
		  GraphProperty::sparse },
		[](Graph &G) { customGraph(G, 1, {{0,0}}); });

	testInstance("graph with two nodes and no edge",
		{ GraphProperty::arborescenceForest,
		  GraphProperty::maxDeg4,
		  GraphProperty::acyclic,
		  GraphProperty::simple },
		[](Graph &G) { emptyGraph(G, 2); });

	testInstance("graph with two nodes and one edge",
		{ GraphProperty::arborescenceForest,
		  GraphProperty::triconnected,
		  GraphProperty::maxDeg4,
		  GraphProperty::acyclic,
		  GraphProperty::simple },
		[](Graph &G) { customGraph(G, 2, {{0,1}}); });

	testInstance("graph with two nodes and two edges (one self-loop)",
		{ GraphProperty::planar,
		  GraphProperty::triconnected,
		  GraphProperty::maxDeg4,
		  GraphProperty::sparse },
		[](Graph &G) { customGraph(G, 2, {{0,0}, {0,1}}); });

	testInstance("graph with two nodes and directed parallel edges",
		{ GraphProperty::planar,
		  GraphProperty::acyclic,
		  GraphProperty::triconnected,
		  GraphProperty::maxDeg4,
		  GraphProperty::loopFree,
		  GraphProperty::sparse },
		[](Graph &G) { customGraph(G, 2, {{0,1}, {0,1}}); });

	testInstance("graph with two nodes and undirected parallel edges",
		{ GraphProperty::planar,
		  GraphProperty::triconnected,
		  GraphProperty::maxDeg4,
		  GraphProperty::loopFree,
		  GraphProperty::sparse },
		[](Graph &G) { customGraph(G, 2, {{0,1}, {1,0}}); });

	testInstance("graph with three nodes and no edge",
		{ GraphProperty::arborescenceForest,
		  GraphProperty::acyclic,
		  GraphProperty::maxDeg4,
		  GraphProperty::simple },
		[](Graph &G) { emptyGraph(G, 3); });

	testInstance("graph with three nodes and one edge",
		{ GraphProperty::arborescenceForest,
		  GraphProperty::acyclic,
		  GraphProperty::maxDeg4,
		  GraphProperty::simple },
		[](Graph &G) { customGraph(G, 3, {{0,1}}); });

	testInstance("K2,3",
		{ GraphProperty::maxDeg4,
		  GraphProperty::acyclic,
		  GraphProperty::planar,
		  GraphProperty::simple,
		  GraphProperty::biconnected },
		[](Graph& G) { completeBipartiteGraph(G, 2, 3); });

	testInstance("K3,3",
		{ GraphProperty::nonPlanar,
		  GraphProperty::maxDeg4,
		  GraphProperty::acyclic,
		  GraphProperty::simple,
		  GraphProperty::triconnected },
		[](Graph& G) { completeBipartiteGraph(G, 3, 3); });

	testInstance("K4",
		{ GraphProperty::maxDeg4,
		  GraphProperty::planar,
		  GraphProperty::simple,
		  GraphProperty::acyclic,
		  GraphProperty::triconnected},
		[](Graph& G) { completeGraph(G, 4); });

	testInstance("K5",
		{ GraphProperty::nonPlanar,
		  GraphProperty::maxDeg4,
		  GraphProperty::simple,
		  GraphProperty::acyclic,
		  GraphProperty::triconnected },
		[](Graph& G) { completeGraph(G, 5); });

	testInstance("Petersen graph",
		{ GraphProperty::nonPlanar,
		  GraphProperty::maxDeg4,
		  GraphProperty::triconnected,
		  GraphProperty::simple,
		  GraphProperty::sparse },
		[](Graph& G) { petersenGraph(G); });

	testInstance("path-like tree",
		{ GraphProperty::connected,
		  GraphProperty::planar,
		  GraphProperty::simple },
		[](Graph& G) {
			std::stringstream ss{ResourceFile::get("misc/path-like_tree.gml")->data()};
			GraphIO::read(G, ss);
		});

	testInstance("non-upward planar graph",
		{ GraphProperty::planar,
		  GraphProperty::acyclic,
		  GraphProperty::simple,
		  GraphProperty::sparse,
		  GraphProperty::connected },
		[](Graph &G) { customGraph(G, 6, {{0,1}, {0,2}, {1,3}, {1,4}, {2,3}, {2,4}, {3,5}, {4,5}}); });

	// Groups of similar test instances
	testInstances("arborescence",
		{ GraphProperty::arborescenceForest,
		  GraphProperty::connected,
		  GraphProperty::simple,
		  GraphProperty::sparse },
		[](Graph &G, int n) { randomTree(G, n); });

	testInstances("arborescence forest",
		{ GraphProperty::arborescenceForest,
		  GraphProperty::simple,
		  GraphProperty::sparse },
		[](Graph &G, int n) {
			randomTree(G, n);

			// make graph disconnected
			for(int i = 0; i < std::min(3, G.numberOfEdges()); i++) {
				G.delEdge(G.chooseEdge());
			}
		});

	testInstances("3-regular arborescence",
		{ GraphProperty::arborescenceForest,
		  GraphProperty::connected,
		  GraphProperty::maxDeg4,
		  GraphProperty::simple },
		[](Graph &G, int n) { regularTree(G, n, 3); });

	testInstances("isolated nodes",
		{ GraphProperty::arborescenceForest,
		  GraphProperty::maxDeg4,
		  GraphProperty::simple },
		[](Graph &G, int n) { emptyGraph(G, n); });

	testInstances("connected sparse graph",
		{ GraphProperty::connected,
		  GraphProperty::simple,
		  GraphProperty::sparse },
		[](Graph &G, int n) {
			randomSimpleGraph(G, n, 2*n);
			makeConnected(G);
		});

	testInstances("connected dense graph",
		{ GraphProperty::connected,
		  GraphProperty::simple },
		[](Graph &G, int n) {
			randomSimpleGraph(G, n,(n*n)/4);
			makeConnected(G);
		});

	testInstances("4-regular graph",
		{ GraphProperty::maxDeg4 },
		[](Graph &G, int n) { randomRegularGraph(G, n, 4); });

	testInstances("acyclic grid graph",
		{ GraphProperty::acyclic,
		  GraphProperty::biconnected,
		  GraphProperty::maxDeg4,
		  GraphProperty::planar,
		  GraphProperty::simple },
		[](Graph &G, int n) { gridGraph(G, sqrt(n), sqrt(n), false, false); });

	testInstances("wheel graph",
		{ GraphProperty::biconnected,
		  GraphProperty::planar,
		  GraphProperty::simple },
		[](Graph &G, int n) { wheelGraph(G, n); });

	testInstances("series parallel DAG",
		{ GraphProperty::acyclic,
		  GraphProperty::connected,
		  GraphProperty::planar,
		  GraphProperty::simple },
		[](Graph &G, int n) { randomSeriesParallelDAG(G, n); });

	testInstances("path with multi-edges",
		{ GraphProperty::connected,
		  GraphProperty::loopFree,
		  GraphProperty::planar },
		[](Graph &G, int n) {
			randomTree(G, n, 2, 1);
			addMultiEdges(G, .3);
			makeLoopFree(G);
		});

	testInstances("connected planar graph",
		{ GraphProperty::connected,
		  GraphProperty::planar,
		  GraphProperty::simple },
		[](Graph &G, int n) { randomPlanarConnectedGraph(G, n, 2*n); });

	testInstances("biconnected almost planar graph",
		{ GraphProperty::biconnected,
		  GraphProperty::nonPlanar,
		  GraphProperty::simple,
		  GraphProperty::sparse },
		[](Graph &G, int n) { createAlmostPlanarGraph(G, n, 2*n, 10); });

	testInstances("biconnected graph",
		{ GraphProperty::biconnected,
		  GraphProperty::simple,
		  GraphProperty::sparse },
		[](Graph &G, int n) {
			randomBiconnectedGraph(G, n, 2*n);
			splitParallelEdges(G);
		});

	testInstances("acyclic biconnected planar graph",
		{ GraphProperty::biconnected,
		  GraphProperty::planar,
		  GraphProperty::simple },
		[](Graph &G, int n) {
			randomPlanarBiconnectedDigraph(G, n, 2*n);
			splitParallelEdges(G);
		});

	testInstances("acyclic biconnected non-planar graph",
		{ GraphProperty::biconnected,
		  GraphProperty::nonPlanar,
		  GraphProperty::simple,
		  GraphProperty::sparse },
		[](Graph &G, int n) {
			randomBiconnectedGraph(G, n, 3*n - 5);
			splitParallelEdges(G);
		});

	testInstances("triconnected graph",
		{ GraphProperty::simple,
		  GraphProperty::triconnected },
		[](Graph &G, int n) { randomTriconnectedGraph(G, n, .5, .5); });

	testInstances("triconnected planar graph",
		{ GraphProperty::planar,
		  GraphProperty::simple,
		  GraphProperty::triconnected },
		[](Graph &G, int n) { randomPlanarTriconnectedGraph(G, n, .5, .5); });

	testInstances("maximal planar graph",
		{ GraphProperty::planar,
		  GraphProperty::simple,
		  GraphProperty::triconnected },
		[](Graph &G, int n) { randomPlanarBiconnectedGraph(G, n, 3*n - 6); });

	testInstances("disconnected planar graph",
		{ GraphProperty::planar,
		  GraphProperty::simple },
		[](Graph &G, int n) { createDisconnectedGraph(G, n, 1.4, 2.6, 3, 3); });

	testInstances("planar dense triconnected multi-graph",
		{ GraphProperty::planar,
		  GraphProperty::triconnected },
		[](Graph &G, int n) {
			randomPlanarTriconnectedGraph(G, n, .5, .5);
			addMultiEdges(G, .5);
		});

	testInstances("planar sparse triconnected multi-graph",
		{ GraphProperty::planar,
		  GraphProperty::sparse,
		  GraphProperty::triconnected },
		[](Graph &G, int n) {
			randomPlanarTriconnectedGraph(G, n, .5, .5);
			addMultiEdges(G, std::min(5.0/n, 0.95));
		});
}

inline void forEachGraphItWorks(std::set<GraphProperty> requirements,
		std::function<void(Graph&)> doTest,
		GraphSizes sizes = GraphSizes(),
		int minSize = 0,
		int maxSize = std::numeric_limits<int>::max()) {
	forEachGraphItWorks(
			requirements,
			[&](Graph& G, const std::string&, const std::set<GraphProperty>&) { doTest(G); },
			sizes, minSize, maxSize);
}

//! Shorthand for forEachGraphItWorks() with describable set to true.
inline void forEachGraphDescribe(std::set<GraphProperty> requirements,
		std::function<void(Graph&, const std::string &graphName, const std::set<GraphProperty>& props)> doTest,
		GraphSizes sizes = GraphSizes(),
		int minSize = 0,
		int maxSize = std::numeric_limits<int>::max()) {
	forEachGraphItWorks(requirements, doTest, sizes, minSize, maxSize, true);
}

inline void forEachGraphDescribe(std::set<GraphProperty> requirements,
		std::function<void(Graph&)> doTest,
		GraphSizes sizes = GraphSizes(),
		int minSize = 0,
		int maxSize = std::numeric_limits<int>::max()) {
	forEachGraphDescribe(
			requirements,
			[&](Graph& G, const std::string&, const std::set<GraphProperty>&) { doTest(G); },
			sizes, minSize, maxSize);
}
