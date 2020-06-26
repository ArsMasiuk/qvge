/** \file
 * \brief Implementation of MedianHeuristic
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

#include <ogdf/layered/MedianHeuristic.h>

namespace ogdf {

void MedianHeuristic::call(Level &L)
{
	const HierarchyLevels &levels = L.levels();

	for (int i = 0; i <= L.high(); ++i) {
		node v = L[i];

		const Array<node> &adjNodes = L.adjNodes(v);
		const int high = adjNodes.high();

		if (high < 0) {
			m_weight[v] = 0;
		} else if (high & 1) {
			m_weight[v] = levels.pos(adjNodes[high/2]) + levels.pos(adjNodes[1+high/2]);
		} else {
			m_weight[v] = 2*levels.pos(adjNodes[high/2]);
		}
	}

	L.sort(m_weight, 0, 2*levels.adjLevel(L.index()).high());
}

}
