/** \file
 * \brief Regression test for planarity tests and embeddings
 *
 * \author Carsten Gutwenger, Tilo Wiedera, Mirko Wagner
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

#include <random>

#include <ogdf/planarity/BoothLueker.h>
#include <ogdf/planarity/BoyerMyrvold.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/planarity/NonPlanarCore.h>
#include <ogdf/planarity/PlanarizationLayout.h>
#include <ogdf/planarity/SubgraphPlanarizer.h>
#include <ogdf/graphalg/MaxFlowSTPlanarItaiShiloach.h>
#include <ogdf/graphalg/MinSTCutMaxFlow.h>

#include <graphs.h>

using std::minstd_rand;
using std::uniform_int_distribution;
using ReturnType = CrossingMinimizationModule::ReturnType;

template<typename T>
void testNPCWeighted(string description, string alg, bool useDijkstra) {
	it("recognizes weight in " + description + " with " + alg, [&]() {
		Graph graph;
		completeGraph(graph, 5);
		EdgeArray<T> weight(graph, T(1));
		edge e = graph.chooseEdge();
		edge f = graph.newEdge(e->target(), e->source());
		weight[graph.split(e)] = T(32.32);
		weight[graph.split(f)] = T(64.64);
		weight[graph.newEdge(e->target(), f->target())] = T(4.04);
		weight[e] = T(8.08);
		weight[f] = T(16.16);
		NonPlanarCore<T> *npc;
		if(useDijkstra) {
			npc = new NonPlanarCore<T>(graph, weight);
		} else {
			MinSTCutMaxFlow<T> minSTCutMaxFlow(true, new MaxFlowSTPlanarItaiShiloach<T>());
			npc = new NonPlanarCore<T>(graph, weight, &minSTCutMaxFlow);
		}
		const Graph &core = npc->core();
		for (edge eCore : core.edges) {
			if (npc->isVirtual(eCore)) {
				AssertThat(npc->cost(eCore), Equals(T(28.28)));
			}
		}
		delete npc;
	});
}

void randomizeAdjLists(Graph G, minstd_rand &rng){
	for(node v : G.nodes){
		List<adjEntry> L;
		v->allAdjEntries(L);
		L.permute(rng);
		G.sort(v, L);
	}
}

void describeModule(const std::string &name, PlanarityModule &pm){
describe(name, [&](){
	minstd_rand rng(42);
	srand(4711);

	forEachGraphItWorks({GraphProperty::planar}, [&](Graph &G) {
		int n = G.numberOfNodes();
		int m = G.numberOfEdges();
		randomizeAdjLists(G, rng);
		AssertThat(pm.isPlanar(G), IsTrue());
		AssertThat(pm.planarEmbed(G), IsTrue());
		AssertThat(G.representsCombEmbedding(), IsTrue());

		// Destructive embeddings of a planar graph should not alter it.
		AssertThat(pm.isPlanarDestructive(G), IsTrue());
		AssertThat(G.numberOfNodes(), Equals(n));
		AssertThat(G.numberOfEdges(), Equals(m));
	});

	forEachGraphItWorks({GraphProperty::nonPlanar}, [&](Graph &G) {
		randomizeAdjLists(G, rng);
		AssertThat(pm.isPlanar(G), IsFalse());
		AssertThat(pm.planarEmbed(G), IsFalse());
		AssertThat(G.representsCombEmbedding(), IsFalse());
		AssertThat(pm.isPlanarDestructive(G), IsFalse());
	});
});
}

void describeDestructiveBoyerMyrvold(bool bundles, bool limitStructures,
		bool randomDFSTree, bool avoidE2Minors) {
	// bundles on big non-planar graphs takes too long.
	const int maxNNonPlanar = bundles ? 6 : std::numeric_limits<int>::max();
	const int maxKuratowskis = 5;

	BoyerMyrvold bm;
	minstd_rand rng(42);
	srand(4711);

	describe("bundles=" + to_string(bundles) +
		", limitStructures=" + to_string(limitStructures) +
		", randomDFSTree=" + to_string(randomDFSTree) +
		", avoidE2Minors=" + to_string(avoidE2Minors), [&](){

		forEachGraphItWorks({GraphProperty::planar}, [&](Graph &G) {
			SList<KuratowskiWrapper> kuratowskis;
			int n = G.numberOfNodes();
			int m = G.numberOfEdges();
			randomizeAdjLists(G, rng);
			bool result = bm.planarEmbedDestructive(G, kuratowskis, maxKuratowskis,
				bundles, limitStructures, randomDFSTree, avoidE2Minors);

			// Destructive embeddings of a planar graph should not alter it.
			AssertThat(result, IsTrue());
			AssertThat(kuratowskis.empty(), IsTrue());
			AssertThat(G.representsCombEmbedding(), IsTrue());
			AssertThat(G.numberOfNodes(), Equals(n));
			AssertThat(G.numberOfEdges(), Equals(m));
		});

		forEachGraphItWorks({GraphProperty::nonPlanar}, [&](Graph &G) {
			SList<KuratowskiWrapper> kuratowskis;
			randomizeAdjLists(G, rng);
			bool result = bm.planarEmbedDestructive(G, kuratowskis, maxKuratowskis,
				bundles, limitStructures, randomDFSTree, avoidE2Minors);
			AssertThat(result, IsFalse());
			AssertThat(kuratowskis.empty(), IsFalse());
		}, GraphSizes(), 0, maxNNonPlanar);
	});
}

void describeDestructiveBoyerMyrvold() {
describe("Destructive Boyer-Myrvold Embedding", [](){
	for (bool bundles : {false, true}) {
		for (bool limitStructures : {false, true}) {
			for (bool randomDFSTree : {false, true}) {
				for (bool avoidE2Minors : {false, true}) {
					describeDestructiveBoyerMyrvold(
						bundles, limitStructures, randomDFSTree, avoidE2Minors);
				}
			}
		}
	}
});
}

void testNonPlanarCore()
{
	for_each_graph_it("returns a simple core", {"north/g.41.26.gml", "north/g.73.8.gml"},
	                  [&](Graph &graph){
		makeBiconnected(graph);
		NonPlanarCore<int> npc(graph);
		const Graph &core = npc.core();
		AssertThat(isSimpleUndirected(core), IsTrue());

		for(edge e : core.edges){
			AssertThat(npc.cost(e), IsGreaterThan(0));
			if(!npc.isVirtual(e)){
				AssertThat(npc.realEdge(e), !IsNull());
			}
		}
	});

	it("works on a minimal previously failing instance (2 x K5)", [](){
		Graph graph;
		EdgeArray<int> weight(graph);

		node s = graph.newNode();
		node t = graph.newNode();
		graph.newEdge(t, s);

		node v = graph.newNode();
		graph.newEdge(s, v);
		graph.newEdge(v, t);

		for(int k = 0; k < 2; k++){
			List<node> nodes;
			nodes.pushBack(s);
			nodes.pushBack(t);

			for(int i = 0; i < 3; i++){
				nodes.pushBack(graph.newNode());
			}

			for(node x : nodes){
				for(node w : nodes){
					if(x->index() < w->index() && (x != s || w != t)){
						graph.newEdge(x, w);
					}
				}
			}
		}

		NonPlanarCore<int> npc(graph);
		const Graph &core = npc.core();

		for(edge e : core.edges) {
			if(npc.isVirtual(e)) {
				for(auto eCut : npc.mincut(e)) {
					if(eCut.e->source() == npc.original(e->source()) || eCut.e->target() == npc.original(e->target())) {
						AssertThat(eCut.dir, IsTrue());
					} else {
						AssertThat(eCut.dir, IsFalse());
					}
				}
			}
		}
		AssertThat(isLoopFree(core), IsTrue());
		AssertThat(isSimpleUndirected(core), IsTrue());
		AssertThat(core.numberOfNodes(), Equals(graph.numberOfNodes() - 1));
		AssertThat(core.numberOfEdges(), Equals(graph.numberOfEdges() - 2));
	});

	testNPCWeighted<int>("int", "Dijkstra", true);
	testNPCWeighted<int>("int", "ItaiShiloach", false);
	testNPCWeighted<unsigned int>("unsigned int", "Dijkstra", true);
	testNPCWeighted<double>("double", "Dijkstra", true);
	testNPCWeighted<double>("double", "ItaiShiloach", false);

	for_each_graph_it("retransforms while preserving the genus", {"north/g.41.26.gml", "north/g.73.8.gml"}, [&](Graph &graph) {
		makeBiconnected(graph);
		List<edge> edges;
		graph.allEdges(edges);
		for(edge e : edges) {
			edge f = graph.newEdge(e->source(), e->target());
			edge g = graph.split(e);
			edge h = graph.split(f);
			graph.newEdge(g->source(), h->source());
		}
		NonPlanarCore<int> C(graph);
		const Graph &core = C.core();
		AssertThat(isPlanar(core), IsFalse());
		AssertThat(core.numberOfNodes(), !Equals(0));
		SubgraphPlanarizer SP;
		PlanRep planarCore(core);
		planarCore.initCC(0);

		GraphCopy endGraph(graph);

		C.retransform(planarCore, endGraph, false);

		AssertThat(planarCore.genus(), Equals(endGraph.genus()));
	});

	for_each_graph_it("retransforms", {"north/g.41.26.gml", "north/g.73.8.gml"}, [&](Graph &graph) {
		makeBiconnected(graph);
		NonPlanarCore<int> C(graph);
		const Graph &core = C.core();
		AssertThat(isPlanar(core), IsFalse());
		AssertThat(core.numberOfNodes(), !Equals(0));
		SubgraphPlanarizer SP;
		PlanRep planarCore(core);

		GraphCopy endGraph(graph);
		int crossingNumber = 0;
		ReturnType ret = SP.call(planarCore, 0, crossingNumber, &C.cost());
		AssertThat(ret == ReturnType::TimeoutFeasible
		        || ret == ReturnType::Feasible
		        || ret == ReturnType::Optimal, IsTrue());
		AssertThat(planarEmbed(planarCore), IsTrue());
		planarCore.removePseudoCrossings();

		C.retransform(planarCore, endGraph);

		AssertThat(isPlanar(endGraph), IsTrue());
		AssertThat(endGraph.genus(), Equals(0));

		// now the embedding of the endGraph is tested to assert that the embedding of planarCore was
		// used to embed the endGraph
		for(node v : planarCore.nodes){
			if(planarCore.isDummy(v)){
				continue;
			}
			node endNode = endGraph.copy(C.original(planarCore.original(v)));
			List<adjEntry> adjEntries;
			endNode->allAdjEntries(adjEntries);
			int stComponentCounter(0);
			Array<int> componentList;
			componentList.grow(adjEntries.size(), -1);
			for(adjEntry pcAdj : v->adjEntries){
				edge coreEdge = planarCore.original(pcAdj->theEdge());
				node stNode = (pcAdj == pcAdj->theEdge()->adjSource() ? C.sNode(coreEdge) : C.tNode(coreEdge));
				EdgeArray<edge> &mapE = *C.mapE(coreEdge);
				for(adjEntry stAdj : stNode->adjEntries){
					List<edge> chain = endGraph.chain(mapE[stAdj->theEdge()]);
					adjEntry endAdj = nullptr;
					for(edge e : chain){
						if(e->source() == endNode){
							endAdj = e->adjSource();
						}
						if(e->target() == endNode){
							endAdj = e->adjTarget();
						}

					}
					auto searchIt = adjEntries.search(endAdj);
					AssertThat(searchIt.valid(), IsTrue());
					int position = adjEntries.pos(searchIt);
					componentList[position] = stComponentCounter;
				}
				stComponentCounter++;
			}
			int before(*componentList.rbegin());
			for(int i : componentList){
				if(i != before){
					AssertThat((before + 1) % stComponentCounter, Equals(i));
					before = i;
				}
			}
		}
	});

	it("contracts chains", [&](){
		Graph graph;
		GraphAttributes GA(graph);
		GA.addAttributes(GraphAttributes::nodeType | GraphAttributes::edgeType | GraphAttributes::nodeLabel |
				GraphAttributes::nodeStyle | GraphAttributes::edgeLabel | GraphAttributes::edgeStyle | GraphAttributes::edgeArrow);
		for(int i = 0; i < 13; i++){
			node curr = graph.newNode();
			GA.label(curr) = to_string(curr->index());
			GA.fillColor(curr) = Color::Name::Turquoise;
		}

		List<node> v;
		graph.allNodes(v);

		graph.newEdge(*v.get(0), *v.get(1));
		graph.newEdge(*v.get(1), *v.get(2));
		graph.newEdge(*v.get(2), *v.get(4));
		graph.newEdge(*v.get(1), *v.get(3));
		graph.newEdge(*v.get(4), *v.get(3));
		graph.newEdge(*v.get(3), *v.get(5));
		graph.newEdge(*v.get(5), *v.get(6));
		graph.newEdge(*v.get(5), *v.get(2));
		graph.newEdge(*v.get(4), *v.get(6));
		edge e67 = graph.newEdge(*v.get(6), *v.get(7));
		edge e78 = graph.newEdge(*v.get(7), *v.get(8));
		graph.newEdge(*v.get(0), *v.get(11));
		graph.newEdge(*v.get(0), *v.get(10));
		graph.newEdge(*v.get(11), *v.get(12));
		graph.newEdge(*v.get(10), *v.get(12));
		graph.newEdge(*v.get(10), *v.get(9));
		graph.newEdge(*v.get(9), *v.get(8));
		graph.newEdge(*v.get(5), *v.get(4));
		graph.newEdge(*v.get(12), *v.get(8));
		graph.newEdge(*v.get(11), *v.get(9));

		EdgeArray<int> weight(graph, 1);
		weight[e67] = 2;
		weight[e78] = 3;
		NonPlanarCore<int> C(graph, weight);
		const Graph &core = C.core();
		node v6(core.chooseNode()), v8(core.chooseNode());
		for(node w : core.nodes){
			if(C.original(w) == *v.get(6)){
				v6 = w;
			}
			if(C.original(w) == *v.get(8)){
				v8 = w;
			}
		}
		edge virt = nullptr;
		for(edge e : core.edges){
			if ((e->source() == v6 && e->target() == v8)
			 || (e->source() == v8 && e->target() == v6)) {
				virt = e;
			}
		}
		AssertThat(virt, !Equals((void *) nullptr));
		AssertThat(C.isVirtual(virt), IsTrue());
		AssertThat(C.cost(virt), Equals(2));
	});

	it("eliminates multiedges", [](){
		Graph graph;
		completeGraph(graph, 5);
		edge e = graph.chooseEdge();
		graph.newEdge(e->source(), e->target());
		e = graph.chooseEdge();
		graph.newEdge(e->target(), e->source());
		NonPlanarCore<int> npc(graph);
		const Graph &core = npc.core();
		AssertThat(isSimpleUndirected(core), IsTrue());
		AssertThat(core.numberOfNodes(), Equals(graph.numberOfNodes()));
		AssertThat(core.numberOfEdges(), Equals(10));
	});

	it("returns a list of original edges of a core edge", [](){
		Graph graph;
		completeGraph(graph, 5);
		edge e = graph.chooseEdge();
		edge f = graph.split(e);
		NonPlanarCore<int> npc(graph);
		for(edge eCore : npc.core().edges) {
			List<edge> list = npc.original(eCore);
			if(npc.isVirtual(eCore)){
				AssertThat(list.size(), Equals(2));
				if(list.front() == e) {
					AssertThat(list.back(), Equals(f));
				} else {
					AssertThat(list.front(), Equals(f));
					AssertThat(list.back(), Equals(e));
				}
			} else {
				AssertThat(list.size(), Equals(1));
				AssertThat(list.front(), Equals(npc.realEdge(eCore)));
			}
		}
	});
}

go_bandit([](){
	describe("Planarity tests", [](){
		BoothLueker bl;
		describeModule("Booth-Lueker", bl);
		BoyerMyrvold bm;
		describeModule("Boyer-Myrvold", bm);
		describeDestructiveBoyerMyrvold();

		it("transforms based on the right graph, when it's a GraphCopySimple", [](){
			Graph G;
			randomRegularGraph(G, 10, 6);
			GraphCopySimple gcs(G);
			BoyerMyrvold boyerMyrvold;
			SList<KuratowskiWrapper> kur_subs;
			SList<KuratowskiSubdivision> lksGcs;
			SList<KuratowskiSubdivision> lksG;

			boyerMyrvold.planarEmbed(gcs, kur_subs, BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited);
			boyerMyrvold.transform(kur_subs,lksGcs,gcs);
			boyerMyrvold.transform(kur_subs,lksG,G);
		});
	});

	describe("NonPlanarCore", [](){
		testNonPlanarCore();
	});
});
