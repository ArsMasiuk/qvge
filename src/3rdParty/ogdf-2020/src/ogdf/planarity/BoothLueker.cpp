/** \file
 * \brief  Implementation of the Booth-Lueker planarity test.
 *
 * Implements planarity test and planar embedding algorithm.
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


#include <ogdf/basic/basic.h>
#include <ogdf/basic/Array.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/SList.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/STNumbering.h>
#include <ogdf/planarity/booth_lueker/PlanarPQTree.h>
#include <ogdf/planarity/BoothLueker.h>
#include <ogdf/planarity/booth_lueker/EmbedPQTree.h>

namespace ogdf{

using namespace booth_lueker;

bool BoothLueker::isPlanarDestructive(Graph &G)
{
	bool ret = preparation(G,false);
	m_parallelEdges.init();
	m_isParallel.init();

	return ret;
}

bool BoothLueker::isPlanar(const Graph &G)
{
	Graph Gp(G);
	bool ret = preparation(Gp,false);
	m_parallelEdges.init();
	m_isParallel.init();

	return ret;
}

// Prepares the planarity test and the planar embedding
// Parallel edges:  do not need to be ignored, they can be handled
// by the planarity test.
// Selfloops: need to be ignored.
bool BoothLueker::preparation(Graph &G, bool embed)
{
	if (G.numberOfEdges() < 9 && !embed)
		return true;
	else if (G.numberOfEdges() < 3 && embed)
		return true;

	SListPure<node> selfLoops;
	makeLoopFree(G,selfLoops);

	prepareParallelEdges(G);

	int  isolated = 0;
	for(node v : G.nodes)
		if (v->degree() == 0)
			isolated++;

	if (((G.numberOfNodes()-isolated) > 2) &&
		((3*(G.numberOfNodes()-isolated) -6) < (G.numberOfEdges() - m_parallelCount)))
		return false;

	bool planar = true;

	NodeArray<node> tableNodes(G,nullptr);
	EdgeArray<edge> tableEdges(G,nullptr);
	NodeArray<bool> mark(G,0);

	EdgeArray<int> componentID(G);

	// Determine Biconnected Components
	int bcCount = biconnectedComponents(G,componentID);

	// Determine edges per biconnected component
	Array<SList<edge> > blockEdges(0,bcCount-1);
	for(edge e : G.edges)
	{
		blockEdges[componentID[e]].pushFront(e);
	}

	// Determine nodes per biconnected component.
	Array<SList<node> > blockNodes(0,bcCount-1);
	int i;
	for (i = 0; i < bcCount; i++)
	{
		for (edge e : blockEdges[i])
		{
			if (!mark[e->source()])
			{
				blockNodes[i].pushBack(e->source());
				mark[e->source()] = true;
			}
			if (!mark[e->target()])
			{
				blockNodes[i].pushBack(e->target());
				mark[e->target()] = true;
			}
		}

		for (node v : blockNodes[i])
			mark[v] = false;
	}

	// Perform Planarity Test for every biconnected component

	if (bcCount == 1)
	{
		if (G.numberOfEdges() >= 2)
		{
			// Compute st-numbering
			NodeArray<int> numbering(G,0);
#ifdef OGDF_HEAVY_DEBUG
			int n =
#endif
			computeSTNumbering(G, numbering);
			OGDF_HEAVY_ASSERT(isSTNumbering(G, numbering, n));

			EdgeArray<edge> backTableEdges(G,nullptr);
			for(edge e : G.edges)
				backTableEdges[e] = e;

			if (embed)
				planar = doEmbed(G,numbering,backTableEdges,backTableEdges);
			else
				planar = doTest(G,numbering);
		}
	}
	else
	{
		NodeArray<SListPure<adjEntry> > entireEmbedding(G);
		for (i = 0; i < bcCount; i++)
		{
			Graph C;

			for (node v : blockNodes[i])
			{
				node w = C.newNode();
				tableNodes[v] = w;
			}

			NodeArray<node> backTableNodes(C,nullptr);
			if (embed)
			{
				for (node v : blockNodes[i])
					backTableNodes[tableNodes[v]] = v;
			}

			for (edge e : blockEdges[i])
			{
				edge f = C.newEdge(tableNodes[e->source()],tableNodes[e->target()]);
				tableEdges[e] = f;
			}

			EdgeArray<edge> backTableEdges(C,nullptr);
			for (edge e : blockEdges[i])
				backTableEdges[tableEdges[e]] = e;

			if (C.numberOfEdges() >= 2)
			{
				// Compute st-numbering
				NodeArray<int> numbering(C,0);
#ifdef OGDF_HEAVY_DEBUG
				int n =
#endif
				computeSTNumbering(C, numbering);
				OGDF_HEAVY_ASSERT(isSTNumbering(C, numbering, n));

				if (embed)
					planar = doEmbed(C,numbering,backTableEdges,tableEdges);
				else
					planar = doTest(C,numbering);

				if (!planar)
					break;
			}

			if (embed)
			{
				for(node v : C.nodes)
				{
					node w = backTableNodes[v];
					for(adjEntry a : v->adjEntries)
					{
						edge e = backTableEdges[a->theEdge()];
						adjEntry adj = (e->adjSource()->theNode() == w)?
										e->adjSource() : e->adjTarget();
						entireEmbedding[w].pushBack(adj);
					}
				}
			}
		}

		if (planar && embed)
		{
			for(node v : G.nodes)
				G.sort(v,entireEmbedding[v]);
		}

	}

	while (!selfLoops.empty())
	{
		node v = selfLoops.popFrontRet();
		G.newEdge(v,v);
	}

	OGDF_HEAVY_ASSERT(!planar || !embed || G.representsCombEmbedding());

	return planar;
}


// Performs a planarity test on a biconnected component
// of G. numbering contains an st-numbering of the component.
bool BoothLueker::doTest(Graph &G,NodeArray<int> &numbering)
{
	bool planar = true;

	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > inLeaves(G);
	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > outLeaves(G);
	Array<node> table(G.numberOfNodes()+1);

	for(node v : G.nodes)
	{
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			if (numbering[e->opposite(v)] > numbering[v])
				//sideeffect: loops are ignored
			{
				PlanarLeafKey<IndInfo*>* L = new PlanarLeafKey<IndInfo*>(e);
				inLeaves[v].pushFront(L);
			}
		}
		table[numbering[v]] = v;
	}

	for(node v : G.nodes)
	{
		for (PlanarLeafKey<IndInfo*>* L : inLeaves[v])
		{
			outLeaves[L->userStructKey()->opposite(v)].pushFront(L);
		}
	}

	PlanarPQTree T;

	T.Initialize(inLeaves[table[1]]);
	for (int i = 2; i < G.numberOfNodes(); i++)
	{
		if (T.Reduction(outLeaves[table[i]]))
		{
			T.ReplaceRoot(inLeaves[table[i]]);
			T.emptyAllPertinentNodes();

		}
		else
		{
			planar = false;
			break;
		}
	}
	if (planar)
		T.emptyAllPertinentNodes();


	// Cleanup
	for(node v : G.nodes)
	{
		while (!inLeaves[v].empty())
		{
			PlanarLeafKey<IndInfo*>* L = inLeaves[v].popFrontRet();
			delete L;
		}
	}

	return planar;
}


// Performs a planarity test on a biconnected component
// of G and embedds it planar.
// numbering contains an st-numbering of the component.
bool BoothLueker::doEmbed(
	Graph &G,
	NodeArray<int>  &numbering,
	EdgeArray<edge> &backTableEdges,
	EdgeArray<edge> &forwardTableEdges)
{

	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > inLeaves(G);
	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > outLeaves(G);
	NodeArray<SListPure<edge> > frontier(G);
	NodeArray<SListPure<node> > opposed(G);
	NodeArray<SListPure<node> > nonOpposed(G);
	Array<node> table(G.numberOfNodes()+1);
	Array<bool> toReverse(1,G.numberOfNodes()+1,false);

	for(node v : G.nodes)
	{
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			if (numbering[e->opposite(v)] > numbering[v])
			{
				PlanarLeafKey<IndInfo*>* L = new PlanarLeafKey<IndInfo*>(e);
				inLeaves[v].pushFront(L);
			}
		}
		table[numbering[v]] = v;
	}

	for(node v : G.nodes)
	{
		for (PlanarLeafKey<IndInfo*>* L : inLeaves[v])
		{
			outLeaves[L->userStructKey()->opposite(v)].pushFront(L);
		}
	}

	booth_lueker::EmbedPQTree T;

	T.Initialize(inLeaves[table[1]]);
	int i;
	for (i = 2; i <= G.numberOfNodes(); i++)
	{
		if (T.Reduction(outLeaves[table[i]]))
		{
			T.ReplaceRoot(inLeaves[table[i]], frontier[table[i]], opposed[table[i]], nonOpposed[table[i]], table[i]);
			T.emptyAllPertinentNodes();
		}
		else
		{
			// Cleanup
			for(node v : G.nodes)
			{
				while (!inLeaves[v].empty())
				{
					PlanarLeafKey<IndInfo*>* L = inLeaves[v].popFrontRet();
					delete L;
				}
			}
			return false;
		}
	}

	// Reverse adjacency lists if necessary
	// This gives an upward embedding
	for (i = G.numberOfNodes(); i >= 2; i--)
	{
		if (toReverse[i])
		{
			while (!nonOpposed[table[i]].empty())
			{
				node v = nonOpposed[table[i]].popFrontRet();
				toReverse[numbering[v]] =  true;
			}
			frontier[table[i]].reverse();
		}
		else
		{
			while (!opposed[table[i]].empty())
			{
				node v = opposed[table[i]].popFrontRet();
				toReverse[numbering[v]] =  true;
			}
		}
		nonOpposed[table[i]].clear();
		opposed[table[i]].clear();
	}

	// Compute the entire embedding
	NodeArray<SListPure<adjEntry> > entireEmbedding(G);
	for(node v : G.nodes)
	{
		while (!frontier[v].empty())
		{
			edge e = frontier[v].popFrontRet();
			entireEmbedding[v].pushBack(
				(e->adjSource()->theNode() == v)? e->adjSource() : e->adjTarget());
		}
	}

	NodeArray<bool> mark(G,false);
	NodeArray<SListIterator<adjEntry> > adjMarker(G,nullptr);
	for(node v : G.nodes)
		adjMarker[v] = entireEmbedding[v].begin();

	entireEmbed(G, entireEmbedding, adjMarker, mark, table[G.numberOfNodes()]);

	NodeArray<SListPure<adjEntry> > newEntireEmbedding(G);
	if (m_parallelCount > 0)
	{
		for(node v : G.nodes)
		{
			for(adjEntry a : entireEmbedding[v])
			{
				edge e = a->theEdge(); // edge in biconnected component
				edge trans = backTableEdges[e]; // edge in original graph.
				if (!m_parallelEdges[trans].empty())
				{
					// This original edge is the reference edge
					// of a bundle of parallel edges

					// If v is source of e, insert the parallel edges
					// in the order stored in the list.
					if (e->adjSource()->theNode() == v)
					{
						adjEntry adj = e->adjSource();
						newEntireEmbedding[v].pushBack(adj);
						for (edge ei : m_parallelEdges[trans])
						{
							edge parallel = forwardTableEdges[ei];
							adjEntry adjParallel = parallel->adjSource()->theNode() == v ?
								parallel->adjSource() : parallel->adjTarget();
							newEntireEmbedding[v].pushBack(adjParallel);
						}
					}
					else
					// v is target of e, insert the parallel edges
					// in the opposite order stored in the list.
					// This keeps the embedding.
					{
						for (edge parEdge : reverse(m_parallelEdges[trans])) {
							edge parallel = forwardTableEdges[parEdge];
							adjEntry adj = parallel->adjSource()->theNode() == v ?
								parallel->adjSource() : parallel->adjTarget();
							newEntireEmbedding[v].pushBack(adj);
						}
						adjEntry adj = e->adjTarget();
						newEntireEmbedding[v].pushBack(adj);
					}
				}
				else if (!m_isParallel[trans])
					// normal non-multi-edge
				{
					adjEntry adj = e->adjSource()->theNode() == v?
									e->adjSource() : e->adjTarget();
					newEntireEmbedding[v].pushBack(adj);
				}
				// else e is a multi-edge but not the reference edge
			}
		}

		for(node v : G.nodes)
			G.sort(v,newEntireEmbedding[v]);
	}
	else
	{
		for(node v : G.nodes)
			G.sort(v,entireEmbedding[v]);
	}


	//cleanup
	for(node v : G.nodes)
	{
		while (!inLeaves[v].empty())
		{
			PlanarLeafKey<IndInfo*>* L = inLeaves[v].popFrontRet();
			delete L;
		}
	}

	return true;
}

// Used by doEmbed. Computes an entire embedding from an
// upward embedding.
void BoothLueker::entireEmbed(
	Graph &G,
	NodeArray<SListPure<adjEntry> > &entireEmbedding,
	NodeArray<SListIterator<adjEntry> > &adjMarker,
	NodeArray<bool> &mark,
	node v)
{
	mark[v] = true;
	SListIterator<adjEntry> it;
	for (it = adjMarker[v]; it.valid(); ++it)
	{
		adjEntry a = *it;
		edge e = a->theEdge();
		adjEntry adj = (e->adjSource()->theNode() == v)?
						e->adjTarget() : e->adjSource();
		node w = adj->theNode();
		entireEmbedding[w].pushFront(adj);
		if (!mark[w])
			entireEmbed(G,entireEmbedding,adjMarker,mark,w);
	}
}

void BoothLueker::prepareParallelEdges(Graph &G)
{
	// Stores for one reference edge all parallel edges.
	m_parallelEdges.init(G);
	// Is true for any multiedge, except for the reference edge.
	m_isParallel.init(G,false);
	getParallelFreeUndirected(G,m_parallelEdges);
	m_parallelCount = 0;
	for(edge e : G.edges)
	{
		if (!m_parallelEdges[e].empty())
		{
			for (edge ei : m_parallelEdges[e])
			{
				m_isParallel[ei] = true;
				m_parallelCount++;
			}
		}
	}
}


}
