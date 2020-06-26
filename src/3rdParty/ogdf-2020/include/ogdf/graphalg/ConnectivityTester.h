/** \file
 * \brief Class for computing the connectivity of a graph.
 *
 * \author Tilo Wiedera
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

#include <ogdf/graphalg/MaxFlowModule.h>
#include <ogdf/graphalg/MaxFlowGoldbergTarjan.h>
#include <ogdf/basic/GraphCopy.h>

namespace ogdf {

/**
 * \brief Naive implementation for testing the connectivity of a graph.
 *
 * The connectivity is computed utilizing ogdf::MaxFlowModule.
 *
 * Note that the runtime might be improved by implementing a Gomory-Hu Tree.
 */
class OGDF_EXPORT ConnectivityTester {
private:
	MaxFlowModule<int> *m_flowAlgo;
	NodeArray<node> *m_source;
	bool m_usingDefaultMaxFlow;
	bool m_graphCopied;
	bool m_nodeConnectivity;
	bool m_directed;
	const Graph *m_graph;

	/**
	* Prepares the graph.
	* Might create a copy of the actual graph to apply transformations.
	* This is necessary to compute node connectivity and for undirected graphs.
	*
	* @param graph The original graph
	*/
	void prepareGraph(const Graph &graph);

	/**
	* Computes the connectivity of two nodes of the transformed graph.
	*
	* @param v The source node
	* @param u The target node
	*/
	int computeConnectivity(node v, node u);

	/**
	* Computes the connectivity of all nodes of the transformed graph.
	*
	* @param result The connectivity of two nodes each.
	*               For directed graphs, the first index denotes the source node.
	*               The connectivity of a node with itself is returned as 0.
	* @return The minimal connectivity of any two nodes in the graph.
	*/
	int computeConnectivity(NodeArray<NodeArray<int>> &result);

	/**
	* Makes the graph bi-directed.
	*
	* @param graph The graph to be altered.
	*/
	void duplicateEdges(Graph &graph);

	/**
	* Restricts the flow through each node to 1.
	* Must be called after #duplicateEdges().
	*
	* @param graph The graph to be altered.
	*/
	void restrictNodes(Graph &graph);

	/**
	* Retuns the node of the transformed graph corresponding to node \p v.
	*
	* @param v the original node
	* @param isSource Whether to return the corresponding source node.
	*                 If node connectivity is to be computed, for each original node, two copies exist.
	*/
	node copyOf(node v, bool isSource = false) const;

public:
	/**
	* Initializes a new connectivity tester using ogdf::MaxFlowGoldbergTarjan.
	*
	* @param nodeConnectivity Whether to compute node connectivity instead of edge connectivity
	* @param directed Whether to consider edges to be directed
	*/
	explicit ConnectivityTester(bool nodeConnectivity = true, bool directed = false) :
	  ConnectivityTester(new MaxFlowGoldbergTarjan<int>(), nodeConnectivity, directed) {
		m_usingDefaultMaxFlow = true;
	}

	/**
	* Initializes a new onnectivity tester  using a custom ogdf::MaxFlowModule.
	*
	* @param flowAlgo The maximum flow algorithm to be used.
	* @param nodeConnectivity Whether to compute node connectivity instead of edge connectivity
	* @param directed Whether to consider edges to be directed
	*/
	ConnectivityTester(MaxFlowModule<int> *flowAlgo, bool nodeConnectivity = true, bool directed = false) :
	  m_flowAlgo(flowAlgo),
	  m_source(nullptr),
	  m_usingDefaultMaxFlow(false),
	  m_graphCopied(false),
	  m_nodeConnectivity(nodeConnectivity),
	  m_directed(directed),
	  m_graph(nullptr) {
	}

	/**
	* Destroys the connectivity tester and frees allocated memory.
	*/
	~ConnectivityTester() {
		if (m_usingDefaultMaxFlow) {
			delete m_flowAlgo;
		}

		if (m_graphCopied) {
			delete m_graph;
		}

		delete m_source;
	}

	/**
	* Computes the connectivity of two nodes.
	* To reduce duplicate graph transformations, #computeConnectivity(const Graph &graph, NodeArray<NodeArray<int>> &result)
	* should be used to compute the connectivity of all nodes.
	*
	* @param graph The graph to be investigated
	* @param v The source node
	* @param u The target node
	*/
	int computeConnectivity(const Graph &graph, node v, node u) {
		prepareGraph(graph);

		return computeConnectivity(copyOf(v, true), copyOf(u));
	}

	/**
	* Computes the connectivity of all nodes of the provided graph.
	*
	* @param graph The graph to be investigated
	* @param result The connectivity of two nodes each.
	*               For directed graphs, the first index denotes the source node.
	*               The connectivity of a node with itself is returned as 0.
	* @return The minimal connectivity of any two nodes in the graph.
	*/
	int computeConnectivity(const Graph &graph, NodeArray<NodeArray<int>> &result) {
		prepareGraph(graph);

		return computeConnectivity(result);
	}
};

}
