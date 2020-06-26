/** \file
 * \brief Implementation of class ogdf::CliqueFinderModule.
 *
 * \author Karsten Klein, Max Ilsen
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

#include <ogdf/clique/CliqueFinderModule.h>
#include <ogdf/basic/simple_graph_alg.h>

namespace ogdf {

void CliqueFinderModule::call(const Graph &G, NodeArray<int> &cliqueNumber)
{
	beginCall(G);
	setResults(cliqueNumber);
	endCall();
}

void CliqueFinderModule::call(const Graph &G, List<List<node>*> &cliqueLists)
{
	beginCall(G);
	setResults(cliqueLists);
	endCall();
}

void CliqueFinderModule::beginCall(const Graph &G)
{
	m_pGraph = &G;
	m_pCopy = new GraphCopy(G);
	makeSimpleUndirected(*m_pCopy); // ignore multi-edges
	m_copyCliqueNumber.init(*m_pCopy, -1);

	if (!handleTrivialCases()) {
		doCall();
	}
}

void CliqueFinderModule::endCall()
{
	m_copyCliqueNumber.init();
	m_pGraph = nullptr;
	delete m_pCopy;
}

void CliqueFinderModule::setResults(NodeArray<int> &cliqueNum)
{
	cliqueNum.fill(-1);

	for (node v : m_pGraph->nodes) {
		node w = m_pCopy->copy(v);
		// The algorithm may have deleted nodes from m_pCopy.
		// These do not belong to any clique.
		if (w != nullptr) {
			cliqueNum[v] = m_copyCliqueNumber[w];
		}
	}
}

void CliqueFinderModule::setResults(List<List<node>*> &cliqueLists)
{
	cliqueLists.clear();

	List<List<node>*> copyCliqueLists;
	cliqueNumberToList(*m_pCopy, m_copyCliqueNumber, copyCliqueLists);
	for (List<node> *copyClique : copyCliqueLists) {
		List<node> *clique = new List<node>();
		for (node vCopy : *copyClique) {
			clique->pushBack(m_pCopy->original(vCopy));
		}
		cliqueLists.pushBack(clique);
		delete copyClique;
	}
}

bool CliqueFinderModule::handleTrivialCases() {
	int nodeNum = m_pGraph->numberOfNodes();

	// If there are not enough nodes for a clique of min size, return.
	if (nodeNum < m_minDegree) {
		return true;
	}

	if (nodeNum < 3) {
		node v = m_pCopy->firstNode();
		if (nodeNum == 2) {
			if (m_minDegree <= 1 && m_pGraph->numberOfEdges() >= 1) {
				m_copyCliqueNumber[v] = 0;
				m_copyCliqueNumber[v->succ()] = 0;
			} else if (m_minDegree == 0) {
				m_copyCliqueNumber[v] = 0;
				m_copyCliqueNumber[v->succ()] = 1;
			}
		} else if (nodeNum == 1 && m_minDegree == 0) {
			m_copyCliqueNumber[v] = 0;
		}

		return true;
	}

	return false;
}

void CliqueFinderModule::cliqueListToNumber(const Graph &G,
		const List<List<node>*> &cliqueLists,
		NodeArray<int> &cliqueNumber)
{
	int nextCliqueNum = 0;
	cliqueNumber.init(G, -1);

	for (List<node> *clique : cliqueLists) {
		for (node v : *clique) {
			cliqueNumber[v] = nextCliqueNum;
		}
		nextCliqueNum++;
	}
}

void CliqueFinderModule::cliqueNumberToList(const Graph &G,
		const NodeArray<int> &cliqueNumber,
		List<List<node>*> &cliqueLists)
{
	cliqueLists.clear();

	List<node> nodesByCliqueNumber;
	G.allNodes(nodesByCliqueNumber);
	nodesByCliqueNumber.quicksort(GenericComparer<node, int>(cliqueNumber));

	List<node> *curClique = nullptr;
	for (auto itNode = nodesByCliqueNumber.begin(); itNode.valid(); itNode++) {
		node v = *itNode;
		// Ignore nodes with clique number -1.
		if (cliqueNumber[v] >= 0) {
			// Create a new clique if necessary.
			if (curClique == nullptr) {
				curClique = new List<node>();
			}
			curClique->pushBack(v);

			// If we are at the end or the next node is in a new clique,
			// push the clique to cliqueLists and reset the current clique.
			if (!itNode.succ().valid() ||
			    cliqueNumber[v] != cliqueNumber[*(itNode.succ())]) {
				cliqueLists.pushBack(curClique);
				curClique = nullptr;
			}
		}
	}
}

void CliqueFinderModule::cliqueGraphAttributes(const Graph &G,
		const NodeArray<int> &cliqueNumber,
		GraphAttributes &GA)
{
	const int RGB_MAX = 256;
	const int RGB_MAX_HALF = RGB_MAX / 2;

	GA.addAttributes(GraphAttributes::nodeGraphics
		| GraphAttributes::nodeStyle
		| GraphAttributes::nodeLabel
	);

	for (node v : G.nodes) {
		int num = cliqueNumber[v];
		int colVals[3];

		setSeed(num);
		for (int i = 0; i < 3; ++i) {
			colVals[i] = num < 0 ? RGB_MAX - 1 :
				randomNumber(0, RGB_MAX_HALF) + RGB_MAX_HALF;
		}

		GA.fillColor(v) = Color(colVals[0], colVals[1], colVals[2]);
		GA.label(v) = to_string(num);
	}
}

bool CliqueFinderModule::cliqueOK(const Graph &G,
		List<node> *clique,
		double density)
{
	// Get desired number of edges in clique/dense subgraph (times two).
	int k = clique->size();
	int desiredCliqueEdges = int(ceil(density * k * (k-1)));

	NodeArray<int> inClique(G, 0);
	for (node v : *clique) {
		inClique[v] = true;
	}

	// Count each edge between clique members (twice).
	int cliqueEdges = 0;
	for (node v : *clique) {
		for (adjEntry adj : v->adjEntries) {
			if (inClique[adj->twinNode()]) {
				cliqueEdges++;
			}
		}
	}

	return cliqueEdges >= desiredCliqueEdges;
}

}
