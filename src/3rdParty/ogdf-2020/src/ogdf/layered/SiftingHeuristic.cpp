/** \file
 * \brief Implementation of SiftingHeuristic
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

#include <ogdf/layered/SiftingHeuristic.h>

namespace ogdf {

SiftingHeuristic::SiftingHeuristic() :
  m_crossingMatrix(nullptr),
  m_strategy(Strategy::LeftToRight) { }

SiftingHeuristic::SiftingHeuristic(const SiftingHeuristic &crossMin) :
  m_crossingMatrix(nullptr),
  m_strategy(crossMin.m_strategy) { }


void SiftingHeuristic::init(const HierarchyLevels &levels)
{
	cleanup();
	m_crossingMatrix = new CrossingsMatrix(levels);
}

SiftingHeuristic::~SiftingHeuristic()
{
	cleanup();
}

void SiftingHeuristic::cleanup()
{
	delete m_crossingMatrix;
	m_crossingMatrix = nullptr;
}

void SiftingHeuristic::call(Level &L)
{
	List<node> vertices;
	int i;

	const int n = L.size();

	m_crossingMatrix->init(L); // initialize crossing matrix

	if (m_strategy == Strategy::LeftToRight || m_strategy == Strategy::Random) {
		for (i = 0; i < n; i++) {
			vertices.pushBack(L[i]);
		}

		if (m_strategy == Strategy::Random) {
			vertices.permute();
		}

	} else { // m_strategy == Strategy::DescDegree
		int max_deg = 0;

		for (i = 0; i < n; i++) {
			int deg = L.adjNodes(L[i]).size();
			if (deg > max_deg) max_deg = deg;
		}

		Array<List<node>, int> bucket(0, max_deg);
		for (i = 0; i < n; i++) {
			bucket[L.adjNodes(L[i]).size()].pushBack(L[i]);
		}

		for (i = max_deg; i >= 0; i--) {
			while(!bucket[i].empty()) {
				vertices.pushBack(bucket[i].popFrontRet());
			}
		}
	}

	for(i = 0; i< vertices.size(); i++) {
		int dev = 0;

		// sifting left
		for(; i > 0; --i) {
			dev = dev - (*m_crossingMatrix)(i-1,i) + (*m_crossingMatrix)(i,i-1);
			L.swap(i-1,i);
			m_crossingMatrix->swap(i-1,i);
		}

		// sifting right and searching optimal position
		int opt = dev, opt_pos = 0;
		for (; i < n-1; ++i) {
			dev = dev - (*m_crossingMatrix)(i,i+1) + (*m_crossingMatrix)(i+1,i);
			L.swap(i,i+1);
			m_crossingMatrix->swap(i,i+1);
			if (dev <= opt) {
				opt = dev; opt_pos = i+1;
			}
		}

		// set optimal position
		for (; i > opt_pos; --i) {
			L.swap(i-1,i);
			m_crossingMatrix->swap(i-1,i);
		}
	}
}

}
