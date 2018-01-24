/** \file
 * \brief Test helpers for layout algorithms
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

#pragma once

#include <iomanip>
#include <random>
#include <regex>

#include <ogdf/basic/DisjointSets.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/PriorityQueue.h>
#include <ogdf/module/LayoutModule.h>
#include <ogdf/basic/LayoutStatistics.h>

#include <ogdf/planarity/PlanarSubgraphCactus.h>
#include <ogdf/planarity/MaximalPlanarSubgraphSimple.h>

#include <graphs.h>

//#define OGDF_LAYOUT_HELPERS_PRINT_DRAWINGS

#define TEST_LAYOUT(TYPE, ...) describeLayout<TYPE>(string(#TYPE), 0, {__VA_ARGS__})

#ifdef OGDF_LAYOUT_HELPERS_PRINT_DRAWINGS
namespace layout_helpers {
	int drawingCounter = 0;
}
#endif

inline void getRandomLayout(GraphAttributes &GA)
{
	const Graph &G = GA.constGraph();
	double max_x = 2.0 * sqrt(G.numberOfNodes());
	double max_y = max_x;

	std::minstd_rand rng(randomSeed());
	std::uniform_real_distribution<> rand_x(0.0,max_x);
	std::uniform_real_distribution<> rand_y(0.0,max_y);

	for(node v : G.nodes) {
		GA.x(v) = rand_x(rng);
		GA.y(v) = rand_y(rng);
	}
}

//! Calls the layout algorithm \p L on \p G.
/**
 * Executes the layout algorithm, prints statistics and performs several assertions.
 *
 * @param name Name of the instance. Used only for debug output.
 * @param G Input graph.
 * @param L Algorithm to execute.
 * @param extraAttributes GraphAttribute flags that this algorithm requires (besides graphics and style).
 * @param algoPlanarizes Whether the algorithm planarizes non-planar graphs internally (i.e., produces drawings with a reasonable crossing number).
 * @param algoRequiresPlanar Whether the algorithm requires planar graphs (not necessarily embeddings) as input.
 * @param instanceIsPlanar Whether \p L is a planar graph.
 *
 **/
inline int64_t callLayout(const string& name, const Graph &G, LayoutModule &L, long extraAttributes, bool algoPlanarizes, bool algoRequiresPlanar, bool instanceIsPlanar)
{
	GraphAttributes GA(G, extraAttributes | GraphAttributes::nodeGraphics | GraphAttributes::nodeStyle | GraphAttributes::edgeGraphics | GraphAttributes::edgeStyle);
	getRandomLayout(GA);

	int64_t result;
	int64_t time;
	System::usedRealTime(time);
	L.call(GA);
	result = System::usedRealTime(time);

#ifdef OGDF_LAYOUT_HELPERS_PRINT_DRAWINGS
	double sumWidths = 0;
	double sumHeights = 0;

	GA.addAttributes(GraphAttributes::nodeLabel | GraphAttributes::edgeArrow);

	for (node v : G.nodes) {
		sumWidths += GA.width(v);
		sumHeights += GA.height(v);

		GA.fillColor(v) = Color::Name::Red;
		GA.strokeColor(v) = Color::Name::Black;
		GA.label(v) = to_string(v->index());
	}

	for(edge e : G.edges) {
		GA.strokeWidth(e) = 1;
		GA.strokeColor(e) = Color::Name::Blue;
		GA.arrowType(e) = EdgeArrow::Last;
	}

	GA.scale(sumWidths / GA.boundingBox().width(), sumHeights / GA.boundingBox().height(), false);
	GA.scale(1.5, false);

	std::regex reg("\\W+");
	string filename = name;
	std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
	std::ofstream of("drawing-" + std::regex_replace(filename, reg, "_")
	               + "-n=" + to_string(G.numberOfNodes()) + "-m=" + to_string(G.numberOfEdges())
	               + "-" + to_string(layout_helpers::drawingCounter) + ".svg");
	GraphIO::drawSVG(GA, of);
	layout_helpers::drawingCounter++;
#endif

	string indent = "        ";
	std::cout << std::endl;
	double resolution = (LayoutStatistics::angularResolution(GA)*100)/(2*Math::pi);
	std::cout << indent << "angular resolution: " << std::setw(9) << std::setprecision(2) << std::fixed << resolution << " %" << std::endl;
	std::cout << indent << "average edge length: " << std::setw(8) << LayoutStatistics::edgeLengths(GA) / (1.0 * G.numberOfEdges()) << std::endl;
	std::cout << indent << "average bends per edge: " << std::setw(5) << LayoutStatistics::numberOfBends(GA) / (1.0 * G.numberOfEdges()) << std::endl;

	// Assert that we do not have any needless bendpoints
	for(edge e : G.edges) {
		auto toPoint = [&](node v) { return DPoint(GA.x(v), GA.y(v)); };
		DPolyline bends = GA.bends(e);

		if(!bends.empty()) {
			AssertThat(bends.front(), !Equals(toPoint(e->source())));
			AssertThat(bends.back(), !Equals(toPoint(e->target())));
		}

		int size = bends.size();
		bends.normalize();
		AssertThat(bends.size(), Equals(size));
	}

	// Assume that any layout algorithm that requires planar graphs or planarize produces planar drawings
	if(algoPlanarizes || algoRequiresPlanar) {
		int crossingNumber = LayoutStatistics::numberOfCrossings(GA);

		std::cout << indent << "crossing number: " << std::setw(9) << crossingNumber << std::endl;

		if(instanceIsPlanar) {
			AssertThat(crossingNumber, Equals(0));
		}
	}

	return result;
}

/**
 * Runs several tests for a given layout module.
 * The layout algorithm is executed for different graphs.
 * There are no assertions yet.
 *
 * \param name
 * 	the name to be used for describing this module
 * \param L
 * 	the module to be tested
 * \param extraAttributes
 *  init attributes for GraphAttributes
 * \param req
 * 	the requirements for graphs to be drawn, see GraphPropertyFlags for details
 * \param sizes
 * 	determines the approximate number of nodes (and instances) of randomly generated graphs
 * \param planarizes
 * 	whether the layout computes a planarization (i.e., we can expect few crossings for non-planar graphs)
 * \param skipMe
 *  set this to true to skip the entire test
 */
inline void describeLayout(
  const std::string name,
  LayoutModule &L,
  long extraAttributes = 0,
  std::set<GraphProperty> req = {},
  bool planarizes = false,
  const GraphSizes& sizes = GraphSizes(),
  bool skipMe = false)
{
	describe(name, [&] {
		forEachGraphItWorks(req, [&](const Graph& G, const std::string& graphName, const std::set<GraphProperty>& props) {
			callLayout(graphName, G, L, extraAttributes, planarizes,
					doesInclude({GraphProperty::planar}, req),
					doesInclude({GraphProperty::planar}, props));
		}, sizes);
	}, skipMe);
}

template<typename T>
inline void describeLayout(
  const string &name,
  int extraAttr = 0,
  std::set<GraphProperty> req = {},
  bool planarizes = false,
  const GraphSizes& sizes = GraphSizes(),
  bool skipMe = false) {
	T layout;
	describeLayout(name, layout, extraAttr, req, planarizes, sizes, skipMe);
}
