/** \file
 * \brief Tests for EdgeIndependentSpanningTrees class
 *
 * \author Manuel Fiedler
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

#include <ogdf/graphalg/EdgeIndependentSpanningTrees.h>
#include <graphs.h>
#include <testing.h>

go_bandit([] {
	describe("algorithm to find edge-independent spanning trees of a graph", [] {
		it("tests whether a graph with one node has no edge-independent spanning trees", [] {
			Graph G;
			node root = G.newNode();
			EdgeIndependentSpanningTrees E(G, root);
			EdgeIndependentSpanningTrees::Solution f;
			AssertThat(E.getRoot(), Equals(root));
			AssertThat(E.findOne(1,f), IsFalse());
			AssertThat(E.findOne(2,f), IsFalse());
			AssertThat(E.findOne(3,f), IsFalse());
			AssertThat(E.findOne(4,f), IsFalse());
		});

		it("calculates the number of edge-independent spanning trees of K4", [] {
			Graph G;
			completeGraph(G,4);
			EdgeIndependentSpanningTrees E;
			List<EdgeIndependentSpanningTrees::Solution> f2,f3;
			E.setGraph(G);
			E.setRoot(G.firstNode());
			f2 = E.findAll(2);
			f3 = E.findAll(3);
			AssertThat(f2.size(), Equals(30));
			AssertThat(f3.size(), Equals(1));
		});

		it("checks whether the number of edge-independent spanning trees with and without permutation agrees", [] {
			Graph G;
			randomSimpleConnectedGraph(G, 5, 9);

			EdgeIndependentSpanningTrees E(G);
			List<EdgeIndependentSpanningTrees::Solution> f, g;

			f = E.findAll(2);
			g = E.findAllPerm(2);
			AssertThat(2*f.size(), Equals(g.size()));
			f = E.findAll(3);
			g = E.findAllPerm(3);
			AssertThat(6*f.size(), Equals(g.size()));
		});
	});
});
