/** \file
 * \brief Tests for ogdf::ConstCombinatorialEmbedding and ogdf::CombinatorialEmbedding.
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

#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/Math.h>
#include <ogdf/basic/FaceArray.h>

#include <testing.h>
#include <graphs.h>

constexpr int NUMBER_OF_ITERATIONS = 17;
constexpr int NUMBER_OF_NODES = 100;
constexpr int NUMBER_OF_EDGES = 200;

//! Runs a single iteration of generic tests that do not modify the \c graph.
template<typename T>
void testConstCombinatorialEmbedding(Graph &graph) {
	OGDF_ASSERT(graph.representsCombEmbedding());

	T emb(graph);

	it("returns its graph", [&] {
		AssertThat(emb.valid(), IsTrue());
		AssertThat(&emb.getGraph(), Equals(&graph));
		AssertThat(&((const Graph&)(emb)), Equals(&graph));
	});

	it("iterates faces", [&] {
		face f = emb.firstFace();
		AssertThat(f->index(), Equals(0));
		AssertThat(f->pred(), IsNull());

		int counter = 0;
		for(; f != nullptr; f = f->succ()) {
			counter++;
		}

		AssertThat(counter, Equals(emb.numberOfFaces()));
	});

	it("iterates faces in reverse", [&] {
		face f = emb.lastFace();
		AssertThat(f->index(), Equals(emb.maxFaceIndex()));
		AssertThat(f->succ(), IsNull());

		int counter = 0;
		for(; f != nullptr; f = f->pred()) {
			counter++;
		}

		AssertThat(counter, Equals(emb.numberOfFaces()));
	});

	it("returns a maximal face", [&] {
		int maxSize = -1;
		for(face f : emb.faces) {
			Math::updateMax(maxSize, f->size());
		}

		AssertThat(emb.maximalFace()->size(), Equals(maxSize));
	});

	it("chooses a random face", [&] {
		for(int i = 0; i < 20; i++) {
			AssertThat(emb.chooseFace(), !Equals(nullptr));
		}
	});

	it("supports setting an external face", [&] {
		AssertThat(emb.externalFace(), Equals(nullptr));
		face f = emb.chooseFace();
		emb.setExternalFace(f);
		AssertThat(emb.externalFace(), Equals(f));
	});

	it("creates faces with correct size", [&] {
		int sizesSum = 0;
		for(face f : emb.faces) {
			sizesSum += f->size();
		}

		AssertThat(sizesSum, Equals(graph.numberOfEdges() * 2));
	});

	it("returns all left and right faces", [&] {
		FaceArray<bool> visited(emb, false);

		for(edge e : graph.edges) {
			adjEntry adj = e->adjSource();
			visited[emb.leftFace(adj)] = true;
			visited[emb.rightFace(adj)] = true;
		}

		for(face f : emb.faces) {
			AssertThat(visited[f], IsTrue());
		}
	});
}

//! Create K4 rotation system with a single crossing
void createBadK4(Graph &graph) {
	completeGraph(graph, 4);
	planarEmbed(graph);
	adjEntry adj = graph.chooseNode()->firstAdj();
	graph.moveAdjAfter(adj, adj->succ());
}

//! Runs tests that apply for ogdf::ConstCombinatorialEmbedding and ogdf::CombinatorialEmbedding.
//! Also executes several iterations of generic tests.
template<typename T>
void testConstCombinatorialEmbedding() {
	Graph planarGraph;
	randomPlanarConnectedGraph(planarGraph, NUMBER_OF_NODES, NUMBER_OF_EDGES);
	Graph K5;
	completeGraph(K5, 5);
	Graph badK4;
	createBadK4(badK4);

	describe("initialization",[&] {
		it("works", [&] {
			T emb(planarGraph);
			AssertThat(emb.valid(), IsTrue());
			AssertThat(&emb.getGraph(), Equals(&planarGraph));
		});

		it("works w/o a graph", [&] {
			T emb;
			AssertThat(emb.valid(), IsFalse());
		});

#ifdef OGDF_USE_ASSERT_EXCEPTIONS
		it("rejects graphs that are not embedded", [&] {
			AssertThrows(AssertionFailed, CombinatorialEmbedding(K5));
			AssertThrows(AssertionFailed, CombinatorialEmbedding(badK4));
		});
#endif

		it("works using init()", [&] {
			T emb;
			emb.init(planarGraph);
			AssertThat(emb.valid(), IsTrue());
			AssertThat(&emb.getGraph(), Equals(&planarGraph));
		});

#ifdef OGDF_USE_ASSERT_EXCEPTIONS
		it("rejects graphs that are not embedded using init()", [&] {
			T emb;
			AssertThrows(AssertionFailed, emb.init(K5));
			AssertThrows(AssertionFailed, emb.init(badK4));
		});
#endif
	});

	it("works on a single loop", [&] {
		Graph graph;
		node v = graph.newNode();
		graph.newEdge(v, v);
		T emb(graph);

		AssertThat(emb.numberOfFaces(), Equals(2));
		adjEntry adj = v->firstAdj();
		AssertThat(emb.leftFace(adj), !Equals(emb.rightFace(adj)));
		adj = v->lastAdj();
		AssertThat(emb.leftFace(adj), !Equals(emb.rightFace(adj)));
	});

	it("works on a K3 with a dangling node", [&] {
		Graph graph;
		completeGraph(graph, 3);
		node w = graph.chooseNode();
		node v = graph.newNode();
		edge e = graph.newEdge(v, w);
		T emb(graph);

		AssertThat(emb.numberOfFaces(), Equals(2));
		adjEntry adj = v->firstAdj();
		AssertThat(emb.leftFace(adj), Equals(emb.rightFace(adj)));

		for(edge f : graph.edges) {
			AssertThat(emb.isBridge(f), Equals(f == e));
		}
	});

	it("works on a triconnected graph", [&] {
		Graph graph;
		randomPlanarTriconnectedGraph(graph, NUMBER_OF_NODES, NUMBER_OF_EDGES);
		T emb(graph);

		int counter = 0;
		int size = 0;
		for(face f : emb.faces){
			counter++;
			size += f->size();
		}

		AssertThat(size, Equals(graph.numberOfEdges()*2));
		AssertThat(counter, Equals(emb.numberOfFaces()));
	});

	it("knows which faces are incident to a node or edge on a K3", [&] {
		Graph graph;
		node u = graph.newNode();
		node v = graph.newNode();
		node w = graph.newNode();
		edge e = graph.newEdge(u, v);
		edge f = graph.newEdge(v, w);
		edge g = graph.newEdge(w, u);
		T emb(graph);
		AssertThat(u->firstAdj()->theEdge(), Equals(e));
		face rightFace = emb.rightFace(e->adjSource());
		AssertThat(emb.rightFace(f->adjSource()), Equals(rightFace));
		AssertThat(emb.rightFace(g->adjSource()), Equals(rightFace));
		face leftFace = emb.leftFace(e->adjSource());
		AssertThat(emb.leftFace(f->adjSource()), Equals(leftFace));
		AssertThat(emb.leftFace(g->adjSource()), Equals(leftFace));
		AssertThat(emb.numberOfFaces(), Equals(2));
	});

	it("detects bridges on a tree", [&] {
		Graph graph;
		randomTree(graph, NUMBER_OF_NODES);
		T emb(graph);

		AssertThat(emb.numberOfFaces(), Equals(1));

		for(edge e : graph.edges){
			AssertThat(emb.isBridge(e), IsTrue());
		}
	});

	it("detects bridges", [&] {
		Graph graph;
		randomPlanarBiconnectedGraph(graph, NUMBER_OF_NODES, NUMBER_OF_EDGES);

		EdgeArray<bool> isBridge(graph, false);
		node chosenNode = graph.chooseNode();
		node v = chosenNode;
		for(int i = 0; i < NUMBER_OF_NODES; i++) {
			node u = graph.newNode();
			edge e = graph.newEdge(v, u);
			v = u;
			isBridge[e] = true;
		}

		T emb(graph);

		for(edge e : graph.edges) {
			AssertThat(emb.isBridge(e), Equals(isBridge[e]));
		}

		graph.newEdge(v, chosenNode);
		planarEmbed(graph);

		emb.computeFaces();

		for(edge e : graph.edges) {
			AssertThat(emb.isBridge(e), IsFalse());
		}
	});

	it("returns a sane size of its face array", [&] {
		Graph graph;
		randomPlanarTriconnectedGraph(graph, NUMBER_OF_NODES*10, NUMBER_OF_EDGES*10);
		T emb(graph);
		AssertThat(emb.faceArrayTableSize(), IsGreaterThan(emb.numberOfFaces() - 1));
	});

	for(int i = 1; i <= NUMBER_OF_ITERATIONS; i++) {
		describe("iteration #" + to_string(i), [&] {
			Graph graph;
			randomPlanarConnectedGraph(graph, NUMBER_OF_NODES, NUMBER_OF_EDGES);
			testConstCombinatorialEmbedding<T>(graph);
		});
	}

	forEachGraphItWorks({GraphProperty::planar, GraphProperty::connected}, [&](Graph &graph) {
		planarEmbed(graph);
		T emb(graph);

		int phi = 0;
		int size = 0;
		for (face f : emb.faces) {
			phi++;
			size += f->size();
		}

		// Test whether Euler's formula holds.
		int n = graph.numberOfNodes();
		int m = graph.numberOfEdges();
		AssertThat(phi, Equals(emb.numberOfFaces()));
		AssertThat(size, Equals(2*m));
		if (graph.numberOfEdges() > 0) {
			AssertThat(n - m + phi, Equals(2));
		}
	}, GraphSizes(4, NUMBER_OF_NODES, 1));
}

//! Performs single iteration of generic tests that modify the \c graph.
void testCombinatorialEmbedding(Graph &graph) {
	CombinatorialEmbedding emb(graph);
	int numberOfNodes;
	int numberOfEdges;
	int numberOfFaces;

	before_each([&] {
		emb.computeFaces();
		numberOfNodes = graph.numberOfNodes();
		numberOfEdges = graph.numberOfEdges();
		numberOfFaces = emb.numberOfFaces();
	});

	describe("updating", [&] {
		it("clears itself",[&] {
			emb.clear();

			AssertThat(graph.numberOfNodes(), Equals(0));
			AssertThat(graph.numberOfEdges(), Equals(0));
			AssertThat(emb.numberOfFaces(), Equals(0));
		});

		it("adds edges to isolated nodes", [&] {
			adjEntry adj = graph.chooseNode()->firstAdj();

			face f = emb.rightFace(adj);
			int size = f->size();

			edge e = emb.addEdgeToIsolatedNode(graph.newNode(), adj);

			AssertThat(emb.numberOfFaces(), Equals(numberOfFaces));
			AssertThat(emb.rightFace(e->adjSource()), Equals(f));
			AssertThat(emb.leftFace(e->adjSource()), Equals(f));
			AssertThat(f->size(), Equals(size + 2));
		});

		it("splits an edge", [&] {
			edge splitEdgeBeginning = graph.chooseEdge();
			face leftFace = emb.leftFace(splitEdgeBeginning->adjSource());
			int leftFaceSize = leftFace->size();
			face rightFace = emb.rightFace(splitEdgeBeginning->adjSource());
			int rightFaceSize = rightFace->size();

			edge splitEdgeEnd = emb.split(splitEdgeBeginning);

			AssertThat(graph.numberOfNodes(), Equals(numberOfNodes + 1));
			AssertThat(graph.numberOfEdges(), Equals(numberOfEdges + 1));
			AssertThat(emb.numberOfFaces(), Equals(numberOfFaces));
			AssertThat(emb.leftFace(splitEdgeBeginning->adjSource()), Equals(leftFace));
			AssertThat(emb.rightFace(splitEdgeBeginning->adjSource()), Equals(rightFace));
			AssertThat(emb.leftFace(splitEdgeEnd->adjSource()), Equals(leftFace));
			AssertThat(emb.rightFace(splitEdgeEnd->adjSource()), Equals(rightFace));

			if(leftFace == rightFace) {
				AssertThat(leftFace->size(), Equals(leftFaceSize + 2));
			} else {
				AssertThat(leftFace->size(), Equals(leftFaceSize + 1));
				AssertThat(rightFace->size(), Equals(rightFaceSize + 1));
			}
		});

		it("unsplits an edge", [&] {
			edge splitEdgeBeginning = graph.chooseEdge();
			face leftFace = emb.leftFace(splitEdgeBeginning->adjSource());
			int leftFaceSize = leftFace->size();
			face rightFace = emb.rightFace(splitEdgeBeginning->adjSource());
			int rightFaceSize = rightFace->size();

			edge splitEdgeEnd = emb.split(splitEdgeBeginning);
			emb.unsplit(splitEdgeBeginning, splitEdgeEnd);

			AssertThat(graph.numberOfNodes(), Equals(numberOfNodes));
			AssertThat(graph.numberOfEdges(), Equals(numberOfEdges));
			AssertThat(emb.numberOfFaces(), Equals(numberOfFaces));
			AssertThat(leftFace->size(), Equals(leftFaceSize));
			AssertThat(rightFace->size(), Equals(rightFaceSize));
		});

		auto pickNode = [&] {
			for(node v : graph.nodes) {
				if(v->degree() > 1) {
					return v;
				}
			}
			OGDF_ASSERT(false); // there should be a node with degree > 1
			return node(nullptr);
		};

		it("splits a node", [&] {
			node vl = pickNode();
			int degree = vl->degree();
			adjEntry adjStartLeft = vl->firstAdj();
			adjEntry adjStartRight =  vl->lastAdj();

			node vr = emb.splitNode(adjStartLeft, adjStartRight);

			AssertThat(graph.numberOfNodes(), Equals(numberOfNodes + 1));
			AssertThat(graph.numberOfEdges(), Equals(numberOfEdges + 1));
			AssertThat(vl->degree(), Equals(degree));
			AssertThat(vr->degree(), Equals(2));
			AssertThat(graph.searchEdge(vl, vr), !Equals(nullptr));
			AssertThat(vl->firstAdj()->theEdge(), Equals(vr->firstAdj()->theEdge()));
		});

		it("contracts a node", [&] {
			node vl = pickNode();
			int degree = vl->degree();
			adjEntry adjStartLeft = vl->firstAdj();
			adjEntry adjStartRight =  vl->lastAdj();
			node vr = emb.splitNode(adjStartLeft, adjStartRight);

			node contractedNode = emb.contract(graph.searchEdge(vl, vr));

			AssertThat(graph.numberOfNodes(), Equals(numberOfNodes));
			AssertThat(graph.numberOfEdges(), Equals(numberOfEdges));
			AssertThat(contractedNode->degree(), Equals(degree));
		});

		it("reverses an edge", [&] {
			edge e = graph.chooseEdge();
			node src = e->source();
			node tgt = e->target();
			adjEntry adjSrc = e->adjSource();
			face rightFace = emb.rightFace(adjSrc);
			face leftFace = emb.leftFace(adjSrc);

			emb.reverseEdge(e);

			AssertThat(e->source(), Equals(tgt));
			AssertThat(e->target(), Equals(src));
			adjSrc = e->adjSource();
			AssertThat(emb.rightFace(adjSrc), Equals(leftFace));
			AssertThat(emb.leftFace(adjSrc), Equals(rightFace));
		});

		it("removes a degree-1 node", [&] {
			node v = graph.newNode();
			node w = graph.chooseNode([&](node u) { return u != v; });

			graph.newEdge(w, v);
			emb.computeFaces();

			face f = emb.rightFace(v->firstAdj());
			int size = f->size();

			AssertThat(emb.leftFace(v->firstAdj()), Equals(f));

			emb.removeDeg1(v);

			AssertThat(emb.numberOfFaces(), Equals(numberOfFaces));
			AssertThat(f->size(), Equals(size - 2));
		});
	});

	describe("splitting faces", [&] {
		int sizeOfFace;
		face fSplitMe;
		adjEntry adjFirst;
		adjEntry adjSecond;

		before_each([&](){
			fSplitMe = emb.chooseFace([](face f) { return f->size() > 4; });
			adjFirst = fSplitMe->firstAdj();
			adjSecond = adjFirst->faceCycleSucc()->faceCycleSucc();
			sizeOfFace = fSplitMe->size();
		});

		it("works given two adjacency entries", [&] {
			edge e = emb.splitFace(adjFirst, adjSecond);

			AssertThat(e, !Equals(nullptr));
			AssertThat(e->source(), Equals(adjFirst->theNode()));
			AssertThat(e->target(), Equals(adjSecond->theNode()));

			face f = emb.leftFace(e->adjSource());
			face g = emb.rightFace(e->adjSource());

			AssertThat(f, !Equals(g));
			AssertThat(fSplitMe, Equals(f) || Equals(g));

			AssertThat(f->size(), Equals(3));
			AssertThat(g->size(), Equals(sizeOfFace - 1));

			AssertThat(emb.rightFace(adjFirst), Equals(f));
			AssertThat(emb.rightFace(adjSecond), Equals(g));
		});

		it("works given a deg-0 node and an adjacency entry as target", [&] {
			node v = graph.newNode();

			edge e = emb.addEdgeToIsolatedNode(v, adjFirst);

			AssertThat(e, !Equals(nullptr));
			AssertThat(e->source(), Equals(v));
			AssertThat(e->target(), Equals(adjFirst->theNode()));

			AssertThat(fSplitMe->size(), Equals(sizeOfFace + 2));
			AssertThat(emb.rightFace(e->adjSource()), Equals(fSplitMe));
			AssertThat(emb.leftFace(e->adjSource()), Equals(fSplitMe));
		});

		it("works given an adjacency entry as source and a deg-0 node", [&] {
			node v = graph.newNode();

			edge e = emb.addEdgeToIsolatedNode(adjFirst, v);

			AssertThat(e, !Equals(nullptr));
			AssertThat(e->source(), Equals(adjFirst->theNode()));
			AssertThat(e->target(), Equals(v));

			AssertThat(fSplitMe->size(), Equals(sizeOfFace + 2));
			AssertThat(emb.rightFace(e->adjSource()), Equals(fSplitMe));
			AssertThat(emb.leftFace(e->adjSource()), Equals(fSplitMe));
		});

#ifdef OGDF_USE_ASSERT_EXCEPTIONS
		it("rejects splitting given adjacency entries from different faces", [&] {
			adjEntry v = graph.chooseEdge()->adjSource();
			adjEntry w;

			do {
				w = graph.chooseEdge()->adjSource();
			} while(emb.rightFace(v) == emb.rightFace(w) ||
			        emb.rightFace(v) == emb.leftFace(w) ||
			        emb.leftFace(v) == emb.rightFace(w) ||
			        emb.leftFace(v) == emb.leftFace(w));

			AssertThrows(AssertionFailed, emb.splitFace(v, w));
		});
#endif

		it("removes the edge when joining two faces", [&] {
			int embNumberOfFaces = emb.numberOfFaces();
			edge nonBridgeEdge = graph.chooseEdge([&](edge e) { return !emb.isBridge(e); });

			face faceLeft = emb.leftFace(nonBridgeEdge->adjSource());
			face faceRight = emb.rightFace(nonBridgeEdge->adjSource());

			int sizeLeft = faceLeft->size();
			int sizeRight = faceRight->size();

			face jointFace = emb.joinFaces(nonBridgeEdge);

			AssertThat(jointFace->size(), Equals(sizeLeft + sizeRight - 2));
			AssertThat(emb.numberOfFaces(), Equals(embNumberOfFaces - 1));
		});
	});
}

go_bandit([] {
	describe("ConstCombinatorialEmbedding", [&] {
		testConstCombinatorialEmbedding<ConstCombinatorialEmbedding>();
	});

	describe("CombinatorialEmbedding", [&] {
		for(int i = 1; i <= NUMBER_OF_ITERATIONS; i++) {
			describe("iteration #" + to_string(i), [&] {
				Graph graph;

				before_each([&] {
					randomPlanarConnectedGraph(graph, NUMBER_OF_NODES, NUMBER_OF_EDGES);
				});

				testCombinatorialEmbedding(graph);
			});
		}

		testConstCombinatorialEmbedding<CombinatorialEmbedding>();
	});
});
