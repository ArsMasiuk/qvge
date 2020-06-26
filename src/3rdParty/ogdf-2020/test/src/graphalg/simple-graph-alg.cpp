#include <set>
#include <ogdf/basic/Array.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/basic/simple_graph_alg.h>

#include <testing.h>
#include <graphs.h>

/**
 * Assert that there is a one-to-one mapping of values in assignedVals to values
 * in expectedVals, e.g. [3,1,1,2,0,3,1] <=> [2,0,0,3,1,2,0].
 * @tparam ArrayType is the type of assignedVals.
 *
 * @param assignedVals is the first array.
 * @param expVals is an initializer list with values for the second array.
 */
template<template<typename> class ArrayType>
void bijectiveMappingAssert(ArrayType<int> assignedVals, std::initializer_list<int> expVals)
{
	std::set<int> expSet(expVals);
	int size = expSet.size();
	Array<int> expectedVals(expVals);

	Array<int> expToAssign(0, size-1, -1);
	Array<int> assignToExp(0, size-1, -1);

	int i = 0;
	for (int assigned : assignedVals) {
		int expected = expectedVals[i++];

		AssertThat(assigned, IsGreaterThan(-1));

		if (expToAssign[expected] == -1) {
			AssertThat(assignToExp[assigned], Equals(-1));
			expToAssign[expected] = assigned;
			assignToExp[assigned] = expected;
		} else {
			AssertThat(assigned, Equals(expToAssign[expected]));
			AssertThat(expected, Equals(assignToExp[assigned]));
		}
	}
}

/**
 * Assert that calling biconnectedComponents() on G returns the correct number
 * of biconnected components and assigns the edges the correct biconnected
 * component ids. The exptected ids can differ from the assigned ids in value as
 * long as there is a one-to-one mapping of expected ids to assigned ids.
 *
 * @param G is the graph to be tested.
 * @param expCount is the expected number of biconnected components.
 * @param expectedComps is the expected biconnected component id for each edge.
 */
void biconnectedComponentsAssert(Graph &G, int expCount, std::initializer_list<int> expectedComps)
{
	EdgeArray<int> comps(G,-1);
	int nonEmptyBiComps;
	AssertThat(biconnectedComponents(G, comps, nonEmptyBiComps), Equals(expCount));

	bijectiveMappingAssert(comps, expectedComps);

	// nonEmptyBiComps-1 should be equal to max(component).
	int maxUsedIndex = 0;
	for (int c: comps) {
		maxUsedIndex = std::max(maxUsedIndex, c);
	}
	AssertThat(maxUsedIndex, Equals(nonEmptyBiComps - 1));
}

/**
 * Assert that calling strongComponents() on G returns the correct number
 * of strong components and assigns the nodes the correct strong component ids.
 * The exptected ids can differ from the assigned ids in value as long as there
 * is a one-to-one mapping of expected ids to assigned ids.
 *
 * @param G is the graph to be tested.
 * @param expectedComps is the expected strong component id for each node.
 */
void strongComponentsAssert(Graph &G, std::initializer_list<int> expectedComps)
{
	std::set<int> expSet(expectedComps);
	int expCount = expSet.size();
	NodeArray<int> comps(G,-1);
	AssertThat(strongComponents(G, comps), Equals(expCount));
	bijectiveMappingAssert(comps, expectedComps);
}

//! Returns whether a path from source to target exists in the graph.
bool pathExists(const Graph &graph, const node source, const node target)
{
	OGDF_ASSERT(source != target);
	OGDF_ASSERT(source->graphOf() == &graph);
	OGDF_ASSERT(target->graphOf() == &graph);

	List<node> queue;
	NodeArray<bool> visited(graph, false);
	visited[source] = true;
	queue.pushBack(source);

	bool result = false;
	while(!queue.empty() && !result) {
		node v = queue.popFrontRet();
		for(adjEntry adj : v->adjEntries) {
			node w = adj->theEdge()->target();
			if(!visited[w]) {
				result |= w == target;
				visited[w] = true;
				queue.pushBack(w);
			}
		}
	}

	return result;
}

