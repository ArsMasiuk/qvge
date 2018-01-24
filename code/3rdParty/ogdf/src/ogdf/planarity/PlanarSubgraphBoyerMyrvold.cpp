/** \file
 * \brief Declaration of the subgraph wrapper class of the Boyer-Myrvold planarity test
 *
 * \author Tilo Wiedera
 *
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

#include <ogdf/planarity/PlanarSubgraphBoyerMyrvold.h>

namespace ogdf {

Module::ReturnType PlanarSubgraphBoyerMyrvold::doCall(
	const Graph &graph,
	const List<edge> &preferedEdges,
	List<edge> &delEdges,
	const EdgeArray<int>  *pCosts,
	bool /* unused parameter */)
{
	int bestCost = -1;

	for(int i = 0; i < m_runs; i++) {
		SListPure<KuratowskiStructure> tmp;
		GraphCopy copy(graph);
		EdgeArray<int> *costs = nullptr;

		if(pCosts != nullptr) {
			costs = new EdgeArray<int>(copy);

			for(edge e : copy.edges) {
				(*costs)[e] = (*pCosts)[copy.original(e)];
			}
		}

		BoyerMyrvoldPlanar bmp(copy, false, BoyerMyrvoldPlanar::EmbeddingGrade::doFindUnlimited, false, tmp, m_randomness, true, true, costs);
		std::minstd_rand rand(m_rand());
		bmp.seed(rand);
		bmp.start();

		OGDF_ASSERT(m_planModule.isPlanar(copy));
		OGDF_ASSERT(copy.numberOfEdges() == graph.numberOfEdges());

		int totalCost = 0;
		if(i != 0) {
			for(edge e : graph.edges) {
				if(isRemoved(copy, e)) {
					totalCost += costs == nullptr ? 1 : (*pCosts)[e];
				}
			}
		}

		if(i == 0 || totalCost < bestCost) {
			bestCost = totalCost;
			delEdges.clear();
			for(edge e : graph.edges) {
				if(isRemoved(copy, e)) {
					delEdges.pushBack(e);
				}
			}
		}

		delete costs;
	}

	return Module::ReturnType::Feasible;
}

}
