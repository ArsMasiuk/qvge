/** \file
 * \brief Declaration of class MinimumEdgeDistances which maintains
 *        minimum distances between attached edges at a vertex
 *       (deltas and epsilons)
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

#include <ogdf/orthogonal/OrthoRep.h>


namespace ogdf {


//! Maintains input sizes for improvement compaction (deltas and epsilons)
template <class ATYPE>
class MinimumEdgeDistances {
public:
	// constructor
	MinimumEdgeDistances(const Graph &G, ATYPE sep) : m_delta(G), m_epsilon(G)
	{
		m_sep = sep;
	}

	// returns delta_s(v)^i (with i = 0 => l, i = 1 => r)
	const ATYPE &delta(node v, OrthoDir s, int i) const {
		OGDF_ASSERT(0 <= int(s));
		OGDF_ASSERT(int(s) <= 3);
		OGDF_ASSERT(0 <= i);
		OGDF_ASSERT(i <= 1);
		return m_delta[v].info[static_cast<int>(s)][i];
	}

	ATYPE &delta(node v, OrthoDir s, int i) {
		OGDF_ASSERT(0 <= int(s));
		OGDF_ASSERT(int(s) <= 3);
		OGDF_ASSERT(0 <= i);
		OGDF_ASSERT(i <= 1);
		return m_delta[v].info[static_cast<int>(s)][i];
	}

	// returns epsilon_s(v)^i (with i = 0 => l, i = 1 => r)
	const ATYPE &epsilon(node v, OrthoDir s, int i) const {
		OGDF_ASSERT(0 <= int(s));
		OGDF_ASSERT(int(s) <= 3);
		OGDF_ASSERT(0 <= i);
		OGDF_ASSERT(i <= 1);
		return m_epsilon[v].info[static_cast<int>(s)][i];
	}

	ATYPE &epsilon(node v, OrthoDir s, int i) {
		OGDF_ASSERT(0 <= int(s));
		OGDF_ASSERT(int(s) <= 3);
		OGDF_ASSERT(0 <= i);
		OGDF_ASSERT(i <= 1);
		return m_epsilon[v].info[static_cast<int>(s)][i];
	}

	ATYPE separation() const {
		return m_sep;
	}

	void separation(ATYPE sep) {m_sep = sep;}


private:
	struct InfoType {
		ATYPE info[4][2];
	};

	NodeArray<InfoType> m_delta;
	NodeArray<InfoType> m_epsilon;
	ATYPE m_sep;
};

}