/**
 * Assert that isAcylic()/isAcyclicUndirected() returns the correct value and
 * that the list of collected backedges is filled correctly. For cyclic graphs
 * assert that removing all backedges makes the graph acyclic but maintains
 * connectivity.
 *
 * @param G is the graph to be tested (will be copied)
 * @param directed sets whether isAcyclic() or isAcyclicUndirected() is tested.
 * @param expected is the expected result of the function call.
 */
void isAcyclicAssert(Graph G, bool directed, bool expected)
{
	List<edge> backedges;
	bool result = directed ?
	              isAcyclic(G, backedges) : isAcyclicUndirected(G, backedges);

	if (expected) {
		AssertThat(result, IsTrue());
		AssertThat(backedges.empty(), IsTrue());
	} else {
		AssertThat(result, IsFalse());
		AssertThat(backedges.size(), IsGreaterThan(0));
		AssertThat(backedges.size(), IsLessThan(G.numberOfEdges() + 1));

		bool connected = isConnected(G);

		for (edge e : backedges) {
			G.delEdge(e);
		}

		result = directed ?
		         isAcyclic(G, backedges) : isAcyclicUndirected(G, backedges);
		AssertThat(result, IsTrue());
		AssertThat(backedges.empty(), IsTrue());
		AssertThat(isConnected(G), Equals(connected));
	}
}

/**
 * Perform tests for isAcylic() or isAcyclicUndirected().
 *
 * @param directed sets whether isAcyclic() or isAcyclicUndirected() is tested.
 */
void describeIsAcyclic(bool directed)
{
	Graph G;

	before_each([&](){
		G.clear();
	});

	forEachGraphItWorks(directed ?
			std::set<GraphProperty>({GraphProperty::acyclic}) :
			std::set<GraphProperty>({GraphProperty::arborescenceForest}),
			[&](const Graph& testG) {
		isAcyclicAssert(testG, directed, true);
	});

	if (!directed) {
		forEachGraphItWorks({GraphProperty::biconnected} , [&](const Graph& testG) {
			bool acylic = testG.numberOfNodes() <= 2 && isSimpleUndirected(testG);
			isAcyclicAssert(testG, directed, acylic);
		});
	}

	it("works on a cyclic graph", [&](){
		customGraph(G, 3, {{0,1}, {1,2}, {2,1}});
		isAcyclicAssert(G, directed, false);
	});

	it("works on a disconnected cyclic graph", [&](){
		customGraph(G, 4, {{1,2}, {2,3}, {3,1}});
		isAcyclicAssert(G, directed, false);
	});

	it("works on an acyclic graph requiring multiple dfs starts if directed", [&](){
		customGraph(G, 4, {{0,1}, {1,2}, {3,1}});
		isAcyclicAssert(G, directed, true);
	});

	it("works on a cyclic graph requiring multiple dfs starts if directed", [&](){
		customGraph(G, 4, {{0,1}, {1,2}, {2,0}, {3,1}});
		isAcyclicAssert(G, directed, false);
	});

	it("works on a directed acyclic but undirected cyclic graph", [&](){
		customGraph(G, 3, {{0,1}, {0,2}, {1,2}});
		isAcyclicAssert(G, directed, directed);
	});

	it("works on an extremely large acyclic graph", [&](){
		randomTree(G, 125000, 1, 0);
		isAcyclicAssert(G, directed, true);
	});

	it("works on an extremely large cyclic graph", [&](){
		randomBiconnectedGraph(G, 125000, 250000);
		isAcyclicAssert(G, directed, false);
	});
}

