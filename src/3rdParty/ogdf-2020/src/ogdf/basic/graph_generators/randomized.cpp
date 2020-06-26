/** \file
 * \brief Implementation of some randomized graph generators
 *
 * \author Carsten Gutwenger, Markus Chimani, Jöran Schierbaum
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

#include <unordered_set>

#include <ogdf/basic/graph_generators.h>
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
using std::unordered_set;

namespace ogdf {

void randomRegularGraph(Graph &G, int n, int d)
{
	OGDF_ASSERT(n >= 0);
	OGDF_ASSERT(n*d % 2 == 0);

	minstd_rand rng(randomSeed());

	do {
		G.clear();

		// create the set of required half-edges
		std::vector<node> pairs(n*d);
		for (int i = 0; i < n; i++) {
			node v = G.newNode();

			for (int j = 0; j < d; j++) {
				pairs[i * d + j] = v;
			}
		}

		bool promising = true;
		while (promising && !pairs.empty()) {
			// test whether feasible pair exists (=promising)
			promising = false;
			for (auto i = 0u; !promising && i < pairs.size(); i++) {
				for (auto j = i+1; !promising && j < pairs.size(); j++) {
					node v = pairs[i];
					node w = pairs[j];
					promising |= v != w && G.searchEdge(v, w) == nullptr;
				}
			}

			// randomly pick a feasible pair if possible
			bool edgeCreated = !promising;
			while (!edgeCreated) {
				uniform_int_distribution<> dist(0, int(pairs.size()) - 1);

				int idV = dist(rng);
				int idW = dist(rng);
				node v = pairs[idV];
				node w = pairs[idW];

				// create edge if feasible and update required half-edges
				if (v != w && G.searchEdge(v, w) == nullptr) {
					G.newEdge(v, w);

					if (idV < idW) {
						std::swap(idV, idW);
					}

					pairs.erase(pairs.begin() + idV);
					pairs.erase(pairs.begin() + idW);

					edgeCreated = true;
				}
			}
		}
	} while (G.numberOfEdges() != n*d/2);
}

void randomGraph(Graph &G, int n, int m)
{
	G.clear();
	if (n == 0) return;

	Array<node> v(n);

	int i;
	for(i = 0; i < n; i++)
		v[i] = G.newNode();

	minstd_rand rng(randomSeed());
	uniform_int_distribution<> dist(0,n-1);

	for(i = 0; i < m; i++) {
		int v1 = dist(rng);
		int v2 = dist(rng);

		G.newEdge(v[v1],v[v2]);
	}
}

constexpr int getMaxNumberEdges(int n)
{
	return n * (n-1) / 2;
}

constexpr int getEdgeIndex(int a, int b, int n, int max)
{
	return max - getMaxNumberEdges(n - a) + b - a - 1;
}

/**
 * Auxiliary function for ogdf::randomSimpleGraph and ogdf::randomSimpleConnectedGraph.
 * Adds/removes randomly picked edges into the graph and keeps track of edges already
 * added/removed by using a mask bool array.
 *
 * @param G the graph to be generated
 * @param n the number of nodes
 * @param m the number of edges
 * @param preEdges
 *   An array mask containing booleans for each edge (given by its unique computed index).
 *   If an edge's value is true, it will be contained in \p G.
 *   After the generation, all contained edges will be set to true.
 * @param preAdded The number of edges that MUST BE ADDED. (Must equal the number of true values in \p preEdges.)
 */
static bool randomSimpleGraphByMask(Graph& G, int n, int m, Array<bool>& preEdges, int preAdded = 0)
{
	OGDF_ASSERT(preEdges.low() == 0);

	G.clear();

	if (n == 0 && m == 0) {
		return true;
	}

	if (n < 1) {
		return false;
	}

	const int max = preEdges.size();
	OGDF_ASSERT(max == getMaxNumberEdges(n));
	OGDF_ASSERT(max == preEdges.high() + 1);

	if (m > max) {
		return false;
	}

	Array<node> v(n);

	for (int i = 0; i < n; i++) {
		v[i] = G.newNode();
	}

	if (m == 0) {
		return true;
	}

	minstd_rand rng(randomSeed());
	uniform_int_distribution<> dist_a(0, n-1);
	uniform_int_distribution<> dist_b(0, n-2);

	bool maskRemoveNotAdd = (m > max / 2);
	if (maskRemoveNotAdd) {
		m = max - m;
	} else {
		m -= preAdded;
	}

	Array<bool> mask(0, max-1, false);
	while (m > 0) {
		int a = dist_a(rng);
		int b = dist_b(rng);
		if (b >= a) {
			b++;
		} else {
			std::swap(a, b);
		}

		int i = getEdgeIndex(a, b, n, max);
		if (!mask[i] && !preEdges[i]) {
			mask[i] = true;
			m--;
		}
	}

	for (int a = 0; a < n; a++) {
		for (int b = a + 1; b < n; b++) {
			int i = getEdgeIndex(a, b, n, max);
			if (preEdges[i] || mask[i] == !maskRemoveNotAdd) {
				G.newEdge(v[a], v[b]);
			}
		}
	}

	return true;
}

/**
 * Auxiliary function for ogdf::randomSimpleGraph and ogdf::randomSimpleConnectedGraph.
 * Adds randomly picked edges into the graph and keeps track of edges already added
 * by using a hashset.
 *
 * @param G the graph to be generated
 * @param n the number of nodes
 * @param m the number of edges
 * @param preEdges
 *   A vector containing std::pair<int,int> for each edge that must be added into G.
 *   After the generation, all contained edges will be part of G.
 * @pre preEdges may only contain edges (i, j) where j > i and no duplicate edges
 */
