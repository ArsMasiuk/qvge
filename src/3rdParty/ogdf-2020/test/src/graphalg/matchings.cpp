/** \file
 * \brief Tests for matching algorithms
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

#include <ogdf/graphalg/Matching.h>
#include <graphs.h>
#include <testing.h>

static std::vector<edge> getEdges(const Graph &graph, std::set<int>&& edgeIndices) {
	std::vector<edge> result;
	int idx{0};
	for (edge e : graph.edges) {
		if (edgeIndices.find(idx) != edgeIndices.end()) {
			result.push_back(e);
		}
		++idx;
	}

	return result;
}

static void describeIsMatching() {
	it("identifies a disconnected matching graph as matching", [&] {
		Graph graph;
		customGraph(graph, 8, {{2, 5}, {6, 0}, {4, 3}, {1, 7}});
		AssertThat(Matching::isMatching(graph, graph.edges), IsTrue());
	});

	it("accepts matchings on self-loops", [&] {
		Graph graph;
		customGraph(graph, 3, {{0, 0}, {0, 1}, {1, 1}, {1, 2}, {2, 2}});
		AssertThat(Matching::isMatching(graph, getEdges(graph, {0, 2, 4})), IsTrue());
		AssertThat(Matching::isMatching(graph, getEdges(graph, {0, 3})), IsTrue());
		AssertThat(Matching::isMatching(graph, getEdges(graph, {1, 4})), IsTrue());
	});

	it("accepts matchings on a graph with parallel edges", [&] {
		Graph graph;
		customGraph(graph, 2, {{0, 1}, {0, 1}, {0, 1}, {0, 1}});
		for (edge e : graph.edges) {
			AssertThat(Matching::isMatching(graph, List<edge>{e}), IsTrue());
		}
	});

	Graph graph;
	Array<node> nodes;
	customGraph(graph, 5, {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}, {3, 1}}, nodes);

	it("accepts an empty set", [&] {
		AssertThat(Matching::isMatching(graph, List<edge>{}), IsTrue());
	});

	it("does not identify all edges as matching on a graph with minimum degree 2", [&] {
		AssertThat(Matching::isMatching(graph, graph.edges), IsFalse());
	});
}

static void describeIsMaximal() {
	it("accepts an empty matching on isolated nodes", [] {
		Graph graph;
		emptyGraph(graph, 10);
		AssertThat(Matching::isMaximal(graph, List<edge>{}), IsTrue());
		edge addable{graph.firstEdge()}; // initialize with wrong value
		AssertThat(Matching::isMaximal(graph, List<edge>{}, addable), IsTrue());
		AssertThat(addable, IsNull());
	});

	it("accepts a maximal but not maximum matching", [] {
		Graph graph;
		customGraph(graph, 6, {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}});
		edge addable{graph.firstEdge()}; // initialize with wrong value
		AssertThat(Matching::isMaximal(graph, getEdges(graph, {1, 3}), addable), IsTrue());
		AssertThat(addable, IsNull());
	});

	it("finds an edge that can be added to the matching", [] {
		Graph graph;
		customGraph(graph, 6, {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}});
		edge addable{nullptr};
		AssertThat(Matching::isMaximal(graph, getEdges(graph, {2, 4}), addable), IsFalse());
		AssertThat(addable, Equals(graph.firstEdge()));
	});

	it("accepts a maximal edge set that is not a matching", [] {
		Graph graph;
		customGraph(graph, 5, {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {3, 4}, {4, 1}});
		AssertThat(Matching::isMaximal(graph, getEdges(graph, {0, 1, 2, 4})), IsTrue());
	});

	it("finds an edge that can be added although the edge set is not a matching", [] {
		Graph graph;
		customGraph(graph, 5, {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {4, 1}, {3, 4}});
		edge addable{nullptr};
		AssertThat(Matching::isMaximal(graph, getEdges(graph, {0, 1, 2}), addable), IsFalse());
		AssertThat(addable, Equals(graph.lastEdge()));
	});
}

static void describeIsPerfectMatching() {
	it("accepts perfect matchings", [] {
		Graph graph;
		customGraph(graph, 4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}});
		AssertThat(Matching::isPerfectMatching(graph, getEdges(graph, {0, 2})), IsTrue());
		AssertThat(Matching::isPerfectMatching(graph, getEdges(graph, {1, 3})), IsTrue());
	});

	it("rejects maximal but not perfect matchings", [] {
		Graph graph;
		customGraph(graph, 3, {{0, 1}, {1, 2}, {2, 0}});
		AssertThat(Matching::isPerfectMatching(graph, getEdges(graph, {0})), IsFalse());
		AssertThat(Matching::isPerfectMatching(graph, getEdges(graph, {1})), IsFalse());
		AssertThat(Matching::isPerfectMatching(graph, getEdges(graph, {2})), IsFalse());
	});

	it("rejects non-matchings", [] {
		Graph graph;
		customGraph(graph, 4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}});
		for (auto&& edgeSet : List<std::set<int>>{{0, 1}, {0, 3}, {1, 2}, {2, 3}}) {
			AssertThat(Matching::isPerfectMatching(graph, getEdges(graph, std::move(edgeSet))), IsFalse());
		}
	});
}

static void describeMaximalMatching() {
	forEachGraphItWorks({}, [](const Graph& graph) {
		ArrayBuffer<edge> matching;
		Matching::findMaximalMatching(graph, matching);
		AssertThat(Matching::isMaximalMatching(graph, matching), IsTrue());
	});
}

go_bandit([] {
	describe("Matching algorithms", [] {
		describe("isMatching()", [] {
			describeIsMatching();
		});

		describe("isMaximal()", [] {
			describeIsMaximal();
		});

		describe("isPerfectMatching()", [] {
			describeIsPerfectMatching();
		});

		describe("findMaximalMatching()", [] {
			describeMaximalMatching();
		});
	});
});