static void describeIsTwoEdgeConnected() {
	forEachGraphItWorks({GraphProperty::biconnected} , [&](const Graph& testG) {
		edge bridge = testG.lastEdge();
		bool isSingleEdge = testG.numberOfNodes() == 2 && isParallelFreeUndirected(testG);
		AssertThat(isTwoEdgeConnected(testG, bridge), Equals(!isSingleEdge));
		AssertThat(bridge == nullptr, Equals(!isSingleEdge));
		if (isSingleEdge) {
			AssertThat(bridge->isSelfLoop(), IsFalse());
		}
	});

	forEachGraphItWorks({GraphProperty::arborescenceForest} , [&](const Graph& testG) {
		edge bridge = nullptr;
		bool connected = isConnected(testG);
		bool twoEdgeConnected = connected && testG.numberOfNodes() <= 1;
		AssertThat(isTwoEdgeConnected(testG, bridge), Equals(twoEdgeConnected));
		if (connected && !twoEdgeConnected) {
			AssertThat(bridge, !IsNull());
		} else {
			AssertThat(bridge, IsNull());
		}
	});

	it("works on a connected but not two-edge-connected graph", [&](){
		Graph G;
		Array<node> nodes;
		customGraph(G, 7, {
			{0,1},{0,2},{1,2},{3,4},{4,5},{5,6},{6,2},{6,3}
		}, nodes);
		node v = nodes[6];
		node u = nodes[2];
		edge e = G.searchEdge(u,v);
		edge bridge = nullptr;
		AssertThat(isTwoEdgeConnected(G, bridge), IsFalse());
		AssertThat(bridge, Equals(e));
	});

	it("works on an extremely large tree", [&](){
		Graph G;
		randomTree(G, 250000);
		AssertThat(isTwoEdgeConnected(G), IsFalse());
	});

	it("works on an extremely large 2-edge-connected graph", [&](){
		Graph G;
		randomBiconnectedGraph(G, 250000, 500000);
		AssertThat(isTwoEdgeConnected(G), IsTrue());
	});
}

static void describeIsBiconnected() {
	forEachGraphItWorks({GraphProperty::biconnected} , [](const Graph& G) {
		node cutVertex = G.firstNode();
		AssertThat(isBiconnected(G, cutVertex), IsTrue());
		AssertThat(cutVertex, IsNull());
	});

	forEachGraphItWorks({GraphProperty::arborescenceForest} , [](const Graph& G) {
		node cutVertex = G.firstNode();
		bool connected = isConnected(G);
		bool biconnected = connected && G.numberOfNodes() <= 2;
		AssertThat(isBiconnected(G, cutVertex), Equals(biconnected));
		if (!biconnected && connected) {
			AssertThat(cutVertex, !IsNull());
		} else {
			AssertThat(cutVertex, IsNull());
		}
	});

	it("works on an extremely large tree", [&](){
		Graph G;
		randomTree(G, 250000);
		AssertThat(isBiconnected(G), IsFalse());
	});

	it("works on an extremely large biconnected graph", [&](){
		Graph G;
		randomBiconnectedGraph(G, 250000, 500000);
		AssertThat(isBiconnected(G), IsTrue());
	});
}

static void describeMakeBiconnected() {
	forEachGraphItWorks({}, [](Graph &G) {
		List<edge> added;
		NodeArray<int> comps(G);
		int numComps = connectedComponents(G, comps);
		bool wasBiconnected = isBiconnected(G);

		makeBiconnected(G, added);
		AssertThat(isBiconnected(G), IsTrue());
		AssertThat(added.empty(), Equals(wasBiconnected));
		if (!wasBiconnected) {
			if (G.numberOfNodes() == 2) {
				AssertThat(added.size(), Equals(1));
			} else {
				AssertThat(added.size(), IsGreaterThanOrEqualTo(numComps));
			}
		}
	});

	it("works on an extremely large graph", [](){
		Graph G;
		List<edge> added;
		emptyGraph(G, 250000);
		AssertThat(isBiconnected(G), IsFalse());

		// A graph with n nodes needs at least n edges to be biconnected
		makeBiconnected(G, added);
		AssertThat(isBiconnected(G), IsTrue());
		AssertThat(added.size(), IsGreaterThan(250000));
	});
}

