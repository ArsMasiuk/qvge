/** \file
 * \brief Declaration of class ArrayGraph.
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
#include <ogdf/energybased/fast_multipole_embedder/EdgeChain.h>

namespace ogdf {
namespace fast_multipole_embedder {

class ArrayGraph
{
public:
	//! Constructor. Does not allocate memory for the members.
	ArrayGraph();

	//! Constructor. Allocates memory via OGDF_MALLOC_16.
	ArrayGraph(uint32_t maxNumNodes, uint32_t maxNumEdges);

	//! Constructor.
	ArrayGraph(const GraphAttributes& GA, const EdgeArray<float>& edgeLength, const NodeArray<float>& nodeSize);

	//! Destructor. Deallocates the memory via OGDF_FREE_16 if needed.
	~ArrayGraph();

	//! Returns the number of nodes.
	inline uint32_t numNodes() const { return m_numNodes; }

	//! Returns the number of edges
	inline uint32_t numEdges() const { return m_numEdges; }

	//! Updates an ArrayGraph from GraphAttributes with the given edge lengths and node sizes and creates the edges.
	/**
	 * The nodes and edges are ordered in the same way like in the Graph instance.
	 * @param GA the GraphAttributes to read from
	 * @param edgeLength the desired edge length
	 * @param nodeSize the size of the nodes
	 */
	void readFrom(const GraphAttributes& GA, const EdgeArray<float>& edgeLength, const NodeArray<float>& nodeSize);

	//! Updates an ArrayGraph with the given positions, edge lengths and node sizes and creates the edges
	/**
	 * The nodes and edges are ordered in the same way like in the Graph instance.
	 * @param G the Graph to traverse
	 * @param xPos the x coordinate for each node
	 * @param yPos the y coordinate for each node
	 * @param nodeSize the x coordinate for each node in G
	 * @param edgeLength the desired edge length for each edge
	 * @tparam CoordinateType type of the \a x and \a y positions
	 * @tparam LengthType type of the edge lengths
	 * @tparam SizeType type of the node sizes
	 */
	template<typename CoordinateType, typename LengthType, typename SizeType>
	void readFrom(const Graph& G, NodeArray<CoordinateType>& xPos, NodeArray<CoordinateType>& yPos, const EdgeArray<LengthType>& edgeLength, const NodeArray<SizeType>& nodeSize)
	{
		m_numNodes = 0;
		m_numEdges = 0;
		NodeArray<uint32_t> nodeIndex(G);
		m_numNodes = 0;
		m_numEdges = 0;
		m_desiredAvgEdgeLength = 0;
		m_avgNodeSize = 0;
		for(node v : G.nodes)
		{
			m_nodeXPos[m_numNodes] = (float)xPos[v];
			m_nodeYPos[m_numNodes] = (float)yPos[v];
			m_nodeSize[m_numNodes] = (float)nodeSize[v];
			m_avgNodeSize += nodeSize[v];
			nodeIndex[v] = m_numNodes;
			m_numNodes++;
		}
		m_avgNodeSize = m_avgNodeSize / (double)m_numNodes;

		for(edge e : G.edges)
		{
			pushBackEdge(nodeIndex[e->source()], nodeIndex[e->target()], (float)edgeLength[e]);
		}
		m_desiredAvgEdgeLength = m_desiredAvgEdgeLength / (double)m_numEdges;
	}

	//! Store the data back in GraphAttributes
	/**
	 * The function does not require to be the same Graph, only the order of nodes and edges
	 * is important
	 * @param GA the GraphAttributes to update
	 */
	void writeTo(GraphAttributes& GA);

	//! Store the data back to NodeArray arrays with the given coordinate type
	/**
	 * The function does not require to be the same Graph, only the order of nodes and edges
	 * is important
	 *
	 * @param G the graph containing all nodes
	 * @param xPos the x coordinate array to update
	 * @param yPos the y coordinate array to update
	 * @tparam CoordinateType type of the \a x and \a y positions
	 */
	template<typename CoordinateType>
	void writeTo(const Graph& G, NodeArray<CoordinateType>& xPos, NodeArray<CoordinateType>& yPos)
	{
		uint32_t i = 0;
		for(node v : G.nodes)
		{
			xPos[v] = m_nodeXPos[i];
			yPos[v] = m_nodeYPos[i];
			i++;
		}
	}

	//! Returns the adjacency information for the node at index \p i in #m_nodeAdj.
	inline NodeAdjInfo& nodeInfo(uint32_t i) { return m_nodeAdj[i];	}

	//! Returns the adjacency information for the node at index \p i in #m_nodeAdj.
	inline const NodeAdjInfo& nodeInfo(uint32_t i) const { return m_nodeAdj[i];	}

	//! Returns the adjacency information for the edge at index \p i in #m_edgeAdj.
	inline EdgeAdjInfo& edgeInfo(uint32_t i) { return m_edgeAdj[i];	}

	//! Returns the adjacency information for the edge at index \p i in #m_edgeAdj.
	inline const EdgeAdjInfo& edgeInfo(uint32_t i) const { return m_edgeAdj[i];	}

	//! Returns the NodeAdjInfo array for all nodes.
	inline NodeAdjInfo* nodeInfo() { return m_nodeAdj; }

	//! Returns the NodeAdjInfo array for all nodes.
	inline const NodeAdjInfo* nodeInfo() const { return m_nodeAdj; }

	//! Returns the EdgeAdjInfo array for all edges.
	inline EdgeAdjInfo* edgeInfo() { return m_edgeAdj; }

	//! Returns the EdgeAdjInfo array for all edges.
	inline const EdgeAdjInfo* edgeInfo() const { return m_edgeAdj; }

	//! Returns the \a x coord array for all nodes.
	inline float* nodeXPos() { return m_nodeXPos; }

	//! Returns the \a x coord array for all nodes.
	inline const float* nodeXPos() const { return m_nodeXPos; }

	//! Returns the \a y coord array for all nodes.
	inline float* nodeYPos() { return m_nodeYPos; }

	//! Returns the \a y coord array for all nodes.
	inline const float* nodeYPos() const { return m_nodeYPos; }

	//! Returns the node size array for all nodes.
	inline float* nodeSize() { return m_nodeSize; }

	//! Returns the node size array for all nodes.
	inline const float* nodeSize() const { return m_nodeSize; }

	//! Returns the node movement radius array for all nodes.
	inline float* nodeMoveRadius() { return m_nodeMoveRadius; }

	//! Returns the edge length array for all edges.
	inline float* desiredEdgeLength() { return m_desiredEdgeLength; }

	//! Returns the edge length array for all edges.
	inline const float* desiredEdgeLength() const { return m_desiredEdgeLength; }

	//! Returns the index of the first pair of the node with index \p nodeIndex in #m_nodeAdj.
	inline uint32_t firstEdgeAdjIndex(uint32_t nodeIndex) const
	{
		return nodeInfo(nodeIndex).firstEntry;
	};

	//! Returns the index of the next pair of \p currEdgeAdjIndex of the node with index \p nodeIndex.
	inline uint32_t nextEdgeAdjIndex(uint32_t currEdgeAdjIndex, uint32_t nodeIndex) const
	{
		return edgeInfo(currEdgeAdjIndex).nextEdgeAdjIndex(nodeIndex);
	}

	//! Returns the other node (not \p nodeIndex) of the pair with index \p currEdgeAdjIndex.
	inline uint32_t twinNodeIndex(uint32_t currEdgeAdjIndex, uint32_t nodeIndex) const
	{
		return edgeInfo(currEdgeAdjIndex).twinNode(nodeIndex);
	}

	//! Calls \p func on all nodes with indices from \p begin to \p end.
	void for_all_nodes(uint32_t begin, uint32_t end, std::function<void(uint32_t)> func)
	{
		for(uint32_t i=begin; i <=end; i++)
			func(i);
	}

	//! Average edge length.
	inline float avgDesiredEdgeLength() const { return (float)m_desiredAvgEdgeLength; }

	//! Average node size.
	inline float avgNodeSize() const { return (float)m_avgNodeSize; }

	//! Transforms all positions via shifting them by \p translate and afterwards scaling by \p scale.
	void transform(float translate, float scale);

	//! Transforming all positions such that the new center is at \a (0,0).
	void centerGraph();

