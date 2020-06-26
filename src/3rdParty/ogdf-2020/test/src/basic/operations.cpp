/** \file
 * \brief Tests for graph operations.
 *
 * \author Max Ilsen
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
#include "ogdf/basic/graph_generators/operations.h"
#include "ogdf/basic/graph_generators.h"
#include "ogdf/basic/simple_graph_alg.h"
#include <graphs.h>
#include <testing.h>


/**
 * Calls a binary graph operation on a random graph and a simple random graph.
 * The result is tested with respect to its number of nodes and edges.
 *
 * @param name Name of the binary operation.
 * @param func Calls the operation on two graphs and assigns the result to the
 * third one. May already perform additional assertions.
 * @param numNodes Calculates the expected number of nodes of the result.
 * @param numEdges Calculates the expected number of edges of the result.
 * @param reqs The properties the first graph needs in order for the operation
 * to be performed on it.
 */
static void testBinaryOperation(const string &name,
	std::function<void(const Graph&, const Graph&, Graph&)> func,
	std::function<int(int, int)> numNodes,
	std::function<int(int, int, int, int)> numEdges,
	std::set<GraphProperty> reqs = {})
{
	describe(name, [&] {
		forEachGraphItWorks(reqs, [&](const Graph &G1) {
			Graph G2;
			randomSimpleGraph(G2, 15, 20);
			int n1 = G1.numberOfNodes();
			int m1 = G1.numberOfEdges();
			int n2 = G2.numberOfNodes();
			int m2 = G2.numberOfEdges();

			// Do operation.
			Graph result;
			func(G1, G2, result);

			// Test result of operation.
			AssertThat(result.numberOfNodes(), Equals(numNodes(n1, n2)));
			AssertThat(result.numberOfEdges(), Equals(numEdges(n1, m1, n2, m2)));
		}, GraphSizes(5, 45, 20));
	});
}

//! Shortcut for testBinaryOperation with numNodes = n1 * n2 and NodeMap param.
static void testGraphProduct(const string &name,
	std::function<void(const Graph&, const Graph&, Graph&, NodeMap&)> func,
	std::function<int(int, int, int, int)> numEdges,
	std::set<GraphProperty> reqs = {})
{
	testBinaryOperation(name
		, [&func](const Graph& G1, const Graph& G2, Graph& product)
			{ NodeMap nodeInProduct; func(G1, G2, product, nodeInProduct); }
		, [](int n1, int n2) { return n1 * n2; }
		, numEdges, reqs);
}


go_bandit([] {
describe("Graph operations", [] {

	describe("graph union", [] {
		testBinaryOperation("disjoint"
		, [](const Graph &G1, const Graph &G2, Graph &result) {
				result = G1;
				graphUnion(result, G2);

				// Check number of components.
				NodeArray<int> compNum(G1);
				int comps = connectedComponents(G1, compNum);
				compNum.init(G2);
				comps += connectedComponents(G2, compNum);
				compNum.init(result);
				AssertThat(comps, Equals(connectedComponents(result, compNum)));
			}
		, [](int n1, int n2) { return n1 + n2; }
		, [](int n1, int m1, int n2, int m2) { return m1 + m2; }
		);

		// Test non-disjoint graph union also for parallel-free cases.
		std::vector<std::pair<bool, bool>> paramList =
			{{false, true}, {true, false}, {true, true}};
		for (auto params : paramList) {
			bool parallelfree = std::get<0>(params);
			bool directed = std::get<1>(params);
			string paramStr = (parallelfree ? ", " + ((directed ? "" : "un" ) +
				string("directed ")) + "parallel-free" : "");

			int mappedNodes = 0;
			int removedEdges = 0;

			testBinaryOperation("non-disjoint" +  paramStr
			, [&](const Graph &G1, const Graph &G2, Graph &result) {
					result = G1;
					NodeArray<node> map2to1(G2, nullptr);

					// Map nodes from G2 to result.
					mappedNodes = randomNumber(0, min(G1.numberOfNodes(), G2.numberOfNodes()));
					node vG2 = G2.firstNode();
					node vResult = result.firstNode();
					for (int i = 0; i < mappedNodes; ++i) {
						map2to1[vG2] = vResult;
						vG2 = vG2->succ();
						vResult = vResult->succ();
					}

					// Find multi-edges that will be removed.
					removedEdges = 0;
					if (parallelfree) {
						vG2 = G2.firstNode();
						for (int i = 0; i < mappedNodes; ++i) {
							node wG2 = G2.firstNode();
							for (int j = 0; j < mappedNodes; ++j) {
								if (G2.searchEdge(vG2, wG2, directed) &&
									result.searchEdge(map2to1[vG2], map2to1[wG2], directed)) {
									removedEdges++;
								}
								wG2 = wG2->succ();
							}
							vG2 = vG2->succ();
						}
						if (!directed) {
							removedEdges /= 2;
						}
					}
					graphUnion(result, G2, map2to1, parallelfree, directed);

					// Assert deletion of multi-edges according to params.
					if (parallelfree) {
						if (directed) {
							isParallelFree(result);
							removedEdges += numParallelEdges(G1);
							removedEdges += numParallelEdges(G2);
						} else {
							isParallelFreeUndirected(result);
							removedEdges += numParallelEdgesUndirected(G1);
							removedEdges += numParallelEdgesUndirected(G2);
						}
					}

					// Assert that map2to1 was filled correctly.
					for (node v2 : G2.nodes) {
						AssertThat(map2to1[v2], !IsNull());
					}
				}
			, [&mappedNodes](int n1, int n2) { return n1 + n2 - mappedNodes; }
			, [&removedEdges](int n1, int m1, int n2, int m2) { return m1 + m2 - removedEdges; }
			);
		}
	});

	describe("graph products", [] {
		testGraphProduct("cartesianProduct", cartesianProduct
		, [](int n1, int m1, int n2, int m2) { return m1 * n2 + m2 * n1; }
		);

		testGraphProduct("tensorProduct", tensorProduct
		, [](int n1, int m1, int n2, int m2) { return 2 * m1 * m2; }
		);

		testGraphProduct("lexicographicalProduct", lexicographicalProduct
		, [](int n1, int m1, int n2, int m2) { return m1 * n2 * n2 + m2 * n1; }
		);

		testGraphProduct("strongProduct", strongProduct
		, [](int n1, int m1, int n2, int m2) { return m1 * n2 + m2 * n1 + 2 * m1 * m2; }
		);

		testGraphProduct("coNormalProduct", coNormalProduct
		, [](int n1, int m1, int n2, int m2) { return m1 * n2 * n2 + m2 * n1 * n1; }
		);

		testGraphProduct("modularProduct", modularProduct
		, [](int n1, int m1, int n2, int m2) { return 2 * (m1 * m2 + (n1*(n1-1)/2 - m1) * (n2*(n2-1)/2 - m2)); }
		, {GraphProperty::simple} // calculation of edge number depends on graphs being simple
		);

		testGraphProduct("rootedProduct"
		, [](const Graph &G1, const Graph &G2, Graph &product, NodeMap &nodeInProduct)
			{ rootedProduct(G1, G2, product, nodeInProduct, G2.firstNode()); }
		, [](int n1, int m1, int n2, int m2) { return m1 + m2 * n1; }
		);
	});

});
});
