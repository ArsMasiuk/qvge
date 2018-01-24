/** \file
 * \brief Computes an embedding of a graph with maximum external face.
 * See paper "Graph Embedding with Minimum Depth and Maximum External
 * Face" by C. Gutwenger and P. Mutzel (2004) for details.
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

#include <ogdf/planarity/EmbedderMaxFace.h>
#include <ogdf/planarity/embedder/ConnectedSubgraph.h>
#include <ogdf/planarity/embedder/EmbedderMaxFaceBiconnectedGraphs.h>

namespace ogdf {

void EmbedderMaxFace::doCall(Graph& G, adjEntry& adjExternal)
{
	adjExternal = nullptr;
	pAdjExternal = &adjExternal;
	node rootBlockNode = initBCTree(G);

	if(rootBlockNode == nullptr) {
		return;
	}

	//First step: calculate maximum face and node lengths

	//compute block graphs and SPQR trees:
	blockG.init(pBCTree->bcTree());
	nBlockEmbedding_to_nH.init(pBCTree->bcTree());
	eBlockEmbedding_to_eH.init(pBCTree->bcTree());
	nH_to_nBlockEmbedding.init(pBCTree->bcTree());
	eH_to_eBlockEmbedding.init(pBCTree->bcTree());
	nodeLength.init(pBCTree->bcTree());
	cstrLength.init(pBCTree->bcTree());
	spqrTrees.init(pBCTree->bcTree(),nullptr);
	computeBlockGraphs(rootBlockNode, nullptr);

	//Bottom-Up-Traversal:
	for(adjEntry adj : rootBlockNode->adjEntries) {
		edge e = adj->theEdge();
		node cT = e->source();
		node cH = pBCTree->cutVertex(cT, rootBlockNode);
		node cB = nH_to_nBlockEmbedding[rootBlockNode][cH];

		//set length of v in block graph of root block node:
		int length_v_in_rootBlock = 0;
		for(adjEntry adjCT : cT->adjEntries) {
			edge e2 = adjCT->theEdge();
			//check if edge is an incoming edge:
			if (e2->target() != cT)
				continue;

			node blockNode = e2->source();
			node cutVertex = pBCTree->cutVertex(cT, blockNode);
			length_v_in_rootBlock += constraintMaxFace(blockNode, cutVertex);
		}
		nodeLength[rootBlockNode][cB] = length_v_in_rootBlock;
	}

	node bT_opt = G.chooseNode(); //= G.chooseNode() only to get rid of warning
	int ell_opt = 0;
	maximumFaceRec(rootBlockNode, bT_opt, ell_opt);


	//Second step: Embed G by expanding a maximum face in bT_opt
	newOrder.init(G);
	treeNodeTreated.init(pBCTree->bcTree(), false);
	embedBlock(bT_opt);

	for(node v : G.nodes)
		G.sort(v, newOrder[v]);

	for(node v : pBCTree->bcTree().nodes)
		delete spqrTrees[v];

	delete pBCTree;
}


void EmbedderMaxFace::computeBlockGraphs(const node& bT, const node& cH)
{
	//recursion:
	for(adjEntry adj : bT->adjEntries) {
		edge e = adj->theEdge();
		if (e->source() == bT)
			continue;

		node cT = e->source();
		for(adjEntry adjCT : cT->adjEntries) {
			edge e2 = adjCT->theEdge();
			if (e2->source() == cT)
				continue;
			node cH2 = pBCTree->cutVertex(cT, e2->source());
			computeBlockGraphs(e2->source(), cH2);
		}
	}

	//embed block bT:
	node m_cH = cH;
	if (m_cH == nullptr)
		m_cH = pBCTree->cutVertex(bT->firstAdj()->twinNode(), bT);
	embedder::ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockG[bT], m_cH,
		nBlockEmbedding_to_nH[bT], eBlockEmbedding_to_eH[bT],
		nH_to_nBlockEmbedding[bT], eH_to_eBlockEmbedding[bT]);
	nodeLength[bT].init(blockG[bT], 0);
	cstrLength[bT].init(blockG[bT], 0);
	if (   !blockG[bT].empty()
		&& blockG[bT].numberOfNodes() != 1
		&& blockG[bT].numberOfEdges() > 2)
	{
		spqrTrees[bT] = new StaticSPQRTree(blockG[bT]);
	}
}


int EmbedderMaxFace::constraintMaxFace(const node& bT, const node& cH)
{
	computeNodeLength(bT, [&](node vH) -> int& { return nodeLength[bT][nH_to_nBlockEmbedding[bT][vH]]; });

	EdgeArray<int> edgeLength(blockG[bT], 1);
	int cstrLengthBc = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
		blockG[bT],
		nH_to_nBlockEmbedding[bT][cH],
		nodeLength[bT],
		edgeLength,
		spqrTrees[bT]);
	cstrLength[bT][nH_to_nBlockEmbedding[bT][cH]] = cstrLengthBc;
	return cstrLengthBc;
}


void EmbedderMaxFace::maximumFaceRec(const node& bT, node& bT_opt, int& ell_opt)
{
	internalMaximumFaceRec(
			bT, bT_opt, ell_opt,
			blockG[bT],
			nodeLength[bT],
			spqrTrees[bT],
			[&](node cH) -> node& { return  nH_to_nBlockEmbedding[bT][cH]; },
			[&](node v, node u) -> int& { return cstrLength[v][nH_to_nBlockEmbedding[v][u]]; },
			[&](node v, node u) -> int& { return nodeLength[v][nH_to_nBlockEmbedding[v][u]]; });
}


void EmbedderMaxFace::embedBlock(const node& bT)
{
	ListIterator<adjEntry> after;
	node cT = nullptr;
	embedBlock(bT, cT, after);
}


void EmbedderMaxFace::embedBlock(
	const node& bT,
	const node& cT,
	ListIterator<adjEntry>& after)
{
	treeNodeTreated[bT] = true;
	node cH = nullptr;
	if (cT != nullptr)
		cH = pBCTree->cutVertex(cT, bT);

	EdgeArray<int> edgeLength(blockG[bT], 1);
	internalEmbedBlock(bT, cT, after, blockG[bT], nodeLength[bT], edgeLength, nBlockEmbedding_to_nH[bT], eBlockEmbedding_to_eH[bT], cH == nullptr ? nullptr : nH_to_nBlockEmbedding[bT][cH]);
}

}
