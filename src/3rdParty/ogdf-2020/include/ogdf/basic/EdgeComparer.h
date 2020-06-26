/** \file
 * \brief Declares EdgeComparer class.
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

#include <ogdf/planarity/PlanRep.h>
#include <ogdf/basic/GraphAttributes.h>

namespace ogdf {

/**
 * Compares adjacency entries based on the position of the nodes
 * given by GraphAttribute layout information. An adjacency entry
 * considered greater than another if it comes after the other
 * in a clockwise manner.
 *
 * @ingroup comparer
 * @todo Check if sorting order fits adjacency list.
 */
class OGDF_EXPORT EdgeComparer : public VComparer<adjEntry>
{
public:
	/**
	 * Constructor for a given PlanRep and given GraphAttributes
	 *
	 * Assumes that \p PR is a PlanRep on original \p AG and that \p PR is
	 * not normalized, i.e., if there are bends in \p AG, they do not have
	 * a counterpart in \p PR (all nodes in \p PR have an original) temporary:
	 * we assume that we have two adjacency entries at a common point, so we
	 * leave the check for now if they meet and at which point.
	 */
	EdgeComparer(const GraphAttributes& AG, const PlanRep& PR) : m_AG(&AG), m_PR(&PR) { }

	/**
	 * Constructor for given GraphAttributes
	 *
	 * Compares the edges directly in \p AG.
	 */
	explicit EdgeComparer(const GraphAttributes &AG) : m_AG(&AG), m_PR(nullptr) {}

	int compare(const adjEntry &e1, const adjEntry &e2) const override;

	/**
	 * Checks if vector from \p u to \p v lies within the 180-degree halfcircle before
	 * the vector from \p u to \p w in clockwise order (i.e. twelve o'clock lies before 1).
	 */
	bool before(const DPoint &u, const DPoint &v, const DPoint &w) const;

private:
	/**
	 * Returns 1 if \p v lies to the left of the line through \p u and \p w,
	 * -1 if it lies to the right, and 0 if it lies on that line.
	 */
	int orientation(
		const DPoint &u,
		const DPoint &v,
		const DPoint &w) const;

	const GraphAttributes *m_AG;
	const PlanRep *m_PR;
};

}
