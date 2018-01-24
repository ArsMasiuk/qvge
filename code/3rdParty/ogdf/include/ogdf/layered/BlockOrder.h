/** \file
 * \brief Declaration of BlockOrder and related classes
 *
 * \author Paweł Schmidt
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

#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/layered/CrossingMinInterfaces.h>

namespace ogdf {

//! The simple implementation of LevelBase interface.
class ArrayLevel : public LevelBase {

private:
	Array<node> m_nodes;
public:

	explicit ArrayLevel(unsigned int size) : m_nodes(size) { }

	explicit ArrayLevel(const Array<node> &nodes) : m_nodes(nodes) { }

	const node &operator[](int i) const override { return m_nodes[i]; }

	node &operator[](int i) override { return m_nodes[i]; }

	int size() const override { return m_nodes.size(); }

	int high() const override { return m_nodes.high(); }

};


class BlockOrder;

/**
 * \brief Class representing idea of Blocks used in GlobalSifting
 * and GridSifting algorithms.
 *
 * For more information see papers and BlockOrder.
 */
class OGDF_EXPORT Block {

	friend class BlockOrder;
private:
	int m_index = 0; //!< The index of this block in BlockOrder.

	int m_upper = 0; //!< The top level of this block.

	int m_lower = 0; //!< The bottom level of this block.

	Array<node> m_nodes; //!< Vertices from the proper hierarchy corresponding to this block

	Array<int> m_NeighboursIncoming; //!< Indices of neighbouring incoming blocks.

	Array<int> m_InvertedIncoming; //!< Positions of this block in m_NeighboursOutgoing of neighbours.

	Array<int> m_NeighboursOutgoing; //!< Indices of neighbouring outgoing blocks.

	Array<int> m_InvertedOutgoing;  //!< Positions of this block in m_NeighboursIncoming of neighbours.

	// exactly one of those below is non null!
	node m_Node = nullptr; //!< The node for which this block was created.
	edge m_Edge = nullptr; //!< The edge for which this block was created.

	bool m_isEdgeBlock;
	bool m_isNodeBlock;

public:
	~Block() { }

	bool isEdgeBlock() { return m_isEdgeBlock; }

	bool isVertexBlock() { return m_isNodeBlock; }

	//! Creates new vertex block for a node \p v.
	explicit Block(node v);

	//! Creates new edge block for an edge \p e.
	explicit Block(edge e);

};

/**
 * \brief Hierarchical graph representation used by GlobalSifting
 * and GridSifting algorithms.
 *
 * This representation is based on blocks. Each block is a single vertex from
 * the original graph or edge that consists of some dummy vertices in hierarchical
 * embedding of this graph.
 *
 * BlockOrder stores permutation of blocks (their x-coordinates) and uses
 * this information in \a translation to Hierarchy and HierarchyLevelsBase.
 */
class OGDF_EXPORT BlockOrder : public HierarchyLevelsBase {

private:
	enum class Direction { Plus, Minus };

	GraphCopy m_GC; //!< The graph copy representing the topology of the proper hierarchy.

	NodeArray<int> m_ranks; //!< The rank (level) of a node.

	// Block X -> pi(X)
	Array<int> m_storedPerm; //!< The permutation from which the sifting step starts.
	Array<int> m_currentPerm; //!< The permutation modified in the sifting step.
	Array<int> m_bestPerm; //!< The best found permutation in the sifting step.

	// int i -> Block X s.t. pi(X) = i
	Array<int> m_currentPermInv; //!< Inversion of m_currenPerm.

	int m_storedCrossings; //!< Numebr of crossings stored in the sifting step.
	int m_bestCrossings; //!< The lowest number of crossing found in the sifting step.

#if 0
	unsigned int m_storedCrossings;
	unsigned int m_currentCrossings;
#endif

	Array<Block *> m_Blocks; //!< The array of all blocks.
	NodeArray<Block *> m_NodeBlocks; //!< The array of all vertex blocks.
	EdgeArray<Block *> m_EdgeBlocks; //!< The array of all edge blocks.

	EdgeArray<bool> m_isActiveEdge; //!< Stores information about active edge blocks.

#if 0
	unsigned int m_blocksCount;
#endif
	int m_activeBlocksCount;

	Hierarchy &m_hierarchy; //!< The hierarchy on grid- and globalsifting operates.

	void deconstruct(); //!< Deletes levels and blocks owned by this instance of BlockOrder.

	NodeArray<int> m_pos; //!< The position of a node on its level.

