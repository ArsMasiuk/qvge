/** \file
 * \brief Implements class UpwardPlanarSubgraphSimple which computes
 * an upward planar subgraph of a single-source acyclic digraph
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


#include <ogdf/upward/UpwardPlanarSubgraphSimple.h>
#include <ogdf/upward/UpwardPlanarity.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>


namespace ogdf {


void UpwardPlanarSubgraphSimple::call(const Graph &G, List<edge> &delEdges)
{
	delEdges.clear();

	// We construct an auxiliary graph H which represents the current upward
	// planar subgraph.
	Graph H;
	NodeArray<node> mapToH(G);

	for(node v : G.nodes)
		mapToH[v] = H.newNode();


	// We currently support only single-source acyclic digraphs ...
	node s;
	hasSingleSource(G,s);

	OGDF_ASSERT(s != nullptr);
	OGDF_ASSERT(isAcyclic(G));

	// We start with a spanning tree of G rooted at the single source.
	NodeArray<bool> visitedNode(G,false);
	SListPure<edge> treeEdges;
	dfsBuildSpanningTree(s,treeEdges,visitedNode);


	// Mark all edges in the spanning tree so they can be skipped in the
	// loop below and add (copies of) them to H.
	EdgeArray<bool> visitedEdge(G,false);
	SListConstIterator<edge> it;
	for(it = treeEdges.begin(); it.valid(); ++it) {
		edge eG = *it;
		visitedEdge[eG] = true;
		H.newEdge(mapToH[eG->source()],mapToH[eG->target()]);
	}


	// Add subsequently the remaining edges to H and test if the resulting
	// graph is still upward planar. If not, remove the edge again from H
	// and add it to delEdges.

	for(edge eG : G.edges)
	{
		if(visitedEdge[eG])
			continue;

		edge eH = H.newEdge(mapToH[eG->source()],mapToH[eG->target()]);

		if (!UpwardPlanarity::isUpwardPlanar_singleSource(H)) {
			H.delEdge(eH);
			delEdges.pushBack(eG);
		}
	}

}


void UpwardPlanarSubgraphSimple::dfsBuildSpanningTree(
	node v,
	SListPure<edge> &treeEdges,
	NodeArray<bool> &visited)
{
	visited[v] = true;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		node w = e->target();
		if(w == v) continue;

		if(!visited[w]) {
			treeEdges.pushBack(e);
			dfsBuildSpanningTree(w,treeEdges,visited);
		}
	}
}



void UpwardPlanarSubgraphSimple::call(GraphCopy &GC, List<edge> &delEdges)
{
	const Graph &G = GC.original();
	delEdges.clear();

	// We construct an auxiliary graph H which represents the current upward
	// planar subgraph.
	Graph H;
	NodeArray<node> mapToH(G,nullptr);
	NodeArray<node> mapToG(H,nullptr);

	for(node v : G.nodes)
		mapToG[ mapToH[v] = H.newNode() ] = v;


	// We currently support only single-source acyclic digraphs ...
	node s;
	hasSingleSource(G,s);

	OGDF_ASSERT(s != nullptr);
	OGDF_ASSERT(isAcyclic(G));

	// We start with a spanning tree of G rooted at the single source.
	NodeArray<bool> visitedNode(G,false);
	SListPure<edge> treeEdges;
	dfsBuildSpanningTree(s,treeEdges,visitedNode);


	// Mark all edges in the spanning tree so they can be skipped in the
	// loop below and add (copies of) them to H.
	EdgeArray<bool> visitedEdge(G,false);
	SListConstIterator<edge> it;
	for(it = treeEdges.begin(); it.valid(); ++it) {
		edge eG = *it;
		visitedEdge[eG] = true;
		H.newEdge(mapToH[eG->source()],mapToH[eG->target()]);
	}


	// Add subsequently the remaining edges to H and test if the resulting
	// graph is still upward planar. If not, remove the edge again from H
	// and add it to delEdges.

	SList<Tuple2<node,node> > augmented;
	GraphCopySimple graphAcyclicTest(G);

	for(edge eG : G.edges)
	{
		// already treated ?
		if(visitedEdge[eG])
			continue;

		// insert edge into H
		edge eH = H.newEdge(mapToH[eG->source()],mapToH[eG->target()]);

		node superSink;
		SList<edge> augmentedEdges;
		if (!UpwardPlanarity::upwardPlanarAugment_singleSource(H,superSink,augmentedEdges)) {
			// if H is no longer upward planar, remove eG from subgraph
			H.delEdge(eH);
			delEdges.pushBack(eG);

		} else {
			// add augmented edges as node-pair to tmpAugmented and remove
			// all augmented edges from H again
			SList<Tuple2<node,node> > tmpAugmented;
			for(edge e : augmentedEdges) {
				node v = mapToG[e->source()];
				node w = mapToG[e->target()];

				if (v && w)
					tmpAugmented.pushBack(Tuple2<node,node>(v,w));

				H.delEdge(e);
			}

			if (mapToG[superSink] == nullptr)
				H.delNode(superSink);

			// The following is a simple workaround to assure the following
			// property of the upward planar subgraph:
			//   The st-augmented upward planar subgraph plus the edges not
			//   in the subgraph must be acyclic. (This is a special property
			//   of the embedding, not the augmentation.)
			// The upward-planar embedding function gives us ANY upward-planar
			// embedding. We check if the property above holds with this
			// embedding. If it doesn't, we have actually no idea if another
			// embedding would do.
			// The better solution would be to incorporate the acyclicity
			// property into the upward-planarity test, but this is compicated.

			// test if original graph plus augmented edges is still acyclic
			if(checkAcyclic(graphAcyclicTest,tmpAugmented)) {
				augmented = tmpAugmented;

			} else {
				// if not, remove eG from subgraph
				H.delEdge(eH);
				delEdges.pushBack(eG);
			}
		}

	}

	// remove edges not in the subgraph from GC
	ListConstIterator<edge> itE;
	for(itE = delEdges.begin(); itE.valid(); ++itE)
		GC.delEdge(GC.copy(*itE));

	// add augmented edges to GC
	SListConstIterator<Tuple2<node,node> > itP;
	for(itP = augmented.begin(); itP.valid(); ++itP) {
		node v = (*itP).x1();
		node w = (*itP).x2();

		GC.newEdge(GC.copy(v),GC.copy(w));
	}

	// add super sink to GC
	node sGC = nullptr;
	SList<node> sinks;
	for(node v : GC.nodes) {
		if(v->indeg() == 0)
			sGC = v;
		if(v->outdeg() == 0)
			sinks.pushBack(v);
	}

	node superSinkGC = GC.newNode();
	SListConstIterator<node> itV;
	for(itV = sinks.begin(); itV.valid(); ++itV)
		GC.newEdge(*itV,superSinkGC);

	// add st-edge to GC, so that we now have a planar st-digraph
	GC.newEdge(sGC,superSinkGC);

	OGDF_ASSERT(isAcyclic(GC));
	OGDF_ASSERT(isPlanar(GC));
}


// test if graphAcyclicTest plus edges in tmpAugmented is acyclic
// removes added edges again
bool UpwardPlanarSubgraphSimple::checkAcyclic(
	GraphCopySimple &graphAcyclicTest,
	SList<Tuple2<node,node> > &tmpAugmented)
{
	SListPure<edge> added;

	SListConstIterator<Tuple2<node,node> > it;
	for(it = tmpAugmented.begin(); it.valid(); ++it)
		added.pushBack(graphAcyclicTest.newEdge(
			graphAcyclicTest.copy((*it).x1()),
			graphAcyclicTest.copy((*it).x2())));

	bool acyclic = isAcyclic(graphAcyclicTest);

	SListConstIterator<edge> itE;
	for(itE = added.begin(); itE.valid(); ++itE)
		graphAcyclicTest.delEdge(*itE);

	return acyclic;
}

}
