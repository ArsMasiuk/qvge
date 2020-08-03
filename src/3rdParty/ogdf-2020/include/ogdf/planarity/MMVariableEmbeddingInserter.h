/** \file
 * \brief declaration of class MMVariableEmbeddingInserter
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

#include <ogdf/planarity/MMEdgeInsertionModule.h>
#include <ogdf/planarity/RemoveReinsertType.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/NodeSet.h>
#include <ogdf/basic/tuples.h>

namespace ogdf {

//! Minor-monotone edge insertion with variable embedding.
/**
 * @ingroup ga-insert
 */
class OGDF_EXPORT MMVariableEmbeddingInserter : public MMEdgeInsertionModule
{
public:
	//! Creates a minor-monotone fixed embedding inserter.
	MMVariableEmbeddingInserter();

	// destruction
	virtual ~MMVariableEmbeddingInserter() { }


	/**
	 * Sets the remove-reinsert option for postprocessing.
	 *
	 * Note that RemoveReinsertType::IncInserted is not implemented.
	 */
	void removeReinsert(RemoveReinsertType rrOption) {
		OGDF_ASSERT(rrOption != RemoveReinsertType::IncInserted);
		m_rrOption = rrOption;
	}

	//! Returns the current setting of the remove-reinsert option.
	RemoveReinsertType removeReinsert() const {
		return m_rrOption;
	}


	//! Sets the portion of most crossed edges used during postprocessing.
	/**
	 * The value is only used if the remove-reinsert option is set to rrMostCrossed.
	 * The number of edges used in postprocessing is then
	 * number of edges * percentMostCrossed() / 100.
	 */
	void percentMostCrossed(double percent) {
		m_percentMostCrossed = percent;
	}

	//! Returns the current setting of the option percentMostCrossed.
	double percentMostCrossed() const {
		return m_percentMostCrossed;
	}


private:
	class Block;
	class ExpandedSkeleton;

	using Crossing = PlanRepExpansion::Crossing;

	struct AnchorNodeInfo {
		AnchorNodeInfo() { m_adj_1 = m_adj_2 = nullptr; }
		AnchorNodeInfo(adjEntry adj) {
			m_adj_1 = adj;
			m_adj_2 = nullptr;
		}
		AnchorNodeInfo(adjEntry adj_1, adjEntry adj_2) {
			m_adj_1 = adj_1;
			m_adj_2 = adj_2;
		}

		adjEntry m_adj_1;
		adjEntry m_adj_2;
	};

	enum class PathType { pathToEdge = 0, pathToSource = 1, pathToTarget = 2 };

	struct Paths {
		Paths() :
			m_addPartLeft(3), m_addPartRight(3),
			m_paths(3),
			m_src(0,2,nullptr), m_tgt(0,2,nullptr),
			m_pred(0,2,PathType::pathToEdge)
		{ }

		Array<SList<adjEntry> > m_addPartLeft;
		Array<SList<adjEntry> > m_addPartRight;
		Array<List<Crossing> >  m_paths;
		Array<AnchorNodeInfo>   m_src;
		Array<AnchorNodeInfo>   m_tgt;
		Array<PathType>         m_pred;
	};

	/**
	 * \brief Implementation of algorithm call.
	 *
	 * @param PG is the input planarized expansion and will also receive the result.
	 * @param origEdges is the list of original edges (edges in the original graph
	 *        of \p PG) that have to be inserted.
	 * @param forbiddenEdgeOrig points to an edge array indicating if an original edge is
	 *        forbidden to be crossed.
	 */
	virtual ReturnType doCall(
		PlanRepExpansion &PG,
		const List<edge> &origEdges,
		const EdgeArray<bool> *forbiddenEdgeOrig) override;

	/**
	 * \brief Collects all anchor nodes (including dummies) of a node.
	 *
	 * @param v is the current node when traversing all copy nodes of an original node
	 *        that are connected in a tree-wise manner.
	 * @param nodes is assigned the set of anchor nodes.
	 * @param nsParent is the parent node split.
	 */
	void collectAnchorNodes(
		node v,
		NodeSet<> &nodes,
		const PlanRepExpansion::NodeSplit *nsParent) const;

	/**
	 * \brief Finds the set of anchor nodes of \p src and \p tgt.
	 *
	 * @param src is a node in \a PG representing an original node.
	 * @param tgt is a node in \a PG representing an original node.
	 * @param sources ia assigned the set of anchor nodes of \p src's original node.
	 * @param targets ia assigned the set of anchor nodes of \p tgt's original node.
	 */
	void findSourcesAndTargets(
		node src, node tgt,
		NodeSet<> &sources,
		NodeSet<> &targets) const;

