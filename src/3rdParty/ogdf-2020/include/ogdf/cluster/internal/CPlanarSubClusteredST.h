/** \file
 * \brief Declaration of CPlanarSubClusteredST class.
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

#pragma once

#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterArray.h>
#include <ogdf/basic/EdgeArray.h>

namespace ogdf {
namespace cluster_planarity {

//! Constructs a c-planar subclustered spanning tree of the input by setting edgearray values
class CPlanarSubClusteredST
{
public:
	CPlanarSubClusteredST() { }

	//! sets values in inST according to membership in c-planar STGraph
	virtual void call(const ClusterGraph& CG, EdgeArray<bool>& inST);
	//! sets values in inST according to membership in c-planar STGraph,
	//! computes minimum spanning tree according to weight in \p weight
	virtual void call(const ClusterGraph& CG,
		EdgeArray<bool>& inST,
		EdgeArray<double>& weight);

private:

	//! builds spanning tree on original graph out of repgraphs STs
	void dfsBuildOriginalST(/*ClusterGraph& CG,*/
	node v,
	ClusterArray< EdgeArray<bool> > &treeEdges,    //edges in repgraph
	EdgeArray<bool>& inST,                         //original edges
	NodeArray<bool> &visited);
	//builds spanning tree on graph of node v in treeEdges
	void dfsBuildSpanningTree(node v,
	                          EdgeArray<bool> &treeEdges,
	                          NodeArray<bool> &visited);

	//! constructs for every cluster a graph representing its
	//! main structure (children & their connections)
	//! only insert nodes here
	void constructRepresentationGraphNodes(const ClusterGraph& CG,
	                                       Graph& g,
	                                       cluster c)
	{

		// insert nodes for all child clusters
		ListConstIterator<cluster> it;
		for (auto child : c->children) {
			node v = g.newNode();
			m_cRepNode[child] = v;
		}
		// insert nodes for all node entries in c
		ListConstIterator<node> itn;
		for (auto u : c->nodes) {
			node v = g.newNode();
			m_vRepNode[u] = v;
		}
	}

	//! insert rep edges for all edges in clustergraph
	void constructRepresentationGraphEdges(const ClusterGraph& CG,
										   ClusterArray<Graph*>& RepGraph)
	{
		for(edge e : CG.constGraph().edges)
		{
			//insert representation in RepGraph[allocation cluster]
			//defined by lowest common ancestor of end points
			node u = e->source();
			node v = e->target();
			cluster uAncestor, vAncestor;
			cluster allocCluster =
				CG.commonClusterLastAncestors(u,v, uAncestor, vAncestor);
			m_allocCluster[e] = allocCluster;
			//now derive the real ancestors (maybe the nodes themselves from
			//the supplied clusters

			//Case1: both nodes in same cluster => connect the nodes in the
			//cluster representation graph
			if (uAncestor == vAncestor)
			{
				m_repEdge[e] = RepGraph[uAncestor]->newEdge(
								m_vRepNode[u], m_vRepNode[v]);
			} else {
				OGDF_ASSERT(uAncestor != CG.rootCluster()
				         || vAncestor != CG.rootCluster());
				//now only one node can be directly in rootcluster
				//this case now seems to fall together with else else...
				if (uAncestor == CG.rootCluster())
				{
					m_repEdge[e] = RepGraph[uAncestor]->newEdge(
								m_vRepNode[u], m_cRepNode[vAncestor]);
				} else if (vAncestor == CG.rootCluster()) {
					m_repEdge[e] = RepGraph[vAncestor]->newEdge(
								m_cRepNode[uAncestor], m_vRepNode[v]);
				} else {
					OGDF_ASSERT(allocCluster != nullptr);
					//now create edge in lowest common cluster
					node v1, v2;
					v1 = ( (uAncestor == nullptr) ? m_vRepNode[u] :
								m_cRepNode[uAncestor]);
					v2 = ( (vAncestor == nullptr) ? m_vRepNode[v] :
								m_cRepNode[vAncestor]);
					m_repEdge[e] = RepGraph[allocCluster]->newEdge(v1, v2);
				}
			}
		}
	}

	//! Computes representation graphs used for spanning tree computation
	void computeRepresentationGraphs(const ClusterGraph& CG,
	                                 ClusterArray<Graph*>& RepGraph)
	{
		for(cluster c : CG.clusters) {
			RepGraph[c] = new Graph;
			constructRepresentationGraphNodes(CG, *RepGraph[c], c);
		}
		constructRepresentationGraphEdges(CG, RepGraph);
	}

	void deleteRepresentationGraphs(const ClusterGraph& CG,
	                                ClusterArray<Graph*>& RepGraph)
	{
		for(cluster c : CG.clusters) {
			delete RepGraph[c];
		}
	}

	//! Initializes some internally used members on CG
	void initialize(const ClusterGraph& CG);

#if 0
	//! store status of original edge: in subclustered graph? also used to check spanning tree
	EdgeArray<int> m_edgeStatus;
#endif

	//! store the allocation cluster to avoid multiple computation
	EdgeArray<cluster> m_allocCluster;
	//! store the representation edge
	EdgeArray<edge> m_repEdge;
	//! store the representation nodes for nodes and clusters
	ClusterArray<node> m_cRepNode;
	NodeArray<node> m_vRepNode;
#if 0
	int m_genDebug; //only for debugging
#endif
};

}
}