	Array<ArrayLevel *> m_levels; //!< The array of all levels.

	NodeArray<Array<node> > m_lowerAdjNodes; //!< (Sorted) adjacent nodes on lower level.
	NodeArray<Array<node> > m_upperAdjNodes; //!< (Sorted) adjacent nodes on upper level.

	NodeArray<int> m_nSet; //!< (Only used by buildAdjNodes().)

public:

	//! \name HierarchyLevelsBase members
	//! @{

	//! Returns the <i>i</i>-th level.
	const ArrayLevel &operator[](int i) const override {
		return *(m_levels[i]);
	}
	//! Returns the position of node \p v on its level.
	int pos(node v) const override {
		return m_pos[v];
	}
	//! Returns the number of levels.
	int size() const override {
		return m_levels.size();
	}

	const Hierarchy &hierarchy() const override {
		return m_hierarchy;
	}

	//! Returns the adjacent nodes of \p v.
	const Array<node> &adjNodes(node v, TraversingDir dir) const override {
		if ( dir == TraversingDir::upward ) {
			return m_upperAdjNodes[v];
		} else {
			return m_lowerAdjNodes[v];
		}
	}

	//! @}

	// destruction
	~BlockOrder() { deconstruct(); }

	//! Returns the number of blocks.
	int blocksCount() { return m_Blocks.size(); }

#if 0
	BlockOrder(const Graph &G, const NodeArray<int> &rank);
#endif

	explicit BlockOrder(Hierarchy& hierarchy, bool longEdgesOnly = true);

	//! Calls the global sifting algorithm on graph (its hierarchy).
	void globalSifting( int rho = 1, int nRepeats = 10, int *pNumCrossings = nullptr );

private:
	//! Does some initialization.
	void doInit( bool longEdgesOnly = true);
#if 0
	void doInit( bool longEdgesOnly = true, const NodeArray<int> &ranks);
#endif

	/**
	 * \brief Creates sorted lists of neighbours for all blocks.
	 *
	 * See function SORT-ADJACENCIES in paper.
	 */
	void sortAdjacencies();

	/**
	 * \brief Updates adjacencies lists before swaping two blocks.
	 *
	 * Updates adjacencies lists of two blocks and their
	 * neighbours in direction \p d. This function is called before
	 * blocks are swapped.
	 * See UPDATE-ADJACENCIES in papers.
	 */
	void updateAdjacencies(Block *blockOfA, Block *blockOfB, Direction d);

	/**
	 * \brief Calculates change of crossings made by a single swap.
	 *
	 * Calculates change in number of crossing after swapping two consecutive
	 * blocks in current permutation.
	 * See USWAP in papers.
	 */
	int uswap(Block *blockOfA, Block *blockOfB, Direction d, int level);

	/**
	 * \brief Swaps two consecutive blocks.
	 *
	 * See SIFTING-STEP in papers.
	 */
	int siftingSwap(Block *blockOfA, Block *blockOfB);

	/**
	 * \brief Performs sifting for a single block.
	 *
	 * See SIFTING-STEP in papers.
	 */
	int siftingStep( Block *blockOfA );

	/**
	 * \brief Builds levels of vertices from original graph.
	 */
	void buildLevels();

	/**
	 * \brief Builds list of dummy nodes laying inside edge blocks.
	 */
	void buildDummyNodesLists();

	/**
	 * \brief Builds lists of adjacent nodes (needed by HierarchyLevelsBase).
	 */
	void buildAdjNodes();

	/**
	 * \brief Builds arrays that allow using BlockOrder as HierarchyLevelsBase implementation.
	 */
	void buildHierarchy()
	{
		buildDummyNodesLists();
		buildLevels();
		buildAdjNodes();
		m_storedCrossings = calculateCrossings();
	}


	//! \name GridSifting
	//! @{

	/**
	 * \brief Moves block to next level.
	 */
	int verticalSwap( Block *b, int level );

	//! Only used in verticalSwap().
	int localCountCrossings( const Array<int> &levels );

	/**
	 * \brief Performs vertical step for block b.
	 *
	 * See VERTICAL-STEP in papers.
	 */
	void verticalStep( Block *b );

	Array<int> m_nNodesOnLvls;
public:
	int m_verticalStepsBound;

	/**
	 * \brief Calls the grid sifting algorithm on a graph (its hierarchy).
	 */
	void gridSifting( int nRepeats = 10 );

	//! @}
};

}
