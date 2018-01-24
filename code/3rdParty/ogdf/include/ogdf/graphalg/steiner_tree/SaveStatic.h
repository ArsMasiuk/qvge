/** \file
 * \brief Implementation of the staticLCATree option for calculating
 *        save edges in Zelikovsky's 11/6-approximation
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
 * \brief This class behaves basically the same as SaveDynamic except
 *  that the update of the weighted graph is not done dynamically here
 */
template<typename T>
class SaveStatic: public Save<T> {
public:
	explicit SaveStatic(EdgeWeightedGraphCopy<T> &steinerTree)
	  : Save<T>()
	  , m_tree()
	  , m_treeEdge(m_tree, nullptr)
	  , m_steinerTree(&steinerTree)
	  , m_cTerminals(*m_steinerTree, nullptr)
	{
		m_root = buildHeaviestEdgeInComponentTree(*m_steinerTree, m_cTerminals, m_treeEdge, m_tree);
		m_lca = new LCA(m_tree, m_root);
	}

	virtual ~SaveStatic()
	{
		delete m_lca;
	}

	/*!
	 * \brief Determines the weight of the save edge between two nodes by a LCA query
	 * @param u First node
	 * @param v First node
	 * @return Weight of the save edge between two given nodes
	 */
	virtual T saveWeight(node u, node v) const
	{
		return m_steinerTree->weight(saveEdge(u, v));
	}

	/*!
	 * \brief Determines the save edge between two nodes by a LCA query
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
	 * \brief Returns the gain (sum of the save edges) of a node triple by an LCA query
	 * @param u First triple node
	 * @param v Second triple node
	 * @param w Third triple node
	 * @return Sum of the save edges between the three nodes
	 */
	virtual T gain(node u, node v, node w) const
	{
		node save0 = lca(m_steinerTree->copy(u), m_steinerTree->copy(v));
		node save1 = lca(m_steinerTree->copy(u), m_steinerTree->copy(w));
		if (save0 == save1) {
			save1 = lca(m_steinerTree->copy(v), m_steinerTree->copy(w));
		}
		return (save0 ? m_steinerTree->weight(m_treeEdge[save0]) : 0)
		     + (save1 ? m_steinerTree->weight(m_treeEdge[save1]) : 0);
	}

	/*!
	 * \brief Rebuild the data structure (necessary if the tree has changed)
	 */
	void rebuild()
	{
		m_tree.clear();
		m_cTerminals.fill(nullptr);
		m_root = buildHeaviestEdgeInComponentTree(*m_steinerTree, m_cTerminals, m_treeEdge, m_tree);
		delete m_lca;
		m_lca = new LCA(m_tree, m_root);
	}

	/*!
	 * \brief Updates the weighted tree data structure given a contracted triple
	 *
	 * The update first identifies the save edges and removes them. After removing links between the triple
	 * terminals and replacing them with 0-cost edges a new weighted tree is created and the LCA data
	 * structure is rebuilt as well.
	 * @param t The contracted triple
	 */
	virtual void update(const Triple<T> &t)
	{
		const edge save0 = saveEdge(t.s0(), t.s1());
		const edge save1 = saveEdge(t.s0(), t.s2());
		const edge save2 = saveEdge(t.s1(), t.s2());
		contractTripleInSteinerTree(t, *m_steinerTree, save0, save1, save2);

		rebuild();
	}

	/*!
	 * \brief Returns the node corresponding to the LCA between two given nodes.
	 * @param u First node
	 * @param v Second node
	 * @return The LCA of u and v
	 */
	node lca(node u, node v) const
	{
		return m_lca->call(m_cTerminals[u], m_cTerminals[v]);
	}

protected:

private:
	Graph m_tree; //!< The weighted binary tree to represent the edge weight hierarchy
	NodeArray<edge> m_treeEdge; //!< Maps each inner node of m_tree to an edge in m_steinerTree
	EdgeWeightedGraphCopy<T> *m_steinerTree; //!< A pointer to the tree we represent the save for
	node m_root; //!< The root of m_tree
	LCA *m_lca; //!< The LCA data structure for m_tree
	NodeArray<node> m_cTerminals;
};

}
}
