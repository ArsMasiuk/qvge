/** \file
 * \brief Computes an embedding of a graph with minimum depth and
 * maximum external face. See paper "Graph Embedding with Minimum
 * Depth and Maximum External Face" by C. Gutwenger and P. Mutzel
 * (2004) for details.
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

#include <ogdf/planarity/EmbedderMinDepthMaxFace.h>
#include <ogdf/planarity/embedder/ConnectedSubgraph.h>
#include <ogdf/planarity/embedder/EmbedderMaxFaceBiconnectedGraphs.h>

namespace ogdf {

void EmbedderMinDepthMaxFace::doCall(Graph& G, adjEntry& adjExternal)
{
	int maxint = std::numeric_limits<int>::max();

	adjExternal = nullptr;
	pAdjExternal = &adjExternal;
	node rootBlockNode = initBCTree(G);

	if(rootBlockNode == nullptr) {
		return;
	}


	// First step: calculate min depth and node lengths


	/* MIN DEPTH                                                                */

	//Node lengths of block graph:
	md_nodeLength.init(pBCTree->auxiliaryGraph(), 0);

	//Edge lengths of BC-tree, values m_{c, B} for all (c, B) \in bcTree:
	cB.init(pBCTree->bcTree(), 0);

	//Bottom-up traversal: (set m_cB for all {c, B} \in bcTree)
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
			cB[e2] = bottomUpTraversal(blockNode, cutVertex);
		}
	}

	//Top-down traversal: (set m_cB for all {B, c} \in bcTree and get min depth
	//for each block)
	md_nodeLength.fill(0);
	minDepth.init(pBCTree->bcTree(), maxint);
	md_M_B.init(pBCTree->bcTree());
	M2.init(pBCTree->bcTree());
	topDownTraversal(rootBlockNode);


	/* MAX FACE                                                                 */

	mf_cstrLength.init(pBCTree->auxiliaryGraph(), 0);
	mf_nodeLength.init(pBCTree->auxiliaryGraph(), 0);
	maxFaceSize.init(pBCTree->bcTree(), 0);

	//Bottom-Up-Traversal:
	for(adjEntry adj : rootBlockNode->adjEntries) {
		edge e = adj->theEdge();
		node cT = e->source();
		node cH = pBCTree->cutVertex(cT, rootBlockNode);

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
		mf_nodeLength[cH] = length_v_in_rootBlock;
	}

	node mf_bT_opt = G.chooseNode(); //= G.chooseNode() only to get rid of warning
	int mf_ell_opt = 0;
	maximumFaceRec(rootBlockNode, mf_bT_opt, mf_ell_opt);


	/* MIN DEPTH + MAX FACE                                                     */

	//compute bT_opt:
	edgeLength.init(pBCTree->auxiliaryGraph(), MDMFLengthAttribute(0, 1));
	mdmf_nodeLength.init(pBCTree->auxiliaryGraph(), MDMFLengthAttribute(0, 0));
	int d_opt = maxint;
	int ell_opt = -1;
	node bT_opt;
	for(node bT : pBCTree->bcTree().nodes)
	{
		if (pBCTree->typeOfBNode(bT) != BCTree::BNodeType::BComp)
			continue;
		if (minDepth[bT] < d_opt
			|| (minDepth[bT] == d_opt && maxFaceSize[bT] > ell_opt))
		{
			d_opt = minDepth[bT];
			ell_opt = maxFaceSize[bT];
			bT_opt = bT;
		}
	}

	// Second step: Embed G by expanding a maximum face in bT_opt

	newOrder.init(G);
	treeNodeTreated.init(pBCTree->bcTree(), false);
	//reset md_nodeLength and set them during embedBlock call, because they are
	//calculated for starting embedding with rootBlockNode, which is not
	//guarenteed
	md_nodeLength.fill(0);
	embedBlock(bT_opt);

	for(node n : G.nodes)
		G.sort(n, newOrder[n]);

	delete pBCTree;
}


