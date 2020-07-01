/** \file
 * \brief Declaration of the Fraysseix, Pach, Pollack Algorithm (FPPLayout)
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

namespace ogdf
{

/**
 * The class FPPLayout represents the layout algorithm by
 * de Fraysseix, Pach, Pollack [DPP90]. This algorithm draws a planar graph G
 * straight-line without crossings. G must not contain self-loops or multiple
 * edges. The grid layout size is (2<i>n</i>-4) * (<i>n</i>-2) for a graph with
 * n nodes (<i>n</i> ≥ 3).
 * The algorithm runs in three phases. In the ﬁrst phase, the graph is
 * augmented by adding new artiﬁcial edges to get a triangulated plane graph.
 * Then, a so-called shelling order (also called canonical ordering)
 * for triangulated planar graphs is computed. In the third phase the vertices
 * are placed incrementally according to the shelling order.
 */
class OGDF_EXPORT FPPLayout : public PlanarGridLayoutModule {
public:
	FPPLayout();

private:
	virtual void doCall(
		const Graph &G,
		adjEntry adjExternal,
		GridLayout &gridLayout,
		IPoint &boundingBox,
		bool fixEmbedding ) override;

	void computeOrder(
		const GraphCopy &G,
		NodeArray<int> &num,
		NodeArray<adjEntry> &e_wp,
		NodeArray<adjEntry> &e_wq,
		adjEntry e_12,
		adjEntry e_2n,
		adjEntry e_n1 );

	void computeCoordinates(
		const GraphCopy &G,
		IPoint &boundingBox,
		GridLayout &gridLayout,
		NodeArray<int> &num,
		NodeArray<adjEntry> &e_wp,
		NodeArray<adjEntry> &e_wq );
};

}
