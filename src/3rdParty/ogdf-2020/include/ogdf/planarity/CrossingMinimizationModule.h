/** \file
 * \brief Declaration of CrossingMinimization Module, an interface for crossing minimization algorithms
 *
 * \author Markus Chimani
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

#include <ogdf/planarity/PlanRep.h>
#include <ogdf/basic/Module.h>
#include <ogdf/basic/Timeouter.h>

namespace ogdf {

//! Base class for crossing minimization algorithms.
class OGDF_EXPORT CrossingMinimizationModule : public Module, public Timeouter
{
public:
	//! Initializes a crossing minimization module (default constructor).
	CrossingMinimizationModule() { }

	//! Initializes an crossing minimization module (copy constructor).
	CrossingMinimizationModule(const CrossingMinimizationModule &cmm) : Timeouter(cmm) { }

	//! Destructor.
	virtual ~CrossingMinimizationModule() { }

	//! Returns a new instance of the crossing minimization module with the same option settings.
	virtual CrossingMinimizationModule *clone() const = 0;


	//! Computes a planarized representation of the input graph.
	/**
	 * @param pr             represents the input graph as well as the computed planarized representation
	 *                       after the call. \p pr has to be initialzed as a PlanRep of the input graph and
	 *                       is modified to obatain the planarized representation (crossings are replaced
	 *                       by dummy vertices with degree four).
	 * @param cc             is the index of the connected component in \p pr that is considered.
	 * @param crossingNumber is assigned the number of crossings.
	 * @param pCostOrig      points to an edge array (of the original graph) that gives the cost of each edge.
	 *                       May be a 0-pointer, in which case all edges have cost 1.
	 * @param pForbiddenOrig points to an edge array (of the original graph) specifying which edges are not
	 *                       allowed to be crossed. May be a 0-pointer, in which case no edges are forbidden.
	 * @param pEdgeSubGraphs points to an edge array (of the original graph) specifying to which subgraph an edge belongs.
	 * @return the status of the result.
	 */
	ReturnType call(PlanRep &pr,
		int cc,
		int&  crossingNumber,
		const EdgeArray<int>      *pCostOrig = nullptr,
		const EdgeArray<bool>     *pForbiddenOrig = nullptr,
		const EdgeArray<uint32_t> *pEdgeSubGraphs = nullptr)
	{
		return doCall(pr, cc, pCostOrig, pForbiddenOrig, pEdgeSubGraphs, crossingNumber);
	}

	//! Computes a planarized representation of the input graph.
	/**
	 * @param pr             represents the input graph as well as the computed planarized representation
	 *                       after the call. \p pr has to be initialzed as a PlanRep of the input graph and
	 *                       is modified to obatain the planarized representation (crossings are replaced
	 *                       by dummy vertices with degree four).
	 * @param cc             is the index of the connected component in \p pr that is considered.
	 * @param crossingNumber is assigned the number of crossings.
	 * @param pCostOrig      points to an edge array (of the original graph) that gives the cost of each edge.
	 *                       May be a 0-pointer, in which case all edges have cost 1.
	 * @param pForbiddenOrig points to an edge array (of the original graph) specifying which edges are not
	 *                       allowed to be crossed. May be a 0-pointer, in which case no edges are forbidden.
	 * @param pEdgeSubGraphs points to an edge array (of the original graph) specifying to which subgraph an edge belongs.
	 * @return the status of the result.
	 */
	ReturnType operator()(PlanRep &pr,
		int cc,
		int & crossingNumber,
		const EdgeArray<int>      *pCostOrig = nullptr,
		const EdgeArray<bool>     *pForbiddenOrig = nullptr,
		const EdgeArray<uint32_t> *pEdgeSubGraphs = nullptr)
	{
		return call(pr, cc, crossingNumber, pCostOrig, pForbiddenOrig, pEdgeSubGraphs);
	}

protected:
	//! Actual algorithm call that needs to be implemented by derived classes.
	/**
	 * @param pr             represents the input graph as well as the computed planarized representation
	 *                       after the call. \p pr has to be initialzed as a PlanRep of the input graph and
	 *                       is modified to obatain the planarized representation (crossings are replaced
	 *                       by dummy vertices with degree four).
	 * @param cc             is the index of the connected component in \p pr that is considered.
	 * @param crossingNumber is assigned the number of crossings.
	 * @param pCostOrig      points to an edge array (of the original graph) that gives the cost of each edge.
	 *                       May be a 0-pointer, in which case all edges have cost 1.
	 * @param pForbiddenOrig points to an edge array (of the original graph) specifying which edges are not
	 *                       allowed to be crossed. May be a 0-pointer, in which case no edges are forbidden.
	 * @param pEdgeSubGraphs points to an edge array (of the original graph) specifying to which subgraph an edge belongs.
	 * @return the status of the result.
	 */
	virtual ReturnType doCall(PlanRep &pr,
		int cc,
		const EdgeArray<int>      *pCostOrig,
		const EdgeArray<bool>     *pForbiddenOrig,
		const EdgeArray<uint32_t> *pEdgeSubGraphs,
		int &crossingNumber) = 0;

	OGDF_MALLOC_NEW_DELETE
};

}
