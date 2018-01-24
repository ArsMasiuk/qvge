/** \file
 * \brief Declares EdgeComparerSimple class.
 *
 * \author Bernd Zey
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
 * Compares incident edges of a node based on the position of
 * the last bend point or the position of the adjacent node
 * given by the layout information of the graph.
 */
class OGDF_EXPORT EdgeComparerSimple : public VComparer<adjEntry>
{
public:
	EdgeComparerSimple(const GraphAttributes& AG, const node v, bool useBends = true)
	: m_basis(v), m_AG(&AG), m_useBends(useBends) { }

	int compare(const adjEntry &e1, const adjEntry &e2) const override;

private:
	node m_basis;
	const GraphAttributes *m_AG;
	bool m_useBends; //!< \c true iff the algorithm should consider the bend-points

};

}
