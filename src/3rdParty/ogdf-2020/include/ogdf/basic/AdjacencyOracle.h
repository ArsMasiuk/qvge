/** \file
 * \brief Declaration of ogdf::AdjacencyOracle class
 *
 * \author Rene Weiskircher, Stephan Beyer
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

namespace ogdf {

//! Tells you in constant time if two nodes are adjacent
/**
 * @ingroup graphs
 *
 * AdjacencyOracle is initialized with a graph and returns for
 * any pair of nodes in constant time if they are adjacent.
 */
class OGDF_EXPORT AdjacencyOracle {
public:
	/**
	 * The constructor for the class, needs time O(n^2 + m) ∩ Ω(n).
	 *
	 * Builds the bottom-left part of an adjacency matrix for the subset of nodes
	 * with degree above \p degreeThreshold.
	 */
	explicit AdjacencyOracle(const Graph &G, int degreeThreshold = 32);

	//! The destructor
	~AdjacencyOracle() { }

	//! Returns true iff vertices \p v and \p w are adjacent.
	bool adjacent(node v, node w) const;

private:
	//! Returns an index for #m_adjacencies that corresponds to the entry of nodes \p v and \p w
	int index(node v, node w) const;

	NodeArray<int> m_nodeNum; //!< The internal number given to each node
	std::vector<bool> m_adjacencies; //!< An entry is true iff the corresponding nodes are adjacent
};

}
