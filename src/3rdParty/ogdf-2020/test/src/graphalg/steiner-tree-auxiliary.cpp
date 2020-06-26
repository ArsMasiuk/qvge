/** \file
 * \brief Tests for Steiner Tree approximation algorithm helpers
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

#include <set>
#include <ogdf/basic/EpsilonTest.h>
#include <ogdf/graphalg/MinSteinerTreeModule.h>
#include <ogdf/graphalg/steiner_tree/FullComponentStore.h>
#include <ogdf/graphalg/steiner_tree/Full2ComponentGenerator.h>
#include <ogdf/graphalg/steiner_tree/Full3ComponentGeneratorVoronoi.h>
#include <ogdf/graphalg/steiner_tree/Full3ComponentGeneratorEnumeration.h>
#include <ogdf/graphalg/steiner_tree/FullComponentGeneratorDreyfusWagner.h>
#include <testing.h>

using namespace std::placeholders;

static EpsilonTest epst(1e-6);

template<typename T>
struct EdgeData {
	int source;
	int target;
	T cost;
};

//! Predefined instances that can be used by using their index
template<typename T>
static std::pair<std::vector<int>, std::vector<EdgeData<T>>> predefinedInstanceData(int i) {
	switch (i) {
	case 1:
		// an instance with two terminal nodes
		return {{0, 3},
		  {{0, 2, 3},
		   {1, 0, 1},
		   {3, 2, 4},
		   {2, 1, 1},
		   {1, 3, 6}}};
	case 2:
		// a simple instance with all nodes being terminals
		return {{0, 1, 2}, {{0, 1, 2}, {0, 2, 2}}};
	case 3:
		// an instance to check heuristics preferring terminals
		return {{0, 1, 2},
		  {{0, 3, 1},
		   {0, 1, 2},
		   {1, 3, 1},
		   {2, 5, 1},
		   {5, 4, 1},
		   {4, 3, 1},
		   {2, 1, 2}}};
	case 4:
		// a more complicated instance
		return {{0, 1, 2, 3, 4},
		  {{0, 5, 1}, {1, 5, 1}, {3, 5, 1},
		   {5, 6, 1}, {2, 6, 1},
		   {2, 7, 4}, {4, 7, 3},
		   {0, 1, 2}, {2, 1, 3}}};
	}
	return {{}, {}};
};

//! An auxiliary class for nicer tests
template<typename T>
struct Instance {
	EdgeWeightedGraph<T> graph;
	List<node> terminals;
	NodeArray<bool> isTerminal;
	Array<node> v;

	//! Constructs a custom instance
	Instance(std::vector<int> terminalList, std::vector<EdgeData<T>> edges) {
		int n = 0;
		for (auto e : edges) {
			Math::updateMax(n, e.source + 1);
			Math::updateMax(n, e.target + 1);
		}

		v.init(n);
		for (int i = 0; i < n; ++i) {
			v[i] = graph.newNode();
		}

		for (auto e : edges) {
			graph.newEdge(v[e.source], v[e.target], e.cost);
		}

		setTerminals(terminalList);
	}

	//! Constructs a predefined instance with index \p i
	explicit Instance(int i)
	  : Instance(predefinedInstanceData<T>(i).first, predefinedInstanceData<T>(i).second)
	{}

	void setTerminals(std::vector<int> terminalList) {
		isTerminal.init(graph, false);
		for (auto t : terminalList) {
			OGDF_ASSERT(t <= v.high());
			isTerminal[v[t]] = true;
		}
		terminals.clear();
		MinSteinerTreeModule<T>::getTerminals(terminals, graph, isTerminal);
	}
};

template<typename T>
struct Arguments {
	NodeArray<NodeArray<T>> distance;
	NodeArray<NodeArray<edge>> pred;
};

template<typename T>
struct ModifiedShortestPathAlgorithmsTest {
	//! Assert something when considering a shortest path tree from a start node
	struct AssertFrom {
		int start;
		std::function<void(const Instance<T> &, const NodeArray<T> &, const NodeArray<edge> &)> doAssert;
	};

	//! Test a predefined instance
	static void testIt(std::string &&title, int instance,
			std::function<void(const Instance<T> &, Arguments<T> &)> doAPSP,
			std::initializer_list<AssertFrom> list) {
		it(title, [&] {
			Instance<T> S(instance);
			Arguments<T> arg;

			doAPSP(S, arg);

			for (auto af : list) {
				af.doAssert(S, arg.distance[S.v[af.start]], arg.pred[S.v[af.start]]);
			}
		});
	}

	//! Assert that \p nodes have no predecessor
	static void assertHasNoPred(const Instance<T> &S, const NodeArray<edge> &pred, std::initializer_list<int> nodes) {
		for (int i : nodes) {
			AssertThat(pred[S.v[i]], IsNull());
		}
	}

	//! For each pair (\a u, \a d) in \p nodeDistancePairs, assert that
	//! the distance to \a u equals \a d.
	static void assertDistanceTo(const Instance<T> &S, const NodeArray<T> &distance, std::initializer_list<std::pair<int, T>> nodeDistancePairs) {
		for (auto p : nodeDistancePairs) {
			AssertThat(epst.equal(distance[S.v[p.first]], T(p.second)), IsTrue());
		}
	}

	//! For each pair (\a u, \a v) in \p nodePredPairs, assert that the predecessor of \a u is \a v
	//! if \a v is nonnegative. Otherwise just make sure \a u has a predecessor.
	static void assertPred(const Instance<T> &S, const NodeArray<edge> &pred, std::initializer_list<std::pair<int, int>> nodePredPairs) {
		for (auto p : nodePredPairs) {
			AssertThat(pred[S.v[p.first]], !IsNull());
			if (p.second >= 0) {
				AssertThat(pred[S.v[p.first]]->opposite(S.v[p.first]), Equals(S.v[p.second]));
			}
		}
	}

	static void itMimicsOrdinaryShortestPath(const std::string spName,
			std::function<void(const Instance<T> &S, Arguments<T> &arg)> spAlg) {
		testIt("mimics ordinary " + spName + " when terminals are not in between",
		  1, spAlg, {{0, [&](const Instance<T> &S, const NodeArray<T> &distance, const NodeArray<edge> &pred) {
			assertHasNoPred(S, pred, {0});
			assertDistanceTo(S, distance, {{0, 0}, {1, 1}, {2, 2}, {3, 6}});
			assertPred(S, pred, {{1, 0}, {2, 1}, {3, 2}});
		}}, {3, [&](const Instance<T> &S, const NodeArray<T> &distance, const NodeArray<edge> &pred) {
			assertHasNoPred(S, pred, {3});
			assertDistanceTo(S, distance, {{3, 0}, {1, 5}, {2, 4}, {0, 6}});
			assertPred(S, pred, {{0, 1}, {1, 2}, {2, 3}});
		}}});
	}

	//! The test for the algorithm variants preferring paths over terminals
	static void callExpectPreferTerminals(const std::string spName,
			std::function<void(const Instance<T> &S, Arguments<T> &arg)> spAlg) {
		describe(spName + " preferring terminals heuristic", [&] {
			itMimicsOrdinaryShortestPath(spName, spAlg);

			testIt("marks the third terminal on a path of three terminals as unreachable (by predecessor only)",
			  2, spAlg, {{2, [&](const Instance<T> &S, const NodeArray<T> &distance, const NodeArray<edge> &pred) {
				assertHasNoPred(S, pred, {1, 2});
				assertPred(S, pred, {{0, -1}});
				assertDistanceTo(S, distance, {{0, 2}, {2, 0}});
				AssertThat(distance[S.v[1]], IsGreaterThan(3));
				AssertThat(distance[S.v[1]], IsLessThan(5));
			}}});

			testIt("prefers terminals in shortest paths",
			  3, spAlg, {{0, [&](const Instance<T> &S, const NodeArray<T> &distance, const NodeArray<edge> &pred) {
				assertHasNoPred(S, pred, {0, 2});
				assertPred(S, pred, {{1, -1}, {3, 0}, {4, 3}, {5, 4}});
				AssertThat(pred[S.v[1]]->opposite(S.v[1]), Equals(S.v[0]) || Equals(S.v[3]));
				assertDistanceTo(S, distance, {{0, 0}, {3, 1}, {4, 2}, {5, 3}, {1, 2}, {2, 4}});
			}}, {2, [&](const Instance<T> &S, const NodeArray<T> &distance, const NodeArray<edge> &pred) {
				assertHasNoPred(S, pred, {2, 0, 3});
				assertPred(S, pred, {{1, 2}, {4, 5}, {5, 2}});
				assertDistanceTo(S, distance, {{2, 0}, {3, 3}, {4, 2}, {5, 1}, {0, 4}, {1, 2}});
			}}});
		});
	}

	//! The test for the algorithm variants avoiding paths over terminals
	static void callExpectStandard(const std::string spName,
			std::function<void(const Instance<T> &S, Arguments<T> &arg)> spAlg) {
		describe(spName +" (standard)", [&] {
			itMimicsOrdinaryShortestPath(spName, spAlg);

			testIt("marks no terminal as unreachable",
			  2, spAlg, {{2, [&](const Instance<T> &S, const NodeArray<T> &distance, const NodeArray<edge> &pred) {
				assertHasNoPred(S, pred, {2});
				assertPred(S, pred, {{1, 0}, {0, 2}});
				assertDistanceTo(S, distance, {{1, 4}, {0, 2}, {2, 0}});
			}}});

			testIt("works on a graph with three terminals",
			  3, spAlg, {{0, [&](const Instance<T> &S, const NodeArray<T> &distance, const NodeArray<edge> &pred) {
				assertHasNoPred(S, pred, {0});
				assertPred(S, pred, {{1, -1}, {2, -1}, {3, -1}, {4, 3}, {5, 4}});
				AssertThat(pred[S.v[1]]->opposite(S.v[1]), Equals(S.v[0]) || Equals(S.v[3]));
				AssertThat(pred[S.v[2]]->opposite(S.v[2]), Equals(S.v[1]) || Equals(S.v[5]));
				AssertThat(pred[S.v[3]]->opposite(S.v[3]), Equals(S.v[0]) || Equals(S.v[1]));
				assertDistanceTo(S, distance, {{0, 0}, {3, 1}, {4, 2}, {5, 3}, {1, 2}, {2, 4}});
			}}, {2, [&](const Instance<T> &S, const NodeArray<T> &distance, const NodeArray<edge> &pred) {
				assertHasNoPred(S, pred, {2});
				assertPred(S, pred, {{0, -1}, {1, 2}, {3, -1}, {4, 5}, {5, 2}});
				AssertThat(pred[S.v[0]]->opposite(S.v[0]), Equals(S.v[1]) || Equals(S.v[3]));
				AssertThat(pred[S.v[3]]->opposite(S.v[3]), Equals(S.v[1]) || Equals(S.v[4]));
				assertDistanceTo(S, distance, {{2, 0}, {3, 3}, {4, 2}, {5, 1}, {0, 4}, {1, 2}});
			}}});
		});
	}
};

template<typename T>
static void ssspStandard(const Instance<T> &S, Arguments<T> &arg) {
	MinSteinerTreeModule<T>::allTerminalShortestPathsStandard(S.graph, S.terminals, S.isTerminal, arg.distance, arg.pred);
}

template<typename T>
static void ssspPrefer(const Instance<T> &S, Arguments<T> &arg) {
	MinSteinerTreeModule<T>::allTerminalShortestPathsPreferringTerminals(S.graph, S.terminals, S.isTerminal, arg.distance, arg.pred);
}

template<typename T>
static void ssspAllPairStandard(const Instance<T> &S, Arguments<T> &arg) {
	MinSteinerTreeModule<T>::allNodeShortestPathsStandard(S.graph, S.terminals, S.isTerminal, arg.distance, arg.pred);
}

template<typename T>
static void ssspAllPairPrefer(const Instance<T> &S, Arguments<T> &arg) {
	MinSteinerTreeModule<T>::allNodeShortestPathsPreferringTerminals(S.graph, S.terminals, S.isTerminal, arg.distance, arg.pred);
}

template<typename T>
static void apspStandard(const Instance<T> &S, Arguments<T> &arg) {
	MinSteinerTreeModule<T>::allPairShortestPathsStandard(S.graph, S.isTerminal, arg.distance, arg.pred);
}

template<typename T>
static void apspPrefer(const Instance<T> &S, Arguments<T> &arg) {
	MinSteinerTreeModule<T>::allPairShortestPathsPreferringTerminals(S.graph, S.isTerminal, arg.distance, arg.pred);
}

//! Tests for shortest path algorithms (SSSP and APSP variants)
template<typename T>
static void testShortestPathAlgorithms() {
	ModifiedShortestPathAlgorithmsTest<T>::callExpectStandard("all-terminal SSSP", ssspStandard<T>);
	ModifiedShortestPathAlgorithmsTest<T>::callExpectPreferTerminals("all-terminal SSSP", ssspPrefer<T>);
	ModifiedShortestPathAlgorithmsTest<T>::callExpectStandard("APSP", apspStandard<T>);
	ModifiedShortestPathAlgorithmsTest<T>::callExpectPreferTerminals("APSP", apspPrefer<T>);
	ModifiedShortestPathAlgorithmsTest<T>::callExpectStandard("all-node SSSP", ssspAllPairStandard<T>);
	ModifiedShortestPathAlgorithmsTest<T>::callExpectPreferTerminals("all-node SSSP", ssspAllPairPrefer<T>);
}

//! Test MinSteinerTreeModule<T>::isSteinerTree()
template<typename T>
static void testIsSteinerTree() {
	std::unique_ptr<Instance<T>> S;
	std::unique_ptr<EdgeWeightedGraphCopy<T>> tree;

	before_each([&] {
		S.reset(new Instance<T>({0, 2}, {{0, 1, 2}, {1, 2, 3}, {2, 0, 7}}));
		edge eCycle = S->graph.lastEdge();
		OGDF_ASSERT(eCycle->source() == S->v[2]);
		OGDF_ASSERT(eCycle->target() == S->v[0]);

		tree.reset(new EdgeWeightedGraphCopy<T>(S->graph));
		tree->delEdge(tree->copy(eCycle));
	});

	it("recognizes a valid Steiner tree", [&] {
		AssertThat(MinSteinerTreeModule<T>::isSteinerTree(S->graph, S->terminals, S->isTerminal, *tree), IsTrue());
	});

	it("recognizes a disconnected Steiner tree", [&] {
		tree->delEdge(tree->chooseEdge());

		AssertThat(MinSteinerTreeModule<T>::isSteinerTree(S->graph, S->terminals, S->isTerminal, *tree), IsFalse());
	});

	it("recognizes a Steiner tree with extra nodes", [&] {
		node v = S->graph.newNode();
		S->isTerminal[v] = true;
		S->terminals.pushFront(v);

		AssertThat(MinSteinerTreeModule<T>::isSteinerTree(S->graph, S->terminals, S->isTerminal, *tree), IsFalse());
	});

	it("recognizes a Steiner tree with redundant Steiner node", [&] {
		node u = S->terminals.front();
		node v = S->graph.newNode();
		edge e = S->graph.newEdge(u, v, 1);
		tree->newNode(v);
		tree->newEdge(e, 1);

		AssertThat(MinSteinerTreeModule<T>::isSteinerTree(S->graph, S->terminals, S->isTerminal, *tree), IsFalse());
	});
}

//! Test that the module handles calls on trivial instances
template<typename T>
static void testCallTrivial() {
	class MinSteinerTreeDummy : public MinSteinerTreeModule<T> {
	protected:
		T computeSteinerTree(const EdgeWeightedGraph<T> &G, const List<node> &terminals,
		                     const NodeArray<bool> &isTerminal,
		                     EdgeWeightedGraphCopy<T> *&finalSteinerTree) override {
			throw std::runtime_error("Not implemented.");
		}
	};

	MinSteinerTreeDummy dummy;
	Instance<T> S({},
	  {{1, 2, 26}, {2, 3, 16}, {3, 1, 10},
	   {0, 1, 15}, {0, 2, 14}, {0, 3, 1}});
	EdgeWeightedGraphCopy<T> *solution;

	before_each([&] {
		solution = nullptr;
	});

	after_each([&] {
		delete solution;
	});

	it("solves an instance without terminals", [&] {
		T cost = dummy.call(S.graph, S.terminals, S.isTerminal, solution);
		AssertThat(cost, Equals(0));
		AssertThat(solution, !IsNull());
		AssertThat(solution->empty(), IsTrue());
	});

	it("solves an instance with exactly one terminal", [&] {
		S.setTerminals({3});
		T cost = dummy.call(S.graph, S.terminals, S.isTerminal, solution);
		AssertThat(cost, Equals(0));
		AssertThat(solution, !IsNull());
		AssertThat(solution->numberOfNodes(), Equals(1));
		AssertThat(solution->numberOfEdges(), Equals(0));
		AssertThat(solution->original(solution->firstNode()), Equals(S.v[3]));
	});

	it("solves an instance with exactly two terminals", [&] {
		S.setTerminals({2, 1});
		T cost = dummy.call(S.graph, S.terminals, S.isTerminal, solution);
		AssertThat(cost, Equals(25));
		AssertThat(solution, !IsNull());
		AssertThat(solution->numberOfNodes(), Equals(4));
		AssertThat(solution->numberOfEdges(), Equals(3));
		AssertThat(MinSteinerTreeModule<T>::isSteinerTree(S.graph, S.terminals, S.isTerminal, *solution), IsTrue());
	});
}

template<typename T>
static void describeMinSteinerTreeModule(const std::string &&type) {
	describe("MinSteinerTreeModule<" + type + ">", [&] {
		describe("Modified shortest path algorithms", [] {
			testShortestPathAlgorithms<T>();
		});
		describe("isSteinerTree", [] {
			testIsSteinerTree<T>();
		});
		describe("call on trivial cases", [] {
			testCallTrivial<T>();
		});
	});
}

template<typename T>
static void assertTerminals(const Instance<T> &S, std::initializer_list<node> terminals) {
	for (node t : terminals) {
		AssertThat(S.isTerminal[t], IsTrue());
	}
}

template<typename T>
static void testFull2ComponentGenerator(const steiner_tree::Full2ComponentGenerator<T> &fcg) {
	Instance<T> S({0, 1, 2},
	  {{0, 3, 1}, {3, 2, 1}, {2, 4, 2}, {4, 1, 1},
	   {3, 5, 1}, {5, 4, 2}});

	it("generates full components with standard APSP", [&] {
		Arguments<T> arg;
		apspStandard(S, arg);

		int number = 0;
		fcg.call(S.graph, S.terminals, arg.distance, arg.pred,
		  [&](node u, node v, T minCost) {
			++number;
			assertTerminals(S, {u, v});
			std::set<std::set<node>> allowed = {{S.v[0], S.v[1]}, {S.v[0], S.v[2]}, {S.v[1], S.v[2]}};
			std::set<node> actual = {u, v};
			AssertThat(allowed, Contains(actual));
			if (std::set<node>{u, v} == std::set<node>{S.v[0], S.v[1]}) {
				AssertThat(epst.equal(minCost, T(5)), IsTrue());
			} else
			if (std::set<node>{u, v} == std::set<node>{S.v[0], S.v[2]}) {
				AssertThat(epst.equal(minCost, T(2)), IsTrue());
			} else
			if (std::set<node>{u, v} == std::set<node>{S.v[1], S.v[2]}) {
				AssertThat(epst.equal(minCost, T(3)), IsTrue());
			}
		});
		AssertThat(number, Equals(3));
	});

	it("generates full components with terminal-preferring APSP", [&] {
		Arguments<T> arg;
		apspPrefer(S, arg);

		int number = 0;
		fcg.call(S.graph, S.terminals, arg.distance, arg.pred,
		  [&](node u, node v, T minCost) {
			++number;
			assertTerminals(S, {u, v});
			std::set<std::set<node>> allowed = {{S.v[0], S.v[2]}, {S.v[1], S.v[2]}};
			std::set<node> actual = {u, v};
			AssertThat(allowed, Contains(actual));
			if (std::set<node>{u, v} == std::set<node>{S.v[0], S.v[2]}) {
				AssertThat(epst.equal(minCost, T(2)), IsTrue());
			} else
			if (std::set<node>{u, v} == std::set<node>{S.v[1], S.v[2]}) {
				AssertThat(epst.equal(minCost, T(3)), IsTrue());
			}
		});
		AssertThat(number, Equals(2));
	});
}

template<typename T>
static void testFull3ComponentGeneratorModule(std::string name, const steiner_tree::Full3ComponentGeneratorModule<T> &fcg) {
	describe(name, [&] {
		std::unique_ptr<Instance<T>> S; // a pointer because we may change the instance in the "it"s

		before_each([&] {
			S.reset(new Instance<T>(4));
		});

		it("generates full components with standard APSP", [&] {
			Arguments<T> arg;
			apspStandard(*S, arg);

			int number = 0;
			fcg.call(S->graph, S->terminals, S->isTerminal, arg.distance, arg.pred,
			  [&](node u, node v, node w, node center, T minCost) {
				++number;
				assertTerminals(*S, {u, v, w});
				AssertThat(center, Equals(S->v[5]));
			});
			AssertThat(number, IsGreaterThan(4) && IsLessThan(8));
		});

		it("generates full components with terminal-preferring APSP", [&] {
			Arguments<T> arg;
			apspPrefer(*S, arg);

			int number = 0;
			fcg.call(S->graph, S->terminals, S->isTerminal, arg.distance, arg.pred,
			  [&](node u, node v, node w, node center, T minCost) {
				++number;
				assertTerminals(*S, {u, v, w});
				AssertThat(S->isTerminal[center], IsFalse());
				AssertThat(u, !Equals<node>(S->v[4]));
				AssertThat(v, !Equals<node>(S->v[4]));
				AssertThat(w, !Equals<node>(S->v[4]));
				AssertThat(center, Equals<node>(S->v[5]));
			});
			AssertThat(number, IsGreaterThan(2) && IsLessThan(5));
		});

		it("omits generating 3-components that are dominated by 2-components", [&] {
			S->graph.newEdge(S->v[2], S->v[3], 1);
			S->graph.newEdge(S->v[3], S->v[1], 1);
			S->graph.newEdge(S->v[1], S->v[2], 1);

			Arguments<T> arg;
			apspPrefer(*S, arg);

			int number = 0;
			fcg.call(S->graph, S->terminals, S->isTerminal, arg.distance, arg.pred,
			  [&](node u, node v, node w, node center, T minCost) {
				++number;
			});
			AssertThat(number, Equals(0));
		});
	});
}

template<typename T>
static void testFullComponentGeneratorDreyfusWagner() {
	std::unique_ptr<Instance<T>> S;
	using FCG = steiner_tree::FullComponentGeneratorDreyfusWagner<T>;

	before_each([&] {
		S.reset(new Instance<T>(4));
	});

	auto testComponents = [&](const Arguments<T>& arg, const FCG& fcg, int k) {
		int nTotal = 0;
		int nValid = 0;

		SubsetEnumerator<node> terminalSubset(S->terminals);
		for (terminalSubset.begin(k); terminalSubset.valid(); terminalSubset.next()) {
			EdgeWeightedGraphCopy<T> component;
			List<node> terminals;
			terminalSubset.list(terminals);
			fcg.getSteinerTreeFor(terminals, component);
			if (fcg.isValidComponent(component)) {
				for (node t : terminals) {
					AssertThat(component.copy(t)->degree(), Equals(1));
				};
				++nValid;
			}
			++nTotal;
		};
		AssertThat(nTotal, Equals(Math::binomial(S->terminals.size(), k)));
		return nValid;
	};

	it("generates full components with standard APSP", [&] {
		Arguments<T> arg;
		apspStandard(*S, arg);

		FCG fcg(S->graph, S->terminals, S->isTerminal, arg.distance, arg.pred);
		fcg.call(5);

		AssertThat(testComponents(arg, fcg, 2), Equals(10));
		AssertThat(testComponents(arg, fcg, 3), Equals(7));
		AssertThat(testComponents(arg, fcg, 4), Equals(2));
		AssertThat(testComponents(arg, fcg, 5), Equals(0));
	});

	it("generates full components with terminal-preferring APSP", [&] {
		Arguments<T> arg;
		apspPrefer(*S, arg);

		FCG fcg(S->graph, S->terminals, S->isTerminal, arg.distance, arg.pred);
		fcg.call(5);

		AssertThat(testComponents(arg, fcg, 2), Equals(7));
		AssertThat(testComponents(arg, fcg, 3), Equals(4));
		AssertThat(testComponents(arg, fcg, 4), Equals(1));
		AssertThat(testComponents(arg, fcg, 5), Equals(0));
	});

	it("omits generating 3-components that are dominated by 2-components", [&] {
		S->graph.newEdge(S->v[2], S->v[3], 1);
		S->graph.newEdge(S->v[3], S->v[1], 1);
		S->graph.newEdge(S->v[1], S->v[2], 1);

		Arguments<T> arg;
		apspPrefer(*S, arg);

		FCG fcg(S->graph, S->terminals, S->isTerminal, arg.distance, arg.pred);
		fcg.call(3);

		AssertThat(testComponents(arg, fcg, 3), Equals(0));
	});
}

template<typename T>
static void describeFullComponentGenerators(const std::string &&type) {
	describe("Full2ComponentGenerator<" + type + ">", [&] {
		steiner_tree::Full2ComponentGenerator<T> fcg;
		testFull2ComponentGenerator(fcg);
	});
	describe("Full3ComponentGeneratorModule<" + type + ">", [&] {
		steiner_tree::Full3ComponentGeneratorVoronoi<T> fcgVoronoi;
		testFull3ComponentGeneratorModule("Voronoi", fcgVoronoi);

		steiner_tree::Full3ComponentGeneratorEnumeration<T> fcgEnumeration;
		testFull3ComponentGeneratorModule("Enumeration", fcgEnumeration);
	});
	describe("FullComponentGeneratorDreyfusWagner<" + type + ">", [&] {
		testFullComponentGeneratorDreyfusWagner<T>();
	});
}

template<typename T>
static void testFullComponentStore(std::unique_ptr<steiner_tree::FullComponentStore<T>>& fcs,
		const Instance<T>& S,
		const EdgeWeightedGraphCopy<T>& component, const EdgeWeightedGraphCopy<T>& path) {
	describe("only one 2-component", [&] {
		before_each([&] {
			fcs->insert(path);
		});

		it("inserts the component", [&] {
			AssertThat(fcs->isEmpty(), IsFalse());
			AssertThat(fcs->size(), Equals(1));
			AssertThat(fcs->terminals(0).size(), Equals(2));
			AssertThat(fcs->terminals(0)[0]->index(), Equals(0));
			AssertThat(fcs->terminals(0)[1]->index(), Equals(1));
			AssertThat(fcs->graph().numberOfNodes(), Equals(4));
			AssertThat(fcs->graph().numberOfEdges(), Equals(1));
		});

		it("iterates over all critical nodes only", [&] {
			NodeArray<int> marked(S.graph, 0);

			fcs->foreachNode(0, [&](node v) {
				++marked[v];
			});

			AssertThat(marked[S.v[0]], Equals(1));
			AssertThat(marked[S.v[1]], Equals(1));
		});

		it("iterates over all nodes using predecessor matrix", [&] {
			Arguments<T> arg;
			MinSteinerTreeModule<T>::allPairShortestPathsPreferringTerminals(S.graph, S.isTerminal, arg.distance, arg.pred);
			NodeArray<int> marked(S.graph, 0);

			fcs->foreachNode(0, arg.pred, [&](node v) {
				++marked[v];
			});

			for (int i : {2, 3, 6, 7}) {
				AssertThat(marked[S.v[i]], Equals(0));
			}
			for (int i : {0, 1, 4, 5, 8}) {
				AssertThat(marked[S.v[i]], Equals(1));
			}
		});
	});

	describe("only one 4-component", [&] {
		before_each([&] {
			fcs->insert(component);
		});

		it("inserts the component", [&] {
			AssertThat(fcs->isEmpty(), IsFalse());
			AssertThat(fcs->size(), Equals(1));
			AssertThat(fcs->terminals(0).size(), Equals(4));
			AssertThat(fcs->terminals(0)[0]->index(), Equals(0));
			AssertThat(fcs->terminals(0)[1]->index(), Equals(1));
			AssertThat(fcs->terminals(0)[2]->index(), Equals(2));
			AssertThat(fcs->terminals(0)[3]->index(), Equals(3));
			AssertThat(fcs->graph().numberOfNodes(), Equals(5));
			AssertThat(fcs->graph().numberOfEdges(), Equals(4));
		});

		it("iterates over all critical nodes only", [&] {
			NodeArray<int> marked(S.graph, 0);

			fcs->foreachNode(0, [&](node v) {
				++marked[v];
			});

			AssertThat(marked[S.v[0]], Equals(1));
			AssertThat(marked[S.v[1]], Equals(1));
			AssertThat(marked[S.v[2]], Equals(1));
			AssertThat(marked[S.v[3]], Equals(1));
			AssertThat(marked[S.v[4]], Equals(0));
			AssertThat(marked[S.v[5]], Equals(0));
			AssertThat(marked[S.v[6]], Equals(0));
			AssertThat(marked[S.v[7]], Equals(0));
			AssertThat(marked[S.v[8]], Equals(1));
		});

		it("iterates over all nodes using predecessor matrix", [&] {
			Arguments<T> arg;
			MinSteinerTreeModule<T>::allPairShortestPathsPreferringTerminals(S.graph, S.isTerminal, arg.distance, arg.pred);
			NodeArray<int> marked(S.graph, 0);

			fcs->foreachNode(0, arg.pred, [&](node v) {
				++marked[v];
			});

			for (int count : marked) {
				AssertThat(count, Equals(1));
			}
		});
	});

	describe("one 2-component and one 4-component", [&] {
		before_each([&] {
			fcs->insert(path);
			fcs->insert(component);
		});

		it("inserts the components", [&] {
			AssertThat(fcs->isEmpty(), IsFalse());
			AssertThat(fcs->size(), Equals(2));
			AssertThat(fcs->terminals(0).size(), Equals(2));
			AssertThat(fcs->terminals(0)[0]->index(), Equals(0));
			AssertThat(fcs->terminals(0)[1]->index(), Equals(1));
			AssertThat(fcs->terminals(1).size(), Equals(4));
			AssertThat(fcs->terminals(1)[0]->index(), Equals(0));
			AssertThat(fcs->terminals(1)[1]->index(), Equals(1));
			AssertThat(fcs->terminals(1)[2]->index(), Equals(2));
			AssertThat(fcs->terminals(1)[3]->index(), Equals(3));
			AssertThat(fcs->graph().numberOfNodes(), Equals(5));
			AssertThat(fcs->graph().numberOfEdges(), Equals(5));
		});

		it("removes the components", [&] {
			fcs->remove(0);
			AssertThat(fcs->isEmpty(), IsFalse());
			fcs->remove(0);
			AssertThat(fcs->isEmpty(), IsTrue());
		});
	});

	it("inserts the same component twice", [&] {
		fcs->insert(component);
		fcs->insert(component);
		AssertThat(fcs->isEmpty(), IsFalse());
		AssertThat(fcs->size(), Equals(2));
		AssertThat(fcs->terminals(0).size(), Equals(4));
		AssertThat(fcs->terminals(1).size(), Equals(4));
		AssertThat(fcs->graph().numberOfNodes(), Equals(6));
		AssertThat(fcs->graph().numberOfEdges(), Equals(8));
	});
}

template<typename T>
static void describeFullComponentStore(const std::string&& type) {
	describe("FullComponentStore<" + type + ">", [&] {
		Instance<T> S({0, 1, 2, 3}, {
		  {0, 4, 1}, {4, 8, 1},
		  {1, 5, 1}, {5, 8, 1},
		  {2, 6, 1}, {6, 8, 1},
		  {3, 7, 1}, {7, 8, 1},
		});

		std::unique_ptr<steiner_tree::FullComponentStore<T>> fcs;
		before_each([&] {
			fcs.reset(new steiner_tree::FullComponentStore<T>(S.graph, S.terminals, S.isTerminal));
		});

		it("is empty when nothing is inserted", [&] {
			AssertThat(fcs->isEmpty(), IsTrue());
		});

		describe("containing component with degree-2 nodes", [&] {
			EdgeWeightedGraphCopy<T> copy(S.graph);
			EdgeWeightedGraphCopy<T> path(S.graph);
			for (int i : {2, 3, 6, 7}) {
				path.delNode(path.copy(S.v[i]));
			}

			testFullComponentStore(fcs, S, copy, path);
		});

		describe("containing component without degree-2 nodes", [&] {
			EdgeWeightedGraphCopy<T> component(S.graph);
			for (int i : {4, 5, 6, 7}) {
				component.delNode(component.copy(S.v[i]));
			}
			for (int i : {0, 1, 2, 3}) {
				component.newEdge(component.copy(S.v[i]), component.copy(S.v[8]), 2);
			}

			EdgeWeightedGraphCopy<T> path;
			path.createEmpty(S.graph);
			path.newNode(S.v[0]);
			path.newNode(S.v[1]);
			path.newEdge(path.firstNode(), path.lastNode(), 4);

			testFullComponentStore(fcs, S, component, path);
		});
	});
}

go_bandit([]{
	describe("Steiner tree approximation helpers", [] {
		describeMinSteinerTreeModule<int>("int");
		describeMinSteinerTreeModule<double>("double");
		describeFullComponentGenerators<int>("int");
		describeFullComponentGenerators<double>("double");
		describeFullComponentStore<int>("int");
		describeFullComponentStore<double>("double");
	});
});
