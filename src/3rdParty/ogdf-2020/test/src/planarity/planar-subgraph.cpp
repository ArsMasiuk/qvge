#include <random>

#include <ogdf/planarity/PlanarSubgraphBoyerMyrvold.h>
#include <ogdf/planarity/PlanarSubgraphFast.h>
#include <ogdf/planarity/MaximalPlanarSubgraphSimple.h>
#include <ogdf/planarity/MaximumPlanarSubgraph.h>
#include <ogdf/planarity/PlanarSubgraphCactus.h>
#include <ogdf/planarity/PlanarSubgraphTriangles.h>
#include <ogdf/planarity/PlanarSubgraphTree.h>

#include <graphs.h>

using std::minstd_rand;

template <typename TCost>
void testSubgraphInstance(Graph &graph, PlanarSubgraphModule<TCost> &psm, PlanarityModule &tester, bool assertMaximality, bool weighEdges, bool connects) {
	makeSimpleUndirected(graph);
	if (graph.numberOfEdges() == 0) {
		return;
	}
	makeConnected(graph);

	EdgeArray<TCost> costs;
	edge mustHaveEdge = nullptr;

	if(weighEdges) {
		costs.init(graph);

		for(edge e : graph.edges) {
			costs[e] = TCost(1);
		}

		mustHaveEdge = graph.chooseEdge();
		costs[mustHaveEdge] = TCost(1 + graph.numberOfEdges());
	}

	List<edge> removedEdges;
	if(weighEdges) {
		psm.call(graph, costs, removedEdges);
	} else {
		psm.call(graph, removedEdges);
	}

	std::cout << std::endl << "      removed " << removedEdges.size() << " edges" << std::endl;
	bool connected = isConnected(graph);

	Graph::HiddenEdgeSet set(graph);
	List<edge> hiddenEdges;
	for(edge e : removedEdges) {
		set.hide(e);
		hiddenEdges.pushBack(e);
		AssertThat(e, Is().Not().EqualTo(mustHaveEdge));
	}

	if(connects) {
		AssertThat(isConnected(graph), Equals(connected));
	}

	AssertThat(tester.isPlanar(graph), Equals(true));

	if(assertMaximality) {
		ListConstIterator<edge> it = removedEdges.begin();
		for(edge e : hiddenEdges) {
			set.restore(e);
			AssertThat(tester.isPlanar(graph), Equals(false));
			set.hide(e);
			it = it.succ();
		}
	}
}

void testSubgraphInstanceForIntAndDouble(Graph &graph, PlanarSubgraphModule<int> &psmi, PlanarSubgraphModule<double> &psmd) {
	makeSimpleUndirected(graph);
	makeConnected(graph);
	EdgeArray<int> costsInt(graph);
	EdgeArray<double> costsDouble(graph);
	int cnt = 0;
	for(edge e : graph.edges) {
		cnt++;
		costsInt(e) = cnt;
		costsDouble(e) = double(cnt);
	}
	List<edge> removedEdgesInt;
	List<edge> removedEdgesDouble;
	psmi.call(graph, costsInt, removedEdgesInt);
	psmd.call(graph, costsDouble, removedEdgesDouble);
	int sumOfRemovedInt = 0;
	double sumRemovedDouble = 0;
	for (edge e: removedEdgesInt) { sumOfRemovedInt += costsInt(e); }
	for (edge e: removedEdgesDouble) { sumRemovedDouble += costsDouble(e); }
	AssertThat( sumOfRemovedInt, Equals(int(sumRemovedDouble)) );
}

void performGenericTests(const string &name, bool optimal, bool respectsEdgeWeight, bool skip, std::function<void(Graph &, bool)> callFunc) {
	describe(name, [&]() {
		auto doTest = [&](bool weighted) {
			forEachGraphItWorks({GraphProperty::sparse}, [&](Graph &G) { callFunc(G, weighted); }, optimal ? GraphSizes(10) : GraphSizes());
		};

		doTest(false);

		if(respectsEdgeWeight) {
			describe("weighted", [&] { doTest(true); });
		}
	}, skip);
}

