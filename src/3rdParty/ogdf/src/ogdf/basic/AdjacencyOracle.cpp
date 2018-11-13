/** \file
 * \brief  Implementation of class AjacencyOracle
 *
 * This class is used to efficiently test if two vertices
 * are adjacent. It is basically a wrapper for a 2D-Matrix.
 * This file contains the code for the construction of the
 * matrix and the query function.
 *
 * \author Rene Weiskircher
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

AdjacencyOracle::AdjacencyOracle(const Graph &G)
  : m_nodeNum(G)
{
	int i = 1;
	for(node v : G.nodes) m_nodeNum[v] = i++;
	int nodeNum = i-1;
	m_adjacencyMatrix.init(1, i, 1, i);
	for(i = 1; i < nodeNum; i++)
		for(int j = i+1; j <= nodeNum; j++)
			m_adjacencyMatrix(i, j) = false;
	for(edge e : G.edges) {
		int num1 = m_nodeNum[e->source()];
		int num2 = m_nodeNum[e->target()];
		m_adjacencyMatrix(min(num1, num2), max(num1, num2)) = true;
	}
}

bool AdjacencyOracle::adjacent(const node v, const node w) const
{
	// Only the entries in the 2D matrix where the first
	// index is smaller than the second is used. So we have to
	// pay attention that the first index is smaller than the second.
	int num1 = m_nodeNum[v];
	int num2 = m_nodeNum[w];
	return m_adjacencyMatrix(min(num1, num2), max(num1, num2));
}

}
