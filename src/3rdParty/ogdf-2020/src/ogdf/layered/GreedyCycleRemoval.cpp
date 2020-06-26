/** \file
 * \brief Implementation of GreedyCycleRemoval
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

#include <ogdf/layered/GreedyCycleRemoval.h>
#include <ogdf/basic/simple_graph_alg.h>

namespace ogdf {

void GreedyCycleRemoval::dfs(node v, const Graph &G)
{
	m_visited[v] = true;

	int i;
	if (v->outdeg() == 0) i = m_min;
	else if (v->indeg() == 0) i = m_max;
	else i = v->outdeg() - v->indeg();

	m_item[v] = m_B[m_index[v] = i].pushBack(v);
	m_in [v] = v->indeg();
	m_out[v] = v->outdeg();
	m_counter++;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		node u = e->opposite(v);
		if (!m_visited[u])
			dfs(u,G);
	}
}

void GreedyCycleRemoval::call(const Graph &G, List<edge> &arcSet)
{
	arcSet.clear();

	m_max = m_min = 0;
	for (node v : G.nodes) {
		if (-v->indeg() < m_min) m_min = -v->indeg();
		if ( v->outdeg() > m_max) m_max =  v->outdeg();
	}

	if (G.numberOfEdges() == 0) return;

	m_visited.init(G,false); m_item.init(G);
	m_in     .init(G);       m_out .init(G);
	m_index  .init(G);       m_B   .init(m_min,m_max);

	SListPure<node> S_l, S_r;
	NodeArray<int> pos(G);

	m_counter = 0;
	for(node v : G.nodes) {
		if (m_visited[v]) continue;
		dfs(v,G);

		int max_i = m_max-1, min_i = m_min+1;

		for ( ; m_counter > 0; m_counter--) {
			node u;
			if (!m_B[m_min].empty()) {
				u = m_B[m_min].front(); m_B[m_min].popFront();
				S_r.pushFront(u);

			} else if (!m_B[m_max].empty()) {
				u = m_B[m_max].front(); m_B[m_max].popFront();
				S_l.pushBack(u);

			} else {
				while (m_B[max_i].empty())
					max_i--;
				while (m_B[min_i].empty())
					min_i++;

				if (abs(max_i) > abs(min_i)) {
					u = m_B[max_i].front(); m_B[max_i].popFront();
					S_l.pushBack(u);
				} else {
					u = m_B[min_i].front(); m_B[min_i].popFront();
					S_r.pushFront(u);
				}
			}

			m_item[u] = ListIterator<node>();

			for(adjEntry adj : u->adjEntries) {
				edge e = adj->theEdge();
				node w;
				if (e->target() == u) {
					w = e->source();
					if (m_item[w].valid()) {
						m_out[w]--;
						int i = m_index[w];
						m_B[i].del(m_item[w]);
						if (m_out[w] == 0)
							i = m_min;
						else if (m_in[w] == 0)
							i = m_max;
						else
							i--;
						m_item[w] = m_B[m_index[w] = i].pushBack(w);

						if (m_index[w] < min_i)
							min_i = m_index[w];
					}
				} else {
					w = e->target();
					if (m_item[w].valid()) {
						m_in[w]--;
						int i = m_index[w];
						m_B[i].del(m_item[w]);
						if (m_out[w] == 0)
							i = m_min;
						else if (m_in[w] == 0)
							i = m_max;
						else
							i++;
						m_item[w] = m_B[m_index[w] = i].pushBack(w);

						if (m_index[w] > max_i)
							max_i = m_index[w];
					}
				}
			}
		}

		int i = 0;
		for(node vi : S_l)
			pos[vi] = i++;
		for(node vi : S_r)
			pos[vi] = i++;

		S_l.clear(); S_r.clear();
	}

	for(edge e : G.edges) {
		if (pos[e->source()] >= pos[e->target()]) {
			arcSet.pushBack(e);
		}
	}

	m_visited.init(); m_item.init();
	m_in     .init(); m_out .init();
	m_index  .init(); m_B.init();
}

}
