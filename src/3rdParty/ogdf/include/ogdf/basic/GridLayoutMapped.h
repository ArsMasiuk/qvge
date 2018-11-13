/** \file
 * \brief Declaration of class GridLayoutMapped which extends GridLayout
 *        by a grid mapping mechanism.
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

#include <ogdf/basic/GridLayout.h>
#include <ogdf/planarity/PlanRep.h>
#include <ogdf/orthogonal/OrthoRep.h>

namespace ogdf {

/**
 * \brief Extends GridLayout by a grid mapping mechanism.
 *
 * @ingroup gd-helper
 */
class OGDF_EXPORT GridLayoutMapped : public GridLayout
{
	//! scaling to allow correct edge anchors
	static const int cGridScale;

public:

	// construction (determines mapping factor)
	GridLayoutMapped(const PlanRep &PG,
		const OrthoRep &OR,
		double separation,
		double cOverhang,
		int fineness = 4);


	// writes grid layout to layout using re-mapping
	void remap(Layout &drawing) override;

	// transforms real coordinates to grid coordinates
	int toGrid(double x) const {
		return cGridScale*int(m_fMapping * x + 0.5);
	}

	// transforms grid coordinates to real coordinates
	double toDouble(int i) const {
		return (i/cGridScale) / m_fMapping;
	}


	const NodeArray<int> &width() const { return m_gridWidth; }
	// returns a reference to the array storing grid widths of nodes
	NodeArray<int> &width() { return m_gridWidth; }

	const NodeArray<int> &height() const { return m_gridHeight; }
	// returns a reference to the array storing grid heights of nodes
	NodeArray<int> &height() { return m_gridHeight; }

	const int &width(node v) const { return m_gridWidth[v]; }
	// returns grid width of node v
	int &width(node v) { return m_gridWidth[v]; }

	const int &height(node v) const { return m_gridWidth[v]; }
	// returns grid height of node v
	int &height(node v) { return m_gridWidth[v]; }


private:
	NodeArray<int> m_gridWidth;  // grid width of nodes
	NodeArray<int> m_gridHeight; // grid heights of nodes

	const PlanRep *m_pPG;     // planarized representation of grid layout
	double m_fMapping;           // mapping factor
};

}
