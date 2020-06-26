/** \file
 * \brief Implementation of some graph generators
 *
 * \author Carsten Gutwenger, Markus Chimani
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


#include <ogdf/basic/graph_generators/deterministic.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/Array2D.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/basic/Math.h>

#include <ogdf/planarity/PlanarizationGridLayout.h>
#include <ogdf/planarlayout/SchnyderLayout.h>
using std::minstd_rand;
using std::uniform_int_distribution;
using std::uniform_real_distribution;


namespace ogdf {

void customGraph(Graph &G, int n, List<std::pair<int,int>> edges, Array<node> &nodes)
{
	nodes.init(n);

	G.clear();

	for (int i = 0; i < n; i++) {
		nodes[i] = G.newNode();
	}

	for (auto e : edges) {
		G.newEdge(nodes[std::get<0>(e)], nodes[std::get<1>(e)]);
	}
}


void circulantGraph(Graph &G, int n, Array<int> jumps)
{
	G.clear();
	Array<node> nodes(n);
	for (int i=0; i<n; i++) {
		nodes[i] = G.newNode();
	}
	Array2D<bool> buildEdge(0,n-1,0,n-1,false);
	auto pos_modulo = [&n](int i) {return (i % n + n) % n;};
	for (int s: jumps) {
		for (int i=0; i<n; i++) {
			buildEdge(i, pos_modulo(i+s)) = true;
			buildEdge(i, pos_modulo(i-s)) = true;
		}
	}
	for (int i=0; i<n; i++) {
		for (int j=i; j<n; j++) {
			if (buildEdge(i,j)) {
				G.newEdge(nodes[i], nodes[j]);
			}
		}
	}
}


void regularTree(Graph& G, int n, int children)
{
	G.clear();
	node* id2node = new node[n];
	id2node[0] = G.newNode();
	for(int i=1; i<n; i++) {
		G.newEdge(id2node[(i-1)/children], id2node[i] = G.newNode());
	}
	delete[] id2node;
}


void completeGraph(Graph &G, int n)
{
	G.clear();

	Array<node> v(n);

	int i,j;
	for(i = n; i-- > 0;)
		v[i] = G.newNode();

	for(i = n; i-- > 0;)
		for(j = i; j-- > 0;)
			G.newEdge(v[i],v[j]);
}

void completeKPartiteGraph(Graph &G, const Array<int> &signature)
{
	G.clear();
	if (signature.size() <= 0) {
		return;
	}

	Array<Array<node>> partitions(signature.size());

	// generate nodes in partitions
	for (int i = 0; i < partitions.size(); ++i) {
		OGDF_ASSERT(signature[i] > 0);
		partitions[i].init(signature[i]);
		for (int j = 0; j < signature[i]; ++j) {
			partitions[i][j] = G.newNode();
		}
	}

	// generate edges
	for (int i = 0; i < partitions.size(); ++i) {
		for (node u : partitions[i]) {
			for (int j = i+1; j < partitions.size(); ++j) {
				for (node v : partitions[j]) {
					G.newEdge(u, v);
				}
			}
		}
	}
}

void completeBipartiteGraph(Graph &G, int n, int m)
{
	completeKPartiteGraph(G, {n, m});
}


void wheelGraph(Graph &G, int n)
{
	G.clear();
	if (n <= 2) return;

	node center = G.newNode();
	node n0 = nullptr;
	node n1 = nullptr;

	while (n--) {
		node n2 = G.newNode();
		G.newEdge(center, n2);
		if (n1)
			G.newEdge(n1, n2);
		else
			n0 = n2;
		n1 = n2;
	}
	G.newEdge(n1, n0);
}


void suspension(Graph &G, int n)
{
	if(n == 0) return;
	OGDF_ASSERT( n>0 );

	List<node> nds;
	G.allNodes(nds);
	while (n--) {
		node n0 = G.newNode();
		for(node v : nds)
			G.newEdge(n0,v);
	}
}


void cubeGraph(Graph &G, int n)
{
	OGDF_ASSERT(n >= 0);
	OGDF_ASSERT(n < 8*(int)sizeof(int)-1); // one sign bit, one less to be safe
	G.clear();

	int c = 1 << n;
	Array<node> lu(c);
	for(int i=0; i<c; ++i) {
		lu[i] = G.newNode();
		int q = 1;
		while( q <= i ) {
			if(q&i) G.newEdge(lu[i^q],lu[i]);
			q <<= 1;
		}
	}
}


void gridGraph(Graph &G, int n, int m, bool loopN, bool loopM)
{
	G.clear();
	Array<node> front(0,n-1,nullptr);
	Array<node> fringe(0,n-1,nullptr);
	node first = nullptr;
	node last = nullptr;
	node cur;
	for(int j = m; j-- > 0;) {
		for(int i = n; i-- > 0;) {
			cur = G.newNode();
			if(!last) first=cur;
			else G.newEdge(last,cur);
			if(fringe[i]) G.newEdge(fringe[i],cur);
			else front[i] = cur;
			fringe[i] = cur;
			last = cur;
		}
		if(loopN)
			G.newEdge(last, first);
		last = nullptr;
	}
	if(loopM) {
		for(int i = n; i-- > 0;) {
			G.newEdge(fringe[i],front[i]);
		}
	}
}


void petersenGraph(Graph &G, int n, int m)
{
	G.clear();
	Array<node> inner(0, n-1, nullptr);
	node first = nullptr;
	node last = nullptr;
	for(int i = n; i-- > 0;) {
		node outn = G.newNode();
		node inn = G.newNode();
		G.newEdge(outn,inn);
		inner[i]=inn;
		if(!last) first=outn;
		else G.newEdge(last,outn);
		last = outn;
	}
	G.newEdge(last, first);
	for(int i = n; i-- > 0;) {
		G.newEdge(inner[i],inner[(i+m)%n]);
	}
}


void regularLatticeGraph(Graph &G, int n, int k)
{
	OGDF_ASSERT(n >= 4); // For a circle with all even degrees, we need at least four nodes.
	OGDF_ASSERT(k > 0);
	OGDF_ASSERT(k <= n-2);
	OGDF_ASSERT(k % 2 == 0);

	Array<int> jumps = Array<int>(k/2);
	for (int i = 0; i < k/2; i++) {
		jumps[i] = i+1;
	}
	circulantGraph(G, n, jumps);
}


void emptyGraph(Graph &G, int nodes)
{
	G.clear();
	for (int i = 0; i < nodes; i++) {
		G.newNode();
	}
}

}
