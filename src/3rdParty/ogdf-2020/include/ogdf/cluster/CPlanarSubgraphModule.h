/** \file
 * \brief Declaration of an interface for c-planar subgraph algorithms.
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

#include <ogdf/basic/Module.h>
#include <ogdf/basic/Timeouter.h>

#include <ogdf/cluster/ClusterGraph.h>

namespace ogdf {

//! Interface of algorithms for the computation of c-planar subgraphs.
class CPlanarSubgraphModule : public Module, public Timeouter
{

public:
	//! Constructs a cplanar subgraph module
	CPlanarSubgraphModule() {}
	//! Destruction
	virtual ~CPlanarSubgraphModule() {}

	/**
	 *  \brief  Computes set of edges delEdges, which have to be deleted
	 *  in order to get a c-planar subgraph.
	 *
	 * Must be implemented by derived classes.
	 * @param G is the clustergraph.
	 * @param delEdges holds the edges not in the subgraph on return.
	 *
	 */
	ReturnType call(const ClusterGraph &G, List<edge> &delEdges) {
		return call(G, nullptr, delEdges);
	}

	/**
	 *  \brief  Computes set of edges delEdges, which have to be deleted
	 *  in order to get a c-planar subgraph.
	 *
	 * Must be implemented by derived classes.
	 * @param G is the clustergraph.
	 * @param pCost Assigns integral weight to all edges.
	 *        We ask for a heavy subgraph.
	 *        If set to \c nullptr all edges have a weight of 1.
	 * @param delEdges holds the edges not in the subgraph on return.
	 *
	 */
	ReturnType call(const ClusterGraph &G, const EdgeArray<double> *pCost, List<edge> &delEdges) {
		return doCall(G, pCost, delEdges);
	}


protected:

	/**
	 * \brief Computes a c-planar subgraph.
	 *
	 * If delEdges is empty on return, the clustered graph G is c-planar-
	 * The actual algorithm call that must be implemented by derived classes!
	 *
	 * @param CG is the given cluster graph.
	 * @param pCost Assigns integral weight to all edges.
	 *        We ask for a heavy subgraph.
	 *        If set to \c nullptr all edges have a weight of 1.
	 * @param delEdges holds the set of edges that have to be deleted.
	 */
	virtual ReturnType doCall(const ClusterGraph &CG, const EdgeArray<double> *pCost, List<edge> &delEdges) = 0;

	OGDF_MALLOC_NEW_DELETE
};

}
