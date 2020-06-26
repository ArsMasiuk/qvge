/** \file
 * \brief Declares the class BitonicOrdering...
 *
 * ... that computes a bitonic st ordering as described by Gronemann in Bitonic st-orderings of biconnected planar graphs
 *
 * \author Martin Gronemann
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

#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/AdjEntryArray.h>
#include <ogdf/decomposition/StaticPlanarSPQRTree.h>
#include <ogdf/planarlayout/LeftistOrdering.h>
#include <sstream>


namespace ogdf {

class BitonicOrdering
{
public:
	// constructs a bitonic st ordering for G and embeds G accordingly.
	// Requires G to be planar, embedded, biconnected
	BitonicOrdering(Graph& G, adjEntry adj_st_edge);

#ifdef OGDF_DEBUG
	//! Asserts that this ordering is consistent.
	void consistencyCheck(GraphAttributes& GA) const;
#endif

	// returns the index in the st order of v
	int getIndex(node v) const
	{
		return m_orderIndex[v];
	}

	// returns the i-th node in the bitonic st order
	node getNode(int i) const
	{
		return m_indexToNode[i];
	}
private:

	// used to distinguish between the three cases below
	void handleCase(node v_T);

	// helper function that finds the st-adjEntry in the skeleton of v_T
	adjEntry getAdjST(node v_T) const;

	// the S-node case
	void handleSerialCase(node v_T);

	// the P-node case
	void handleParallelCase(node v_T);

	// transforms the result of the canonical ordering into two arrays,
	// one holding the index in the temporary order for a node,
	// the other is the reverse map. Function assumes proper init for indices and vertices
	void partitionToOrderIndices(const List<List<node> >& partitionlist,
	                             NodeArray<int>& indices,
	                             Array<node>& vertices) const;

	// the R-node case
	void handleRigidCase(node v_T);

	// label a new node
	void assignLabel(node v_T, node v)
	{
		// the real graph node
		node v_G = m_tree.skeleton(v_T).original(v);

		// set the label
		m_orderIndex[v_G] = m_currLabel++;

		// and update the other map
		m_indexToNode[m_orderIndex[v_G]] = v_G;
	}

	// returns the label of a node v in the skeleton of v_T
	int getLabel(node v_T, node v) const
	{
		// the real graph node
		node v_G = m_tree.skeleton(v_T).original(v);

		// return the index
		return m_orderIndex[v_G];
	}

	// mark a skeleton as temporarily flipped
	void setFlipped(node v_T, bool flag)
	{
		m_flipped[v_T] = flag;
	}

	// returns true if a skeleton is temporarily flipped
	bool isFlipped(node v_T) const
	{
		return m_flipped[v_T];
	}

	// the graph
	Graph& m_graph;

	// the curr label index
	int m_currLabel;

	// index in the order for all graph nodes
	NodeArray<int> m_orderIndex;

	// maps an index to the node
	Array<node> m_indexToNode;

	// flag for keeping track of if a node has been temporary flipped
	NodeArray<bool> m_flipped;

	// the SPQR tree
	StaticPlanarSPQRTree m_tree;
};

}
