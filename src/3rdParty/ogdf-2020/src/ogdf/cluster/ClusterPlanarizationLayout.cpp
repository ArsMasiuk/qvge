/** \file
 * \brief implementation of class ClusterPlanarizationLayout
 * applies planarization approach for drawing Cluster diagrams
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


#include <ogdf/cluster/ClusterPlanarizationLayout.h>
#include <ogdf/cluster/CconnectClusterPlanarEmbed.h>
#include <ogdf/cluster/CPlanarSubClusteredGraph.h>
#include <ogdf/cluster/ClusterOrthoLayout.h>
#include <ogdf/packing/TileToRowsCCPacker.h>
#include <ogdf/basic/extended_graph_alg.h>


namespace ogdf {


ClusterPlanarizationLayout::ClusterPlanarizationLayout()
{
	m_pageRatio = 1.0;

	m_planarLayouter.reset(new ClusterOrthoLayout);
	m_packer.reset(new TileToRowsCCPacker);
}


//the call function that lets ClusterPlanarizationLayout compute a layout
//for the input
void ClusterPlanarizationLayout::call(
	Graph& G,
	ClusterGraphAttributes& acGraph,
	ClusterGraph& cGraph,
	bool simpleCConnect) //default true
{
	EdgeArray<double> edgeWeight;
	call(G, acGraph, cGraph, edgeWeight, simpleCConnect);
}


//the call function that lets ClusterPlanarizationLayout compute a layout
//for the input using \p edgeWeight for the computation of the cluster planar subgraph
void ClusterPlanarizationLayout::call(
	Graph& G,
	ClusterGraphAttributes& acGraph,
	ClusterGraph& cGraph,
	EdgeArray<double>& edgeWeight,
	bool simpleCConnect) //default true
{
	m_nCrossings = 0;
	bool subGraph = false; // c-planar subgraph computed?

	// Check some simple cases
	if (G.numberOfNodes() == 0) return;

	// We set pointers and arrays to the working graph, which can be
	// the original or, in the case of non-c-planar input, a copy.

	Graph* workGraph = &G;
	ClusterGraph* workCG = &cGraph;
	ClusterGraphAttributes* workACG = &acGraph;

	// Potential copy of original if non c-planar
	Graph GW;
	// List of non c-planarity causing edges
	List<edge> deletedEdges;

	// Store some information
	// Original to copy
	NodeArray<node> resultNode(G);
	EdgeArray<edge> resultEdge(G);
	ClusterArray<cluster> resultCluster(cGraph);
	// Copy to original
	NodeArray<node> origNode(G);
	EdgeArray<edge> origEdges(G);
	ClusterArray<cluster> orCluster(cGraph);

	for(node workv : G.nodes) {
		resultNode[workv] = workv; //will be set to copy if non-c-planar
		origNode[workv] = workv;
	}
	for(edge worke : G.edges) {
		resultEdge[worke] = worke; //will be set to copy if non-c-planar
		origEdges[worke] = worke;
	}
	for (cluster workc : cGraph.clusters) {
		resultCluster[workc] = workc; //will be set to copy if non-c-planar
		orCluster[workc] = workc;
	}


	// Check if instance is clusterplanar and embed it
	CconnectClusterPlanarEmbed CCPE; //cccp

	bool cplanar = CCPE.embed(cGraph, G);

	List<edge> connectEdges;

	// If the graph is not c-planar, we have to check the reason and to
	// correct the problem by planarizing or inserting connection edges.
	if (!cplanar)
	{
		bool connect = false;

		if ( (CCPE.errCode() == CconnectClusterPlanarEmbed::ErrorCode::nonConnected) ||
			(CCPE.errCode() == CconnectClusterPlanarEmbed::ErrorCode::nonCConnected) )
		{
			//we insert edges to make the input c-connected
			makeCConnected(cGraph, G, connectEdges, simpleCConnect);

			//save edgearray info for inserted edges
			for(edge e : connectEdges)
			{
				resultEdge[e] = e;
				origEdges[e] = e;
			}

			connect = true;

			CCPE.embed(cGraph, G);

			if ( (CCPE.errCode() == CconnectClusterPlanarEmbed::ErrorCode::nonConnected) ||
				(CCPE.errCode() == CconnectClusterPlanarEmbed::ErrorCode::nonCConnected) )
			{
				std::cerr << "no correct connection made\n"<<std::flush;
				OGDF_THROW(AlgorithmFailureException);
			}
		}
		if ((CCPE.errCode() == CconnectClusterPlanarEmbed::ErrorCode::nonPlanar) ||
			(CCPE.errCode() == CconnectClusterPlanarEmbed::ErrorCode::nonCPlanar))
		{
			subGraph = true;
			EdgeArray<bool> inSubGraph(G, false);

			CPlanarSubClusteredGraph cps;
			if (edgeWeight.valid())
				cps.call(cGraph, inSubGraph, deletedEdges, edgeWeight);
			else
				cps.call(cGraph, inSubGraph, deletedEdges);
#ifdef OGDF_DEBUG
#if 0
			for(edge worke : G.edges) {
				if (inSubGraph[worke])
					acGraph.strokeColor(worke) = "#FF0000";
			}
#endif
#endif
			//now we delete the copies of all edges not in subgraph and embed
			//the subgraph (use a new copy)

			//construct copy

			workGraph = &GW;
			workCG = new ClusterGraph(cGraph, GW, resultCluster, resultNode, resultEdge);

			//reinit original arrays
			origNode.init(GW, nullptr);
			origEdges.init(GW, nullptr);
			orCluster.init(*workCG, nullptr);

			//set array entries to the appropriate values
			for (node workv : G.nodes)
				origNode[resultNode[workv]] = workv;
			for (edge worke : G.edges)
				origEdges[resultEdge[worke]] = worke;
			for (cluster workc : cGraph.clusters)
				orCluster[resultCluster[workc]] = workc;

			//create new ACG and copy values (width, height, type)

			workACG = new ClusterGraphAttributes(*workCG, workACG->attributes());
			for (node workv : GW.nodes)
			{
				//should set same attributes in construction!!!
				if (acGraph.has(GraphAttributes::nodeType)) {
					workACG->type(workv) = acGraph.type(origNode[workv]);
				}
				workACG->height(workv) = acGraph.height(origNode[workv]);
				workACG->width(workv) = acGraph.width(origNode[workv]);
			}
			if (acGraph.has(GraphAttributes::edgeType)) {
				for (edge worke : GW.edges) {
					workACG->type(worke) = acGraph.type(origEdges[worke]);
					//all other attributes are not needed or will be set
				}
			}
#if 1
			// hide leftEdges edges while embedding the c-planar subgraph

			Graph::HiddenEdgeSet hiddenEdges(GW);

			for(edge e : deletedEdges) {
				hiddenEdges.hide(resultEdge[e]);
			}

			CconnectClusterPlanarEmbed CCP;

#ifdef OGDF_DEBUG
			bool subPlanar =
#endif
				CCP.embed(*workCG, GW);
			OGDF_ASSERT(subPlanar);
#endif
		} else {
			// assert that we fixed c-connection if non-c-planarity is not (or no longer) the issue
			OGDF_ASSERT(connect);
		}
	}

	//if multiple CCs are handled, the connectedges (their copies resp.)
	//can be deleted here

	//now CCPE should give us the external face

	ClusterPlanRep CP(*workACG, *workCG);

	OGDF_ASSERT(CP.representsCombEmbedding());

	const int numCC = CP.numberOfCCs(); //equal to one
	//preliminary
	OGDF_ASSERT(numCC == 1);

	// (width,height) of the layout of each connected component
	Array<DPoint> boundingBox(numCC);

	List<edge> resultDeletedEdges;
	for(edge e : deletedEdges) {
		resultDeletedEdges.pushBack(resultEdge[e]);
	}

	for (int ikl = 0; ikl < numCC; ikl++)
	{

			CP.initCC(ikl);
			CP.setOriginalEmbedding();

			for(edge e : resultDeletedEdges) {
				CP.delEdge(CP.copy(e));
			}

			OGDF_ASSERT(CP.representsCombEmbedding());

			Layout drawing(CP);

#if 0
			m_planarLayouter->setOptions(4);//progressive
#endif

			adjEntry ae = nullptr;

			//internally compute adjEntry for outer face

			//edges that are reinserted in workGraph (in the same
			//order as leftWNodes)
			m_planarLayouter->call(CP, ae, drawing, resultDeletedEdges, *workGraph);

			//hash index over cluster ids
			HashArray<int, ClusterPosition> CA;

			computeClusterPositions(CP, drawing, CA);

			// copy layout into acGraph
			// Later, we move nodes and edges in each connected component, such
			// that no two overlap.

			for(int i = CP.startNode(); i < CP.stopNode(); ++i) {
				node vG = CP.v(i);

				acGraph.x(origNode[vG]) = drawing.x(CP.copy(vG));
				acGraph.y(origNode[vG]) = drawing.y(CP.copy(vG));

				for(adjEntry adj : vG->adjEntries)
				{
					if ((adj->index() & 1) == 0) continue;
					edge eG = adj->theEdge();

					edge orE = origEdges[eG];
					if (orE)
						drawing.computePolylineClear(CP,eG,acGraph.bends(orE));
				}
			}

			//even assignment for all nodes is not enough, we need all clusters
			for(cluster c : workCG->clusters)
			{
				int clNumber = c->index();
#if 0
				int orNumber = originalClId[c];
#endif
				cluster orCl = orCluster[c];

				if (c != workCG->rootCluster())
				{
					OGDF_ASSERT(CA.isDefined(clNumber));
					acGraph.height(orCl) = CA[clNumber].m_height;
					acGraph.width(orCl) = CA[clNumber].m_width;
					acGraph.y(orCl) = CA[clNumber].m_miny;
					acGraph.x(orCl) = CA[clNumber].m_minx;
				}
			}

			// the width/height of the layout has been computed by the planar
			// layout algorithm; required as input to packing algorithm
			boundingBox[ikl] = m_planarLayouter->getBoundingBox();
	}

#if 0
	postProcess(acGraph);
#endif

	// arrange layouts of connected components
	//

	Array<DPoint> offset(numCC);

	m_packer->call(boundingBox,offset,m_pageRatio);

	// The arrangement is given by offset to the origin of the coordinate
	// system. We still have to shift each node, edge and cluster by the offset
	// of its connected component.

	const Graph::CCsInfo &ccInfo = CP.ccInfo();
	for(int i = 0; i < numCC; ++i)
	{
		const double dx = offset[i].m_x;
		const double dy = offset[i].m_y;

		HashArray<int, bool> shifted(false);

		// iterate over all nodes in ith CC
		for(int j = ccInfo.startNode(i); j < ccInfo.stopNode(i); ++j)
		{
			node v = ccInfo.v(j);

			acGraph.x(origNode[v]) += dx;
			acGraph.y(origNode[v]) += dy;

			// update cluster positions accordingly
#if 0
			int clNumber = cGraph.clusterOf(orNode[v])->index();
#endif
			cluster cl = cGraph.clusterOf(origNode[v]);

			if ((cl->index() > 0) && !shifted[cl->index()])
			{
				acGraph.y(cl) += dy;
				acGraph.x(cl) += dx;
				shifted[cl->index()] = true;
			}

			for(adjEntry adj : v->adjEntries) {
				if ((adj->index() & 1) == 0) continue;
				edge e = adj->theEdge();

#if 0
				edge eOr = origEdges[e];
#endif
				if (origEdges[e])
				{
					DPolyline &dpl = acGraph.bends(origEdges[e]);
					for(DPoint &p : dpl) {
						p.m_x += dx;
						p.m_y += dy;
					}
				}
			}
		}
	}

	while (!connectEdges.empty()) {
		G.delEdge(connectEdges.popFrontRet());
	}

	if (subGraph) { // if subgraph created
#if 0
		originalClId.init();
#endif
		orCluster.init();
		origNode.init();
		origEdges.init();
		delete workCG;
		delete workACG;
	}

	acGraph.removeUnnecessaryBendsHV();
}

void ClusterPlanarizationLayout::computeClusterPositions(
	ClusterPlanRep& CP,
	Layout drawing,
	HashArray<int, ClusterPosition>& CA)
{
	for(edge e : CP.edges)
	{
		if (CP.isClusterBoundary(e))
		{
			ClusterPosition cpos;
			//minimum vertex position values
			double minx, maxx, miny, maxy;
			minx = min(drawing.x(e->source()), drawing.x(e->target()));
			maxx = max(drawing.x(e->source()), drawing.x(e->target()));
			miny = min(drawing.y(e->source()), drawing.y(e->target()));
			maxy = max(drawing.y(e->source()), drawing.y(e->target()));

			//check if x,y of lower left corner have to be updated
			if (CA.isDefined(CP.ClusterID(e)))
			{
				cpos = CA[CP.ClusterID(e)];

				double preValmaxx = cpos.m_maxx;
				double preValmaxy = cpos.m_maxy;

				if (cpos.m_minx > minx) cpos.m_minx = minx;
				if (cpos.m_miny > miny) cpos.m_miny = miny;
				if (preValmaxx < maxx) cpos.m_maxx = maxx;
				if (preValmaxy < maxy) cpos.m_maxy = maxy;

			} else {
				cpos.m_minx = minx;
				cpos.m_maxx = maxx;
				cpos.m_miny = miny;
				cpos.m_maxy = maxy;
			}

			//not necessary for all boundary edges, but we only have the ids
			//to adress, they may have holes
			cpos.m_width = cpos.m_maxx - cpos.m_minx;
			cpos.m_height = cpos.m_maxy - cpos.m_miny;

			//write values back
			CA[CP.ClusterID(e)] = cpos;
		}
	}
}

}
