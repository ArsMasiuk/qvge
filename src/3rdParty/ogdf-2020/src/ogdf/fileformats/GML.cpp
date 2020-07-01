/** \file
 * \brief Implementation of GML string conversion functions.
 *
 * \author Jöran Schierbaum
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

#include <ogdf/fileformats/GML.h>
#include <ogdf/fileformats/Utils.h>

namespace ogdf {

namespace gml {


std::string toString(const Key &attr)
{
	switch(attr) {
		case Key::Id: return "id";
		case Key::Label: return "label";
		case Key::Creator: return "creator";
		case Key::Name: return "name";
		case Key::Graph: return "graph";
		case Key::Version: return "version";
		case Key::Directed: return "directed";
		case Key::Node: return "node";
		case Key::Edge: return "edge";
		case Key::Graphics: return "graphics";
		case Key::X: return "x";
		case Key::Y: return "y";
		case Key::Z: return "z";
		case Key::W: return "w";
		case Key::H: return "h";
		case Key::Type: return "type";
		case Key::Width: return "width";
		case Key::Height: return "height";
		case Key::Source: return "source";
		case Key::Target: return "target";
		case Key::Arrow: return "arrow";
		case Key::Outline: return "outline";
		case Key::Point: return "point";
		case Key::Bends: return "Line";
		case Key::Generalization: return "generalization";
		case Key::SubGraph: return "subgraph";
		case Key::Fill: return "fill";
		case Key::FillBg: return "fillbg";
		case Key::Cluster: return "cluster";
		case Key::Root: return "rootcluster";
		case Key::Vertex: return "vertex";
		case Key::Color: return "color";
		case Key::Stipple: return "stipple";
		case Key::Pattern: return "pattern";
		case Key::LineWidth: return "lineWidth";
		case Key::Template: return "template";
		case Key::Weight: return "weight";
		case Key::EdgeIntWeight: return "intWeight";
		default: return "comment";
	}
}

std::string toString(const EdgeArrow &arrow)
{
	switch (arrow) {
		case EdgeArrow::None: return "none";
		case EdgeArrow::Last: return "last";
		case EdgeArrow::First: return "first";
		case EdgeArrow::Both: return "both";
		default: return "none"; // Not supported.
	}
}


Key toKey(const std::string &str)
{
	return toEnum(str, toString, static_cast<Key>(0), Key::Unknown, Key::Unknown);
}

EdgeArrow toArrow(const std::string &str)
{
	return toEnum(str, toString, static_cast<EdgeArrow>(0), EdgeArrow::Undefined, EdgeArrow::Undefined);
}


std::string toString(const Graph::NodeType &type)
{
	switch(type) {
	case Graph::NodeType::vertex: return "vertex";
	case Graph::NodeType::dummy: return "dummy";
	case Graph::NodeType::generalizationMerger: return "generalization-merger";
	case Graph::NodeType::generalizationExpander: return "generalization-expander";
	case Graph::NodeType::highDegreeExpander: return "high-degree-expander";
	case Graph::NodeType::lowDegreeExpander: return "low-degree-expander";
	case Graph::NodeType::associationClass: return "association-class";
	default: return "vertex";
	}
}

Graph::NodeType toNodeType(const std::string &str)
{
	return toEnum(
		str, toString,
		static_cast<Graph::NodeType>(0), Graph::NodeType::associationClass, Graph::NodeType::vertex);
}

}

}
