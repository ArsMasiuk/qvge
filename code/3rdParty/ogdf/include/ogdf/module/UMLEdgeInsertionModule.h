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

#include <ogdf/planarity/PlanRepLight.h>

#include <ogdf/basic/Logger.h>
#include <ogdf/basic/Module.h>
#include <ogdf/basic/Timeouter.h>


namespace ogdf {

//! Interface for UML edge insertion algorithms.
/**
 * UML edge insertion algorithms take care that generalization edges do not cross in the
 * resulting planarzation.
 *
 * \see SubgraphPlanarizerUML
 */
class OGDF_EXPORT UMLEdgeInsertionModule : public Module, public Timeouter {
public:

	//! Initializes a UML edge insertion module (default constructor).
	UMLEdgeInsertionModule() { }

	//! Initializes a UML edge insertion module (copy constructor).
	UMLEdgeInsertionModule(const UMLEdgeInsertionModule &eim) : Timeouter(eim) { }

	//! Destructor
	virtual ~UMLEdgeInsertionModule() { }

	//! Returns a new instance of the UML edge insertion module with the same option settings.
	virtual UMLEdgeInsertionModule *clone() const = 0;


	//! Inserts all edges in \p origEdges into \p pr while avoiding crossings between generalizations.
	/**
	 * @param pr        is the input planarized representation and will also receive the result.
	 * @param origEdges is the array of original edges (edges in the original graph of \p pr)
	 *                  that have to be inserted.
	 * @return the status of the result.
	 */
	ReturnType call(PlanRepLight &pr, const Array<edge> &origEdges)
	{
		return doCall(pr, origEdges, nullptr, nullptr);
	}

	//! Inserts all edges in \p origEdges with given costs into \p pr while avoiding crossings between generalizations.
	/**
	 * @param pr        is the input planarized representation and will also receive the result.
	 * @param costOrig  is an edge array containing the costs of original edges; edges in
	 *                  \p pr without an original edge have zero costs.
	 * @param origEdges is the array of original edges (edges in the original graph of \p pr) that have to be inserted.
	 * @return the status of the result.
	 */
	ReturnType call(PlanRepLight &pr,
		const Array<edge> &origEdges,
		const EdgeArray<int> &costOrig)
	{
		return doCall(pr, origEdges, &costOrig, nullptr);
	}


	//! Inserts all edges in \p origEdges into \p pr while avoiding crossings between generalizations, optionally costs and subgraphs may be given.
	/**
	 * @param pr             is the input planarized representation and will also receive the result.
	 * @param origEdges      is the array of original edges (edges in the original graph of \p pr) that have to be inserted.
	 * @param pCostOrig      is an edge array containing the costs of original edges; edges in \p pr without an original edge have zero costs.
	 *                       May be a 0-pointer, in which case all edges have cost 1.
	 * @param pEdgeSubGraphs points to an edge array specifying to which subgraph an edge belongs.
	 *                       May be a 0-poiner, in which case no subgraphs / simultaneous embedding is used.
	 * @return the status of the result.
	 */
	ReturnType callEx(
		PlanRepLight              &pr,
		const Array<edge>         &origEdges,
		const EdgeArray<int>      *pCostOrig = nullptr,
		const EdgeArray<uint32_t> *pEdgeSubGraphs = nullptr)
	{
		return doCall(pr, origEdges, pCostOrig, pEdgeSubGraphs);
	}


protected:
	//! Actual algorithm call that has to be implemented by derived classes.
	/**
	 * @param pr             is the input planarized representation and will also receive the result.
	 * @param origEdges      is the array of original edges (edges in the original graph of \p pr) that have to be inserted.
	 * @param pCostOrig      is an edge array containing the costs of original edges; edges in \p pr without an original edge have zero costs.
	 *                       May be a 0-pointer, in which case all edges have cost 1.
	 * @param pEdgeSubGraphs points to an edge array specifying to which subgraph an edge belongs.
	 *                       May be a 0-poiner, in which case no subgraphs / simultaneous embedding is used.
	 * @return the status of the result.
	 */
	virtual ReturnType doCall(
		PlanRepLight              &pr,
		const Array<edge>         &origEdges,
		const EdgeArray<int>      *pCostOrig,
		const EdgeArray<uint32_t> *pEdgeSubGraphs) = 0;

	OGDF_MALLOC_NEW_DELETE
};

}
