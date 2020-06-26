/** \file
 * \brief Declares ogdf::EmbedderMaxFace.
 *
 * \author Thorsten Kerkhof
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

#include <ogdf/planarity/embedder/EmbedderBCTreeBase.h>
#include <ogdf/planarity/embedder/EmbedderMaxFaceBiconnectedGraphs.h>
#include <ogdf/decomposition/StaticSPQRTree.h>

namespace ogdf {

//! Embedder that maximizes the external face.
/**
 * @ingroup ga-planembed
 *
 * See the paper "Graph Embedding with Minimum Depth and Maximum External Face"
 * by C. Gutwenger and P. Mutzel (2004) for details.
 */
class OGDF_EXPORT EmbedderMaxFace : public embedder::EmbedderBCTreeBase<false>
{
public:
	/**
	 * \brief Computes an embedding of \p G with maximum external face.
	 * \param G is the original graph. Its adjacency list has to be  changed by the embedder.
	 * \param adjExternal is assigned an adjacency entry on the external face and has to be set by the embedder.
	 */
	virtual void doCall(Graph& G, adjEntry& adjExternal) override;

protected:
	//! Calls \p fun for every ingoing edge (\a w,\p v).
	void forEachIngoingNeighbor(node v, std::function<void(node)> fun) {
		for (adjEntry adj : v->adjEntries) {
			if (adj->theEdge()->target() == v) {
				fun(adj->twinNode());
			}
		}
	}

	void computeNodeLength(node bT, std::function<int &(node)> setter) {
		forEachIngoingNeighbor(bT, [&](node vT) {
			node vH = pBCTree->cutVertex(vT, bT);
			int length_v_in_block = 0;

			// set length of vertex v in block graph of bT:
			forEachIngoingNeighbor(vT, [&](node bT2) {
				node cutVertex = pBCTree->cutVertex(vT, bT2);
				length_v_in_block += constraintMaxFace(bT2, cutVertex);
			});

			setter(vH) = length_v_in_block;
		});
	}

	void internalMaximumFaceRec(
			const node& bT,
			node& bT_opt,
			int& ell_opt,
			Graph &blockGraph,
			NodeArray<int> &paramNodeLength,
			StaticSPQRTree *spqrTree,
			std::function<node &(node)> getBENode,
			std::function<int &(node, node)> getCstrLength,
			std::function<int &(node, node)> getNodeLength,
			int * const maxFaceSizeToUpdate = nullptr) {
		node tmp_bT_opt = bT;
		NodeArray<EdgeArray<int>> edgeLengthSkel;
		EdgeArray<int> edgeLengthForEllOpt(blockGraph, 1);

		int tmp_ell_opt = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
				blockGraph,
				paramNodeLength,
				edgeLengthForEllOpt,
				spqrTree,
				edgeLengthSkel);

		if (maxFaceSizeToUpdate != nullptr) {
			*maxFaceSizeToUpdate = tmp_ell_opt;
		}

