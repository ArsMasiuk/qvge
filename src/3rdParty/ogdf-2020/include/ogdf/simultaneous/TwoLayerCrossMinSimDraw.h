/** \file
 * \brief Declaration of interface for two-layer crossing
 *        minimization algorithms for Simultaneous Drawing.
 *
 * \author Michael Schulz
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

#include <ogdf/layered/LayerByLayerSweep.h>

namespace ogdf {

class OGDF_EXPORT TwoLayerCrossMinSimDraw : public LayerByLayerSweep
{
public:
	//! Initializes a two-layer crossing minimization module.
	TwoLayerCrossMinSimDraw() : LayerByLayerSweep() { }

	//! Returns a new instance of the two-layer crossing minimization module with the same option settings.
	virtual TwoLayerCrossMinSimDraw *clone() const = 0;

	/**
	* \brief Performs crossing minimization for level \p L.
	*
	* @param L is the level in the hierarchy on which nodes are permuted; the
	*        neighbor level (fixed level) is determined by the hierarchy.
	* @param esg points to an edge array which specifies to which subgraphs
	*        an edge belongs; there are up to 32 possible subgraphs each of which
	*        is represented by a bit of an <code>uint32_t</code>.
	*/
	virtual void call(Level &L, const EdgeArray<uint32_t> *esg) = 0;

	void call(Level &L) = 0;
};

}
