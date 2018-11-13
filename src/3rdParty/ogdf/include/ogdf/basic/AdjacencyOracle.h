/** \file
 * \brief Declares class AdjacencyOracle.
 *
 * \author Rene Weiskircher
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

#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/Array2D.h>

namespace ogdf {

//! Tells you in constant time if two nodes are adjacent
/**
 * @ingroup graphs
 *
 * AdjacencyOracle is initialized with a Graph and returns for
 * any pair of nodes in constant time if they are adajcent.
 */
class AdjacencyOracle {
public:
	/**
	 * The constructor for the class, needs time O(n + m)
	 *
	 * Builds a 2D-array indexed by the numbers of vertices.
	 *
	 * It uses only the part
	 * of the matrix left of the diagonal (where the first index is smaller than the
	 * second. For each pair of vertices, the corresponding entry in the matrix is set true
	 * if and only if the two vertices are adjacent.
	 */
	explicit AdjacencyOracle(const Graph &G);

	//! The destructor
	~AdjacencyOracle() { }

	//! Returns true iff two vertices are adjacent.
	bool adjacent(const node, const node) const;

private:
	NodeArray<int> m_nodeNum; //!< The internal number given to each node
	Array2D<bool> m_adjacencyMatrix; //!< A 2D-array where the entry is true if the nodes with the corresponding number are adjacent
};

}