static bool randomSimpleGraphBySet(Graph& G, int n, int m, std::vector<std::pair<int, int>>& preEdges)
{
	G.clear();

	if (n == 0 && m == 0) {
		return true;
	}

	if (n < 1) {
		return false;
	}

	const int max = getMaxNumberEdges(n);
	if (m > max || m < (int) preEdges.size()) {
		return false;
	}

	Array<node> v(n);
	for (int i = 0; i < n; i++) {
		v[i] = G.newNode();
	}

	unordered_set<int> edgeIndices(2*m);
	for (auto e: preEdges) {
		OGDF_ASSERT(e.first < e.second);
		OGDF_ASSERT(edgeIndices.find(getEdgeIndex(e.first, e.second, n, max)) == edgeIndices.end());
		edgeIndices.emplace(getEdgeIndex(e.first, e.second, n, max));
		G.newEdge(v[e.first], v[e.second]);
		m--;
	}

	if (m == 0) {
		return true;
	}

	minstd_rand rng(randomSeed());
	uniform_int_distribution<> dist_a(0, n-1);
	uniform_int_distribution<> dist_b(0, n-2);

	while (m > 0) {
		int a = dist_a(rng);
		int b = dist_b(rng);
		if (b >= a) {
			b++;
		} else {
			std::swap(a, b);
		}

		int edgeIndex = getEdgeIndex(a, b, n, max);
		if (edgeIndices.find(edgeIndex) == edgeIndices.end()) {
			edgeIndices.emplace(edgeIndex);
			G.newEdge(v[a], v[b]);
			m--;
		}
	}

	return true;
}

bool randomSimpleGraph(Graph &G, int n, int m)
{
	const int max = getMaxNumberEdges(n);
	// Set implementation showed to be only efficient for very sparse graphs, i.e. <0.5% of possible edges
	if (m > 0.005 * max) {
		Array<bool> preEdges(0, max-1, false);
		return randomSimpleGraphByMask(G, n, m, preEdges, 0);
	} else {
		std::vector<std::pair<int, int>> preEdges(0);
		return randomSimpleGraphBySet(G, n, m, preEdges);
	}
}


// Algorithm based on PreZER/LogZER from http://dx.doi.org/10.1145/1951365.1951406
bool randomSimpleGraphByProbability(Graph &G, int n, double pEdge)
{
	G.clear();

	if (pEdge > 1 || pEdge < 0)
		return false;

	Array<node> v(n);
	for (int i = 0; i < n; i++)
		v[i] = G.newNode();

	// Precomputation of cumulative probability distribution
	const int SIZE = 50; // good compromise for sparse and dense graphs (tested)
	double F[SIZE];
	for (int k = 0; k < SIZE; k++) {
		F[k] = 1 - pow(1 - pEdge, k + 1.0);
	}

	minstd_rand rng(randomSeed());
	uniform_real_distribution<> dist(0, 1);

	double log1p = log(1 - pEdge);
	std::pair<int, int> e = {0, 0};
	while (e.first < n - 1) {
		double alpha = dist(rng);
		// Determine how many sequentially enumerated edges to skip
		int skip = 1;
		while (skip-1 < SIZE && F[skip-1] <= alpha) {
			skip++;
		}
		if (skip-1 == SIZE) {
			skip = log(1 - alpha) / log1p + 1;
		}
		// Decoding from an edge index to vertex indices is too slow so we
		// iterate directly over the vertex indices
		while (skip != 0) {
			if (skip <= n - 1 - e.second) {
				e.second += skip;
				skip = 0;
			} else {
				e.first++;
				skip -= n - e.second;
				e.second = e.first + 1;
			}
		}
		if (e.first < n - 1) {
			G.newEdge(v[e.first], v[e.second]);
		}
	}

	return true;
}

bool randomSimpleConnectedGraph(Graph &G, int n, int m)
{
	if (m < n - 1) {
		G.clear();
		return false;
	}

	Graph tree;
	randomTree(tree, n);

	const int max = getMaxNumberEdges(n);
	// Set implementation showed to be only efficient for very sparse graphs, i.e. <0.5% of possible edges
	if (m > 0.005 * max) {
		Array<bool> preEdges(0, max - 1, false);
		for (edge e : tree.edges) {
			preEdges[getEdgeIndex(e->source()->index(), e->target()->index(), n, max)] = true;
		}
		return randomSimpleGraphByMask(G, n, m, preEdges, tree.numberOfEdges());
	} else {
		std::vector<std::pair<int, int>> preEdges;
		for (edge e : tree.edges) {
			preEdges.emplace_back(e->source()->index(), e->target()->index());
		}
		return randomSimpleGraphBySet(G, n, m, preEdges);
	}
}


void randomTree(Graph &G, int n, int maxDeg, int maxWidth)
{
	G.clear();

	if (n <= 0) return;
	if (maxDeg   <= 0) maxDeg   = n;
	if (maxWidth <= 0) maxWidth = n;

	int max = 0;
	Array<node> possible(n);
	Array<int> width(0,n,0);
	NodeArray<int> level(G,0);

	level[possible[0] = G.newNode()] = 0;
	--n;

	minstd_rand rng(randomSeed());

	while(n > 0) {
		uniform_int_distribution<> dist(0,max);
		int  i = dist(rng);
		node v = possible[i];

		if (width[level[v]+1] == maxWidth) {
			possible[i] = possible[max--];
			continue;
		}

		if (v->outdeg()+1 == maxDeg)
			possible[i] = possible[max--];

		node w = G.newNode();
		possible[++max] = w;
		G.newEdge(v,w);
		width[level[w] = level[v]+1]++;

		--n;
	}
}


