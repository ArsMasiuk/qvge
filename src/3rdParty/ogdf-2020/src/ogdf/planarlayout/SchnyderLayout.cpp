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

SchnyderLayout::SchnyderLayout()
	: PlanarGridLayoutModule()
	, m_combinatorialObjects(CombinatorialObjects::VerticesMinusDepth)
	{}


void SchnyderLayout::doCall(
	const Graph &G,
	adjEntry adjExternal,
	GridLayout &gridLayout,
	IPoint &boundingBox,
	bool fixEmbedding)
{
	if (G.numberOfNodes() < 3) {
		if (G.numberOfNodes() == 2) {
			gridLayout.x()[G.firstNode()] = 0;
			gridLayout.y()[G.firstNode()] = 0;
			gridLayout.x()[G.lastNode()] = 1;
			gridLayout.y()[G.lastNode()] = 0;
		}
		return;
	}

	// check for double edges & self loops
	OGDF_ASSERT(isSimple(G));

	// make a copy for triangulation
	GraphCopy GC(G);

	// embed
	bool isPlanar = planarEmbed(GC);
	OGDF_ASSERT(fixEmbedding || isPlanar);

	triangulate(GC);

	schnyderEmbedding(GC, gridLayout, adjExternal);

#ifdef OGDF_DEBUG
	// Test for correct grid sizes.
	int n = G.numberOfNodes();
	int xmin = 0;
	int xmax = 0;
	int ymin = 0;
	int ymax = 0;
	gridLayout.computeBoundingBox(xmin, xmax, ymin, ymax);

	if (m_combinatorialObjects == CombinatorialObjects::VerticesMinusDepth) {
		OGDF_ASSERT(xmax - xmin == n - 2);
		OGDF_ASSERT(ymax - ymin == n - 2);
	} else if (m_combinatorialObjects == CombinatorialObjects::Faces) {
		OGDF_ASSERT(xmax - xmin == 2*n - 5);
		OGDF_ASSERT(ymax - ymin == 2*n - 5);
	}
#endif
}


void SchnyderLayout::schnyderEmbedding(
	GraphCopy& GC,
	GridLayout &gridLayout,
	adjEntry adjExternal)
{
	// Choose outer face a, b, c.
	adjEntry adja;
	if (adjExternal != nullptr) {
		edge eGC = GC.copy(adjExternal->theEdge());
		adja = adjExternal->isSource() ? eGC->adjSource() : eGC->adjTarget();
	} else {
		adja = GC.firstEdge()->adjSource();
	}
	adjEntry adjb = adja->faceCyclePred();
	adjEntry adjc = adjb->faceCyclePred();

	// Initialize the realizer graph (edge direction is reversed!) and the
	// realizer value for each edge.
	GraphCopy T = GraphCopy(GC);
	EdgeArray<int> rValues(T);

	// External nodes a, b and c.
	node a = adja->theNode();
	node b = adjb->theNode();
	node c = adjc->theNode();
	node a_in_T = T.copy(GC.original(a));
	node b_in_T = T.copy(GC.original(b));
	node c_in_T = T.copy(GC.original(c));

	// Get the realizer, i.e. an edge labeling with values in {1,2,3}, by
	// contracting nodes.
	List<node> L;
	contract(GC, a, b, c, L);
	realizer(GC, L, a, b, c, rValues, T);

	// The following code, including variable names, is in line with Schnyder
	// [Sch90], section 8, last paragraph!

	// Get sizes of all subtrees of T1 and T2.
	NodeArray<int> t1(T);
	NodeArray<int> t2(T);
	subtreeSizes(rValues, 1, a_in_T, t1);
	subtreeSizes(rValues, 2, b_in_T, t2);

	// Get depth for all nodes in trees T1, T2 and T3 (with depth of root = 1).
	NodeArray<int> p1(T);
	NodeArray<int> p2(T);
	NodeArray<int> p3(T);
	NodeArray<int> val(T, 1);
	prefixSum(rValues, 1, a_in_T, val, p1);
	prefixSum(rValues, 2, b_in_T, val, p2);
	prefixSum(rValues, 3, c_in_T, val, p3);

	// Initialize prefix-sums of subtree-sizes.
	NodeArray<int> sum1(T);
	NodeArray<int> sum2(T);

	// Calculate x-coordinates.
	prefixSum(rValues, 2, b_in_T, t1, sum1);
	sum1[a_in_T] = t1[a_in_T];

	prefixSum(rValues, 3, c_in_T, t1, sum2);
	sum2[a_in_T] = t1[a_in_T];

	for (node v : T.nodes) {
		if (!T.isDummy(v)) {
			// r1[v] = sum1[v] + sum2[v] - t1[v] is the number of nodes in region 1 of v.
			sum1[v] += sum2[v] - t1[v];
			if (m_combinatorialObjects == CombinatorialObjects::VerticesMinusDepth) {
				gridLayout.x()[T.original(v)] = sum1[v] - p3[v];
			} else if (m_combinatorialObjects == CombinatorialObjects::Faces) {
				gridLayout.x()[T.original(v)] = 2*sum1[v] - p2[v] - p3[v] - 3;
			}
		}
	}

	// Calculate y-coordinates.
	prefixSum(rValues, 3, c_in_T, t2, sum1);
	sum1[b_in_T] = t2[b_in_T];

	prefixSum(rValues, 1, a_in_T, t2, sum2);
	sum2[b_in_T] = t2[b_in_T];

	for (node v : T.nodes) {
		if (!T.isDummy(v)) {
			// r2[v] = sum1[v] + sum2[v] - t2[v] is the number of nodes in region 2 of v.
			sum1[v] += sum2[v] - t2[v];
			if (m_combinatorialObjects == CombinatorialObjects::VerticesMinusDepth) {
				gridLayout.y()[T.original(v)] = sum1[v] - p1[v];
			} else if (m_combinatorialObjects == CombinatorialObjects::Faces) {
				gridLayout.y()[T.original(v)] = 2*sum1[v] - p1[v] - p3[v] - 3;
			}
		}
	}
}


/*
 * Constructs List L
 * L is the ordering for uncontracting the nodes in realizer
 */
void SchnyderLayout::contract(Graph& G, node a, node b, node c, List<node>& L)
{
	List<node> candidates;
	NodeArray<bool> marked(G, false); // considered nodes
	NodeArray<int> deg(G, 0); // # virtual neighbours

	int N = G.numberOfEdges();

	marked[a] = marked[b] = marked[c] = true; // init outer face

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
				deg[v]--; // u is virtualy deleted
				if (!marked[v]) { // v is new neighbour of a
					marked[v] = true;
					for(adjEntry adj2 : v->adjEntries) {
						deg[adj2->twinNode()]++; // degree of virtaul neighbours increase
					}
					if (deg[v] <= 2) candidates.pushBack(v); // next candidate v
				}
				else
					if (deg[v] == 2) candidates.pushBack(v); // next candidate v
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
		ord[v] = i++; // enumerate V(G)
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
