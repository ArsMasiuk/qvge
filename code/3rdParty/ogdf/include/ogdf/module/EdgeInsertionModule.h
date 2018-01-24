/** \file
 * \brief Declaration of interface for edge insertion algorithms
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

#include <ogdf/basic/Logger.h>
#include <ogdf/basic/Module.h>
#include <ogdf/basic/Timeouter.h>
#include <ogdf/planarity/PlanRepLight.h>


namespace ogdf {

//! Interface for edge insertion algorithms.
/**
 * \see SubgraphPlanarizer
 */
class OGDF_EXPORT EdgeInsertionModule : public Module, public Timeouter {
public:
	//! Initializes an edge insertion module (default constructor).
	EdgeInsertionModule() { }

	//! Initializes an edge insertion module (copy constructor).
	EdgeInsertionModule(const EdgeInsertionModule &eim) : Timeouter(eim) { }

	//! Destructor.
	virtual ~EdgeInsertionModule() { }

	//! Returns a new instance of the edge insertion module with the same option settings.
	virtual EdgeInsertionModule *clone() const = 0;

	//! Inserts all edges in \p origEdges into \p pr.
	/**
	 * @param pr        is the input planarized representation and will also receive the result.
	 * @param origEdges is the array of original edges (edges in the original graph of \p pr)
	 *                  that have to be inserted.
	 * @return the status of the result.
	 */
	ReturnType call(PlanRepLight &pr, const Array<edge> &origEdges) {
		return doCall(pr, origEdges, nullptr, nullptr, nullptr);
	}

	//! Inserts all edges in \p origEdges with given costs into \p pr.
	/**
	 * @param pr        is the input planarized representation and will also receive the result.
	 * @param costOrig  is an edge array containing the costs of original edges; edges in
	 *                  \p pr without an original edge have zero costs.
	 * @param origEdges is the array of original edges (edges in the original graph of \p pr)
	 *                  that have to be inserted.
	 * @return the status of the result.
	 */
	ReturnType call(PlanRepLight &pr,
		const EdgeArray<int> &costOrig,
		const Array<edge>    &origEdges)
	{
		return doCall(pr, origEdges, &costOrig, nullptr, nullptr);
	}


	//! Inserts all edges in \p origEdges with given costs and subgraphs (for simultaneous drawing) into \p pr.
	/**
	 * @param pr            is the input planarized representation and will also receive the result.
	 * @param costOrig      is an edge array containing the costs of original edges; edges in
	 *                      \p pr without an original edge have zero costs.
	 * @param origEdges     is the array of original edges (edges in the original graph of \p pr)
	 *                      that have to be inserted.
	 * @param edgeSubGraphs is an edge array specifying to which subgraph an edge belongs.
	 * @return the status of the result.
	 */
	ReturnType call(PlanRepLight &pr,
		const EdgeArray<int>      &costOrig,
		const Array<edge>         &origEdges,
		const EdgeArray<uint32_t> &edgeSubGraphs)
	{
		return doCall(pr, origEdges, &costOrig, nullptr, &edgeSubGraphs);
	}


	//! Inserts all edges in \p origEdges with given forbidden edges into \p pr.
	/**
	 * \pre No forbidden edge may be in \p origEdges.
	 *
	 * @param pr            is the input planarized representation and will also receive the result.
	 * @param forbiddenOrig is an edge array indicating if an original edge is forbidden to be crossed.
	 * @param origEdges     is the array of original edges (edges in the original graph of \p pr)
	 *                      that have to be inserted.
	 * @return the status of the result.
	 */
	ReturnType call(PlanRepLight &pr,
		const EdgeArray<bool> &forbiddenOrig,
		const Array<edge>     &origEdges)
	{
		return doCall(pr, origEdges, nullptr, &forbiddenOrig, nullptr);
	}