private:

	//! Internal function used by #readFrom.
	void pushBackEdge(uint32_t a, uint32_t b, float desiredEdgeLength);

	//! Allocate all arrays.
	void allocate(uint32_t numNodes, uint32_t numEdges);

	//! Deallocate all arrays.
	void deallocate();

	//! Clear the arrays.
	void clear()
	{
		for (uint32_t i=0; i < m_numNodes; i++)
			nodeInfo(i).degree = 0;

		m_numNodes = 0;
		m_numEdges = 0;
	}

	uint32_t m_numNodes; //!< Number of nodes in the graph.
	uint32_t m_numEdges; //!< Number of edges in the graph.

	float* m_nodeXPos; //!< The \a x coordinates.
	float* m_nodeYPos; //!< The \a y coordinates.

	float* m_nodeSize;    //!< Sizes of the nodes.
	double m_avgNodeSize; //!< Avg. node size.

	float* m_nodeMoveRadius; //!< Maximum node movement lengths.

	float* m_desiredEdgeLength;    //!< Edge lengths.
	double m_desiredAvgEdgeLength; //!< Avg. edge length.

	NodeAdjInfo* m_nodeAdj; //!< Information about adjacent edges.
	EdgeAdjInfo* m_edgeAdj; //!< Information about adjacent nodes.
};

}
}
