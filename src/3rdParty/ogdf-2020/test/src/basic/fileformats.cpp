/** \file
 * \brief Tests for fileformat reading and writing using GraphIO,
 *   only graphs without attributes
 *
 * \author Stephan Beyer, Tilo Wiedera
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

#include <cstdio>
#include <algorithm>
#include <string>
#include <regex>
#include <unordered_map>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/EpsilonTest.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/fileformats/GraphIO.h>
#include <resources.h>
#include <graphs.h>

using namespace std;
using Reader = function<bool(Graph&, istream&)>;
using Writer = function<bool(const Graph&, ostream&)>;
using ClusterReader = function<bool(ClusterGraph&, Graph&, istream&)>;
using ClusterWriter = function<bool(const ClusterGraph&, ostream&)>;

void assertSeemsEqual(const Graph &G1, const Graph &G2) {
	AssertThat(G1.numberOfNodes(), Equals(G2.numberOfNodes()));
	AssertThat(G1.numberOfEdges(), Equals(G2.numberOfEdges()));

	Array<int> counter1, counter2;
	degreeDistribution(G1, counter1);
	degreeDistribution(G2, counter2);

	AssertThat(counter1.size(), Equals(counter2.size()));
	AssertThat(counter1.low(), Equals(counter2.low()));

	for(int i = counter1.low(); i < counter1.high(); i++) {
		AssertThat(counter1[i], Equals(counter2[i]));
	}
}

void assertSeemsEqual(const ClusterGraph &CG1, const ClusterGraph &CG2) {
	const Graph &G = CG1.constGraph();
	assertSeemsEqual(G, CG2.constGraph());

	AssertThat(CG1.numberOfClusters(), Equals(CG2.numberOfClusters()));
}

void establishNodeMapping(NodeArray<node> &map1to2, const GraphAttributes &GA1,
                          const GraphAttributes &GA2)
{
	const Graph &G1 = GA1.constGraph();
	const Graph &G2 = GA2.constGraph();
	std::vector<node> mapIndexToNode;
	mapIndexToNode.resize(G1.numberOfNodes());
	for(node v1 : G1.nodes) {
		int x1;
		if(GA1.has(GraphAttributes::nodeGraphics)) {
			x1 = GA1.x(v1) - 1;
		} else {
			AssertThat(GA1.has(GraphAttributes::nodeLabel), IsTrue());
			x1 = atoi(GA1.label(v1).c_str());
		}
		AssertThat(mapIndexToNode[x1], Equals(nullptr));
		mapIndexToNode[x1] = v1;
	}
	for(node v2 : G2.nodes) {
		int x2;
		if(GA1.has(GraphAttributes::nodeGraphics)) {
			x2 = GA2.x(v2) - 1;
		} else {
			AssertThat(GA1.has(GraphAttributes::nodeLabel), IsTrue());
			x2 = atoi(GA2.label(v2).c_str());
		}
		AssertThat(map1to2[mapIndexToNode[x2]], Equals(nullptr));
		map1to2[mapIndexToNode[x2]] = v2;
	}
}
void establishClusterMapping(ClusterArray<cluster> &map1to2, const ClusterGraphAttributes &CGA1,
                          const ClusterGraphAttributes &CGA2)
{
	const ClusterGraph &CG1 = CGA1.constClusterGraph();
	const ClusterGraph &CG2 = CGA2.constClusterGraph();
	std::vector<cluster> mapIndexToCluster;
	mapIndexToCluster.resize(CG1.numberOfClusters());
	for(cluster c1 : CG1.clusters) {
		int x1;
		if(CGA1.has(ClusterGraphAttributes::clusterGraphics)) {
			x1 = CGA1.x(c1) - 1;
		} else {
			AssertThat(CGA1.has(ClusterGraphAttributes::clusterLabel), IsTrue());
			x1 = atoi(CGA1.label(c1).c_str());
		}
		AssertThat(mapIndexToCluster[x1], Equals(nullptr));
		mapIndexToCluster[x1] = c1;
	}
	for(cluster c2 : CG2.clusters) {
		int x2;
		if(CGA1.has(ClusterGraphAttributes::clusterGraphics)) {
			x2 = CGA2.x(c2) - 1;
		} else {
			AssertThat(CGA1.has(ClusterGraphAttributes::clusterLabel), IsTrue());
			x2 = atoi(CGA2.label(c2).c_str());
		}
		map1to2[mapIndexToCluster[x2]] = c2;
	}
}

void assertEqualGAs(const GraphAttributes &GA1, const GraphAttributes &GA2, bool supportsDirected) {
	const Graph &G1 = GA1.constGraph();
	const Graph &G2 = GA2.constGraph();
	NodeArray<node> map1to2(G1, nullptr);
	AssertThat(GA1.attributes(), Equals(GA2.attributes()));
	AssertThat(G1.numberOfNodes(), Equals(G2.numberOfNodes()));
	AssertThat(G1.numberOfEdges(), Equals(G2.numberOfEdges()));
	if (supportsDirected) AssertThat(GA1.directed(), Equals(GA2.directed()));

	establishNodeMapping(map1to2, GA1, GA2);

	constexpr double delta { 0.5 };

	for(node v : G1.nodes) {
		if(GA1.has(GraphAttributes::nodeGraphics)) {
			AssertThat(GA2.x(map1to2[v]), Equals(GA1.x(v)));
			AssertThat(GA2.y(map1to2[v]), EqualsWithDelta(GA1.y(v), delta));
			if(GA1.has(GraphAttributes::threeD)) {
				AssertThat(GA2.z(map1to2[v]), EqualsWithDelta(GA1.z(v), delta));
			}
			AssertThat(GA2.width(map1to2[v]), EqualsWithDelta(GA1.width(v), delta));
			AssertThat(GA2.height(map1to2[v]), EqualsWithDelta(GA1.height(v), delta));
			AssertThat(GA2.shape(map1to2[v]), Equals(GA1.shape(v)));
		}
		if(GA1.has(GraphAttributes::nodeId)) {
			AssertThat(GA2.idNode(map1to2[v]), Equals(GA1.idNode(v)));
		}
		if(GA1.has(GraphAttributes::nodeLabel)) {
			AssertThat(GA2.label(map1to2[v]), Equals(GA1.label(v)));
		}
		if(GA1.has(GraphAttributes::nodeLabelPosition)) {
			AssertThat(GA2.xLabel(map1to2[v]), EqualsWithDelta(GA1.xLabel(v), delta));
			AssertThat(GA2.yLabel(map1to2[v]), EqualsWithDelta(GA1.yLabel(v), delta));
			if(GA1.has(GraphAttributes::threeD)) {
				AssertThat(GA2.zLabel(map1to2[v]), Equals(GA1.zLabel(v)));
			}
		}
		if(GA1.has(GraphAttributes::nodeStyle)) {
			AssertThat(GA2.fillColor(map1to2[v]), Equals(GA1.fillColor(v)));
			AssertThat(GA2.strokeColor(map1to2[v]), Equals(GA1.strokeColor(v)));
			AssertThat(GA2.strokeType(map1to2[v]), Equals(GA1.strokeType(v)));
			AssertThat(GA2.strokeWidth(map1to2[v]), Equals(GA1.strokeWidth(v)));
			AssertThat(GA2.fillPattern(map1to2[v]), Equals(GA1.fillPattern(v)));
			AssertThat(GA2.fillBgColor(map1to2[v]), Equals(GA1.fillBgColor(v)));
		}
		if(GA1.has(GraphAttributes::nodeTemplate)) {
			AssertThat(GA2.templateNode(map1to2[v]), Equals(GA1.templateNode(v)));
		}
		if(GA1.has(GraphAttributes::nodeType)) {
			AssertThat(int(GA2.type(map1to2[v])), Equals(int(GA1.type(v))));
		}
		if(GA1.has(GraphAttributes::nodeWeight)) {
			AssertThat(GA2.weight(map1to2[v]), EqualsWithDelta(GA1.weight(v), delta));
		}
	}
	for(edge e : G1.edges) {
		edge e2 = G2.searchEdge(map1to2[e->source()], map1to2[e->target()], GA1.directed() && supportsDirected);
		AssertThat(e2, !Equals(nullptr));

		if(GA1.has(GraphAttributes::edgeArrow)) {
			AssertThat(GA2.arrowType(e2), Equals(GA1.arrowType(e)));
		}
		if(GA1.has(GraphAttributes::edgeGraphics)) {
			AssertThat(GA2.bends(e2), Equals(GA1.bends(e)));
		}
		if(GA1.has(GraphAttributes::edgeLabel)) {
			AssertThat(GA2.label(e2), Equals(GA1.label(e)));
		}
		if(GA1.has(GraphAttributes::edgeType)) {
			AssertThat(GA2.type(e2), Equals(GA1.type(e)));
		}
		if(GA1.has(GraphAttributes::edgeStyle)) {
			AssertThat(GA2.strokeColor(e2), Equals(GA1.strokeColor(e)));
			AssertThat(GA2.strokeType(e2), Equals(GA1.strokeType(e)));
			AssertThat(GA2.strokeWidth(e2), EqualsWithDelta(GA1.strokeWidth(e), delta));
		}
		if(GA1.has(GraphAttributes::edgeDoubleWeight)) {
			AssertThat(GA2.doubleWeight(e2), EqualsWithDelta(GA1.doubleWeight(e), delta));
		}
		if(GA1.has(GraphAttributes::edgeIntWeight)) {
			AssertThat(GA2.intWeight(e2), Equals(GA1.intWeight(e)));
		}
		if(GA1.has(GraphAttributes::edgeSubGraphs)) {
			AssertThat(GA2.subGraphBits(e2), Equals(GA1.subGraphBits(e)));
		}
	}
}

void assertEqualCGAs(const ClusterGraphAttributes& CGA1, const ClusterGraphAttributes& CGA2, bool supportsDirected) {
	AssertThat(CGA1.attributes(), Equals(CGA2.attributes()));

	// First test for inherited non-cluster attributes
	// and the underlying graphs
	assertEqualGAs(CGA1, CGA2, supportsDirected);

	const Graph &G1 = CGA1.constGraph();
	const ClusterGraph &CG1 = CGA1.constClusterGraph();
	const ClusterGraph &CG2 = CGA2.constClusterGraph();
	NodeArray<node> nodeMap1to2(G1, nullptr);
	establishNodeMapping(nodeMap1to2, CGA1, CGA2);
	ClusterArray<cluster> clusterMap1to2(CG1, nullptr);
	establishClusterMapping(clusterMap1to2, CGA1, CGA2);

	constexpr double delta { 0.5 };

	AssertThat(CG1.numberOfClusters(), Equals(CG2.numberOfClusters()));

	for (cluster c : CG1.clusters) {
		if(CGA1.has(ClusterGraphAttributes::clusterGraphics)) {
			AssertThat(CGA2.x(clusterMap1to2[c]), Equals(CGA1.x(c)));
			AssertThat(CGA2.y(clusterMap1to2[c]), EqualsWithDelta(CGA1.y(c), delta));
			AssertThat(CGA2.width(clusterMap1to2[c]), EqualsWithDelta(CGA1.width(c), delta));
			AssertThat(CGA2.height(clusterMap1to2[c]), EqualsWithDelta(CGA1.height(c), delta));
		}
		if(CGA1.has(ClusterGraphAttributes::clusterStyle)) {
			AssertThat(CGA2.strokeType(clusterMap1to2[c]), Equals(CGA1.strokeType(c)));
			AssertThat(CGA2.strokeColor(clusterMap1to2[c]), Equals(CGA1.strokeColor(c)));
			AssertThat(CGA2.strokeWidth(clusterMap1to2[c]), EqualsWithDelta(CGA1.strokeWidth(c), delta));
			AssertThat(CGA2.fillPattern(clusterMap1to2[c]), Equals(CGA1.fillPattern(c)));
			AssertThat(CGA2.fillColor(clusterMap1to2[c]), Equals(CGA1.fillColor(c)));
			AssertThat(CGA2.fillBgColor(clusterMap1to2[c]), Equals(CGA1.fillBgColor(c)));
		}
		if(CGA1.has(ClusterGraphAttributes::clusterLabel)) {
			AssertThat(CGA2.label(clusterMap1to2[c]), Equals(CGA1.label(c)));
		}
		if(CGA1.has(ClusterGraphAttributes::clusterTemplate)) {
			AssertThat(CGA2.templateCluster(clusterMap1to2[c]), Equals(CGA1.templateCluster(c)));
		}
	}

	for (node v : G1.nodes) {
		AssertThat(CG2.clusterOf(nodeMap1to2[v]), Equals(clusterMap1to2[CG1.clusterOf(v)]));
	}
}

//! Writes the graph \p out using \p writer, then reads it into \p in using \p reader
//! and checks if \p out and \p in seem to be "equivalent" graphs.
void testWriteAndRead(const Graph& out, Writer writer, Graph& in, Reader reader) {
	std::ostringstream write;
	AssertThat(writer(out, write), IsTrue());
	std::istringstream read(write.str());
	AssertThat(reader(in, read), IsTrue());
	assertSeemsEqual(out, in);
}

//! Shortcut of #testWriteAndRead for the case that you do not need to process the read graph
void testWriteAndRead(const Graph& out, Writer writer, Reader reader) {
	Graph in;
	testWriteAndRead(out, writer, in, reader);
}

void testWriteAndRead(const ClusterGraph& out, ClusterWriter writer, ClusterGraph& in, Graph& inG, ClusterReader reader) {
	std::ostringstream write;
	AssertThat(writer(out, write), IsTrue());
	std::istringstream read(write.str());
	AssertThat(reader(in, inG, read), IsTrue());
	assertSeemsEqual(out, in);
}
void testWriteAndRead(const ClusterGraph& out, ClusterWriter writer, ClusterReader reader) {
	Graph inG;
	ClusterGraph in(inG);
	testWriteAndRead(out, writer, in, inG, reader);
}

//! Perform tests that first write and then read a file with given \p writer and \p reader
static void describeWriteAndRead(std::set<GraphProperty> reqs, Writer writer, Reader reader, int minSize = 0) {
	describe("first writing then reading", [&] {
		forEachGraphItWorks(reqs, [&](const Graph& graph) {
			testWriteAndRead(graph, writer, reader);
		}, GraphSizes(), minSize);

		it("works on a big complete graph", [&]() {
			Graph G;
			completeGraph(G, 243);
			testWriteAndRead(G, writer, reader);
		});
	});
}
//! Perform tests that first write and then read a file with given cluster \p writer and \p reader
static void describeWriteAndRead(std::set<GraphProperty> reqs, ClusterWriter writer, ClusterReader reader) {
	describe("first writing then reading", [&] {
		forEachGraphItWorks(reqs, [&](Graph& graph) {
			ClusterGraph CG(graph);
			randomClusterGraph(CG, graph, 7);
			testWriteAndRead(CG, writer, reader);
		}, GraphSizes(), 10);
	});
}

//! Perform tests reading resource files with given \p reader
static void describeResourceBased(const std::string name, bool isXml, Reader reader) {
	describe("reading particular files", [&] {
		std::string lowerCaseName{name};
		std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), ::tolower);

		auto invalidTest = [&](const ResourceFile* file, bool skip = false) {
			it("detects errors in " + file->fullPath(), [&]() {
				Graph graph;
				stringstream ss{file->data()};
				AssertThat(reader(graph, ss), IsFalse());
			}, skip);
		};
		auto validTest = [&](const ResourceFile* file, bool skip = false) {
			it("successfully parses " + file->fullPath(), [&](){
				Graph graph;
				stringstream ss{file->data()};
				AssertThat(reader(graph, ss), IsTrue());
				AssertThat(graph.numberOfNodes(), IsGreaterThan(0));
				AssertThat(graph.numberOfEdges(), IsGreaterThan(0));
			}, skip);
		};

		for_each_file("fileformats/" + lowerCaseName + "/valid", validTest);
		for_each_file("fileformats/" + lowerCaseName + "/valid/skip", std::bind(validTest, std::placeholders::_1, true));
		for_each_file("fileformats/" + lowerCaseName + "/invalid", invalidTest);
		for_each_file("fileformats/" + lowerCaseName + "/invalid/skip", std::bind(invalidTest, std::placeholders::_1, true));

		if(isXml) {
			for_each_file("fileformats/xml/invalid", invalidTest);
		}
	});
}

void describeIssueHandling(Reader reader, Writer writer, int minSize) {
	describe("general issue handling", [&] {
		it("detects invalid input streams", [&]() {
			Graph G;
			std::istringstream badStream;
			badStream.setstate(std::istringstream::badbit);
			AssertThat(reader(G, badStream), IsFalse());
		});

		it("detects invalid output streams", [&]() {
			Graph G;
			randomGraph(G, 10, 20);
			std::ostringstream badStream;
			badStream.setstate(std::ostringstream::badbit);
			AssertThat(writer(G, badStream), IsFalse());
		}, writer == nullptr);

		it("returns false if input file does not exist", [&] {
			Graph graph;
			ifstream input;
			input.close();
			AssertThat(reader(graph, input), IsFalse());
		});

		it("clears the graph", [&]() {
			Graph writeGraph;
			emptyGraph(writeGraph, minSize);
			std::ostringstream write;
			AssertThat(writer(writeGraph, write), IsTrue());

			Graph readGraph;
			customGraph(readGraph, 2, {{0, 1}});
			std::istringstream read(write.str());
			AssertThat(reader(readGraph, read), IsTrue());
			AssertThat(readGraph.numberOfNodes(), Equals(minSize));
			AssertThat(readGraph.numberOfEdges(), Equals(0));
		}, writer == nullptr);
	});
}
void describeIssueHandling(ClusterReader reader, ClusterWriter writer) {
	describe("general issue handling", [&] {
		it("detects invalid input streams", [&]() {
			Graph G;
			ClusterGraph CG(G);
			std::istringstream badStream;
			badStream.setstate(std::istringstream::badbit);
			AssertThat(reader(CG, G, badStream), IsFalse());
		});

		it("detects invalid output streams", [&]() {
			Graph G;
			randomGraph(G, 10, 20);
			ClusterGraph CG(G);
			std::ostringstream badStream;
			badStream.setstate(std::ostringstream::badbit);
			AssertThat(writer(CG, badStream), IsFalse());
		}, writer == nullptr);

		it("returns false if input file does not exist", [&] {
			Graph graph;
			ClusterGraph CG(graph);
			ifstream input;
			input.close();
			AssertThat(reader(CG, graph, input), IsFalse());
		});

		it("clears the graph", [&]() {
			Graph writeGraph;
			emptyGraph(writeGraph, 10);
			ClusterGraph writeCG(writeGraph);
			std::ostringstream write;
			AssertThat(writer(writeCG, write), IsTrue());

			Graph readGraph;
			customGraph(readGraph, 2, {{0, 1}});
			ClusterGraph readCG(readGraph);
			randomClusterGraph(readCG, readGraph, 2);
			std::istringstream read(write.str());
			AssertThat(reader(readCG, readGraph, read), IsTrue());
			AssertThat(readGraph.numberOfNodes(), Equals(10));
			AssertThat(readGraph.numberOfEdges(), Equals(0));
			AssertThat(readCG.numberOfClusters(), Equals(1));
		}, writer == nullptr);
	});
}

/**
 * Used to describe a format parser and writer.
 *
 * \param name The name of the format.
 * \param reader The parse function to be tested.
 * \param writer The write function to be tested.
 * \param isXml Whether the format is based on XML.
 * \param reqs The requirements a graph has to fulfill in order to be read and written properly using the format.
 * \param minSize Minimum number of nodes for tested instances.
 */