int EmbedderMinDepthMaxFace::bottomUpTraversal(const node &bT, const node &cH)
{
	int m_B = 0; //max_{c \in B} m_B(c)
	List<node> M_B; //{c \in B | m_B(c) = m_B}

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
			cB[e_cT_bT2] = bottomUpTraversal(bT2, c_in_bT2);

			//update m_B and M_B:
			if (m_B < cB[e_cT_bT2])
			{
				node cV_in_bT = pBCTree->cutVertex(cT, bT);
				m_B = cB[e_cT_bT2];
				M_B.clear();
				M_B.pushBack(cV_in_bT);
			}
			else if (m_B == cB[e_cT_bT2] && !M_B.search(pBCTree->cutVertex(cT, bT)).valid())
			{
				node cV_in_bT = pBCTree->cutVertex(cT, bT);
				M_B.pushBack(cV_in_bT);
			}
		}
	}

	//set vertex length for all vertices in bH to 1 if vertex is in M_B:
	for (ListIterator<node> iterator = M_B.begin(); iterator.valid(); ++iterator)
		md_nodeLength[*iterator] = 1;

	//generate block graph of bT:
	Graph blockGraph_bT;
	node cInBlockGraph_bT;
	NodeArray<int> nodeLengthSG(blockGraph_bT);
	embedder::ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockGraph_bT, cH,
		cInBlockGraph_bT, md_nodeLength, nodeLengthSG);

	//leafs of BC-tree:
	if (M_B.size() == 0)
		return 1;

	//set edge length for all edges in block graph to 0:
	EdgeArray<int> zeroEdgeLength(blockGraph_bT, 0);

	//compute maximum external face of block graph and get its size:
	int cstrLength_B_c
		= EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
			blockGraph_bT, cInBlockGraph_bT, nodeLengthSG, zeroEdgeLength);

	if (cstrLength_B_c == M_B.size())
		return m_B;
	//else:
	return m_B + 2;
}


