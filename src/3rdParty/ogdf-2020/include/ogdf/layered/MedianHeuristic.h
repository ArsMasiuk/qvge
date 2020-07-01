/** \file
 * \brief Declaration of class MedianHeuristic
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

#include <ogdf/layered/LayerByLayerSweep.h>

namespace ogdf {

//! The median heuristic for 2-layer crossing minimization.
/**
 * @ingroup gd-layered-crossmin
 */
class OGDF_EXPORT MedianHeuristic : public LayerByLayerSweep
{
public:
	//! Creates a new instance of the median heuristic.
	MedianHeuristic() { }

	//! Creates a new instance of the median heuristic.
	MedianHeuristic(const MedianHeuristic &crossMin) { }

	//! Returns a new instance of the median heuristic with the same option settings.
	virtual LayerByLayerSweep *clone() const override
	{
		return new MedianHeuristic;
	}

	//! Initializes crossing minimization for hierarchy \a H.
	virtual void init (const HierarchyLevels &levels) override { m_weight.init(levels.hierarchy()); }

	//! Calls the median heuristic for level \p L.
	virtual void call (Level &L) override;

	//! Does some clean-up after calls.
	virtual void cleanup () override { m_weight.init(); }

private:
	NodeArray<int> m_weight; //!< The median weight of a node.
};

}
