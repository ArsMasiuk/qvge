/** \file
 * \brief Declaration of class UpwardPlanarSubgraphSimple which
 *        computes an upward planar subgraph by using upward planarity testing.
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

#pragma once

#include <ogdf/upward/UpwardPlanarSubgraphModule.h>
#include <ogdf/basic/tuples.h>

namespace ogdf {

//! A maximal planar subgraph algorithm using planarity testing
class OGDF_EXPORT UpwardPlanarSubgraphSimple : public UpwardPlanarSubgraphModule
{
public:
	// construction
	UpwardPlanarSubgraphSimple() { }
	// destruction
	~UpwardPlanarSubgraphSimple() { }

	// computes set of edges delEdges, which have to be deleted
	// in order to get a planar subgraph; edges in preferedEdges
	// should be contained in planar subgraph
	virtual void call(const Graph &G, List<edge> &delEdges) override;

	void call(GraphCopy &GC, List<edge> &delEdges);


private:
	bool checkAcyclic(
		GraphCopySimple &graphAcyclicTest,
		SList<Tuple2<node,node> > &tmpAugmented);

	void dfsBuildSpanningTree(
		node v,
		SListPure<edge> &treeEdges,
		NodeArray<bool> &visited);
};

}