void describeFormat(const std::string name,
		Reader reader,
		Writer writer,
		bool isXml,
		std::set<GraphProperty> reqs = {},
		int minSize = 0) {
	OGDF_ASSERT(reader != nullptr);
	describeResourceBased(name, isXml, reader);
	if (writer != nullptr) {
		describeWriteAndRead(reqs, writer, reader, minSize);
	}
	describeIssueHandling(reader, writer, minSize);
}

// Auxiliary function to cast between std::function and function pointer
void describeFormat(const std::string name,
		GraphIO::ReaderFunc reader,
		GraphIO::WriterFunc writer,
		bool isXml,
		std::set<GraphProperty> reqs = {},
		int minSize = 0) {
	describeFormat(name, static_cast<Reader>(reader), static_cast<Writer>(writer), isXml, reqs, minSize);
}

void describeClusterFormat(
		ClusterReader reader,
		ClusterWriter writer,
		std::set<GraphProperty> reqs = {}) {
	describe("ClusterGraph handling", [&] {
		if (writer != nullptr) {
			describeWriteAndRead(reqs, writer, reader);
		}
		describeIssueHandling(reader, writer);
	});
}
void describeClusterFormat(
		GraphIO::ClusterReaderFunc reader,
		GraphIO::ClusterWriterFunc writer,
		std::set<GraphProperty> reqs = {}) {
	describeClusterFormat(static_cast<ClusterReader>(reader), static_cast<ClusterWriter>(writer), reqs);
}

