/** \file
 * \brief Implements class ShellingOrder.
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

#include <ogdf/planarlayout/ShellingOrder.h>
#include <ogdf/basic/SList.h>


namespace ogdf {


void ShellingOrder::init(const Graph &G, const List<ShellingOrderSet> &partition)
{
	m_pGraph = &G;
	m_V.init(1,partition.size());
	m_rank.init(G);

	int i = 1;
	for (const ShellingOrderSet &S : partition)
	{
		for(int j = 1; j <= S.len(); ++j)
			m_rank[S[j]] = i;

		m_V[i++] = S;
	}
}


void ShellingOrder::initLeftmost(
	const Graph &G,
	const List<ShellingOrderSet> &partition)
{
	m_pGraph = &G;
	m_V.init(1,partition.size());
	m_rank.init(G);

	NodeArray<SListPure<const ShellingOrderSet *> > crSets(G);
	ArrayBuffer<node> outerfaceStack(G.numberOfNodes());

	int i, j;

	for(const ShellingOrderSet &S : partition) {
		node cr = S.right();
		if (cr != nullptr)
			crSets[cr].pushBack(&S);
	}

	const ShellingOrderSet &V1 = partition.front();
	for (j = V1.len(); j >= 2; j--)
		outerfaceStack.push(V1[j]);

	m_V[1] = V1;

	i = 2;
	while (!outerfaceStack.empty()) {
		node cr = outerfaceStack.top();
		if (crSets[cr].empty())
			outerfaceStack.pop();
		else {
			m_V[i] = *(crSets[cr].popFrontRet());
			for (j = len(i); j >= 1; j--)
				outerfaceStack.push ( (m_V[i])[j] );
			i++;
		}
	}


	for (i = 1; i <= length(); i++) {
		for (j = 1; j <= m_V[i].len(); ++j) {
			m_rank [(m_V[i])[j]] = i;
		}
	}
}

}
