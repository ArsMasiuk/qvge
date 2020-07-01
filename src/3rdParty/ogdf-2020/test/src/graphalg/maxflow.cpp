/** \file
 * \brief Tests for Max Flow Algorithms
 *
 * \author Ivo Hedtke, Tilo Wiedera
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

#include <ogdf/graphalg/MaxFlowEdmondsKarp.h>
#include <ogdf/graphalg/MaxFlowGoldbergTarjan.h>
#include <ogdf/graphalg/MaxFlowSTPlanarDigraph.h>
#include <ogdf/graphalg/MaxFlowSTPlanarItaiShiloach.h>
#include <ogdf/graphalg/ConnectivityTester.h>
#include <ogdf/basic/graph_generators.h>

#include <resources.h>

// if defined will print the first failed instance
#define PRINT_FIRST_FAIL

using std::string;
using std::endl;

#ifdef PRINT_FIRST_FAIL
bool printedFailedInstance = false;

/**
 * Defines which properties a graph fullfills
 * or an algorithm requires.
 */
enum MaxFlowRequirement {
	MFR_NONE = 0,
	MFR_PLANAR = 1,
	MFR_ST_PLANAR = 2,
	MFR_CONNECTED = 4,
	MFR_ST_NON_INCIDENT_FACE = 8,
};

/**
 * Used to combine requirements or properties.
 */
MaxFlowRequirement operator|(MaxFlowRequirement a, MaxFlowRequirement b)
{
	return MaxFlowRequirement(int(a) | int(b));
}

/**
 * Establishes the properties of the given graph.
 *
 * @param graph The graph to be investigated
 * @param s source node of the instance
 * @param t target node of the instance
 * @return all properties except for \c MFR_ST_NON_INCIDENT_FACE
 */
MaxFlowRequirement determineProperties(const Graph &graph, node s, node t)
{
	MaxFlowRequirement result = MFR_NONE;

	if(isPlanar(graph)) {
		result = result | MFR_PLANAR;

		if(isSTPlanar(graph, s, t)) {
			result = result | MFR_ST_PLANAR;
		}
	}

	if(isConnected(graph)) {
		result = result | MFR_CONNECTED;
	}

	return result;
}

/**
 * Used to print the first encountered failed instance.
 * Always returns false and can thus be used in an bandit assertion.
 *
 * @param graph the graph to be printed
 * @param caps the capacities
 * @param node s the source node
 * @param node t the sink node
 * @param flow the calculated flow
 */
template<typename T>
bool printInstance(const Graph &graph, const EdgeArray<T> caps, const node s, const node t, const EdgeArray<T> &flows)
{
	if(!printedFailedInstance) {
		printedFailedInstance = true;

		std::cout << std::endl << "Graph consists of " << graph.numberOfNodes() << " nodes:" << std::endl;
		for(node v : graph.nodes) {
			std::cout << "  " << v;
			if(v == s) { std::cout << " [source]"; }
			if(v == t) { std::cout << " [sink]"; }
			std::cout << std::endl;
		}
		std::cout << "Graph has " << graph.numberOfEdges() << " edges:" << std::endl;
		for(edge e : graph.edges) {
			std::cout << "  " << e << " " << flows[e] << " / " << caps[e] << std::endl;
		}
	}
	return false;
}
#endif

/**
 * Asserts the provided flow is valid.
 * Utilizes EpsilonTest to check the flow values.
 *
 * @param graph the problem instance
 * @param caps the capacity of each edge
 * @param s the source node
 * @param t the sink node
 * @param flows the flow to be validated
 * @param the total flow from source to sink
 * @param computeFlow if true the reference algorithms result is
 *        compared to the given flow
 */
