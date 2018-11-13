/** \file
 * \brief Declaration of class WSPD (well-separated pair decomposition).
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

#include <ogdf/energybased/fast_multipole_embedder/LinearQuadtree.h>
#include <ogdf/energybased/fast_multipole_embedder/EdgeChain.h>

namespace ogdf {
namespace fast_multipole_embedder {

//! Class for the Well-Separated-Pairs-Decomposition (%WSPD)
class WSPD
{
public:
	using NodeID = LinearQuadtree::NodeID;

	//! Constructor. Allocates the memory via OGDF_MALLOC_16.
	explicit WSPD(uint32_t maxNumNodes);

	//! Destructor. Deallocates via OGDF_FREE_16.
	~WSPD();

	//! Returns the maximum number of nodes. Equals the maximum number of nodes in the LinearQuadtree.
	inline uint32_t maxNumNodes() const
	{
		return m_maxNumNodes;
	}

	//! Returns the number of well separated nodes for node \p a.
	inline uint32_t numWSNodes(NodeID a) const
	{
		return m_nodeInfo[a].degree;
	}

	//! Returns the total number of pairs.
	inline uint32_t numPairs() const
	{
		return m_numPairs;
	}

	//! Returns the maximum number of pairs.
	inline uint32_t maxNumPairs() const
	{
		return m_maxNumPairs;
	}

	//! Resets the array #m_nodeInfo.
	void clear();

	//! Adds a well separated pair (\p a, \p b)
	void addWSP(NodeID a, NodeID b);

	//! Returns the pair info for index \p pairIndex.
	inline EdgeAdjInfo& pairInfo(uint32_t pairIndex) const
	{
		return m_pairs[pairIndex];
	}

	//! Returns the node info for index \p nodeID.
	inline NodeAdjInfo& nodeInfo(NodeID nodeID) const
	{
		return m_nodeInfo[nodeID];
	}

	//! Returns the index of the next pair of \p currPairIndex of the node with index \p a.
	inline uint32_t nextPair(uint32_t currPairIndex, NodeID a) const
	{
		return pairInfo(currPairIndex).nextEdgeAdjIndex(a);
	}

	//! Returns the other node (not \p a) of the pair with index \p currPairIndex.
	inline uint32_t wsNodeOfPair(uint32_t currPairIndex, NodeID a) const
	{
		return pairInfo(currPairIndex).twinNode(a);
	}

	//! Returns the index of the first pair of node \p nodeID.
	inline uint32_t firstPairEntry(NodeID nodeID) const
	{
		return m_nodeInfo[nodeID].firstEntry;
	}

	// Returns the size excluding small member vars (for profiling only).
	unsigned long sizeInBytes() const;

private:
	//! Allocates all memory.
	void allocate();

	//! Releases all memory.
	void deallocate();

	uint32_t m_maxNumNodes; //!< Maximum number of nodes.
	NodeAdjInfo* m_nodeInfo; //!< Array which holds the wspd information for one quadtree node.
	EdgeAdjInfo* m_pairs; //!< Array containing all pairs.
	uint32_t m_numPairs; //!< Total number of pairs.
	uint32_t m_maxNumPairs; //!< Upper bound for the number of pairs.
};

}
}
