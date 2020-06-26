/** \file
 * \brief Implementation of DOT string conversion functions.
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

#include <ogdf/fileformats/DOT.h>
#include <ogdf/fileformats/Utils.h>

namespace ogdf {

namespace dot {


std::string toString(const Attribute &attr)
{
	switch(attr) {
		case Attribute::Id: return "id";
		case Attribute::Label: return "label";
		case Attribute::Template: return "comment";
		case Attribute::Width: return "width";
		case Attribute::Height: return "height";
		case Attribute::Shape: return "shape";
		case Attribute::Position: return "pos";
		case Attribute::LabelPosition: return "labelpos";
		case Attribute::Stroke: return "color";
		case Attribute::StrokeType: return "stroketype";
		case Attribute::Fill: return "fillcolor";
		case Attribute::Weight: return "weight";
		case Attribute::Arrow: return "arrow";
		case Attribute::StrokeWidth: return "strokewidth";
		case Attribute::FillPattern: return "fillpattern";
		case Attribute::FillBackground: return "fillbgcolor";
		case Attribute::Type: return "type";
		case Attribute::Dir: return "dir";
		case Attribute::SubGraphs: return "available_for";
		default: return "comment";
	}
}


std::string toString(const Shape &shape)
{
	switch(shape) {
		case Shape::Rect:             return "rect";
		case Shape::RoundedRect:      return "roundedrect";
		case Shape::Ellipse:          return "ellipse";
		case Shape::Triangle:         return "triangle";
		case Shape::Pentagon:         return "pentagon";
		case Shape::Hexagon:          return "hexagon";
		case Shape::Octagon:          return "octagon";
		case Shape::Rhomb:            return "diamond";
		case Shape::Trapeze:          return "trapezium";
		case Shape::Parallelogram:    return "parallelogram";
		case Shape::InvTriangle:      return "invtriangle";
		case Shape::InvTrapeze:       return "invtrapezium";
		case Shape::InvParallelogram: return "invparallelogram";
		case Shape::Image:            return "image";
	}
	OGDF_ASSERT(false);
	return "UNKNOWN";
}


std::string toString(const EdgeArrow &arrow)
{
	switch(arrow) {
		case EdgeArrow::None:      return "none";
		case EdgeArrow::Last:      return "forward";
		case EdgeArrow::First:     return "back";
		case EdgeArrow::Both:      return "both";
		case EdgeArrow::Undefined: return "none"; // Not supported.
	}
	OGDF_ASSERT(false);
	return "UNKNOWN";
}


std::string toString(const Graph::EdgeType &type)
{
	switch(type) {
		case Graph::EdgeType::association:    return "association";
		case Graph::EdgeType::generalization: return "generalization";
		case Graph::EdgeType::dependency:     return "dependency";
	}
	OGDF_ASSERT(false);
	return "UNKNOWN";
}


Attribute toAttribute(const std::string &str)
{
	return toEnum(str, toString, static_cast<Attribute>(0), Attribute::Unknown, Attribute::Unknown);
}

Shape toShape(const std::string &str) {
	return toEnum(str, toString, Shape::Rect, Shape::Image, Shape::Rect);
}

EdgeArrow toArrow(const std::string &str)
{
	return toEnum(str, toString, EdgeArrow::None, EdgeArrow::Undefined, EdgeArrow::Undefined);
}

Graph::EdgeType toEdgeType(const std::string &str) {
	return toEnum(str, toString, Graph::EdgeType::association, Graph::EdgeType::dependency, Graph::EdgeType::association);
}

}

}
