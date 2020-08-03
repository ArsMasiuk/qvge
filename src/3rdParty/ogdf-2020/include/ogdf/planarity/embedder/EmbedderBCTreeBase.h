/** \file
 * \brief Definition of ogdf::EmbedderBCTreeBase.
 *
 * \author Thorsten Kerkhof, Tilo Wiedera
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

#include <ogdf/planarity/EmbedderModule.h>
#include <ogdf/decomposition/BCTree.h>
#include <ogdf/planarity/embedder/EmbedderMaxFaceBiconnectedGraphs.h>
#include <ogdf/planarity/embedder/EmbedderMaxFaceBiconnectedGraphsLayers.h>

namespace ogdf {
namespace embedder {

//! Common base for embedder algorithms based on BC trees.
template<bool EnableLayers>
class OGDF_EXPORT EmbedderBCTreeBase : public EmbedderModule {
	using BicompEmbedder = typename std::conditional<EnableLayers, EmbedderMaxFaceBiconnectedGraphsLayers<int>, EmbedderMaxFaceBiconnectedGraphs<int>>::type;
protected:
	//! BC-tree of the original graph
	BCTree* pBCTree = nullptr;

	//! an adjacency entry on the external face
	adjEntry* pAdjExternal = nullptr;

	//! Initialization code for biconnected input.
	//! Returns an adjacency entry that lies on the external face.
	virtual adjEntry trivialInit(Graph &G) {
		NodeArray<int> m_nodeLength(G, 0);
		EdgeArray<int> m_edgeLength(G, 0);
		adjEntry m_adjExternal;
		BicompEmbedder::embed(G, m_adjExternal, m_nodeLength, m_edgeLength);

		return m_adjExternal->twin();
	}

	//! Initializes #pBCTree and returns the root node of this tree
	//! or \c nullptr if \p G is biconnected.
	node initBCTree(Graph &G) {
		node result = nullptr;

		// HINT: Edges are directed from child to parent in BC-trees
		pBCTree = new BCTree(G);

		// base case of biconnected graph
		if (pBCTree->bcTree().numberOfNodes() == 1) {
			*pAdjExternal = trivialInit(G);
			delete pBCTree;
		} else {
			// Find root Block (only node with out-degree of 0):
			for (node v : pBCTree->bcTree().nodes) {
				if (v->outdeg() == 0) {
					result = v;
					break;
				}
			}

			OGDF_ASSERT(result != nullptr);
		}

		return result;
	}
};

}}
