/** \file
 * \brief Tests for ogdf::GraphAttributes.
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
#include <resources.h>

using GA = GraphAttributes;

/**
 * Tests getters and setter of an attribute.
 *
 * \param elemFunc Returns a list of elements which properties are to be tested.
 * \param refFunc Returns a non-const reference to the attribute (getter & setter to be tested).
 * \param constRefFunc Returns a copy of the attribute (second getter to be tested).
 * \param defaultValue Value that the attribute is supposed to be initialized to.
 * \param secondValue Differs from \p defaultValue, used for testing setters.
 * \param neededAttributes Attribute flags that are required to enable the attribute.
 * \param attributeName Human-readable name of the property. Used to create a title for the test.
 * \tparam Attribute Type of property to be read/written.
 * \tparam Element Type of the graph element that has the property.
 */
template<class Attribute, class Element>
void testAttribute(
	std::function<List<Element>(const Graph&)> elemFunc,
	std::function<Attribute& (GraphAttributes&, Element)> refFunc,
	std::function<Attribute (const GraphAttributes&, Element)> constRefFunc,
	Attribute defaultValue,
	Attribute secondValue,
	long neededAttributes,
	string attributeName)
{
	describe(attributeName, [&] {
		Graph graph;
		GraphAttributes attr(graph);
		GraphAttributes attrCopy;
		List<Element> elements;

		before_each([&] {
			completeGraph(graph, 7);
			attr.init(neededAttributes);
			attrCopy = attr;
			elements = elemFunc(graph);
		});

#ifdef OGDF_USE_ASSERT_EXCEPTIONS
		it("throws an exception on access if the attribute is disabled", [&] {
			attr.destroyAttributes(neededAttributes);
			AssertThrows(AssertionFailed, refFunc(attr, elements.front()));
		});
#endif

		it("gets the value", [&] {
			for(Element elem : elements) {
				AssertThat(constRefFunc(attr, elem), Equals(defaultValue));
				AssertThat(refFunc(attr, elem), Equals(defaultValue));
				AssertThat(constRefFunc(attrCopy, elem), Equals(defaultValue));
				AssertThat(refFunc(attrCopy, elem), Equals(defaultValue));
			}
		});

		it("sets the value", [&] {
			for(Element elem : elements) {
				Attribute &value = refFunc(attr, elem);
				value = secondValue;
				AssertThat(refFunc(attr, elem), Equals(secondValue));
				AssertThat(constRefFunc(attr, elem), Equals(secondValue));
				AssertThat(refFunc(attrCopy, elem), Equals(defaultValue));
				AssertThat(constRefFunc(attrCopy, elem), Equals(defaultValue));
			}
		});

		it("enables the attribute when enabling all", [&] {
			attr.init(GA::all);
			AssertThat(attr.has(neededAttributes), IsTrue());
		});
	});
}

//! @see testAttribute
template<class Attribute, class... Args>
void testNodeAttribute(Args... args) {
	testAttribute<Attribute, node>([](const Graph &graph) {
		List<node> result;
		graph.allNodes(result);
		return result;
	}, std::forward<Args>(args)...);
}

//! @see testAttribute
template<class Attribute, class... Args>
void testEdgeAttribute(Args... args) {
	testAttribute<Attribute, edge>([](const Graph &graph) {
		List<edge> result;
		graph.allEdges(result);
		return result;
	}, std::forward<Args>(args)...);
}

