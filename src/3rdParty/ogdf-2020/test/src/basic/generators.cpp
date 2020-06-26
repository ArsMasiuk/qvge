/** \file
 * \brief Simple tests for generating various graphs.
 *
 * \author Christoph Schulz, Tilo Wiedera
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

#include "ogdf/basic/Graph.h"
#include "ogdf/basic/graph_generators.h"
#include "ogdf/basic/simple_graph_alg.h"
#include "ogdf/basic/extended_graph_alg.h"
#include <testing.h>

/**
 * Checks for a given graph \p G and a given list of
 * pairs {\a d, \a n} in \p degNumberPairs, that there
 * are \a n occurrences of degree \a d.
 */
static void assertNodeDegrees(const Graph &G, std::vector<std::pair<int,int>> degNumberPairs) {
	Array<int> degdist;
	degreeDistribution(G, degdist);

	for (auto degNumberPair : degNumberPairs) {
		const int d = degNumberPair.first;
		const int n = degNumberPair.second;

		AssertThat(d, !(IsGreaterThan(degdist.high()) || IsLessThan(degdist.low())));
		AssertThat(degdist[d], Equals(n));
	}
}

/**
 * Checks if the nodes in a given graph \p G are constructed
 * in a circulant way, with each node being connected to its
 * (\a idx &plusmn; \a i) neighbors, for each \a i in \p jumps.
 */
static void assertCirculant(Graph &G, Array<int>& jumps) {
	Array<node> nodes;
	NodeArray<int> indices = NodeArray<int>(G);
	G.allNodes(nodes);
	for (int i = 0; i < nodes.size(); i++) {
		indices[nodes[i]] = i;
	}

	// Make sure our nodes are generated in order with our edges being of the requested length
	for (node v : nodes) {
		std::vector<node> expected;
		for (int j : jumps) {
			expected.push_back(nodes[(indices[v] + j + nodes.size()) % nodes.size()]);
			expected.push_back(nodes[(indices[v] - j + nodes.size()) % nodes.size()]);
		}

		List<edge> vEdges;
		v->adjEdges(vEdges);
		for (edge e : vEdges) {
			AssertThat(expected, Contains(e->opposite(v)));
			auto it = std::find(expected.begin(), expected.end(), e->opposite(v));
			if (it != expected.end()) {
				expected.erase(it);
			}
		}
		AssertThat(expected, IsEmpty());
	}
}

/**
 * Checks whether two graphs \p G and \p H are equal based on their internal structure.
 *
 * This compares indices for nodes, edges and adjacency entries to make sure two graphs
 * have been constructed identically. This fails on any permutations like different
 * construction order!
 */
static void assertStructurallyEqual(const Graph &G, const Graph &H) {
	AssertThat(G.numberOfEdges(), Equals(H.numberOfEdges()));
	AssertThat(G.numberOfNodes(), Equals(H.numberOfNodes()));

	// Assert equality for all nodes
	for (auto it_G = G.nodes.begin(), it_H = H.nodes.begin(); it_G != G.nodes.end(); ++it_G, ++it_H) {
		node n_G = *it_G;
		node n_H = *it_H;
		AssertThat(n_G->index(), Equals(n_H->index()));
		AssertThat(n_G->adjEntries.size(), Equals(n_H->adjEntries.size()));

		// Assert equality for all adjacency entries of this node
		auto it_n_G = n_G->adjEntries.begin();
		auto it_n_H = n_H->adjEntries.begin();
		for (; it_n_G != n_G->adjEntries.end(); ++it_n_G, ++it_n_H) {
			adjEntry adj_G = *it_n_G;
			adjEntry adj_H = *it_n_H;
			AssertThat(adj_G->index(), Equals(adj_H->index()));
			AssertThat(adj_G->theEdge()->index(), Equals(adj_H->theEdge()->index()));
			AssertThat(adj_G->twinNode()->index(), Equals(adj_H->twinNode()->index()));
		}
	}
}

/**
 * Checks if \p generator constructs the same graph on multiple runs
 */
static void itKeepsStructuralEquality(std::function<void(Graph &G)> generator) {
	it("constructs the same graph in multiple runs with the same seed", [&] {
		int seed = randomNumber(0, std::numeric_limits<int>::max());
		Graph G;
		Graph H;
		setSeed(seed);
		generator(G);
		setSeed(seed);
		generator(H);
		assertStructurallyEqual(G, H);
	});
}