void randomBiconnectedGraph(Graph &G, int n, int m)
{
	if (n < 3) n = 3;
	if (m < n) m = n;

	int kse = n-3; // number of split edge operations
	int kae = m-n; // number of add edge operations

	G.clear();

	Array<edge> edges(m);
	Array<node> nodes(n);

	// we start with a triangle
	nodes[0] = G.newNode();
	nodes[1] = G.newNode();
	nodes[2] = G.newNode();
	edges[0] = G.newEdge(nodes[0],nodes[1]);
	edges[1] = G.newEdge(nodes[1],nodes[2]);
	edges[2] = G.newEdge(nodes[2],nodes[0]);

	int nNodes = 3, nEdges = 3;

	minstd_rand rng(randomSeed());

	while(kse+kae > 0)
	{
		int p = uniform_int_distribution<>(1,kse+kae)(rng);

		if (p <= kse) {
			// split edge operation
			edge e = edges[uniform_int_distribution<>(0,nEdges-1)(rng)];
			edge e1 = G.split(e);

			edges[nEdges++] = e1;
			nodes[nNodes++] = e1->source();

			--kse;

		} else {
			// add edge operation
			int i = uniform_int_distribution<>(0,nNodes-1)(rng);
			int j = ( i + uniform_int_distribution<>(1,nNodes-1)(rng) ) % nNodes;

			edges[nEdges++] = G.newEdge(nodes[i], nodes[j]);

			--kae;
		}
	}
}

void randomTriconnectedGraph(Graph &G, int n, double p1, double p2)
{
	if(n < 4) n = 4;

	// start with K_4
	completeGraph(G,4);

	// nodes[0],...,nodes[i-1] is array of all nodes
	Array<node> nodes(n);

	int i = 0;
	for(node v : G.nodes)
		nodes[i++] = v;

	// Will be used below as array of neighbors of v
	Array<edge> neighbors(n);

	// used to mark neighbors
	//   0 = not marked
	//   1 = marked left
	//   2 = marked right
	//   3 = marked both
	Array<int>  mark(0,n-1,0);

	minstd_rand rng(randomSeed());

	for(; i < n; ++i)
	{
		// pick a random node
		node v = nodes[uniform_int_distribution<>(0,i-1)(rng)];

		// create a new node w such that v is split into v and w
		node w = nodes[i] = G.newNode();

		// build array of all neighbors
		int d = v->degree();

		int j = 0;
		for(adjEntry adj : v->adjEntries)
			neighbors[j++] = adj->theEdge();

		// mark two distinct neighbors for left
		for(j = 2; j > 0; ) {
			int r = uniform_int_distribution<>(0,d-1)(rng);
			if((mark[r] & 1) == 0) {
				mark[r] |= 1; --j;
			}
		}

		// mark two distinct neighbors for right
		for(j = 2; j > 0; ) {
			int r = uniform_int_distribution<>(0,d-1)(rng);
			if((mark[r] & 2) == 0) {
				mark[r] |= 2; --j;
			}
		}

		for(j = 0; j < d; ++j) {
			int m = mark[j];
			mark[j] = 0;

			// decide to with which node each neighbor is connected
			// (possible: v, w, or both)
			double x = uniform_real_distribution<>(0.0,1.0)(rng);
			switch(m)
			{
			case 0:
				if(x < p1)
					m = 1;
				else if(x < p1+p2)
					m = 2;
				else
					m = 3;
				break;
			case 1:
			case 2:
				if(x >= p1+p2) m = 3;
				break;
			}

			// move edge or create new one if necessary
			edge e = neighbors[j];
			switch(m)
			{
			case 2:
				if(v == e->source())
					G.moveSource(e,w);
				else
					G.moveTarget(e,w);
				break;
			case 3:
				G.newEdge(w,e->opposite(v));
				break;
			}
		}

		G.newEdge(v,w);
	}
}


void randomPlanarTriconnectedGraph(Graph &G, int n, double p1, double p2)
{
	if (n < 4) n = 4;

	// start with K_4
	completeGraph(G,4);

	planarEmbedPlanarGraph(G);

	// nodes[0],...,nodes[i-1] is array of all nodes
	Array<node> nodes(n);

	int i = 0;
	for(node v : G.nodes)
		nodes[i++] = v;

	minstd_rand rng(randomSeed());
	uniform_int_distribution<> dist_0_1(0,1);

	for(; i < n; ++i)
	{
		// pick a random node
		node v = nodes[uniform_int_distribution<>(0,i-1)(rng)];

		int m = v->degree();
		int a1 = uniform_int_distribution<>(0,m-1)(rng);
		int a2 = uniform_int_distribution<>(0,m-2)(rng);

		int j;
		adjEntry adj1, adj2;
		for(adj1 = v->firstAdj(), j = 0; j < a1; adj1 = adj1->succ(), ++j) ;
		for(adj2 = adj1->cyclicSucc(), j = 0; j < a2; adj2 = adj2->cyclicSucc(), ++j) ;

		adjEntry adj_b1 = adj2->cyclicPred();
		adjEntry adj_b2 = adj1->cyclicPred();

		nodes[i] = G.splitNode(adj1, adj2);

		if(adj1 == adj_b1)
			G.newEdge(adj_b1, adj2->twin());
		else if(adj2 == adj_b2)
			G.newEdge(adj2, adj_b1->twin(), Direction::before);
		else {
			double r = uniform_real_distribution<>(0.0,1.0)(rng);
			if(r <= p1) {
				int s = dist_0_1(rng);
				if(s == 0)
					G.newEdge(adj_b1, adj2->twin());
				else
					G.newEdge(adj2, adj_b1->twin(), Direction::before);
			}
		}

		double r = uniform_real_distribution<>(0.0,1.0)(rng);
		if(r <= p2) {
			int s = dist_0_1(rng);
			if(s == 0)
				G.newEdge(adj1, adj_b2->twin(), Direction::before);
			else
				G.newEdge(adj_b2, adj1->twin());
		}
	}
}


