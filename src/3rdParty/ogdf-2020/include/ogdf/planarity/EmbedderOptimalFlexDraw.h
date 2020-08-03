/** \file
 * \brief The algorithm computes a planar embedding with minimum cost.
 *
 * \author Dzmitry Sledneu
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
#include <memory>
#include <ogdf/graphalg/MinCostFlowModule.h>
#include <ogdf/decomposition/StaticPlanarSPQRTree.h>

namespace ogdf {

//! The algorithm computes a planar embedding with minimum cost.
/**
 * @ingroup ga-planembed
 *
 * See paper "Optimal Orthogonal Graph Drawing with Convex Bend Costs"
 * by Thomas Blasius, Ignaz Rutter, Dorothea Wagner (2012) for details.
 */
class OGDF_EXPORT EmbedderOptimalFlexDraw : public EmbedderModule
{
public:
	EmbedderOptimalFlexDraw();

	virtual void doCall(Graph &G, adjEntry &adjExternal) override;

	/// Sets the module option to compute min-cost flow.
	void setMinCostFlowComputer(MinCostFlowModule<int> *pMinCostFlowComputer) {
		m_minCostFlowComputer.reset(pMinCostFlowComputer);
	}

	/// Sets bend costs for each edge.
	void cost(EdgeArray<int> *cost) {
		m_cost = cost;
	}

private:

	std::unique_ptr<MinCostFlowModule<int>> m_minCostFlowComputer;

	EdgeArray<int> *m_cost;

	void createNetwork(
			node parent,
		node mu,
		int bends,
		NodeArray<int> cost[],
		Skeleton &skeleton,
		EdgeArray<node> &edgeNode,
		Graph &N,
		EdgeArray<int> &upper,
		EdgeArray<int> &perUnitCost,
		NodeArray<int> &supply);

	void optimizeOverEmbeddings(
		StaticPlanarSPQRTree &T,
		node parent,
		node mu,
		int bends,
		NodeArray<int> cost[],
		NodeArray<long long> embedding[]);

	void computePrincipalSplitComponentCost(
		StaticPlanarSPQRTree &T,
		NodeArray<int> cost[],
		NodeArray<long long> embedding[],
		node parent,
		node mu);
};

}
