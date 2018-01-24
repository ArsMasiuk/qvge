/** \file
 * \brief Implementation of methods used in HierarchyLevelsBase
 *
 * \author Carsten Gutwenger, Paweł Schmidt
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

#include <ogdf/layered/CrossingMinInterfaces.h>

namespace ogdf {

// calculation of edge crossings between level i and i+1
// implementation by Michael Juenger, Decembre 2000, adapted by Carsten Gutwenger
// implements the algorithm by Barth, Juenger, Mutzel
int HierarchyLevelsBase::calculateCrossings(int i) const
{
#if 0
	const Level &L = *m_pLevel[i];             // level i
	const int nUpper = m_pLevel[i+1]->size();  // number of nodes on level i+1
#endif

	const LevelBase &L = (*this)[i];             // level i
	const int nUpper = (*this)[i+1].size();  // number of nodes on level i+1

	int nc = 0; // number of crossings

	int fa = 1;
	while (fa < nUpper) {
		fa *= 2;
	}

	int nTreeNodes = 2*fa - 1; // number of tree nodes
	--fa; // "first address:" indexincrement in tree

	Array<int> nin(0,nTreeNodes-1,0);

	for (int j = 0; j < L.size(); ++j) {
#if 0
		const Array<node> &adjNodes = m_upperAdjNodes[L[j]];
#endif
		const Array<node> &adjNodes = this->adjNodes(L[j], TraversingDir::upward); // m_upperAdjNodes[L[j]];

		for (auto &adjNode : adjNodes) {
			// index of tree node for vertex adjNode[k]
#if 0
			int index = m_pos[adjNodes[k]] + fa;
#endif
			int index = pos(adjNode) + fa;
			nin[index]++;

			while (index > 0) {
				if (index % 2) {
					nc += nin[index+1]; // new crossing
				}
				index = (index - 1) / 2;
				nin[index]++;
			}
		}
	}

	return nc;
}

int HierarchyLevelsBase::calculateCrossings() const
{
	int nCrossings = 0;

	for (int i = 0; i < this->high(); ++i) {
		nCrossings += calculateCrossings(i);
	}

	return nCrossings;
}

}