static void describeBiconnectedComponents() {
	Graph G;

	before_each([&](){
		G.clear();
	});

	forEachGraphItWorks({GraphProperty::biconnected, GraphProperty::simple}, [&](const Graph& testG) {
		EdgeArray<int> component(testG,-1);
		AssertThat(biconnectedComponents(testG, component), Equals(testG.numberOfNodes() == 0 ? 0 : 1));
	});

	it("works on a graph with a self-loop", [&](){
		customGraph(G, 2, {{0,0}, {0,1}});
		auto expectedComps = {0,1};
		biconnectedComponentsAssert(G, 2, expectedComps);
	});

	it("works on a disconnected graph", [&](){
		customGraph(G, 3, {{0,1}});
		auto expectedComps = {0};
		biconnectedComponentsAssert(G, 2, expectedComps);
	});

	it("works on a connected but not biconnected graph", [&](){
		customGraph(G, 3, {{0,1}, {0,2}});
		auto expectedComps = {0,1};
		biconnectedComponentsAssert(G, 2, expectedComps);
	});

	it("works on a biconnected graph", [&](){
		completeGraph(G, 3);
		auto expectedComps = {0,0,0};
		biconnectedComponentsAssert(G, 1, expectedComps);
	});

	it("works on a graph with 2 biconnected components", [&](){
		customGraph(G, 4, {{0,1}, {0,2}, {1,2}, {0,3}});
		auto expectedComps = {0,0,0,1};
		biconnectedComponentsAssert(G, 2, expectedComps);
	});

	it("works on a graph with 4 biconnected components", [&](){
		customGraph(G, 10, {{0,1}, {1,2}, {2,3}, {3,1}, {3,4}, {4,1}, {1,5}, {5,6}, {6,0}, {0,7}, {7,8}, {8,9}, {9,7}});
		auto expectedComps = {0,1,1,1,1,1,0,0,0,2,3,3,3};
		biconnectedComponentsAssert(G, 4, expectedComps);
	});

	it("works on a graph with 5 biconnected components", [&](){
		customGraph(G, 12, {{0,1}, {1,2}, {2,3}, {3,4}, {4,2}, {3,1}, {1,5}, {5,6}, {6,0}, {5,7}, {7,8}, {5,8}, {8,9}, {10,11}});
		auto expectedComps = {0,1,1,1,1,1,0,0,0,2,2,2,3,4};
		biconnectedComponentsAssert(G, 5, expectedComps);
	});

	it("works on an extremely large graph", [&](){
		randomGraph(G, 250000, 500000);

		EdgeArray<int> component(G,-1);
		NodeArray<int> conComp(G);
		int result = biconnectedComponents(G,component);

		AssertThat(result, IsGreaterThan(0));
		AssertThat(result, !IsLessThan(connectedComponents(G,conComp)));
		for (edge e : G.edges) {
			AssertThat(component[e], IsGreaterThan(-1));
		}
	});

	it("works on an extremely large biconnected graph", [&](){
		randomBiconnectedGraph(G, 250000, 500000);

		EdgeArray<int> component(G,-1);
		AssertThat(biconnectedComponents(G,component), Equals(1));
		for (edge e : G.edges) {
			AssertThat(component[e], Equals(0));
		}
	});
}