/**
 * Creates dummy graph attributes
 *
 * @param GA attributes to fill
 * @param squareNodes whether nodes have to be square in the current format (e.g. GEXF)
 */
void createGraphAttributes(GraphAttributes &GA, bool squareNodes = false) {
	const Graph &graph = GA.constGraph();
	long attr = GA.attributes();

	GA.directed() = false;
	for(node v : graph.nodes) {
		if(attr & GraphAttributes::nodeLabelPosition) {
			GA.xLabel(v) = v->index();
			GA.yLabel(v) = randomNumber(1, std::numeric_limits<int>::max());
			if(attr & GraphAttributes::threeD) {
				GA.zLabel(v) = randomNumber(1, std::numeric_limits<int>::max());
			}
		}
		if(attr & GraphAttributes::nodeStyle) {
			GA.strokeColor(v) = randomNumber(0,1) ? Color::Name::Peru : Color::Name::Whitesmoke;
			GA.strokeType(v) = randomNumber(0,1) ? StrokeType::Dashdotdot : StrokeType::Solid;
			GA.strokeWidth(v) = randomNumber(1, std::numeric_limits<int>::max());
			GA.fillPattern(v) = randomNumber(0, 1) ? FillPattern::Cross : FillPattern::Dense1;
			GA.fillColor(v) = randomNumber(0, 1) ? Color::Name::Blanchedalmond : Color::Name::Gainsboro;
			GA.fillBgColor(v) = randomNumber(0, 1) ? Color::Name::Mistyrose : Color::Name::Mintcream;
		}
		if(attr & GraphAttributes::threeD) {
			GA.z(v) = randomNumber(1, std::numeric_limits<int>::max());
		}
		if(attr & GraphAttributes::nodeWeight) {
			GA.weight(v) = randomNumber(1, std::numeric_limits<int>::max());
		}
		if(attr & GraphAttributes::nodeTemplate) {
			GA.templateNode(v) = to_string(randomNumber(1, std::numeric_limits<int>::max()));
		}
		if(attr & GraphAttributes::nodeType) {
			GA.type(v) = (randomNumber(0, 1) ? Graph::NodeType::dummy : Graph::NodeType::associationClass);
		}
		if(attr & GraphAttributes::nodeLabel) {
			GA.label(v) = to_string(v->index());
		}
		if(attr & GraphAttributes::nodeId) {
			GA.idNode(v) = v->index();
		}
		if(attr & GraphAttributes::nodeGraphics) {
			GA.x(v) = v->index() + 1;
			GA.y(v) = randomNumber(1, std::numeric_limits<int>::max());
			int size = randomNumber(1, 10);
			GA.width(v) = size;
			GA.height(v) = squareNodes ? size : randomNumber(1, 10);
			GA.shape(v) = (randomNumber(0, 1) ? Shape::Ellipse : Shape::Image);
		}
	}
	for(edge e : graph.edges) {
		if(attr & GraphAttributes::edgeGraphics) {
			DPolyline bends1;
			bends1.emplaceFront(randomNumber(1, std::numeric_limits<int>::max()), randomNumber(1, std::numeric_limits<int>::max()));
			GA.bends(e) = bends1;
		}
		if(attr & GraphAttributes::edgeIntWeight) {
			GA.intWeight(e) = randomNumber(2, std::numeric_limits<int>::max());
		}
		if(attr & GraphAttributes::edgeDoubleWeight) {
			GA.doubleWeight(e) = randomNumber(2, std::numeric_limits<int>::max());
		}
		if(attr & GraphAttributes::edgeLabel) {
			GA.label(e) = to_string(randomNumber(1, std::numeric_limits<int>::max()));
		}
		if(attr & GraphAttributes::edgeType) {
			GA.type(e) = randomNumber(0,1) ? Graph::EdgeType::generalization : Graph::EdgeType::association;
		}
		if(attr & GraphAttributes::edgeArrow) {
			GA.arrowType(e) = randomNumber(0,1) ? EdgeArrow::Both : EdgeArrow::First;
		}
		if(attr & GraphAttributes::edgeStyle) {
			GA.strokeColor(e) = randomNumber(0,1) ? Color::Name::Papayawhip : Color::Name::Cornsilk;
			GA.strokeType(e) = randomNumber(0,1) ? StrokeType::Dashdotdot : StrokeType::Dashdot;
			GA.strokeWidth(e) = randomNumber(1, std::numeric_limits<int>::max());
		}
		if(attr & GraphAttributes::edgeSubGraphs) {
			GA.addSubGraph(e, e->index() % 2 + 2);
			GA.addSubGraph(e, e->index() % 2);
		}
	}
}
/**
 * Creates dummy cluster graph attributes.
 * Also fills regular attributes.
 *
 * @param CGA attributes to fill
 */
