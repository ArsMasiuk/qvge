/** \file
 * \brief Definition of ogdf::EmbedderMaxFaceLayers.
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

#include <ogdf/planarity/EmbedderMaxFaceLayers.h>
#include <ogdf/planarity/embedder/EmbedderMaxFaceBiconnectedGraphsLayers.h>
#include <ogdf/planarity/embedder/ConnectedSubgraph.h>

namespace ogdf {

void EmbedderMaxFaceLayers::embedBlock(
	const node& bT,
	const node& cT,
	ListIterator<adjEntry>& after)
{
	treeNodeTreated[bT] = true;
	node cH = nullptr;
	if (cT != nullptr)
		cH = pBCTree->cutVertex(cT, bT);

	EdgeArray<int> edgeLength(blockG[bT], 1);

	internalEmbedBlock(
			blockG[bT],
			nodeLength[bT],
			edgeLength,
			nBlockEmbedding_to_nH[bT],
			eBlockEmbedding_to_eH[bT],
			cH == nullptr ? nullptr : nH_to_nBlockEmbedding[bT][cH],
			cT,
			after);
}

}
