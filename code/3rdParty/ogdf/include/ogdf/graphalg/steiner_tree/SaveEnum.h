/** \file
 * \brief Implementation of the staticTree option for calculating
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

#include <ogdf/basic/Queue.h>
#include <ogdf/basic/HashArray.h>
#include <ogdf/graphalg/steiner_tree/Triple.h>
#include <ogdf/graphalg/steiner_tree/Save.h>
#include <ogdf/graphalg/steiner_tree/common_algorithms.h>

namespace ogdf {
namespace steiner_tree {

/*!
 * \brief This class computes save edges recursively and stores for every node pair their save edge in a HashArray.
 */
template<typename T>
class SaveEnum: public Save<T> {

public:
	/*!
	 * \brief Initializes the data structures and calculates a MST of the given complete terminal graph
	 * @param steinerTree the given terminal spanning tree
	 */
	explicit SaveEnum(EdgeWeightedGraphCopy<T> &steinerTree)
	  : Save<T>()
	  , m_save(0, steinerTree.maxNodeIndex(), 0, steinerTree.maxNodeIndex())
	  , m_steinerTree(&steinerTree)
	{
		build();
	}

	virtual ~SaveEnum()
	{
	}

	/*!
	 * \brief Determines the weight of the save edge between two nodes by a table lookup
	 * @param u First terminal
	 * @param v Second terminal
	 * @return Weight of the save edge between two given nodes
	 */
	virtual T saveWeight(node u, node v) const
	{
		OGDF_ASSERT(u);
		OGDF_ASSERT(v);
		return m_steinerTree->weight(m_save(m_steinerTree->copy(u)->index(), m_steinerTree->copy(v)->index()));
	}

	/*!
	 * \brief Determines the save edge between two nodes by a table lookup
	 * @param u First terminal
	 * @param v Second terminal
	 * @return The save edge between two given nodes
	 */
	virtual edge saveEdge(node u, node v) const
	{
		OGDF_ASSERT(u);
		OGDF_ASSERT(v);
		return m_save(m_steinerTree->copy(u)->index(), m_steinerTree->copy(v)->index());
	}

	/*!
	 * \brief Returns the gain (sum of the save edges) of a node triple by an table lookup
	 * @param u First triple node
	 * @param v Second triple node
	 * @param w Third triple node
	 * @return Sum of the save edges between the three nodes
	 */
	virtual T gain(node u, node v, node w) const
	{
		const int uIndex = m_steinerTree->copy(u)->index();
		const int vIndex = m_steinerTree->copy(v)->index();
		const int wIndex = m_steinerTree->copy(w)->index();
		const edge uvSave = m_save(uIndex, vIndex);
		const edge vwSave = m_save(vIndex, wIndex);

		if (uvSave == vwSave) {
			return m_steinerTree->weight(uvSave) + m_steinerTree->weight(m_save(uIndex, wIndex));
		}
		return m_steinerTree->weight(uvSave) + m_steinerTree->weight(vwSave);
	}

	/*!
	 * \brief Rebuild the lookup table (necessary if the tree has changed)
	 */
	void rebuild()
	{
		m_save.init(0, m_steinerTree->maxNodeIndex(), 0, m_steinerTree->maxNodeIndex());
		build();
	}

	/*!
	 * \brief Updates the weighted tree data structure given a contracted triple
	 *
	 * The update first inserts two 0-cost edges into the complete terminal graph and removes
	 * the two save edges. Afterward the lookup table is rebuild.
	 * @param t The contracted triple
	 */
	virtual void update(const Triple<T> &t)
	{
		const int uIndex = m_steinerTree->copy(t.s0())->index();
		const int vIndex = m_steinerTree->copy(t.s1())->index();
		const int wIndex = m_steinerTree->copy(t.s2())->index();
		contractTripleInSteinerTree(t, *m_steinerTree, m_save(uIndex, vIndex), m_save(vIndex, wIndex), m_save(uIndex, wIndex));
		build();
	}


protected:

	/*!
	 * \brief Build the lookup table
	 */
	inline void build()
	{
		m_save.fill(nullptr);
		EdgeArray<bool> hidden(*m_steinerTree, false);
		List<node> processedNodes;
		buildRecursively(hidden, m_steinerTree->firstNode(), processedNodes);
	}

	/*!
	 * \brief Builds the lookup table for the save edges recursively.
	 *
	 * This is done by first finding the maximum weighted edge in the graph. This edge
	 * partitions the graph and is the save edge for each pair of nodes such that not both
	 * of the nodes come from the same partition. This procedure is then applied to both
	 * partitions recursively.
	 *
	 * @param hidden Bool array of hidden edges
	 * @param u Starting node for traversing a partition in order to find a maximum weighted edge
	 * @param processedNodes List of seen nodes during the traversing (all nodes of the component)
	 */
	void buildRecursively(EdgeArray<bool> &hidden, node u, List<node> &processedNodes)
	{
		Queue<node> q;
		q.append(u);

		NodeArray<bool> processed(*m_steinerTree, false);
		processed[u] = true;

		// traverse through tree and find heaviest edge
		T maxWeight = -1;
		edge maxE = nullptr;
		while (!q.empty()) {
			const node v = q.pop();
			processedNodes.pushBack(v);
			for (adjEntry adj : v->adjEntries) {
				const edge e = adj->theEdge();
				if (!hidden[e]) {
					const node w = adj->twinNode();
					if (!processed[w]) {
						q.append(w);
						processed[w] = true;
						if (m_steinerTree->weight(e) > maxWeight) {
							maxE = e;
							maxWeight = m_steinerTree->weight(e);
						}
					}
				}
			}
		}

		if (maxE) {
			OGDF_ASSERT(maxWeight > -1);

			hidden[maxE] = true;

			List<node> processedNodes1;
			buildRecursively(hidden, maxE->source(), processedNodes1);
			List<node> processedNodes2;
			buildRecursively(hidden, maxE->target(), processedNodes2);

			for (node f : processedNodes1) {
				for (node g : processedNodes2) {
					m_save(f->index(), g->index()) = maxE;
					m_save(g->index(), f->index()) = maxE;
				}
			}
		}
	}

private:
	Array2D<edge> m_save; //!< Data structure for the lookup table
	EdgeWeightedGraphCopy<T> *m_steinerTree; //!< The current terminal spanning tree

};

}
}