void createClusterGraphAttributes(ClusterGraphAttributes& CGA) {
	const ClusterGraph& CG = CGA.constClusterGraph();
	long attr = CGA.attributes();

	createGraphAttributes(CGA);

	for (cluster c : CG.clusters) {
		if(attr & ClusterGraphAttributes::clusterGraphics) {
			CGA.x(c) = c->index() + 1;
			CGA.y(c) = randomNumber(1, std::numeric_limits<int>::max());
			CGA.width(c) = randomNumber(1, 10);
			CGA.height(c) = randomNumber(1, 10);
		}
		if(attr & ClusterGraphAttributes::clusterStyle) {
			CGA.strokeType(c) = randomNumber(0,1) ? StrokeType::Dashdotdot : StrokeType::Dashdot;
			CGA.strokeColor(c) = randomNumber(0,1) ? Color::Name::Burlywood : Color::Name::Oldlace;
			CGA.strokeWidth(c) = randomNumber(1, std::numeric_limits<int>::max());
			CGA.fillPattern(c) = randomNumber(0,1) ? FillPattern::ForwardDiagonal : FillPattern::Cross;
			CGA.fillColor(c) = randomNumber(0,1) ? Color::Name::Lightseagreen : Color::Name::Firebrick;
			CGA.fillBgColor(c) = randomNumber(0,1) ? Color::Name::Darkorchid : Color::Name::Mediumspringgreen;
		}
		if(attr & ClusterGraphAttributes::clusterLabel) {
			CGA.label(c) = to_string(c->index());
		}
		if(attr & ClusterGraphAttributes::clusterTemplate) {
			CGA.templateCluster(c) = to_string(randomNumber(1, std::numeric_limits<int>::max()));
		}
	}
}

/**
 * Used to describe a format parser and writer that respects GraphAttributes.
 *
 * @copydetails describeFormat
 *
 * @param name The name of the format.
 * @param readerGA The parse function respecting GraphAttributes to be tested.
 * @param writerGA The write function respecting GraphAttributes to be tested.
 * @param isXml Whether the format is based on XML.
 * @param attr The chosen GraphAttributes.
 * @param reqs The requirements a graph has to fulfill in order to be read and written properly using the format.
 * @param supportsDirected Whether the format can encode directedness of edges
 */
