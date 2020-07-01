/** \file
 * \brief Declaration of CPlanarSubClusteredGraph class.
 *
 * \author Karsten Klein
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

#include <ogdf/cluster/ClusterPlanRep.h>
#include <ogdf/cluster/internal/CPlanarSubClusteredST.h>

namespace ogdf {

//! Constructs a c-planar subclustered graph of the input based on a spanning tree.
/**
 * @ingroup ga-cplanarity
 */
class OGDF_EXPORT CPlanarSubClusteredGraph
{
public:
	CPlanarSubClusteredGraph() { }

	virtual void call(const ClusterGraph& CG, EdgeArray<bool>& inSub);

	virtual void call(
		const ClusterGraph& CGO,
		EdgeArray<bool>& inSub,
		List<edge>& leftOver);

	//! Uses \p edgeWeight to compute clustered planar subgraph
	virtual void call(
		const ClusterGraph& CGO,
		EdgeArray<bool>& inSub,
		List<edge>& leftOver,
		EdgeArray<double>& edgeWeight);

private:
	//! Store status of original edge: in subclustered graph?
	//! Also used to check spanning tree
	EdgeArray<int> m_edgeStatus;

};

}