		forEachIngoingNeighbor(bT, [&](node cT) {
			node cH = pBCTree->cutVertex(cT, bT);

			EdgeArray<int> uniformLengths;

			if (maxFaceSizeToUpdate == nullptr) {
				uniformLengths.init(blockGraph, 1);
			}

			getCstrLength(bT, cH) = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
					blockGraph,
					getBENode(cH),
					paramNodeLength,
					maxFaceSizeToUpdate == nullptr ? uniformLengths : edgeLengthForEllOpt,
					spqrTree,
					edgeLengthSkel);

			int L = 0;

			// L := \sum_{(B', c) \in bcTree} cstrLength(B', c)
			forEachIngoingNeighbor(cT, [&](node bT2) {
				// get partner vertex of c in the block graph of B'=e->target() and add cstrLength(B', c) to L:
				L += getCstrLength(bT2, pBCTree->cutVertex(cT, bT2));
			});

			forEachIngoingNeighbor(cT, [&](node pT) {
				if (pT != bT) {
					// get partner vertex of c in the block graph of B'=e->source():
					node partnerV = pBCTree->cutVertex(cT, pT);
					getNodeLength(pT, partnerV) = L - getCstrLength(pT, partnerV);

					// pBCTree->originalGraph().chooseNode() just to get rid of warning:
					node thisbT_opt = pBCTree->originalGraph().chooseNode();
					int thisell_opt = 0;
					maximumFaceRec(pT, thisbT_opt, thisell_opt);

					if (thisell_opt > tmp_ell_opt) {
						tmp_bT_opt = thisbT_opt;
						tmp_ell_opt = thisell_opt;
					}
				}
			});
		});

		// return (B*, \ell*):
		bT_opt = tmp_bT_opt;
		ell_opt = tmp_ell_opt;
	}

	template<typename T>
	void internalEmbedBlock(
			const node bT,
			const node cT,
			ListIterator<adjEntry>& after,
			Graph &blockGraph,
			NodeArray<T> &paramNodeLength,
			EdgeArray<T> &paramEdgeLength,
			NodeArray<node> &mapNodeToH,
			EdgeArray<edge> &mapEdgeToH,
			const node nodeInBlock) {
		// 1. Compute embedding of block
		adjEntry m_adjExternal = nullptr;
		EmbedderMaxFaceBiconnectedGraphs<T>::embed(blockGraph, m_adjExternal, paramNodeLength, paramEdgeLength, nodeInBlock);

		// 2. Copy block embedding into graph embedding and call recursively
		//    embedBlock for all cut vertices in bT
		CombinatorialEmbedding CE(blockGraph);
		face f = CE.leftFace(m_adjExternal);

		if (*pAdjExternal == nullptr) {
			node on = pBCTree->original(mapNodeToH[m_adjExternal->theNode()]);

			for (adjEntry ae : on->adjEntries) {
				if (ae->theEdge() == pBCTree->original(mapEdgeToH[m_adjExternal->theEdge()])) {
					*pAdjExternal = ae->twin();
					break;
				}
			}
		}

		for (node nSG : blockGraph.nodes) {
			node nH = mapNodeToH[nSG];
			node nG = pBCTree->original(nH);
			adjEntry ae = nSG->firstAdj();
			ListIterator<adjEntry>* pAfter = pBCTree->bcproper(nG) == cT ? &after : new ListIterator<adjEntry>;

			if (pBCTree->typeOfGNode(nG) == BCTree::GNodeType::CutVertex) {
				node cT2 = pBCTree->bcproper(nG);
				bool doRecurse = true;

				if (cT2 == cT) {
					node parent_bT_of_cT2 = nullptr;

					for (adjEntry adj : cT2->adjEntries) {
						if (adj->theEdge()->source() == cT2) {
							parent_bT_of_cT2 = adj->twinNode();
							break;
						}
					}

					OGDF_ASSERT(parent_bT_of_cT2 != nullptr);

					if (treeNodeTreated[parent_bT_of_cT2]) {
						doRecurse = false;
					}
				}

				// (if exists) find adjacency entry of nSG which lies on external face f:
				for (adjEntry aeFace : f->entries) {
					if (aeFace->theNode() == nSG) {
						ae = aeFace->succ() == nullptr ? nSG->firstAdj() : aeFace->succ();
						break;
					}

				}

				if (doRecurse) {
					for (adjEntry adj : cT2->adjEntries) {
						node bT2 = adj->theEdge()->opposite(cT2);

						if (!treeNodeTreated[bT2]) {
							embedBlock(bT2, cT2, *pAfter);
						}
					}
				}
			}

			// embed all edges of block bT:
			bool after_ae = true;
			for (adjEntry aeNode = ae; after_ae || aeNode != ae; aeNode = aeNode->succ() == nullptr ? nSG->firstAdj() : aeNode->succ()) {
				edge e = pBCTree->original(mapEdgeToH[aeNode->theEdge()]);
				if (nG == e->source()) {
					if (pAfter->valid()) {
						*pAfter = newOrder[nG].insertAfter(e->adjSource(), *pAfter);
					} else {
						*pAfter = newOrder[nG].pushBack(e->adjSource());
					}
				} else {
					if (pAfter->valid()) {
						*pAfter = newOrder[nG].insertAfter(e->adjTarget(), *pAfter);
					} else {
						*pAfter = newOrder[nG].pushBack(e->adjTarget());
					}
				}

				after_ae &= aeNode->succ() != nullptr;
			}

			if (*pAfter != after) {
				delete pAfter;
			}
		}
	}

	/**
	 * \brief Computes recursively the block graph for every block.
	 *
	 * \param bT is a block node in the BC-tree.
	 * \param cH is a node of bT in the block graph.
	 */
	void computeBlockGraphs(const node& bT, const node& cH);

	/**
	 * \brief Bottom up traversal of BC-tree.
	 *
	 * \param bT is the BC-tree node treated in this function call.
	 * \param cH is the block node which is related to the cut vertex which is
	 *   parent of bT in BC-tree.
	 */
	virtual int constraintMaxFace(const node& bT, const node& cH);

	/**
	 * \brief Top down traversal of BC-tree.
	 *
	 * \param bT is the tree node treated in this function call.
	 * \param bT_opt is assigned a block node in BC-tree which contains a face which
	 *   cann be expanded to a maximum face.
	 * \param ell_opt is the size of a maximum face.
	 */
	virtual void maximumFaceRec(const node& bT, node& bT_opt, int& ell_opt);

	/**
	 * \brief Computes the adjacency list for all nodes in a block and calls
	 * recursively the function for all blocks incident to nodes in bT.
	 *
	 * \param bT is the tree node treated in this function call.
	 */
	void embedBlock(const node& bT);

	/**
	 * \brief Computes the adjacency list for all nodes in a block and calls
	 * recursively the function for all blocks incident to nodes in bT.
	 *
	 * \param bT is the tree node treated in this function call.
	 * \param cT is the parent cut vertex node of bT in the BC-tree. cT is 0 if bT
	 *   is the root block.
	 * \param after is the adjacency entry of the cut vertex, after which bT has to
	 *   be inserted.
	 */
	virtual void embedBlock(const node& bT, const node& cT, ListIterator<adjEntry>& after);

	/** all blocks */
	NodeArray<Graph> blockG;

	/** a mapping of nodes in the auxiliaryGraph of the BC-tree to blockG */
	NodeArray< NodeArray<node> > nH_to_nBlockEmbedding;

	/** a mapping of edges in the auxiliaryGraph of the BC-tree to blockG */
	NodeArray< EdgeArray<edge> > eH_to_eBlockEmbedding;

	/** a mapping of nodes in blockG to the auxiliaryGraph of the BC-tree */
	NodeArray< NodeArray<node> > nBlockEmbedding_to_nH;

	/** a mapping of edges in blockG to the auxiliaryGraph of the BC-tree */
	NodeArray< EdgeArray<edge> > eBlockEmbedding_to_eH;

	/** saving for each node in the block graphs its length */
	NodeArray< NodeArray<int> > nodeLength;

	/** is saving for each node in the block graphs its cstrLength */
	NodeArray< NodeArray<int> > cstrLength;

	/** saves for every node of PG the new adjacency list */
	NodeArray< List<adjEntry> > newOrder;

	/** treeNodeTreated saves for all block nodes in the
	 *  BC-tree if it has already been treated or not. */
	NodeArray<bool> treeNodeTreated;

	/** The SPQR-trees of the blocks */
	NodeArray<StaticSPQRTree*> spqrTrees;
};

}
