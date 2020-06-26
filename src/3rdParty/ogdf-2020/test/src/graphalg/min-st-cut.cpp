#include <ogdf/graphalg/MaxFlowEdmondsKarp.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/graphalg/MinSTCutMaxFlow.h>
#include <ogdf/graphalg/MinSTCutDijkstra.h>
#include <ogdf/graphalg/MinSTCutBFS.h>

#include <graphs.h>

using std::string;

template<typename T>
void describeMSTCutFromMaxFlowSuite(const string &name) {
describe("MinSTCutMaxFlow<" + name + ">", [] {
	it("can handle an isolated node", [](){
		Graph graph;
		node v = graph.newNode();
		EdgeArray<T> weights(graph, 4);

		EdgeArray<T> flow;
		MaxFlowEdmondsKarp<T> maxFlowEdmondsKarp(graph);
		maxFlowEdmondsKarp.computeFlow(weights, v, v, flow);
		MinSTCutMaxFlow<T> minSTCut;
		minSTCut.call(graph, weights, flow, v, v);
	});

	it("works on a simple example", [](){
		Graph graph;
		node s = graph.newNode();
		node t = graph.newNode();
		node v1 = graph.newNode();
		node v2 = graph.newNode();

		graph.newEdge(s, v1);
		graph.newEdge(v2, t);

		EdgeArray<T> weights(graph, 4);
		weights[graph.newEdge(s, v2)] = 1;
		weights[graph.newEdge(v1, t)] = 2;

		EdgeArray<T> flow;
		MaxFlowEdmondsKarp<T> mfek(graph);
		mfek.computeFlow(weights, s, t, flow);
		MinSTCutMaxFlow<T> minSTCut;
		minSTCut.call(graph, weights, flow, s, t);

		AssertThat(minSTCut.isInFrontCut(s), Equals(true));
		AssertThat(minSTCut.isInFrontCut(v1), Equals(true));
		AssertThat(minSTCut.isInBackCut(t), Equals(true));
		AssertThat(minSTCut.isInBackCut(v2), Equals(true));
	});

	it("works on a more complex example", [](){
		Graph graph;
		emptyGraph(graph, 8);
		List<node> nodes;
		graph.allNodes(nodes);
		EdgeArray<T> weights(graph);
		weights[graph.newEdge(*nodes.get(0), *nodes.get(1))] = 16;
		weights[graph.newEdge(*nodes.get(0), *nodes.get(2))] = 13;
		weights[graph.newEdge(*nodes.get(1), *nodes.get(2))] = 10;
		weights[graph.newEdge(*nodes.get(1), *nodes.get(3))] = 12;
		weights[graph.newEdge(*nodes.get(2), *nodes.get(1))] =  4;
		weights[graph.newEdge(*nodes.get(2), *nodes.get(4))] = 14;
		weights[graph.newEdge(*nodes.get(3), *nodes.get(2))] =  9;
		weights[graph.newEdge(*nodes.get(3), *nodes.get(5))] = 20;
		weights[graph.newEdge(*nodes.get(4), *nodes.get(3))] =  7;
		weights[graph.newEdge(*nodes.get(4), *nodes.get(5))] =  4;

		weights[graph.newEdge(*nodes.get(5), *nodes.get(6))] =  100;
		weights[graph.newEdge(*nodes.get(7), *nodes.get(5))] =  100;

		EdgeArray<T> flow;
		MaxFlowEdmondsKarp<T> mfek(graph);
		mfek.computeFlow(weights, *nodes.get(0), *nodes.get(5), flow);
		MinSTCutMaxFlow<T> minSTCut;
		minSTCut.call(graph, weights, flow, *nodes.get(0), *nodes.get(5));

		AssertThat(minSTCut.isInFrontCut(*nodes.get(0)), Equals(true));
		AssertThat(minSTCut.isInFrontCut(*nodes.get(1)), Equals(true));
		AssertThat(minSTCut.isInFrontCut(*nodes.get(2)), Equals(true));
		AssertThat(minSTCut.isInFrontCut(*nodes.get(4)), Equals(true));

		AssertThat(minSTCut.isInBackCut(*nodes.get(3)), Equals(true));
		AssertThat(minSTCut.isInBackCut(*nodes.get(5)), Equals(true));

		AssertThat(minSTCut.isInFrontCut(*nodes.get(6)), Equals(false));
		AssertThat(minSTCut.isInBackCut(*nodes.get(6)), Equals(false));
		AssertThat(minSTCut.isOfType(*nodes.get(6), MinSTCutMaxFlow<T>::cutType::NO_CUT), Equals(true));
		AssertThat(minSTCut.isInBackCut(*nodes.get(7)), Equals(true));
		AssertThat(minSTCut.isInFrontCut(*nodes.get(7)), Equals(false));
	});

	describe("detection of complementary back cuts", [] {
		MinSTCutMaxFlow<T> minSTCut;
		forEachGraphItWorks({GraphProperty::connected}, [&] (const Graph& graph, const std::string& graphName, const std::set<GraphProperty>& props) {
			EdgeArray<T> caps(graph);

			for(edge e : graph.edges) {
				caps[e] = randomNumber(1, 10);
			}

			for(node v : graph.nodes) if(v != graph.firstNode()) {
				List<edge> cutEdges;
				minSTCut.call(graph, caps, graph.firstNode(), v, cutEdges, nullptr);

				bool isComplement = true;

				for(node w : graph.nodes) {
					isComplement &= minSTCut.isInFrontCut(w) != minSTCut.isInBackCut(w);
				}

				AssertThat(minSTCut.frontCutIsComplementOfBackCut(), Equals(isComplement));
			}
		});
	});
});
}

