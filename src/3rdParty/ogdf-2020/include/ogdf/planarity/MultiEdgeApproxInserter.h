/** \file
 * \brief Declaration of class MultiEdgeApproxInserter.
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

#include <ogdf/basic/FaceArray.h>
#include <ogdf/planarity/EdgeInsertionModule.h>
#include <ogdf/decomposition/StaticPlanarSPQRTree.h>
#include <ogdf/planarity/RemoveReinsertType.h>

namespace ogdf {

//! Multi edge inserter with approximation guarantee.
/**
 * @ingroup ga-insert
 *
 * The implementation is based on the following publication:
 *
 * Markus Chimani and Carsten Gutwenger: <i>Advances in the Planarization Method:
 * Effective Multiple Edge Insertions</i>. Journal of %Graph Algorithms and Applications (JGAA), 16(3), pp. 729-757, 2012.
 */
class OGDF_EXPORT MultiEdgeApproxInserter : public EdgeInsertionModule
{
public:
	//! Creates an instance of multi-edge approx inserter with default option settings.
	MultiEdgeApproxInserter();

	//! Creates an instance of multi-edge approx inserter with the same settings as \p inserter.
	MultiEdgeApproxInserter(const MultiEdgeApproxInserter &inserter);

	//! Destructor.
	~MultiEdgeApproxInserter() { }

	//! Returns a new instance of the multi-edge approx inserter with the same option settings.
	virtual EdgeInsertionModule *clone() const override;

	//! Assignment operator. Copies option settings only.
	MultiEdgeApproxInserter &operator=(const MultiEdgeApproxInserter &inserter);

	//! Sets the remove-reinsert postprocessing method.
	void removeReinsertFix(RemoveReinsertType rrOption) {
		m_rrOptionFix = rrOption;
	}

	//! Returns the current setting of the remove-reinsert postprocessing method.
	RemoveReinsertType removeReinsertFix() const {
		return m_rrOptionFix;
	}

	//! Sets the remove-reinsert postprocessing method.
	void removeReinsertVar(RemoveReinsertType rrOption) {
		m_rrOptionVar = rrOption;
	}

	//! Returns the current setting of the remove-reinsert postprocessing method.
	RemoveReinsertType removeReinsertVar() const {
		return m_rrOptionVar;
	}

	//! Sets the option <i>percentMostCrossed</i> to \p percent.
	/**
	 * This option determines the portion of most crossed edges used if the remove-reinsert
	 * method is set to RemoveReinsertType::MostCrossed. This portion is number of edges * percentMostCrossed() / 100.
	 */
	void percentMostCrossedFix(double percent) {
		m_percentMostCrossedFix = percent;
	}

	//! Returns the current setting of option percentMostCrossed.
	double percentMostCrossedFix() const {
		return m_percentMostCrossedFix;
	}

	//! Sets the option <i>percentMostCrossedVar</i> to \p percent.
	/**
	 * This option determines the portion of most crossed edges used if the remove-reinsert
	 * method (variable embedding) is set to RemoveReinsertType::MostCrossed. This portion is number of edges * percentMostCrossed() / 100.
	 */
	void percentMostCrossedVar(double percent) {
		m_percentMostCrossedVar = percent;
	}

	//! Returns the current setting of option percentMostCrossed (variable embedding).
	double percentMostCrossedVar() const {
		return m_percentMostCrossedVar;
	}

	void statistics(bool b) {
		m_statistics = b;
	}

	bool statistics() const {
		return m_statistics;
	}

	int sumInsertionCosts() const { return m_sumInsertionCosts; }
	int sumFEInsertionCosts() const { return m_sumFEInsertionCosts; }

private:
	enum class PathDir { Left, Right, None };

	//! Returns the opposite direction of \p dir.
	static inline PathDir oppDir(PathDir dir){
		switch (dir) {
			case PathDir::Left:
				return PathDir::Right;
			case PathDir::Right:
				return PathDir::Left;
			default:
				return PathDir::None;
		}
	};

	//! Maintains a block in the graph
	class Block;

	//! Encodes an embedding preference
	class EmbeddingPreference;

	struct VertexBlock {
		VertexBlock(node v, int b) : m_vertex(v), m_block(b) { }

		node m_vertex;
		int m_block;
	};

	//! Implements the algorithm call.
	ReturnType doCall(
		PlanRepLight              &pr,
		const Array<edge>         &origEdges,
		const EdgeArray<int>      *costOrig,
		const EdgeArray<bool>     *forbiddenEdge,
		const EdgeArray<uint32_t> *edgeSubGraphs) override;

	MultiEdgeApproxInserter::Block *constructBlock(int i);
	node copy(node vOrig, int b);

	static bool dfsPathSPQR(node v, node v2, edge eParent, List<edge> &path);
	int computePathSPQR(int b, node v, node w, int k);

	bool dfsPathVertex(node v, int parent, int k, node t);
	bool dfsPathBlock(int b, node parent, int k, node t);
	void computePathBC(int k);

	void embedBlock(int b, int m);
	void recFlipPref(adjEntry adjP, NodeArray<EmbeddingPreference> &pi_pick, const NodeArray<bool> &visited, StaticPlanarSPQRTree &spqr);

	void cleanup();

	RemoveReinsertType m_rrOptionFix; //!< The remove-reinsert method for fixed embedding.
	RemoveReinsertType m_rrOptionVar; //!< The remove-reinsert method for variable embedding.
	double m_percentMostCrossedFix;   //!< The portion of most crossed edges considered (fixed embedding).
	double m_percentMostCrossedVar;   //!< The portion of most crossed edges considered (variable embedding).
	bool m_statistics; // !< Generates further statistic information.

	PlanRepLight *m_pPG; // pointer to plan-rep passed in call
	const EdgeArray<int> *m_costOrig;

	NodeArray<SList<int> > m_compV;     //  m_compV[v] = list of blocks containing v
	Array<SList<node> >    m_verticesB; // m_verticesB[i] = list of vertices in block i
	Array<SList<edge> >    m_edgesB;    // edgesB[i] = list of edges in block i
	NodeArray<node>        m_GtoBC;     // temporary mapping of nodes in PG to copies in block
	NodeArray<SList<VertexBlock> > m_copyInBlocks; // mapping of nodes in PG to copies in block

	const Array<edge> *m_edge;           // pointer to array of edges to be inserted
	Array<List<VertexBlock> > m_pathBCs; // insertion path in BC-tree for each edge
	Array<int> m_insertionCosts;         // computed insertion costs for each edge
	Array<Block*> m_block;               // array of blocks

	// statistics of last run
	int m_sumInsertionCosts;
	int m_sumFEInsertionCosts;

	// just for testing
	void constructDual(const PlanRepLight &pr);
	int findShortestPath(node s, node t);

	ConstCombinatorialEmbedding m_E;
	Graph                       m_dual;
	FaceArray<node>             m_faceNode;
	AdjEntryArray<adjEntry>     m_primalAdj;
	node                        m_vS, m_vT;
};

}