static void describeStrongComponents() {
	Graph G;

	before_each([&](){
		G.clear();
	});

	forEachGraphItWorks({GraphProperty::acyclic}, [](const Graph& testG) {
		NodeArray<int> component(testG,-1);
		AssertThat(strongComponents(testG, component), Equals(testG.numberOfNodes()));
	});

	it("works on a graph with a self-loop", [&](){
		customGraph(G, 2, {{0,0}, {0,1}});
		auto expectedComps = {0,1};
		strongComponentsAssert(G, expectedComps);
	});

	it("works on a strongly connected graph", [&](){
		customGraph(G, 3, {{0,1}, {1,2}, {2,0}});
		auto expectedComps = {0,0,0};
		strongComponentsAssert(G, expectedComps);
	});

	it("works on a graph with 2 strongly connected components", [&](){
		customGraph(G, 4, {{0,1}, {1,2}, {2,0}, {0,3}});
		auto expectedComps = {0,0,0,1};
		strongComponentsAssert(G, expectedComps);
	});

	it("works on a graph with 3 strongly connected components", [&](){
		customGraph(G, 10, {{0,1}, {1,2}, {2,3}, {3,1}, {3,4}, {4,1}, {0,5}, {5,6}, {6,0}, {0,7}, {7,8}, {8,9}, {9,7}});
		auto expectedComps = {0,1,1,1,1,0,0,2,2,2};
		strongComponentsAssert(G, expectedComps);
	});

	it("works on a graph with 5 strongly connected components", [&](){
		customGraph(G, 12, {{0,1}, {1,2}, {2,3}, {3,4}, {4,2}, {1,3}, {1,5}, {5,6}, {6,0}, {5,7}, {7,8}, {8,5}, {8,9}, {10,11}});
		auto expectedComps = {0,0,1,1,1,0,0,0,0,2,3,4};
		strongComponentsAssert(G, expectedComps);
	});

	it("works on a graph with overlapping circles", [&](){
		customGraph(G, 8, {{2,5}, {3,6}, {4,7}, {5,4}, {6,5}, {6,1}, {7,2}, {7,3}, {7,6}});
		auto expectedComps = {0,1,2,2,2,2,2,2};
		strongComponentsAssert(G, expectedComps);
	});

	it("works on an extremely large graph", [&](){
		randomGraph(G, 250000, 500000);

		NodeArray<int> component(G,-1);
		NodeArray<int> conComp(G);
		int result = strongComponents(G,component);

		AssertThat(result, IsGreaterThan(0));
		AssertThat(result, !IsLessThan(connectedComponents(G,conComp)));
		for (node v : G.nodes) {
			AssertThat(component[v], IsGreaterThan(-1));
		}
	});

	it("works on an extremely large strongly connected graph", [&](){
		randomBiconnectedGraph(G, 250000, 250000);

		// Ensure that G is strongly connected.
		List<edge> edges;
		G.allEdges(edges);
		for (edge e : edges) {
			G.newEdge(e->target(), e->source());
		}

		NodeArray<int> component(G,-1);
		AssertThat(strongComponents(G,component), Equals(1));
		for (node v : G.nodes) {
			AssertThat(component[v], Equals(0));
		}
	});

	for(int n = 0; n < 75; n++) {
		it("works on a random graph of size " + to_string(n), [&](){
			Graph graph;
			randomDigraph(graph, n, randomDouble(0, 1));

			NodeArray<int> components(graph);
			int nComponents = strongComponents(graph, components);
			for (node v = graph.firstNode(); v; v = v->succ()) {
				for (node w = v->succ(); w; w = w->succ()) {
					AssertThat(components[v], IsGreaterThan(-1) && IsLessThan(nComponents));
					AssertThat(components[w], IsGreaterThan(-1) && IsLessThan(nComponents));
					if (components[v] == components[w]) {
						AssertThat(pathExists(graph, v, w), IsTrue());
						AssertThat(pathExists(graph, w, v), IsTrue());
					} else {
						AssertThat(pathExists(graph, w, v) && pathExists(graph, v, w), IsFalse());
					}
				}
			}
		});
	}
}