void randomPlanarTriconnectedGraph(Graph &G, int n, int m)
{
	if (n < 4) n = 4;
	if(n % 2) ++n; // need an even number

	// start with K_4
	completeGraph(G,4);

	planarEmbedPlanarGraph(G);

	// nodes[0],...,nodes[i-1] is array of all nodes
	Array<node> nodes(n);

	int i = 0;
	for(node v : G.nodes)
		nodes[i++] = v;

	minstd_rand rng(randomSeed());
	uniform_int_distribution<> dist_0_1(0,1);
	uniform_int_distribution<> dist_0_2(0,2);

	// create planar triconnected 3-graph
	for(; i < n; )
	{
		// pick a random node
		node v = nodes[uniform_int_distribution<>(0,i-1)(rng)];

		adjEntry adj2 = v->firstAdj();
		int r = dist_0_2(rng);
		while (r--) { // continue to r-th successor
			adj2 = adj2->succ();
		}
		adjEntry adj1 = adj2->cyclicSucc();

		nodes[i++] = G.splitNode(adj1,adj2);

		r = dist_0_1(rng);
		if(r == 0) {
			adjEntry adj = adj1->twin();
			G.newEdge(adj2,adj);
			nodes[i++] = G.splitNode(adj,adj->cyclicSucc()->cyclicSucc());

		} else {
			adjEntry adj = adj1->cyclicSucc()->twin();
			G.newEdge(adj2,adj,Direction::before);
			nodes[i++] = G.splitNode(adj->cyclicPred(),adj->cyclicSucc());
		}
	}

	nodes.init();
	Array<edge> edges(m);

	CombinatorialEmbedding E(G);
	Array<face> faces(2*n);

	i = 0;
	for(face f : E.faces) {
		if(f->size() >= 4)
			faces[i++] = f;
	}

	while(G.numberOfEdges() < m && i > 0)
	{
		int r = uniform_int_distribution<>(0,i-1)(rng);
		face f = faces[r];
		faces[r] = faces[--i];

		int p = uniform_int_distribution<>(0,f->size()-1)(rng);
		int j = 0;
		adjEntry adj, adj2;
		for(adj = f->firstAdj(); j < p; adj = adj->faceCycleSucc(), ++j) ;

		p = uniform_int_distribution<>(2, f->size()-2)(rng);
		for(j = 0, adj2 = adj; j < p; adj2 = adj2->faceCycleSucc(), ++j) ;

		edge e = E.splitFace(adj,adj2);

		f = E.rightFace(e->adjSource());
		if(f->size() >= 4) faces[i++] = f;

		f = E.rightFace(e->adjTarget());
		if(f->size() >= 4) faces[i++] = f;
	}
}


void randomPlanarConnectedGraph(Graph &G, int n, int m)
{
	if (n < 1) n = 1;
	if (m < n-1) m = n-1;
	if (m > 3*n-6) m = 3*n-6;

	G.clear();
	Array<node> nodes(n);

	nodes[0] = G.newNode();

	minstd_rand rng(randomSeed());

	// build tree
	for(int i = 1; i < n; ++i) {
		node on = nodes[uniform_int_distribution<>(0,i-1)(rng)];
		node nn = nodes[i] = G.newNode();
		G.firstNode()->degree();
		if(on->degree() > 1) {
			adjEntry adj = on->firstAdj();
			for(int fwd = uniform_int_distribution<>(0,on->degree()-1)(rng); fwd > 0; --fwd)
				adj = adj->succ();
			G.newEdge(nn, adj);
		} else {
			G.newEdge(nn, on);
		}
	}

	List<face> bigFaces; // not a triangle

	CombinatorialEmbedding E(G);
	bigFaces.pushBack(E.firstFace());
	for(int i = m-n+1; i-- > 0;) {
		ListIterator<face> fi = bigFaces.chooseIterator();
		face f = *fi;
		bigFaces.del(fi);

		List<adjEntry> fnodes;
		for(adjEntry adj : f->entries) {
			fnodes.pushBack(adj);
		}
		fnodes.permute();
		adjEntry adj1, adj2;
		bool okay = false;
		do {
			adj1 = fnodes.popFrontRet();
			node n1 = adj1->theNode();
			for(adjEntry adj : fnodes) {
				node n2 = adj->theNode();

				if(n1==n2 || adj1->faceCyclePred() == adj || adj->faceCyclePred() == adj1) {
					continue;
				}
				okay = true;
				for(adjEntry adjN1 : n1->adjEntries) {
					if(adjN1->twinNode() == n2) {
						okay = false;
						break;
					}
				}
				if (okay) {
					adj2 = adj;
					break;
				}
			}
		} while(!okay);

		edge ne = E.splitFace(adj1,adj2);

		face f1 = E.rightFace(ne->adjSource());
		face f2 = E.rightFace(ne->adjTarget());

		if (f1->size() > 3) bigFaces.pushBack(f1);
		if (f2->size() > 3) bigFaces.pushBack(f2);
	}
}