	/**
	 * \brief Returns all anchor nodes of \p vOrig in n\p nodes.
	 *
	 * @param vOrig is a node in the original graph.
	 * @param nodes ia assigned the set of anchor nodes.
	 */
	void anchorNodes(
		node vOrig,
		NodeSet<> &nodes) const;

	static node commonDummy(
		NodeSet<> &sources,
		NodeSet<> &targets);

	/**
	 * \brief Computes insertion path \p eip.
	 *
	 * The possible start and end nodes of the insertion path have to be stored in
	 * #m_pSources and #m_pTargets.
	 * @param eip    is assigned the insertion path (the crossed edges).
	 * @param vStart is assigned the start point of the insertion path.
	 * @param vEnd   is assigned the end point of the insertion path.
	 */
	void insert(List<Crossing> &eip, AnchorNodeInfo &vStart, AnchorNodeInfo &vEnd);

	node prepareAnchorNode(
		const AnchorNodeInfo &anchor,
		node vOrig,
		bool isSrc,
		edge &eExtra);

	void preprocessInsertionPath(
		const AnchorNodeInfo &srcInfo,
		const AnchorNodeInfo &tgtInfo,
		node srcOrig,
		node tgtOrig,
		node &src,
		node &tgt,
		edge &eSrc,
		edge &eTgt);

	node preparePath(
		node vAnchor,
		adjEntry adjPath,
		bool bOrigEdge,
		node vOrig);

	void findPseudos(
		node vDummy,
		adjEntry adjSrc,
		AnchorNodeInfo &infoSrc,
		SListPure<node> &pseudos);

	void insertWithCommonDummy(
		edge eOrig,
		node vDummy,
		node &src,
		node &tgt);

	/**
	 * \brief Implements vertex case of recursive path search in BC-tree.
	 *
	 * @param v      is the node in the graph currently visited during BC-tree traversal.
	 * @param parent is the parent block in DFS-traversal.
	 * @param eip is (step-by-step) assigned the insertion path (crossed edges).
	 * @param vStart is assigned the start point of \p eip.
	 * @param vEnd   is assigned the end point of \p eip.
	 */
	bool dfsVertex(node v,
		int parent,
		List<Crossing> &eip,
		AnchorNodeInfo &vStart,
		AnchorNodeInfo &vEnd);

	/**
	 * \brief Implements block case of recursive path search in BC-tree.
	 *
	 * @param i is the block in the graph currently visited during BC-tree traversal.
	 * @param parent is the parent node in DFS-traversal.
	 * @param repS is assigned the representative (nodein the graph) of a source node.
	 * @param eip is (step-by-step) assigned the insertion path (crossed edges).
	 * @param vStart is assigned the start point of \p eip.
	 * @param vEnd   is assigned the end point of \p eip.
	 */
	bool dfsBlock(int i,
		node parent,
		node &repS,
		List<Crossing> &eip,
		AnchorNodeInfo &vStart,
		AnchorNodeInfo &vEnd);

	bool pathSearch(node v, edge parent, const Block &BC, List<edge> &path);

	/**
	 * \brief Computes optimal insertion path in block \p BC.
	 *
	 * @param BC      is the block.
	 * @param L       is assigned the insertion path (the crossed edges).
	 * @param srcInfo is assigned the start point of the insertion path.
	 * @param tgtInfo is assigned the end point of the insertion path.
	 */
	void blockInsert(
		Block &BC,
		List<Crossing> &L,
		AnchorNodeInfo &srcInfo,
		AnchorNodeInfo &tgtInfo);

	void buildSubpath(
		node v,
		edge eIn,
		edge eOut,
		Paths &paths,
		bool &bPathToEdge,
		bool &bPathToSrc,
		bool &bPathToTgt,
		ExpandedSkeleton &Exp);

	void contractSplitIfReq(node u);
	void convertDummy(
		node u,
		node vOrig,
		PlanRepExpansion::nodeSplit ns_0);

	void writeEip(const List<Crossing> &eip);

	RemoveReinsertType m_rrOption; //!< The remove-reinsert option.
	double m_percentMostCrossed;   //!< The percentMostCrossed option.

	PlanRepExpansion *m_pPG; //!< Pointer to the planarized expansion.

	NodeSet<> *m_pSources; //!< The set of possible start nodes of an insertion path.
	NodeSet<> *m_pTargets; //!< The set of possible end nodes of an insertion path.

	NodeArray<SList<int> > m_compV; //!< The list of blocks containing a node \a v.
	Array<SList<node> >    m_nodeB; //!< The list of nodes in block \a i.
	Array<SList<edge> >    m_edgeB; //!< The list of edges in block \a i.
	NodeArray<node>        m_GtoBC; //!< Maps a node in the planarized expansion to the corresponding node in block.

	bool m_conFinished; //!< Stores if a possible target node in a block has already been found.
	const EdgeArray<bool> *m_forbiddenEdgeOrig;
};

}
