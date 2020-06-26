/** \file
 * \brief A simple embedder algorithm.
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

#include <ogdf/planarity/SimpleEmbedder.h>
#include <ogdf/basic/FaceArray.h>

namespace ogdf {

void SimpleEmbedder::doCall(Graph& G, adjEntry& adjExternal)
{
	// determine embedding of G

	// We currently compute any embedding and choose the maximal face
	// as external face

	// if we use FixedEmbeddingInserterOld, we have to re-use the computed
	// embedding, otherwise crossing nodes can turn into "touching points"
	// of edges (alternatively, we could compute a new embedding and
	// finally "remove" such unnecessary crossings).
	adjExternal = nullptr;
	if(!G.representsCombEmbedding())
		planarEmbed(G);

	CombinatorialEmbedding CE(G);
	PlanRep PR(G);
	//face fExternal = E.maximalFace();
	face fExternal = findBestExternalFace(PR, CE);
	adjExternal = fExternal->firstAdj();
}


face SimpleEmbedder::findBestExternalFace(
	const PlanRep& PG,
	const CombinatorialEmbedding& E)
{
	FaceArray<int> weight(E);

	for(face f : E.faces)
		weight[f] = f->size();

	for(node v : PG.nodes)
	{
		if(PG.typeOf(v) != Graph::NodeType::generalizationMerger)
			continue;

		adjEntry adjFound = nullptr;
		for(adjEntry adj : v->adjEntries) {
			if (adj->theEdge()->source() == v) {
				adjFound = adj;
				break;
			}
		}

		OGDF_ASSERT(adjFound->theEdge()->source() == v);

		node w = adjFound->theEdge()->target();
		bool isBase = true;

		for(adjEntry adj : w->adjEntries) {
			edge e = adj->theEdge();
			if(e->target() != w && PG.typeOf(e) == Graph::EdgeType::generalization) {
				isBase = false;
				break;
			}
		}

		if(!isBase)
			continue;

		face f1 = E.leftFace(adjFound);
		face f2 = E.rightFace(adjFound);

		weight[f1] += v->indeg();
		if(f2 != f1)
			weight[f2] += v->indeg();
	}

	face fBest = E.firstFace();
	for(face f : E.faces)
		if(weight[f] > weight[fBest])
			fBest = f;

	return fBest;
}

}