void randomPlanarBiconnectedGraph(Graph &G, int n, int m, bool multiEdges)
{
	if (n < 3) n = 3;
	if (m < n) m = n;
	if (m > 3*n-6) m = 3*n-6;

	int ke = n-3, kf = m-n;

	G.clear();

	Array<edge> edges(m);
	Array<face> bigFaces(m);
#if 0
	random_source S;
#endif

	// we start with a triangle
	node v1 = G.newNode(), v2 = G.newNode(), v3 = G.newNode();
	edges[0] = G.newEdge(v1,v2);
	edges[1] = G.newEdge(v2,v3);
	edges[2] = G.newEdge(v3,v1);

	CombinatorialEmbedding E(G);
	FaceArray<int> posBigFaces(E);
	int nBigFaces = 0, nEdges = 3;

	minstd_rand rng(randomSeed());

	while(ke+kf > 0) {
		int p = uniform_int_distribution<>(1,ke+kf)(rng);

		if (nBigFaces == 0 || p <= ke) {
			edge e  = edges[uniform_int_distribution<>(0,nEdges-1)(rng)];
			face f  = E.rightFace(e->adjSource());
			face fr = E.rightFace(e->adjTarget());

			edges[nEdges++] = E.split(e);

			if (f->size() == 4) {
				posBigFaces[f] = nBigFaces;
				bigFaces[nBigFaces++] = f;
			}
			if (fr->size() == 4) {
				posBigFaces[fr] = nBigFaces;
				bigFaces[nBigFaces++] = fr;
			}

			ke--;

		} else {
			int pos = uniform_int_distribution<>(0,nBigFaces-1)(rng);
			face f = bigFaces[pos];
			int df = f->size();
			int i = uniform_int_distribution<>(0,df-1)(rng);
			int j = uniform_int_distribution<>(2,df-2)(rng);

			adjEntry adj1;
			for (adj1 = f->firstAdj(); i > 0; adj1 = adj1->faceCycleSucc())
				i--;

			adjEntry adj2;
			for (adj2 = adj1; j > 0; adj2 = adj2->faceCycleSucc())
				j--;

			edge e = E.splitFace(adj1,adj2);
			edges[nEdges++] = e;

			face f1 = E.rightFace(e->adjSource());
			face f2 = E.rightFace(e->adjTarget());

			bigFaces[pos] = f1;
			posBigFaces[f1] = pos;
			if (f2->size() >= 4) {
				posBigFaces[f2] = nBigFaces;
				bigFaces[nBigFaces++] = f2;
			}
			if (f1->size() == 3) {
				bigFaces[pos] = bigFaces[--nBigFaces];
			}

			kf--;
		}
	}

	if (!multiEdges) {
		SListPure<edge> allEdges;
		EdgeArray<int> minIndex(G), maxIndex(G);

		parallelFreeSortUndirected(G,allEdges,minIndex,maxIndex);

		SListConstIterator<edge> it = allEdges.begin();
		edge ePrev = *it, e;
		for(it = ++it; it.valid(); ++it, ePrev = e) {
			e = *it;
			if (minIndex[ePrev] == minIndex[e] &&
				maxIndex[ePrev] == maxIndex[e])
			{
				G.move(e,
					e->adjTarget()->faceCycleSucc()->twin(), Direction::before,
					e->adjSource()->faceCycleSucc()->twin(), Direction::before);
			}
		}
	}
}

void randomUpwardPlanarBiconnectedDigraph(Graph &G, int n, int m)
{
	randomPlanarBiconnectedDigraph(G,n,m);
}

void randomPlanarBiconnectedDigraph(Graph &G, int n, int m, double p, bool multiEdges)
{
	OGDF_ASSERT(p >= 0);
	OGDF_ASSERT(p < 1.0);

	GraphAttributes GA(G, GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics);

	randomPlanarBiconnectedGraph(G,n,m,multiEdges);

	SchnyderLayout sl;
	sl.call(GA);

	for(edge e : G.edges) {

		node u = e->source();
		node v = e->target();

		bool x = GA.x(u) >  GA.x(v);
		bool y = GA.x(u) == GA.x(v) && GA.y(u) > GA.y(v);

		if (x || y) G.reverseEdge(e);
	}

	const int MAX_ERR = (int)(G.numberOfEdges() * (1/(1-p)));
	List<edge> backedges;
	int it_dag = 0;
	int err_dl = 0;
	const double th = G.numberOfEdges()*p;
	while(it_dag < th && err_dl < MAX_ERR) {
		edge e = G.chooseEdge();
		G.reverseEdge(e);
		if (isAcyclic(G, backedges)) {
			it_dag++;
		} else {
			err_dl++;
			G.reverseEdge(e);
		}
	}
}


void randomPlanarCNBGraph(Graph &G, int n, int m, int b)
{
	OGDF_ASSERT(b > 1);
	OGDF_ASSERT(n > 1);
	OGDF_ASSERT((n == 2 && m == 1) || m >= n);
	m = std::min(m, 3*n-6);

	G.clear();
	G.newNode();

	for (int i = 1; i <= b; i++){
		// Set new cut vertex and number of nodes for the current bicomp.
		node cutv = G.chooseNode();
		int actN = randomNumber(2, n);

		if (actN <= 2){
			G.newEdge(G.newNode(), cutv);
		} else {
			// Set number of edges for the current bicomp.
			int actM = randomNumber(actN, std::min(m, 3*actN-6));

			// Create a planar biconnected graph and insert it into G as a
			// biconnected component.
			Graph H;
			randomPlanarBiconnectedGraph(H, actN, actM, false);
			NodeArray<node> nodeMap(H);
			G.insert(H, nodeMap);

			node cutv2 = nodeMap[H.chooseNode()];
			edge newE = G.newEdge(cutv2, cutv);
			G.contract(newE);
		}
	}
}


static void constructCConnectedCluster(node v, ClusterGraph &C, minstd_rand &rng);
static void constructCluster(node v, ClusterGraph &C);
static void bfs(node v, SList<node> &newCluster, NodeArray<bool> &visited, ClusterGraph &C, minstd_rand &rng);

void randomClusterGraph(ClusterGraph &C,Graph &G,int cNum)
{
	int n = G.numberOfNodes();

	int count = 0;
	NodeArray<int> num(G);
	Array<node> numNode(0,n-1,nullptr);
	for(node v : G.nodes)
	{
		num[v] =  count;
		numNode[count] = v;
		count++;
	}

	minstd_rand rng(randomSeed());
	uniform_int_distribution<> dist(0,n-1);

	for (int i = 0; i < cNum; i++)
		constructCluster( numNode[dist(rng)], C );

#ifdef OGDF_DEBUG
	C.consistencyCheck();
#endif

}


