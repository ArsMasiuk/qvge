/** \file
 * \brief Implementation of GEXF string conversion functions.
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

#include <ogdf/fileformats/GEXF.h>
#include <ogdf/fileformats/Utils.h>


namespace ogdf {

namespace gexf {


std::string toString(const Shape &shape)
{
	switch(shape) {
	case Shape::Rect: return "square";
	case Shape::RoundedRect: return "rect"; // Not supported.
	case Shape::Ellipse: return "disc";
	case Shape::Triangle: return "triangle";
	case Shape::Rhomb: return "diamond";
	case Shape::Image: return "image";
	default: return "disc";
	}
}


Shape toShape(const std::string &str)
{
	return toEnum(str, toString, Shape::Rect, Shape::Image, Shape::Rect);
}

std::string toGEXFStrokeType(const StrokeType &type)
{
	switch(type) {
	case StrokeType::Solid: return "solid";
	case StrokeType::Dot: return "dotted";
	case StrokeType::Dash: return "dashed";
	case StrokeType::Dashdot: return "dashdot";
	case StrokeType::Dashdotdot: return "dashdotdot";
	default: return "";
	}
}

StrokeType toStrokeType(const std::string &str)
{
	// GEXF supports solid, dotted, dashed, double.
	// We don't support double, but dashdot and dashdotdot instead.
	return toEnum(str, toGEXFStrokeType, StrokeType::None, StrokeType::Dashdotdot, StrokeType::Solid);
}

}

}