void EmbedderMinDepthMaxFace::topDownTraversal(const node &bT)
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
			if (m_B < cB[e_cT_bT2])
			{
				m_B = cB[e_cT_bT2];
				md_M_B[bT].clear();
				md_M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
			}
			else if (m_B == cB[e_cT_bT2] && !md_M_B[bT].search(pBCTree->cutVertex(cT, bT)).valid())
			{
				md_M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
			}
		}
	}

	//set vertex length for all vertices in bH to 1 if vertex is in M_B:
	NodeArray<int> m_nodeLength(pBCTree->auxiliaryGraph(), 0);
	for (ListIterator<node> iterator = md_M_B[bT].begin(); iterator.valid(); ++iterator)
	{
		md_nodeLength[*iterator] = 1;
		m_nodeLength[*iterator] = 1;
	}

	//generate block graph of bT:
	Graph blockGraph_bT;
	NodeArray<int> nodeLengthSG(blockGraph_bT);
	NodeArray<node> nG_to_nSG;
	embedder::ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockGraph_bT,
	                                       (*(pBCTree->hEdges(bT).begin()))->source(),
	                                       m_nodeLength, nodeLengthSG, nG_to_nSG);

	//set edge length for all edges in block graph to 0:
	EdgeArray<int> edgeLengthBlock(blockGraph_bT, 0);

	//compute size of a maximum external face of block graph:
	StaticSPQRTree* spqrTree = nullptr;
	if (!blockGraph_bT.empty()
		&& blockGraph_bT.numberOfNodes() != 1
		&& blockGraph_bT.numberOfEdges() > 2)
	{
		spqrTree = new StaticSPQRTree(blockGraph_bT);
	}

	NodeArray< EdgeArray<int> > edgeLengthSkel;
	int cstrLength_B_c = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
		blockGraph_bT, nodeLengthSG, edgeLengthBlock, spqrTree, edgeLengthSkel);

	//Prepare recursion by setting m_{c, B} for all edges {B, c} \in bcTree:
	if (md_M_B[bT].size() > 0)
	{
		node cT1 = pBCTree->bcproper(pBCTree->original(*(md_M_B[bT].begin())));
		bool calculateNewNodeLengths;
		calculateNewNodeLengths = md_M_B[bT].size() == 1 && cT1 == cT_parent;
		for(adjEntry adj : bT->adjEntries) {
			edge e_bT_cT = adj->theEdge();
			if (e_bT_cT->target() != bT)
				continue;
			node cT = e_bT_cT->source();
			node cH = pBCTree->cutVertex(cT, bT);

			if (md_M_B[bT].size() == 1 && cT1 == cT)
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
						if (m2 < cB[e_cT2_bT2])
						{
							m2 = cB[e_cT2_bT2];
							M2[bT].clear();
							M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
						}
						else if (m2 == cB[e_cT2_bT2] && !M2[bT].search(pBCTree->cutVertex(cT2, bT)).valid())
						{
							M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
						}
					}
				}

				//set vertex length for all vertices in bH to 1 if vertex is in M2 and
				//0 otherwise:
				md_nodeLength[*(md_M_B[bT].begin())] = 0;
				for (ListIterator<node> iterator = M2[bT].begin(); iterator.valid(); ++iterator)
					md_nodeLength[*iterator] = 1;

				Graph blockGraphBT;
				node cInBlockGraph_bT;
				NodeArray<int> nodeLengthSGBT(blockGraphBT);
				embedder::ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockGraphBT, cH,
					cInBlockGraph_bT, md_nodeLength, nodeLengthSGBT);

				//set edge length for all edges in block graph to 0:
				EdgeArray<int> zeroEdgeLength(blockGraphBT, 0);

				//compute a maximum external face size of a face containing c in block graph:
				int maxFaceSizeInBlock = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
					blockGraphBT,
					cInBlockGraph_bT,
					nodeLengthSGBT,
					zeroEdgeLength);
				if (M2[bT].size() == 0)
					cB[e_bT_cT] = 1;
				else
				{
					if (maxFaceSizeInBlock == M2[bT].size())
						cB[e_bT_cT] = m2;
					else
						cB[e_bT_cT] = m2 + 2;
				}

				if (calculateNewNodeLengths)
					calculateNewNodeLengths = false;
				else
				{
					//reset node lengths:
					for (ListIterator<node> iterator = M2[bT].begin(); iterator.valid(); ++iterator)
						md_nodeLength[*iterator] = 0;
					md_nodeLength[*(md_M_B[bT].begin())] = 1;
				}
			}
			else //M_B.size() != 1
			{
				//compute a maximum external face size of a face containing c in block graph:
				node cInBlockGraph_bT = nG_to_nSG[cH];
				int maxFaceSizeInBlock = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
					blockGraph_bT,
					cInBlockGraph_bT,
					nodeLengthSG,
					edgeLengthBlock,
					spqrTree,
					edgeLengthSkel);
				if (md_M_B[bT].size() == 0)
					cB[e_bT_cT] = 1;
				else
				{
					if (maxFaceSizeInBlock == md_M_B[bT].size())
						cB[e_bT_cT] = m_B;
					else
						cB[e_bT_cT] = m_B + 2;
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
					if (m2 < cB[e_cT2_bT2])
					{
						m2 = cB[e_cT2_bT2];
						M2[bT].clear();
						M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
					else if (m2 == cB[e_cT2_bT2] && !M2[bT].search(pBCTree->cutVertex(cT2, bT)).valid())
					{
						M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
				}
			}

			//set vertex length for all vertices in bH to 1 if vertex is in M2 and
			//0 otherwise:
			md_nodeLength[*(md_M_B[bT].begin())] = 0;
			for (ListIterator<node> iterator = M2[bT].begin(); iterator.valid(); ++iterator)
				md_nodeLength[*iterator] = 1;
		} else if (md_M_B[bT].size() == 1) {
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
					if (m2 < cB[e_cT2_bT2])
					{
						m2 = cB[e_cT2_bT2];
						M2[bT].clear();
						M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
					else if (m2 == cB[e_cT2_bT2] && !M2[bT].search(pBCTree->cutVertex(cT2, bT)).valid())
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
		md_M_B[bT].clear();
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
				if (m_B < cB[e_cT_bT2])
				{
					m_B = cB[e_cT_bT2];
					md_M_B[bT].clear();
					md_M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
				}
				else if (m_B == cB[e_cT_bT2] && !md_M_B[bT].search(pBCTree->cutVertex(cT, bT)).valid())
				{
					md_M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
				}
			}
		}

		if (md_M_B[bT].size() == 1)
		{
			int m2 = 0;
			node cT1 = pBCTree->bcproper(pBCTree->original(*(md_M_B[bT].begin())));
			for(adjEntry adj : bT->adjEntries) {
				edge e_bT_cT = adj->theEdge();
				node cT2 = (e_bT_cT->source() == bT) ? e_bT_cT->target() : e_bT_cT->source();
				if (cT1 == cT2)
					continue;
				node cT = (e_bT_cT->source() == bT) ? e_bT_cT->target() : e_bT_cT->source();
				for(adjEntry adjCT : cT->adjEntries) {
					edge e_cT_bT2 = adjCT->theEdge();
					//update m2 and M2:
					if (m2 < cB[e_cT_bT2])
					{
						m2 = cB[e_cT_bT2];
						M2[bT].clear();
						M2[bT].pushBack(pBCTree->cutVertex(cT, bT));
					}
					else if (m2 == cB[e_cT_bT2]
									 && !M2[bT].search(pBCTree->cutVertex(cT, bT)).valid())
					{
						M2[bT].pushBack(pBCTree->cutVertex(cT, bT));
					}
				}
			}
		}
	}

	if (cstrLength_B_c == md_M_B[bT].size())
		minDepth[bT] = m_B;
	else
		minDepth[bT] = m_B + 2;

	delete spqrTree;
}


