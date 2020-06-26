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

#include <ogdf/graphalg/ConnectivityTester.h>
#include <ogdf/basic/Math.h>

namespace ogdf {

void ConnectivityTester::duplicateEdges(Graph & graph)
{
	List<edge> edges;
	graph.allEdges(edges);

	for (edge e : edges) {
		graph.newEdge(e->target(), e->source());
	}
}

void ConnectivityTester::restrictNodes(Graph & graph)
{
	List<node> nodes;
	graph.allNodes(nodes);

	delete m_source;

	m_source = new NodeArray<node>(graph);

	for (node v : nodes) {
		node w = graph.newNode();

		(*m_source)[v] = w;

		List<edge> edges;
		v->adjEdges(edges);

		for (edge e : edges) {
			if (e->source() == v) {
				graph.moveSource(e, w);
			}
		}

		graph.newEdge(v, w);
	}
}

void ConnectivityTester::prepareGraph(const Graph &graph)
{
	if(m_graphCopied) {
		delete m_graph;
	}

	m_graphCopied = m_nodeConnectivity || !m_directed;

	if (m_graphCopied) {
		Graph *copy = new GraphCopy(graph);

		if (!m_directed) {
			duplicateEdges(*copy);
		}

		if (m_nodeConnectivity) {
			restrictNodes(*copy);
		}

		m_graph = copy;
	} else {
		m_graph = &graph;
	}
}

int ConnectivityTester::computeConnectivity(node v, node u)
{
	OGDF_ASSERT(v != u);

	m_flowAlgo->init(*m_graph);
	EdgeArray<int> cap(*m_graph, 1);

	return m_flowAlgo->computeValue(cap, v, u);
}

int ConnectivityTester::computeConnectivity(NodeArray<NodeArray<int>> &Connectivity)
{
	node v = m_graph->firstNode();
	int result = m_graph->numberOfNodes();

	if(m_graphCopied) {
		v = ((GraphCopy*)m_graph)->original().firstNode();
	}

	for (; v != nullptr; v = v->succ()) {
		Connectivity[v][v] = 0;

		for (node u = v->succ(); u != nullptr; u = u->succ()) {
			Connectivity[v][u] = computeConnectivity(copyOf(v, true), copyOf(u));
			Math::updateMin(result, Connectivity[v][u]);

			if (m_directed) {
				Connectivity[u][v] = computeConnectivity(copyOf(u, true), copyOf(v));
				Math::updateMin(result, Connectivity[u][v]);
			} else {
				Connectivity[u][v] = Connectivity[v][u];
			}
		}
	}

	return result;
}

node ConnectivityTester::copyOf(node v, bool isSource) const
{
	node result = v;

	if(m_graphCopied) {
		result = ((GraphCopy*) m_graph)->copy(v);

		if(isSource && m_nodeConnectivity) {
			result = (*m_source)[result];
		}
	}

	return result;
}

}
