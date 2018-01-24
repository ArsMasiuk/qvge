/** \file
 * \brief Edge types and patterns for planar representations
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
	//FOUR TYPE LEVELS:
	//primary holds information about generalization/association,...
	//secondary about merger edges,...
	//user edge types can be set locally

#pragma once

namespace ogdf {

using edgeType = long long;

enum class UMLEdgeTypePatterns : edgeType {
	Primary   = 0x0000000f,
	Secondary = 0x000000f0,
	Tertiary  = 0x00000f00,
	Fourth    = 0x0000f000,
	User      = 0xff000000,
	All       = 0xffffffff
}; // attention sign, 7fffffff

inline edgeType operator & (edgeType lhs, UMLEdgeTypePatterns rhs) {
	return lhs & static_cast<edgeType>(rhs);
}

inline edgeType operator & (UMLEdgeTypePatterns lhs, edgeType rhs) {
	return static_cast<edgeType>(lhs) & rhs;
}

inline edgeType operator << (edgeType lhs, UMLEdgeTypePatterns rhs) {
	return lhs << static_cast<edgeType>(rhs);
}

enum class UMLEdgeTypeConstants {
	//primary types (should be disjoint bits)
	PrimAssociation = 0x1, PrimGeneralization = 0x2, PrimDependency = 0x4,
	//secondary types: reason of insertion (should be disjoint types, but not bits,
	//but may not completely cover others that are allowed to be set together)
	//preliminary: setsecondarytype deletes old type
	//edge in Expansion, dissection edge, face splitter, cluster boundary
	SecExpansion = 0x1, SecDissect = 0x2, SecFaceSplitter = 0x3,
	SecCluster = 0x4, SecClique, //the boundaries
	//tertiary types: special types
	//merger edge, vertical in hierarchy, alignment, association class connnection
	Merger = 0x1, Vertical = 0x2, Align = 0x3, AssClass = 0x8,
	//fourth types: relation of nodes
	//direct neighbours in hierarchy = brother, neighbour = halfbrother
	//same level = cousin, to merger = ToMerger, from Merger = FromMerger
	Brother = 0x1, HalfBrother = 0x2, Cousin= 0x3,
	//fifth level types
	FifthToMerger = 0x1, FifthFromMerger = 0x2
	//user type hint: what you have done with the edge, e.g. brother edge
	//that is embedded crossing free and should be drawn bend free
};

inline edgeType operator & (edgeType lhs, UMLEdgeTypeConstants rhs) {
	return lhs & static_cast<edgeType>(rhs);
}

inline bool operator == (edgeType lhs, UMLEdgeTypeConstants rhs) {
	return lhs == static_cast<edgeType>(rhs);
}

enum class UMLEdgeTypeOffsets {
	Primary = 0,
	Secondary = 4,
	Tertiary = 8,
	Fourth = 12,
	Fifth = 16,
	User = 24
};

inline edgeType operator >> (edgeType lhs, UMLEdgeTypeOffsets rhs) {
	return lhs >> static_cast<edgeType>(rhs);
}

inline edgeType operator << (edgeType lhs, UMLEdgeTypeOffsets rhs) {
	return lhs << static_cast<edgeType>(rhs);
}

inline edgeType operator << (UMLEdgeTypeConstants lhs, UMLEdgeTypeOffsets rhs) {
	return static_cast<edgeType>(lhs) << static_cast<edgeType>(rhs);
}

}
