/** \file
 * \brief Datastructures for edge chains itself and the edge
 * chains of nodes.
 *
 * \author Ivo Hedtke
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

#include <ogdf/basic/basic.h>
#include <functional>

namespace ogdf {
namespace fast_multipole_embedder{

//! Information about incident edges (16 bytes).
class NodeAdjInfo {
public:
	uint32_t degree;     //!< Total count of pairs where is either the first or second node.
	uint32_t firstEntry; //!< The first pair in the edges chain.
	uint32_t lastEntry;  //!< The last pair in the edges chain.
	uint32_t unused;     //!< Not used yet. Only for 16-byte alignment of array elements.
};

//! Information about an edge (16 bytes).
class EdgeAdjInfo {
public:
	uint32_t a;      //!< First node of the pair.
	uint32_t b;      //!< Second node of the pair.
	uint32_t a_next; //!< Next pair in the chain of the first node.
	uint32_t b_next; //!< Next pair in the chain of the second node.

	//! Returns the other node (not \p index).
	inline uint32_t twinNode(uint32_t index) const {
		OGDF_ASSERT(a == index || b == index);
		return a == index ? b : a;
	}

	//! Returns the index of the next pair of \p index.
	inline uint32_t nextEdgeAdjIndex(uint32_t index) const {
		OGDF_ASSERT(a == index || b == index);
		return a == index ? a_next : b_next;
	}
};

//! Helper method used by ArrayGraph and WSPD.
void pushBackEdge(uint32_t a, uint32_t b,
                  std::function<EdgeAdjInfo&(uint32_t)> edgeInform,
                  std::function<NodeAdjInfo&(uint32_t)> nodeInform,
                  int e_index);

}
}