	//! Inserts all edges in \p origEdges with given costs and forbidden edges into \p pr.
	/**
	 * \pre No forbidden edge may be in \p origEdges.
	 *
	 * @param pr            is the input planarized representation and will also receive the result.
	 * @param costOrig      is an edge array containing the costs of original edges; edges in
	 *                      \p pr without an original edge have zero costs.
	 * @param forbiddenOrig is an edge array indicating if an original edge is forbidden to be crossed.
	 * @param origEdges     is the array of original edges (edges in the original graph of \p pr)
	 *                      that have to be inserted.
	 * @return the status of the result.
	 */
	ReturnType call(PlanRepLight &pr,
		const EdgeArray<int>  &costOrig,
		const EdgeArray<bool> &forbiddenOrig,
		const Array<edge>     &origEdges)
	{
		return doCall(pr, origEdges, &costOrig, &forbiddenOrig, nullptr);
	}


	//! Inserts all edges in \p origEdges with given costs, forbidden edges, and subgraphs (for simultaneous drawing) into \p pr.
	/**
	 * \pre No forbidden edge may be in \p origEdges.
	 *
	 * @param pr            is the input planarized representation and will also receive the result.
	 * @param costOrig      is an edge array containing the costs of original edges; edges in
	 *                      \p pr without an original edge have zero costs.
	 * @param forbiddenOrig is an edge array indicating if an original edge is forbidden to be crossed.
	 * @param origEdges     is the array of original edges (edges in the original graph of \p pr)
	 *                      that have to be inserted.
	 * @param edgeSubGraphs is an edge array specifying to which subgraph an edge belongs.
	 * @return the status of the result.
	 */
	ReturnType call(PlanRepLight &pr,
		const EdgeArray<int>      &costOrig,
		const EdgeArray<bool>     &forbiddenOrig,
		const Array<edge>         &origEdges,
		const EdgeArray<uint32_t> &edgeSubGraphs)
	{
		return doCall(pr, origEdges, &costOrig, &forbiddenOrig, &edgeSubGraphs);
	}


	//! Inserts all edges in \p origEdges into \p pr, optionally costs, forbidden edges, and subgraphs (for simultaneous drawing) may be given.
	/**
	 * @param pr             is the input planarized representation and will also receive the result.
	 * @param origEdges      is the array of original edges (edges in the original graph of \p pr)
	 *                       that have to be inserted.
	 * @param pCostOrig      points to an edge array containing the costs of original edges; edges in
	 *                       \p pr without an original edge have zero costs. May be a 0-pointer, in which case all edges have cost 1.
	 * @param pForbiddenOrig points to an edge array indicating whether an original edge is forbidden to be crossed.
	 *                       May be a 0-pointer, in which case no edges are forbidden.
	 * @param pEdgeSubGraphs points to an edge array specifying to which subgraph an edge belongs.
	 *                       May be a 0-poiner, in which case no subgraphs / simultaneous embedding is used.
	 * @return the status of the result.
	 */
	ReturnType callEx(
		PlanRepLight              &pr,
		const Array<edge>         &origEdges,
		const EdgeArray<int>      *pCostOrig = nullptr,
		const EdgeArray<bool>     *pForbiddenOrig = nullptr,
		const EdgeArray<uint32_t> *pEdgeSubGraphs = nullptr)
	{
		return doCall(pr, origEdges, pCostOrig, pForbiddenOrig, pEdgeSubGraphs);
	}


protected:
	//! Actual algorithm call that has to be implemented by derived classes.
	/**
	 * @param pr             is the input planarized representation and will also receive the result.
	 * @param origEdges      is the array of original edges (edges in the original graph of \p pr)
	 *                       that have to be inserted.
	 * @param pCostOrig      points to an edge array containing the costs of original edges; edges in
	 *                       \p pr without an original edge have zero costs.
	 * @param pForbiddenOrig points to an edge array indicating whether an original edge is forbidden to be crossed.
	 * @param pEdgeSubGraphs points to an edge array specifying to which subgraph an edge belongs.
	 * @return the status of the result.
	 */
	virtual ReturnType doCall(
		PlanRepLight              &pr,
		const Array<edge>         &origEdges,
		const EdgeArray<int>      *pCostOrig,
		const EdgeArray<bool>     *pForbiddenOrig,
		const EdgeArray<uint32_t> *pEdgeSubGraphs) = 0;


	OGDF_MALLOC_NEW_DELETE
};

}
