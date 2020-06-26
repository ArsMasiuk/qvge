/** \file
 * \brief Declares the class TriconnectedShellingOrder...
 *
 * ...which computes a shelling order for a triconnected planar graph.
 *
 * \author
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

#include <ogdf/planarlayout/ShellingOrderModule.h>

namespace ogdf {

//! Computation of a shelling order for a triconnected and
//! simple (no multi-edges, no self-loops) planar graph
class OGDF_EXPORT TriconnectedShellingOrder : public ShellingOrderModule
{
public:
	TriconnectedShellingOrder() {
		m_baseRatio = 0.33;
	}

protected:
	// does the actual computation; must be overridden by derived classes
	// the computed order is returned in partition
	virtual void doCall(
		const Graph &G,
		adjEntry adj,
		List<ShellingOrderSet> &partition) override;
};

}