template<typename T>
void describeMSTCutSuite(MinSTCutModule<T> &minst, const string &name, const string &type, bool canHandleNonPlanar) {
describe("MinSTCut"+ name + "<" + type + ">", [&] {
	it("works on a planar unweighted example", [&]() {
		Graph graph;
		node s = graph.newNode();
		node t = graph.newNode();
		node v1 = graph.newNode();
		node v2 = graph.newNode();
		node v3 = graph.newNode();
		node v4 = graph.newNode();
		node v5 = graph.newNode();

		edge e1 = graph.newEdge(s, v1);
		edge e2 = graph.newEdge(s, v2);
		edge e_st = graph.newEdge(s, t);
		graph.newEdge(v2, v4);
		graph.newEdge(v2, v5);
		graph.newEdge(v1, v3);
		graph.newEdge(v1, v4);
		graph.newEdge(v5, t);
		graph.newEdge(v4, t);
		graph.newEdge(v3, t);

		List<edge> edgeList;
		minst.call(graph, s, t, edgeList, e_st);

		AssertThat(edgeList.size(), Equals(2));
		AssertThat(edgeList.popFrontRet(), Equals(e2));
		AssertThat(edgeList.popFrontRet(), Equals(e1));
	});

	it("works on a planar weighted example", [&]() {
		Graph graph;
		node s = graph.newNode();
		node t = graph.newNode();
		node v1 = graph.newNode();
		node v2 = graph.newNode();
		node v3 = graph.newNode();

		graph.newEdge(s, v1);
		graph.newEdge(s, v2);
		graph.newEdge(s, v3);
		edge e_st = graph.newEdge(s, t);

		EdgeArray<T> weights(graph, 4);
		edge e1, e2, e3;
		weights[e1 = graph.newEdge(v3, t)] = 2;
		weights[e2 = graph.newEdge(v2, t)] = 2;
		weights[e3 = graph.newEdge(v1, t)] = 2;

		List<edge> edgeList;
		minst.call(graph, weights, s, t, edgeList, e_st);

		AssertThat(edgeList.size(), Equals(3));
		AssertThat(edgeList.popFrontRet(), Equals(e1));
		AssertThat(edgeList.popFrontRet(), Equals(e2));
		AssertThat(edgeList.popFrontRet(), Equals(e3));
	});
	if(canHandleNonPlanar) {
		it("works on a non-planar weighted example", [&]() {
			Graph graph;
			completeGraph(graph, 5);

			EdgeArray<T> weight(graph, 5);

			node s = graph.chooseNode();
			node t = graph.newNode();
			edge e = graph.newEdge(s,t);
			weight[e] = 1;
			List<edge> edgeList;
			minst.call(graph, weight, s, t, edgeList);

			AssertThat(((MinSTCutMaxFlow<T>&) minst).isInFrontCut(s), IsTrue());
			AssertThat(((MinSTCutMaxFlow<T>&) minst).isInBackCut(t), IsTrue());

			AssertThat(edgeList.size(), Equals(1));
			AssertThat(edgeList.front(), Equals(e));
		});
		it("works on a non-planar unweighted example", [&]() {
			Graph graph;
			completeGraph(graph, 5);

			List<node> nodes;
			graph.allNodes(nodes);
			node s = *nodes.get(0);
			node t = *nodes.get(1);
			List<edge> edgeList;
			minst.call(graph, s, t, edgeList);

			AssertThat(edgeList.size(), Equals(4));
		});
	}

});
}

go_bandit([](){
	describe("MinSTCut from a flow", []() {
		describeMSTCutFromMaxFlowSuite<int>("int");
		describeMSTCutFromMaxFlowSuite<double>("double");
	});
	describe("MinSTCut from a graph", []() {
		MinSTCutMaxFlow<int> mFint(true, new MaxFlowGoldbergTarjan<int>());
		describeMSTCutSuite<int>(mFint, "MaxFlow(GoldbergTarjan)", "int", true);
		MinSTCutMaxFlow<double> mFdouble;
		describeMSTCutSuite<double>(mFdouble, "MaxFlow(EdmondsKarp)", "double", true);
		MinSTCutDijkstra<int> mDint;
		describeMSTCutSuite<int>(mDint, "Dijkstra", "int", false);
		MinSTCutDijkstra<double> mDdouble;
		describeMSTCutSuite<double>(mDdouble, "Dijkstra", "double", false);
		MinSTCutBFS<int> mBint;
		describeMSTCutSuite<int>(mBint, "BFS", "int", false);
		MinSTCutBFS<double> mBdouble;
		describeMSTCutSuite<double>(mBdouble, "BFS", "double", false);
	});
});
