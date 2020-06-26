/** \file
 * \brief Declaration of the Random Hierarchy graph generator.
 *
 * \author Carsten Gutwenger, Christoph Buchheim
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

#include <ogdf/basic/Graph.h>

namespace ogdf {

/**
 * @addtogroup graph-generators
 * @{
 */

//! @name Randomized graph generators
//! @{

//! Creates a random hierarchical graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes.
 * @param m is the number of edges.
 * @param planar determines if the resulting graph is (level-)planar.
 * @param singleSource determines if the graph is a single-source graph.
 * @param longEdges determines if the graph has long edges (spanning 2 layers
 *        or more); otherwise the graph is proper.
 */
OGDF_EXPORT void randomHierarchy(
	Graph &G,
	int n,
	int m,
	bool planar,
	bool singleSource,
	bool longEdges);

//! @}

/** @} */

}
