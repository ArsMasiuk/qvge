/** \file
 * \brief Declaration of class GalaxyMultilevelBuilder.
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
#include <ogdf/basic/tuples.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/energybased/fast_multipole_embedder/ArrayGraph.h>
#include <ogdf/energybased/fast_multipole_embedder/FastUtils.h>

namespace ogdf {
namespace fast_multipole_embedder {

class GalaxyMultilevel
{
public:
	using NearSunList = List<Tuple2<node,int>>;

	struct LevelNodeInfo
	{
		float mass;
		float radius;
		node parent;
		NearSunList nearSuns;
	};

	struct LevelEdgeInfo
	{
		float length;
	};

	explicit GalaxyMultilevel(Graph* pGraph)
	{
		m_pFinerMultiLevel = nullptr;
		m_pCoarserMultiLevel = nullptr;
		m_pGraph = pGraph;
		m_pNodeInfo = new NodeArray<LevelNodeInfo>(*m_pGraph);
		m_pEdgeInfo = new EdgeArray<LevelEdgeInfo>(*m_pGraph);
		for(node v : m_pGraph->nodes)
		{
			(*m_pNodeInfo)[v].mass = 1.0;
		}
		levelNumber = 0;
	}

	GalaxyMultilevel(GalaxyMultilevel* prev)
	{
		m_pCoarserMultiLevel = nullptr;
		m_pFinerMultiLevel = prev;
		m_pFinerMultiLevel->m_pCoarserMultiLevel = this;
		m_pGraph = nullptr;
		m_pNodeInfo = nullptr;
		levelNumber = prev->levelNumber + 1;
	}

	~GalaxyMultilevel() { }

	GalaxyMultilevel* m_pFinerMultiLevel;
	GalaxyMultilevel* m_pCoarserMultiLevel;
	Graph* m_pGraph;
	NodeArray<LevelNodeInfo>* m_pNodeInfo;
	EdgeArray<LevelEdgeInfo>* m_pEdgeInfo;
	int levelNumber;
};


class GalaxyMultilevelBuilder
{
public:
	struct LevelNodeState
	{
		node lastVisitor;
		double sysMass;
		int label;
		float edgeLengthFromSun;
	};

	struct NodeOrderInfo
	{
		node theNode;
	};

	GalaxyMultilevel* build(GalaxyMultilevel* pMultiLevel);

private:
	void computeSystemMass();
	void sortNodesBySystemMass();
	void createResult(GalaxyMultilevel* pMultiLevelResult);
	void labelSystem(node u, node v, int d, float df);
	void labelSystem();
	Graph* m_pGraph = nullptr;
	Graph* m_pGraphResult = nullptr;
	List<node> m_sunNodeList;
	List<edge> m_interSystemEdges;
	NodeArray<GalaxyMultilevel::LevelNodeInfo>* m_pNodeInfo = nullptr;
	EdgeArray<GalaxyMultilevel::LevelEdgeInfo>* m_pEdgeInfo = nullptr;
	NodeArray<GalaxyMultilevel::LevelNodeInfo>* m_pNodeInfoResult = nullptr;
	EdgeArray<GalaxyMultilevel::LevelEdgeInfo>* m_pEdgeInfoResult = nullptr;
	NodeArray<LevelNodeState> m_nodeState;
	NodeOrderInfo* m_nodeMassOrder = nullptr;
	RandomNodeSet* m_pRandomSet = nullptr;
	int m_dist = 0;
};


class NodeMassComparer
{
public:
	explicit NodeMassComparer(const NodeArray< GalaxyMultilevelBuilder::LevelNodeState>& nodeState) : m_nodeState(nodeState) { }

	// used for std::sort
	inline bool operator()(const GalaxyMultilevelBuilder::NodeOrderInfo& a, const GalaxyMultilevelBuilder::NodeOrderInfo& b) const
	{
		return m_nodeState[a.theNode].sysMass < m_nodeState[b.theNode].sysMass;
	}

private:
	const NodeArray< GalaxyMultilevelBuilder::LevelNodeState >& m_nodeState;
};

}
}
