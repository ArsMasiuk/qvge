/** \file
 * \brief Tests for the ogdf::SvgPrinter
 *
 * \author Tilo Wiedera
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

#include <regex>
#include <ogdf/lib/pugixml/pugixml.h>
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/basic/graph_generators.h>
#include <testing.h>

void createDocument(GraphAttributes attr, pugi::xml_document &doc, GraphIO::SVGSettings *settings = nullptr, bool reassignPositions = true) {
	std::ostringstream write;

	if(reassignPositions) {
		int i = 0;
		for(node v : attr.constGraph().nodes) {
			attr.x(v) = attr.y(v) = i++ * 100;
			attr.width(v) = attr.height(v) = 10;
		}
	}

	if(settings == nullptr) {
		GraphIO::drawSVG(attr, write);
	} else {
		GraphIO::drawSVG(attr, write, *settings);
	}

	pugi::xml_parse_result result =  doc.load_string(write.str().c_str());
	AssertThat((bool) result, IsTrue());
}

go_bandit([](){
describe("GraphIO", []() {
describe("SVG", []() {
	std::unique_ptr<Graph> graph;
	int numberOfNodes = 42;

	before_each([&](){
		graph.reset(new Graph);
		randomBiconnectedGraph(*graph, numberOfNodes, 3*numberOfNodes);
	});

	it("is well-formed", [&]() {
		GraphAttributes attr(*graph);
		pugi::xml_document doc;
		createDocument(attr, doc);

		pugi::xml_node svg = doc.child("svg");
		AssertThat((bool) svg, IsTrue());
		AssertThat(svg.attribute("viewBox").empty(), IsFalse());

		AssertThat(static_cast<int>(svg.select_nodes("//rect").size()), Equals(graph->numberOfNodes()));
		AssertThat(static_cast<int>(svg.select_nodes("//path").size()), Equals(graph->numberOfEdges()));
	});

	it("supports 3D", [&]() {
		GraphAttributes attr(*graph,
				GraphAttributes::nodeGraphics |
				GraphAttributes::nodeStyle |
				GraphAttributes::edgeGraphics |
				GraphAttributes::threeD |
				GraphAttributes::nodeLabel |
				GraphAttributes::nodeLabelPosition);
		List<node> nodes;
		graph->allNodes(nodes);
		nodes.permute();
		int i = 0;

		for(node v : nodes) {
			attr.fillColor(v) = Color::Name::Gray;
			attr.x(v) = randomNumber(0, numberOfNodes*5);
			attr.y(v) = randomNumber(0, numberOfNodes*5);
			attr.label(v) = to_string(i);
			attr.z(v) = i++;
		}

		List<int> expected;
		for(i = 0; i < numberOfNodes; i++) {
			expected.pushBack(i);
		}

		pugi::xml_document doc;
		createDocument(attr, doc);
		pugi::xpath_node_set xmlNodes = doc.select_nodes("//text");
		AssertThat(static_cast<int>(xmlNodes.size()), Equals(graph->numberOfNodes()));

		for(pugi::xpath_node xmlNode : xmlNodes) {
			int index = xmlNode.node().text().as_int();
			AssertThat(expected.search(index).valid(), IsTrue());
			expected.removeFirst(index);
		}
	});

	it("respects the requested size", [&]() {
		GraphAttributes attr(*graph);
		GraphIO::SVGSettings settings;
		settings.width("100%");
		settings.height("700px");

		pugi::xml_document doc;
		createDocument(attr, doc, &settings);
		pugi::xml_node svg = doc.child("svg");

		AssertThat(std::string("100%") == svg.attribute("width").value(), IsTrue());
		AssertThat(std::string("700px") == svg.attribute("height").value(), IsTrue());
	});

	it("doesn't set a default size", [&]() {
		GraphAttributes attr(*graph);

		pugi::xml_document doc;
		createDocument(attr, doc);
		pugi::xml_node svg = doc.child("svg");

		AssertThat(svg.attribute("width").empty(), IsTrue());
		AssertThat(svg.attribute("height").empty(), IsTrue());
	});

	it("supports fill color", [&]() {
		GraphAttributes attr(*graph, GraphAttributes::nodeGraphics | GraphAttributes::nodeStyle);

		for(node v : graph->nodes) {
			attr.fillColor(v) = "#0ACDC0";
		}

		pugi::xml_document doc;
		createDocument(attr, doc);

		AssertThat(static_cast<int>(doc.select_nodes(".//*[@fill='#0ACDC0']").size()), Equals(graph->numberOfNodes()));
	});

	it("supports stroke color", [&]() {
		GraphAttributes attr(*graph, GraphAttributes::nodeGraphics | GraphAttributes::nodeStyle);

		for(node v : graph->nodes) {
			attr.strokeColor(v) = "#0ACDC0";
		}

		pugi::xml_document doc;
		createDocument(attr, doc);

		AssertThat(static_cast<int>(doc.select_nodes(".//*[@stroke='#0ACDC0']").size()), Equals(graph->numberOfNodes()));
	});

	it("sets the viewBox", [&]() {
		const double tolerance = 1;
		GraphAttributes attr(*graph, GraphAttributes::nodeGraphics | GraphAttributes::nodeStyle);

		GraphIO::SVGSettings settings;
		settings.margin(10);

		for(node v : graph->nodes) {
			double dx = attr.width(v)/2 + settings.margin() + tolerance;
			double dy = attr.height(v)/2 + settings.margin() + tolerance;
			attr.x(v) = randomDouble(dx, 100 - dx);
			attr.y(v) = randomDouble(dy, 100 - dy);
		}

		pugi::xml_document doc;
		createDocument(attr, doc, &settings, false);

		std::string viewBox = doc.child("svg").attribute("viewBox").value();
		std::stringstream ss(viewBox);

		double xmin, xmax, ymin, ymax;

		ss >> xmin;
		ss >> ymin;
		ss >> xmax;
		ss >> ymax;

		xmax += xmin;
		ymax += ymin;

		AssertThat(xmin, IsGreaterThan(0) || Equals(0));
		AssertThat(ymin, IsGreaterThan(0) || Equals(0));
		AssertThat(xmax, IsLessThan(100) || Equals(100));
		AssertThat(ymax, IsLessThan(100) || Equals(100));

		AssertThat(xmin, IsLessThan(xmax));
		AssertThat(ymin, IsLessThan(ymax));
	});

	it("draws clusters", [&]() {
		ClusterGraph clusterGraph(*graph);
		randomClusterGraph(clusterGraph, *graph, 10);
		ClusterGraphAttributes attr(clusterGraph, GraphAttributes::nodeGraphics | GraphAttributes::nodeStyle);

		for(node v : graph->nodes) {
			attr.shape(v) = Shape::Octagon;
		}

		std::ostringstream write;
		GraphIO::drawSVG(attr, write);
		pugi::xml_document doc;
		pugi::xml_parse_result result =  doc.load_string(write.str().c_str());

		AssertThat((bool) result, IsTrue());
		AssertThat(static_cast<int>(doc.select_nodes(".//rect").size()), Equals(clusterGraph.numberOfClusters() - 1));
	});

	for(bool directed : {true, false}) {
		it("supports arrow heads when directed=" + to_string(directed) + " but edge arrow attribute is disabled", [&]() {
			GraphAttributes attr(*graph, GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics);
			attr.directed() = directed;

			pugi::xml_document doc;
			createDocument(attr, doc);

			AssertThat(static_cast<int>(doc.select_nodes(".//polygon").size()), Equals(directed ? graph->numberOfEdges() : 0));
		});

		for(EdgeArrow ea : {EdgeArrow::Undefined, EdgeArrow::First, EdgeArrow::Last, EdgeArrow::Both, EdgeArrow::None}) {
			std::stringstream ss;
			ss << "supports arrow heads when directed=" << directed << " and type=";
			switch(ea) {
				case EdgeArrow::Undefined:
					ss << "UNDEFINED";
					break;
				case EdgeArrow::First:
					ss << "FIRST";
					break;
				case EdgeArrow::Last:
					ss << "LAST";
					break;
				case EdgeArrow::Both:
					ss << "BOTH";
					break;
				default:
					OGDF_ASSERT(ea == EdgeArrow::None);
					ss << "NONE";
		            break;
			}

			it(ss.str(), [&]() {
				GraphAttributes attr(*graph, GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics | GraphAttributes::edgeArrow);
				attr.directed() = directed;

				for(edge e : graph->edges) {
					attr.arrowType(e) = ea;
				}

				pugi::xml_document doc;
				createDocument(attr, doc);

				int expected = 0;
				if((directed && ea == EdgeArrow::Undefined) || ea == EdgeArrow::First || ea == EdgeArrow::Last) {
					expected = graph->numberOfEdges();
				}
				if(ea == EdgeArrow::Both) {
					expected = 2*graph->numberOfEdges();
				}
				AssertThat(static_cast<int>(doc.select_nodes(".//polygon").size()), Equals(expected));
			});
		}
	}
});
});
});
