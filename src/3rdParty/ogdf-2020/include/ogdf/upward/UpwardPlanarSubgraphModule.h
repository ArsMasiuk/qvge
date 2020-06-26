/** \file
 * \brief Declaration of interface for upward planar subgraph
 *        algorithms.
 *
 * \author Carsten Gutwenger
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

#include <ogdf/basic/GraphCopy.h>


namespace ogdf {

/**
 * \brief Interface for algorithms for computing an upward planar subgraph.
 */
class OGDF_EXPORT UpwardPlanarSubgraphModule {
public:
	//! Initializes an upward planar subgraph module.
	UpwardPlanarSubgraphModule() { }

	// destruction
	virtual ~UpwardPlanarSubgraphModule() { }

	/**
	 * \brief Computes set of edges \p delEdges which have to be deleted to obtain the upward planar subgraph.
	 *
	 * Must be implemented by derived classes.
	 * @param G is the input graph.
	 * @param delEdges is assigned the set of edges which have to be deleted in \p G
	 *        to obtain the upward planar subgraph.
	 */
	virtual void call(const Graph &G, List<edge> &delEdges) = 0;


	//! Computes set of edges \p delEdges which have to be deleted to obtain the upward planar subgraph.
	void operator()(const Graph &G, List<edge> &delEdges) {
		call(G,delEdges);
	}


	/**
	 * \brief Makes \p GC upward planar by deleting edges.
	 * @param GC is a copy of the input graph.
	 * @param delOrigEdges is the set of original edges whose copies have been
	 *        deleted in \p GC.
	 */
	void callAndDelete(GraphCopy &GC, List<edge> &delOrigEdges);

	OGDF_MALLOC_NEW_DELETE
};

}
