/** \file
 * \brief Implementation of split heuristic.
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


#include <ogdf/layered/SplitHeuristic.h>

namespace ogdf
{

void SplitHeuristic::init (const HierarchyLevels &levels)
{
	cleanup();
	m_cm = new CrossingsMatrix(levels);
}

void SplitHeuristic::cleanup()
{
	delete m_cm;
	m_cm = nullptr;
}

// ordinary call
void SplitHeuristic::call(Level &L)
{
	m_cm->init(L);
	m_buffer = Array<node>(L.size());

	recCall(L, 0, L.size() - 1);

	m_buffer = Array<node>(-1);
}

// SimDraw call
void SplitHeuristic::call(Level &L, const EdgeArray<uint32_t> *edgeSubGraphs)
{
	// only difference to call is the different calculation of the crossingsmatrix
	m_cm->init(L, edgeSubGraphs);
	m_buffer = Array<node>(L.size());

	recCall(L, 0, L.size() - 1);

	m_buffer = Array<node>(-1);
}

void SplitHeuristic::recCall(Level &L, int low, int high)
{
	if (high <= low) return;

	const HierarchyLevels &levels = L.levels();
	CrossingsMatrix &crossings = *m_cm;
	int up = high, down = low;

	// chooses L[low] as pivot
	int i;
	for (i = low+1; i <= high; i++)
	{
		if (crossings(i,low) < crossings(low,i))
			m_buffer[down++] = L[i];
	}

	// use two for-loops in order to keep the number of swaps low
	for (i = high; i >= low+1; i--)
	{
		if (crossings(i,low) >= crossings(low,i))
			m_buffer[up--] = L[i];
	}

	m_buffer[down] = L[low];

	for (i = low; i < high; i++)
	{
		int j = levels.pos(m_buffer[i]);
		if (i != j)
		{
			L.swap(i,j);
			crossings.swap(i,j);
		}
	}

	recCall(L,low,down-1);
	recCall(L,up+1,high);
}

}
