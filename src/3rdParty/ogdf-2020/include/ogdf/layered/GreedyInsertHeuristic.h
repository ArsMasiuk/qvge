/** \file
 * \brief Declaration of class GreedyInsertHeuristic
 *
 * \author Till Schäfer
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
#include <ogdf/layered/CrossingsMatrix.h>
#include <ogdf/basic/NodeArray.h>

namespace ogdf {

//! The greedy-insert heuristic for 2-layer crossing minimization.
/**
 * @ingroup gd-layered-crossmin
 */
class OGDF_EXPORT GreedyInsertHeuristic : public LayerByLayerSweep
{
public:
	//! Creates a new instance of the greedy-insert heuristic.
	GreedyInsertHeuristic() { }

	//! Creates a new instance of the greedy-insert heuristic.
	GreedyInsertHeuristic(const GreedyInsertHeuristic &crossMin) { }

	//! Returns a new instance of the greed-insert heuristic with the same option settings.
	virtual LayerByLayerSweep *clone() const override
	{
		return new GreedyInsertHeuristic(*this);
	}

	//! Initializes weights and crossing minimization for hierarchy \a H.
	virtual void init (const HierarchyLevels &levels) override;

	//! Calls the greedy insert heuristic for level \p L.
	virtual void call (Level &L) override;

	//! Does some clean-up after calls.
	virtual void cleanup () override;

private:
	CrossingsMatrix *m_crossingMatrix;
	NodeArray<double> m_weight;
};

}
