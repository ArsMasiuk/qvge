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
					// TODO: write isBipartite(), test it, use it here!
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
		});

		it("generates K_{4,1,1}", [] {
			Graph G;
			completeKPartiteGraph(G, {4, 1, 1});
			AssertThat(G.numberOfNodes(), Equals(6));
			AssertThat(G.numberOfEdges(), Equals(9));
			AssertThat(isConnected(G), IsTrue());
			AssertThat(isSimpleUndirected(G), IsTrue());
			assertNodeDegrees(G, {{2, 4}, {5, 2}});
		});

		it("generates K_{1,2,1,2}", [] {
			Graph G;
			completeKPartiteGraph(G, {1, 2, 1, 2});
			AssertThat(G.numberOfNodes(), Equals(6));
			AssertThat(G.numberOfEdges(), Equals(13));
			AssertThat(isConnected(G), IsTrue());
			AssertThat(isSimpleUndirected(G), IsTrue());
			assertNodeDegrees(G, {{4, 4}, {5, 2}});
		});
	});

	// TODO: Write tests for the following deterministic graph generators:
	//  - regularTree
	//  - wheelGraph
	//  - suspension
	//  - gridGraph
	//  - petersenGraph

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

		for(int n = 0; n < 100; n++) {
			int m = randomNumber(0, (n*(n-1))/2);
			it(string("generates a graph with " + to_string(n) + " nodes and " + to_string(m) + " edges"), [&](){
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

		for(int n = 0; n < 100; n++) {
			int m = randomNumber(0, (n*(n-1))/2);
			it(string("generates a graph with " + to_string(n) + " nodes and " + to_string(m) + " edges"), [&](){
				Graph G;
				randomSimpleGraph(G, n, m);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(m));
				AssertThat(isSimple(G), Equals(true));
			});
		}
	});

	describe("randomSimpleConnectedGraph", []() {
		itClearsGraph([](Graph &G) {
			randomSimpleConnectedGraph(G, 0, 0);
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
		for(int n = 3; n < 100; n++) {
			int m = randomNumber(n, (n*(n-1))/2);
			it(string("generates a graph with " + to_string(n) + " nodes and " + to_string(m) + " edges"), [&](){
				Graph G;
				randomBiconnectedGraph(G, n, m);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(m));
				AssertThat(isBiconnected(G), Equals(true));
			});
		}
	});

	describe("randomTriconnectedGraph", [](){
		for(int n = 4; n < 100; n++) {
			it(string("generates a graph with " + to_string(n) + " nodes"), [&](){
				Graph G;
				randomTriconnectedGraph(G, n, .5, .5);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(isTriconnected(G), Equals(true));
			});
		}
	});

	describe("randomTree", [](){
		itClearsGraph([](Graph &G) {
			randomTree(G, 0);
		});

		for(int n = 0; n < 100; n++) {
			it(string("generates a graph with " + to_string(n) + " nodes"), [&](){
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
			it(string("generates a graph with " + to_string(n) + " nodes and " + to_string(m) + " edges"), [&](){
				Graph G;
				randomHierarchy(G, n, m, false, false, true);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(G.numberOfEdges(), Equals(m));
			});
		}
	});

	describe("randomDiGraph", [](){
		for(int n = 1; n < 100; n++) {
			it(string("generates a graph with " + to_string(n) + " nodes"), [&](){
				Graph G;
				randomDiGraph(G, n, .5);
				AssertThat(G.numberOfNodes(), Equals(n));
				AssertThat(isSimple(G), Equals(true));
			});
		}
	});

	describe("randomRegularGraph", []() {
		for (int n = 10; n <= 30; n += 5) {
			for (int d = 2; d <= 6; d += 2) {
				it(string("generates a graph with degree " + to_string(d) + " and " + to_string(n) + " nodes"), [&]() {
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
		for(int d = 1; d < 4; d++){
			for(double t : {0.0, 0.1, 0.5}) {
				for(int n = 0; n < 100; n++) {
					it(string("generates a graph with " + to_string(n) +
							  " nodes in dim " + to_string(d) +
							  " and threshold " + to_string(t)), [&](){
						Graph G;
						randomGeometricCubeGraph(G,n,t,d);
						AssertThat(G.numberOfNodes(), Equals(n));
						AssertThat(isSimple(G), Equals(true));
					});
				}
			}
		}
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
