/** \file
 * \brief Declarations for GDF file format
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

#include <ogdf/basic/graphics.h>
#include <ogdf/basic/Hashing.h>
#include <ogdf/basic/HashArray.h>

#include <string>


namespace ogdf {

namespace gdf {


enum class NodeAttribute {
	// GDF standard
	Name = 0,
	Label,
	X,
	Y,
	Z,
	FillColor,
	FillPattern,
	StrokeColor,
	StrokeType,
	StrokeWidth,

	Shape,
	Width,
	Height,
	// OGDF specific
	Template,
	Weight,
	FillBgColor,
	Unknown
};


enum class EdgeAttribute {
	// GDF standard
	Label = 0,
	Source,
	Target,
	Weight,
	Directed,
	Color,
	// OGDF specific
	Bends,
	Unknown
};


std::string toString(const NodeAttribute &attr);
std::string toString(const EdgeAttribute &attr);
std::string toString(const Shape &shape);

NodeAttribute toNodeAttribute(const std::string &str);
EdgeAttribute toEdgeAttribute(const std::string &str);
Shape toShape(const std::string &str);

}
}
