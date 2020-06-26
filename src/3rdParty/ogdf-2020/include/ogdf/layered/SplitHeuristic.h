/** \file
 * \brief Declaration of class SplitHeuristic.
 *
 * \author Andrea Wagner
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

#include <ogdf/basic/EdgeArray.h>
#include <ogdf/layered/CrossingsMatrix.h>
#include <ogdf/simultaneous/TwoLayerCrossMinSimDraw.h>

namespace ogdf {


//! The split heuristic for 2-layer crossing minimization.
/**
 * @ingroup gd-layered-crossmin
 */
class OGDF_EXPORT SplitHeuristic : public TwoLayerCrossMinSimDraw
{
public:
	//! Creates a new instance of the split heuristic.
	SplitHeuristic() : m_cm(nullptr) { }

	//! Creates a new instance of the split heuristic.
	SplitHeuristic(const SplitHeuristic &crossMin) : m_cm(nullptr) { }

	~SplitHeuristic() {
		cleanup();
	}

	//! Returns a new instance of the splitheurisitc with the same option settings.
	TwoLayerCrossMinSimDraw *clone() const override { return new SplitHeuristic(*this); }

	//! Initializes crossing minimization for hierarchy \a H.
	void init (const HierarchyLevels &levels) override;

	//! Calls the split heuristic for level \p L.
	void call (Level &L) override;

	//! Calls the median heuristic for level \p L (simultaneous drawing).
	void call (Level &L, const EdgeArray<uint32_t> *edgeSubGraphs) override;

	//! Does some clean-up after calls.
	void cleanup () override;

private:
	CrossingsMatrix *m_cm;
	Array<node> m_buffer;

	void recCall(Level&, int low, int high);
};

}
