/** \file
 * \brief Implementation of GreedyInsertHeuristic
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

#include <ogdf/layered/GreedyInsertHeuristic.h>

namespace ogdf {

void GreedyInsertHeuristic::init(const HierarchyLevels &levels)
{
	m_weight.init(levels.hierarchy());
	m_crossingMatrix = new CrossingsMatrix(levels);
}

void GreedyInsertHeuristic::cleanup()
{
	m_weight.init();
	delete m_crossingMatrix;
}

void GreedyInsertHeuristic::call(Level &L)
{
	m_crossingMatrix->init(L);
	int index, i;

	// initialisation & priorisation
	for (i = 0; i < L.size(); i++) {
		double prio = 0;
		for (index = 0; index < L.size(); index++) {
			prio += (*m_crossingMatrix)(i,index);
		}

		// stable quicksort: no need for unique prio
		m_weight[L[i]] = prio;
	}

	L.sort(m_weight);
}

}
