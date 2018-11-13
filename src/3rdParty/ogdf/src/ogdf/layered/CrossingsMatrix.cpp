/** \file
 * \brief Implementation of crossings matrix.
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


#include <ogdf/layered/CrossingsMatrix.h>

namespace ogdf
{

CrossingsMatrix::CrossingsMatrix(const HierarchyLevels &levels)
{
	int max_len = 0;
	for (int i = 0; i < levels.size(); i++)
	{
		int len = levels[i].size();
		if (len > max_len)
			max_len = len;
	}

	map.init(max_len);
	matrix.init(0, max_len - 1, 0, max_len - 1);
	m_bigM = 10000;
}


void CrossingsMatrix::init(Level &L)
{
	for (int i = 0; i < L.size(); i++)
	{
		map[i] = i;
		for (int j = 0; j < L.size(); j++)
			matrix(i,j) = 0;
	}

	for (int i = 0; i < L.size(); i++)
	{
		node v = L[i];
		const Array<node> &L_adj_i = L.adjNodes(v);

		for(auto pos_adj_k : L_adj_i)
		{
			for (int j = i + 1; j < L.size(); j++)
			{
				const Array<node> &L_adj_j = L.adjNodes(L[j]);

				for (auto pos_adj_l : L_adj_j)
				{
					matrix(i,j) += (pos_adj_k > pos_adj_l);
					matrix(j,i) += (pos_adj_l > pos_adj_k);
				}
			}
		}
	}
}


void CrossingsMatrix::init(Level &L, const EdgeArray<uint32_t> *edgeSubGraphs)
{
	OGDF_ASSERT(edgeSubGraphs != nullptr);
	init(L);

	const HierarchyLevels &levels = L.levels();
	const GraphCopy &GC = levels.hierarchy();

	// calculate max number of graphs in edgeSubGraphs
	int max = 0;
	for(edge d : GC.original().edges) {
		for (int i = 31; i > max; i--)
		{
			if((*edgeSubGraphs)[d] & (1 << i))
				max = i;
		}
	}

	// calculation differs from ordinary init since we need the edges and not only the nodes
	for (int k = 0; k <= max; k++) {
		for (int i = 0; i < L.size(); i++)
		{
			node v = L[i];
			// H.direction == 1 if direction == upward
			if (levels.direction() == HierarchyLevelsBase::TraversingDir::upward) {
				for(adjEntry adj : v->adjEntries) {
					edge e = adj->theEdge();
					if ((e->source() == v) && ((*edgeSubGraphs)[GC.original(e)] & (1 << k))) {
						int pos_adj_e = levels.pos(e->target());
						for (int j = i+1; j < L.size(); j++) {
							node w = L[j];
							for(adjEntry adjW : w->adjEntries) {
								edge f = adjW->theEdge();
								if ((f->source() == w) && ((*edgeSubGraphs)[GC.original(f)] & (1 << k)))
								{
									int pos_adj_f = levels.pos(f->target());
									matrix(i,j) += m_bigM * (pos_adj_e > pos_adj_f);
									matrix(j,i) += m_bigM * (pos_adj_f > pos_adj_e);
								}
							}
						}
					}
				}
			}
			else {
				for(adjEntry adj : v->adjEntries) {
					edge e = adj->theEdge();
					if ((e->target() == v) && ((*edgeSubGraphs)[GC.original(e)] & (1 << k))) {
						int pos_adj_e = levels.pos(e->source());
						for (int j = i+1; j < L.size(); j++) {
							node w = L[j];
							for(adjEntry adjW : w->adjEntries) {
								edge f = adjW->theEdge();
								if ((f->target() == w) && ((*edgeSubGraphs)[GC.original(f)] & (1 << k)))
								{
									int pos_adj_f = levels.pos(f->source());
									matrix(i,j) += m_bigM * (pos_adj_e > pos_adj_f);
									matrix(j,i) += m_bigM * (pos_adj_f > pos_adj_e);
								}
							}
						}
					}
				}
			}
		}
	}
}

}
