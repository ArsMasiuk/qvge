/** \file
 * \brief Declaration of interface for edge insertion algorithms
 *
 * \author Hoi-Ming Wong
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

#include <ogdf/upward/UpwardPlanRep.h>
#include <ogdf/basic/Module.h>

namespace ogdf {

class OGDF_EXPORT UpwardEdgeInserterModule : public Module{
public:
	//! Initializes an edge insertion module.
	UpwardEdgeInserterModule() { }

	// destruction
	virtual ~UpwardEdgeInserterModule() { }

	/**
	 * \brief Inserts all edges in \p origEdges into \p UPR.
	 *
	 * @param UPR is the input upward planarized representation of a FUPS and will also receive the result.
	 * @param origEdges is the list of original edges (edges in the original graph
	 *			of \p UPR) that have to be inserted.
	 * \return the status of the result.
	 */
	ReturnType call(UpwardPlanRep &UPR, const List<edge> &origEdges) {
		return doCall(UPR, origEdges, nullptr, nullptr);
	}

	/**
	 * \brief Inserts all edges in \p origEdges with given costs into \p UPR.
	 *
	 * @param UPR is the input upward planarized representation of a FUPS and will also receive the result.
	 * @param costOrig points to an edge array containing the costs of original edges; edges in
	 *        \p UPR without an original edge have zero costs.
	 * @param origEdges is the list of original edges (edges in the original graph
	 *        of \p UPR) that have to be inserted.
	 * \return the status of the result.
	 */
	ReturnType call(UpwardPlanRep &UPR,
		const EdgeArray<int> &costOrig,
		const List<edge> &origEdges)
	{
		return doCall(UPR, origEdges, &costOrig, nullptr);
	}

	/**
	 * \brief Inserts all edges in \p origEdges with given forbidden edges into \p UPR.
	 *
	 * @param UPR is the input upward planarized representation of a FUPS and will also receive the result.
	 * @param costOrig points to an edge array containing the costs of original edges; edges in
	 *        \p UPR without an original edge have zero costs.
	 * @param forbidOriginal points to an edge array indicating if an original edge is
	 *        forbidden to be crossed.
	 * @param origEdges is the list of original edges (edges in the original graph
	 *        of \p UPR) that have to be inserted.
	 */
	ReturnType call(UpwardPlanRep &UPR,
		const EdgeArray<int>  &costOrig,
		const EdgeArray<bool> &forbidOriginal,
		const List<edge> &origEdges)
	{
		return doCall(UPR, origEdges, &costOrig, &forbidOriginal);
	}

	/**
	 * \brief Inserts all edges in \p origEdges with given forbidden edges into \p UPR.
	 *
	 * \pre No forbidden edge may be in \p origEdges.
	 *
	 * @param UPR is the input upward planarized representation of a FUPS and will also receive the result.
	 * @param forbidOriginal points to an edge array indicating if an original edge is
	 *        forbidden to be crossed.
	 * @param origEdges is the list of original edges (edges in the original graph
	 *        of \p UPR) that have to be inserted.
	 * \return the status of the result.
	 */
	ReturnType call(UpwardPlanRep &UPR,
		const EdgeArray<bool> &forbidOriginal,
		const List<edge> &origEdges)
	{
		return doCall(UPR, origEdges, nullptr, &forbidOriginal);
	}

protected:
	/**
	 * \brief Actual algorithm call that has to be implemented by derived classes.
	 *
	 * @param UPR is the input upward planarized representation of a FUPS and will also receive the result.
	 * @param origEdges is the list of original edges (edges in the original graph
	 *        of \p UPR) that have to be inserted.
	 * @param costOrig points to an edge array containing the costs of original edges; edges in
	 *        \p UPR without an original edge have zero costs.
	 * @param forbiddenEdgeOrig points to an edge array indicating if an original edge is
	 *        forbidden to be crossed.
	 */
	virtual ReturnType doCall(UpwardPlanRep &UPR,
		const List<edge> &origEdges,
		const EdgeArray<int>  *costOrig,
		const EdgeArray<bool> *forbiddenEdgeOrig
		) = 0;

	OGDF_MALLOC_NEW_DELETE
};

}
