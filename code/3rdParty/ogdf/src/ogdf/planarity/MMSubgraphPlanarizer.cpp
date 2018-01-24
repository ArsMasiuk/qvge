/** \file
 * \brief Implements class MMSubgraphPlanarizer.
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

#include <ogdf/planarity/MMSubgraphPlanarizer.h>
#include <ogdf/planarity/PlanarSubgraphFast.h>
#include <ogdf/planarity/MMFixedEmbeddingInserter.h>


namespace ogdf {


MMSubgraphPlanarizer::MMSubgraphPlanarizer()
{
	auto *s = new PlanarSubgraphFast<int>();
	s->runs(100);
	m_subgraph.reset(s);

	MMFixedEmbeddingInserter *pInserter = new MMFixedEmbeddingInserter();
	pInserter->removeReinsert(RemoveReinsertType::All);
	m_inserter.reset(pInserter);

	m_permutations = 1;
}


Module::ReturnType MMSubgraphPlanarizer::doCall(PlanRepExpansion &PG,
	int cc,
	const EdgeArray<bool> *forbid,
	int& crossingNumber,
	int& numNS,
	int& numSN)
{
	OGDF_ASSERT(m_permutations >= 1);

	List<edge> deletedEdges;
	PG.initCC(cc);

	ReturnType retValue ;

	if(forbid != nullptr) {
		List<edge> preferedEdges;
		for(edge e : PG.edges) {
			edge eOrig = PG.originalEdge(e);
			if(eOrig && (*forbid)[eOrig])
				preferedEdges.pushBack(e);
		}

		retValue = m_subgraph->call(PG, preferedEdges, deletedEdges, true);

	} else {
		retValue = m_subgraph->call(PG, deletedEdges);
	}

	if(!isSolution(retValue))
		return retValue;

	for(ListIterator<edge> it = deletedEdges.begin(); it.valid(); ++it)
		*it = PG.originalEdge(*it);

	int bestcr = -1;

	for(int i = 1; i <= m_permutations; ++i)
	{
		for(ListConstIterator<edge> it = deletedEdges.begin(); it.valid(); ++it)
			PG.delEdge(PG.copy(*it));

		deletedEdges.permute();

		if(forbid != nullptr)
			m_inserter->call(PG, deletedEdges, *forbid);
		else
			m_inserter->call(PG, deletedEdges);

		crossingNumber = PG.computeNumberOfCrossings();

		if(i == 1 || crossingNumber < bestcr) {
			bestcr = crossingNumber;
			numNS = PG.numberOfNodeSplits();
			numSN = PG.numberOfSplittedNodes();
		}

		PG.initCC(cc);
	}

	crossingNumber = bestcr;

	return ReturnType::Feasible;
}

}
