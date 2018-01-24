/** \file
 * \brief Declaration of interface for acyclic subgraph algorithms
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

#include <ogdf/basic/Graph.h>

namespace ogdf {

/**
 * \brief Base class of algorithms for computing a maximal acyclic subgraph.
 *
 * \see SugiyamaLayout
 */
class OGDF_EXPORT AcyclicSubgraphModule {
public:
	//! Initializes an acyclic subgraph module.
	AcyclicSubgraphModule() { }

	//! Destruction.
	virtual ~AcyclicSubgraphModule() { }

	/**
	 * \brief Computes the set of edges \p arcSet which have to be removed
	 *        for obtaining an acyclic subgraph of \p G.
	 *
	 * This is the actual algorithm call and must be implemented by derived classes.
	 * @param G is the input graph.
	 * @param arcSet is assigned the list of edges that have to be removed in \p G.
	 */
	virtual void call(const Graph &G, List<edge> &arcSet) = 0;

	/**
	 * \brief Computes the set of edges \p arcSet which have to be removed
	 *        for obtaining an acyclic subgraph of \p G.
	 * @param G is the input graph.
	 * @param arcSet is assigned the list of edges that have to be removed in \p G.
	 */
	void operator()(const Graph &G, List<edge> &arcSet) {
		call(G,arcSet);
	}

	/**
	 * \brief Makes \p G acyclic by reversing edges.
	 *
	 * This method will ignore self-loops in the input graph \p G; thus self-loops
	 * are neither reversed or removed nor added to \p reversed.
	 * @param G is the input graph.
	 * @param reversed is assigned the list of edges that have been reversed in \p G.
	 */
	void callAndReverse(Graph &G, List<edge> &reversed);

	// makes G acyclic (except for self-loops!) by reversing edges
	/**
	 * \brief Makes \p G acyclic by reversing edges.
	 *
	 * This method will ignore self-loops in the input graph \p G; thus self-loops
	 * are neither reversed nor removed. This is the simplified version of callAndDelete()
	 * that does not return the list of reversed edges.
	 * @param G is the input graph.
	 */
	void callAndReverse(Graph &G);

	// makes G acyclic by deleting edges
	/**
	 * \brief Makes \p G acyclic by removing edges.
	 *
	 * This method will also remove self-loops in the input graph \p G.
	 * @param G is the input graph.
	 */
	void callAndDelete(Graph &G);

	OGDF_MALLOC_NEW_DELETE
};

}
