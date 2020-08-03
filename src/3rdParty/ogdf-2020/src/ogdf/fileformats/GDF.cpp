/** \file
 * \brief String conversions and Hashing for GDF fileformat
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

#include <ogdf/fileformats/GDF.h>
#include <ogdf/fileformats/Utils.h>


namespace ogdf {

namespace gdf {


std::string toString(const NodeAttribute &attr)
{
	switch(attr) {
	case NodeAttribute::Name: return "name";
	case NodeAttribute::X: return "x";
	case NodeAttribute::Y: return "y";
	case NodeAttribute::Z: return "z";
	case NodeAttribute::FillColor: return "color";
	case NodeAttribute::FillBgColor: return "fillbg";
	case NodeAttribute::FillPattern: return "fillpattern";
	case NodeAttribute::StrokeColor: return "strokecolor";
	case NodeAttribute::StrokeType: return "stroketype";
	case NodeAttribute::StrokeWidth: return "strokewidth";
	case NodeAttribute::Shape: return "style";
	case NodeAttribute::Width: return "width";
	case NodeAttribute::Height: return "height";
	case NodeAttribute::Label: return "label";
	case NodeAttribute::Template: return "template";
	case NodeAttribute::Weight: return "weight";
	case NodeAttribute::Unknown: return "unknown";
	}

	return "";
}


std::string toString(const EdgeAttribute &attr)
{
	switch(attr) {
	case EdgeAttribute::Label: return "label";
	case EdgeAttribute::Source: return "node1";
	case EdgeAttribute::Target: return "node2";
	case EdgeAttribute::Weight: return "weight";
	case EdgeAttribute::Directed: return "directed";
	case EdgeAttribute::Color: return "color";
	case EdgeAttribute::Bends: return "bends";
	case EdgeAttribute::Unknown: return "unknown";
	}

	return "";
}


std::string toString(const Shape &shape)
{
	/*
	 * Based on official documentation:
	 * http://guess.wikispot.org/The_GUESS_.gdf_format
	 */
	switch(shape) {
	case Shape::Rect: return "1";
	case Shape::Ellipse: return "2";
	case Shape::RoundedRect: return "3";
	case Shape::Image: return "7";
	default: return "1";
	}
}


NodeAttribute toNodeAttribute(const std::string &str)
{
	return toEnum(
		str, toString,
		static_cast<NodeAttribute>(0), NodeAttribute::Unknown, NodeAttribute::Unknown);
}

EdgeAttribute toEdgeAttribute(const std::string &str)
{
	return toEnum(
		str, toString,
		static_cast<EdgeAttribute>(0), EdgeAttribute::Unknown, EdgeAttribute::Unknown);
}

Shape toShape(const std::string &str)
{
	return toEnum(
		str, toString,
		Shape::Rect, Shape::Image, Shape::Rect);
}

}

}
