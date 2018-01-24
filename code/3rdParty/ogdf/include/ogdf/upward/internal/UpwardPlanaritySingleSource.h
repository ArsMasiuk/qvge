/** \file
 * \brief Declaration of class UpwardPlanaritySingleSource, which implements
 *        the upward-planarity testing and embedding algorithm for
 *        single-source digraphs by Bertolazzi et al.
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

#pragma once

#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/SList.h>
#include <ogdf/decomposition/SPQRTree.h>
#include <ogdf/decomposition/StaticPlanarSPQRTree.h>
#include <ogdf/upward/ExpansionGraph.h>
#include <ogdf/upward/FaceSinkGraph.h>

namespace ogdf {

//! Performs upward planarity testing and embedding for single-source digraphs.
class OGDF_EXPORT UpwardPlanaritySingleSource
{
public:

	// test and compute adjacency lists of embedding
	static bool testAndFindEmbedding(
		const Graph &G,
		bool  embed,
		NodeArray<SListPure<adjEntry> > &adjacentEdges);

	// embed and compute st-augmentation (new implementation - inserts only
	// one new node into G which is the super sink)
	static void embedAndAugment(
		Graph &G,
		NodeArray<SListPure<adjEntry> > &adjacentEdges,
		bool augment,
		node &superSink,
		SList<edge> &augmentedEdges);

private:

	struct DegreeInfo {
		int m_indegSrc;
		int m_outdegSrc;
		int m_indegTgt;
		int m_outdegTgt;
	};

	// classes defined and used in UpwardPlanaritySingleSource.cpp
	class OGDF_EXPORT SkeletonInfo;

	//! Maintains constraints set during upward-planarity test on rooting of SPQR-tree
	class ConstraintRooting;


	// performs the actual test (and computation of sorted adjacency lists) for
	// each biconnected component
	static bool testBiconnectedComponent(
		ExpansionGraph &exp,
		node sG,
		int parentBlock,
		bool embed,
		NodeArray<SListPure<adjEntry> > &adjacentEdges);

	//! \name Computation of st-skeletons
	//! @{

	// compute sT-skeletons
	// test for upward-planarity, build constraints for rooting, and find a
	// rooting of the tree satisfying all constraints
	// returns true iff such a rooting exists
	static edge directSkeletons(
		SPQRTree &T,
		NodeArray<SkeletonInfo> &skInfo);

	// precompute information: in-/outdegrees in pertinent graph, contains
	// pertinent graph the source?
	static void computeDegreesInPertinent(
		const SPQRTree &T,
		node s,
		NodeArray<SkeletonInfo> &skInfo,
		node vT);

	//! @}
	//! \name Embedding of skeletons
	//! @{

	static bool initFaceSinkGraph(const Graph &M, SkeletonInfo &skInfo);

	static void embedSkeleton(
		Graph &G,
		StaticPlanarSPQRTree &T,
		NodeArray<SkeletonInfo> &skInfo,
		node vT,
		bool extFaceIsLeft);

	//! @}
	//! \name Assigning sinks to faces
	//! @{

	static void assignSinks(
		FaceSinkGraph &F,
		face extFace,
		NodeArray<face> &assignedFace);

	static node dfsAssignSinks(
		FaceSinkGraph &F,
		node v,                      // current node
		node parent,                 // its parent
		NodeArray<face> &assignedFace);

	//! @}
	//! \name For testing / debugging only
	//! @{

	static bool checkDegrees(
		SPQRTree &T,
		node s,
		NodeArray<SkeletonInfo> &skInfo);

	static bool virtualEdgesDirectedEqually(const SPQRTree &T);

	//! @}
};

}