int EmbedderMinDepthMaxFace::constraintMaxFace(const node &bT, const node &cH)
{
	computeNodeLength(bT, [&](node vH) -> int& { return mf_nodeLength[vH]; });

	mf_nodeLength[cH] = 0;
	Graph blockGraph;
	node cInBlockGraph;
	NodeArray<int> nodeLengthSG(blockGraph);
	embedder::ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockGraph, cH, cInBlockGraph,
		mf_nodeLength, nodeLengthSG);
	EdgeArray<int> edgeLengthSG(blockGraph, 1);
	int cstrLengthBc = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
		blockGraph, cInBlockGraph, nodeLengthSG, edgeLengthSG);
	mf_cstrLength[cH] = cstrLengthBc;
	return cstrLengthBc;
}


void EmbedderMinDepthMaxFace::maximumFaceRec(const node &bT, node &bT_opt, int &ell_opt)
{
	//(B*, \ell*) := (B, size of a maximum face in B):
	Graph blockGraph_bT;
	NodeArray<int> nodeLengthSG(blockGraph_bT);
	NodeArray<node> nG_to_nSG;
	embedder::ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockGraph_bT,
		(*(pBCTree->hEdges(bT).begin()))->source(), mf_nodeLength, nodeLengthSG, nG_to_nSG);
	StaticSPQRTree* spqrTree = nullptr;
	if (!blockGraph_bT.empty()
		&& blockGraph_bT.numberOfNodes() != 1
		&& blockGraph_bT.numberOfEdges() > 2)
	{
		spqrTree = new StaticSPQRTree(blockGraph_bT);
	}

	internalMaximumFaceRec(
			bT, bT_opt, ell_opt,
			blockGraph_bT,
			nodeLengthSG,
			spqrTree,
			[&](node cH) -> node& { return  nG_to_nSG[cH]; },
			[&](node v, node u) -> int& { return mf_cstrLength[u]; },
			[&](node v, node u) -> int& { return mf_nodeLength[u]; },
			&maxFaceSize[bT]);

	delete spqrTree;
}

void EmbedderMinDepthMaxFace::embedBlock(
	const node& bT,
	const node& cT,
	ListIterator<adjEntry>& after)
{
	treeNodeTreated[bT] = true;
	node cH = nullptr;
	if (cT != nullptr)
		cH = pBCTree->cutVertex(cT, bT);

	// 1. Compute MinDepth node lengths depending on M_B, M2 and cT
	if (cT != nullptr && md_M_B[bT].size() == 1 && *(md_M_B[bT].begin()) == cH)
	{
		//set node length to 1 if node is in M2 and 0 otherwise
		for (ListIterator<node> iterator = M2[bT].begin(); iterator.valid(); ++iterator)
			md_nodeLength[*iterator] = 1;
	}
	else
	{
		//set node length to 1 if node is in M_B and 0 otherwise
		for (ListIterator<node> iterator = md_M_B[bT].begin(); iterator.valid(); ++iterator)
			md_nodeLength[*iterator] = 1;
	}

	// 2. Set MinDepthMaxFace node lengths

	//create subgraph (block bT):
	node nodeInBlock = cH;
	if (nodeInBlock == nullptr)
		nodeInBlock = (*(pBCTree->hEdges(bT).begin()))->source();
	Graph SG;
	NodeArray<MDMFLengthAttribute> nodeLengthSG;
	EdgeArray<MDMFLengthAttribute> edgeLengthSG;
	NodeArray<node> nSG_to_nG;
	EdgeArray<edge> eSG_to_eG;
	node nodeInBlockSG;
	embedder::ConnectedSubgraph<MDMFLengthAttribute>::call(
		pBCTree->auxiliaryGraph(), SG,
		nodeInBlock, nodeInBlockSG,
		nSG_to_nG, eSG_to_eG,
		mdmf_nodeLength, nodeLengthSG,
		edgeLength, edgeLengthSG);

	//copy (0, 1)-min depth node lengths into nodeLengthSG "a" component and max
	//face sice node lengths into "b" component:
	for(node nSG : SG.nodes)
	{
		nodeLengthSG[nSG].a = md_nodeLength[nSG_to_nG[nSG]];
		nodeLengthSG[nSG].b = mf_nodeLength[nSG_to_nG[nSG]];
	}

	internalEmbedBlock(bT, cT, after, SG, nodeLengthSG, edgeLengthSG, nSG_to_nG, eSG_to_eG, cH == nullptr ? nullptr : nodeInBlockSG);
}

}