void describeGAFormat(const std::string name,
		GraphIO::AttrReaderFunc readerGA, GraphIO::AttrWriterFunc writerGA,
		bool isXml, long attr, std::set<GraphProperty> reqs = {}, bool supportsDirected = true) {
	Reader graphOnlyReader = [&](Graph& G,istream& is) {
		GraphAttributes GA(G, 0);
		return readerGA(GA, G, is);
	};

	Writer graphOnlyWriter = [&](const Graph& G,ostream& os) {
		GraphAttributes GA(G, 0);
		return writerGA(GA, os);
	};

	describe("with GraphAttributes", [&]() {
		describeFormat(name, graphOnlyReader, graphOnlyWriter, isXml, reqs);

		// the following test needs nodeGraphics or nodeLabel in order to work;
		// skip it if not available
		bool infoAvailable(attr & (GraphAttributes::nodeGraphics | GraphAttributes::nodeLabel));
		it("writes and reads a big graph while maintaining GraphAttributes", [&](){
			Graph graph;
			randomSimpleGraph(graph, 20, 40);
			GraphAttributes GA(graph, attr); // all attributes specified in the call activated
			createGraphAttributes(GA, name == "GEXF");

			std::ostringstream write;
			auto flagsBefore = write.flags();
			AssertThat(writerGA(GA, write), Equals(true));
			AssertThat(write.flags(), Equals(flagsBefore));
			std::istringstream read(write.str());
			Graph G2;
			GraphAttributes GA2(G2, attr);
			flagsBefore = read.flags();
			AssertThat(readerGA(GA2, G2, read), Equals(true));
			AssertThat(read.flags(), Equals(flagsBefore));
			assertEqualGAs(GA, GA2, supportsDirected);
		}, !infoAvailable);
	});
}

void describeClusterGAFormat(
		GraphIO::ClusterAttrReaderFunc readerGA,
		GraphIO::ClusterAttrWriterFunc writerGA,
		long attr,
		std::set<GraphProperty> reqs = {},
		bool supportsDirected = true) {

	ClusterReader graphOnlyReader = [&](ClusterGraph& CG, Graph& G, istream& is) {
		ClusterGraphAttributes CGA(CG, 0);
		return readerGA(CGA, CG, G, is);
	};

	ClusterWriter graphOnlyWriter = [&](const ClusterGraph& CG,ostream& os) {
		ClusterGraphAttributes CGA(CG, 0);
		return writerGA(CGA, os);
	};

	describe("ClusterGraph with GraphAttributes", [&]() {
		describeClusterFormat(graphOnlyReader, graphOnlyWriter, reqs);
		// the following test needs nodeGraphics or nodeLabel as well as clusterGraphics or clusterLabel
		// in order to work; skip it if not available
		bool infoAvailable = bool(attr & (GraphAttributes::nodeGraphics | GraphAttributes::nodeLabel))
		    && bool(attr & (ClusterGraphAttributes::clusterGraphics | ClusterGraphAttributes::clusterLabel));
		it("writes and reads a big graph while maintaining GraphAttributes", [&](){
			Graph graph;
			randomSimpleGraph(graph, 20, 40);
			ClusterGraph CG(graph);
			ClusterGraphAttributes CGA(CG, attr); // all attributes specified in the call activated
			randomClusterGraph(CG, graph, 3);
			createClusterGraphAttributes(CGA);

			std::ostringstream write;
			auto flagsBefore = write.flags();
			AssertThat(writerGA(CGA, write), Equals(true));
			AssertThat(write.flags(), Equals(flagsBefore));

			std::istringstream read(write.str());
			Graph G2;
			ClusterGraph CG2(G2);
			ClusterGraphAttributes CGA2(CG2, attr);
			flagsBefore = read.flags();
			AssertThat(readerGA(CGA2, CG2, G2, read), Equals(true));
			AssertThat(read.flags(), Equals(flagsBefore));
			assertEqualCGAs(CGA, CGA2, supportsDirected);
		}, !infoAvailable);
	});
}

/**
 * @copydoc describeGAFormat
 * Use this if the fileformat does not support both edgeDoubleWeight and edgeIntWeight simultaneously!
 */
void describeGAFormatPerEdgeWeightType(
		const std::string name,
		GraphIO::AttrReaderFunc readerGA, GraphIO::AttrWriterFunc writerGA,
		bool isXml, long attr, std::set<GraphProperty> reqs = {}, bool supportsDirected = true)
{
	attr &= ~(GraphAttributes::edgeDoubleWeight | GraphAttributes::edgeIntWeight);
	describeGAFormat(name, readerGA, writerGA, isXml, attr | GraphAttributes::edgeDoubleWeight, reqs, supportsDirected);
	describeGAFormat(name, readerGA, writerGA, isXml, attr | GraphAttributes::edgeIntWeight, reqs, supportsDirected);
}

/**
 * @copydoc describeClusterGAFormat
 * Use this if the fileformat does not support both edgeDoubleWeight and edgeIntWeight simultaneously!
 */
void describeClusterGAFormatPerEdgeWeightType(
		GraphIO::ClusterAttrReaderFunc readerGA,
		GraphIO::ClusterAttrWriterFunc writerGA,
		long attr,
		std::set<GraphProperty> reqs = {},
		bool supportsDirected = true)
{
	attr &= ~(GraphAttributes::edgeDoubleWeight | GraphAttributes::edgeIntWeight);
	describeClusterGAFormat(readerGA, writerGA, attr | GraphAttributes::edgeDoubleWeight, reqs, supportsDirected);
	describeClusterGAFormat(readerGA, writerGA, attr | GraphAttributes::edgeIntWeight, reqs, supportsDirected);
}

/* Specific formats */

void describeGML() {
	describe("GML", [] {
		describeFormat("GML", GraphIO::readGML, GraphIO::writeGML, false);
		// edgeIntWeight and edgeDoubleWeight simultaneously possible
		describeGAFormat("GML", GraphIO::readGML, GraphIO::writeGML, false, GraphAttributes::all);
		describeClusterGAFormat(GraphIO::readGML, GraphIO::writeGML, ClusterGraphAttributes::all);
	});
}

void describeRome() {
	describe("Rome", [] {
		describeFormat("Rome", GraphIO::readRome, GraphIO::writeRome, false);
	});
}

void describeLEDA() {
	describe("LEDA", [] {
		describeFormat("LEDA", GraphIO::readLEDA, GraphIO::writeLEDA, false);
	});
}

void describeChaco() {
	describe("Chaco", [] {
		describeFormat("Chaco", GraphIO::readChaco, GraphIO::writeChaco, false);
	});
}

void describePMDissGraph() {
	describe("PMDissGraph", [] {
		describeFormat("PMDissGraph", GraphIO::readPMDissGraph, GraphIO::writePMDissGraph, false);
	});
}

void describeYGraph() {
	describe("YGraph", [] {
		describeFormat("YGraph", GraphIO::readYGraph, nullptr, false);
	});
}

void describeGraph6() {
	describe("Graph6", [] {
		describeFormat("Graph6", [&](Graph& G, istream &is) { return GraphIO::readGraph6(G, is, false); },
		               GraphIO::writeGraph6, false, {GraphProperty::simple});
	});
	describe("Digraph6", [] {
		describeFormat("Digraph6", [&](Graph& G, istream &is) { return GraphIO::readDigraph6(G, is, false); },
		               GraphIO::writeDigraph6, false, {GraphProperty::simple});
	});
	describe("Sparse6", [] {
		describeFormat("Sparse6", [&](Graph& G, istream &is) { return GraphIO::readSparse6(G, is, false); },
		               GraphIO::writeSparse6, false);
	});
}

void describeMatrixMarket() {
	describe("MatrixMarket", [] {
		describeFormat("MatrixMarket", GraphIO::readMatrixMarket, nullptr, false);
	});
}

void describeRudy() {
	describe("Rudy", [] {
		describeGAFormatPerEdgeWeightType("Rudy", GraphIO::readRudy, GraphIO::writeRudy, false, 0);
	});
}

