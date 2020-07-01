/** \file
 * \brief Declaration of upward planarization layout algorithm.
 *
 * \author Hoi-Ming Wong
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

#include <memory>
#include <ogdf/upward/UpwardPlanRep.h>
#include <ogdf/layered/RankingModule.h>
#include <ogdf/upward/UPRLayoutModule.h>
#include <ogdf/layered/HierarchyLayoutModule.h>
#include <ogdf/layered/OptimalHierarchyLayout.h>
#include <ogdf/layered/FastHierarchyLayout.h>
#include <ogdf/layered/OptimalRanking.h>
#include <ogdf/layered/HierarchyLevels.h>

namespace ogdf {

class OrderComparer
{
public:
	OrderComparer(const UpwardPlanRep &_UPR, Hierarchy &_H);

	/**
	 * Returns true iff \p vH1 and \p vH2 are placed on the same layer and
	 * node \p vH1 has to drawn on the left-hand side of \p vH2 (according
	 * to #m_UPR)
	 */
	bool less(node vH1, node vH2) const;

private:
	const UpwardPlanRep &m_UPR;
	Hierarchy &H;
	NodeArray<int> m_dfsNum;
#if 0
	EdgeArray<int> outEdgeOrder;
#endif
	mutable NodeArray<bool> crossed;

	//! Traverses with dfs using edge order from left to right and compute the dfs number.
	void dfs_LR(edge e,
	            NodeArray<bool> &visited,
	            NodeArray<int> &dfsNum,
	            int &num);

	//! Returns true if \p vUPR1 is on the left-hand side of \p vUPR2 according to #m_UPR.
	bool left(node vUPR1, //!< the node that is tested to be on the left-hand side
	          const List<edge> &chain1, //!< if \p vUPR1 is associated with a long edge dummy vH1, then \p chain1 contain vH1
	          node vUPR2, //!< the other node
	          const List<edge> &chain2 //!< if \p vUPR2 is associated with a long edge dummy vH2, then \p chain2 contain vH2
	          ) const;

	/**
	 * Returns true iff \p vUPR1 is on the left-hand side of \p vUPR2
	 * according to #m_UPR.
	 *
	 * @pre source or target of both edges must be identical
	 */
	bool left(edge e1UPR, edge e2UPR) const;

	/**
	 * Returns true iff \p vUPR1 is on the left-hand side of \p vUPR2
	 * according to #m_UPR.
	 * Used only by method #less for the case when both node \a vH1 and
	 * \a vH2 are long-edge dummies, where \p level is the current level
	 * of the long-edge dummies
	 */
	bool left(List<edge> &chain1, List<edge> &chain2, int level) const;

	//! Returns true iff there is a node above \p vUPR with rank \p level or lower
	bool checkUp(node vUPR, int level) const;
};


/**
 @ingroup gd-layered
 */
class OGDF_EXPORT LayerBasedUPRLayout : public UPRLayoutModule
{
public:

	// constructor: sets options to default values
	LayerBasedUPRLayout()
	{
		// set default value
		FastHierarchyLayout *fhl = new FastHierarchyLayout();
		fhl->nodeDistance(40.0);
		fhl->layerDistance(40.0);
		fhl->fixedLayerDistance(true);
		m_layout.reset(fhl);
		OptimalRanking *opRank = new OptimalRanking();
		opRank->separateMultiEdges(false);
		m_ranking.reset(opRank);
		m_numLevels = 0;
		m_maxLevelSize = 0;
	}

	// destructor
	~LayerBasedUPRLayout() { }

	// returns the number of crossings in the layout after the algorithm
	// has been applied
	int numberOfCrossings() const { return m_crossings; }

	// module option for the computation of the final layout
	void setLayout(HierarchyLayoutModule *pLayout) {
		m_layout.reset(pLayout);
	}


	void setRanking(RankingModule *pRanking) {
		m_ranking.reset(pRanking);
	}

	//! Use only the 3. phase of Sugiyama' framework for layout.
	void UPRLayoutSimple(const UpwardPlanRep &UPR, GraphAttributes &AG);

	//! Return the number of layers/levels. Not implemented if use methode callSimple(..).
	int numberOfLayers() { return m_numLevels; }

	//! Return the max. number of elements on a layer. Not implemented if use methode callSimple(..).
	int maxLayerSize() { return m_maxLevelSize; }

protected :

	/*
	 * @param UPR is the upward planarized representation of the input graph.
	 * @param AG has to be assigned the hierarchy layout.
	 */
	virtual void doCall(const UpwardPlanRep &UPR, GraphAttributes &AG) override;

	int m_crossings;

	std::unique_ptr<RankingModule> m_ranking;

	std::unique_ptr<HierarchyLayoutModule> m_layout;

private:

	// compute a ranking of the nodes of UPR.
	// Precond. a ranking module muss be set
	void computeRanking(const UpwardPlanRep &UPR, NodeArray<int> &rank);


	//! rearanging the position of the sources in order to reduce some crossings.
	void postProcessing_sourceReorder(HierarchyLevels &levels, List<node> &sources);


	//! reduce the long edge dummies (LED)
	void postProcessing_reduceLED(Hierarchy &H, HierarchyLevels &levels, const List<node> &sources) {
		for(node s : sources)
			postProcessing_reduceLED(H, levels, s);
	}

	void postProcessing_reduceLED(Hierarchy &H, HierarchyLevels &levels, node vH);

	void post_processing_reduce(Hierarchy &H, HierarchyLevels &levels, int &i, node s, int minIdx, int maxIdx, NodeArray<bool> &markedNodes);

	//! mark all the nodes dominated by sH.	(Help method for postProcessing_reduceLED() )
	void postProcessing_markUp(HierarchyLevels &levels, node sH, NodeArray<bool> &markedNodes);


	//! delete level i of H.
	void post_processing_deleteLvl(Hierarchy &H, HierarchyLevels &levels, int i);

	//! delete the interval [beginIdx,endIdx] on the level j.
	void post_processing_deleteInterval(Hierarchy &H, HierarchyLevels &levels, int beginIdx, int endIdx, int &j);

	//! insert the interval  [beginIdx,endIdx] of level i-1 to level i at position pos.
	void post_processing_CopyInterval(Hierarchy &H, HierarchyLevels &levels, int i, int beginIdx, int endIdx, int pos);

	int m_numLevels;
	int m_maxLevelSize;
	ArrayBuffer<node> m_dummies;


	//! \name UPRLayoutSimple methods
	//! @{

	void callSimple(GraphAttributes &AG, adjEntry adj //left most edge of the source
					);

	// needed for UPRLayoutSimple
	void dfsSortLevels(
		adjEntry adj1,
		const NodeArray<int> &rank,
		Array<SListPure<node> > &nodes);

	// needed for UPRLayoutSimple
	void longestPathRanking(const Graph &G, NodeArray<int> &rank);

	//! @}
};

}