static void describeIsArborescenceForest() {
	Graph G;
	List<node> roots;

	before_each([&](){
		G.clear();
		roots.clear();
	});

	forEachGraphItWorks({GraphProperty::arborescenceForest}, [&](const Graph& testG) {
		NodeArray<int> comps(testG);
		int numComps = connectedComponents(testG, comps);
		AssertThat(isArborescenceForest(testG, roots), IsTrue());
		AssertThat(roots.size(), Equals(numComps));
	});

	forEachGraphItWorks({GraphProperty::biconnected}, [&](const Graph& testG) {
		AssertThat(isArborescenceForest(testG, roots), Equals(testG.numberOfNodes() <= 2 && isSimpleUndirected(testG)));
	});

	it("works on a graph without a source", [&](){
		customGraph(G, 2, {{0,0}, {0,1}});
		AssertThat(isArborescenceForest(G, roots), IsFalse());
	});

	it("works on a graph with one tree and one cyclic subgraph", [&](){
		customGraph(G, 5, {{0,1}, {2,3}, {3,4}, {4,2}});
		AssertThat(isArborescenceForest(G, roots), IsFalse());
	});

	it("works on a directed tree that is not an arborescence", [&](){
		customGraph(G, 4, {{0,1}, {1,2}, {3,1}});
		AssertThat(isArborescenceForest(G, roots), IsFalse());
	});

	it("works on an extremely large biconnected graph", [&](){
		randomBiconnectedGraph(G, 250000, 500000);
		AssertThat(isArborescenceForest(G, roots), IsFalse());
	});

	it("works on an extremely large arborescence", [&](){
		constexpr int n = 125000;
		node nodes[n];
		nodes[0] = G.newNode();

		for (int i = 1; i < n; i++) {
			nodes[i] = G.newNode();
			G.newEdge(nodes[randomNumber(0, i-1)], nodes[i]);
		}
		AssertThat(isArborescenceForest(G, roots), IsTrue());
		AssertThat(roots.size(), Equals(1));
		AssertThat(roots.front(), Equals(G.firstNode()));
	});

	it("works on an extremely large path", [&](){
		node v = G.newNode();
		for (int i = 0; i < 125000; i++) {
			node w = G.newNode();
			G.newEdge(v, w);
			v = w;
		}
		AssertThat(isArborescenceForest(G, roots), IsTrue());
		AssertThat(roots.size(), Equals(1));
		AssertThat(roots.front(), Equals(G.firstNode()));
	});
}

static void describeIsBipartite() {
	forEachGraphItWorks({GraphProperty::arborescenceForest}, [&](const Graph& G) {
		NodeArray<bool> color(G, false);
		AssertThat(isBipartite(G, color), IsTrue());
		for (node v : G.nodes) {
			for (adjEntry adj : v->adjEntries) {
				AssertThat(color[v], !Equals(color[adj->twinNode()]));
			}
		}
	});

	it("works on a disconnected non-bipartite graph", [&](){
		Graph G;
		customGraph(G, 4, {{1,2}, {2,3}, {3,1}});
		AssertThat(isBipartite(G), IsFalse());
	});

	it("works on a bipartite graph with multi-edges", [&](){
		Graph G;
		NodeArray<bool> color(G, false);
		Array<node> nodes;
		customGraph(G, 3, {{0,1}, {1,0}, {1,2}}, nodes);
		AssertThat(isBipartite(G, color), IsTrue());
		AssertThat(color[nodes[0]], !Equals(color[nodes[1]]));
		AssertThat(color[nodes[1]], !Equals(color[nodes[2]]));
		AssertThat(color[nodes[0]], Equals(color[nodes[2]]));
	});

	it("works on a non-bipartite graph with multi-edges", [&](){
		Graph G;
		customGraph(G, 4, {{1,2}, {2,3}, {3,1}});
		AssertThat(isBipartite(G), IsFalse());
	});

	it("works on a graph with a self-loop", [&](){
		Graph G;
		customGraph(G, 2, {{0,1}, {1,1}});
		AssertThat(isBipartite(G), IsFalse());
	});

	it("works on an extremely large tree", [&](){
		Graph G;
		randomTree(G, 250000);
		AssertThat(isBipartite(G), IsTrue());
	});

	it("works on an extremely large non-bipartite graph", [&](){
		Graph G;
		randomTree(G, 250000);
		node u = G.chooseNode();
		node v = G.chooseNode();
		node w = G.chooseNode();
		G.newEdge(u, v);
		G.newEdge(u, w);
		G.newEdge(v, w);
		AssertThat(isBipartite(G), IsFalse());
	});
}

