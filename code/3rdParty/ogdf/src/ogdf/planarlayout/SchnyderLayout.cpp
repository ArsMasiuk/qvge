/** \file
 * \brief Definition of the Schnyder Layout Algorithm (SchnyderLayout)
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

#include <ogdf/planarlayout/SchnyderLayout.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/simple_graph_alg.h>

namespace ogdf {

SchnyderLayout::SchnyderLayout() : PlanarGridLayoutModule() {
}


void SchnyderLayout::doCall(
	const Graph &G,
	adjEntry adjExternal,
	GridLayout &gridLayout,
	IPoint &boundingBox,
	bool fixEmbedding)
{
	// check for double edges & self loops
	OGDF_ASSERT(isSimple(G));

	// make a copy for triangulation
	GraphCopy GC(G);

	// embed
	if (!fixEmbedding && !planarEmbed(GC)) {
		OGDF_THROW_PARAM(PreconditionViolatedException, PreconditionViolatedCode::Planar);
	}

	triangulate(GC);

	schnyderEmbedding(GC, gridLayout, adjExternal);
}


void SchnyderLayout::schnyderEmbedding(
	GraphCopy& GC,
	GridLayout &gridLayout,
	adjEntry adjExternal)
{
	NodeArray<int> &xcoord = gridLayout.x();
	NodeArray<int> &ycoord = gridLayout.y();

	List<node> L;						// (un)contraction order
	GraphCopy T = GraphCopy(GC);		// the realizer tree (reverse direction of edges!!!)
	EdgeArray<int> rValues(T);			// the realizer values

	// choose outer face a,b,c
	adjEntry adja;
	if (adjExternal != nullptr) {
		edge eG  = adjExternal->theEdge();
		edge eGC = GC.copy(eG);
		adja = (adjExternal == eG->adjSource()) ? eGC->adjSource() : eGC->adjTarget();
	}
	else {
		adja = GC.firstEdge()->adjSource();
	}
	adjEntry adjb = adja->faceCyclePred();
	adjEntry adjc = adjb->faceCyclePred();

	node a = adja->theNode();
	node b = adjb->theNode();
	node c = adjc->theNode();

	node a_in_T = T.copy(GC.original(a));
	node b_in_T = T.copy(GC.original(b));
	node c_in_T = T.copy(GC.original(c));

	contract(GC, a, b, c, L);

	realizer(GC, L, a, b, c, rValues, T);

	NodeArray<int>  t1(T);
	NodeArray<int>  t2(T);
	NodeArray<int>  val(T, 1);

	NodeArray<int>  P1(T);
	NodeArray<int>  P3(T);
	NodeArray<int>  v1(T);
	NodeArray<int>  v2(T);

	subtreeSizes(rValues, 1, a_in_T, t1);
	subtreeSizes(rValues, 2, b_in_T, t2);

	prefixSum(rValues, 1, a_in_T, val, P1);
	prefixSum(rValues, 3, c_in_T, val, P3);
	// now Pi  =  depth of all nodes in Tree T(i) (depth[root] = 1)

	prefixSum(rValues, 2, b_in_T, t1, v1);
	// special treatment for a
	v1[a_in_T] = t1[a_in_T];

	/*
	 * v1[v] now is the sum of the
	 * "count of nodes in t1" minus the "subtree size for node x"
	 * for every node x on a path from b to v in t2
	 */

	prefixSum(rValues, 3, c_in_T, t1, val);
	// special treatment for a
	val[a_in_T] = t1[a_in_T];

	/*
	 * val[v] now is the sum of the
	 * "count of nodes in t1" minus the "subtree size for node x"
	 * for every node x on a path from c to v in t3
	 */

	// r1[v]=v1[v]+val[v]-t1[v] is the number of nodes in region 1 from v
	for(node v : T.nodes) {
		// calc v1'
		v1[v] += val[v] - t1[v] - P3[v];
	}

	prefixSum(rValues, 3, c_in_T, t2, v2);
	// special treatment for b
	v2[b_in_T] = t2[b_in_T];

	prefixSum(rValues, 1, a_in_T, t2, val);
	// special treatment for b
	val[b_in_T] = t2[b_in_T];

	for(node v : T.nodes) {
		// calc v2'
		v2[v] += val[v] - t2[v] - P1[v];
	}

	// copy coordinates to the GridLayout
	for(node v : GC.nodes) {
		xcoord[GC.original(v)] = v1[T.copy(GC.original(v))];
		ycoord[GC.original(v)] = v2[T.copy(GC.original(v))];
	}
}


/*
 * Constructs List L
 * L is the ordering for uncontracting the nodes in realizer
 */