/**
 * Checks if \p clearFunction clears the graph
 */
static void itClearsGraph(std::function<void(Graph &G)> clearFunction) {
	it("clears the graph", [&] {
		Graph G;
		G.newEdge(G.newNode(), G.newNode());
		clearFunction(G);
		AssertThat(G.empty(), IsTrue());
	});
}

static void testDeterministicGenerators() {
	describe("circulantGraph", [] {
		itClearsGraph([](Graph &G) {
			circulantGraph(G, 0, Array<int>{});
		});

		it("generates two circulant graphs",[](){
			Graph G;
			circulantGraph(G, 11, Array<int>{1, 2, 4});
			AssertThat(G.numberOfEdges(), Equals(33));
			AssertThat(G.numberOfNodes(), Equals(11));
			AssertThat(isConnected(G),Equals(true));

			circulantGraph(G, 12, Array<int>{2, 4, 6});
			AssertThat(G.numberOfNodes(), Equals(12));
			AssertThat(isConnected(G),Equals(false));
		});

		for (int n = 10; n < 40; n+=3) {
			Array<int> jumps = Array<int>(3);
			for (int jumpmod = 1; jumpmod*4+4 < n; jumpmod++) {
				jumps[0] = jumpmod;
				jumps[1] = jumpmod*2;
				jumps[2] = jumpmod*2+2;
				string jumpstr = "{" + to_string(jumps[0]) + ", " + to_string(jumps[1]) + ", " + to_string(jumps[2]) + "}";
				it("generates a circulant graphs with " + to_string(n) + " nodes and jumps " + jumpstr, [&](){
					Graph G;
					circulantGraph(G, n, jumps);
					AssertThat(G.numberOfEdges(), Equals(n*3));
					AssertThat(G.numberOfNodes(), Equals(n));
					assertCirculant(G, jumps);
				});
			}
		}
	});

	describe("emptyGraph", [] {
		itClearsGraph([](Graph &G) {
			emptyGraph(G, 0);
		});

		for (int n = 0; n < 20; n++) {
			it("generates a graph with " + to_string(n) + " isolated nodes", [&] {
				Graph G;
				emptyGraph(G, n);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(0));
			});
		}
	});

	describe("completeGraph", [] {
		itClearsGraph([](Graph &G) {
			completeGraph(G, 0);
		});

		for (int n = 0; n < 20; n++) {
			it("generates K_" + to_string(n), [&] {
				Graph G;
				completeGraph(G, n);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(n * (n-1) / 2));
				AssertThat(isSimpleUndirected(G), IsTrue());
				AssertThat(isAcyclic(G), IsTrue());
			});
		}
	});

	describe("completeBipartiteGraph", [] {
		for (int n = 1; n <= 5; n++) {
			for (int m = 1; m <= 5; m++) {
				it("generates K_{" + to_string(n) + "," + to_string(m) + "}", [&] {
					Graph G;
					completeBipartiteGraph(G, n, m);
					AssertThat(G.numberOfNodes(), Equals(n + m));
					AssertThat(G.numberOfEdges(), Equals(n * m));
					AssertThat(isSimpleUndirected(G), IsTrue());
					AssertThat(isBipartite(G), IsTrue());
					AssertThat(isAcyclic(G), IsTrue());
				});
			}
		}
	});

	describe("completeKPartiteGraph", [] {
		itClearsGraph([](Graph &G) {
			completeKPartiteGraph(G, {});
		});

		it("generates K_{1,1,1}", [] {
			Graph G;
			completeKPartiteGraph(G, {1, 1, 1});
			AssertThat(G.numberOfNodes(), Equals(3));
			AssertThat(isSimpleUndirected(G), IsTrue());
			AssertThat(isAcyclicUndirected(G), IsFalse());
			AssertThat(isAcyclic(G), IsTrue());
		});

		it("generates K_{4,1,1}", [] {
			Graph G;
			completeKPartiteGraph(G, {4, 1, 1});
			AssertThat(G.numberOfNodes(), Equals(6));
			AssertThat(G.numberOfEdges(), Equals(9));
			AssertThat(isConnected(G), IsTrue());
			AssertThat(isSimpleUndirected(G), IsTrue());
			assertNodeDegrees(G, {{2, 4}, {5, 2}});
			AssertThat(isAcyclic(G), IsTrue());
		});

		it("generates K_{1,2,1,2}", [] {
			Graph G;
			completeKPartiteGraph(G, {1, 2, 1, 2});
			AssertThat(G.numberOfNodes(), Equals(6));
			AssertThat(G.numberOfEdges(), Equals(13));
			AssertThat(isConnected(G), IsTrue());
			AssertThat(isSimpleUndirected(G), IsTrue());
			assertNodeDegrees(G, {{4, 4}, {5, 2}});
			AssertThat(isAcyclic(G), IsTrue());
		});
	});

	describe("regularLatticeGraph", [] {
		for (int n = 4; n < 50; n++) {
			for (int k = 2; k < n-2; k+=2) {
				it("generates graph with " + to_string(n) + " nodes and " + to_string(k) + " degrees", [&] {
					Graph G;
					regularLatticeGraph(G, n, k);
					AssertThat(G.numberOfNodes(), Equals(n));
					AssertThat(G.numberOfEdges(), Equals(n*k/2));
					AssertThat(isConnected(G), IsTrue());
					AssertThat(isSimple(G), IsTrue());
					assertNodeDegrees(G, {{k, n}});
					Array<int> jumps = Array<int>(k/2);
					for (int i = 0; i < k/2; i++) {
						jumps[i] = i+1;
					}
					assertCirculant(G, jumps);
				});
			}
		}
	});

	describe("regularTree", [] {
		for (int n = 1; n < 50; n++) {
			for (int d = 1; d < n; d++) {
				it("generates the regular tree with " + to_string(n) + " nodes and " + to_string(d) + " children", [&]() {
					Graph G;
					regularTree(G, n, d);
					AssertThat(G.numberOfNodes(), Equals(n));
					AssertThat(isTree(G), IsTrue());
					// Test on k-arity:
					// Calculate number of expected inner nodes
					int nodesOnLevel = 1; // Number of nodes on the current level
					int sumNumberOfNodes = nodesOnLevel; // Total number of nodes up to current level
					int numberOfNodes = G.numberOfNodes();
					while (sumNumberOfNodes < numberOfNodes) {
						nodesOnLevel *= d;
						sumNumberOfNodes += nodesOnLevel;
					}
					sumNumberOfNodes -= nodesOnLevel;
					nodesOnLevel /= d;
					sumNumberOfNodes -= nodesOnLevel + 1;
					/* We now have:
					   - at least 1 node of degree d
					   - at least nodesOnLevel nodes of degree 1 (leaves)
					   - at least sumNumberOfNodes node of degree d+1.
					 */
					Array<int> degdist;
					degreeDistribution(G, degdist);
					AssertThat(degdist[d], IsGreaterThanOrEqualTo(1));
					// Our array is not initialized if no such nodes exist.
					if (sumNumberOfNodes > 0) AssertThat(degdist[d+1], IsGreaterThanOrEqualTo(sumNumberOfNodes));
					AssertThat(degdist[1], IsGreaterThanOrEqualTo(nodesOnLevel));
				});
			}
		}
	});

	describe("wheelGraph", [] {
		for (int n = 3; n < 50; n++) {
			it("generates the wheel graph with " + to_string(n) + " exterior nodes", [&]() {
				Graph G;
				wheelGraph(G, n);
				AssertThat(G.numberOfNodes(), Equals(n+1));
				AssertThat(G.numberOfEdges(), Equals(n*2));
				AssertThat(isSimpleUndirected(G), IsTrue());
				AssertThat(isConnected(G), IsTrue());
				if (n == 3) {
					// Special case: complete graph K_4
					AssertThat(isRegular(G, 3), IsTrue());
				}
				else {
					assertNodeDegrees(G, {{n, 1}, {3, n}});
				}
			});
		}
	});

	describe("suspension", [] {
		for (int n = 1; n < 50; n++) {
			for (int s = 1; s < 5; s++) {
				string label;
				if (s == 0) {
					label = "does not modify a graph with " + to_string(n) + " nodes if no nodes added";
				}
				else {
					label = "adds " + to_string(s) + " suspension nodes to a graph with " + to_string(n) + " nodes";
				}
				it(label, [&]() {
					Graph G;
					randomSimpleGraph(G, n, n/2);
					int numberOfNodes = G.numberOfNodes();
					int numberOfEdges = G.numberOfEdges();
					bool connected = isConnected(G);
					suspension(G, s);
					AssertThat(G.numberOfNodes(), Equals(numberOfNodes + s));
					AssertThat(G.numberOfEdges(), Equals(numberOfEdges + s*numberOfNodes));
					if (s == 0) AssertThat(isConnected(G), Equals(connected));
					else AssertThat(isConnected(G), IsTrue());
					AssertThat(isSimpleUndirected(G), IsTrue());
				});
			}
		}
	});

	describe("gridGraph", [] {
		for (int n = 2; n <= 10; n++) {
			for (int m = 2; m <= 10; m++) {
				for (bool loopN : {true, false}) {
					for (bool loopM : {true, false}) {
						it("generates a grid of " + to_string(n) + "x" + to_string(m) + " (loop:" + (loopN ? "yes" : " no") + "/" + (loopM ? "yes" : "no ") + ")", [&](){
							Graph G;
							gridGraph(G, n, m, loopN, loopM);
							int expectedEdges = 2*n*m;
							// Fewer edges if we do not close the torus in either direction.
							if (!loopN) expectedEdges -= m;
							if (!loopM) expectedEdges -= n;
							AssertThat(G.numberOfNodes(), Equals(n*m));
							AssertThat(G.numberOfEdges(), Equals(expectedEdges));
							AssertThat(isLoopFree(G), IsTrue());
							// If the grid is two nodes wide or tall, parallel edges are inserted for the loop.
							if ( (n > 2 || !loopN) && (m > 2 || !loopM)) {
								AssertThat(isParallelFreeUndirected(G), IsTrue());
							}
							AssertThat(isConnected(G), IsTrue());
							std::vector<std::pair<int, int>> expectedDegrees;
							// We expect degree 2 only for the corners if we do not close the torus in either direction.
							// We expect degree 4 for every node if we loop in all directions. For each direction that we
							// do not loop in, the sides of that direction are degree 3 instead.
							int e2 = 0;
							int e3 = 0;
							if ( loopN && !loopM) e3 = 2*n;
							if (!loopN &&  loopM) e3 = 2*m;
							if (!loopN && !loopM) {
								e2 = 4;
								e3 = 2*(m-2 + n-2); // Do not count corners
							}
							int e4 = n*m - (e2 + e3);
							if (e2 > 0) expectedDegrees.push_back({2, e2});
							if (e3 > 0) expectedDegrees.push_back({3, e3});
							if (e4 > 0) expectedDegrees.push_back({4, e4});
							assertNodeDegrees(G, expectedDegrees);
						});
					}
				}
			}
		}
	});

	describe("petersenGraph", [] {
		it("generates the standard Petersen graph if no additional arguments are supplied", [&]() {
			Graph G;
			petersenGraph(G);
			AssertThat(G.numberOfNodes(), Equals(10));
			AssertThat(G.numberOfEdges(), Equals(15));
			AssertThat(isSimpleUndirected(G), IsTrue());
			AssertThat(isRegular(G, 3), IsTrue());
		});
		for (int n = 3; n <= 10; n++) {
			for (int d = 1; d < n/2; d++) {
				it("generates the generalized Petersen graph with " + to_string(n) + " outer nodes and an inner jump width of " + to_string(d), [&](){
					Graph G;
					petersenGraph(G, n, d);
					AssertThat(G.numberOfNodes(), Equals(2 * n));
					AssertThat(G.numberOfEdges(), Equals(3 * n));
					AssertThat(isSimpleUndirected(G), IsTrue());
					AssertThat(isRegular(G, 3), IsTrue());
				});
			}
		}
	});

	describe("customGraph", [] {
		itClearsGraph([](Graph &G) {
			customGraph(G, 0, {});
		});

		for (int n = 0; n < 50; n++) {
			int m = randomNumber(0, (n*(n-1))/2);
			List<std::pair<int,int>> edges;

			for (int i = 0; i < m; i++) {
				std::pair<int,int> e({randomNumber(0, n-1),
				                      randomNumber(0, n-1)});
				edges.pushBack(e);
			}

			it("generates a custom graph with " + to_string(n) + " nodes and " + to_string(m) + " edges", [&]() {
				Graph G;
				customGraph(G, n, edges);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(m));

				Array<node> nodes(n);
				int i = 0;
				for (auto v : G.nodes) {
					nodes[i++] = v;
				}

				for (auto e : G.edges) {
					std::pair<int,int> nodePair = edges.popFrontRet();
					AssertThat(nodes[std::get<0>(nodePair)], Equals(e->source()));
					AssertThat(nodes[std::get<1>(nodePair)], Equals(e->target()));
				}
			});
		}

		it("returns a correct mapping", [] {
			Graph G;
			Array<node> nodes;
			customGraph(G, 5, {{0, 2}, {1, 2}, {2, 2}, {3, 2}, {4, 2}}, nodes);
			AssertThat(G.numberOfNodes(), Equals(5));
			AssertThat(G.numberOfEdges(), Equals(5));
			G.delNode(nodes[2]);
			AssertThat(G.numberOfNodes(), Equals(4));
			AssertThat(G.numberOfEdges(), Equals(0));
		});
	});
}

