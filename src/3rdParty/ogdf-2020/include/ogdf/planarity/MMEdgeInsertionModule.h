/** \file
 * \brief Declaration of interface for minor-monotone edge
 *        insertion algorithms.
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

#include <ogdf/planarity/PlanRepExpansion.h>
#include <ogdf/basic/Module.h>


namespace ogdf {

/**
 * \brief Interface for minor-monotone edge insertion algorithms.
 *
 * \see MMSubgraphPlanarizer
 */
class OGDF_EXPORT MMEdgeInsertionModule : public Module {
public:
	//! Initializes a minor-monotone edge insertion module.
	MMEdgeInsertionModule() { }

	// destruction
	virtual ~MMEdgeInsertionModule() { }

	/**
	 * \brief Inserts all edges in \p origEdges into \p PG.
	 *
	 * @param PG is the input planarized expansion and will also receive the result.
	 * @param origEdges is the list of original edges (edges in the original graph
	 *        of \p PG) that have to be inserted.
	 * \return the status of the result.
	 */
	ReturnType call(PlanRepExpansion &PG, const List<edge> &origEdges) {
		return doCall(PG, origEdges, nullptr);
	}

	/**
	 * \brief Inserts all edges in \p origEdges into \p PG and forbids crossing \p forbiddenEdges.
	 *
	 * @param PG is the input planarized expansion and will also receive the result.
	 * @param origEdges is the list of original edges (edges in the original graph
	 *        of \p PG) that have to be inserted.
	 * @param forbiddenEdgeOrig is an edge array indicating if an original edge is
	 *        forbidden to be crossed.
	 * \return the status of the result.
	 */
	ReturnType call(PlanRepExpansion &PG,
		const List<edge> &origEdges,
		const EdgeArray<bool> &forbiddenEdgeOrig)
	{
		return doCall(PG, origEdges, &forbiddenEdgeOrig);
	}

protected:
	/**
	 * \brief Actual algorithm call that has to be implemented by derived classes.
	 *
	 * @param PG is the input planarized expansion and will also receive the result.
	 * @param origEdges is the list of original edges (edges in the original graph
	 *        of \p PG) that have to be inserted.
	 * @param forbiddenEdgeOrig points to an edge array indicating if an original edge is
	 *        forbidden to be crossed.
	 */
	virtual ReturnType doCall(PlanRepExpansion &PG,
		const List<edge> &origEdges, const EdgeArray<bool> *forbiddenEdgeOrig) = 0;

	OGDF_MALLOC_NEW_DELETE
};

}