static void describeNodeDistribution() {
	it("can compute an indegree distribution", [] {
		Graph G;
		customGraph(G, 3, {{0, 1}, {1, 2}, {2, 0}});
		Array<int> dist;
		nodeDistribution(G, dist, [](node v) {
			return v->indeg();
		});
		AssertThat(dist.low(), Equals(1));
		AssertThat(dist.size(), Equals(1));
		AssertThat(dist[1], Equals(3));
	});

	it("can compute the number of nodes that belong to connected components", [] {
		Graph G;
		customGraph(G, 4, {{0, 0}, {1, 2}});
		NodeArray<int> comp(G);
		Array<int> dist;
		connectedComponents(G, comp);
		nodeDistribution(G, dist, comp);
		AssertThat(dist.low(), Equals(0));
		AssertThat(dist.size(), Equals(3));
		AssertThat(dist[0] + dist[1] + dist[2], Equals(G.numberOfNodes()));
	});
}

static void describeDegreeDistribution() {
	forEachGraphItWorks({GraphProperty::maxDeg4}, [&](const Graph& testG) {
		Array<int> dist;
		degreeDistribution(testG, dist);
		AssertThat(dist.size() <= 4, IsTrue());
		AssertThat(dist.empty() == testG.empty(), IsTrue());
	});

	it("works on isolated nodes", [] {
		Graph G;
		emptyGraph(G, 100);
		Array<int> dist;
		degreeDistribution(G, dist);
		AssertThat(dist.low(), Equals(0));
		AssertThat(dist.size(), Equals(1));
		AssertThat(dist[0], Equals(100));
	});

	it("works on a complete graph", [] {
		Graph G;
		const int n = 12;
		completeGraph(G, n);
		Array<int> dist;
		degreeDistribution(G, dist);
		AssertThat(dist.low(), Equals(n-1));
		AssertThat(dist.size(), Equals(1));
		AssertThat(dist[n-1], Equals(n));
	});

	it("works on an isolated node with a lot of self-loops", [] {
		Graph G;
		node v = G.newNode();
		const int n = 42;
		for (int i = 0; i < n; ++i) {
			G.newEdge(v, v);
		}
		Array<int> dist;
		degreeDistribution(G, dist);
		AssertThat(dist.low(), Equals(2*n));
		AssertThat(dist.size(), Equals(1));
		AssertThat(dist[2*n], Equals(1));
	});

	it("works with a very untypical distribution", [] {
		Graph G;
		const int n = 30;
		completeGraph(G, n);
		for (int i = 0; i < n; ++i) {
			node u = G.newNode();
			node v = G.newNode();
			G.newEdge(u, v);
		}
		Array<int> dist;
		degreeDistribution(G, dist);
		AssertThat(dist.low(), Equals(1));
		AssertThat(dist.high(), Equals(n-1));
		AssertThat(dist[dist.low()], Equals(2*n));
		for (int i = dist.low() + 1; i < dist.high(); ++i) {
			AssertThat(dist[i], Equals(0));
		}
		AssertThat(dist[dist.high()], Equals(n));
	});

	it("works with a multigraph", [] {
		Graph G;
		customGraph(G, 7,
		  {{0, 1}, {1, 2}, {2, 3}, {2, 4}, {3, 4}, {3, 4}, {3, 5}, {4, 5}, {4, 5}, {5, 5}});
		Array<int> dist;
		degreeDistribution(G, dist);
		AssertThat(dist.low(), Equals(0));
		AssertThat(dist.high(), Equals(5));
		for (int i = dist.low(); i < dist.high(); ++i) {
			AssertThat(dist[i], Equals(1));
		}
		AssertThat(dist[dist.high()], Equals(2));
	});
}