void randomClusterPlanarGraph(ClusterGraph &C,Graph &G,int cNum)
{
	int n = G.numberOfNodes();

	int count = 0;
	NodeArray<int> num(G);
	Array<node> numNode(0,n-1,nullptr);
	for(node v : G.nodes)
	{
		num[v] =  count;
		numNode[count] = v;
		count++;
	}

	minstd_rand rng(randomSeed());
	uniform_int_distribution<> dist(0,n-1);

	for (int i = 0; i < cNum; i++)
		constructCConnectedCluster( numNode[dist(rng)], C, rng );

	// By construction, clusters might have just one child.
	// remove these clusters
	SListPure<cluster> store;
	for(cluster c : C.clusters)
	{
		if ((c->cCount() + c->nCount()) == 1 )
			store.pushBack(c);
	}
	while (!store.empty())
	{
		cluster c = store.popFrontRet();
		if (c != C.rootCluster())
			C.delCluster(c);
	}
	if ((C.rootCluster()->cCount() == 1) && (C.rootCluster()->nCount() == 0))
	{
		cluster cl = *C.rootCluster()->cBegin();
		C.delCluster(cl);
	}

#ifdef OGDF_DEBUG
	C.consistencyCheck();
#endif
}


static void constructCConnectedCluster(node v, ClusterGraph &C, minstd_rand &rng)
{
	SList<node> newCluster;
	newCluster.pushBack(v);
	NodeArray<bool> visited(C,false);
	visited[v] = true;
	bfs(v, newCluster, visited, C, rng);
	if (newCluster.size() > 1)
	{
		cluster cl = C.newCluster(C.clusterOf(v));
		while (!newCluster.empty())
		{
			node w = newCluster.popFrontRet();
			C.reassignNode(w,cl);
		}
	}
}


// Construct new (child) cluster by randomly choosing nodes in v's cluster
static void constructCluster(node v,ClusterGraph &C)
{
	if (C.clusterOf(v)->nCount() < 2) return;

	SList<node> newCluster;
	newCluster.pushBack(v);

	minstd_rand rng(randomSeed());
	uniform_int_distribution<> dist(0,99);

	// store the cluster nodes for random selection
	// we could just randomly select by running up the list
	for (node u : C.clusterOf(v)->nodes) {
		if (u != v && dist(rng) > 65) {
			newCluster.pushBack(u);
		}
	}

	cluster cl = C.newCluster(C.clusterOf(v));
	while (!newCluster.empty())
	{
		node w = newCluster.popFrontRet();
		C.reassignNode(w,cl);
	}

}


// Insert nodes in v's cluster to new cluster with a certain probability
static void bfs(node v, SList<node> &newCluster, NodeArray<bool> &visited, ClusterGraph &C, minstd_rand &rng)
{
	uniform_int_distribution<> dist(0,99);

	SListPure<node> bfsL;
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		node w = e->opposite(v);
		int probability = dist(rng);
		if (probability < 70 && !visited[w])
		{
			visited[w] = true;
			if (C.clusterOf(v) == C.clusterOf(w))
			{
				newCluster.pushBack(w);
				bfsL.pushBack(w);
			}
		}
		else
			visited[w] = true;
	}
	while(!bfsL.empty())
		bfs(bfsL.popFrontRet(), newCluster, visited, C, rng);
}


void randomTree(Graph& G, int n)
{
	G.clear();
	if (n > 0) {
		minstd_rand rng(randomSeed());
		Array<node> nodes(n);
		nodes[0] = G.newNode();
		for(int i=1; i<n; i++) {
			uniform_int_distribution<> dist(0, i-1);
			node on = nodes[dist(rng)];
			nodes[i] = G.newNode();
			G.newEdge(on, nodes[i]);
		}
	}
}


void createClustersHelper(ClusterGraph& C, const node curr, const node pred, const cluster predC, List<cluster>& internal, List<cluster>& leaves)
{
	cluster currC = predC ? C.createEmptyCluster(predC) : C.rootCluster();
	if(curr->degree()==1 && pred!=nullptr) {
		leaves.pushBack(currC);
	} else {
		for(adjEntry adj : curr->adjEntries) {
			node next = adj->twinNode();
			if(next == pred) continue;
			createClustersHelper(C,  next,curr,currC,  internal,leaves);
		}
		internal.pushBack(currC);
	}
}


void randomClusterGraph(ClusterGraph& C, const Graph& G, const node root, int moreInLeaves)
{
	C.init(G);

	// Build cluster structure (and store which clusters are internal and which are leaves)
	List<cluster> internal;
	List<cluster> leaves;
	createClustersHelper(C, root, nullptr, nullptr, internal, leaves);

	// Assign nodes to clusters
	List<node> nodes;
	G.allNodes<List<node> >(nodes);

	// Step 1: Ensure two node per leaf-cluster
	nodes.permute();
	for(cluster c : leaves) {
		C.reassignNode(nodes.popFrontRet(),c);
		C.reassignNode(nodes.popFrontRet(),c);
	}

	// Step 2: Distribute the other nodes
	int n = G.numberOfNodes();
	int numI = internal.size();
	int numL = leaves.size();
	double chanceForInternal = ( numI*n/double(numL*moreInLeaves+numI) ) / double(n-2*numL);
	// a leaf-cluster should have (on average) moreInLeaves-times as many vertices as in internal-cluster.
	// #verticesInInternalCluster = n / (numL*moreInLeaves + numI)
	// #nodesToDistribute = n - 2*numL
	// => chance that a node goes into an internal cluster = numI * #verticesInInternalCluster / (n-2*numL)

	minstd_rand rng(randomSeed());
	uniform_real_distribution<> dist_0_1(0.0,1.0);

	while(!nodes.empty()) {
		cluster cl;
		if(dist_0_1(rng) < chanceForInternal) {
			cl = *internal.get(uniform_int_distribution<>(0,internal.size()-1)(rng));
		} else {
			cl = *leaves.get(uniform_int_distribution<>(0,leaves.size()-1)(rng));
		}
		C.reassignNode(nodes.popFrontRet(),cl);
	}
}


