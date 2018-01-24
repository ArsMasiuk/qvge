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


#include <ogdf/basic/graph_generators.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/Array2D.h>

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
 * Auxiliary function for ogdf::randomSimpleGraph and ogdf::randomSimpleConnectedGraph
 *
 * @param G the graph to be generated
 * @param n the number of nodes
 * @param m the number of edges
 * @param mask
 *   An array mask containing booleans for each edge (given by its unique computed index).
 *   If an edge's value is true, it will (not) be contained in \p G.
 *   After the generation, all (not) contained edges will be set to true.
 *   The "not" applies iff \p negate is true.
 * @param masked The number of masked edges. (Must equal the number of true values in \p mask.)
 * @param negate True iff the meaning of the mask should be negated (see \p mask).
 */
static bool randomSimpleGraphByMask(Graph& G, int n, int m, Array<bool>& mask, int masked = 0, bool negate = false)
{
	OGDF_ASSERT(mask.low() == 0);

	G.clear();

	if (n == 0 && m == 0)
		return true;

	if (n < 1)
		return false;

	const int max = mask.size();
	OGDF_ASSERT(max == getMaxNumberEdges(n));
	OGDF_ASSERT(max == mask.high() + 1);

	if (m > max)
		return false;

	Array<node> v(n);

	int i;
	for (i = 0; i < n; i++)
		v[i] = G.newNode();

	if (m == 0)
		return true;

	minstd_rand rng(randomSeed());
	uniform_int_distribution<> dist_a(0, n-1);
	uniform_int_distribution<> dist_b(0, n-2);

	if (negate) {
		m = max - m;
	}

	m -= masked;

	while (m > 0) {
		int a = dist_a(rng);
		int b = dist_b(rng);
		if (b >= a) {
			b++;
		} else {
			int c = a;
			a = b;
			b = c;
		}
		if (mask[i = getEdgeIndex(a, b, n, max)] == false) {
			mask[i] = true;
			m--;
		}
	}

	for (int a = 0; a < n; a++) {
		for (int b = a + 1; b < n; b++) {
			if (mask[getEdgeIndex(a, b, n, max)] == !negate) {
				G.newEdge(v[a], v[b]);
			}
		}
	}

	return true;
}

bool randomSimpleGraph(Graph &G, int n, int m)
{
	// save some running time by exploiting addition/removal symmetry
	bool negate = false;
	const int max = getMaxNumberEdges(n);
	if (m > max / 2) {
		negate = true;
	}

	Array<bool> mask(0, max-1, false);
	return randomSimpleGraphByMask(G, n, m, mask, 0, negate);
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
	Array<bool> used(0, max - 1, false);
	for (edge e : tree.edges) {
		used[getEdgeIndex(e->source()->index(), e->target()->index(), n, max)] = true;
	}

	return randomSimpleGraphByMask(G, n, m, used, tree.numberOfEdges());
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


void planarTriconnectedGraph(Graph &G, int n, double p1, double p2)
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


void planarTriconnectedGraph(Graph &G, int n, int m)
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


void planarConnectedGraph(Graph &G, int n, int m)
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


void planarBiconnectedGraph(Graph &G, int n, int m, bool multiEdges)
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

void upwardPlanarBiconnectedDiGraph(Graph &G, int n, int m)
{
	planarBiconnectedDiGraph(G,n,m);
}

void planarBiconnectedDiGraph(Graph &G, int n, int m, double p, bool multiEdges)
{
	OGDF_ASSERT(p >= 0);
	OGDF_ASSERT(p < 1.0);

	GraphAttributes GA(G, GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics);

	planarBiconnectedGraph(G,n,m,multiEdges);

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


void planarCNBGraph(Graph &G, int n, int m, int b)
{
	G.clear();
	if (b <= 0) b = 1;
	if (n <= 0) n = 1;
	if ((m <= 0) || (m > 3*n-6)) m = std::max(0, 3*n-6);

	OGDF_ASSERT(n >= 0);
	OGDF_ASSERT(m >= 0);

	G.newNode();

	minstd_rand rng(randomSeed());
	uniform_int_distribution<> dist_1_n(1, n);
	uniform_int_distribution<> dist_1_m(n > 2 ? 1 : 0, m);
	uniform_int_distribution<> dist_1_2(1, 2);

	for (int nB=1; nB<=b; nB++){
		node cutv = G.chooseNode();
		// set number of nodes for the current created block
		int actN = dist_1_n(rng);

		node v1 = G.newNode();

		if (actN <= 1){
			G.newEdge(v1, cutv);
		}
		else
			if (actN == 2) {
				node v2 = G.newNode();
				G.newEdge(v1, v2);

				int rnd = dist_1_2(rng);
				edge newE;
				int rnd2 = dist_1_2(rng);
				if (rnd == 1){
					newE = G.newEdge(v1, cutv);
				}
				else{
					newE = G.newEdge(v2, cutv);
				}
				if (rnd2 == 1){
					G.contract(newE);
				}

			} else {
				// set number of edges for the current created block
				int actM;
				if (m > 3*actN-6)
					actM = uniform_int_distribution<>(1, 3*actN-6)(rng);
				else
					actM = dist_1_m(rng);
				if (actM < actN)
					actM = actN;

				int ke = actN-3, kf = actM-actN;

				Array<node> nodes(actN);
				Array<edge> edges(actM);
				Array<face> bigFaces(actM);

				// we start with a triangle
				node v2 = G.newNode(), v3 = G.newNode();
				nodes[0] = v1;
				nodes[1] = v2;
				nodes[2] = v3;
				edges[0] = G.newEdge(v1,v2);
				edges[1] = G.newEdge(v2,v3);
				edges[2] = G.newEdge(v3,v1);

				int actInsertedNodes = 3;

				CombinatorialEmbedding E(G);
				FaceArray<int> posBigFaces(E);
				int nBigFaces = 0, nEdges = 3;

				while(ke+kf > 0) {
					int p = uniform_int_distribution<>(1,ke+kf)(rng);

					if (nBigFaces == 0 || p <= ke) {
						int eNr = uniform_int_distribution<>(0,nEdges-1)(rng);
						edge e  = edges[eNr];
						face f  = E.rightFace(e->adjSource());
						face fr = E.rightFace(e->adjTarget());

						node u = e->source();
						node v = e->target();

						edges[nEdges++] = E.split(e);

						if (e->source() != v && e->source() != u)
							nodes[actInsertedNodes++] = e->source();
						else
							nodes[actInsertedNodes++] = e->target();

						if (f->size() == 4) {
							posBigFaces[f] = nBigFaces;
							bigFaces[nBigFaces++] = f;
						}
						if (fr->size() == 4) {
							posBigFaces[fr] = nBigFaces;
							bigFaces[nBigFaces++] = fr;
						}

						ke--;
					}
					else {
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

				// delete multi edges
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

				node cutv2 = nodes[uniform_int_distribution<>(0,actN-1)(rng)];

				int rnd = dist_1_2(rng);
				edge newE = G.newEdge(cutv2, cutv);
				if (rnd == 1) {
					G.contract(newE);
				}
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


void randomDiGraph(Graph &G, int n, double p)
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
	std::random_device rd;
	std::mt19937 gen(rd());
	uniform_real_distribution<> dist(0, 1);
	for (node v : G.nodes) {
		for (int i = 0; i < dimension; i++){
			cord[v][i] = dist(gen);
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

void emptyGraph(Graph &G, int nodes)
{
	G.clear();
	for (int i = 0; i < nodes; i++) {
		G.newNode();
	}
}

}
