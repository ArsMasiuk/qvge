/** \file
 * \brief Declaration of class StaticPlanarSPQRTree.
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

#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/decomposition/PlanarSPQRTree.h>

namespace ogdf {

template<class A, class B> class Tuple2;

//! SPQR-trees of planar graphs.
/**
 * @ingroup decomp
 *
 * The class StaticPlanarSPQRTree maintains the triconnected components of a
 * planar biconnected graph G and represents all possible embeddings
 * of G. Each skeleton graph is embedded.
 *
 * The current embeddings of the skeletons define an embedding of G.
 * There are two basic operations for obtaining another embedding
 * of G: reverse(v), which flips the skeleton of an R-node v
 * around its poles, and swap(v,e_1,e_2), which exchanges the
 * positions of the edges e_1 and e_2 in the skeleton of a P-node v.
 */
class OGDF_EXPORT StaticPlanarSPQRTree : public StaticSPQRTree, public PlanarSPQRTree
{
public:
	// constructors

	//! Creates an SPQR tree \a T for planar graph \p G rooted at the first edge of \p G.
	/**
	 * If \p isEmbedded is set to true, \p G must represent a combinatorial
	 * embedding, i.e., the counter-clockwise order of the adjacency entries
	 * around each vertex defines an embedding.
	 * \pre \p G is planar and biconnected and contains at least 3 nodes,
	 *      or \p G has exactly 2 nodes and at least 3  edges.
	 */
	explicit StaticPlanarSPQRTree(const Graph &G, bool isEmbedded = false) :
		StaticSPQRTree(G)
	{
		PlanarSPQRTree::init(isEmbedded);
	}

	//! Creates an SPQR tree \a T for planar graph \p G rooted at edge \p e.
	/**
	 * If \p isEmbedded is set to true, \p G must represent a combinatorial
	 * embedding, i.e., the counter-clockwise order of the adjacency entries
	 * around each vertex defines an embedding.
	 * \pre \p e is an edge in \p G, and \p G is planar and biconnected and
	 * contains at least 3 nodes, or \p G has exactly 2 nodes and at least 3
	 * edges.
	 */
	StaticPlanarSPQRTree(const Graph &G, edge e, bool isEmbedded = false) :
		StaticSPQRTree(G,e)
	{
		PlanarSPQRTree::init(isEmbedded);
	}
};

}
