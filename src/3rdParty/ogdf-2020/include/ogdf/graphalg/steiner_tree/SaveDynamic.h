/** \file
 * \brief A weighted tree as auxiliary data structure for
 * contraction based algorithms
 *
 * \author Matthias Woste, Stephan Beyer
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

#include <ogdf/tree/LCA.h>
#include <ogdf/graphalg/steiner_tree/Save.h>
#include <ogdf/graphalg/steiner_tree/Triple.h>
#include <ogdf/graphalg/steiner_tree/common_algorithms.h>

namespace ogdf {
namespace steiner_tree {

/*!
 * \brief Dynamically updatable weighted Tree for determining save edges via LCA computation.
 *  Note that in this dynamic approach, only the auxiliary tree is updated and not
 *  the actual terminal spanning tree (if OGDF_SAVEDYNAMIC_CHANGE_TST is not set).
 */
//#define OGDF_SAVEDYNAMIC_CHANGE_TST
template<typename T>
class SaveDynamic: public Save<T> {
public:
	/*!
	 * \brief Builds a weighted binary tree based on the given terminal spanning tree.
	 *
	 * Additionally the LCA data structure is initialized.
	 * @param steinerTree the given terminal spanning tree
	 */
	explicit SaveDynamic(EdgeWeightedGraphCopy<T> &steinerTree)
	  : Save<T>()
	  , m_tree()
	  , m_treeEdge(m_tree, nullptr)
	  , m_steinerTree(&steinerTree)
	  , m_cTerminals(*m_steinerTree, nullptr)
	{
		m_root = buildHeaviestEdgeInComponentTree(*m_steinerTree, m_cTerminals, m_treeEdge, m_tree);
		m_lca = new LCA(m_tree, m_root);
	}

	virtual ~SaveDynamic()
	{
		delete m_lca;
	}

	/*!
	 * \brief Returns the gain (sum of save edge costs) of the given triple, calculated by an LCA query
	 *
	 * @param u First terminal
	 * @param v Second terminal
	 * @param w Third terminal
	 * @return The gain (sum of save edge costs) of the given triple
	 */
	virtual T gain(node u, node v, node w) const
	{
		node save1 = lca(m_steinerTree->copy(u), m_steinerTree->copy(v));
		node save2 = lca(m_steinerTree->copy(u), m_steinerTree->copy(w));
		if (save1 == save2) {
			save2 = lca(m_steinerTree->copy(v), m_steinerTree->copy(w));
		}
		return weight(save1) + weight(save2);
	}

	/*!
	 * \brief Determines the weight of the save edge between two nodes by a LCA query
	 *
	 * @param u First node
	 * @param v First node
	 * @return Weight of the save edge between two given nodes
	 */
	virtual T saveWeight(node u, node v) const
	{
		return weight(saveEdge(u, v));
	}

	/*!
	 * \brief Determines the save edge between two nodes by a LCA query
	 *
	 * @param u First node
	 * @param v First node
	 * @return The save edge between two given nodes
	 */
	virtual edge saveEdge(node u, node v) const
	{
		OGDF_ASSERT(m_steinerTree->copy(u));
		OGDF_ASSERT(m_steinerTree->copy(v));
		node anc = lca(m_steinerTree->copy(u), m_steinerTree->copy(v));
		OGDF_ASSERT(anc);
		return m_treeEdge[anc];
	}

