/** \file
 * \brief Implementation of class MaximumPlanarSubgraph
 *
 * \author Karsten Klein
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

#include <ogdf/planarity/MaximumPlanarSubgraph.h>
#include <ogdf/cluster/MaximumCPlanarSubgraph.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/simple_graph_alg.h>

namespace ogdf {

Module::ReturnType MaximumPlanarSubgraph::doCall(
	const Graph &G,
	const List<edge> &preferedEdges,
	List<edge> &delEdges,
	const EdgeArray<int>  *pCost,
	bool preferredImplyPlanar)
{
	if (G.numberOfEdges() < 9)
		return ReturnType::Optimal;

	//if the graph is planar, we don't have to do anything
	if (isPlanar(G))
		return ReturnType::Optimal;

	//Exact ILP
	MaximumCPlanarSubgraph mc;

	delEdges.clear();

	NodeArray<node> tableNodes(G,nullptr);
	EdgeArray<edge> tableEdges(G,nullptr);
	NodeArray<bool> mark(G,0);

	EdgeArray<int> componentID(G);

	// Determine biconnected components
	int bcCount = biconnectedComponents(G,componentID);
	OGDF_ASSERT(bcCount >= 1);

	// Determine edges per biconnected component
	Array<SList<edge> > blockEdges(0,bcCount-1);
	for(edge e : G.edges)
	{
		if (!e->isSelfLoop())
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


	// Perform computation for every biconnected component
	ReturnType mr;
	if (bcCount == 1) {
		ClusterGraph CG(G);
		mr = mc.call(CG, pCost, delEdges);
	}
	else
	{
		for (i = 0; i < bcCount; i++)
		{
			Graph C;

			for (node v : blockNodes[i])
			{
				node w = C.newNode();
				tableNodes[v] = w;
			}

			EdgeArray<int> cost(C);

			for (edge e : blockEdges[i])
			{
				edge f = C.newEdge(tableNodes[e->source()],tableNodes[e->target()]);
				tableEdges[e] = f;
				if(pCost != nullptr) {
					cost[tableEdges[e]] = (*pCost)[e];
				}
			}

			// Construct a translation table for the edges.
			// Necessary, since edges are deleted in a new graph
			// that represents the current biconnected component
			// of the original graph.
			EdgeArray<edge> backTableEdges(C,nullptr);
			for (edge e : blockEdges[i])
				backTableEdges[tableEdges[e]] = e;

			// The deleted edges of the biconnected component
			List<edge> delEdgesOfBC;

			ClusterGraph CG(C);
			mr = mc.call(CG, pCost == nullptr ? nullptr : &cost, delEdgesOfBC);

			// Abort if no optimal solution found, i.e., feasible is also not allowed
			if (mr != ReturnType::Optimal)
				break;

			// Get the original edges that are deleted and
			// put them on the list delEdges.
			while (!delEdgesOfBC.empty())
				delEdges.pushBack(backTableEdges[delEdgesOfBC.popFrontRet()]);

		}
	}
	return mr;
}

}
