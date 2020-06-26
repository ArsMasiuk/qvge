/** \file
 * \brief Definition of odgf::EmbedderMinDepthMaxFaceLayers.
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

#include <ogdf/planarity/EmbedderMinDepthMaxFaceLayers.h>
#include <ogdf/planarity/embedder/ConnectedSubgraph.h>

namespace ogdf {

void EmbedderMinDepthMaxFaceLayers::embedBlock(
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

	internalEmbedBlock(
			SG,
			nodeLengthSG,
			edgeLengthSG,
			nSG_to_nG,
			eSG_to_eG,
			cH == nullptr ? nullptr : nodeInBlockSG,
			cT,
			after);
}

}