static void testRandomGenerators() {
	describe("randomGraph", [](){
		itClearsGraph([](Graph &G) {
			randomGraph(G, 0, 0);
		});
		itKeepsStructuralEquality([](Graph &G) {
			randomGraph(G, 20, 100);
		});

		for(int n = 0; n < 100; n++) {
			int m = randomNumber(0, (n*(n-1))/2);
			it("generates a graph with " + to_string(n) + " nodes and " + to_string(m) + " edges", [&] {
				Graph G;
				randomGraph(G, n, m);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(m));
			});
		}
	});

	describe("randomSimpleGraph", [](){
		itClearsGraph([](Graph &G) {
			randomSimpleGraph(G, 0, 0);
		});
		itKeepsStructuralEquality([](Graph &G) {
			randomSimpleGraph(G, 20, 100);
		});

		for(int n = 0; n < 100; n++) {
			int m = randomNumber(0, (n*(n-1))/2);
			it("generates a graph with " + to_string(n) + " nodes and " + to_string(m) + " edges", [&] {
				Graph G;
				randomSimpleGraph(G, n, m);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(m));
				AssertThat(isSimple(G), Equals(true));
			});
		}
	});

	describe("randomSimpleGraphByProbability", [](){
		itClearsGraph([](Graph &G) {
			randomSimpleGraphByProbability(G, 0, 0);
		});
		itKeepsStructuralEquality([](Graph &G) {
			randomSimpleGraphByProbability(G, 20, 0.5);
		});

		for(int n = 0; n < 100; n++) {
			double p = randomDouble(0, 1);
			it("generates a graph with " + to_string(n) + " nodes and " + to_string(p) + " edge probability", [&] {
				Graph G;
				AssertThat(randomSimpleGraphByProbability(G, n, p), Equals(true));
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(isSimple(G), Equals(true));
			});
		}
	});

	describe("randomSimpleConnectedGraph", []() {
		itClearsGraph([](Graph &G) {
			randomSimpleConnectedGraph(G, 0, 0);
		});
		itKeepsStructuralEquality([](Graph &G) {
			randomSimpleConnectedGraph(G, 20, 100);
		});

		it("fails if it cannot be simple", []() {
			Graph G;
			AssertThat(randomSimpleConnectedGraph(G, 1, 1), IsFalse());
			AssertThat(randomSimpleConnectedGraph(G, 2, 2), IsFalse());
			AssertThat(randomSimpleConnectedGraph(G, 3, 4), IsFalse());
		});

		it("fails if it cannot be connected", []() {
			Graph G;
			AssertThat(randomSimpleConnectedGraph(G, 2, 0), IsFalse());
			AssertThat(randomSimpleConnectedGraph(G, 3, 1), IsFalse());
		});

		for (int n = 0; n < 100; n++) {
			int m = randomNumber(max(0, n-1), (n*(n-1))/2);
			it("generates a graph with " + to_string(n) + " nodes and " + to_string(m) + " edges", [&]() {
				Graph G;
				bool ret = randomSimpleConnectedGraph(G, n, m);
				AssertThat(ret, IsTrue());
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(m));
				AssertThat(isSimple(G), IsTrue());
				AssertThat(isConnected(G), IsTrue());
			});
		}
	});

	describe("randomBiconnectedGraph", [](){
		itKeepsStructuralEquality([](Graph &G) {
			randomBiconnectedGraph(G, 20, 100);
		});

		for(int n = 3; n < 100; n++) {
			int m = randomNumber(n, (n*(n-1))/2);
			it("generates a graph with " + to_string(n) + " nodes and " + to_string(m) + " edges", [&] {
				Graph G;
				randomBiconnectedGraph(G, n, m);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(m));
				AssertThat(isBiconnected(G), Equals(true));
			});
		}
	});

	describe("randomTriconnectedGraph", [](){
		itKeepsStructuralEquality([](Graph &G) {
			randomTriconnectedGraph(G, 20, .5, .5);
		});

		for(int n = 4; n < 100; n++) {
			it("generates a graph with " + to_string(n) + " nodes", [&] {
				Graph G;
				randomTriconnectedGraph(G, n, .5, .5);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(isTriconnected(G), Equals(true));
			});
		}
	});

	describe("randomPlanarBiconnectedGraph", [](){
		itKeepsStructuralEquality([](Graph &G) {
			randomPlanarBiconnectedGraph(G, 20, 100, true);
		});

		for(int n = 3; n < 100; n++) {
			int m = randomNumber(n, 3*n-6);
			it("generates a graph with " + to_string(n) + " nodes and " + to_string(m) + " edges", [&] {
				Graph G;
				randomPlanarBiconnectedGraph(G, n, m, false);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(m));
				AssertThat(isSimple(G), IsTrue());
				AssertThat(isPlanar(G), IsTrue());
				AssertThat(isBiconnected(G), IsTrue());
			});
		}
	});

	describe("randomPlanarCNBGraph", [](){
		itKeepsStructuralEquality([](Graph &G) {
			randomPlanarCNBGraph(G, 20, 50, 3);
		});

		for(int b = 2; b < 15; b++) {
			for(int n = 3; n < 30; n++) {
				int m = randomNumber(n, 3*n-6);
				it("generates a graph with " + to_string(b) +
						" biconnected components and max. " + to_string(n) +
						" nodes per component", [&] {
					Graph G;
					EdgeArray<int> comps(G);
					randomPlanarCNBGraph(G, n, m, b);
					AssertThat(G.numberOfNodes(), IsLessThanOrEqualTo(n*b));
					AssertThat(G.numberOfEdges(), IsLessThanOrEqualTo(m*b));
					AssertThat(isConnected(G), IsTrue());
					AssertThat(isSimple(G), IsTrue());
					AssertThat(isPlanar(G), IsTrue());
					AssertThat(biconnectedComponents(G, comps), Equals(b));
				});
			}
		}
	});


	describe("randomTree", [](){
		itClearsGraph([](Graph &G) {
			randomTree(G, 0);
		});
		itKeepsStructuralEquality([](Graph &G) {
			randomTree(G, 20);
		});

		for(int n = 0; n < 100; n++) {
			it("generates a graph with " + to_string(n) + " nodes", [&] {
				Graph G;
				randomTree(G, n);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(isTree(G), Equals(true));
			});
		}
	});

	// TODO: dont skip me
	describe_skip("randomHierarchy", [](){
		for(int n = 1; n < 100; n++) {
			int m = randomNumber(n-1, (n*(n-1))/2);
			it("generates a graph with " + to_string(n) + " nodes and " + to_string(m) + " edges", [&] {
				Graph G;
				randomHierarchy(G, n, m, false, false, true);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(m));
			});
		}
	});

	describe("randomDigraph", [](){
		itKeepsStructuralEquality([](Graph &G) {
			randomDigraph(G, 20, 0.4);
		});

		for(int n = 1; n < 100; n++) {
			it("generates a graph with " + to_string(n) + " nodes", [&] {
				Graph G;
				randomDigraph(G, n, .5);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(isSimple(G), Equals(true));
			});
		}
	});

	describe("randomRegularGraph", []() {
		itKeepsStructuralEquality([](Graph &G) {
			randomRegularGraph(G, 20, 4);
		});

		for (int n = 10; n <= 30; n += 5) {
			for (int d = 2; d <= 6; d += 2) {
				it("generates a graph with degree " + to_string(d) + " and " + to_string(n) + " nodes", [&] {
					Graph G;
					randomRegularGraph(G, n, d);
					AssertThat(G.numberOfNodes(), Equals(n));
					AssertThat(isSimple(G), Equals(true));
					AssertThat(isRegular(G, d), Equals(true));
				});
			}
		}
	});

	describe("randomGeometricCubeGraph", [](){
		itKeepsStructuralEquality([](Graph &G) {
			randomGeometricCubeGraph(G, 20, 0.4, 3);
		});

		for(int d = 1; d < 4; d++){
			for(double t : {0.0, 0.1, 0.5}) {
				for(int n = 0; n < 100; n++) {
					it("generates a graph with " + to_string(n) +
							" nodes in dim " + to_string(d) +
							" and threshold " + to_string(t), [&] {
						Graph G;
						randomGeometricCubeGraph(G,n,t,d);
						AssertThat(G.numberOfNodes(), Equals(n));
						AssertThat(isSimple(G), Equals(true));
					});
				}
			}
		}
	});

	describe("randomGeographicalThresholdGraph", [](){
		itKeepsStructuralEquality([](Graph &G) {
			Array<int> weights = Array<int>(20);
			for (int &w : weights) {
				w = randomNumber(0, 20);
			}
			std::exponential_distribution<double> dist(0.5);
			randomGeographicalThresholdGraph(G, weights, dist, 0.4, 2, 2);
		});

		for (int d = 1; d < 4; d++) {
			for (double l : {0.5, 1.0, 2.0}) {
				for (int a = 1; a < 4; a++) {
					for (double t : {0.0, 0.1, 0.5}) {
						for (int n = 0; n < 50; n+=10) {
							it("generates a graph with " + to_string(n) +
									" nodes in dim " + to_string(d) +
									" with alpha " + to_string(a) +
									" and threshold " + to_string(t), [&] {
								Graph G;
								Array<int> weights = Array<int>(n);
								for (int &w : weights) {
									w = randomNumber(0, n);
								}
								std::exponential_distribution<double> dist(l);
								randomGeographicalThresholdGraph(G, weights, dist, t, a, d);
								AssertThat(G.numberOfNodes(), Equals(n));
								AssertThat(isSimple(G), Equals(true));
							});
						}
					}
				}
			}
		}
		for (int n = 0; n < 100; n+=10) {
			it("generates a graph with " + to_string(n) + " nodes with custom function", [&] {
				Graph G;
				Array<int> weights = Array<int>(n);
				for (int &w : weights) {
					w = randomNumber(0, n);
				}
				std::uniform_int_distribution<> dist(0, n);
				randomGeographicalThresholdGraph(G, weights, dist, 0.7, [](double r) { return 1/r; });
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(isSimple(G), Equals(true));
			});
		}
	});

	describe("randomEdgesGraph", [](){
		itKeepsStructuralEquality([](Graph &G) {
			emptyGraph(G, 20);
			randomEdgesGraph(G, [&](node,node) { return 0.4; });
		});

		std::minstd_rand rng(randomSeed());
		std::uniform_real_distribution<> dist(0, 1);
		for (int n = 2; n < 50; n++) {
			it("randomly generates edges in an empty graph with " + to_string(n) + " nodes", [&] {
				Graph G;
				emptyGraph(G, n);
				randomEdgesGraph(G, [&](node, node){ return dist(rng); });
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(isSimpleUndirected(G), IsTrue());
			});
		}
		for (int n = 2; n < 50; n++) {
			it("does not generate edges if probability is 0.0 on a graph with " + to_string(n) + " nodes", [&] {
				Graph G;
				emptyGraph(G, n);
				randomEdgesGraph(G, [](node,node) { return 0.0; });
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(0));
			});
		}
		for (int n = 2; n < 50; n++) {
			int e = n * (n-1) / 2; // probability 1.0 should lead to a complete graph
			it("generates " + to_string(e) + " edges if probability is 1.0 on a graph with " + to_string(n) + " nodes", [&] {
				Graph G;
				emptyGraph(G, n);
				randomEdgesGraph(G, [](node,node) { return 1.0; });
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(e));
			});
		}
		for (int n = 2; n < 50; n++) {
			it("generates edges on a simple graph with " + to_string(n) + " nodes and keeps it free of self-loops", [&] {
				Graph G;
				randomSimpleGraph(G, n, n/2);
				randomEdgesGraph(G, [&](node,node) { return dist(rng); });
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), IsGreaterThanOrEqualTo(n/2));
				AssertThat(isLoopFree(G), IsTrue());
			});
		}
	});

	describe("randomWaxmanGraph", []() {
		itKeepsStructuralEquality([](Graph &G) {
			randomWaxmanGraph(G, 20, 0.4, 0.6);
		});

		for(int n = 1; n < 100; n+=10) {
			it("generates a graph with " + to_string(n) + " nodes", [&] {
				Graph G;
				randomWaxmanGraph(G, n, 0.5, 0.5);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(isSimpleUndirected(G), IsTrue());
			});
		}
		for(int n = 1; n < 100; n+=10) {
			it("generates a graph with " + to_string(n) + " nodes", [&] {
				Graph G;
				randomWaxmanGraph(G, n, 0.5, 0.5, 10, 10);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(isSimpleUndirected(G), IsTrue());
			});
		}
	});

	describe("preferentialAttachmentGraph", [](){
		itKeepsStructuralEquality([](Graph &G) {
			preferentialAttachmentGraph(G, 20, 3);
		});

		for (int n = 0; n < 100; n+=10) {
			for (int d = 1; d < 5; d++) {
				it("generates a graph with " + to_string(n) + " nodes with degree " + to_string(d) + " on an empty input graph", [&] {
					Graph G;
					preferentialAttachmentGraph(G, n, d);
					AssertThat(G.numberOfNodes(), Equals(n));
					AssertThat(isSimple(G), Equals(true));
				});
			}
		}
		for (int n = 3; n < 20; n++) {
			it("fills a tree with " + to_string(n) + " nodes with 50 nodes and stays connected", [&] {
				Graph G;
				randomSimpleConnectedGraph(G, n, n-1);
				preferentialAttachmentGraph(G, 50, 3);
				AssertThat(isConnected(G), Equals(true));
				AssertThat(isSimple(G), Equals(true));
			});
		}
		for (int n = 5; n < 20; n++) {
			it("fills a connected graph with " + to_string(n) + " nodes and " + to_string(n*2) + " edges with 50 nodes and stays connected", [&] {
				Graph G;
				randomSimpleConnectedGraph(G, n, n*2);
				preferentialAttachmentGraph(G, 50, 3);
				AssertThat(isConnected(G), Equals(true));
				AssertThat(isSimple(G), Equals(true));
			});
		}
	});

	describe("randomWattsStrogatzGraph", [] {
		itKeepsStructuralEquality([](Graph &G) {
			randomWattsStrogatzGraph(G, 20, 4, 0.4);
		});

		it("does not modify generated lattice graph at 0.0 probability", [] {
			Graph G;
			randomWattsStrogatzGraph(G, 20, 4, 0.0);
			AssertThat(G.numberOfEdges(), Equals(40));
			AssertThat(G.numberOfNodes(), Equals(20));
			AssertThat(isConnected(G), IsTrue());
			AssertThat(isSimple(G), IsTrue());
			assertNodeDegrees(G, {{4, 20}});

		});
		for (int n = 4; n <= 50; n+=7) {
			for (int k = 2; k < n-2; k+=2) {
				it("generates graph with " + to_string(n) + " nodes and " + to_string(k) + " degrees at 0.5 probability", [&] {
					Graph G;
					randomWattsStrogatzGraph(G, n, k, 0.5);
					AssertThat(G.numberOfNodes(), Equals(n));
					AssertThat(G.numberOfEdges(), Equals(n*k/2));
					AssertThat(isSimple(G), IsTrue());
					for (node v : G.nodes) {
						AssertThat(v->degree(), IsGreaterThanOrEqualTo(k/2));
					}
				});
			}
		}
	});

	describe("randomChungLuGraph", [](){
		itKeepsStructuralEquality([](Graph &G) {
			randomChungLuGraph(G, {1, 2, 2, 3, 3, 3, 4});
		});

		it("generates graph", []() {
			Graph G;
			randomChungLuGraph(G, {1, 2, 2, 3, 3, 3});
			AssertThat(G.numberOfNodes(), Equals(6));
			AssertThat(isSimpleUndirected(G), Equals(true));
		});
	});
}

// TODO: Test overloaded functions

go_bandit([] {
	describe("Graph generators", [] {
		describe("Deterministic graph generators", [] {
			testDeterministicGenerators();
		});
		describe("Random generators", [] {
			testRandomGenerators();
		});
	});
});
