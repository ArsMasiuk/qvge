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
#include <ogdf/basic/SList.h>
#include <ogdf/augmentation/planar/PALabel.h>
#include <ogdf/decomposition/DynamicBCTree.h>

namespace ogdf {

/**
 * The algorithm for planar biconnectivity augmentation (Mutzel, Fialko).
 *
 * @ingroup ga-augment
 *
 * The class PlanarAugmentation implements an augmentation algorithm
 * that augments a graph to a biconnected graph. In addition, if the graph was
 * planar before augmentation, the resulting graph will be biconnected and
 * planar.
 * The algorithm uses (dynamic) BC-trees and achieves biconnectivity by
 * inserting edges between nodes of pendants (that are leaves in the bc-tree).
 * The guaranteed approximation-quality is 5/3.
 *
 * The implementation is based on the following publication:
 *
 * Sergej Fialko, Petra Mutzel: <i>A New Approximation Algorithm for the Planar
 * Augmentation Problem</i>. Proc. SODA 1998, pp. 260-269.
 */
class OGDF_EXPORT PlanarAugmentation : public AugmentationModule {

public:
	//! Creates an instance of the planar augmentation algorithm.
	PlanarAugmentation() { }

	//! Destruction
	~PlanarAugmentation() { }

protected:
	/**
	 * The implementation of the algorithm call.
	 *
	 * \param G is the working graph.
	 * \param list is the list of all new edges.
	 */
	virtual void doCall(Graph& G, List<edge>& list) override;


private:
	//! The number of planarity tests.
	int m_nPlanarityTests = 0;

	//! The working graph.
	Graph* m_pGraph = nullptr;

	//! The corresponding BC-Tree.
	DynamicBCTree* m_pBCTree = nullptr;

	//! The inserted edges by the algorithm.
	List<edge>* m_pResult = nullptr;

	//! The list of all labels, sorted by size (decreasing).
	List<pa_label> m_labels;

	//! The list of all pendants (leaves in the BC-Tree).
	List<node> m_pendants;

	//! The list of pendants that has to be deleted after each reduceChain.
	List<node> m_pendantsToDel;

	//! The label a BC-Node belongs to.
	NodeArray<pa_label> m_belongsTo;

	//! The list iterator in #m_labels if the node in the BC-Tree is a label.
	NodeArray<ListIterator<pa_label>> m_isLabel;

	/**
	 * Stores for each node of the bc-tree the children that have an adjacent bc-node
	 * that doesn't belong to the same parent-node.
	 *
	 * This is necessary because the bc-tree uses an union-find-data-structure to store
	 * dependencies between bc-nodes. The adjacencies in the bc-tree won't be updated.
	 */
	NodeArray<SList<adjEntry>> m_adjNonChildren;

private:
	//! The main function for planar augmentation.
	void augment();

	//! Makes the graph connected by new edges between pendants of the connected components
	void makeConnectedByPendants();

	/**
	 * Traverses to the root and creates a label or updates one.
	 * Is called for every pendant node.
	 *
	 * \param pendant is a pendant in the BC-Tree.
	 * \param labelOld is the old label of \p pendant.
	 */
	void reduceChain(node pendant, pa_label labelOld = nullptr);

	/**
	 * Traverses to the root and checks several stop conditions.
	 * Is called by #reduceChain.
	 *
	 * \param v is a node of the BC-Tree.
	 * \param last is the last found C-vertex in the BC-Tree, is modified by
	 *        the method.
	 * \return the stop-cause.
	 */
	PALabel::StopCause followPath(node v, node& last);

	/**
	 * Checks planarity for a new edge (\p v1, \p v2) in the original graph.
	 *
	 * \param v1 is a node in the original graph.
	 * \param v2 is a node in the original graph.
	 * \return true iff the graph (including the new edge) is planar.
	 */
	bool planarityCheck(node v1, node v2);

	/**
	 * Returns a node that belongs to bc-node \p v and is adjacent to \p cutvertex.
	 *
	 * \param v is a node in the BC-Tree.
	 * \param cutvertex is the last cut-vertex found.
	 * \return a node of the original graph.
	 */
	node adjToCutvertex(node v, node cutvertex = nullptr);

	//! Traverses from \p pendant to \p ancestor and returns the last node before ancestor on the path.
	node findLastBefore(node pendant, node ancestor);

	//! Deletes the pendant \p pendant, and, if \p removeFromLabel is true,
	//! removes it from the corresponding label and updates the label-order.
	void deletePendant(node pendant, bool removeFromLabel = true);

	//! Adds \p pendant to \p label and updates the label-order.
	void addPendant(node pendant, pa_label& label);

	/**
	 * Connects two pendants.
	 *
	 * \return the new edge in the original graph.
	 */
	edge connectPendants(node pendant1, node pendant2);

	//! Removes all pendants of \p label.
	void removeAllPendants(pa_label& label);

	//! Connects all pendants of \p label with new edges.
	void joinPendants(pa_label& label);

	//! Connects the only pendant of \p label with a computed ancestor.
	void connectInsideLabel(pa_label& label);

	/**
	 * Inserts \p label into #m_labels by decreasing order.
	 *
	 * \return the corresponding list iterator.
	 */
	ListIterator<pa_label> insertLabel(pa_label label);

	//! Deletes \p label.
	void deleteLabel(pa_label& label, bool removePendants = true);

	/**
	 * Inserts edges between pendants of label \p first and \p second.
	 *
	 * @pre \c first.size() is greater than \c second.size() or equal.
	 */
	void connectLabels(pa_label first, pa_label second);

	//! Creates a new label and inserts it into #m_labels.
	pa_label newLabel(node cutvertex, node pendant, PALabel::StopCause whyStop);

	/**
	 * Finds two matching labels, so all pendants can be connected
	 * without losing planarity.
	 *
	 * \param first is the label with maximum size, modified by the function.
	 * \param second is the matching label, modified by the function:
	 *        0 if no matching is found.
	 * \return true iff a matching label is found.
	 */
	bool findMatching(pa_label& first, pa_label& second);

	//! Checks if the pendants of label \p a and label \p b can be connected without creating a new pendant.
	bool connectCondition(pa_label a, pa_label b);

	/**
	 * Updates #m_adjNonChildren.
	 *
	 * \param newBlock is a new created block of the BC-Tree.
	 * \param path is the path in the BC-Tree between the two connected nodes.
	 */
	void updateAdjNonChildren(node newBlock, SList<node>& path);

	//! Modifies the root of the BC-Tree that \p newRoot replaces \p oldRoot.
	void modifyBCRoot(node oldRoot, node newRoot);

	/**
	 * Major updates caused by the new edges.
	 *
	 * \param newEdges is a list of all new edges.
	 */
	void updateNewEdges(const SList<edge> &newEdges);

	//! Cleanup.
	void terminate();
};

}