	/*!
	 * \brief Updates the weighted tree data structure given a contracted triple
	 *
	 * The update of the weighted tree is performed dynamically. To achieve this,
	 * the weighted tree is traversed bottom-up, starting at the three leaves corresponding
	 * to the terminals in the triple. It takes time O(height of m_tree).
	 * @param t The contracted triple
	 */
	virtual void update(const Triple<T> &t)
	{
#ifdef OGDF_SAVEDYNAMIC_CHANGE_TST
		edge se0 = saveEdge(t.s0(), t.s1());
		edge se1 = saveEdge(t.s0(), t.s2());
		edge se2 = saveEdge(t.s1(), t.s2());
#endif
		// initialize the three terminal nodes
		node s0 = m_steinerTree->copy(t.s0());
		node s1 = m_steinerTree->copy(t.s1());
		node s2 = m_steinerTree->copy(t.s2());

		// determine the save edges
		node save1 = lca(s0, s1);
		node save2 = lca(s0, s2);
		if (save1 == save2) {
			save2 = lca(s1, s2);
		}

		node v0 = m_cTerminals[s0];
		node v1 = m_cTerminals[s1];
		node v2 = m_cTerminals[s2];

		int v0level = m_lca->level(v0);
		int v1level = m_lca->level(v1);
		int v2level = m_lca->level(v2);

		delete m_lca;

		node v = m_tree.newNode();
		node currentNode = m_tree.newNode();
#ifdef OGDF_SAVEDYNAMIC_CHANGE_TST
		const node newNode0 = v, newNode1 = currentNode;
#endif
		OGDF_ASSERT(!m_treeEdge[currentNode]);
		m_tree.newEdge(currentNode, v);
		m_cTerminals[s0] = v;
		m_cTerminals[s1] = v;
		m_cTerminals[s2] = currentNode;

		while (v0) {
			// bubble pointers such that level(v0) >= level(v1) >= level(v2)
			if (v1level < v2level) {
				std::swap(v1, v2);
				std::swap(v1level, v2level);
			}
			if (v0level < v1level) {
				std::swap(v0, v1);
				std::swap(v0level, v1level);
			}
			if (v1level < v2level) {
				std::swap(v1, v2);
				std::swap(v1level, v2level);
			}
			// bubble pointers such that weight(v0) <= weight(v1), weight(v2)
			if (weight(v1) > weight(v2)) {
				std::swap(v1, v2);
				std::swap(v1level, v2level);
			}
			if (weight(v0) > weight(v1)) {
				std::swap(v0, v1);
				std::swap(v0level, v1level);
			}
			// now v0 is the node with the least weight... if equal, with the highest level.

			if (v0 != save1
			 && v0 != save2) {
				for(adjEntry adj : currentNode->adjEntries) {
					edge e = adj->theEdge();
					if (e->target() == currentNode) {
						m_tree.delEdge(e);
						break;
					}
				}
				m_tree.newEdge(v0, currentNode);
				currentNode = v0;
			} // else: nothing to do since m_tree is binary and save1/save2 are LCAs

			// set v0 to its parent or to nullptr if there is no parent
			v = v0;
			v0 = nullptr;
			edge e = nullptr;
			for(adjEntry adj : v->adjEntries) {
				e = adj->theEdge();
				if (e->target() == v) {
					v0 = e->source();
					--v0level;
					break;
				}
			}
			OGDF_ASSERT(e != nullptr);
			if (v1 == e->target()) {
				v1 = e->source();
				--v1level;
			}
			if (v2 == e->target()) {
				v2 = e->source();
				--v2level;
			}
		}
		m_root = currentNode;
		m_tree.delNode(save1);
		m_tree.delNode(save2);

		m_lca = new LCA(m_tree, m_root);

#ifdef OGDF_SAVEDYNAMIC_CHANGE_TST
		edge newEdge0, newEdge1;
		contractTripleInSteinerTree(t, *m_steinerTree, se0, se1, se2, newEdge0, newEdge1);
		m_treeEdge[newNode0] = newEdge0;
		m_treeEdge[newNode1] = newEdge1;
#endif
	}

protected:

	/*!
	 * \brief Returns the node in m_tree that is the LCA of two nodes
	 *
	 * @param u first node
	 * @param v second node
	 * @return the LCA of u and v
	 */
	node lca(node u, node v) const
	{
		OGDF_ASSERT(u);
		OGDF_ASSERT(v);
		return m_lca->call(m_cTerminals[u], m_cTerminals[v]);
	}

	//! Returns the weight of an edge in the terminal tree or 0
	T weight(edge e) const
	{
		return e ? m_steinerTree->weight(e) : 0;
	}

	//! Returns the associated weight of a node v in m_tree, or 0 if it is not associated.
	T weight(node v) const
	{
		return weight(m_treeEdge[v]);
	}

private:
	Graph m_tree; //!< The weighted binary tree to represent the edge weight hierarchy
	NodeArray<edge> m_treeEdge; //!< Maps each inner node of m_tree to an edge in m_steinerTree
	node m_root; //!< The root node of the weighted binary tree
#ifndef OGDF_SAVEDYNAMIC_CHANGE_TST
	const
#endif
	EdgeWeightedGraphCopy<T> *m_steinerTree; //!< The underlying terminal spanning tree this weighted tree instance represents
	NodeArray<node> m_cTerminals; //!< Connects terminal nodes in the terminal spanning tree to their leafs in the weighted tree
	LCA *m_lca; //!< Data structure for calculating the LCAs
};

}
}