void describeGraphML() {
	describe("GraphML", [] {
		describeFormat("GraphML", GraphIO::readGraphML, GraphIO::writeGraphML, true);
		describeGAFormatPerEdgeWeightType("GraphML", GraphIO::readGraphML, GraphIO::writeGraphML, true, GraphAttributes::all);
	});
}

void describeDOTSpecialCases() {
	it("reads a cluster graph", []() {
		// Tests reading a DOT clustergraph, using a simplified version of:
		// https://graphviz.gitlab.io/_pages/Gallery/directed/cluster.html
		std::stringstream is{ResourceFile::data("fileformats/dot/valid/cluster")};

		Graph G;
		ClusterGraph CG(G);

		const bool readStatus = GraphIO::readDOT(CG, G, is);
		AssertThat(readStatus, Equals(true));

		// this graph has two clusters inside the root cluster, each of which
		// has four nodes.
		AssertThat(CG.numberOfClusters(), Equals(3));
		AssertThat(CG.rootCluster()->children.size(), Equals(2));
		for (const auto &cluster : CG.rootCluster()->children) {
			AssertThat(cluster->nodes.size(), Equals(4));
		}
	});

	it("reads assignment statements", []() {
		std::stringstream is{ResourceFile::get("fileformats/dot/valid/assignments")->data()};

		Graph G;
		ClusterGraph CG(G);
		ClusterGraphAttributes CGA(CG, ClusterGraphAttributes::clusterLabel);

		const bool readStatus = GraphIO::readDOT(CGA, CG, G, is);
		AssertThat(readStatus, Equals(true));
		AssertThat(CGA.label(CG.rootCluster()), Equals("wat"));
	});

	{ // a scope for the variables to deal with arrow types
		std::stringstream is{ResourceFile::get("fileformats/dot/valid/arrowtypes")->data()};
		Graph G;
		GraphAttributes GA(G, GraphAttributes::edgeArrow);
		const bool readStatus = GraphIO::readDOT(GA, G, is);
		AssertThat(readStatus, Equals(true));
		auto checkDir = [&](EdgeArrow e, const edge& ed, const string &s) {
			it("parses dir attribute set to " + s,
			   [&]() { AssertThat(GA.arrowType(ed), Equals(e)); });
		};
		auto ed = G.firstEdge();
		checkDir(EdgeArrow::Both, ed, "both");
		ed = ed->succ();
		checkDir(EdgeArrow::Last, ed, "last");
		ed = ed->succ();
		checkDir(EdgeArrow::First, ed, "first");
		ed = ed->succ();
		checkDir(EdgeArrow::None, ed, "none");
		ed = ed->succ();
		checkDir(EdgeArrow::Undefined, ed, "undefined");
	}
}

void describeDOT() {
	describe("DOT", [] {
		describeFormat("DOT", GraphIO::readDOT, GraphIO::writeDOT, false, {});
		describeGAFormatPerEdgeWeightType("DOT", GraphIO::readDOT, GraphIO::writeDOT, false, GraphAttributes::all, {}, true);
		describeDOTSpecialCases();
		describeClusterFormat(GraphIO::readDOT, GraphIO::writeDOT, {});
		describeClusterGAFormatPerEdgeWeightType(GraphIO::readDOT, GraphIO::writeDOT, ClusterGraphAttributes::all, {});
	});
}

void describeGEXF() {
	describe("GEXF", [] {
		describeFormat("GEXF", GraphIO::readGEXF, GraphIO::writeGEXF, true);
		describeGAFormatPerEdgeWeightType("GEXF", GraphIO::readGEXF, GraphIO::writeGEXF, true, GraphAttributes::all);
	});
}

void describeGDF() {
	describe("GDF", [] {
		describeFormat("GDF", GraphIO::readGDF, GraphIO::writeGDF, false);
		describeGAFormatPerEdgeWeightType("GDF", GraphIO::readGDF, GraphIO::writeGDF, false,
										GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics
										| GraphAttributes::edgeLabel | GraphAttributes::nodeLabel
										| GraphAttributes::nodeTemplate | GraphAttributes::nodeWeight
										| GraphAttributes::threeD | GraphAttributes::nodeStyle);
	});
}


void describeTLP() {
	describe("TLP", [] {
		describeFormat("TLP", GraphIO::readTLP, GraphIO::writeTLP, false);
		describeGAFormat("TLP", GraphIO::readTLP, GraphIO::writeTLP, false,
		                 GraphAttributes::nodeGraphics | GraphAttributes::edgeLabel | GraphAttributes::nodeLabel
		                | GraphAttributes::threeD | GraphAttributes::nodeStyle, {}, false);
	});
}

void describeDL() {
	describe("DL", [] {
		describeFormat("DL", GraphIO::readDL, GraphIO::writeDL, false);
		describeGAFormatPerEdgeWeightType("DL", GraphIO::readDL, GraphIO::writeDL, false, GraphAttributes::nodeLabel, {}, false);
	});
}

void describeSTPasGraphFormat() {
	describe("only graph", [] {
		auto writeSTP = [](const Graph& G, ostream& os) {
			// we copy G into an edge-weighted graph...
			// Note that we lose non-continuous indices here (which is bad in case we test this)
			NodeArray<node> copy{G};
			EdgeWeightedGraph<int> wG;
			for (node v : G.nodes) {
				copy[v] = wG.newNode();
			}
			for (edge e : G.edges) {
				wG.newEdge(copy[e->source()], copy[e->target()], 1);
			}
			return GraphIO::writeSTP(wG, {}, os);
		};

		describeFormat("STP", static_cast<GraphIO::ReaderFunc>(GraphIO::readSTP), writeSTP, false, {}, true);
	});
}

