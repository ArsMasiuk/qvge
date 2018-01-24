#include <set>
#include <ogdf/basic/Array.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/basic/simple_graph_alg.h>

#include <testing.h>

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

/**
 * Assert that isAcylic()/isAcyclicUndirected() returns the correct value and
 * that the list of collected backedges is filled correctly. For cyclic graphs
 * assert that removing all backedges makes the graph acyclic but maintains
 * connectivity.
 *
 * @param G is the graph to be tested.
 * @param directed sets whether isAcyclic() or isAcyclicUndirected() is tested.
 * @param expected is the expected result of the function call.
 */
void isAcyclicAssert(Graph &G, bool directed, bool expected)
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

	it("works on an empty graph", [&](){
		emptyGraph(G, 0);
		isAcyclicAssert(G, directed, true);
	});

	it("works on a graph with a single node", [&](){
		G.newNode();
		isAcyclicAssert(G, directed, true);
	});

	it("works on a graph with a self-loop", [&](){
		customGraph(G, 1, {{0,0}});
		isAcyclicAssert(G, directed, false);
	});

	it("works on a graph with parallel edges", [&](){
		customGraph(G, 2, {{0,1}, {1,0}});
		isAcyclicAssert(G, directed, false);
	});

	it("works on an acylic graph", [&](){
		customGraph(G, 3, {{0,1}, {0,2}});
		isAcyclicAssert(G, directed, true);
	});

	it("works on a cyclic graph", [&](){
		customGraph(G, 3, {{0,1}, {1,2}, {2,1}});
		isAcyclicAssert(G, directed, false);
	});

	it("works on a disconnected acyclic graph", [&](){
		customGraph(G, 4, {{1,2}, {1,3}});
		isAcyclicAssert(G, directed, true);
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

go_bandit([]() {
	describe("Simple Graph Algorithms", [](){
		describe("isTwoEdgeConnected", [](){
			it("works on an empty graph", [&](){
				Graph G;
				AssertThat(isTwoEdgeConnected(G), IsTrue());
			});

			it("works on a graph with one node", [&](){
				Graph G;
				G.newNode();
				AssertThat(isTwoEdgeConnected(G), IsTrue());
			});

			it("works on a graph with two nodes", [&](){
				Graph G;
				customGraph(G, 2, {{0,1}});
				AssertThat(isTwoEdgeConnected(G), IsFalse());
			});

			it("works on a disconnected graph", [&](){
				Graph G;
				customGraph(G, 5, {{0,1},{0,2},{1,2},{3,4}});
				edge bridge = G.chooseEdge();
				AssertThat(isTwoEdgeConnected(G, bridge), IsFalse());
				AssertThat(bridge, Equals(nullptr));
			});

			it("works on a tree", [&](){
				Graph G;
				customGraph(G, 5, {{0,1},{1,2},{1,3},{3,4}});
				edge bridge = nullptr;
				AssertThat(isTwoEdgeConnected(G, bridge), IsFalse());
				AssertThat(bridge, !Equals(nullptr));
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

			it("works on a triangle", [&](){
				Graph G;
				customGraph(G, 3, {{0,1},{1,2},{2,0}});
				edge bridge = G.chooseEdge();
				AssertThat(isTwoEdgeConnected(G, bridge), IsTrue());
				AssertThat(bridge, Equals(nullptr));
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

			it("works with selfloops", [&](){
				Graph G;
				customGraph(G, 1, {{0,0}});
				AssertThat(isTwoEdgeConnected(G), IsTrue());
			});

			it("works with multiedges", [&](){
				Graph G;
				customGraph(G, 2, {{0,1},{0,1}});
				AssertThat(isTwoEdgeConnected(G), IsTrue());
			});
		});

		describe("isBiconnected", [](){
			Graph G;
			node cutVertex;

			before_each([&](){
				G.clear();
				cutVertex = nullptr;
			});

			it("works on an empty graph", [&](){
				AssertThat(isTwoEdgeConnected(G), IsTrue());
			});

			it("works on a graph with one node", [&](){
				G.newNode();
				AssertThat(isBiconnected(G), IsTrue());
			});

			it("works on a path of two nodes", [&](){
				customGraph(G, 2, {{0,1}});
				AssertThat(isBiconnected(G), IsTrue());
			});

			it("works on a disconnected graph", [&](){
				customGraph(G, 3, {{0,1}});
				AssertThat(isBiconnected(G, cutVertex), IsFalse());
				AssertThat(cutVertex, Equals(nullptr));
			});

			it("works on a connected but not biconnected graph", [&](){
				customGraph(G, 3, {{0,1}, {0,2}});
				AssertThat(isBiconnected(G, cutVertex), IsFalse());
				AssertThat(cutVertex, Equals(G.firstNode()));
			});

			it("works on a simple biconnected graph", [&](){
				completeGraph(G, 3);
				AssertThat(isBiconnected(G, cutVertex), IsTrue());
				AssertThat(cutVertex, Equals(nullptr));
			});

			it("works on an extremely large tree", [&](){
				randomTree(G, 250000);
				AssertThat(isBiconnected(G), IsFalse());
			});

			it("works on an extremely large biconnected graph", [&](){
				randomBiconnectedGraph(G, 250000, 500000);
				AssertThat(isBiconnected(G), IsTrue());
			});
		});

		describe("makeBiconnected", [](){
			Graph G;
			List<edge> added;

			before_each([&](){
				G.clear();
				added.clear();
			});

			it("works on a disconnected graph", [&](){
				customGraph(G, 3, {{0,1}});
				makeBiconnected(G, added);
				AssertThat(isBiconnected(G), IsTrue());
				AssertThat(added.size(), Equals(2));
			});

			it("works on a connected but not biconnected graph", [&](){
				customGraph(G, 3, {{0,1}, {0,2}});
				makeBiconnected(G, added);
				AssertThat(isBiconnected(G), IsTrue());
				AssertThat(added.size(), Equals(1));
			});

			it("works on a simple biconnected graph", [&](){
				randomBiconnectedGraph(G, 10, 20);
				AssertThat(isBiconnected(G), IsTrue());

				makeBiconnected(G, added);
				AssertThat(isBiconnected(G), IsTrue());
				AssertThat(added.empty(), IsTrue());
			});

			it("works on an extremely large graph", [&](){
				emptyGraph(G, 250000);
				AssertThat(isBiconnected(G), IsFalse());

				// A graph with n nodes needs at least n edges to be biconnected
				makeBiconnected(G, added);
				AssertThat(isBiconnected(G), IsTrue());
				AssertThat(added.size(), IsGreaterThan(250000));
			});
		});

		describe("biconnectedComponents", [](){
			Graph G;

			before_each([&](){
				G.clear();
			});

			it("works on an empty graph", [&](){
				EdgeArray<int> component(G,-1);
				emptyGraph(G, 0);
				AssertThat(biconnectedComponents(G, component), Equals(0));
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
		});

		describe("strongComponents", [](){
			Graph G;

			before_each([&](){
				G.clear();
			});

			it("works on an empty graph", [&](){
				NodeArray<int> component(G,-1);
				emptyGraph(G, 0);
				AssertThat(strongComponents(G, component), Equals(0));
			});

			it("works on a graph with a self-loop", [&](){
				customGraph(G, 2, {{0,0}, {0,1}});
				auto expectedComps = {0,1};
				strongComponentsAssert(G, expectedComps);
			});

			it("works on a disconnected graph", [&](){
				customGraph(G, 3, {{0,1}});
				auto expectedComps = {0,1,2};
				strongComponentsAssert(G, expectedComps);
			});

			it("works on a connected but not strongly connected graph", [&](){
				customGraph(G, 3, {{0,1}, {0,2}});
				auto expectedComps = {0,1,2};
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
		});

		describe("isAcyclic", [](){
			describeIsAcyclic(true);
		});

		describe("isAcyclicUndirected", [](){
			describeIsAcyclic(false);
		});

		describe("isArborescenceForest", [](){
			Graph G;
			List<node> roots;

			before_each([&](){
				G.clear();
				roots.clear();
			});

			it("works on an empty graph", [&](){
				emptyGraph(G, 0);
				AssertThat(isArborescenceForest(G, roots), IsTrue());
				AssertThat(roots.empty(), IsTrue());
			});

			it("works on a graph with a single node", [&](){
				G.newNode();
				AssertThat(isArborescenceForest(G, roots), IsTrue());
				AssertThat(roots.size(), Equals(1));
				AssertThat(roots.front(), Equals(G.firstNode()));
			});

			it("works on a graph with a self-loop", [&](){
				customGraph(G, 2, {{0,1}, {1,1}});
				AssertThat(isArborescenceForest(G, roots), IsFalse());
			});

			it("works on a graph with parallel edges", [&](){
				customGraph(G, 2, {{0,1}, {0,1}});
				AssertThat(isArborescenceForest(G, roots), IsFalse());
			});

			it("works on a graph without a source", [&](){
				customGraph(G, 2, {{0,0}, {0,1}});
				AssertThat(isArborescenceForest(G, roots), IsFalse());
			});

			it("works on a cyclic graph", [&](){
				customGraph(G, 3, {{0,1}, {0,2}, {1,2}});
				AssertThat(isArborescenceForest(G, roots), IsFalse());
			});

			it("works on a cyclic graph with different edge order", [&](){
				customGraph(G, 3, {{0,2}, {0,1}, {1,2}});
				AssertThat(isArborescenceForest(G, roots), IsFalse());
			});

			it("works on an arborescence", [&](){
				customGraph(G, 4, {{0,1}, {0,2}, {1,3}});
				AssertThat(isArborescenceForest(G, roots), IsTrue());
				AssertThat(roots.size(), Equals(1));
				AssertThat(roots.front(), Equals(G.firstNode()));
			});

			it("works on a disconnected forest", [&](){
				customGraph(G, 3, {{0,1}});
				AssertThat(isArborescenceForest(G, roots), IsTrue());
				AssertThat(roots.size(), Equals(2));
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
		});

		describe("degreeDistribution", [] {
			it("works on an empty graph", [] {
				Graph G;
				Array<int> dist;
				degreeDistribution(G, dist);
				AssertThat(dist.empty(), IsTrue());
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
		});

		describe("nodeDistribution", [] {
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
		});
	});
});
