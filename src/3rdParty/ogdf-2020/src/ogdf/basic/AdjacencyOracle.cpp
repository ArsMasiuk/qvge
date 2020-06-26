/** \file
 * \brief Implementation of ogdf::AdjacencyOracle
 *
 * \author Stephan Beyer
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

#include <ogdf/basic/AdjacencyOracle.h>

namespace ogdf {

//! Returns the base index of row \p j for an array containing a lower triangular matrix
inline static int getRowIndex(int j) {
	return (j + 1) * j / 2;
}

AdjacencyOracle::AdjacencyOracle(const Graph &G, int degreeThreshold)
  : m_nodeNum{G, -1}
{
	int i{0};
	for (node v : G.nodes) {
		if (v->degree() > degreeThreshold) {
			m_nodeNum[v] = i++;
		}
	}

	m_adjacencies.resize(getRowIndex(i), false);

	for (node v : G.nodes) {
		if (m_nodeNum[v] >= 0) {
			for (adjEntry adj : v->adjEntries) {
				node w{adj->twinNode()};
				if (m_nodeNum[w] >= 0) {
					m_adjacencies[index(v, w)] = true;
				}
			}
		}
	}
}

bool AdjacencyOracle::adjacent(node v, node w) const {
	if (m_nodeNum[v] >= 0 && m_nodeNum[w] >= 0) {
		return m_adjacencies[index(v, w)];
	}

	if (w->degree() < v->degree()) {
		std::swap(v, w);
	}
	OGDF_ASSERT(m_nodeNum[v] < 0);

	for (adjEntry adj : v->adjEntries) {
		if (adj->twinNode() == w) {
			return true;
		}
	}
	return false;
}

int AdjacencyOracle::index(node v, node w) const {
	int i{m_nodeNum[v]};
	int j{m_nodeNum[w]};
	if (i > j) {
		std::swap(i, j);
	}

	return getRowIndex(j) + i;
}

}
