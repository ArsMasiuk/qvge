/** \file
 * \brief The Sparse Table Algorithm for the Least Common Ancestor
 *  problem as proposed by Bender and Farach-Colton.
 *
 * \author Stephan Beyer (port), Matthias Woste (original code)
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

#include <ogdf/tree/LCA.h>

#ifdef OGDF_DEBUG
#include <ogdf/basic/simple_graph_alg.h>
#endif

namespace ogdf {

static node findRoot(const Graph &G) {
	for (node v : G.nodes) {
		if (v->indeg() == 0) {
			return v;
		}
	}
	return nullptr;
}

LCA::LCA(const Graph &G, node root)
	: m_root(root == nullptr ? findRoot(G) : root)
	, m_n(G.numberOfNodes())
	, m_len(2 * m_n - 1)
	, m_rangeJ(std::ilogb(m_len))
	, m_euler(m_len)
	, m_representative(G)
	, m_level(m_len)
	, m_table(m_len * m_rangeJ)
{
	if (m_n > 1) {
		OGDF_ASSERT(m_root != nullptr);
		OGDF_ASSERT(m_root->graphOf() == &G);
		dfs(G, m_root);
		buildTable();
	}
}

node LCA::call(node u, node v) const
{
	OGDF_ASSERT(u->graphOf() == m_root->graphOf());
	OGDF_ASSERT(v->graphOf() == m_root->graphOf());

	return m_n == 1 ? m_root : m_euler[rmq(m_representative[v], m_representative[u])];
}

void LCA::dfs(const Graph &G, node root)
{
	OGDF_ASSERT(isSimple(G));
	OGDF_ASSERT(isArborescence(G));
	ArrayBuffer<std::pair<node,int>> todo;
	ArrayBuffer<adjEntry> adjStack;
	int dfscounter = 0;
	todo.push(std::make_pair(root, 0));
	adjStack.push(root->firstAdj());

	while (!todo.empty()) {
		const node u = todo.top().first;
		const int level = todo.top().second;
		adjEntry adj = adjStack.popRet();

		m_euler[dfscounter] = u;
		m_level[dfscounter] = level;
		m_representative[u] = dfscounter;

		while (adj && adj->theEdge()->source() != u) {
			adj = adj->succ();
		}
		if (adj) {
			node v = adj->twinNode();
			adjStack.push(adj->succ());
			todo.push(std::pair<node,int>(v, level + 1));
			adjStack.push(v->firstAdj());
		} else {
			todo.pop();
		}
		++dfscounter;
	}
}

void LCA::buildTable()
{
	for (int i = 0; i < m_len - 1; ++i) {
		sparseTable(i, 1) = (m_level[i] < m_level[i+1] ? i : i+1);
	}
	sparseTable(m_len - 1, 1) = m_len - 1;

	for (int j = 2; j <= m_rangeJ; ++j) {
		for (int i = 0; i < m_len; ++i) {
			int &tn = sparseTable(i, j);
			int &t1 = sparseTable(i, j - 1);
			OGDF_ASSERT(t1 >= 0);
			OGDF_ASSERT(t1 < m_len);
			int ri = i + (1 << (j-1));
			if (ri < m_len) {
				int &t2 = sparseTable(ri, j - 1);
				OGDF_ASSERT(t2 >= 0);
				OGDF_ASSERT(t2 < m_len);
				if (m_level[t1] < m_level[t2]) {
					tn = t1;
				} else {
					tn = t2;
				}
			} else {
				tn = t1;
			}
		}
	}
}

int LCA::rmq(int i, int j) const
{
	if (i > j) std::swap(i, j);
	if (j - i <= 1) {
		if (m_level[i] < m_level[j]) {
			return i;
		} else {
			return j;
		}
	}
	// lookup minima in one precomputed interval at the start and one at the end
	const int k = std::ilogb(j - i);
	const int interval1 = sparseTable(i, k);
	const int interval2 = sparseTable(j - (1 << k) + 1, k);
	OGDF_ASSERT(interval1 >= 0);
	OGDF_ASSERT(interval1 < m_len);
	OGDF_ASSERT(interval2 >= 0);
	OGDF_ASSERT(interval2 < m_len);
	// return the smaller one
	return m_level[interval1] < m_level[interval2] ? interval1 : interval2;
}

}