static void describeRemoveSelfLoops() {
	it("leaves a single node with no further edges unchanged", [] {
		Graph G;
		node v{G.newNode()};
		removeSelfLoops(G, v);
		AssertThat(v->degree(), Equals(0));
	});

	it("removes all incident edges on a single node with only self-loops", [] {
		Graph G;
		node v{G.newNode()};
		for (int i{0}; i < 10; ++i) {
			G.newEdge(v, v);
		}
		removeSelfLoops(G, v);
		AssertThat(v->degree(), Equals(0));
	});

	it("removes no edges when there are no self-loops", [] {
		Graph G;
		Array<node> nodes;
		customGraph(G, 3, {{0, 1}, {1, 2}, {2, 0}, {2, 1}, {1, 0}, {1, 2}}, nodes);
		for (int i{0}; i < 3; ++i) {
			removeSelfLoops(G, nodes[i]);
			AssertThat(G.numberOfEdges(), Equals(6));
		};
	});

	using CaseEdges = List<std::pair<int,int>>;
	struct CaseType {
		std::string removalDesc;
		CaseEdges edges;
	};

	List<CaseType> cases{
			{"one self-loop if it is the first incident edge of a node",
			 CaseEdges{{1, 1}, {0, 1}, {1, 2}, {2, 0}}},
			{"one self-loop if it is the last incident edge of a node",
			 CaseEdges{{0, 1}, {1, 2}, {2, 0}, {1, 1}}},
			{"one self-loop if it is neither the first nor the last incident edge of a node",
			 CaseEdges{{0, 1}, {1, 1}, {1, 2}, {2, 0}}},
			{"three self-loops that are non-consecutive in the incidence list of the node",
			 CaseEdges{{1, 1}, {0, 1}, {1, 1}, {1, 2}, {1, 1}, {2, 0}}},
	};

	for (CaseType testcase : cases) {
		it("removes " + testcase.removalDesc, [&] {
			Graph G;
			Array<node> nodes;
			customGraph(G, 3, testcase.edges, nodes);
			removeSelfLoops(G, nodes[1]);
			for (node v : G.nodes) {
				AssertThat(v->degree(), Equals(2));
			}
			for (edge e : G.edges) {
				AssertThat(e->isSelfLoop(), IsFalse());
			}
		});
	}
}

static void describeMakeLoopFree(bool withList) {
	forEachGraphItWorks({}, [&](Graph& G) {
		int prevEdges{G.numberOfEdges()};
		bool wasLoopFree{isLoopFree(G)};
		List<node> nodes;

		if (withList) {
			makeLoopFree(G, nodes);
		} else {
			makeLoopFree(G);
		}

		AssertThat(isLoopFree(G), IsTrue());
		if (withList || wasLoopFree) {
			AssertThat(nodes.size(), Equals(prevEdges - G.numberOfEdges()));
		}
	});
}

go_bandit([]() {
	describe("Simple Graph Algorithms", [](){
		describe("isTwoEdgeConnected", [](){
			describeIsTwoEdgeConnected();
		});

		describe("isBiconnected", [](){
			describeIsBiconnected();
		});

		describe("makeBiconnected", [](){
			describeMakeBiconnected();
		});

		describe("biconnectedComponents", [](){
			describeBiconnectedComponents();
		});

		describe("strongComponents", [](){
			describeStrongComponents();
		});

		describe("isAcyclic", [](){
			describeIsAcyclic(true);
		});

		describe("isAcyclicUndirected", [](){
			describeIsAcyclic(false);
		});

		describe("isArborescenceForest", [](){
			describeIsArborescenceForest();
		});

		describe("isBipartite", [](){
			describeIsBipartite();
		});

		describe("nodeDistribution", [] {
			describeNodeDistribution();
		});

		describe("degreeDistribution", [] {
			describeDegreeDistribution();
		});

		describe("removeSelfLoops", [] {
			describeRemoveSelfLoops();
		});

		describe("makeLoopFree", [] {
			describe("without node list", [] {
				describeMakeLoopFree(false);
			});
			describe("with node list", [] {
				describeMakeLoopFree(true);
			});
		});
	});
});