template <typename TCost>
void testSubgraphAlgorithm(const string &name, PlanarSubgraphModule<TCost> &psm, bool optimal, bool maximal, bool respectsEdgeWeight, bool connects, bool skip = false) {
	maximal |= optimal;
	BoothLueker bl;
	performGenericTests(name, optimal, respectsEdgeWeight, skip, [&](Graph &graph, bool weighEdges) {
		testSubgraphInstance<TCost>(graph, psm, bl, maximal, weighEdges, connects);
	});
}

void testSubgraphAlgorithmForIntAndDouble(const string &name, PlanarSubgraphModule<int> &psmi, PlanarSubgraphModule<double> &psmd) {
	performGenericTests(name + " int VS double", false, true, false, [&](Graph &graph, bool weighEdges) {
		testSubgraphInstanceForIntAndDouble(graph, psmi, psmd);
	});
}

template<template<typename> class Algorithm>
void describeAlgorithm(const string &name, bool optimal, bool maximal, bool respectsEdgeWeight, bool connects, bool skip = false) {
	Algorithm<int> algoInt;
	testSubgraphAlgorithm(name + "<int>", algoInt, optimal, maximal, respectsEdgeWeight, connects, skip);

	Algorithm<double> algoDouble;
	testSubgraphAlgorithm(name + "<double>", algoDouble, optimal, maximal, respectsEdgeWeight, connects, skip);
}

go_bandit([]() {
	describe("Planar Subgraphs", []() {
		PlanarSubgraphBoyerMyrvold bms;

		testSubgraphAlgorithm("PlanarSubgraphBoyerMyrvold",              bms, false, false, true,  true, true);
		describeAlgorithm<PlanarSubgraphFast>("PlanarSubgraphFast",           false, false, false, true);
		describeAlgorithm<PlanarSubgraphCactus>("PlanarSubgraphCactus",       false, false, false, true);
		describeAlgorithm<PlanarSubgraphTriangles>("PlanarSubgraphTriangles", false, false, false, true);
		describeAlgorithm<PlanarSubgraphTree>("PlanarSubgraphTree",           false, false, false, true);
		describeAlgorithm<MaximumPlanarSubgraph>("MaximumPlanarSubgraph",     true,  true,  true,  true);
		describeAlgorithm<PlanarSubgraphEmpty>("PlanarSubgraphEmpty",         false, false, false, false);

		MaximalPlanarSubgraphSimple<int> mpss;
		PlanarSubgraphCactus<int> psc;
		MaximalPlanarSubgraphSimple<int> mpssPsc(psc);
		PlanarSubgraphTriangles<int> pst;
		MaximalPlanarSubgraphSimple<int> mpssPst(pst);
		PlanarSubgraphFast<int> fps;
		MaximalPlanarSubgraphSimple<int> mpssFps(fps);
		MaximalPlanarSubgraphSimple<int> mpssBms(bms);

		testSubgraphAlgorithm("MaximalPlanarSubgraphSimple",        mpss,    false, true, false, true);
		testSubgraphAlgorithm("Maximal PlanarSubgraphCactus",       mpssPsc, false, true, false, true);
		testSubgraphAlgorithm("Maximal PlanarSubgraphTriangles",    mpssPst, false, true, false, true);
		testSubgraphAlgorithm("Maximal PlanarSubgraphFast",         mpssFps, false, true, false, true);
		testSubgraphAlgorithm("Maximal PlanarSubgraphBoyerMyrvold", mpssBms, false, true, false, true, true);

		PlanarSubgraphCactus<double> pscd;
		MaximalPlanarSubgraphSimple<double> mpssPscd(pscd);
		testSubgraphAlgorithmForIntAndDouble("Maximal PlanarSubgraphCactus", mpssPsc, mpssPscd);
		PlanarSubgraphTriangles<double> psdt;
		MaximalPlanarSubgraphSimple<double> mpssPsdt(psdt);
		testSubgraphAlgorithmForIntAndDouble("Maximal PlanarSubgraphTriangles", mpssPst, mpssPsdt);
	});
});