void SchnyderLayout::contract(Graph& G, node a, node b, node c, List<node>& L)
{
	List<node> candidates;
	NodeArray<bool> marked(G, false);			// considered nodes
	NodeArray<int> deg(G, 0);					// # virtual neighbours

	int N = G.numberOfEdges();

	marked[a] = marked[b] = marked[c] = true;	// init outer face

	deg[a] = deg[b] = deg[c] = N;

	// mark neighbours of a and calc the degree of the second (virtual) neighbours
	for(adjEntry adj1 : a->adjEntries) {
		marked[adj1->twinNode()] = true;
		for(adjEntry adj2 : adj1->twinNode()->adjEntries) {
			deg[adj2->twinNode()]++;
		}
	}

	// find first candidates
	for(adjEntry adj1 : a->adjEntries) {
		if (deg[adj1->twinNode()] <= 2) {
			candidates.pushBack(adj1->twinNode());
		}
	}

	while (!candidates.empty()) {
		node u = candidates.popFrontRet();
		if (deg[u] == 2) {
			L.pushFront(u);
			deg[u] = N;
			for(adjEntry adj1 : u->adjEntries) {
				node v = adj1->twinNode();
				deg[v]--;										// u is virtualy deleted
				if (!marked[v]) {								// v is new neighbour of a
					marked[v] = true;
					for(adjEntry adj2 : v->adjEntries) {
						deg[adj2->twinNode()]++;				// degree of virtaul neighbours increase
					}
					if (deg[v] <= 2) candidates.pushBack(v);	// next candidate v
				}
				else
					if (deg[v] == 2) candidates.pushBack(v);	// next candidate v
			}
		}
	}
}


/*
 * Construct the realiszer and the Tree T
 * rValues = realizer value
 * T = Tree
 */
void SchnyderLayout::realizer(
	GraphCopy& G,
	const List<node>& L,
	node a,
	node b,
	node c,
	EdgeArray<int>& rValues,
	GraphCopy& T)
{
	int  i = 0;
	edge e;
	NodeArray<int> ord(G, 0);

	// ordering: b,c,L,a
	ord[b] = i++;
	ord[c] = i++;

	for(node v : L) {
		ord[v] = i++;				// enumerate V(G)
	}
	ord[a] = i++;

	// remove all edges (they will be re-added later with different orientation)
	while (T.numberOfEdges() > 0) {
		e = T.firstEdge();
		T.delEdge(e);
	}

	for(node v : L) {
		node u = T.copy(G.original(v));   // u is copy of v in T

		adjEntry adj = nullptr;
		for(adjEntry adjRun : v->adjEntries) {
			if (ord[adjRun->twinNode()] > ord[v]) {
				adj = adjRun;
				break;
			}
		}

		adjEntry adj1 = adj;
		while (ord[adj1->twinNode()] > ord[v]) {
			adj1 = adj1->cyclicSucc();
		}
		e = T.newEdge(T.copy(G.original(adj1->twinNode())), u);
		rValues[e] = 2;

		adjEntry adj2 = adj;
		while (ord[adj2->twinNode()] > ord[v]) {
			adj2 = adj2->cyclicPred();
		}
		e = T.newEdge(T.copy(G.original(adj2->twinNode())), u);
		rValues[e] = 3;

		for (adj = adj1->cyclicSucc(); adj != adj2; adj = adj->cyclicSucc()) {
			e =  T.newEdge(u, T.copy(G.original(adj->twinNode())));
			rValues[e] = 1;
		}
	}

	// special treatement of a,b,c
	node a_in_T = T.copy(G.original(a));
	node b_in_T = T.copy(G.original(b));
	node c_in_T = T.copy(G.original(c));

	// all edges to node a get realizer value 1
	for(adjEntry adj : a->adjEntries) {
		e = T.newEdge(a_in_T, T.copy(G.original(adj->twinNode())));
		rValues[e] = 1;
	}

	// rest of outer triangle (reciprocal linked, realizer values 2 and 3)
	e = T.newEdge(b_in_T, a_in_T);
	rValues[e] = 2;
	e = T.newEdge(b_in_T, c_in_T);
	rValues[e] = 2;

	e = T.newEdge(c_in_T, a_in_T);
	rValues[e] = 3;
	e = T.newEdge(c_in_T, b_in_T);
	rValues[e] = 3;
}


/*
 * computes the sizes of all subtrees of a tree with root r
 */
void SchnyderLayout::subtreeSizes(
	EdgeArray<int>& rValues,
	int i,
	node r,
	NodeArray<int>& size)
{
	int sum = 0;
	for(adjEntry adj : r->adjEntries) {
		if (adj->theEdge()->source() == r && rValues[adj->theEdge()] == i) {
			node w = adj->twinNode();
			subtreeSizes(rValues, i, w, size);
			sum += size[w];
		}
	}
	size[r] = sum + 1;
}

/*
 * computes for every node u in the subtree of T(i) with root r
 * the sum of all val[v] where v is a node on the path from r to u
 */
void SchnyderLayout::prefixSum(
	EdgeArray<int>& rValues,
	int i,
	node r,
	const NodeArray<int>& val,
	NodeArray<int>& sum)
{
	List<node> Q;

	Q.pushBack(r);
	sum[r] = val[r];

	while (!Q.empty()) {
		node v = Q.popFrontRet();
		for (adjEntry adj : v->adjEntries) {
			if (adj->theEdge()->source() == v && rValues[adj->theEdge()] == i) {
				node w = adj->twinNode();
				Q.pushBack(w);
				sum[w] = val[w] + sum[v];
			}
		}
	}
}

}
