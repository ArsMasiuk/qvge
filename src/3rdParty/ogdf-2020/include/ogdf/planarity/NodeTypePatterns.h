/** \file
 * \brief Declaration of node types and patterns for planar
 *        representations
 *
 * \author Karsten Klein
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

//edge type patterns:
//THREE TYPE LEVELS:
//primary: holds information about structural/non-structural
//		   nodes, this  influences the handling in algorithms
//secondary: type of node, e.g. flow node, simple label node, ...
//user edge types can be set locally

#pragma once

namespace ogdf {

using nodeType = long long;

enum class UMLNodeTypePatterns : nodeType {
	Primary   = 0x0000000f,
	Secondary = 0x000000f0,
	Tertiary  = 0x00000f00,
	Fourth    = 0x0000f000,
	User      = 0xff000000,
	All       = 0xffffffff
};

enum class UMLNodeTypeConstants {
	//primary types (should be disjoint bits)
	PrimOriginal = 0x1, PrimCopy = 0x2,
	//secondary types: type of node (should be disjoint types, but not bits,
	//but may not completely cover others that are allowed to be set together)
	//preliminary: setsecondarytype deletes old type
	//defines the structure of the diagram, e.g. as flow transmitter
	SecStructural = 0x1, SecNonStructural = 0x2,
	//tertiary
	//crossing node, high/low degree expander
	TerCrossing = 0x1, TerExpander = 0x2, TerHDExpander = 0x6,
	TerLDExpander = 0xA,
	//fourth level types: special types
	//flow node, simple label node, type label node, expansion corner node
	FourFlow = 0x1, FourLabel = 0x2, FourType = 0x3, FourCorner = 0x4

	//user type hint: what you have done with the edge, e.g. brother edge
	//that is embedded crossing free and should be drawn bend free
};

enum class UMLNodeTypeOffsets {
	Primary = 0,
	Secondary = 4,
	Tertiary = 8,
	Fourth = 12,
	Fifth = 16,
	User = 24
};

inline int operator << (UMLNodeTypeConstants lhs, UMLNodeTypeOffsets rhs) {
	return static_cast<int>(lhs) << static_cast<int>(rhs);
}

}
