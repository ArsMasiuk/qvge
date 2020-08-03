/** \file
 * \brief Declaration of ogdf::PlanarAugmentation
 *
 * \author Bernd Zey
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

#include <ogdf/augmentation/AugmentationModule.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/augmentation/planar/PALabel.h>
#include <ogdf/decomposition/DynamicBCTree.h>

namespace ogdf {

/**
 * The algorithm for biconnectivity augmentation with fixed combinatorial embedding.
 *
 * @ingroup ga-augment
 */
class OGDF_EXPORT PlanarAugmentationFix : public AugmentationModule {

public:
	//! Creates an instance of planar augmentation with fixed embedding.
	PlanarAugmentationFix() { }

	//! Destruction
	~PlanarAugmentationFix() { }

protected:
	/**
	 * The implementation of the algorithm call.
	 *
	 * \param g is the working graph.
	 * \param list is the list of all new edges.
	 */
	virtual void doCall(Graph& g, List<edge>& list) override;

private:
	//! The embedding of #m_pGraph.
	CombinatorialEmbedding* m_pEmbedding = nullptr;

	//! The embedding of #m_graphCopy.
	CombinatorialEmbedding* m_pActEmbedding = nullptr;

	//! The working graph.
	Graph* m_pGraph = nullptr;

	//! The inserted edges by the algorithm.
	List<edge>* m_pResult = nullptr;

	//! The actual dynamic bc-tree.
	DynamicBCTree* m_pBCTree = nullptr;

	//! The actual partial graph.
	GraphCopy m_graphCopy;

	//! Edge-array required for construction of the graph copy.
	EdgeArray<edge> m_eCopy;

	//! The list of all labels.
	List<pa_label> m_labels;

	//! Array that contains iterators to the list of labels if a node is a parent of a label.
	NodeArray<ListIterator<pa_label>> m_isLabel;

	//! Array that contains the label a node belongs to.
	NodeArray<pa_label> m_belongsTo;

	//! Array that contains the iterator of the label a node belongs to.
	NodeArray<ListIterator<node>> m_belongsToIt;

	//! The actual root of the bc-tree.
	node m_actBCRoot;

	//! The main function for planar augmentation.
	void augment(adjEntry adjOuterFace);

	//! Modifies the root of the bc-tree.
	void modifyBCRoot(node oldRoot, node newRoot);

	//! Exchanges \p oldRoot by \p newRoot and updates data structurs in the bc-tree.
	void changeBCRoot(node oldRoot, node newRoot);

	//! Adds \p pendant to a label or creates one.
	void reduceChain(node pendant);

	//! Traverses upwards in the bc-tree, starting at the pendant node \p v.
	PALabel::StopCause followPath(node v, node& last);

	//! Finds the next matching pendants.
	bool findMatching(node& pendant1, node& pendant2, adjEntry& v1, adjEntry& v2);

	//! Called by findMatching, if a dominating tree was detected.
	void findMatchingRev(node& pendant1, node& pendant2, adjEntry& v1, adjEntry& v2);

	//! Creates a new label.
	pa_label newLabel(node cutvertex, node parent, node pendant, PALabel::StopCause whyStop);

	//! Adds \p pendant to \p label.
	void addPendant(node pendant, pa_label& label);

	//! Inserts \p label into the list of labels maintaining decreasing order.
	ListIterator<pa_label> insertLabel(pa_label label);

	//! Connects the two pendants.
	void connectPendants(node pendant1, node pendant2, adjEntry adjV1, adjEntry adjV2);

	//! Connects the remaining label.
	void connectSingleLabel();

	//! Deletes the given pendant.
	void deletePendant(node pendant);

	//! Deletes the given label.
	void deleteLabel(pa_label& label);

	//! Removes the given label from the list of labels.
	void removeLabel(pa_label& label);
};

}