template<typename VALUE_TYPE>
void validateFlow(
  const Graph &graph,
  const EdgeArray<VALUE_TYPE> &caps,
  const node s,
  const node t,
  const EdgeArray<VALUE_TYPE> &flows,
  const VALUE_TYPE flow,
  bool computeFlow = false)
{
	EpsilonTest et;
	const VALUE_TYPE ZERO(0);

	for(edge e : graph.edges) {
		AssertThat(et.leq(flows[e], caps[e]) || printInstance(graph, caps, s, t, flows),  IsTrue());
	}

	for(node v : graph.nodes) {
		VALUE_TYPE income(ZERO);
		VALUE_TYPE output(ZERO);
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			if(e->isSelfLoop()) {
				// self-loop, flow must be 0
				AssertThat(et.equal(flows[e], ZERO) || printInstance(graph, caps, s, t, flows), IsTrue());
			} else {
				if(e->source() == v) {
					output += flows[e];
				} else {
					OGDF_ASSERT(e->target() == v);
					income += flows[e];
				}
			}
		}
		if(v == s) {
			// there are Max-Flow algorithms that allow incoming flow in s
			AssertThat(et.equal(output, flow+income) || printInstance(graph, caps, s, t, flows), IsTrue());
		} else if(v == t) {
			// there are Max-Flow algorithms that allow outgoing flow from t
			AssertThat(et.equal(income, flow+output) || printInstance(graph, caps, s, t, flows), IsTrue());
		} else {
			AssertThat(et.equal(income, output) || printInstance(graph, caps, s, t, flows), IsTrue());
		}
	}

	// using Edmonds & Karp algorithm for reference
	if(computeFlow) {
		MaxFlowEdmondsKarp<VALUE_TYPE> mfek(graph);
		VALUE_TYPE refFlow = mfek.computeValue(caps, s, t);
		AssertThat(et.equal(flow, refFlow) || printInstance(graph, caps, s, t, flows), IsTrue());
	}
}

/**
 * Tests a given maximum flow algorithm.
 *
 * @param name the human-readable description of this algorithm
 * @param the requiremets for this algorithm
 */
template<typename MAX_FLOW_ALGO, typename VALUE_TYPE>
void describeMaxFlowModule(const string &name, const MaxFlowRequirement reqs = MFR_NONE)
{
	const int maxCapacity = 100;
	const int maxNodes = 50;

	describe(name, [&](){
		// test predefined instances
		for_each_file("maxflow", [&](const ResourceFile* file){
			it("works on " + file->fullPath(), [&] {
				// optimal solution value is extracted from the filename
				std::string filename = file->name();
				string tmp = filename.substr(0, filename.length() - 4);
				tmp = tmp.substr(tmp.find_last_of('.') + 1);
				std::stringstream ss(tmp);
				VALUE_TYPE opt = -1;
				ss >> opt;

				Graph graph;
				EdgeArray<VALUE_TYPE> caps(graph, 0);
				node s;
				node t;
				std::stringstream is{file->data()};
				AssertThat(GraphIO::readDMF(graph, caps, s, t, is), IsTrue());
				MaxFlowRequirement props = determineProperties(graph, s, t);

				// create non s-t-incident face if required
				if(reqs & MFR_ST_NON_INCIDENT_FACE) {
					props = props | MFR_ST_NON_INCIDENT_FACE;
					node v = graph.newNode();
					caps[graph.newEdge(v, t)] = 0;
					caps[graph.newEdge(t, v)] = 0;
				}

				if(props & MFR_PLANAR) {
					planarEmbed(graph);
				}

				if((reqs & props) == reqs)  {
					MAX_FLOW_ALGO alg(graph);

					VALUE_TYPE value = alg.computeValue(caps, s, t);
					AssertThat(value, Equals(opt));

					EdgeArray<VALUE_TYPE> flow(graph);
					alg.computeFlowAfterValue(flow);
					validateFlow(graph, caps, s, t, flow, value);
				}
			});
		});

		// test random instances
		for(int n = 2; n < maxNodes; n++) {
			it("works on a random graph of approximate size " + to_string(n), [&] {
				Graph graph;
				EdgeArray<VALUE_TYPE> caps(graph);
				node s = nullptr;
				node t = nullptr;

				// generate a connected graph based on the requirements of this algorithm
				if(reqs & MFR_ST_PLANAR) {
					if(n % 2) {
						int r = 1 + (int)sqrt(n);
						gridGraph(graph, r, r, false, false);
						List<node> nodes;
						graph.allNodes(nodes);
						s = *nodes.get(randomNumber(0, r-1));
						t = *nodes.get(randomNumber(r*(r-1), r*r-1));
					} else {
						int m = randomNumber(n-1, max(n-1, 3*n-6));
						randomPlanarConnectedGraph(graph, n, m);
						s = graph.chooseNode();
						CombinatorialEmbedding ce(graph);

						// select sink with common face
						for(adjEntry adj = s->firstAdj();
						    t == nullptr || randomNumber(0, s->degree());
						    adj = adj->faceCycleSucc()) {
							node v = adj->theNode();

							if(s != v) {
								t = v;
							}
						}
					}
				} else if(reqs & MFR_PLANAR) {
					int m = randomNumber(n-1, max(n-1, 3*n-6));
					randomPlanarConnectedGraph(graph, n, m);
				} else {
					int m = randomNumber(n*2, max(n*2, n*(n-1)/2));
					randomBiconnectedGraph(graph, n, m);
				}

				// generate capacities
				caps.init(graph);
				for (edge e: graph.edges) {
					caps[e] = (VALUE_TYPE) randomDouble(1, maxCapacity);
				}

				// choose source and sink
				if(s == nullptr || s == t) {
					s = graph.chooseNode([&](node v) { return v != t; });
				}
				while(t == nullptr || t == s) {
					t = graph.chooseNode([&](node v) { return v != s; });
				}

				// create non s-t-incident face if required
				if(reqs & MFR_ST_NON_INCIDENT_FACE) {
					node v = graph.newNode();
					caps[graph.newEdge(v, t)] = 0;
					caps[graph.newEdge(t, v)] = 0;
				}

				// compute flow and validate it
				MAX_FLOW_ALGO alg(graph);
				EdgeArray<VALUE_TYPE> algFlows(graph);

				VALUE_TYPE algFlow = alg.computeValue(caps, s, t);
				alg.computeFlowAfterValue(algFlows);

				validateFlow(graph, caps, s, t, algFlows, algFlow, true);
			});
		}
	});
}