void randomDigraph(Graph &G, int n, double p)
{
	OGDF_ASSERT(n >= 0);
	OGDF_ASSERT(p <= 1);
	OGDF_ASSERT(p >= 0);

	// permute() doesn't work if n==0
	if (n == 0) return;

	emptyGraph(G,n);

	minstd_rand rng(randomSeed());
	uniform_real_distribution<> dist(0.0,1.0);

	List<node> nodeList;
	G.allNodes(nodeList);
	nodeList.permute();

	for(node v : nodeList) {
		for(node w : G.nodes) {
			if (v==w)
				continue;
			if (dist(rng) < p)
				G.newEdge(v,w);
		}
	}

	//remove anti parallel edges
	makeSimple(G);
}


void randomSeriesParallelDAG(Graph &G, int edges, double p, double flt)
{
	OGDF_ASSERT(edges >= 0);
	OGDF_ASSERT(p <= 1);
	OGDF_ASSERT(p >= 0);
	OGDF_ASSERT(flt < 1);
	OGDF_ASSERT(flt >= 0);

	G.clear();

	NodeArray<node> sT(G);
	List<node> stList;
	for(int i = 0; i < edges; i++) {
		node s = G.newNode();
		node t = G.newNode();
		sT[s] = t;
		stList.pushBack(s);
		G.newEdge(s,t);
	}

	minstd_rand rng(randomSeed());
	uniform_real_distribution<> dist(0.0,1.0);

	while(stList.size() > 1) {
		ListIterator<node> it_1 = stList.chooseIterator();
		node s_1 = *it_1;
		ListIterator<node> it_2 = stList.chooseIterator();
		node s_2 = *it_2;
		while(s_1 == s_2) {
			it_2 = stList.chooseIterator();
			s_2 = *it_2;
		}
		bool serial = dist(rng) < p;
		if (!serial) {
			bool fnd_1 = false, fnd_2 = false;
			for(adjEntry adj : s_1->adjEntries) {
				if (adj->twinNode() == sT[s_1]) fnd_1 = true;
			}
			for(adjEntry adj : s_2->adjEntries) {
				if (adj->twinNode() == sT[s_2]) fnd_2 = true;
			}
			if (fnd_1 && fnd_2) serial = true;
		}
		if (stList.size() == 2) serial = false;
		if (serial) {
			edge e = G.newEdge(sT[s_1], s_2);
			sT[s_1] = sT[s_2];
			G.contract(e);
			stList.del(it_2);
		} else {
			edge e = G.newEdge(s_1, s_2);
			edge f = G.newEdge(sT[s_1], sT[s_2]);
			node s_new = G.contract(e);
			node t_new = G.contract(f);
			sT[s_new] = t_new;
			stList.del(it_1);
			stList.del(it_2);
			stList.pushBack(s_new);
		}
	}
	makeSimple(G);

	node s_pol = stList.popFrontRet();
	node t_pol = sT[s_pol];

	const int MAX_ERR = (int)(G.numberOfEdges() * (1/(1-flt)));
	List<edge> backedges;
	int it_dag = 0;
	int err_dl = 0;
	const double th = G.numberOfEdges()*flt;
	while(it_dag < th && err_dl < MAX_ERR) {
		edge e = G.chooseEdge();
		G.reverseEdge(e);
		if (isAcyclic(G, backedges)) {
			it_dag++;
		} else {
			err_dl++;
			e = G.chooseEdge([&](edge f) { return f->target() != t_pol && f->source() != s_pol; });
			G.reverseEdge(e);
		}
	}
}

void randomGeometricCubeGraph(Graph &G, int nodes, double threshold, int dimension)
{
	OGDF_ASSERT( dimension >= 1 );

	G.clear();

	// create nodes with random d-dim coordinate
	emptyGraph(G, nodes);
	NodeArray<Array<double>> cord(G, Array<double>(dimension));
	std::minstd_rand rng(randomSeed());
	uniform_real_distribution<> dist(0, 1);
	for (node v : G.nodes) {
		for (int i = 0; i < dimension; i++){
			cord[v][i] = dist(rng);
		}
	}

	// connect nodes if distance is smaller than threshold
	threshold *= threshold; //no need for sqrt() when we later compare
	                        //the distance with the threshold
	for (node v : G.nodes) {
		for (node w = v->succ(); w; w = w->succ()) {
			double distance = 0.0;
			for (int i = 0; i < dimension; i++) {
				distance += (cord[v][i] - cord[w][i])*(cord[v][i] - cord[w][i]);
			}
			if (distance < threshold) {
				G.newEdge(v, w);
			}
		}
	}
}

void randomWaxmanGraph(Graph &G, int nodes, double alpha, double beta, double width, double height)
{
	OGDF_ASSERT(0 < alpha);
	OGDF_ASSERT(1 >= alpha);
	OGDF_ASSERT(0 < beta);
	OGDF_ASSERT(1 >= beta);

	emptyGraph(G, nodes);

	// Model RG1: Randomly distribute nodes, then work off their distances.
	NodeArray<IPoint> cord(G);
	std::minstd_rand rng(randomSeed());
	uniform_int_distribution<> distX(0, width);
	uniform_int_distribution<> distY(0, height);
	for (node v : G.nodes) {
		cord[v] = IPoint(distX(rng), distY(rng));
	}

	double maxDistance = 0.0;
	for (node v : G.nodes) {
		for (node w = v->succ(); w; w = w->succ()) {
			Math::updateMax(maxDistance, cord[v].distance(cord[w]));
		}
	}

	randomEdgesGraph(G, [&](node v, node w) { return beta * exp(-cord[v].distance(cord[w])/(maxDistance * alpha)); });
}

