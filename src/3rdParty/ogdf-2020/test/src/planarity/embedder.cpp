#include <ogdf/basic/graph_generators.h>
#include <ogdf/planarity/EmbedderMaxFace.h>
#include <ogdf/planarity/EmbedderMaxFaceLayers.h>
#include <ogdf/planarity/EmbedderMinDepth.h>
#include <ogdf/planarity/EmbedderMinDepthMaxFace.h>
#include <ogdf/planarity/EmbedderMinDepthMaxFaceLayers.h>
#include <ogdf/planarity/EmbedderMinDepthPiTa.h>
#include <ogdf/planarity/EmbedderOptimalFlexDraw.h>
#include <ogdf/planarity/SimpleEmbedder.h>

#include <graphs.h>

#define TEST_EMBEDDER(NAME) describeEmbedder<NAME>(#NAME)

void validateCopy(const Graph &graph, const GraphCopy &copy) {
	AssertThat(graph.numberOfNodes(), Equals(copy.numberOfNodes()));
	AssertThat(graph.numberOfEdges(), Equals(copy.numberOfEdges()));

	for(node v : copy.nodes) {
		AssertThat(copy.isDummy(v), IsFalse());
	}

	for(edge e : copy.edges) {
		AssertThat(copy.isDummy(e), IsFalse());
		edge f = copy.original(e);
		AssertThat(f->source(), Equals(copy.original(e->source())));
		AssertThat(f->target(), Equals(copy.original(e->target())));
	}
}

void shuffleEmbedding(Graph &graph) {
	for(node v : graph.nodes) {
		for(adjEntry adj : v->adjEntries) {
			graph.swapAdjEdges(adj, randomNumber(0, 1) ? v->firstAdj() : v->lastAdj());
		}
	}
}

void testEmbedder(EmbedderModule &embedder, const Graph &graph, bool repeat = true) {
	GraphCopy copy(graph);
	if(repeat) {
		shuffleEmbedding(copy);
	}

	// initialize adjExternal with a corrupt value
	adjEntry adjExternal = graph.numberOfNodes() == 0 ? nullptr : graph.firstNode()->firstAdj();

	embedder(copy, adjExternal);

	validateCopy(graph, copy);
	if (graph.numberOfEdges() == 0) {
		AssertThat(adjExternal, IsNull());
	} else {
		AssertThat(adjExternal, !IsNull());
#ifdef OGDF_DEBUG
		AssertThat(adjExternal->graphOf(), Equals(&copy));
#endif
	}

	AssertThat(copy.representsCombEmbedding(), IsTrue());

	// test planarly embedded input
	if(repeat) {
		testEmbedder(embedder, copy, false);
	}
}

void describeEmbedder(const string &title, EmbedderModule &embedder, std::set<GraphProperty> requirements = {}, bool doSkip = false) {
	describe(title, [&] {
		requirements.insert(GraphProperty::connected);
		requirements.insert(GraphProperty::planar);
		requirements.insert(GraphProperty::simple);

		forEachGraphItWorks(requirements, [&](const Graph& G) { testEmbedder(embedder, G); });

#ifdef OGDF_USE_ASSERT_EXCEPTIONS
		it("fails on a K5", [&] {
			adjEntry adjExternal;
			Graph G;
			completeGraph(G, 5);
			AssertThrows(AssertionFailed, embedder(G, adjExternal));
		});
#endif
	}, doSkip);
}

template<typename EmbedderType>
void describeEmbedder(const string &title) {
	EmbedderType embedder;
	describeEmbedder(title, embedder);
}

template<>
void describeEmbedder<EmbedderMinDepthPiTa>(const string &title) {
	EmbedderMinDepthPiTa embedder;
	bool extendedDD = embedder.useExtendedDepthDefinition();

	// TODO Why does this embedder require biconnectivity?
	//      A BC-tree is used internally...
	std::initializer_list<GraphProperty> reqs = {GraphProperty::biconnected};
	describeEmbedder(title + " [extendedDD=" + to_string(extendedDD) + "]", embedder, reqs);
	embedder.useExtendedDepthDefinition(!extendedDD);
	describeEmbedder(title + " [extendedDD=" + to_string(!extendedDD) + "]", embedder, reqs);
}

// TODO currently skipped since these tests are failing.
template<>
void describeEmbedder<EmbedderOptimalFlexDraw>(const string &title) {
	describe(title, [] {
		EmbedderOptimalFlexDraw embedder;
		describeEmbedder("Non-Weighted Version", embedder, {}, true);

		describe_skip("Weighted Edges", [&] {
			it("works on a random graph", [&] {
				Graph graph;
				constexpr int n = 42;
				randomPlanarConnectedGraph(graph, n, 2*n);
				EdgeArray<int> costs(graph);

				for(edge e : graph.edges) {
					costs[e] = randomNumber(1, n);
				}

				testEmbedder(embedder, graph);
			});
		});
	});
}

go_bandit([]() {
	describe("Embedders", []() {
		TEST_EMBEDDER(EmbedderMaxFace);
		TEST_EMBEDDER(EmbedderMaxFaceLayers);
		TEST_EMBEDDER(EmbedderMinDepth);
		TEST_EMBEDDER(EmbedderMinDepthMaxFace);
		TEST_EMBEDDER(EmbedderMinDepthMaxFaceLayers);
		TEST_EMBEDDER(EmbedderMinDepthPiTa);
		TEST_EMBEDDER(EmbedderOptimalFlexDraw);
		TEST_EMBEDDER(SimpleEmbedder);
	});
});
