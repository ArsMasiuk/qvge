
/** \file
 * \brief Implementation of DfsAcyclicSubgraph
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

#include <ogdf/layered/DfsAcyclicSubgraph.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Queue.h>

namespace ogdf {

void DfsAcyclicSubgraph::call(const Graph &G, List<edge> &arcSet)
{
	isAcyclic(G,arcSet);
}

void DfsAcyclicSubgraph::callUML(const GraphAttributes &AG, List<edge> &arcSet)
{
	const Graph &G = AG.constGraph();

	// identify hierarchies
	NodeArray<int> hierarchy(G,-1);
	int count = 0;
	int treeNum = -1;

	for(node v : G.nodes)
	{
		if(hierarchy[v] == -1) {
			int n = dfsFindHierarchies(AG,hierarchy,count,v);
			if(n > 1) treeNum = count;
			++count;
		}
	}

	arcSet.clear();

	// perform DFS on the directed graph formed by generalizations
	NodeArray<int> number(G,0), completion(G);
	int nNumber = 0, nCompletion = 0;

	for(node v : G.nodes) {
		if(number[v] == 0)
			dfsBackedgesHierarchies(AG,v,number,completion,nNumber,nCompletion);
	}

	// collect all backedges within a hierarchy
	// and compute outdeg of each vertex within its hierarchy
	EdgeArray<bool> reversed(G,false);
	NodeArray<int> outdeg(G,0);

	for(edge e : G.edges) {
		if(AG.type(e) != Graph::EdgeType::generalization || e->isSelfLoop())
			continue;

		node src = e->source(), tgt = e->target();

		outdeg[src]++;

		if (hierarchy[src] == hierarchy[tgt] &&
			number[src] >= number[tgt] && completion[src] <= completion[tgt])
			reversed[e] = true;
	}

	// topologial numbering of nodes within a hierarchy (for each hierarchy)
	NodeArray<int> numV(G);
	Queue<node> Q;
	int countV = 0;

	for(node v : G.nodes)
		if(outdeg[v] == 0)
			Q.append(v);

	while(!Q.empty()) {
		node v = Q.pop();

		numV[v] = countV++;

		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node w = e->source();
			if (w != v && --outdeg[w] == 0) {
				Q.append(w);
			}
		}
	}

	// "direct" associations
	for(edge e : G.edges) {
		if(AG.type(e) == Graph::EdgeType::generalization || e->isSelfLoop())
			continue;

		node src = e->source(), tgt = e->target();

		if(hierarchy[src] == hierarchy[tgt]) {
			if (numV[src] < numV[tgt])
				reversed[e] = true;
		} else {
			if(hierarchy[src] == treeNum || (hierarchy[tgt] != treeNum &&
				hierarchy[src] > hierarchy[tgt]))
				reversed[e] = true;
		}
	}

	// collect reversed edges
	for(edge e : G.edges)
		if(reversed[e])
			arcSet.pushBack(e);
}


int DfsAcyclicSubgraph::dfsFindHierarchies(
	const GraphAttributes &AG,
	NodeArray<int> &hierarchy,
	int i,
	node v)
{
	int count = 1;
	hierarchy[v] = i;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if(AG.type(e) != Graph::EdgeType::generalization)
			continue;

		node w = e->opposite(v);
		if(hierarchy[w] == -1)
			count += dfsFindHierarchies(AG,hierarchy,i,w);
	}

	return count;
}


void DfsAcyclicSubgraph::dfsBackedgesHierarchies(
	const GraphAttributes &AG,
	node v,
	NodeArray<int> &number,
	NodeArray<int> &completion,
	int &nNumber,
	int &nCompletion)
{
	number[v] = ++nNumber;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if(AG.type(e) != Graph::EdgeType::generalization)
			continue;

		node w = e->target();

		if (number[w] == 0)
			dfsBackedgesHierarchies(AG,w,number,completion,nNumber,nCompletion);
	}

	completion[v] = ++nCompletion;
}

}
