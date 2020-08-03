/** \file
 * \brief Implementation of st-numbering algorithm
 *
 * \author Sebastian Leipert
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


#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/STNumbering.h>

namespace ogdf {

// Computes the DFN and LOW numbers of a biconnected component
// Uses DFS strategy
static void stSearch(const Graph &G, node v, int &count,
                     NodeArray<int> &low, NodeArray<int> &dfn,
                     NodeArray<edge> &dfsInEdge, NodeArray<edge> &followLowPath)
{
	dfn[v] = count;
	count++;
	low[v] = dfn[v];

	for (adjEntry adj : v->adjEntries) {
		node w = adj->twinNode();
		edge e = adj->theEdge();

		if (!dfn[w]) { // node not visited yet
			dfsInEdge[w] = e;
			stSearch(G,w,count,low,dfn,dfsInEdge,followLowPath);
			if (low[v] > low[w]) {
				low[v] = low[w];
				followLowPath[v] = e;
			}
		}
		else if (low[v] > dfn[w]) {
			low[v] = dfn[w];
			followLowPath[v] = e;
		}
	}
}

static bool stPath(ArrayBuffer<node> &path, node v, adjEntry &adj,
                   NodeArray<bool> &markedNode, EdgeArray<bool> &markedEdge,
                   NodeArray<int> &dfn, NodeArray<edge> &dfsInEdge,
                   NodeArray<edge> &followLowPath)
{
	edge e;
	node w;
	path.clear();

	if (!adj) {
		adj = v->firstAdj(); // no edge has been visited yet
	}
	do {
		e = adj->theEdge();
		adj = adj->succ();
		if (markedEdge[e]) {
			continue;
		}
		markedEdge[e] = true;

		w = e->opposite(v);

		if (dfsInEdge[w] == e) {
			path.push(v);
			while (!markedNode[w]) {
				e = followLowPath[w];
				path.push(w);
				markedNode[w] = true;
				markedEdge[e] = true;
				w = e->opposite(w);
			}
			return true;
		}
		else if (dfn[v] < dfn[w]) {
			path.push(v);
			while (!markedNode[w]) {
				e = dfsInEdge[w];
				path.push(w);
				markedNode[w] = true;
				markedEdge[e] = true;
				w = e->opposite(w);
			}
			return true;
		}
	} while (adj != nullptr);

	return false;
}

// Computes an st-Numbering.
// Precondition: G must be biconnected and simple.
// Exception: the Graph is allowed to have isolated nodes.
// The st-numbers are stored in NodeArray. Return value is
// the number t. It is 0, if the computation was unsuccessful.
// The nodes s and t may be specified. In this case
// s and t have to be adjacent.
// If s and t are set 0 and parameter randomized is set to true,
// the st edge is chosen to be a random edge in G.

int computeSTNumbering(const Graph &G,
	NodeArray<int> &numbering,
	node s,
	node t,
	bool randomized)
{

	int    count       = 1;

	// Stores for every vertex its LOW number
	NodeArray<int> low(G,0);
	// Stores for every vertex ist DFN number
	NodeArray<int> dfn(G,0);

	// Stores for every vertex if it has been visited dsuring the st-numbering
	NodeArray<bool> markedNode(G,false);
	// Stores for every edge if it has been visited dsuring the st-numbering
	EdgeArray<bool> markedEdge(G,false);

	// Stores for every node its ingoing edge of the dfs tree.
	NodeArray<edge> dfsInEdge(G,nullptr);

	// Stores a path of vertices that have not been visited.
	ArrayBuffer<node> path;

	//Stores for every node the outgoing, first edge on the
	// path that defines the low number of the node.
	NodeArray<edge> followLowPath(G,nullptr);

	edge st = nullptr;

	if (s && t)
	{
		bool found = false;
		for(adjEntry adj : s->adjEntries) {
			if (adj->twinNode() == t)
			{
				st = adj->theEdge();
				found = true;
				break;
			}
		}
		if (!found)
			return 0;
	}
	else if (s)
	{
		st = s->firstAdj()->theEdge();
		t = st->opposite(s);
	}
	else if (t)
	{
		st = t->firstAdj()->theEdge();
		s = st->opposite(t);
	}
	else
	{
		if(randomized) {
			// chose a random edge in G
			st = G.chooseEdge();
			if(!st) // graph is empty?
				return 0;
			s = st->source();
			t = st->target();

		} else {
			for(node v : G.nodes)
			{
				if (v->degree() > 0) {
					s = v;
					st = s->firstAdj()->theEdge();
					t = st->opposite(s);
					break;
				}
			}
		}
	}
	if (!s || !t)
		return 0;

	OGDF_ASSERT(st != nullptr);

	// Compute the DFN and LOW numbers
	// of the block.
	dfn[t] = count++;
	low[t] = dfn[t];
	stSearch(G,s,count,low,dfn,dfsInEdge,followLowPath);
	if (low[t] > low[s])
		low[t] = low[s];

	markedNode[s] = true;
	markedNode[t] = true;
	markedEdge[st] = true;

	ArrayBuffer<node> nodeStack; // nodeStack stores the vertices during the computation of the st-numbering.
	nodeStack.push(t);
	nodeStack.push(s);
	count = 1;
	node v = nodeStack.popRet();
	adjEntry adj = nullptr;
	while (v != t)
	{
		if (!stPath(path,v,adj,markedNode,markedEdge,dfn,dfsInEdge,followLowPath))
		{
			numbering[v] = count;
			count++;
			adj = nullptr;
		}
		else
		{
			while (!path.empty())
				nodeStack.push(path.popRet());
		}
		v = nodeStack.popRet();
	}
	numbering[t] = count;
	return count;
}

bool isSTNumbering(const Graph &G, NodeArray<int> &st_no,int max)
{
	bool   foundLow = false;
	bool   foundHigh = false;
	bool   it_is = true;

	for(node v : G.nodes)
	{
		if (v->degree() == 0)
			continue;

		foundHigh = foundLow = 0;
		if (st_no[v] == 1)
		{
			for(adjEntry adj : v->adjEntries)
			{
				if (st_no[adj->theEdge()->opposite(v)] == max)
					foundLow = foundHigh = 1;
			}
		}

		else if (st_no[v] == max)
		{
			for(adjEntry adj : v->adjEntries)
			{
				if (st_no[adj->theEdge()->opposite(v)] == 1)
					foundLow = foundHigh = 1;
			}
		}

		else
		{
			for(adjEntry adj : v->adjEntries)
			{
				if (st_no[adj->theEdge()->opposite(v)] < st_no[v])
					foundLow = 1;
				else if (st_no[adj->theEdge()->opposite(v)] > st_no[v])
					foundHigh = 1;
			}
		}
		if (!foundLow || !foundHigh)
			it_is = 0;
	}

	return it_is;
}

}