void preferentialAttachmentGraph(Graph &G, int numberNodes, int minDegree) {
	OGDF_ASSERT(1 <= minDegree);

	if (numberNodes == 0) {
		return;
	}

	// Special case: We get an empty graph. Start with a complete graph
	// where each node has minDegree.
	if (G.empty()) {
		completeGraph(G, minDegree + 1);
		numberNodes -= (minDegree + 1);
	}
	else {
#ifdef OGDF_DEBUG
		OGDF_ASSERT(minDegree <= G.numberOfNodes());

		// We need to make sure to have at least minDegree nodes
		// with at least one edge each, so the algorithm does not
		// get stuck in an infinite loop trying to find another
		// node with degree larger than 0.
		int nNodesWithEdge = 0;
		for (node n : G.nodes) {
			if (n->degree() > 0) nNodesWithEdge++;
			if (nNodesWithEdge >= minDegree) break;
		}
		OGDF_ASSERT(nNodesWithEdge >= minDegree);
#endif
	}

	minstd_rand rng(randomSeed());
	List<node> potentialNeighbors;

	for (int i = 0; i < numberNodes; i++) {
		node w = G.newNode();
		// At this point, we have no connections and all nodes are potential neighbors.
		G.allNodes(potentialNeighbors);
		// We will select a uniform, random number between zero and the count of all
		// degrees, and then iterate through the nodes, summing up their degrees until
		// we find our target to connect.
		int sumDegrees = 2 * G.numberOfEdges();

		while (w->degree() < minDegree) {
			OGDF_ASSERT(1 <= sumDegrees);
			uniform_int_distribution<> dist(1, sumDegrees);
			int selected = dist(rng);
			int sumLocal = 0;
			for (auto it = potentialNeighbors.begin(); it != potentialNeighbors.end(); it++) {
				sumLocal += (*it)->degree();
				if (sumLocal >= selected) {
					// For the next iteration, we do not want to try appending to this
					// node again, so remove it from the calculation.
					sumDegrees -= (*it)->degree();
					G.newEdge(*it, w);
					potentialNeighbors.del(it);
					break;
				}
			}
		}
	}
}


void randomWattsStrogatzGraph(Graph &G, int n, int k, double probability)
{
	OGDF_ASSERT(0.0 <= probability);
	OGDF_ASSERT(probability <= 1.0);

	regularLatticeGraph(G, n, k);
	Array<node> nodes;
	G.allNodes(nodes);
	NodeArray<int> indices = NodeArray<int>(G);
	// Assume the nodes to be generated in order
	for (int i = 0; i < nodes.size(); i++) {
		indices[nodes[i]] = i;
	}

	minstd_rand rng(randomSeed());
	uniform_real_distribution<> dist(0, 1);
	uniform_int_distribution<> rand(0, n-1);

	// Construct lists of edges.
	// Each list i contains the edges with (i+1) distance, i.e. {1, 2, 3,...}.
	Array<List<edge>> edges;
	edges.init(k/2);
	for (node v : nodes) {
		List<edge> vEdges;
		v->adjEdges(vEdges);

		for (edge e : vEdges) {
			int delta = indices[e->opposite(v)] - indices[v];

			// The other node is further clockwise without hitting the edge,
			// but its index is still in the interval
			if (delta > 0 && delta <= k/2) {
				edges[delta-1].pushBack(e);
			}
			// The other node has a smaller index, but within the interval when
			// looking at the overlap
			else if (delta < 0 && (n + delta) <= k/2) {
				edges[n+delta-1].pushBack(e);
			}
		}
	}

	for (int i = 0; i < edges.size(); i++) {
		for (edge e : edges[i]) {
			if (dist(rng) <= probability) {
				// We have to rewire this edge.
				// Take the node further anti-clockwise and anchor the edge there.
				node v = e->target();
				bool moveTarget = false;
				// We know how far the nodes are apart (i+1), so adding this to the
				// index and accounting for the wrap-around should have us hit the
				// next node.
				if ((indices[v] + i + 1) % n != indices[e->source()]) {
					v = e->source();
					moveTarget = true;
				}

				// If we happen to have all other nodes connected to this node, we cannot do anything here.
				if (v->degree() == G.numberOfNodes() - 1) continue;

				// NOTE The following algorithm to choose nodes works well if n is
				// much bigger than k. For graphs where k is close to n/2, this can
				// take a long time as we are just randomly poking into our array.
				int newNeighbor;
				do {
					newNeighbor = rand(rng);
				} while (nodes[newNeighbor] == v || G.searchEdge(v, nodes[newNeighbor]) != nullptr);
				if (moveTarget) {
					G.moveTarget(e, nodes[newNeighbor]);
				}
				else {
					G.moveSource(e, nodes[newNeighbor]);
				}
			}
		}
	}
}

void randomChungLuGraph(Graph &G, Array<int> expectedDegreeSequence) {
	int numberNodes = expectedDegreeSequence.size();
	OGDF_ASSERT(numberNodes != 0);

	emptyGraph(G, numberNodes);
	NodeArray<int> expectedDegrees(G);
	int i = 0;
	int sumDegrees = 0;
	// We have exactly the same number of nodes as our sequence is long
	for (node v : G.nodes) {
		expectedDegrees[v] = expectedDegreeSequence[i];
		sumDegrees += expectedDegreeSequence[i];
		i++;
	}

#ifdef OGDF_DEBUG
	for (int deg : expectedDegreeSequence) {
		OGDF_ASSERT(deg > 0);
		OGDF_ASSERT(deg < numberNodes);
		OGDF_ASSERT(deg*deg < sumDegrees);
	}
#endif

	randomEdgesGraph(G, [&](node v, node w) {
		return ((double) expectedDegrees[v] * expectedDegrees[w]) / sumDegrees;
	});
}

void randomEdgesGraph(Graph &G, std::function<double(node, node)> probability)
{
	minstd_rand rng(randomSeed());
	uniform_real_distribution<> dist(0, 1);
	for (node v : G.nodes) {
		for (node w = v->succ(); w; w = w->succ()) {
			if (dist(rng) < probability(v, w)) {
				G.newEdge(v, w);
			}
		}
	}
}

}
