/** \file
 * \brief Computes an embedding of a graph with minimum depth.
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

#include <limits>

#include <ogdf/planarity/EmbedderMinDepth.h>
#include <ogdf/planarity/embedder/EmbedderMaxFaceBiconnectedGraphs.h>
#include <ogdf/planarity/embedder/ConnectedSubgraph.h>

namespace ogdf {

void EmbedderMinDepth::doCall(Graph& G, adjEntry& adjExternal)
{
	adjExternal = nullptr;
	pAdjExternal = &adjExternal;
	node rootBlockNode = initBCTree(G);

	if(rootBlockNode == nullptr) {
		return;
	}

	//First step: calculate min depth and node lengths

	//compute block graphs:
	blockG.init(pBCTree->bcTree());
	nBlockEmbedding_to_nH.init(pBCTree->bcTree());
	eBlockEmbedding_to_eH.init(pBCTree->bcTree());
	nH_to_nBlockEmbedding.init(pBCTree->bcTree());
	eH_to_eBlockEmbedding.init(pBCTree->bcTree());
	nodeLength.init(pBCTree->bcTree());
	spqrTrees.init(pBCTree->bcTree(),nullptr);
	computeBlockGraphs(rootBlockNode, nullptr);

	//Edge lengths of BC-tree, values m_{c, B} for all (c, B) \in bcTree:
	m_cB.init(pBCTree->bcTree(), 0);

	//Bottom-up traversal: (set m_cB for all {c, B} \in bcTree)
	nodeLength[rootBlockNode].init(blockG[rootBlockNode], 0);
	for(adjEntry adj : rootBlockNode->adjEntries) {
		edge e = adj->theEdge();
		node cT = e->source();
		//node cH = pBCTree->cutVertex(cT, rootBlockNode);

		//set length of c in block graph of root block node:
		for(adjEntry adjCT : cT->adjEntries) {
			edge e2 = adjCT->theEdge();
			if (e2->target() != cT)
				continue;

			node blockNode = e2->source();
			node cutVertex = pBCTree->cutVertex(cT, blockNode);

			//Start recursion:
			m_cB[e2] = bottomUpTraversal(blockNode, cutVertex);
		}
	}

	//Top-down traversal: (set m_cB for all {B, c} \in bcTree and get min depth
	//for each block)
	int maxint = std::numeric_limits<int>::max();
	minDepth.init(pBCTree->bcTree(), maxint);
	M_B.init(pBCTree->bcTree());
	M2.init(pBCTree->bcTree());
	topDownTraversal(rootBlockNode);

	//compute bT_opt:
	int depth = maxint;
	node bT_opt;
	for(node n : pBCTree->bcTree().nodes)
	{
		if (pBCTree->typeOfBNode(n) != BCTree::BNodeType::BComp)
			continue;
		if (minDepth[n] < depth)
		{
			depth = minDepth[n];
			bT_opt = n;
		}
	}

	//Second step: Embed G by expanding a maximum face in bT_opt
	newOrder.init(G);
	treeNodeTreated.init(pBCTree->bcTree(), false);
	embedBlock(bT_opt);

	for(node n : G.nodes)
		G.sort(n, newOrder[n]);

	for(node n : pBCTree->bcTree().nodes)
		delete spqrTrees[n];

	delete pBCTree;
}


void EmbedderMinDepth::computeBlockGraphs(const node& bT, const node& cH)
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

	if (!blockG[bT].empty()
		&& blockG[bT].numberOfNodes() != 1
		&& blockG[bT].numberOfEdges() > 2)
	{
		spqrTrees[bT] = new StaticSPQRTree(blockG[bT]);
	}
}


int EmbedderMinDepth::bottomUpTraversal(const node& bT, const node& cH)
{
	int m_B = 0; //max_{c \in B} m_B(c)
	List<node> cInBWithProperty; //{c \in B | m_B(c) = m_B}

	//Recursion:
	for(adjEntry adj : bT->adjEntries) {
		edge e = adj->theEdge();
		if (e->target() != bT)
			continue;
		node cT = e->source();
		//node c_in_bT = pBCTree->cutVertex(cT, bT);

		//set length of c in block graph of root block node:
		for(adjEntry adjCT : cT->adjEntries) {
			edge e_cT_bT2 = adjCT->theEdge();
			if (e == e_cT_bT2)
				continue;

			node bT2 = e_cT_bT2->source();
			node c_in_bT2 = pBCTree->cutVertex(cT, bT2);
			m_cB[e_cT_bT2] = bottomUpTraversal(bT2, c_in_bT2);

			//update m_B and cInBWithProperty:
			if (m_B < m_cB[e_cT_bT2])
			{
				node cV_in_bT = pBCTree->cutVertex(cT, bT);
				m_B = m_cB[e_cT_bT2];
				cInBWithProperty.clear();
				cInBWithProperty.pushBack(cV_in_bT);
			}
			else if (m_B == m_cB[e_cT_bT2] && !cInBWithProperty.search(pBCTree->cutVertex(cT, bT)).valid())
			{
				node cV_in_bT = pBCTree->cutVertex(cT, bT);
				cInBWithProperty.pushBack(cV_in_bT);
			}
		}
	}

	//set vertex length for all vertices in bH to 1 if vertex is in cInBWithProperty:
	nodeLength[bT].init(blockG[bT], 0);
	for (ListIterator<node> iterator = cInBWithProperty.begin(); iterator.valid(); ++iterator)
		nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 1;

	//leafs of BC-tree:
	if (cInBWithProperty.size() == 0)
		return 1;

	//set edge length for all edges in block graph to 0:
	EdgeArray<int> edgeLength(blockG[bT], 0);

	//compute maximum external face of block graph and get its size:
	int cstrLength_B_c = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
		blockG[bT],
		nH_to_nBlockEmbedding[bT][cH],
		nodeLength[bT],
		edgeLength,
		spqrTrees[bT]);

	if (cstrLength_B_c == cInBWithProperty.size())
		return m_B;
	//else:
	return m_B + 2;
}


void EmbedderMinDepth::topDownTraversal(const node& bT)
{
	//m_B(c) = max {0} \cup {m_{c, B'} | c \in B', B' \neq B}
	int m_B = 0; //max_{c \in B} m_B(c)

	//Compute m_B and M_B:
	node cT_parent = nullptr;

	for(adjEntry adj : bT->adjEntries) {
		edge e_bT_cT = adj->theEdge();
		if (e_bT_cT->source() == bT)
			cT_parent = e_bT_cT->target();
		node cT = (e_bT_cT->source() == bT) ? e_bT_cT->target() : e_bT_cT->source();
		for(adjEntry adjCT : cT->adjEntries) {
			edge e_cT_bT2 = adjCT->theEdge();
			if (e_cT_bT2 == e_bT_cT)
				continue;

			//update m_B and M_B:
			if (m_B < m_cB[e_cT_bT2])
			{
				m_B = m_cB[e_cT_bT2];
				M_B[bT].clear();
				M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
			}
			else if (m_B == m_cB[e_cT_bT2] && !M_B[bT].search(pBCTree->cutVertex(cT, bT)).valid())
			{
				M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
			}
		}
	}
	//set vertex length for all vertices in bH to 1 if vertex is in M_B:
	nodeLength[bT].fill(0);
	NodeArray<int> m_nodeLength(blockG[bT], 0);
	for (ListIterator<node> iterator = M_B[bT].begin(); iterator.valid(); ++iterator)
	{
		nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 1;
		m_nodeLength[nH_to_nBlockEmbedding[bT][*iterator]] = 1;
	}

	//set edge length for all edges in block graph to 0:
	EdgeArray<int> edgeLengthBlock(blockG[bT], 0);

	//compute size of a maximum external face of block graph:
	NodeArray< EdgeArray<int> > edgeLengthSkel;
	int cstrLength_B_c = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
		blockG[bT],
		m_nodeLength,
		edgeLengthBlock,
		spqrTrees[bT],
		edgeLengthSkel);

	//Prepare recursion by setting m_{c, B} for all edges {B, c} \in bcTree:
	if (M_B[bT].size() > 0)
	{
		node cT1 = pBCTree->bcproper(pBCTree->original(*(M_B[bT].begin())));
		bool calculateNewNodeLengths;
		calculateNewNodeLengths = M_B[bT].size() == 1 && cT1 == cT_parent;
		for(adjEntry adj : bT->adjEntries) {
			edge e_bT_cT = adj->theEdge();
			if (e_bT_cT->target() != bT)
				continue;
			node cT = e_bT_cT->source();
			node cH = pBCTree->cutVertex(cT, bT);

			if (M_B[bT].size() == 1 && cT1 == cT)
			{
				//Compute new vertex lengths according to
				//m2 = max_{v \in V_B, v != c} m_B(v) and
				//M2 = {c \in V_B \ {v} | m_B(c) = m2}.
				int m2 = 0;

				//Compute m2 and M2:
				for(adjEntry adjBT : bT->adjEntries) {
					edge e_bT_cT2 = adjBT->theEdge();
					node cT2 = (e_bT_cT2->source() == bT) ? e_bT_cT2->target() : e_bT_cT2->source();
					if (cT1 == cT2)
						continue;
					for(adjEntry adjCT2 : cT2->adjEntries) {
						edge e_cT2_bT2 = adjCT2->theEdge();
						if (e_cT2_bT2 == e_bT_cT2)
							continue;

						//update m_B and M_B:
						if (m2 < m_cB[e_cT2_bT2])
						{
							m2 = m_cB[e_cT2_bT2];
							M2[bT].clear();
							M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
						}
						else if (m2 == m_cB[e_cT2_bT2] && !M2[bT].search(pBCTree->cutVertex(cT2, bT)).valid())
						{
							M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
						}
					}
				}

				//set vertex length for all vertices in bH to 1 if vertex is in M2 and
				//0 otherwise:
				nodeLength[bT][nH_to_nBlockEmbedding[bT][*(M_B[bT].begin())]] = 0;
				for (ListIterator<node> iterator = M2[bT].begin(); iterator.valid(); ++iterator)
					nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 1;

				//set edge length for all edges in block graph to 0:
				EdgeArray<int> edgeLength(blockG[bT], 0);

				//compute a maximum external face size of a face containing c in block graph:
				int maxFaceSize = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
					blockG[bT],
					nH_to_nBlockEmbedding[bT][cH],
					nodeLength[bT],
					edgeLength,
					spqrTrees[bT]);
				if (M2[bT].size() == 0)
					m_cB[e_bT_cT] = 1;
				else
				{
					if (maxFaceSize == M2[bT].size())
						m_cB[e_bT_cT] = m2;
					else
						m_cB[e_bT_cT] = m2 + 2;
				}

				if (calculateNewNodeLengths)
					calculateNewNodeLengths = false;
				else
				{
					//reset node lengths:
					for (ListIterator<node> iterator = M2[bT].begin(); iterator.valid(); ++iterator)
						nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 0;
					nodeLength[bT][nH_to_nBlockEmbedding[bT][*(M_B[bT].begin())]] = 1;
				}
			}
			else //M_B.size() != 1
			{
				//Compute a maximum face in block B containing c using the vertex lengths
				//already assigned.

				//set edge length for all edges in block graph to 0:
				EdgeArray<int> edgeLength(blockG[bT], 0);

				//compute a maximum external face size of a face containing c in block graph:
				int maxFaceSize = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
					blockG[bT],
					nH_to_nBlockEmbedding[bT][cH],
					nodeLength[bT],
					edgeLength,
					spqrTrees[bT],
					edgeLengthSkel);
				if (M_B[bT].size() == 0)
					m_cB[e_bT_cT] = 1;
				else
				{
					if (maxFaceSize == M_B[bT].size())
						m_cB[e_bT_cT] = m_B;
					else
						m_cB[e_bT_cT] = m_B + 2;
				}
			}
		}

		if (calculateNewNodeLengths)
		{
			//Compute new vertex lengths according to
			//m2 = max_{v \in V_B, v != c} m_B(v) and
			//M2 = {c \in V_B \ {v} | m_B(c) = m2}.
			int m2 = 0;

			//Compute m2 and M2:
			for(adjEntry adj : bT->adjEntries) {
				edge e_bT_cT2 = adj->theEdge();
				node cT2 = (e_bT_cT2->source() == bT) ? e_bT_cT2->target() : e_bT_cT2->source();
				if (cT1 == cT2)
					continue;
				for(adjEntry adjCT2 : cT2->adjEntries) {
					edge e_cT2_bT2 = adjCT2->theEdge();
					if (e_cT2_bT2 == e_bT_cT2)
						continue;

					//update m_B and M_B:
					if (m2 < m_cB[e_cT2_bT2])
					{
						m2 = m_cB[e_cT2_bT2];
						M2[bT].clear();
						M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
					else if (m2 == m_cB[e_cT2_bT2] && !M2[bT].search(pBCTree->cutVertex(cT2, bT)).valid())
					{
						M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
				}
			}

			//set vertex length for all vertices in bH to 1 if vertex is in M2 and
			//0 otherwise:
			nodeLength[bT][nH_to_nBlockEmbedding[bT][*(M_B[bT].begin())]] = 0;
			for (ListIterator<node> iterator = M2[bT].begin(); iterator.valid(); ++iterator)
				nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 1;
		} else if (M_B[bT].size() == 1) {
			//Compute M2 = {c \in V_B \ {v} | m_B(c) = m2} with
			//m2 = max_{v \in V_B, v != c} m_B(v).
			int m2 = 0;
			for(adjEntry adj : bT->adjEntries) {
				edge e_bT_cT2 = adj->theEdge();
				node cT2 = (e_bT_cT2->source() == bT) ? e_bT_cT2->target() : e_bT_cT2->source();
				if (cT1 == cT2)
					continue;
				for(adjEntry adjCT2 : cT2->adjEntries) {
					edge e_cT2_bT2 = adjCT2->theEdge();
					if (e_cT2_bT2 == e_bT_cT2)
						continue;

					//update m_B and M_B:
					if (m2 < m_cB[e_cT2_bT2])
					{
						m2 = m_cB[e_cT2_bT2];
						M2[bT].clear();
						M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
					else if (m2 == m_cB[e_cT2_bT2] && !M2[bT].search(pBCTree->cutVertex(cT2, bT)).valid())
					{
						M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
				}
			}
		}
	}

	//Recursion:
	for(adjEntry adj : bT->adjEntries) {
		edge e_bT_cT = adj->theEdge();
		if (e_bT_cT->target() != bT)
			continue;

		node cT = e_bT_cT->source();
		for(adjEntry adjCT : cT->adjEntries) {
			edge e_cT_bT2 = adjCT->theEdge();
			if (e_cT_bT2 == e_bT_cT)
				continue;

			topDownTraversal(e_cT_bT2->source());
		}
	}

	//Compute M_B and M2 for embedBlock-function:
	{
		M_B[bT].clear();
		M2[bT].clear();
		m_B = 0;
		for(adjEntry adj : bT->adjEntries) {
			edge e_bT_cT = adj->theEdge();
			node cT = (e_bT_cT->source() == bT) ? e_bT_cT->target() : e_bT_cT->source();
			for(adjEntry adjCT : cT->adjEntries) {
				edge e_cT_bT2 = adjCT->theEdge();
				if (e_bT_cT == e_cT_bT2)
					continue;

				//update m_B and M_B:
				if (m_B < m_cB[e_cT_bT2])
				{
					m_B = m_cB[e_cT_bT2];
					M_B[bT].clear();
					M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
				}
				else if (m_B == m_cB[e_cT_bT2] && !M_B[bT].search(pBCTree->cutVertex(cT, bT)).valid())
				{
					M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
				}
			}
		}

		if (M_B[bT].size() == 1)
		{
			int m2 = 0;
			node cT1 = pBCTree->bcproper(pBCTree->original(*(M_B[bT].begin())));
			for(adjEntry adj : bT->adjEntries) {
				edge e_bT_cT = adj->theEdge();
				node cT2 = (e_bT_cT->source() == bT) ? e_bT_cT->target() : e_bT_cT->source();
				if (cT1 == cT2)
					continue;
				node cT = (e_bT_cT->source() == bT) ? e_bT_cT->target() : e_bT_cT->source();
				for(adjEntry adjCT : cT->adjEntries) {
					edge e_cT_bT2 = adjCT->theEdge();
					//update m2 and M2:
					if (m2 < m_cB[e_cT_bT2])
					{
						m2 = m_cB[e_cT_bT2];
						M2[bT].clear();
						M2[bT].pushBack(pBCTree->cutVertex(cT, bT));
					}
					else if (m2 == m_cB[e_cT_bT2]
									 && !M2[bT].search(pBCTree->cutVertex(cT, bT)).valid())
					{
						M2[bT].pushBack(pBCTree->cutVertex(cT, bT));
					}
				}
			}
		}
	}

	if (cstrLength_B_c == M_B[bT].size())
		minDepth[bT] = m_B;
	else
		minDepth[bT] = m_B + 2;
}


void EmbedderMinDepth::embedBlock(const node& bT)
{
	ListIterator<adjEntry> after;
	node cT = nullptr;
	embedBlock(bT, cT, after);
}


void EmbedderMinDepth::embedBlock(
	const node& bT,
	const node& cT,
	ListIterator<adjEntry>& after)
{
	treeNodeTreated[bT] = true;
	node cH = nullptr;
	if (cT != nullptr)
		cH = pBCTree->cutVertex(cT, bT);

	// 1. Compute node lengths depending on M_B, M2 and cT
	nodeLength[bT].fill(0);
	if (cT != nullptr && M_B[bT].size() == 1 && *(M_B[bT].begin()) == cH)
	{
		//set node length to 1 if node is in M2 and 0 otherwise
		for (ListIterator<node> iterator = M2[bT].begin(); iterator.valid(); ++iterator)
			nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 1;
	}
	else
	{
		//set node length to 1 if node is in M_B and 0 otherwise
		for (ListIterator<node> iterator = M_B[bT].begin(); iterator.valid(); ++iterator)
			nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 1;
	}

	// 2. Compute embedding of block
	EdgeArray<int> edgeLength(blockG[bT], 0);
	adjEntry m_adjExternal = nullptr;
	if (cH == nullptr)
		EmbedderMaxFaceBiconnectedGraphs<int>::embed(blockG[bT], m_adjExternal, nodeLength[bT], edgeLength);
	else
		EmbedderMaxFaceBiconnectedGraphs<int>::embed(blockG[bT], m_adjExternal, nodeLength[bT], edgeLength,
			nH_to_nBlockEmbedding[bT][cH]);

	// 3. Copy block embedding into graph embedding and call recursively
	//    embedBlock for all cut vertices in bT
	CombinatorialEmbedding CE(blockG[bT]);
	face f = CE.leftFace(m_adjExternal);

	if (*pAdjExternal == nullptr)
	{
		node on = pBCTree->original(nBlockEmbedding_to_nH[bT][m_adjExternal->theNode()]);
		adjEntry ae1 = on->firstAdj();
		for (adjEntry ae = ae1; ae; ae = ae->succ())
		{
			if (ae->theEdge() == pBCTree->original(eBlockEmbedding_to_eH[bT][m_adjExternal->theEdge()]))
			{
				*pAdjExternal = ae->twin();
				break;
			}
		}
	}

	for(node nSG : blockG[bT].nodes)
	{
		node nH = nBlockEmbedding_to_nH[bT][nSG];
		node nG = pBCTree->original(nH);
		adjEntry ae = nSG->firstAdj();
		ListIterator<adjEntry>* pAfter;
		if (pBCTree->bcproper(nG) == cT)
			pAfter = &after;
		else
			pAfter = new ListIterator<adjEntry>();

		if (pBCTree->typeOfGNode(nG) == BCTree::GNodeType::CutVertex)
		{
			node cT2 = pBCTree->bcproper(nG);
			bool no_recursion = false;
			if (cT2 == cT)
			{
				node parent_bT_of_cT2 = nullptr;
				for(adjEntry adj : cT2->adjEntries) {
					edge e_cT2_to_bT2 = adj->theEdge();
					if (e_cT2_to_bT2->source() == cT2)
					{
						parent_bT_of_cT2 = e_cT2_to_bT2->target();
						break;
					}
				}
				OGDF_ASSERT(parent_bT_of_cT2 != nullptr);
				if (treeNodeTreated[parent_bT_of_cT2])
					no_recursion = true;
			}

			if (no_recursion)
			{
				//find adjacency entry of nSG which lies on external face f:
				adjEntry aeFace = f->firstAdj();
				do
				{
					if (aeFace->theNode() == nSG)
					{
						if (aeFace->succ())
							ae = aeFace->succ();
						else
							ae = nSG->firstAdj();
						break;
					}
					aeFace = aeFace->faceCycleSucc();
				} while(aeFace != f->firstAdj());
			}
			else //!no_recursion
			{
				// (if exists) find adjacency entry of nSG which lies on external face f:
#if 0
				bool aeExtExists = false;
				List<edge> extFaceEdges;
#endif
				adjEntry aeFace = f->firstAdj();
				do
				{
#if 0
					extFaceEdges.pushBack(aeFace->theEdge());
#endif
					if (aeFace->theNode() == nSG)
					{
						if (aeFace->succ())
							ae = aeFace->succ();
						else
							ae = nSG->firstAdj();
#if 0
						aeExtExists = true;
#endif
						break;
					}
					aeFace = aeFace->faceCycleSucc();
				} while(aeFace != f->firstAdj());

#if 0
				if (aeExtExists)
#endif
				{
					for(adjEntry adj : cT2->adjEntries) {
						edge e_cT2_to_bT2 = adj->theEdge();
						node bT2;
						if (e_cT2_to_bT2->source() == cT2)
							bT2 = e_cT2_to_bT2->target();
						else
							bT2 = e_cT2_to_bT2->source();
						if (!treeNodeTreated[bT2])
							embedBlock(bT2, cT2, *pAfter);
					}
				}
#if 0
				else
				{
					//cannot embed block into external face, so find a face with an adjacent
					//edge of the external face:
					bool foundIt = false;
					edge adjEdge;
					forall_adj_edges(adjEdge, nSG)
					{
						face m_f = CE.leftFace(adjEdge->adjSource());
						adjEntry aeF = m_f->firstAdj();
						do
						{
							if (extFaceEdges.search(aeF->theEdge()).valid())
							{
								ae = adjEdge->adjSource();
								foundIt = true;
								break;
							}
							aeF = aeF->faceCycleSucc();
						} while(aeF != m_f->firstAdj());
						if (foundIt)
							break;
					}
				}
#endif
			}
		}

		//embed all edges of block bT:
		bool after_ae = true;
		for (adjEntry aeNode = ae;
			after_ae || aeNode != ae;
			after_ae = after_ae && aeNode->succ(),
			aeNode = aeNode->succ() ? aeNode->succ() : nSG->firstAdj())
		{
			edge eG = pBCTree->original(eBlockEmbedding_to_eH[bT][aeNode->theEdge()]);
			if (nG == eG->source())
			{
				if (!pAfter->valid())
					*pAfter = newOrder[nG].pushBack(eG->adjSource());
				else
					*pAfter = newOrder[nG].insertAfter(eG->adjSource(), *pAfter);
			}
			else //!(nG == eG->source())
			{
				if (!pAfter->valid())
					*pAfter = newOrder[nG].pushBack(eG->adjTarget());
				else
					*pAfter = newOrder[nG].insertAfter(eG->adjTarget(), *pAfter);
			}
		}

		if (!(*pAfter == after))
			delete pAfter;
	}
}

}
