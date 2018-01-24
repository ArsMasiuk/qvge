/** \file
 * \brief Implementation of GreedySwitchHeuristic
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

#include <ogdf/layered/GreedySwitchHeuristic.h>

namespace ogdf {

void GreedySwitchHeuristic::init(const HierarchyLevels &levels)
{
	delete m_crossingMatrix;
	m_crossingMatrix = new CrossingsMatrix(levels);
}

void GreedySwitchHeuristic::cleanup()
{
	delete m_crossingMatrix;
	m_crossingMatrix = nullptr;
}

void GreedySwitchHeuristic::call(Level &L)
{
	m_crossingMatrix->init(L);
	int index;
	bool nolocalmin;

	do {
		nolocalmin = false;

		for (index = 0; index < L.size() - 1; index++) {
			if ((*m_crossingMatrix)(index,index+1) > (*m_crossingMatrix)(index+1,index)) {

				nolocalmin = true;

				L.swap(index,index+1);
				m_crossingMatrix->swap(index,index+1);
			}
		}
	} while (nolocalmin);
}

}
