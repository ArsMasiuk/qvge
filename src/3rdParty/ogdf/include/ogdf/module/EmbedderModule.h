/** \file
 * \brief Defines ogdf::EmbedderModule.
 *
 * \author Thorsten Kerkhof (thorsten.kerkhof@udo.edu)
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

#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/Module.h>
#include <ogdf/basic/Timeouter.h>

namespace ogdf {

/**
 * \brief Base class for embedder algorithms.
 *
 * An embedder algorithm computes a planar embedding of a planar graph.
 * Usually, such an algorithm optimizes some properties of the embedding.
 * for example, it might maximize the number of nodes incident with the outer face.
 *
 * \see PlanarizationLayout, PlanarizationGridLayout
 */
class OGDF_EXPORT EmbedderModule : public Module, public Timeouter {
public:
	//! Initializes an embedder module.
	EmbedderModule() { }

	virtual ~EmbedderModule() { }

	/**
	 * \brief Calls the embedder algorithm for graph \p G.
	 * \pre \p G is planar.
	 * \param G is the graph that shall be embedded.
	 * \param adjExternal is set (by the algorithm) to an adjacency entry on the
	 *        external face of \p G.
	 */
	void call(Graph& G, adjEntry& adjExternal) {
		if (G.numberOfNodes() > 1 && G.numberOfEdges() > 1) {
			OGDF_ASSERT(isPlanar(G));
			doCall(G, adjExternal);
		} else if (G.numberOfEdges() == 1) {
			adjExternal = G.firstEdge()->adjSource();
		}
	};

	//! Calls the embedder algorithm for graph \p G.
	void operator()(Graph& G, adjEntry& adjExternal) { call(G, adjExternal); }

	OGDF_MALLOC_NEW_DELETE
protected:

	/**
	 * \brief Calls the embedder algorithm for graph \p G.
	 * \p G is guaranteed to be planar.
	 * See #call .
	 */
	virtual void doCall(Graph& G, adjEntry& adjExternal) = 0;
};

}