template<typename T>
void describeSTPasInstanceFormat(const string &typeName) {
	describe("Steiner tree instances with weights of type " + typeName, [] {
		string itDesc[] = {"stores and loads an undirected instance of size ",
			"stores and loads a directed instance of size "};
		for (int dir = 0; dir < 2; dir++) {
			bool directed = static_cast<bool>(dir);
			for (int i = 4; i < 1024; i *= 2) {
				it(itDesc[dir] + to_string(i), [&] {
					std::ostringstream writeStream;

					long attrflags = GraphIO::getEdgeWeightFlag<T>() | GraphAttributes::nodeGraphics | GraphAttributes::threeD;
					Graph graph;
					GraphAttributes attr(graph, attrflags);
					attr.directed() = directed;
					List<node> terminals;
					NodeArray<bool> isTerminal(graph, false);

					randomSimpleGraph(graph, i, (i*(i-1))/2);
					int n = 1;
					for (node v : graph.nodes) {
						if(randomDouble(0, 1) > .5) {
							terminals.pushBack(v);
							isTerminal(v) = true;
							attr.shape(v) = Shape::Rect;
						} else {
							attr.shape(v) = Shape::Ellipse;
						}
						attr.x(v) = n++; // assertEqualGAs uses indices encoded in x to establish node mapping
						attr.y(v) = randomDouble(-100, 100);
						attr.z(v) = randomDouble(-100, 100);
					}
					if (!terminals.empty() && directed) {
						attr.shape(terminals.front()) = Shape::Triangle;
					}
					for (edge e : graph.edges) {
						GraphIO::getEdgeWeightAttribute<T>(attr, e) = randomDouble(0, 1000);
					}

					string myComment = "";
					if (randomDouble(0, 1) > .5) {
						myComment += "Name \"MyRandomInstance\"\n";
						myComment += "Creator \"Tilo Wiedera\"\n";
					}
					AssertThat(GraphIO::writeSTP(attr, terminals, writeStream, myComment), Equals(true));

					Graph readGraph;
					GraphAttributes readAttr(readGraph, GraphIO::getEdgeWeightFlag<T>());
					List<node> readTerminals;
					NodeArray<bool> readIsTerminal;

					std::istringstream readStream(writeStream.str());
					AssertThat(GraphIO::readSTP(readAttr, readGraph, readTerminals, readIsTerminal, readStream), Equals(true));

					AssertThat(readGraph.numberOfNodes(), Equals(graph.numberOfNodes()));
					AssertThat(readGraph.numberOfEdges(), Equals(graph.numberOfEdges()));
					AssertThat(readTerminals.size(), Equals(terminals.size()));
					assertEqualGAs(attr, readAttr, true);
					for(node v : readGraph.nodes) {
						AssertThat(readIsTerminal[v], Equals(readTerminals.search(v).valid()));
					}
				});
			}
		}

		it("clears the graph", [&](){
			EdgeWeightedGraph<T> writeGraph;
			List<node> terminals;
			std::ostringstream write;
			AssertThat(GraphIO::writeSTP(writeGraph, terminals, write), Equals(true));

			EdgeWeightedGraph<T> readGraph;
			customGraph(readGraph, 2, {{0, 1}});
			NodeArray<bool> isTerminal(readGraph, true);
			terminals.pushBack(readGraph.firstNode());
			std::istringstream read(write.str());
			AssertThat(GraphIO::readSTP(readGraph, terminals, isTerminal, read), Equals(true));
			AssertThat(readGraph.empty(), IsTrue());
			AssertThat(terminals.empty(), IsTrue());
			AssertThat(isTerminal.begin(), Equals(isTerminal.end()));
		});

		for_each_file("fileformats/stp/valid", [&](const ResourceFile* file){
			it("successfully parses " + file->fullPath(), [&] {
				EdgeWeightedGraph<T> graph;
				List<node> terminals;
				NodeArray<bool> isTerminal;
				stringstream is{file->data()};
				AssertThat(GraphIO::readSTP(graph, terminals, isTerminal, is), IsTrue());

				AssertThat(graph.numberOfNodes(), IsGreaterThan(0));
				AssertThat(graph.numberOfEdges(), IsGreaterThan(0));
				AssertThat(terminals.size(), IsGreaterThan(0));

				int terminalCounter = 0;
				for(node v : graph.nodes) {
					terminalCounter += isTerminal[v];
				}

				AssertThat(terminalCounter, Equals(terminals.size()));
			});
		});

		for_each_file("fileformats/stp/invalid", [&](const ResourceFile* file){
			it("detects errors in " + file->fullPath(), [&](){
				EdgeWeightedGraph<T> graph;
				List<node> terminals;
				NodeArray<bool> isTerminal;
				stringstream is{file->data()};
				AssertThat(GraphIO::readSTP(graph, terminals, isTerminal, is), IsFalse());
			});
		});
	});
}

void describeSTP() {
	describe("STP", [] {
		describeSTPasGraphFormat();
		describeSTPasInstanceFormat<int>("int");
		describeSTPasInstanceFormat<double>("double");
	});
}

void describeDMFasGraphFormat() {
	describe("only graph", [] {
		Writer writer = [](const Graph& G, ostream& os) {
			EdgeArray<int> weights{G, 1};
			return GraphIO::writeDMF(G, weights, G.firstNode(), G.lastNode(), os);
		};

		describeFormat("DMF", static_cast<GraphIO::ReaderFunc>(GraphIO::readDMF), writer, false, {}, 2);
	});
}

template<typename T>
void describeDMFasInstanceFormat(const string &typeName) {
	describe("Maximum flow instance with capacities of type " + typeName, [] {
		for_each_file("fileformats/dmf/valid", [&](const ResourceFile* file) {
			it("reads " + file->fullPath(), [&]() {
				Graph graph;
				EdgeArray<T> weights;
				node source;
				node sink;

				stringstream is{file->data()};
				AssertThat(GraphIO::readDMF(graph, weights, source, sink, is), IsTrue());
				AssertThat(graph.numberOfNodes(), IsGreaterThan(1));
				AssertThat(weights.valid(), IsTrue());
				AssertThat(source, !IsNull());
				AssertThat(sink, !IsNull());
#ifdef OGDF_DEBUG
				AssertThat(source->graphOf(), Equals(&graph));
				AssertThat(sink->graphOf(), Equals(&graph));
#endif
				AssertThat(source, !Equals(sink));

				for(edge e : graph.edges) {
					AssertThat(weights(e) > 0, IsTrue());
				}
			});
		});

		for_each_file("fileformats/dmf/invalid", [&](const ResourceFile* file) {
			it("reads " + file->fullPath(), [&]() {
				Graph graph;
				EdgeArray<T> weights(graph, 0);
				node source;
				node sink;
				stringstream is{file->data()};
				AssertThat(GraphIO::readDMF(graph, weights, source, sink, is), IsFalse());
			});
		});

		it("writes and reads a random graph", [&]() {
			Graph graph;
			EdgeArray<T> weights(graph, 0);
			node source;
			node sink;

			randomGraph(graph, 42, 189);
			source = graph.chooseNode();
			sink = graph.chooseNode([&](node v) { return v != source; });

			T sum = 0;
			for(edge e : graph.edges) {
				T cap = static_cast<T>(randomDoubleNormal(10, 5));
				if(cap < 0) {
					cap *= -1;
				}
				weights(e) = cap;
				sum += cap;
			}

			std::ostringstream writeStream;

			AssertThat(GraphIO::writeDMF(graph, weights, source, sink, writeStream), IsTrue());

			Graph readGraph;
			EdgeArray<T> readWeights(readGraph, 0);
			node readSource = nullptr;
			node readSink = nullptr;

			std::istringstream readStream(writeStream.str());
			AssertThat(GraphIO::readDMF(readGraph, readWeights, readSource, readSink, readStream), IsTrue());

			AssertThat(readGraph.numberOfNodes(), Equals(graph.numberOfNodes()));
			AssertThat(readGraph.numberOfEdges(), Equals(graph.numberOfEdges()));
			AssertThat(readSource, !IsNull());
			AssertThat(readSink, !IsNull());
#ifdef OGDF_DEBUG
				AssertThat(readSource->graphOf(), Equals(&readGraph));
				AssertThat(readSink->graphOf(), Equals(&readGraph));
#endif
			AssertThat(readSource->degree(), Equals(source->degree()));
			AssertThat(readSink->degree(), Equals(sink->degree()));

			T readSum = 0;
			for(edge e : readGraph.edges) {
				readSum += readWeights(e);
			}

			EpsilonTest eps(1.0e-3);
			AssertThat(eps.equal(sum, readSum), IsTrue());
		});

		it("clears the graph", [&]() {
			Graph writeGraph;
			EdgeArray<T> writeWeights(writeGraph, 42);
			completeGraph(writeGraph, 3);
			node source = writeGraph.firstNode();
			node sink = writeGraph.lastNode();

			std::ostringstream write;
			AssertThat(GraphIO::writeDMF(writeGraph, writeWeights, source, sink, write), IsTrue());

			Graph readGraph;
			EdgeArray<T> readWeights(readGraph, 0);
			customGraph(readGraph, 2, {{0, 1}});
			source = nullptr;
			sink = nullptr;

			std::istringstream read(write.str());
			AssertThat(GraphIO::readDMF(readGraph, readWeights, source, sink, read), IsTrue());
			AssertThat(readGraph.numberOfNodes(), Equals(3));
			AssertThat(readGraph.numberOfEdges(), Equals(3));
			AssertThat(readWeights[readGraph.firstEdge()], Equals(42));
			AssertThat(source, !Equals(sink));
			AssertThat(source, !IsNull());
			AssertThat(sink, !IsNull());
		});
	});
}

