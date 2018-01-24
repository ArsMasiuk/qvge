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

	//! Indicates instances that have a reasonably low number of edges.
	//! These graphs can, e.g., be used for planarization layouts without raising runtime too much.
	sparse
};

inline void insertGraph(Graph &g, const Graph &g2)
{
	NodeArray<node> map(g2);

	for(node v : g2.nodes)
		map[v] = g.newNode();

	for(edge e: g2.edges)
		g.newEdge(map[e->source()], map[e->target()]);
}

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

	for(int i = 0; i < cc; i++) {
		int m = ceil(randomDouble(densityMin*nBcMax, densityMax*nBcMax));
		Graph H;
		planarCNBGraph(H, nBcMax, m, bc);
		insertGraph(G, H);
	}
}

inline void createAlmostPlanarGraph(Graph &G, int n, int m, int add_m)
{
	planarBiconnectedGraph(G, n, m);

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
 * @param stepSize Increase for number of nodes between steps for randomly generated graphs.
 */
inline void forEachGraphItWorks(std::set<GraphProperty> requirements,
		std::function<void(const Graph&, const std::string &graphName, const std::set<GraphProperty>& props)> doTest,
		GraphSizes sizes = GraphSizes()) {
	auto testInstance = [&](const string& desc, std::set<GraphProperty> props, std::function<void(Graph&)> generateGraph) {
		performImplications(props);

		if (doesInclude(requirements, props)) {
			bandit::it("works on a " + desc, [&] {
				Graph G;
				generateGraph(G);
				doTest(G, desc, props);
			});
		}
	};

	auto testInstances = [&](const string& desc, std::set<GraphProperty> props, std::function<void(Graph&, int)> generateGraph) {
		sizes.forEachSize([&](int n) {
			testInstance(desc + " [n≈" + to_string(n) + "]", props, [&](Graph& G) { generateGraph(G, n); });
		});
	};

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
			for(int i = 0; i < 3; i++) {
				G.delEdge(G.chooseEdge());
			}
		});

	testInstances("3-regular arborescence",
		{ GraphProperty::arborescenceForest,
		  GraphProperty::connected,
		  GraphProperty::maxDeg4,
		  GraphProperty::simple },
		[](Graph &G, int n) { regularTree(G, n, 3); });

	testInstance("path-like tree",
		{ GraphProperty::connected,
		  GraphProperty::planar,
		  GraphProperty::simple },
		[](Graph& G) {
			std::ifstream ifs(RESOURCE_DIR + "/misc/path-like_tree.gml");
			GraphIO::read(G, ifs);
		});

	testInstance("K4",
		{ GraphProperty::maxDeg4,
		  GraphProperty::planar,
		  GraphProperty::simple,
		  GraphProperty::triconnected},
		[](Graph& G) { completeGraph(G, 4); });

	testInstance("K2,3",
		{ GraphProperty::maxDeg4,
		  GraphProperty::planar,
		  GraphProperty::simple,
		  GraphProperty::biconnected },
		[](Graph& G) { completeBipartiteGraph(G, 2, 3); });

	testInstance("K5",
		{ GraphProperty::nonPlanar,
		  GraphProperty::maxDeg4,
		  GraphProperty::simple,
		  GraphProperty::triconnected },
		[](Graph& G) { completeGraph(G, 5); });

	testInstance("K3,3",
		{ GraphProperty::nonPlanar,
		  GraphProperty::maxDeg4,
		  GraphProperty::simple,
		  GraphProperty::triconnected },
		[](Graph& G) { completeBipartiteGraph(G, 3, 3); });

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

	testInstances("connected planar graph",
		{ GraphProperty::connected,
		  GraphProperty::planar,
		  GraphProperty::simple },
		[](Graph &G, int n) { planarConnectedGraph(G, n, 2*n); });

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
			planarBiconnectedDiGraph(G, n, 2*n);
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
		[](Graph &G, int n) { planarTriconnectedGraph(G, n, .5, .5); });

	testInstances("maximal planar graph",
		{ GraphProperty::planar,
		  GraphProperty::simple,
		  GraphProperty::triconnected },
		[](Graph &G, int n) { planarBiconnectedGraph(G, n, 3*n - 6); });

	testInstances("disconnected planar graph",
		{ GraphProperty::planar,
		  GraphProperty::simple },
		[](Graph &G, int n) { createDisconnectedGraph(G, n, 1.4, 2.6, 3, 3); });

	testInstances("planar dense triconnected multi-graph",
		{ GraphProperty::planar,
		  GraphProperty::triconnected },
		[](Graph &G, int n) {
			planarTriconnectedGraph(G, n, .5, .5);
			addMultiEdges(G, .5);
		});

	testInstances("planar sparse triconnected multi-graph",
		{ GraphProperty::planar,
		  GraphProperty::sparse,
		  GraphProperty::triconnected },
		[](Graph &G, int n) {
			planarTriconnectedGraph(G, n, .5, .5);
			addMultiEdges(G, 5.0/n);
		});
}

inline void forEachGraphItWorks(std::set<GraphProperty> requirements,
		std::function<void(const Graph&)> doTest,
		GraphSizes sizes = GraphSizes()) {
	forEachGraphItWorks(
			requirements,
			[&](const Graph& G, const std::string&, const std::set<GraphProperty>&) { doTest(G); },
			sizes);
}
