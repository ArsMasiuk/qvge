/** \file
 * \brief Declaration of the Schnyder Layout Algorithm (SchnyderLayout)
 *        algorithm.
 *
 * \author Till Schäfer
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

#include <ogdf/basic/Graph_d.h>
#include <ogdf/module/GridLayoutModule.h>
#include <ogdf/basic/List.h>

namespace ogdf {

/**
 * The class SchnyderLayout represents the layout algorithm by
 * Schnyder [Sch90]. This algorithm draws a planar graph G
 * straight-line without crossings. G must not contain self-loops or
 * multiple edges.
 * The grid layout size is (<i>n</i> − 2) × (<i>n</i> − 2) for a graph with
 * n nodes (<i>n</i> ≥ 3).
 * The algorithm runs in three phases. In the ﬁrst phase, the graph is
 * augmented by adding new artiﬁcial edges to get a triangulated plane graph.
 * Then, a partition of the set of interior edges in three trees
 * (also called Schnyder trees) with special orientation properties is derived.
 * In the third step, the actual coordinates are computed.
 */
class OGDF_EXPORT SchnyderLayout : public PlanarGridLayoutModule {

public:
	SchnyderLayout();

protected:
	virtual void doCall(
		const Graph &G,
		adjEntry adjExternal,
		GridLayout &gridLayout,
		IPoint &boundingBox,
		bool fixEmbedding) override;

private:
	void contract(Graph& G, node a, node b, node c, List<node>& L);

	void realizer(
		GraphCopy& G,
		const List<node>& L,
		node a,
		node b,
		node c,
		EdgeArray<int>& rValues,
		GraphCopy& T);

	void subtreeSizes(
		EdgeArray<int>& rValues,
		int i,
		node r,
		NodeArray<int>& size);

	void prefixSum(
		EdgeArray<int>& rValues,
		int i,
		node r,
		const NodeArray<int>& val,
		NodeArray<int>& sum);

	void schnyderEmbedding(
		GraphCopy& GC,
		GridLayout &gridLayout,
		adjEntry adjExternal);
};

}