void describeDMF() {
	describe("DMF", [] {
		describeDMFasGraphFormat();
		describeDMFasInstanceFormat<int>("int");
		describeDMFasInstanceFormat<double>("double");
	});
}

void describeSpecificFormats() {
	// Use the same order as in GraphIO.h
	describeGML();
	describeRome();
	describeLEDA();
	describeChaco();
	describePMDissGraph();
	describeYGraph();
	describeGraph6();
	describeMatrixMarket();
	describeRudy();
	// TODO: BENCH (only very restrictive reader; point-based expansion of a hypergraph)
	// TODO: PLA (only very restrictive reader; point-based expansion of a hypergraph)
	// TODO: ChallengeGraph (Graph and GridLayout)
	describeGraphML();
	describeDOT();
	describeGEXF();
	describeGDF();
	describeTLP();
	describeDL();
	describeSTP();
	describeDMF();
}

void describeGenericReader() {
	describe("generic reader", [] {
		auto genericTest = [](const ResourceFile* file, bool result) {
			it((result ? "parses " : "does not recognize ") + file->fullPath(), [&]() {
				Graph graph;
				stringstream ss{file->data()};
				AssertThat(GraphIO::read(graph, ss), Equals(result));
			});
		};

		auto genericTestTrue = [&](const ResourceFile* file) { genericTest(file, true); };
		auto genericTestFalse = [&](const ResourceFile* file) { genericTest(file, false); };

		for_each_file("fileformats/gml/valid", genericTestTrue);
		for_each_file("fileformats/gml/invalid", genericTestFalse);

		for_each_file("fileformats/chaco/valid", genericTestTrue);
		for_each_file("fileformats/chaco/invalid", genericTestFalse);

		for_each_file("fileformats/dl/valid", genericTestTrue);
		for_each_file("fileformats/dl/invalid", genericTestFalse);

		for_each_file("fileformats/dot/valid", genericTestTrue);
		for_each_file("fileformats/dot/invalid", genericTestFalse);

		for_each_file("fileformats/gdf/valid", genericTestTrue);

		for_each_file("fileformats/gexf/valid", genericTestTrue);

		for_each_file("fileformats/graphml/valid", genericTestTrue);

		for_each_file("fileformats/leda/valid", genericTestTrue);
		for_each_file("fileformats/leda/invalid", genericTestFalse);

		for_each_file("fileformats/tlp/valid", genericTestTrue);
		for_each_file("fileformats/tlp/invalid", genericTestFalse);

		for_each_file("fileformats/stp/valid", genericTestTrue);

		for_each_file("fileformats/graph6/valid", genericTestTrue);

		for_each_file("fileformats/digraph6/valid", genericTestTrue);

		for_each_file("fileformats/sparse6/valid", genericTestTrue);

		for_each_file("fileformats/dmf/invalid", genericTestFalse);
	});

	describe("generic reader with GraphAttributes", [] {
		auto genericTest = [](const ResourceFile* file, bool result) {
			it((result ? "parses " : "does not recognize ") + file->fullPath(), [&]() {
				Graph graph;
				GraphAttributes attr(graph, GraphAttributes::all);
				stringstream ss{file->data()};
				AssertThat(GraphIO::read(attr, graph, ss), Equals(result));
			});
		};

		auto genericTestTrue = [&](const ResourceFile* file) { genericTest(file, true); };
		auto genericTestFalse = [&](const ResourceFile* file) { genericTest(file, false); };

		for_each_file("fileformats/gml/valid", genericTestTrue);
		for_each_file("fileformats/gml/invalid", genericTestFalse);

		for_each_file("fileformats/dl/valid", genericTestTrue);
		for_each_file("fileformats/dl/invalid", genericTestFalse);

		for_each_file("fileformats/dot/valid", genericTestTrue);
		for_each_file("fileformats/dot/invalid", genericTestFalse);

		for_each_file("fileformats/gdf/valid", genericTestTrue);

		for_each_file("fileformats/gexf/valid", genericTestTrue);

		for_each_file("fileformats/graphml/valid", genericTestTrue);

		for_each_file("fileformats/tlp/valid", genericTestTrue);
		for_each_file("fileformats/tlp/invalid", genericTestFalse);

		for_each_file("fileformats/stp/valid", genericTestTrue);

		for_each_file("fileformats/dmf/invalid", genericTestFalse);
	});
}

void describeGenericWriter() {
	describe("generic writer", [] {
		auto fileExists = [](const string &filename) {
			std::fstream fs(filename);
			return fs.good();
		};

		describe("writing graphs in the correct format", [&]() {
			std::vector<string> autoReadExtensions = {
				"gml",
				"leda",
				"gw",
				"chaco",
				"pm",
				"pmd",
				"g6",
				"d6",
				"s6",
				"graphml",
				"dot",
				"gv",
				"gefx",
				"gdf",
				"tlp",
				"dl"
			};

			Graph out;
			randomTree(out, 50);
			auto writeAndRead = [&out](const string &filename, GraphIO::ReaderFunc reader = nullptr) {
				Graph in;
				AssertThat(GraphIO::write(out, filename), IsTrue());
				std::ifstream read(filename);
				if (reader == nullptr) {
					AssertThat(GraphIO::read(in, read), IsTrue());
				} else {
					AssertThat(reader(in, read), IsTrue());
				}
				assertSeemsEqual(out, in);
				std::remove(filename.c_str());
			};

			for (auto ext : autoReadExtensions) {
				string filename = "mygraph." + ext;
				it("handles " + ext, [&]() {
					writeAndRead(filename);
				}, fileExists(filename));
			}

			string filename = "mygraph.rome";
			it("handles rome", [&]() {
				writeAndRead(filename, GraphIO::readRome);
			}, fileExists(filename));

			filename = "grafo42.50";
			it("handles grafoX.Y (Rome graphs)", [&]() {
				writeAndRead(filename, GraphIO::readRome);
			}, fileExists(filename));
		});

		string filename = "mygraph.xxx";
		it("fails for an unknown file extension", [&filename]() {
			Graph out;
			AssertThat(GraphIO::write(out, filename), IsFalse());
		}, fileExists(filename));
	});
}

go_bandit([]() {
	describe("GraphIO", []() {
		describeSpecificFormats();
		describeGenericReader();
		describeGenericWriter();
	});
});
