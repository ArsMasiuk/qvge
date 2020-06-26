/** \file
 * \brief Tests for ogdf::DualGraph.
 *
 * \author Mirko Wagner, Tilo Wiedera
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

#include <ogdf/basic/DualGraph.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/basic/extended_graph_alg.h>

#include <graphs.h>
#include <testing.h>

//! Creates a DualGraph of \p graph and runs several tests on it.
void describeDualGraph(Graph &graph) {
	if (graph.numberOfEdges() < 1) {
		return;
	}
	planarEmbed(graph);
	ConstCombinatorialEmbedding emb(graph);
	DualGraph dual(emb);

	it("returns its primal embedding", [&] {
		AssertThat(&dual.getPrimalEmbedding(), Equals(&emb));
	});

	it("returns its primal graph", [&] {
		AssertThat(&dual.getPrimalGraph(), Equals(&graph));
	});

	it("has a matching number of nodes, faces, and edges", [&] {
		AssertThat(dual.numberOfFaces(), Equals(graph.numberOfNodes()));
		AssertThat(dual.getGraph().numberOfNodes(), Equals(emb.numberOfFaces()));
		AssertThat(dual.getGraph().numberOfEdges(), Equals(graph.numberOfEdges()));
	});

	it("maps primal faces to dual nodes", [&] {
		for (face f : emb.faces) {
			node v = dual.dualNode(f);
			AssertThat(dual.primalFace(v), Equals(f));
			AssertThat(v->degree(), Equals(f->size()));
		}
	});

	it("maps primal nodes to dual faces", [&] {
		for (node v: graph.nodes) {
			face f = dual.dualFace(v);
			AssertThat(dual.primalNode(f), Equals(v));
			AssertThat(f->size(), Equals(v->degree()));
		}
	});

	it("maps edges and faces", [&] {
		for (edge e : graph.edges) {
			edge g = dual.dualEdge(e);
			AssertThat(g, !Equals(e));
			AssertThat(dual.primalEdge(g), Equals(e));

			face f = dual.primalFace(g->source());
			AssertThat(f, Equals(emb.rightFace(e->adjSource())));
			AssertThat(f, Equals(emb.leftFace(e->adjTarget())));

			f = dual.primalFace(g->target());
			AssertThat(f, Equals(emb.leftFace(e->adjSource())));
			AssertThat(f, Equals(emb.rightFace(e->adjTarget())));
		}
	});
}

go_bandit([]() {
	describe("DualGraph",[] {
		forEachGraphDescribe({GraphProperty::planar, GraphProperty::connected},
			[&](Graph &graph) { describeDualGraph(graph); }
		);
	});
});