go_bandit([] {
describe("graph attributes", [] {
	it("initializes with no attributes by default", [] {
		GraphAttributes attr;
		AssertThat(attr.attributes(), Equals(0));
	});

	it("initializes with a graph and flags", [] {
		Graph graph;
		GraphAttributes attr(graph, GA::nodeId);
		AssertThat(&attr.constGraph(), Equals(&graph));
		AssertThat(attr.attributes(), Equals(GA::nodeId));
	});

	it("initializes with a graph", [] {
		Graph graph;
		GraphAttributes attr(graph);
		AssertThat(&attr.constGraph(), Equals(&graph));
		AssertThat(attr.attributes(), Equals(GA::nodeGraphics | GA::edgeGraphics));
	});

	it("initializes using explicit init", [] {
		Graph graph;
		GraphAttributes attr;
		attr.init(graph, GA::nodeId);
		AssertThat(&attr.constGraph(), Equals(&graph));
		AssertThat(attr.attributes(), Equals(GA::nodeId));
	});

	it("initializes using another GraphAttributes instance", [] {
		Graph graph;
		GraphAttributes attr(graph, GA::nodeId | GA::nodeGraphics);
		GraphAttributes attrCopy(attr);
		AssertThat(&attrCopy.constGraph(), Equals(&graph));
		AssertThat(attrCopy.attributes(), Equals(GA::nodeId | GA::nodeGraphics));
	});

	it("destroys its attributes", [] {
		Graph graph;
		GraphAttributes attr(graph, GA::nodeGraphics | GA::nodeLabel);
		AssertThat(&attr.constGraph(), Equals(&graph));
		AssertThat(attr.attributes(), Equals(GA::nodeGraphics | GA::nodeLabel));
		attr.destroyAttributes(GA::nodeGraphics | GA::nodeId);
		AssertThat(attr.attributes(), Equals(GA::nodeLabel));
	});

	it("adds new attributes", [] {
		Graph graph;
		GraphAttributes attr(graph, GA::nodeGraphics | GA::nodeLabel);
		AssertThat(&attr.constGraph(), Equals(&graph));
		AssertThat(attr.attributes(), Equals(GA::nodeGraphics | GA::nodeLabel));
		attr.addAttributes(GA::nodeId | GA::nodeLabel);
		AssertThat(attr.attributes(), Equals(GA::nodeGraphics | GA::nodeLabel | GA::nodeId));
	});

	it("knows its currently enabled attributes", [] {
		Graph graph;
		GraphAttributes attr(graph, GA::nodeId | GA::nodeLabel);
		AssertThat(attr.has(GA::nodeId | GA::nodeLabel), IsTrue());
		AssertThat(attr.has(GA::nodeId), IsTrue());
		AssertThat(attr.has(GA::nodeId | GA::nodeGraphics), IsFalse());
		AssertThat(attr.has(GA::nodeGraphics), IsFalse());
	});

	describe("attributes", [] {
		it("knows if it's directed", [] {
			Graph graph;
			GraphAttributes attr(graph);
			AssertThat(attr.directed(), IsTrue());
			attr.directed() = false;
			AssertThat(attr.directed(), IsFalse());
		});

		testNodeAttribute<double>(
			[](GraphAttributes &a, node v) -> double& { return a.x(v); },
			[](const GraphAttributes &a, node v) { return a.x(v); },
			0, 42,
			GA::nodeGraphics, "x");

		testNodeAttribute<double>(
			[](GraphAttributes &a, node v) -> double& { return a.xLabel(v); },
			[](const GraphAttributes &a, node v) { return a.xLabel(v); },
			0, 42,
			GA::nodeLabel | GA::nodeLabelPosition, "xLabel");

		testNodeAttribute<double>(
			[](GraphAttributes &a, node v) -> double& { return a.y(v); },
			[](const GraphAttributes &a, node v) { return a.y(v); },
			0, 42,
			GA::nodeGraphics, "y");

		testNodeAttribute<double>(
			[](GraphAttributes &a, node v) -> double& { return a.yLabel(v); },
			[](const GraphAttributes &a, node v) { return a.yLabel(v); },
			0, 42,
			GA::nodeLabel | GA::nodeLabelPosition, "yLabel");

		testNodeAttribute<double>(
			[](GraphAttributes &a, node v) -> double& { return a.z(v); },
			[](const GraphAttributes &a, node v) { return a.z(v); },
			0, 42,
			GA::nodeGraphics | GA::threeD, "z");

		testNodeAttribute<double>(
			[](GraphAttributes &a, node v) -> double& { return a.zLabel(v); },
			[](const GraphAttributes &a, node v) { return a.zLabel(v); },
			0, 42,
			GA::nodeLabel | GA::nodeLabelPosition | GA::threeD | GA::nodeGraphics, "zLabel");

		testNodeAttribute<double>(
			[](GraphAttributes &a, node v) -> double& { return a.width(v); },
			[](const GraphAttributes &a, node v) { return a.width(v); },
			20, 42,
			GA::nodeGraphics, "width of a node");

		testNodeAttribute<int>(
			[](GraphAttributes &a, node v) -> int& { return a.weight(v); },
			[](const GraphAttributes &a, node v) { return a.weight(v); },
			0, 42,
			GA::nodeWeight, "weight of a node");

		testEdgeAttribute<Graph::EdgeType>(
			[](GraphAttributes &a, edge e) -> Graph::EdgeType& { return a.type(e); },
			[](const GraphAttributes &a, edge e) { return a.type(e); },
			Graph::EdgeType::association, Graph::EdgeType::generalization,
			GA::edgeType, "type of an edge");

		testNodeAttribute<Graph::NodeType>(
			[](GraphAttributes &a, node v) -> Graph::NodeType& { return a.type(v); },
			[](const GraphAttributes &a, node v) { return a.type(v); },
			Graph::NodeType::vertex, Graph::NodeType::dummy,
			GA::nodeType, "type of a node");

		testEdgeAttribute<uint32_t>(
			[](GraphAttributes &a, edge e) -> uint32_t& { return a.subGraphBits(e); },
			[](const GraphAttributes &a, edge e) { return a.subGraphBits(e); },
			0, 42,
			GA::edgeSubGraphs, "SubGraphBits");

		testEdgeAttribute<float>(
			[](GraphAttributes &a, edge e) -> float& { return a.strokeWidth(e); },
			[](const GraphAttributes &a, edge e) { return a.strokeWidth(e); },
			LayoutStandards::defaultEdgeStroke().m_width, 42,
			GA::edgeStyle | GA::edgeGraphics, "strokeWidth edge");

		testNodeAttribute<float>(
			[](GraphAttributes &a, node v) -> float& { return a.strokeWidth(v); },
			[](const GraphAttributes &a, node v) { return a.strokeWidth(v); },
			LayoutStandards::defaultEdgeStroke().m_width, 42,
			GA::nodeStyle | GA::nodeGraphics, "strokeWidth node");

		testNodeAttribute<StrokeType>(
			[](GraphAttributes &a, node v) -> StrokeType& { return a.strokeType(v); },
			[](const GraphAttributes &a, node v) { return a.strokeType(v); },
			LayoutStandards::defaultNodeStroke().m_type, StrokeType::Dot,
			GA::nodeStyle | GA::nodeGraphics, "strokeType node");

		testEdgeAttribute<StrokeType>(
			[](GraphAttributes &a, edge e) -> StrokeType& { return a.strokeType(e); },
			[](const GraphAttributes &a, edge e) { return a.strokeType(e); },
			LayoutStandards::defaultEdgeStroke().m_type, StrokeType::Dot,
			GA::edgeStyle | GA::edgeGraphics, "strokeType edge");

		testEdgeAttribute<Color>(
			[](GraphAttributes &a, edge e) -> Color& { return a.strokeColor(e); },
			[](const GraphAttributes &a, edge e) { return a.strokeColor(e); },
			LayoutStandards::defaultEdgeStroke().m_color, ogdf::Color::Name::Turquoise,
			GA::edgeStyle | GA::edgeGraphics, "strokeColor edge");

		testNodeAttribute<Color>(
			[](GraphAttributes &a, node v) -> Color& { return a.strokeColor(v); },
			[](const GraphAttributes &a, node v) { return a.strokeColor(v); },
			LayoutStandards::defaultEdgeStroke().m_color, ogdf::Color::Name::Turquoise,
			GA::nodeStyle | GA::nodeGraphics, "strokeColor node");

		testNodeAttribute<Shape>(
			[](GraphAttributes &a, node v) -> Shape& { return a.shape(v); },
			[](const GraphAttributes &a, node v) { return a.shape(v); },
			LayoutStandards::defaultNodeShape(), Shape::Rect,
			GA::nodeGraphics, "shape node");

		testEdgeAttribute<EdgeArrow>(
			[](GraphAttributes &a, edge e) -> EdgeArrow& { return a.arrowType(e); },
			[](const GraphAttributes &a, edge e) { return a.arrowType(e); },
			LayoutStandards::defaultEdgeArrow(), EdgeArrow::Both,
			GA::edgeArrow, "arrowType");

		testEdgeAttribute<double>(
			[](GraphAttributes &a, edge e) -> double& { return a.doubleWeight(e); },
			[](const GraphAttributes &a, edge e) { return a.doubleWeight(e); },
			1.0, 42.0,
			GA::edgeDoubleWeight, "doubleWeight");

		testNodeAttribute<Color>(
			[](GraphAttributes &a, node v) -> Color& { return a.fillBgColor(v); },
			[](const GraphAttributes &a, node v) { return a.fillBgColor(v); },
			LayoutStandards::defaultNodeFill().m_bgColor, ogdf::Color::Name::Turquoise,
			GA::nodeStyle | GA::nodeGraphics, "fillBgColor");

		testNodeAttribute<Color>(
			[](GraphAttributes &a, node v) -> Color& { return a.fillColor(v); },
			[](const GraphAttributes &a, node v) { return a.fillColor(v); },
			LayoutStandards::defaultNodeFill().m_color,  Color(ogdf::Color::Name::Turquoise),
			GA::nodeStyle | GA::nodeGraphics, "fillColor");

		testNodeAttribute<FillPattern>(
			[](GraphAttributes &a, node v) -> FillPattern& { return a.fillPattern(v); },
			[](const GraphAttributes &a, node v) { return a.fillPattern(v); },
			LayoutStandards::defaultNodeFill().m_pattern, FillPattern::Cross,
			GA::nodeStyle | GA::nodeGraphics, "fillPattern");

		testNodeAttribute<int>(
			[](GraphAttributes &a, node v) -> int& { return a.idNode(v); },
			[](const GraphAttributes &a, node v) { return a.idNode(v); },
			-1, 42,
			GA::nodeId, "idNode");

		describe("advanced", [] {
			Graph graph;
			GraphAttributes attr(graph);
			const GraphAttributes &constAttr = attr;

			before_each([&] {
				completeGraph(graph, 7);
			});

			it("(in|add|remove)SubGraph", [&] {
				edge e = graph.chooseEdge();
	#ifdef OGDF_USE_ASSERT_EXCEPTIONS
				AssertThrows(AssertionFailed, attr.inSubGraph(e, 13));
	#endif
				attr.init(GA::edgeSubGraphs);
				AssertThat(constAttr.inSubGraph(e, 13), IsFalse());
				AssertThat(attr.inSubGraph(e, 13), IsFalse());
				attr.addSubGraph(e, 13);
				AssertThat(constAttr.inSubGraph(e, 13), IsTrue());
				AssertThat(attr.inSubGraph(e, 13), IsTrue());
				attr.removeSubGraph(e, 13);
				AssertThat(constAttr.inSubGraph(e, 13), IsFalse());
				AssertThat(attr.inSubGraph(e, 13), IsFalse());
			});

			it("assigns width using a NodeArray", [&] {
	#ifdef OGDF_USE_ASSERT_EXCEPTIONS
				attr.destroyAttributes(GA::nodeGraphics);
				AssertThrows(AssertionFailed, attr.width());
	#endif
				attr.init(GA::nodeGraphics);
				node v = graph.chooseNode();
				NodeArray<double> widthNA(graph, 42);
				AssertThat(constAttr.width().graphOf(), Equals(&graph));
				AssertThat(attr.width().graphOf(), Equals(&graph));
				AssertThat(constAttr.width()[v], Equals(LayoutStandards::defaultNodeWidth()));
				AssertThat(attr.width()[v], Equals(LayoutStandards::defaultNodeWidth()));
				attr.width() = widthNA;
				AssertThat(constAttr.width()[v], Equals(42));
				AssertThat(attr.width()[v], Equals(42));
				attr.setAllWidth(1337);
				AssertThat(constAttr.width(v), Equals(1337));
				AssertThat(attr.width(v), Equals(1337));
			});

			testNodeAttribute<double>(
					[](GraphAttributes &a, node v) -> double& { return a.height(v); },
					[](const GraphAttributes &a, node v) { return a.height(v); },
					20, 42,
					GA::nodeGraphics, "height of a node");

			it("assigns height using a NodeArray", [&] {
	#ifdef OGDF_USE_ASSERT_EXCEPTIONS
				attr.destroyAttributes(GA::nodeGraphics);
				AssertThrows(AssertionFailed, attr.height());
	#endif
				attr.init(GA::nodeGraphics);
				node v = graph.chooseNode();
				NodeArray<double> heightNA(graph, 42);
				AssertThat(constAttr.height().graphOf(), Equals(&graph));
				AssertThat(attr.height().graphOf(), Equals(&graph));
				attr.height() = heightNA;
				AssertThat(constAttr.height()[v], Equals(42));
				AssertThat(attr.height()[v], Equals(42));
				attr.setAllHeight(1337);
				AssertThat(constAttr.height(v), Equals(1337));
				AssertThat(attr.height(v), Equals(1337));
			});
		});

		testEdgeAttribute<int>(
			[](GraphAttributes &a, edge e) -> int& { return a.intWeight(e); },
			[](const GraphAttributes &a, edge e) { return a.intWeight(e); },
			1,  42,
			GA::edgeIntWeight, "intWeight");

		testNodeAttribute<string>(
			[](GraphAttributes &a, node v) -> string& { return a.label(v); },
			[](const GraphAttributes &a, node v) { return a.label(v); },
			"",  "ogdf" ,
			GA::nodeLabel, "label");

		testEdgeAttribute<string>(
			[](GraphAttributes &a, edge e) -> string& { return a.label(e); },
			[](const GraphAttributes &a, edge e) { return a.label(e); },
			"",  "ogdf" ,
			GA::edgeLabel, "label");


		testNodeAttribute<string>(
			[](GraphAttributes &a, node v) -> string& { return a.templateNode(v); },
			[](const GraphAttributes &a, node v) { return a.templateNode(v); },
			"",  "ogdf" ,
			GA::nodeTemplate, "templateNode");
	});

	describe("change position of elements", [] {
		GraphAttributes attr;
		Graph graph;

		before_each([&] {
			completeGraph(graph, 100);
			attr = GraphAttributes(graph, GA::nodeGraphics | GA::edgeGraphics);
			for(node v : graph.nodes) {
				attr.x(v) = ogdf::randomNumber(-100, 100);
				attr.y(v) = ogdf::randomNumber(-100, 100);
			}
			attr.addNodeCenter2Bends(1);
			attr.translateToNonNeg();
		});

		it("translates to non-negative coordinates", [&] {
			for(node v : graph.nodes) {
				AssertThat(attr.x(v) - attr.width(v) / 2, IsGreaterThan(0) || Equals(0));
				AssertThat(attr.y(v) - attr.width(v) / 2, IsGreaterThan(0) || Equals(0));
			}
			for(edge e : graph.edges) {
				for(DPoint &p : attr.bends(e)) {
					AssertThat(p.m_x, IsGreaterThan(0) || Equals(0));
					AssertThat(p.m_y, IsGreaterThan(0) || Equals(0));
				}
			}
		});

		it("translates", [&] {
			GraphAttributes ga = GraphAttributes(attr);
			attr.translate(1.0, 42.0);
			for(node v : graph.nodes) {
				AssertThat(attr.x(v), Equals(ga.x(v)+1.0));
				AssertThat(attr.y(v), Equals(ga.y(v)+42.0));
			}
			for(edge e : graph.edges) {
				DPolyline &bendpoints = ga.bends(e);
				for(DPoint &p_new : attr.bends(e)) {
					DPoint p_old = bendpoints.popFrontRet();
					AssertThat(p_new.m_x, Equals(p_old.m_x+1.0));
					AssertThat(p_new.m_y, Equals(p_old.m_y+42.0));
				}
			}
		});

		it("scales", [&] {
			GraphAttributes ga = GraphAttributes(attr);
			attr.scale(-1.0, -2.0, true);
			for(node v : graph.nodes) {
				AssertThat(attr.x(v), Equals(-ga.x(v)));
				AssertThat(attr.y(v), Equals(-2.0 * ga.y(v)));
			}
			for(edge e : graph.edges) {
				DPolyline &bendpoints = ga.bends(e);
				for(DPoint &p_new : attr.bends(e)) {
					DPoint p_old = bendpoints.popFrontRet();
					AssertThat(p_new.m_x, Equals(-p_old.m_x));
					AssertThat(p_new.m_y, Equals(-2.0 * p_old.m_y));
				}
			}
		});

		it("scales and then translates", [&] {
			GraphAttributes ga = GraphAttributes(attr);
			attr.scaleAndTranslate(-1.0, -42.0, 13, 37, true);
			for(node v : graph.nodes) {
				AssertThat(attr.x(v), Equals(-ga.x(v) + 13));
				AssertThat(attr.y(v), Equals(-42 * ga.y(v) + 37));
			}
			for(edge e : graph.edges) {
				DPolyline &bendpoints = ga.bends(e);
				for(DPoint &p_new : attr.bends(e)) {
					DPoint p_old = bendpoints.popFrontRet();
					AssertThat(p_new.m_x, Equals(-p_old.m_x + 13));
					AssertThat(p_new.m_y, Equals(-42 * p_old.m_y + 37));
				}
			}
		});

		it("flips vertical within its bounding box", [&] {
			DRect boundingBox = attr.boundingBox();
			double height = boundingBox.height();
			GraphAttributes ga = GraphAttributes(attr);
			attr.flipVertical();
			for(node v : graph.nodes) {
				AssertThat(attr.x(v), Equals(ga.x(v)));
				AssertThat(attr.y(v), Equals(height - ga.y(v)));
			}
			for(edge e : graph.edges) {
				DPolyline &bendpoints = ga.bends(e);
				for(DPoint &p_new : attr.bends(e)) {
					DPoint p_old = bendpoints.popFrontRet();
					AssertThat(p_new.m_x, Equals(p_old.m_x));
					AssertThat(p_new.m_y, Equals(height - p_old.m_y));
				}
			}
		});

		it("flips vertical with a given box", [&] {
			GraphAttributes ga = GraphAttributes(attr);
			attr.flipVertical(DRect());
			for(node v : graph.nodes) {
				AssertThat(attr.x(v), Equals(ga.x(v)));
				AssertThat(attr.y(v), Equals(-ga.y(v)));
			}
			for(edge e : graph.edges) {
				DPolyline &bendpoints = ga.bends(e);
				for(DPoint &p_new : attr.bends(e)) {
					DPoint p_old = bendpoints.popFrontRet();
					AssertThat(p_new.m_x, Equals(p_old.m_x));
					AssertThat(p_new.m_y, Equals(-p_old.m_y));
				}
			}
		});

		it("flips horizontal within its bounding box", [&] {
			DRect boundingBox = attr.boundingBox();
			double width = boundingBox.width();
			GraphAttributes ga = GraphAttributes(attr);
			attr.flipHorizontal();
			for(node v : graph.nodes) {
				AssertThat(attr.x(v), Equals(width - ga.x(v)));
				AssertThat(attr.y(v), Equals(ga.y(v)));
			}
			for(edge e : graph.edges) {
				DPolyline &bendpoints = ga.bends(e);
				for(DPoint &p_new : attr.bends(e)) {
					DPoint p_old = bendpoints.popFrontRet();
					AssertThat(p_new.m_x, Equals(width - p_old.m_x));
					AssertThat(p_new.m_y, Equals(p_old.m_y));
				}
			}
		});

		it("flips horizontal with a given box", [&] {
			GraphAttributes ga = GraphAttributes(attr);
			attr.flipHorizontal(DRect());
			for(node v : graph.nodes) {
				AssertThat(attr.x(v), Equals(-ga.x(v)));
				AssertThat(attr.y(v), Equals(ga.y(v)));
			}
			for(edge e : graph.edges) {
				DPolyline &bendpoints = ga.bends(e);
				for(DPoint &p_new : attr.bends(e)) {
					DPoint p_old = bendpoints.popFrontRet();
					AssertThat(p_new.m_x, Equals(-p_old.m_x));
					AssertThat(p_new.m_y, Equals(p_old.m_y));
				}
			}
		});

		it("rotates left", [&] {
			GraphAttributes ga = GraphAttributes(attr);
			attr.rotateLeft90();
			for(node v : graph.nodes) {
				AssertThat(attr.x(v), Equals(ga.y(v)));
				AssertThat(attr.y(v), Equals(-ga.x(v)));
			}
			for(edge e : graph.edges) {
				DPolyline &bendpoints = ga.bends(e);
				for(DPoint &p_new : attr.bends(e)) {
					DPoint p_old = bendpoints.popFrontRet();
					AssertThat(p_new.m_x, Equals(p_old.m_y));
					AssertThat(p_new.m_y, Equals(-p_old.m_x));
				}
			}
		});

		it("rotates right", [&] {
			GraphAttributes ga = GraphAttributes(attr);
			attr.rotateRight90();
			for(node v : graph.nodes) {
				AssertThat(attr.x(v), Equals(-ga.y(v)));
				AssertThat(attr.y(v), Equals(ga.x(v)));
			}
			for(edge e : graph.edges) {
				DPolyline &bendpoints = ga.bends(e);
				for(DPoint &p_new : attr.bends(e)) {
					DPoint p_old = bendpoints.popFrontRet();
					AssertThat(p_new.m_x, Equals(-p_old.m_y));
					AssertThat(p_new.m_y, Equals(p_old.m_x));
				}
			}
		});
	});

	it("knows its bounding box", [] {
		Graph graph;
		randomGraph(graph, 100, 1000);
		GraphAttributes attr(graph, GA::nodeGraphics | GA::edgeGraphics);
		for(node v : graph.nodes) {
			attr.x(v) = ogdf::randomNumber(-1000, 1000);
			attr.y(v) = ogdf::randomNumber(-1000, 1000);
		}
		attr.addNodeCenter2Bends(1);
		attr.translateToNonNeg();
		DRect boundBox = attr.boundingBox();
		AssertThat(boundBox.p1().m_x, Equals(0) || IsGreaterThan(0));
		AssertThat(boundBox.p1().m_y, Equals(0) || IsGreaterThan(0));
		AssertThat(boundBox.p2().m_x, Equals(2020) || IsLessThan(2020));
		AssertThat(boundBox.p2().m_y, Equals(2020) || IsLessThan(2020));
		for(node v : graph.nodes) {
			AssertThat(boundBox.contains(attr.point(v)), IsTrue());
		}
		for(edge e : graph.edges) {
			for(DPoint &p : attr.bends(e)) {
				AssertThat(boundBox.contains(p), IsTrue());
			}
		}
	});

	describe("bends", [] {
		Graph graph;
		GraphAttributes attr;

		before_each([&] {
			completeGraph(graph, 3);
			attr = GraphAttributes(graph, GA::nodeGraphics | GA::edgeGraphics);
		});

		it("clears all bends", [&] {
			attr.addNodeCenter2Bends(1);
			AssertThat(attr.bends(graph.chooseEdge()).size(), IsGreaterThan(0));
			attr.clearAllBends();
			for(edge e : graph.edges) {
				AssertThat(attr.bends(e).size(), Equals(0));
			}
		});

		it("knows its bends", [&] {
			for(edge e : graph.edges) {
				AssertThat(attr.bends(e).size(), Equals(0));
			}
			attr.addNodeCenter2Bends(0);
			for(edge e : graph.edges) {
				AssertThat(attr.bends(e).size(), Equals(2));
			}
			edge e = graph.chooseEdge();
			DPolyline dpl;
			dpl.emplaceFront(42, 17);
			attr.bends(e) = dpl;
			AssertThat(attr.bends(e).size(), Equals(1));
			AssertThat((*attr.bends(e).get(0)).m_x, Equals(42));
			AssertThat((*attr.bends(e).get(0)).m_y, Equals(17));
		});
	});

	it("can be transferred from a GraphCopy to the original", [] {
		Graph graph;
		completeGraph(graph, 5);
		GraphAttributes attr(graph);

		// Create GraphCopy with bend point and dummy node splitting first edge.
		GraphCopy copy(graph);
		GraphAttributes copyAttr(copy);
		DPoint bendPoint(42,42);
		edge firstCopyEdge = copy.copy(graph.firstEdge());
		copyAttr.bends(firstCopyEdge).pushBack(bendPoint);
		node dummy = copy.split(firstCopyEdge)->source();

		for (node vCopy : copy.nodes) {
			copyAttr.x(vCopy) = randomNumber(0,100);
			copyAttr.y(vCopy) = randomNumber(0,100);
		}
		copyAttr.transferToOriginal(attr);

		for (node v : graph.nodes) {
			AssertThat(attr.x(v), Equals(copyAttr.x(copy.copy(v))));
			AssertThat(attr.y(v), Equals(copyAttr.y(copy.copy(v))));
		}

		// Both the bend point and the dummy node are transferred as bend points.
		DPolyline dpl = attr.bends(graph.firstEdge());
		AssertThat(dpl.size(), Equals(2));
		AssertThat(dpl.front(), Equals(bendPoint));
		AssertThat(dpl.back(), Equals(copyAttr.point(dummy)));
	});

	it("can be transferred from the original to a GraphCopy", [] {
		Graph graph;
		completeGraph(graph, 5);
		GraphAttributes attr(graph, GraphAttributes::nodeGraphics |
				GraphAttributes::edgeGraphics | GraphAttributes::edgeStyle);
		DPoint bendPoint(42,42);
		attr.bends(graph.firstEdge()).pushBack(bendPoint);
		attr.strokeWidth(graph.firstEdge()) = 7;

		// Create GraphCopy with dummy node splitting first edge.
		GraphCopy copy(graph);
		GraphAttributes copyAttr(copy, GraphAttributes::nodeGraphics |
				GraphAttributes::edgeGraphics | GraphAttributes::edgeStyle);
		node dummy = copy.split(copy.copy(graph.firstEdge()))->source();

		// Node attributes are only transferred to non-dummy nodes.
		for (node vOrig : graph.nodes) {
			attr.x(vOrig) = randomNumber(0,100);
			attr.y(vOrig) = randomNumber(0,100);
		}
		for (node vCopy : copy.nodes) {
			copyAttr.x(vCopy) = 200;
			copyAttr.y(vCopy) = 200;
		}
		attr.transferToCopy(copyAttr);

		for (node vCopy : copy.nodes) {
			if (!copy.isDummy(vCopy)) {
				AssertThat(copyAttr.x(vCopy), Equals(attr.x(copy.original(vCopy))));
				AssertThat(copyAttr.y(vCopy), Equals(attr.y(copy.original(vCopy))));
			}
		}
		AssertThat(copyAttr.x(dummy), Equals(200));
		AssertThat(copyAttr.y(dummy), Equals(200));

		// Bends are only transferred to the first edge in the chain, other
		// attributes are transferred to all edges in the chain.
		const List<edge> &chain = copy.chain(graph.firstEdge());
		AssertThat(copyAttr.bends(chain.front()).size(), Equals(1));
		AssertThat(copyAttr.bends(chain.front()).front(), Equals(bendPoint));
		AssertThat(copyAttr.bends(chain.back()).empty(), IsTrue());
		AssertThat(copyAttr.strokeWidth(chain.front()), Equals(7));
		AssertThat(copyAttr.strokeWidth(chain.back()), Equals(7));
	});

	it("is not changed during transfers with disjoint attributes", [] {
		Graph graph;
		customGraph(graph, 1, {{0,0}});
		GraphAttributes attr(graph, GraphAttributes::nodeLabel);
		attr.label(graph.firstNode()) = "node";

		GraphCopy copy(graph);
		GraphAttributes copyAttr(copy, GraphAttributes::edgeLabel);
		copyAttr.label(copy.firstEdge()) = "edge";

		auto assertNothingChanged = [&]() {
			AssertThat(attr.label(graph.firstNode()), Equals("node"));
			AssertThat(copyAttr.label(copy.firstEdge()), Equals("edge"));
#ifdef OGDF_USE_ASSERT_EXCEPTIONS
			AssertThrows(AssertionFailed, attr.label(graph.firstEdge()));
			AssertThrows(AssertionFailed, copyAttr.label(copy.firstNode()));
#endif
		};

		assertNothingChanged();
		attr.transferToCopy(copyAttr);
		assertNothingChanged();
		copyAttr.transferToOriginal(attr);
		assertNothingChanged();
	});
});
});
