/** \file
 * \brief Declaration of ClusterPlanRep class, allowing cluster
 * boundary insertion and shortest path edge insertion
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

#include <ogdf/planarity/PlanRep.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>
#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterArray.h>

#include <ogdf/basic/HashArray.h>


namespace ogdf {

//! Planarized representations for clustered graphs.
/**
 * @ingroup plan-rep gd-helper
 */
class OGDF_EXPORT ClusterPlanRep : public PlanRep
{
public:
	ClusterPlanRep(
		const ClusterGraphAttributes &acGraph,
		const ClusterGraph &clusterGraph);

	virtual ~ClusterPlanRep() { }

	void initCC(int i);

	//edge on the cluster boundary, adjSource
	void setClusterBoundary(edge e) {
		setEdgeTypeOf(e, edgeTypeOf(e) | clusterPattern());
	}
	bool isClusterBoundary(edge e) {
		return (edgeTypeOf(e) & clusterPattern()) == clusterPattern();
	}
	const ClusterGraph &getClusterGraph() const {
		return *m_pClusterGraph;
	}

	/** re-inserts edge eOrig by "crossing" the edges in crossedEdges;
	 *   splits each edge in crossedEdges
	 * Precond.: eOrig is an edge in the original graph,
	 *           the edges in crossedEdges are in this graph,
	 *           cluster boundaries are modelled as edge paths
	 * \param eOrig: Original edge to be inserted
	 * \param crossedEdges: Edges that are crossed by this insertion
	 * \param E: The embedding in which the edge is inserted
	 */
	void insertEdgePathEmbedded(
		edge eOrig,
		CombinatorialEmbedding &E,
		const SList<adjEntry> &crossedEdges);

	void ModelBoundaries();

	//rootadj is set by ModelBoundaries
	adjEntry externalAdj() { return m_rootAdj; }


	//structural alterations

	//! Expands nodes with degree > 4 and merge nodes for generalizations
	virtual void expand(bool lowDegreeExpand = false) override;

	virtual void expandLowDegreeVertices(OrthoRep &OR);

	//! Splits edge e, updates clustercage lists if necessary and returns new edge
	virtual edge split(edge e) override
	{
		edge eNew = PlanRep::split(e);

		//update edge to cluster info
		m_edgeClusterID[eNew] = m_edgeClusterID[e];
		m_nodeClusterID[eNew->source()] = m_edgeClusterID[e];

		return eNew;
	}

	/**
	 * Returns cluster of edge \p e
	 *
	 * Edges only have unique numbers if clusters are already modelled.
	 * We derive the edge cluster from the endnode cluster information.
	 */
	cluster clusterOfEdge(edge e)
	{
		const auto sourceId = ClusterID(e->source());
		const auto targetId = ClusterID(e->target());
		cluster targetCluster = clusterOfIndex(targetId);

		if (sourceId == targetId)
			return targetCluster;

		cluster sourceCluster = clusterOfIndex(sourceId);

		if (sourceCluster == targetCluster->parent())
			return sourceCluster;
		if (targetCluster == sourceCluster->parent())
			return targetCluster;
		if (targetCluster->parent() == sourceCluster->parent())
			return sourceCluster->parent();

		OGDF_ASSERT(false);
		OGDF_THROW(AlgorithmFailureException);
	}

	inline int ClusterID(node v) const {return m_nodeClusterID[v];}
	inline int ClusterID(edge e) const {return m_edgeClusterID[e];}
	cluster clusterOfIndex(int i) {
		OGDF_ASSERT(m_clusterOfIndex.isDefined(i));
		return m_clusterOfIndex[i];
	}

	inline cluster clusterOfDummy(node v)
		{
			OGDF_ASSERT(!original(v));
			OGDF_ASSERT(ClusterID(v) != -1);
			return clusterOfIndex(ClusterID(v));
		}

	//output functions
	void writeGML(const char *fileName, const Layout &drawing);
	void writeGML(const char *fileName);
	void writeGML(std::ostream &os, const Layout &drawing);

protected:
	//! Insert boundaries for all given clusters
	void convertClusterGraph(cluster act,
	                         AdjEntryArray<edge>& currentEdge,
	                         AdjEntryArray<int>& outEdge);

	//! Insert edges to represent the cluster boundary
	void insertBoundary(cluster C,
	                    AdjEntryArray<edge>& currentEdge,
	                    AdjEntryArray<int>& outEdge,
	                    bool clusterIsLeaf);

	//! Reinserts edges to planarize the graph after convertClusterGraph
	void reinsertEdge(edge e);

private:
	const ClusterGraph *m_pClusterGraph;

	edgeType clusterPattern() { return UMLEdgeTypeConstants::SecCluster << UMLEdgeTypeOffsets::Secondary; }

	//! Connects cluster on highest level with non cluster or same level
	adjEntry m_rootAdj;

	EdgeArray<int> m_edgeClusterID;
	NodeArray<int> m_nodeClusterID;
	//we maintain an array of index to cluster mappings (CG is const)
	//cluster numbers don't need to be consecutive
	HashArray<int, cluster> m_clusterOfIndex;
};

}
