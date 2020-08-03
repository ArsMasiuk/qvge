/** \file
 * \brief GraphML related enums and string conversion functions.
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

#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/HashArray.h>
#include <string>

namespace ogdf {
namespace graphml {

enum class Attribute {
	NodeLabel = 0,
	EdgeLabel,

	X, Y, Z,
	Width, Height,
	Size, // Gephi compatibility (size = max(width, height)).
	Shape,

	NodeLabelX,
	NodeLabelY,
	NodeLabelZ,

	NodeStrokeColor,
	NodeStrokeType,
	NodeStrokeWidth,
	EdgeStrokeColor,
	EdgeStrokeType,
	EdgeStrokeWidth,
	ClusterStroke,
	NodeFillPattern,
	NodeFillBackground,
	R, G, B, // Gephi compatibility (fill compounds).

	NodeWeight,
	EdgeWeight,

	NodeType,
	EdgeType,

	NodeId,
	Template,

	EdgeArrow,
	EdgeSubGraph,
	EdgeBends,

	Unknown // Has to be the last one!
};

std::string toString(const Attribute &attr);
std::string toString(const Shape &shape);
std::string toString(const EdgeArrow &arrow);
std::string toString(const Graph::NodeType &type);
std::string toString(const Graph::EdgeType &type);

Attribute toAttribute(const std::string &str);
Shape toShape(const std::string &str);
EdgeArrow toArrow(const std::string &str);
Graph::NodeType toNodeType(const std::string &str);
Graph::EdgeType toEdgeType(const std::string &str);

}
}
