/** \file
 * \brief Basic bandit test suite used for Steiner Tree problem reductions.
 *
 * \author Mihai Popa, Stephan Beyer
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

#include <string>
#include <sstream>

#include <ogdf/graphalg/SteinerTreePreprocessing.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/graphalg/MinSteinerTreeDirectedCut.h>

#include <testing.h>

using namespace std;

template<typename T>
static void
putRandomTerminals(const EdgeWeightedGraph<T> &wg, List<node> &terminals, NodeArray<bool> &isTerminal, int numberOfTerminals)
{
	isTerminal.init(wg, false);

	Array<node> nodes(wg.numberOfNodes());
	wg.allNodes(nodes);
	nodes.permute();

	Math::updateMin(numberOfTerminals, wg.numberOfNodes() - 1);
	for (int i = 0; i < numberOfTerminals; ++i) {
		const node v = nodes[i];
		terminals.pushBack(v);
		isTerminal[v] = true;
	}
}

template<typename T>
static void
putRandomCosts(EdgeWeightedGraph<T> &wg, T x, T y, typename std::enable_if<std::is_integral<T>::value >::type* = 0)
{
	for (edge e : wg.edges) {
		wg.setWeight(e, randomNumber(x, y));
	}
}

template<typename T>
static void
putRandomCosts(EdgeWeightedGraph<T> &wg, T x, T y, typename std::enable_if<std::is_floating_point<T>::value >::type* = 0)
{
	for (edge e : wg.edges) {
		wg.setWeight(e, randomDouble(x, y));
	}
}

template<typename T>
static void
randomEdgeWeightedGraph(int numberOfnodes, int numberOfedges, int numberOfTerminals, T maxEdgeCost, EdgeWeightedGraph<T> &wg, List<node> &terminals, NodeArray<bool> &isTerminal)
{
	randomGraph(wg, numberOfnodes, numberOfedges);
	makeConnected(wg);
	putRandomTerminals<T>(wg, terminals, isTerminal, numberOfTerminals);
	putRandomCosts<T>(wg, 1, maxEdgeCost);
}

template<typename T>
static T
getCostOfSolution(const EdgeWeightedGraphCopy<T> &wg)
{
	T cost = 0;
	for (edge e : wg.edges) {
		cost += wg.weight(e);
	}

	return cost;
}

template<typename T, typename Fun>
static void
testReduction(Fun reductionFun)
{
	int numberOfNodes = randomNumber(50, 120);
	int numberOfEdges = randomNumber(numberOfNodes-1, 3*numberOfNodes);
	int numberOfTerminals = randomNumber(1, numberOfNodes);
	T maxEdgeCost = randomNumber(3, 1000000);
	MinSteinerTreeDirectedCut<T> mst;

	EdgeWeightedGraph<T> wg;
	List<node> terminals;
	NodeArray<bool> isTerminal;
	randomEdgeWeightedGraph<T>(numberOfNodes, numberOfEdges, numberOfTerminals, maxEdgeCost, wg, terminals, isTerminal);

	EdgeWeightedGraphCopy<T> *treeBefore = nullptr;
	T costBefore = mst.call(wg, terminals, isTerminal, treeBefore);

	SteinerTreePreprocessing<T> stprep(wg, terminals, isTerminal);
	reductionFun(stprep);

	EdgeWeightedGraphCopy<T> *treeAfter = nullptr;
	T costAfter = stprep.solve(mst, treeAfter);

	EpsilonTest et(1e-6);
	AssertThat(et.equal(costAfter, costBefore), IsTrue());

	AssertThat(MinSteinerTreeModule<T>::isSteinerTree(wg, terminals, isTerminal, *treeAfter), IsTrue());
	AssertThat(et.equal(getCostOfSolution<T>(*treeAfter), costBefore), IsTrue());

	delete treeBefore;
	delete treeAfter;
}

template<typename T>
static void
testBasicReductions(int numberOfTests)
{
	for (int i = 1; i <= numberOfTests; ++i) {
		it("does not change solution cost and finds a solution in the original graph", [&]() {
			testReduction<T>([](SteinerTreePreprocessing<T> &stp) {
				stp.deleteLeaves();
				stp.degree2Test();
				stp.makeSimple();
				stp.leastCostTest();
			});
		});
	}
}

template<typename T>
static void
testPrecomposedReductions(int numberOfTests)
{
	std::pair<const string, std::function<void(SteinerTreePreprocessing<T> &)>> reductions[] = {
		{ "trivial reductions", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.reduceTrivial();
		}},
		{ "fast reductions", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.reduceFast();
		}},
		{ "fast reductions with dual-ascent-based test", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.reduceFastAndDualAscent();
		}},
	};
	for (const auto &reduction : reductions) {
		describe(reduction.first, [numberOfTests, &reduction]() {
			for (int i = 1; i <= numberOfTests; ++i) {
				it("does not change solution cost and finds a solution in the original graph", [&]() {
					testReduction<T>(reduction.second);
				});
			}
		});
	}
}

template<typename T>
static void
testWildMixesOfReductions(string typeName, int numberOfTests)
{
	std::pair<string, std::function<void(SteinerTreePreprocessing<T> &)>> reductions[] = {
		{ "nearest-vertex", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.makeSimple();
			stprep.deleteComponentsWithoutTerminals();
			stprep.nearestVertexTest();
		}},
		{ "shortest-link", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.deleteComponentsWithoutTerminals();
			stprep.shortLinksTest();
		}},
		{ "PTm", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.deleteComponentsWithoutTerminals();
			stprep.PTmTest();
		}},
		{ "terminal-distance", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.deleteComponentsWithoutTerminals();
			stprep.terminalDistanceTest();
		}},
		{ "long-edge", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.longEdgesTest();
		}},
		{ "NTDk", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.makeSimple();
			stprep.deleteComponentsWithoutTerminals();
			stprep.NTDkTest();
		}},
		{ "lower-bound", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.deleteComponentsWithoutTerminals();
			stprep.lowerBoundBasedTest();
		}},
		{ "dual-ascent", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.deleteComponentsWithoutTerminals();
			stprep.dualAscentBasedTest();
		}},
		{ "reachability", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.makeSimple();
			stprep.deleteComponentsWithoutTerminals();
			stprep.reachabilityTest();
		}},
		{ "cut-reachability", [](SteinerTreePreprocessing<T> &stprep) {
			stprep.deleteLeaves();
			stprep.deleteComponentsWithoutTerminals();
			stprep.cutReachabilityTest();
		}},
	};
	auto size = sizeof reductions / sizeof reductions[0];
	for (auto reductionBitmask = 1; reductionBitmask < (1 << size); ++reductionBitmask) {
		string desc = "appliance of reductions";
		ArrayBuffer<int> usedReductions;
		for (auto cur = reductionBitmask, i = 0; cur; ++i, cur >>= 1) {
			if (cur & 1) {
				usedReductions.push(i);
			}
		}

		for (auto i : usedReductions) {
			desc += " " + reductions[i].first;
		}
		desc += " (" + typeName + ")";

		describe(desc, [numberOfTests, &usedReductions, &reductions]() {
			Array<int> order(usedReductions.size());
			for (int j = 0; j < order.size(); ++j) {
				order[j] = j;
			}
			for (int i = 1; i <= numberOfTests; ++i) {
				std::stringstream ss;
				ss << "does not change solution cost and finds a solution in the original graph (order "
				   << order << ")";
				it(ss.str(), [&]() {
					testReduction<T>([&usedReductions, &reductions](SteinerTreePreprocessing<T> &stp) {
						for (auto redIndex : usedReductions) {
							reductions[redIndex].second(stp);
						}
						stp.deleteComponentsWithoutTerminals();
					});
				});
				order.permute();
			}
		});
	}
}

template<typename T>
static void
registerSuite(const string &typeName)
{
	describe("basic reductions (" + typeName + ")", []() {
		testBasicReductions<T>(15);
	});
	describe_skip("mix of all subsets of reductions (" + typeName + ")", [&typeName]() {
		testWildMixesOfReductions<T>(typeName, 3);
	});
	describe("precomposed reductions (" + typeName + ")", []() {
		testPrecomposedReductions<T>(15);
	});
}

go_bandit([]() {
	describe("SteinerTreePreprocessing", []() {
		registerSuite<int>("int");
		registerSuite<double>("double");
	});
});
