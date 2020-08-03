/** \file
 * \brief Implements GDF write functionality of class GraphIO.
 *
 * \author Łukasz Hanuszczak
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

#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/fileformats/GDF.h>


namespace ogdf {

namespace gdf {


static inline void writeColor(std::ostream &os, const Color &color)
{
	int r = color.red(), g = color.green(), b = color.blue();
	os << "\"" << r << "," << g << "," << b << "\"";
}


static inline void writeNodeHeader(
	std::ostream &os,
	const GraphAttributes *GA)
{
	os << "nodedef>";
	os << toString(NodeAttribute::Name);

	const long attrs = GA ? GA->attributes() : 0;
	if(attrs & GraphAttributes::nodeLabel) {
		os << "," << toString(NodeAttribute::Label);
	}
	if(attrs & GraphAttributes::nodeGraphics) {
		os << "," << toString(NodeAttribute::X);
		os << "," << toString(NodeAttribute::Y);
		if(attrs & GraphAttributes::threeD) {
			os << "," << toString(NodeAttribute::Z);
		}
		os << "," << toString(NodeAttribute::Shape);
		os << "," << toString(NodeAttribute::Width);
		os << "," << toString(NodeAttribute::Height);
	}
	if(attrs & GraphAttributes::nodeStyle) {
		os << "," << toString(NodeAttribute::FillColor);
		os << "," << toString(NodeAttribute::StrokeColor);
		os << "," << toString(NodeAttribute::StrokeType);
		os << "," << toString(NodeAttribute::StrokeWidth);
		os << "," << toString(NodeAttribute::FillPattern);
		os << "," << toString(NodeAttribute::FillBgColor);
	}
	if(attrs & GraphAttributes::nodeTemplate) {
		os << "," << toString(NodeAttribute::Template);
	}
	if(attrs & GraphAttributes::nodeWeight) {
		os << "," << toString(NodeAttribute::Weight);
	}

	os << "\n";
}


static inline void writeNode(
	std::ostream &os,
	const GraphAttributes *GA, node v)
{
	/*
	 * According to official documentation, it is preferred to not name nodes
	 * as number literals and preceed it with some string if needed.
	 */
	os << "n" << v->index();

	const long attrs = GA ? GA->attributes() : 0;
	if(attrs & GraphAttributes::nodeLabel) {
		os << "," << GA->label(v);
	}
	if(attrs & GraphAttributes::nodeGraphics) {
		os << "," << GA->x(v);
		os << "," << GA->y(v);
		if(attrs & GraphAttributes::threeD) {
			os << "," << GA->z(v);
		}
		os << "," << toString(GA->shape(v));
		os << "," << GA->width(v);
		os << "," << GA->height(v);
	}
	if(attrs & GraphAttributes::nodeStyle) {
		os << ",";
		writeColor(os, GA->fillColor(v));
		os << ",";
		writeColor(os, GA->strokeColor(v));
		os << "," << toString(GA->strokeType(v));
		os << "," << GA->strokeWidth(v);
		os << "," << toString(GA->fillPattern(v));
		os << ",";
		writeColor(os, GA->fillBgColor(v));
	}
	if(attrs & GraphAttributes::nodeTemplate) {
		os << "," << GA->templateNode(v);
	}
	if(attrs & GraphAttributes::nodeWeight) {
		os << "," << GA->weight(v);
	}

	os << "\n";
}


static inline void writeEdgeHeader(
	std::ostream &os,
	const GraphAttributes *GA)
{
	os << "edgedef>";
	os << toString(EdgeAttribute::Source);
	os << "," << toString(EdgeAttribute::Target);
	if(GA && GA->directed()) {
		os << "," << toString(EdgeAttribute::Directed);
	}

	const long attrs = GA ? GA->attributes() : 0;
	if(attrs & GraphAttributes::edgeLabel) {
		os << "," << toString(EdgeAttribute::Label);
	}
	if(attrs & (GraphAttributes::edgeIntWeight |
	            GraphAttributes::edgeDoubleWeight))
	{
		os << "," << toString(EdgeAttribute::Weight);
	}
	if(attrs & GraphAttributes::edgeStyle) {
		os << "," << toString(EdgeAttribute::Color);
	}
	if(attrs & GraphAttributes::edgeGraphics) {
		os << "," << toString(EdgeAttribute::Bends);
	}

	os << "\n";
}


static inline void writeEdge(
	std::ostream &os,
	const GraphAttributes *GA, edge e)
{
	os << "n" << e->source()->index() << ","
	   << "n" << e->target()->index();
	if(GA && GA->directed()) {
		os << "," << "true";
	}

	const long attrs = GA ? GA->attributes() : 0;
	if(attrs & GraphAttributes::edgeLabel) {
		os << "," << GA->label(e);
	}
	if(attrs & GraphAttributes::edgeDoubleWeight) {
		os << "," << GA->doubleWeight(e);
	} else if(attrs & GraphAttributes::edgeIntWeight) {
		os << "," << GA->intWeight(e);
	}
	if(attrs & GraphAttributes::edgeStyle) {
		os << ",";
		writeColor(os, GA->strokeColor(e));
	}
	if(attrs & GraphAttributes::edgeGraphics) {
		os << "," << "\"";

		bool comma = false;
		for(const DPoint &p : GA->bends(e)) {
			if(comma) {
				os << ",";
			}
			comma = true;

			os << p.m_x << "," << p.m_y;
		}

		os << "\"";
	}

	os << "\n";
}


static void writeGraph(
	std::ostream &os,
	const Graph &G, const GraphAttributes *GA)
{
	std::ios_base::fmtflags currentFlags = os.flags();
	os.flags(currentFlags | std::ios::fixed);

	// Node definition section.
	writeNodeHeader(os, GA);

	for(node v : G.nodes) {
		writeNode(os, GA, v);
	}

	// Edge definition section.
	writeEdgeHeader(os, GA);

	for(edge e : G.edges) {
		writeEdge(os, GA, e);
	}
	os.flags(currentFlags);
}

}


bool GraphIO::writeGDF(const Graph &G, std::ostream &os)
{
	bool result = os.good();

	if(result) {
		gdf::writeGraph(os, G, nullptr);
	}

	return result;
}


bool GraphIO::writeGDF(const GraphAttributes &GA, std::ostream &os)
{
	bool result = os.good();

	if(result) {
		gdf::writeGraph(os, GA.constGraph(), &GA);
	}

	return result;
}

}
