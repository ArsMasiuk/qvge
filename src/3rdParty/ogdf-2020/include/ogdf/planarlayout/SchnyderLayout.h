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
#include <ogdf/planarlayout/GridLayoutModule.h>
#include <ogdf/basic/List.h>

namespace ogdf {

/**
 * The class SchnyderLayout represents the layout algorithm by
 * Schnyder [Sch90]. This algorithm draws a planar graph G
 * straight-line without crossings. G (with |V| ≥ 3) must not contain
 * self-loops or multiple edges.
 * The algorithm runs in three phases. In the first phase, the graph is
 * augmented by adding new artificial edges to get a triangulated plane graph.
 * Then, a partition of the set of interior edges in three trees
 * (also called Schnyder trees) with special orientation properties is derived.
 * In the third step, the actual coordinates are computed.
 * See:
 *
 * [Sch90] Schnyder, Walter. "Embedding planar graphs on the grid."
 * Proceedings of the first annual ACM-SIAM symposium on Discrete algorithms.
 * Society for Industrial and Applied Mathematics, 1990.
 *
 * [Sch89] Schnyder, Walter. "Planar graphs and poset dimension."
 * Order 5.4 (1989): 323-343.
 */
class OGDF_EXPORT SchnyderLayout : public PlanarGridLayoutModule {

public:
	SchnyderLayout();

	/**
	 * Each node in a Schnyder wood splits the graph into three regions.
	 * The barycentric coordinates of the nodes are given by the count of
	 * combinatorial objects in these regions.
	 */
	enum class CombinatorialObjects {
		VerticesMinusDepth, //!< Count the number of vertices in each region i and
		                    //!< subtract the depth of the (i-1)-path of the node.
		                    //!< This approach is outlined in [Sch90].
		                    //!< The grid layout size is (n - 2) × (n - 2).
		Faces               //!< Count the number of faces in each region i.
		                    //!< This approach is outlined in [Sch89].
		                    //!< The grid layout size is (2n - 5) × (2n - 5).
	};

	//! Returns the type of combinatorial objects whose number corresponds to the node coordinates.
	CombinatorialObjects getCombinatorialObjects() { return m_combinatorialObjects; }

	//! Sets the type of combinatorial objects whose number corresponds to the node coordinates.
	void setCombinatorialObjects(CombinatorialObjects combinatorialObjects) {
		m_combinatorialObjects = combinatorialObjects;
	}

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

	//! Determines how the barycentric coordinates of each node are computed.
	CombinatorialObjects m_combinatorialObjects;

};

}
