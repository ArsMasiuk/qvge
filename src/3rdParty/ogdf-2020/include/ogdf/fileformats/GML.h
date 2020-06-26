/** \file
 * \brief GML related enums and string conversion functions.
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

#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/HashArray.h>
#include <string>


namespace ogdf {

namespace gml {

enum class ObjectType {
	IntValue,
	DoubleValue,
	StringValue,
	ListBegin,
	ListEnd,
	Key,
	Eof,
	Error,
};

enum class Key {
	Id,
	Label,
	Creator,
	Name,
	Graph,
	Version,
	Directed,
	Node,
	Edge,
	Graphics,
	X,
	Y,
	Z,
	W,
	H,
	Type,
	Width,
	Source,
	Target,
	Arrow,
	Outline,
	Point,
	Bends,
	Generalization,
	SubGraph,
	Fill,
	FillBg,
	Cluster,
	Root,
	Vertex,
	Color,
	Height,
	Stipple,
	Pattern,
	LineWidth,
	Template,
	Weight, // Used for node weight and edge double weight
	EdgeIntWeight,
	Unknown
};


std::string toString(const Key &attr);
std::string toString(const EdgeArrow &arrow);
std::string toString(const Graph::NodeType &type);

Key toKey(const std::string &str);
EdgeArrow toArrow(const std::string &str);
Graph::NodeType toNodeType(const std::string &str);

}
}

// Implement hash for gml::Key so we can use it as a key in unordered_map.
namespace std {
	template<>
	struct hash<ogdf::gml::Key> {
		std::size_t operator()(const ogdf::gml::Key& t) const { return size_t(t); }
	};
}
