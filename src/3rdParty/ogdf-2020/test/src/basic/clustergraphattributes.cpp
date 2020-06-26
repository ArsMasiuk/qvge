/** \file
 * \brief Tests for ogdf::ClusterGraphAttributes.
 *
 * \author Max Ilsen
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

#include <ogdf/basic/graph_generators.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>
#include <ogdf/layered/ExtendedNestingGraph.h>
#include <resources.h>

using GA = GraphAttributes;
using CGA = ClusterGraphAttributes;

/**
 * Tests getter and setter of an attribute.
 *
 * \param elemFunc Returns a list of elements whose properties are to be tested.
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
	std::function<List<Element>(const ClusterGraph&)> elemFunc,
	std::function<Attribute& (CGA&, Element)> refFunc,
	std::function<Attribute (const CGA&, Element)> constRefFunc,
	Attribute defaultValue,
	Attribute secondValue,
	long neededAttributes,
	string attributeName)
{
	describe(attributeName, [&] {
		Graph graph;
		ClusterGraph cGraph(graph);
		CGA attr(cGraph);
		List<Element> elements;

		before_each([&] {
			completeGraph(graph, 7);
			cGraph.init(graph);

			attr.init(neededAttributes);
			elements = elemFunc(cGraph);

			// Add one cluster for each node.
			for (node v : graph.nodes) {
				SList<node> nodes;
				nodes.pushBack(v);
				cGraph.createCluster(nodes);
			}
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
			}
		});

		it("sets the value", [&] {
			for(Element elem : elements) {
				Attribute &value = refFunc(attr, elem);
				value = secondValue;
				AssertThat(refFunc(attr, elem), Equals(secondValue));
				AssertThat(constRefFunc(attr, elem), Equals(secondValue));
			}
		});

		it("enables the attribute when enabling all", [&] {
			attr.init(CGA::all);
			AssertThat(attr.has(neededAttributes), IsTrue());
		});
	});
}

//! @see testAttribute
template<class Attribute, class... Args>
void testClusterAttribute(Args... args) {
	testAttribute<Attribute, cluster>([](const ClusterGraph &graph) {
		List<cluster> result;
		graph.allClusters(result);
		return result;
	}, std::forward<Args>(args)...);
}

go_bandit([] {
describe("ClusterGraphAttributes", [] {
	const long defaultAttrs = GA::edgeType | GA::nodeType | GA::nodeGraphics | GA::edgeGraphics;

	it("initializes with no attributes by default", [] {
		CGA attr;
		AssertThat(attr.attributes(), Equals(0));
	});

	it("initializes with a ClusterGraph and flags", [&defaultAttrs] {
		Graph graph;
		ClusterGraph cGraph(graph);
		CGA attr(cGraph, CGA::clusterGraphics);
		AssertThat(&attr.constClusterGraph(), Equals(&cGraph));
		AssertThat(attr.attributes(), Equals(defaultAttrs | CGA::clusterGraphics));
	});

	it("initializes with a ClusterGraph", [&defaultAttrs] {
		Graph graph;
		ClusterGraph cGraph(graph);
		CGA attr(cGraph);
		AssertThat(&attr.constClusterGraph(), Equals(&cGraph));
		AssertThat(attr.attributes(), Equals(defaultAttrs | CGA::clusterGraphics));
	});

	it("initializes using explicit init", [] {
		Graph graph;
		ClusterGraph cGraph(graph);
		CGA attr;
		attr.init(cGraph, CGA::clusterGraphics);
		AssertThat(&attr.constClusterGraph(), Equals(&cGraph));
		AssertThat(attr.attributes(), Equals(CGA::clusterGraphics));
	});

	it("destroys its attributes", [&defaultAttrs] {
		Graph graph;
		ClusterGraph cGraph(graph);
		CGA attr(cGraph, CGA::clusterGraphics | CGA::clusterLabel);
		AssertThat(&attr.constClusterGraph(), Equals(&cGraph));
		AssertThat(attr.attributes(), Equals(defaultAttrs | CGA::clusterGraphics | CGA::clusterLabel));
		attr.destroyAttributes(CGA::clusterGraphics | CGA::clusterTemplate);
		AssertThat(attr.attributes(), Equals(defaultAttrs | CGA::clusterLabel));
	});

	it("adds new attributes", [&defaultAttrs] {
		Graph graph;
		ClusterGraph cGraph(graph);
		CGA attr(cGraph, CGA::clusterGraphics | CGA::clusterLabel);
		AssertThat(&attr.constClusterGraph(), Equals(&cGraph));
		AssertThat(attr.attributes(), Equals(defaultAttrs | CGA::clusterGraphics | CGA::clusterLabel));
		attr.addAttributes(CGA::clusterTemplate | CGA::clusterLabel);
		AssertThat(attr.attributes(), Equals(defaultAttrs | CGA::clusterGraphics | CGA::clusterLabel | CGA::clusterTemplate));
	});

	it("knows its currently enabled attributes", [] {
		Graph graph;
		ClusterGraph cGraph(graph);
		CGA attr(cGraph, CGA::clusterGraphics | CGA::clusterLabel);
		AssertThat(attr.has(CGA::clusterGraphics | CGA::clusterLabel), IsTrue());
		AssertThat(attr.has(CGA::clusterGraphics), IsTrue());
		AssertThat(attr.has(CGA::clusterGraphics | CGA::clusterTemplate), IsFalse());
		AssertThat(attr.has(CGA::clusterTemplate), IsFalse());
	});

	describe("attributes", [] {
		testClusterAttribute<double>(
			[](CGA &a, cluster c) -> double& { return a.x(c); },
			[](const CGA &a, cluster c) { return a.x(c); },
			0, 42,
			CGA::clusterGraphics, "x");

		testClusterAttribute<double>(
			[](CGA &a, cluster c) -> double& { return a.y(c); },
			[](const CGA &a, cluster c) { return a.y(c); },
			0, 42,
			CGA::clusterGraphics, "y");

		testClusterAttribute<double>(
			[](CGA &a, cluster c) -> double& { return a.width(c); },
			[](const CGA &a, cluster c) { return a.width(c); },
			0, 42,
			CGA::clusterGraphics, "width");

		testClusterAttribute<double>(
			[](CGA &a, cluster c) -> double& { return a.height(c); },
			[](const CGA &a, cluster c) { return a.height(c); },
			0, 42,
			CGA::clusterGraphics, "height");

		testClusterAttribute<float>(
			[](CGA &a, cluster c) -> float& { return a.strokeWidth(c); },
			[](const CGA &a, cluster c) { return a.strokeWidth(c); },
			LayoutStandards::defaultClusterStroke().m_width, 42,
			CGA::clusterStyle | CGA::clusterGraphics, "strokeWidth");

		testClusterAttribute<StrokeType>(
			[](CGA &a, cluster c) -> StrokeType& { return a.strokeType(c); },
			[](const CGA &a, cluster c) { return a.strokeType(c); },
			LayoutStandards::defaultClusterStroke().m_type, StrokeType::Dot,
			CGA::clusterStyle | CGA::clusterGraphics, "strokeType");

		testClusterAttribute<Color>(
			[](CGA &a, cluster c) -> Color& { return a.strokeColor(c); },
			[](const CGA &a, cluster c) { return a.strokeColor(c); },
			LayoutStandards::defaultClusterStroke().m_color, ogdf::Color::Name::Turquoise,
			CGA::clusterStyle | CGA::clusterGraphics, "strokeColor");

		testClusterAttribute<Color>(
			[](CGA &a, cluster c) -> Color& { return a.fillBgColor(c); },
			[](const CGA &a, cluster c) { return a.fillBgColor(c); },
			LayoutStandards::defaultClusterFill().m_bgColor, ogdf::Color::Name::Turquoise,
			CGA::clusterStyle | CGA::clusterGraphics, "fillBgColor");

		testClusterAttribute<Color>(
			[](CGA &a, cluster c) -> Color& { return a.fillColor(c); },
			[](const CGA &a, cluster c) { return a.fillColor(c); },
			LayoutStandards::defaultClusterFill().m_color,  Color(ogdf::Color::Name::Turquoise),
			CGA::clusterStyle | CGA::clusterGraphics, "fillColor");

		testClusterAttribute<FillPattern>(
			[](CGA &a, cluster c) -> FillPattern& { return a.fillPattern(c); },
			[](const CGA &a, cluster c) { return a.fillPattern(c); },
			LayoutStandards::defaultClusterFill().m_pattern, FillPattern::Cross,
			CGA::clusterStyle | CGA::clusterGraphics, "fillPattern");

		testClusterAttribute<string>(
			[](CGA &a, cluster c) -> string& { return a.label(c); },
			[](const CGA &a, cluster c) { return a.label(c); },
			"", "42",
			CGA::clusterLabel, "label");

		testClusterAttribute<string>(
			[](CGA &a, cluster c) -> string& { return a.templateCluster(c); },
			[](const CGA &a, cluster c) { return a.templateCluster(c); },
			"", "42",
			CGA::clusterTemplate, "templateCluster");
	});
});
});
