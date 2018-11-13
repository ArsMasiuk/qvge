/** \file
 * \brief Implementation of BarycenterHeuristic
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

#include <ogdf/layered/BarycenterHeuristic.h>

namespace ogdf {

void BarycenterHeuristic::call(Level &L)
{
	const HierarchyLevels &levels = L.levels();

	for (int i = 0; i <= L.high(); ++i) {
		node v = L[i];
		long sumpos = 0L;

		const Array<node> &adjNodes = L.adjNodes(v);
		for (int j = 0; j <= adjNodes.high(); ++j) {
			sumpos += levels.pos(adjNodes[j]);
		}

		m_weight[v] = (adjNodes.high() < 0)
		  ? 0.0 : double(sumpos) / double(adjNodes.size());
	}

	L.sort(m_weight);
}

}
