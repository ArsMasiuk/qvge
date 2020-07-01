/** \file
 * \brief Tests for LCA (lowest common ancestor) class
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

#include <sstream>
#include <ogdf/basic/Graph.h>
#include <ogdf/tree/LCA.h>
#include <ogdf/basic/graph_generators.h>
#include <graphs.h>
#include <testing.h>

static void trivial() {
	it("constructs LCA data structure on an empty graph", [] {
		Graph G;
		LCA lca(G);
	});

	it("answers level query on an arborescence with one node", [] {
		Graph G;
		node root = G.newNode();
		LCA lca(G);
		AssertThat(lca.level(root), Equals(0));
	});

	it("answers LCA query on an arborescence with one node", [] {
		Graph G;
		node root = G.newNode();
		LCA lca(G);
		node commonAncestor = lca.call(root, root);
		AssertThat(commonAncestor, Equals(root));
	});

	it("answers LCA queries on an arborescence with two nodes", [] {
		Graph G;
		customGraph(G, 2, {{0, 1}});
		LCA lca(G);
		AssertThat(lca.call(G.firstNode(), G.firstNode()), Equals(G.firstNode()));
		AssertThat(lca.call(G.lastNode(), G.firstNode()), Equals(G.firstNode()));
		AssertThat(lca.call(G.firstNode(), G.lastNode()), Equals(G.firstNode()));
		AssertThat(lca.call(G.lastNode(), G.lastNode()), Equals(G.lastNode()));
	});

	it("answers LCA queries on an arborescence with three nodes", [] {
		Graph G;
		Array<node> nodes;
		customGraph(G, 3, {{0, 1}, {0, 2}}, nodes);
		LCA lca(G);
		AssertThat(lca.call(nodes[0], nodes[0]), Equals(nodes[0]));
		AssertThat(lca.call(nodes[0], nodes[1]), Equals(nodes[0]));
		AssertThat(lca.call(nodes[0], nodes[2]), Equals(nodes[0]));
		AssertThat(lca.call(nodes[1], nodes[0]), Equals(nodes[0]));
		AssertThat(lca.call(nodes[1], nodes[1]), Equals(nodes[1]));
		AssertThat(lca.call(nodes[1], nodes[2]), Equals(nodes[0]));
		AssertThat(lca.call(nodes[2], nodes[0]), Equals(nodes[0]));
		AssertThat(lca.call(nodes[2], nodes[1]), Equals(nodes[0]));
		AssertThat(lca.call(nodes[2], nodes[2]), Equals(nodes[2]));
	});
}

static void interesting() {
	const List<std::pair<int,int>> arborescence({
		{4, 0},
		{5, 1}, {5, 2}, {5, 3}, {5, 4},
		{7, 6}, {7, 5},
		{13, 9}, {13, 11}, {13, 12},
		{11, 10},
		{9, 8}, {9, 7}
	});
	Graph G;
	Array<node> nodes;
	customGraph(G, 14, arborescence, nodes);

	it("answers level queries on a more interesting arborescence", [&] {
		LCA lca(G);
		AssertThat(lca.level(nodes[0]), Equals(5));
		AssertThat(lca.level(nodes[1]), Equals(4));
		AssertThat(lca.level(nodes[5]), Equals(3));
		AssertThat(lca.level(nodes[7]), Equals(2));
		AssertThat(lca.level(nodes[10]), Equals(2));
		AssertThat(lca.level(nodes[11]), Equals(1));
		AssertThat(lca.level(nodes[13]), Equals(0));
	});

	it("answers LCA queries on a more interesting arborescence", [&] {
		LCA lca(G);
		AssertThat(lca.call(nodes[0], nodes[0]), Equals(nodes[0]));
		AssertThat(lca.call(nodes[0], nodes[1]), Equals(nodes[5]));
		AssertThat(lca.call(nodes[0], nodes[4]), Equals(nodes[4]));
		AssertThat(lca.call(nodes[4], nodes[1]), Equals(nodes[5]));
		AssertThat(lca.call(nodes[6], nodes[0]), Equals(nodes[7]));
		AssertThat(lca.call(nodes[6], nodes[0]), Equals(nodes[7]));
		AssertThat(lca.call(nodes[8], nodes[5]), Equals(nodes[9]));
		AssertThat(lca.call(nodes[10], nodes[1]), Equals(nodes[13]));
	});

	it("answers LCA queries when initialization is on sub-arborescence", [&] {
		LCA lca(G, nodes[5]);
		AssertThat(lca.call(nodes[0], nodes[0]), Equals(nodes[0]));
		AssertThat(lca.call(nodes[0], nodes[1]), Equals(nodes[5]));
		AssertThat(lca.call(nodes[0], nodes[4]), Equals(nodes[4]));
		AssertThat(lca.call(nodes[4], nodes[1]), Equals(nodes[5]));
		AssertThat(lca.call(nodes[2], nodes[3]), Equals(nodes[5]));
		AssertThat(lca.call(nodes[5], nodes[5]), Equals(nodes[5]));
	});
}

go_bandit([] {
	describe("Lowest Common Ancestor algorithm", [] {
		describe("on trivial arborescences", [] {
			trivial();
		});

		describe("on more interesting arborescence", [] {
			interesting();
		});

		describe("on arborescences of varying sizes", [] {
			forEachGraphItWorks(
				{GraphProperty::arborescenceForest, GraphProperty::connected},
				[&](const Graph& G) {
					LCA lca(G);
					for (node v : G.nodes) {
						for (node w : G.nodes) {
							int lcaLevel = lca.level(lca.call(v, w));
							AssertThat(lcaLevel, IsLessThanOrEqualTo(lca.level(v)));
							AssertThat(lcaLevel, IsLessThanOrEqualTo(lca.level(w)));
						}
					}
				},
				GraphSizes(10, 1000, 10)
			);
		});
	});
});
