/** \file
 * \brief Declaration of st-Numbering functions
 *
 * \author Sebastian Leipert
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

//! Computes an st-Numbering of \p G.
/**
 * @ingroup ga-orient
 *
 * \pre \p G must be biconnected and simple, with the exception that
 * the graph is allowed to have isolated nodes. If both \p s and \p t
 * are set to nodes (both are not 0), they must be adjacent.
 *
 * @param G is the input graph.
 * @param numbering is assigned the st-number for each node.
 * @param s is the source node for the st-numbering.
 * @param t is the target node for the st-numbering.
 * @param randomized is only used when both \p s and \p t are not set (both are 0);
 *        in this case a random edge (s,t) is chosen; otherwise the first node s with degree
 *        > 0 is chosen and its first neighbor is used as t.
 * @return the number assigned to \p t, or 0 if no st-numbering could be computed.
 */
OGDF_EXPORT int computeSTNumbering(const Graph &G, NodeArray<int> &numbering,
                                   node s = nullptr, node t = nullptr,
                                   bool randomized = false);

//! Tests, whether a numbering of the nodes is an st-numbering.
/**
 * @ingroup ga-orient
 *
 * \pre \p G must be biconnected and simple, with the exception that
 * the graph is allowed to have isolated nodes.
 */
OGDF_EXPORT bool isSTNumbering(const Graph &G, NodeArray<int> &st_no, int max);

}