template<typename T>
void registerTestSuite(const string typeName)
{
	const string suffix = "<" + typeName + ">";

	describeMaxFlowModule<MaxFlowSTPlanarItaiShiloach<T>, T>("MaxFlowSTPlanarItaiShiloach" + suffix,
	  MFR_CONNECTED | MFR_ST_PLANAR);
	describeMaxFlowModule<MaxFlowSTPlanarDigraph<T>, T>("MaxFlowSTPlanarDigraph" + suffix,
	  MFR_CONNECTED | MFR_ST_PLANAR);
	describeMaxFlowModule<MaxFlowEdmondsKarp<T>, T>("MaxFlowEdmondsKarp" + suffix);
	describeMaxFlowModule<MaxFlowGoldbergTarjan<T>, T>("MaxFlowGoldbergTarjan" + suffix);
}

/**
 * Tests the ConnectivityTester on the given graphs.
 * Node and edge connectivity is computed for both directed and undirected (i.e. bi-directed) graphs.
 * The resulting values are tested for consistency.
 *
 * @param title A description of the graphs
 * @param expected The minimal expected node connectivity
 * @param initializer A lambda expression used to initialize the test instances.
 *                    Each call is provided with the graph to be initialized and a counter.
 */
void describeConnectivityTester(
        string title,
        int expected,
        std::function<void (Graph&, int)> initializer)
{
describe(title, [&]()
{
	const int maxNodes = 50;

	// undirected node connectivity
	ConnectivityTester nodeAlgo;

	// undirected edge connectivity
	ConnectivityTester edgeAlgo(false);

	// directed node connectivity
	ConnectivityTester nodeDirAlgo(true, true);

	// directed edge connectivity
	ConnectivityTester edgeDirAlgo(false, true);

	for (int n = 3; n < maxNodes / 2; n++) {
		it("works for " + to_string(n) + " nodes", [&] {
			Graph graph;
			initializer(graph, n);

			NodeArray<NodeArray<int>> edgeCon(graph, NodeArray<int>(graph));
			NodeArray<NodeArray<int>> nodeCon(graph, NodeArray<int>(graph));
			NodeArray<NodeArray<int>> edgeDirCon(graph, NodeArray<int>(graph));
			NodeArray<NodeArray<int>> nodeDirCon(graph, NodeArray<int>(graph));

			// compute the connectivity
			edgeAlgo.computeConnectivity(graph, edgeCon);
			int minConnectivity = nodeAlgo.computeConnectivity(graph, nodeCon);
			nodeDirAlgo.computeConnectivity(graph, nodeDirCon);
			edgeDirAlgo.computeConnectivity(graph, edgeDirCon);

			AssertThat(minConnectivity, IsGreaterThan(expected - 1));

			// assert consistency with existing tests
			if(n > 3 && isTriconnected(graph)) {
				AssertThat(minConnectivity, IsGreaterThan(2));
			} else if(n > 2 && isBiconnected(graph)) {
				AssertThat(minConnectivity, IsGreaterThan(1));
			} else if(n > 1 && isConnected(graph)) {
				AssertThat(minConnectivity, IsGreaterThan(0));
			}

			// check consistency of connectivity variants
			for (node v : graph.nodes) {
				for (node w : graph.nodes) {
					if (v == w) {
						AssertThat(nodeCon[v][w], Equals(0));
					} else {
						// compare with expected values
						AssertThat(nodeCon[v][w], IsGreaterThan(expected - 1));
						AssertThat(nodeCon[v][w], IsGreaterThan(minConnectivity - 1));

						// edge connectivity is least restrictive
						AssertThat(edgeCon[v][w], IsGreaterThan(nodeCon[v][w] - 1));
						AssertThat(edgeCon[v][w], IsGreaterThan(edgeDirCon[v][w] - 1));

						// (node) connectivity might never be greater than edge connectivity
						AssertThat(edgeCon[v][w], IsGreaterThan(edgeDirCon[v][w] - 1));

						// directed connectivity is most restrictive
						AssertThat(nodeCon[v][w], IsGreaterThan(nodeDirCon[v][w] - 1));
						AssertThat(edgeDirCon[v][w], IsGreaterThan(nodeDirCon[v][w] - 1));
					}
				}
			}

			// create a new node with some edges
			// thus reducing the overall connectivity
			if(minConnectivity > 0) {
				node w = graph.firstNode();
				node v = graph.newNode();
				int modifiedExpected = randomNumber(0, minConnectivity - 1);
				for (int i = 0; i < modifiedExpected; i++) {
					OGDF_ASSERT(w != v);
					graph.newEdge(w, v);
					w = w->succ();
				}

				AssertThat(nodeAlgo.computeConnectivity(graph, nodeCon), Equals(modifiedExpected));
			}
		});
	}
});
}

go_bandit([]() {
	describe("Maximum flow algorithms", [](){
		registerTestSuite<int>("int");
		registerTestSuite<double>("double");
		registerTestSuite<unsigned long long int>("unsigned long long int");
	});

	describe("Connectivity Tester", [](){
		describeConnectivityTester("random graphs", 0, [](Graph &graph, int n) {
			randomGraph(graph, n, randomNumber(n, (n*(n-1))/2));
		});

		describeConnectivityTester("planar connected graphs", 1, [](Graph &graph, int n) {
			randomPlanarConnectedGraph(graph, n, randomNumber(n, (n*(n-1))/2));
		});

		describeConnectivityTester("biconnected graphs", 2, [](Graph &graph, int n) {
			randomBiconnectedGraph(graph, n, randomNumber(n, (n*(n-1))/2));
		});

		describeConnectivityTester("triconnected graphs", 3, [](Graph &graph, int n) {
			randomTriconnectedGraph(graph, n, randomDouble(0, 1), randomDouble(0, 1));
		});
	});
});
