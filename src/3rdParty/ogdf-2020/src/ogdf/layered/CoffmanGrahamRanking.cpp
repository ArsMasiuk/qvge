/** \file
 * \brief Definition of coffman graham ranking algorithm for Sugiyama
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

#include <ogdf/layered/CoffmanGrahamRanking.h>
#include <ogdf/layered/DfsAcyclicSubgraph.h>
#include <ogdf/basic/GraphCopy.h>

namespace ogdf {

CoffmanGrahamRanking::CoffmanGrahamRanking() : m_w(3)
{
	m_subgraph.reset(new DfsAcyclicSubgraph());
}


void CoffmanGrahamRanking::call (const Graph& G, NodeArray<int>& rank)
{
	rank.init(G);
	GraphCopy gc(G);

	m_subgraph->callAndReverse(gc);
	removeTransitiveEdges(gc);

	List<Tuple2<node, int> > ready_nodes;
	NodeArray<int> deg(gc);
	NodeArray<int> pi(gc);
	m_s.init(gc);

	List<edge> edges;

	for(node v : gc.nodes) {
		edges.clear();
		v->inEdges(edges);
		deg[v] = edges.size();
		if (deg[v] == 0) {
			ready_nodes.pushBack(Tuple2<node,int>(v,0));
		}
		m_s[v].init(deg[v]);
	}

	int i = 1;
	while(!ready_nodes.empty()) {
		node v = ready_nodes.popFrontRet().x1();
		pi[v] = i++;

		for(adjEntry adj : v->adjEntries) {
			if ((adj->theEdge()->source()) == v) {
				node u = adj->twinNode();
				m_s[u].insert(pi[v]);
				if (--deg[u] == 0) {
					insert(u,ready_nodes);
				}
			}
		}
	}


	List<node> ready, waiting;

	for(node v : gc.nodes) {
		edges.clear();
		v->outEdges(edges);
		deg[v] = edges.size();
		if (deg[v] == 0) {
			insert(v,ready,pi);  // ready.append(v);
		}
	}

	int k;
	// 	for all ranks
	for (k = 1; !ready.empty(); k++) {

		for (i = 1; i <= m_w && !ready.empty(); i++) {
			node u = ready.popFrontRet();
			rank[gc.original(u)] = k;

			u->inEdges<List<edge>>(edges);
			for (edge e : edges) {
				if (--deg[e->source()] == 0){
					waiting.pushBack(e->source());
				}
			}
		}

		while (!waiting.empty()) {
			insert(waiting.popFrontRet(), ready, pi);
		}
	}

	k--;
	for(node v : G.nodes) {
		rank[v] = k - rank[v];
	}

	m_s.init();
}


void CoffmanGrahamRanking::insert (node u, List<Tuple2<node,int> > &ready_nodes)
{
	int j = 0;

	for( ListReverseIterator<Tuple2<node,int> > it = ready_nodes.rbegin(); it.valid(); ++it) {
		node v     = (*it).x1();
		int  sigma = (*it).x2();

		if (sigma < j) {
			ready_nodes.insertAfter(Tuple2<node,int>(u,j), it);
			return;
		}

		if (sigma > j) continue;

		const _int_set &x = m_s[u], &y = m_s[v];
		int k = min(x.length(), y.length());

		while (j < k && x[j] == y[j]) {
			j++;
		}

		if (j == k) {
			if (x.length() < y.length()) continue;

			(*it).x2() = k;
			ready_nodes.insertAfter(Tuple2<node,int>(u,sigma), it);
			return;
		}

		if (x[j] < y[j]) continue;

		(*it).x2() = j;
		ready_nodes.insert(Tuple2<node,int>(u,sigma), it);
		return;
	}

	ready_nodes.pushFront(Tuple2<node,int>(u,j));
}


void CoffmanGrahamRanking::insert (node v, List<node> &ready, const NodeArray<int> &pi)
{
	for( ListReverseIterator<node> it = ready.rbegin(); it.valid(); ++it) {
		if (pi[v] <= pi[*it]) {
			ready.insertAfter(v, it);
			return;
		}
	}

	ready.pushFront(v);
}


void CoffmanGrahamRanking::dfs(node v)
{
	ArrayBuffer<node> stack;
	stack.push(v);

	while (!stack.empty()) {
		node w = stack.popRet();
		m_mark[w] |= 1; // Mark w as visited.

		// Set 4-bit for every successor u of w with set 2-bit.
		for (adjEntry adj : w->adjEntries) {
			if (adj->isSource()) {
				node u = adj->twinNode();
				if (m_mark[u] & 2) {
					m_mark[u] |= 4;
				}

				// If u is unvisited, push it to the stack.
				if ((m_mark[u] & 1) == 0) {
					stack.push(u);
				}
			}
		}
	}
}


void CoffmanGrahamRanking::removeTransitiveEdges(Graph& G)
{
	List<edge> vout;

	m_mark.init(G,0);
	ArrayBuffer<node> visited;

	for (node v : G.nodes) {
		v->outEdges<List<edge>>(vout);

		// Mark all successors of v with the 2-bit.
		for (edge e : vout) {
			node w = e->target();
			m_mark[w] = 2;
		}

		// Call dfs for all unvisited successors of v.
		for (edge e : vout) {
			node w = e->target();
			if ((m_mark[w] & 1) == 0) {
				dfs(w);
			}
		}

		// Delete all edges from v to nodes with set 4-bit.
		for (edge e : vout) {
			node w = e->target();
			if (m_mark[w] & 4) {
				G.delEdge(e);
			}
		}

		// Reset mark-bits for all visited nodes.
		while (!visited.empty()) {
			m_mark[visited.popRet()] = 0;
		}
	}

	m_mark.init();
}

}
