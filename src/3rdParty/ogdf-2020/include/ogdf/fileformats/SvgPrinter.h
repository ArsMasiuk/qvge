/** \file
 * \brief Generator for visualizing graphs using the XML-based SVG format.
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

#include <list>
#include <sstream>
#include <ogdf/lib/pugixml/pugixml.h>
#include <ogdf/fileformats/GraphIO.h>

namespace ogdf
{

/**
 * \brief SVG Writer
 *
 * Generates and outputs XML-based SVG files depicting the layout of a (cluster-)graph.
 *
 * Curved edges will be drawn if specified by ogdf::GraphIO::SVGSettings.
 * Set the curviness to a value greater than 0 to obtain curved edges.
 * There are two modes for drawing curved edges: Bézier curves and circular arcs.
 */
class SvgPrinter
{
public:

	/**
	 * Creates a new SVG Printer for a ogdf::Graph.
	 *
	 * \param attr The attributes of the graph
	 * \param settings The SVG configuration
	 */
	SvgPrinter(const GraphAttributes &attr, const GraphIO::SVGSettings &settings)
	  : m_attr(attr)
	  , m_clsAttr(nullptr)
	  , m_settings(settings)
	{
	}

	/**
	 * Creates a new SVG Printer for a ogdf::ClusterGraph.
	 *
	 * \param attr The attributes of the graph
	 * \param settings The SVG configuration
	 */
	SvgPrinter(const ClusterGraphAttributes &attr, const GraphIO::SVGSettings &settings)
	  : m_attr(attr)
	  , m_clsAttr(&attr)
	  , m_settings(settings)
	{
	}

	/**
	 * Prints the graph and attributes of this printer to the given output stream.
	 *
	 * @param os The stream to print to
	 */
	bool draw(std::ostream &os);

private:
	//! attributes of the graph to be visualized
	const GraphAttributes &m_attr;

	//! attributes of the cluster graph (\c nullptr if no cluster graph)
	const ClusterGraphAttributes *m_clsAttr;

	//! SVG configuration
	const GraphIO::SVGSettings &m_settings;

	/**
	 * Draws a rectangle for each cluster in the ogdf::ClusterGraph.
	 *
	 * \param xmlNode the XML-node to print to
	 */
	void drawClusters(pugi::xml_node xmlNode);

	/**
	 * Draws a sequence of lines for each edge in the graph.
	 *
	 * \param xmlNode the XML-node to print to
	 */
	void drawEdges(pugi::xml_node xmlNode);

	/**
	 * Draws a sequence of lines for an edge.
	 * Arrow heads are added if requested.
	 *
	 * \param xmlNode the XML-node to print to
	 * \param e the edge to be visualized
	 */
	void drawEdge(pugi::xml_node xmlNode, edge e);

	/**
	 * Draws the curve depicting a particular edge.
	 * Draws a sequence of cubic Bézier curves if requested.
	 * Falls back to straight lines if there are exactly two points or the curviness is set to 0.
	 *
	 * Note that this method clears the list of points.
	 *
	 * \param xmlNode the XML-node to print to
	 * \param points the points along the curve
	 * \param e the edge depicted by the curve
	 * \return the XML-node of the curve
	 */
	pugi::xml_node drawCurve(pugi::xml_node xmlNode, edge e, List<DPoint> &points);

	/**
	 * Draws the path corresponding to a single line to the stream.
	 *
	 * \param ss the output stream
	 * \param p1 the first point of the line
	 * \param p2 the second point of the line
	 */
	void drawLine(std::stringstream &ss, const DPoint &p1, const DPoint &p2);

	/**
	 * Draws a list of points using cubic Bézier interpolation.
	 *
	 * \param ss the output stream
	 * \param points the points to be connected by lines
	 */
	void drawBezierPath(std::stringstream &ss, List<DPoint> &points);

	/**
	 * Draws a list of points as straight lines connected by circular arcs.
	 *
	 * \param ss the output stream
	 * \param points the points to be connected by lines
	 */
	void drawRoundPath(std::stringstream &ss, List<DPoint> &points);

	/**
	 * Draws a list of points as straight lines.
	 *
	 * \param ss the output stream
	 * \param points the points to be connected by lines
	 */
	void drawLines(std::stringstream &ss, List<DPoint> &points);

	/**
	 * Draws a cubic Bezíer path.
	 *
	 * \param ss the output stream
	 * \param p1 the first point of the line
	 * \param p2 the second point of the line
	 * \param c1 the first control point of the line
	 * \param c2 the second control point of the line
	 */
	void drawBezier(std::stringstream &ss, const DPoint &p1, const DPoint &p2, const DPoint &c1, const DPoint &c2);

	/**
	 * Draws all nodes of the graph.
	 *
	 * \param xmlNode the XML-node to print to
	 */
	void drawNodes(pugi::xml_node xmlNode);

	/**
	 * Writes the header including the bounding box as the viewport.
	 *
	 * \param doc the XML-document
	 * \return the root SVG-node
	 */
	pugi::xml_node writeHeader(pugi::xml_document &doc);

	/**
	 * Generates a string that describes the requested dash type.
	 *
	 * \param xmlNode the node to append the XML-attribute to
	 * \param lineStyle specifies the style of the dashes
	 * \param lineWidth the stroke width of the respective edge
	 */
	void writeDashArray(pugi::xml_node xmlNode, StrokeType lineStyle, double lineWidth);

	/**
	 * Draws a single node.
	 *
	 * \param xmlNode the XML-node to print to
	 * \param v the node to be printed
	 */
	void drawNode(pugi::xml_node xmlNode, node v);

	/**
	 * Draws a single cluster as a rectangle.
	 *
	 * \param xmlNode the XML-node to print to
	 * \param c the cluster to be printed
	 */
	void drawCluster(pugi::xml_node xmlNode, cluster c);

	/**
	 * Determines whether a candidate arrow tip lies inside the rectangle of the node.
	 *
	 * \param point the candidate arrow tip
	 * \param adj the adjacency entry
	 */
	bool isCoveredBy(const DPoint &point, adjEntry adj);

	/**
	 * Draws an arrow head at the end of the edge.
	 * Sets the end point of the respective edge segment to the arrow head's tip.
	 *
	 * \param xmlNode the XML-node to print to
	 * \param start the start point of the edge segment the arrow head will be placed on
	 * \param end the end point of the edge segment the arrow head will be placed on, this will usually be modified
	 * \param adj the adjacency entry
	 */
	void drawArrowHead(pugi::xml_node xmlNode, const DPoint &start, DPoint &end, adjEntry adj);

	/**
	 * Returns whether an edge arrow is to be drawn.
	 *
	 * \param adj the adjacency entry
	 */
	bool isArrowEnabled(adjEntry adj);

	/**
	 * Returns the size of the arrow.
	 * The result is zero if the respective arrow is disabled (not to be drawn).
	 *
	 * \param adj the adjacency entry
	 */
	double getArrowSize(adjEntry adj);

	/**
	 * Writes the requested line style to the line's XML-node.
	 *
	 * \param line the XML-node depicting the line
	 * \param e the edge associated with that line
	 */
	void appendLineStyle(pugi::xml_node line, edge e);

	/**
	 * Draws a polygon with the respective points.
	 *
	 * \param xmlNode the XML-node to print to
	 * \param points the list of coordinates
	 * \return The generated XML-node
	 */
	pugi::xml_node drawPolygon(pugi::xml_node xmlNode, const std::list<double> points);
};

}
