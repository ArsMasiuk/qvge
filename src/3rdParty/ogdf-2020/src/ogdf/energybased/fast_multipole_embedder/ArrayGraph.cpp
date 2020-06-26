/** \file
 * \brief Implementation of class ArrayGraph.
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

#include <ogdf/energybased/fast_multipole_embedder/ArrayGraph.h>
#include <ogdf/energybased/fast_multipole_embedder/FastUtils.h>

namespace ogdf {
namespace fast_multipole_embedder {

ArrayGraph::ArrayGraph():   m_numNodes(0),
							m_numEdges(0),
							m_nodeXPos(nullptr),
							m_nodeYPos(nullptr),
							m_nodeSize(nullptr),
							m_nodeMoveRadius(nullptr),
							m_desiredEdgeLength(nullptr),
							m_nodeAdj(nullptr),
							m_edgeAdj(nullptr)
{
}

ArrayGraph::ArrayGraph(uint32_t maxNumNodes, uint32_t maxNumEdges) :
							m_numNodes(maxNumNodes),
							m_numEdges(maxNumEdges),
							m_nodeXPos(nullptr),
							m_nodeYPos(nullptr),
							m_nodeSize(nullptr),
							m_nodeMoveRadius(nullptr),
							m_desiredEdgeLength(nullptr),
							m_nodeAdj(nullptr),
							m_edgeAdj(nullptr)
{
	allocate(maxNumNodes, maxNumEdges);
}

ArrayGraph::ArrayGraph(const GraphAttributes& GA, const EdgeArray<float>& edgeLength, const NodeArray<float>& nodeSize) :
							m_numNodes(0),
							m_numEdges(0),
							m_nodeXPos(nullptr),
							m_nodeYPos(nullptr),
							m_nodeSize(nullptr),
							m_nodeMoveRadius(nullptr),
							m_desiredEdgeLength(nullptr),
							m_nodeAdj(nullptr),
							m_edgeAdj(nullptr)
{
	allocate(GA.constGraph().numberOfNodes(), GA.constGraph().numberOfEdges());
	readFrom(GA, edgeLength, nodeSize);
}

ArrayGraph::~ArrayGraph(void)
{
	if (m_nodeXPos)
		deallocate();
}

void ArrayGraph::allocate(uint32_t numNodes, uint32_t numEdges)
{
	m_nodeXPos = static_cast<float*>(OGDF_MALLOC_16(numNodes*sizeof(float)));
	m_nodeYPos = static_cast<float*>(OGDF_MALLOC_16(numNodes*sizeof(float)));
	m_nodeSize = static_cast<float*>(OGDF_MALLOC_16(numNodes*sizeof(float)));
	m_nodeMoveRadius = static_cast<float*>(OGDF_MALLOC_16(numNodes*sizeof(float)));
	m_nodeAdj = static_cast<NodeAdjInfo*>(OGDF_MALLOC_16(numNodes*sizeof(NodeAdjInfo)));
	m_desiredEdgeLength = static_cast<float*>(OGDF_MALLOC_16(numEdges*sizeof(float)));
	m_edgeAdj = static_cast<EdgeAdjInfo*>(OGDF_MALLOC_16(numEdges*sizeof(EdgeAdjInfo)));

	for (uint32_t i=0; i < numNodes; i++)
		nodeInfo(i).degree = 0;
}

void ArrayGraph::deallocate()
{
	OGDF_FREE_16(m_nodeXPos);
	OGDF_FREE_16(m_nodeYPos);
	OGDF_FREE_16(m_nodeSize);
	OGDF_FREE_16(m_nodeMoveRadius);
	OGDF_FREE_16(m_nodeAdj);
	OGDF_FREE_16(m_desiredEdgeLength);
	OGDF_FREE_16(m_edgeAdj);
}

void ArrayGraph::pushBackEdge(uint32_t a, uint32_t b, float desiredEdgeLength)
{
	// get the index of a free element
	uint32_t e_index = m_numEdges++;

	m_desiredEdgeLength[e_index] = desiredEdgeLength;
	m_desiredAvgEdgeLength += (double)desiredEdgeLength;

	fast_multipole_embedder::pushBackEdge(
		a, b,
		[&](uint32_t i) -> EdgeAdjInfo& {return edgeInfo(i);},
		[&](uint32_t i) -> NodeAdjInfo& {return nodeInfo(i);},
		e_index);
}

void ArrayGraph::readFrom(const GraphAttributes& GA, const EdgeArray<float>& edgeLength, const NodeArray<float>& nodeSize)
{
	const Graph& G = GA.constGraph();
	NodeArray<uint32_t> nodeIndex(G);

	m_numNodes = 0;
	m_numEdges = 0;
	m_avgNodeSize = 0;
	m_desiredAvgEdgeLength = 0;
	for(node v : G.nodes)
	{
		m_nodeXPos[m_numNodes] = (float)GA.x(v);
		m_nodeYPos[m_numNodes] = (float)GA.y(v);
		m_nodeSize[m_numNodes] = nodeSize[v];
		nodeIndex[v] = m_numNodes;
		m_avgNodeSize += nodeSize[v];
		m_numNodes++;
	}
	m_avgNodeSize = m_avgNodeSize / (double)m_numNodes;

	for(edge e : G.edges)
	{
		pushBackEdge(nodeIndex[e->source()], nodeIndex[e->target()], (float)edgeLength[e]);
	}
	m_desiredAvgEdgeLength = m_desiredAvgEdgeLength / (double)m_numEdges;
}

void ArrayGraph::writeTo(GraphAttributes& GA)
{
	const Graph& G = GA.constGraph();
	uint32_t i = 0;
	for(node v : G.nodes)
	{
		GA.x(v) = m_nodeXPos[i];
		GA.y(v) = m_nodeYPos[i];
		i++;
	}
}

void ArrayGraph::transform(float translate, float scale)
{
	for (uint32_t i=0; i < m_numNodes; i++)
	{
		m_nodeXPos[i] = (m_nodeXPos[i] + translate)*scale;
		m_nodeYPos[i] = (m_nodeYPos[i] + translate)*scale;
	}
}

void ArrayGraph::centerGraph()
{
	double dx_sum = 0;
	double dy_sum = 0;

	for (uint32_t i=0; i < m_numNodes; i++)
	{
		dx_sum += m_nodeXPos[i];
		dy_sum += m_nodeYPos[i];
	};

	dx_sum /= (double)m_numNodes;
	dy_sum /= (double)m_numNodes;
	for (uint32_t i=0; i < m_numNodes; i++)
	{
		m_nodeXPos[i] -= (float)dx_sum;
		m_nodeYPos[i] -= (float)dy_sum;
	}
}

}
}
