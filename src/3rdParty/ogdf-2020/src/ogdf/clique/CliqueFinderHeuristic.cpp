/** \file
 * \brief Implementation of a heuristical method to find cliques
 * in a given input graph.
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

//#define OGDF_CLIQUE_FINDER_HEURISTIC_DEBUG

#include <ogdf/clique/CliqueFinderHeuristic.h>
#include <ogdf/basic/geometry.h>

namespace ogdf {

CliqueFinderHeuristic::CliqueFinderHeuristic()
	: CliqueFinderModule()
	, m_density(1.0)
	, m_postProcess(true)
	, m_adjOracle(nullptr)
{ }

void CliqueFinderHeuristic::doCall()
{
	// Delete nodes with degree < m_minDegree, they are not part of cliques.
	preProcess();

	// Initialize member variables.
	m_adjOracle = new AdjacencyOracle(*m_pCopy);
	m_usedNode.init(*m_pCopy, false);

	// Sort the nodes later by the number of high degree neighbours.
	// For density = 1.0 this is done automatically via degree() as all low
	// degree neighbours are deleted. For density != 1.0 the high degree
	// neighbours have to be found manually.
	auto degreeFunction = OGDF_GEOM_ET.equal(m_density, 1.0) ?
		std::function<int(node)>([](node v) { return v->degree(); }) :
		std::function<int(node)>([&](node v) {
			int relativeDegree{0};
			for (adjEntry adj = v->firstAdj(); adj; adj = adj->succ()) {
				if (adj->twinNode()->degree() >= m_minDegree) {
					relativeDegree++;
				}
			}
			return relativeDegree;
		});

	// Sort the nodes by their degree in descending order.
	List<node> nodesSortedByDegree;
	m_pCopy->allNodes(nodesSortedByDegree);
	nodesSortedByDegree.quicksort(
		GenericComparer<node, int, false>(degreeFunction)
	);

	// Add clique candidates to cliqueList:
	// Put new nodes in existing cliques - if not possible, start new cliques.
	List<List<node>*> cliqueList;
	for (node vCand : nodesSortedByDegree) {
		// TODO Alternatively one could accept for every chosen node only its
		// neighbours as candidates. One should test several strategies, e.g.
		// go strictly by list order, try a BFS from first node.

		// Do not use a node in multiple cliques.
		if (m_usedNode[vCand]) {
			continue;
		}

		// For every clique candidate, try to fit vCand into it.
		bool setFound = false;
		for (auto itCand = cliqueList.begin(); itCand.valid() && !setFound; ++itCand) {
			// If vCand can be added to the clique, do so.
			// For dense subgraphs: If each new subgraph node is adjacent to
			// enough nodes, the final subgraph also has high enough density.
			if (allAdjacent(vCand, *itCand)) {
				OGDF_ASSERT(!m_usedNode[vCand]);
				(*itCand)->pushBack(vCand);
				m_usedNode[vCand] = true;
				setFound = true;

				// Move the clique to its correct place in cliqueList,
				// which is sorted from the biggest to the smallest clique.
				auto itSearch = itCand.pred();
				while (itSearch.valid()
				    && (*itCand)->size() > (*itSearch)->size()) {
					--itSearch;
				}

				if (itSearch.valid()) {
					cliqueList.moveToSucc(itCand, itSearch);
				} else {
					cliqueList.moveToFront(itCand);
				}
			}
		}

		// Create a new clique candidate containing only vCand if necessary.
		if (!setFound) {
			List<node>* cliqueCandidate = new List<node>();
			cliqueList.pushBack(cliqueCandidate);
			OGDF_ASSERT(!m_usedNode[vCand]);
			cliqueCandidate->pushBack(vCand);
			m_usedNode[vCand] = true;
		}
	}

#ifdef OGDF_DEBUG
	for (List<node> *clique : cliqueList) {
		OGDF_ASSERT(cliqueOK(*m_pCopy, clique, m_density));
	}
#endif

#ifdef OGDF_CLIQUE_FINDER_HEURISTIC_DEBUG
	int numC1 = cliqueList.size();

	int nodeNum = 0;
	for (List<node> *clique : cliqueList) {
		if (clique->size() > m_minDegree) {
			nodeNum += clique->size();
		}
	}

	double realTime;
	ogdf::usedTime(realTime);
#endif

	// Postprocess cliques that do not have the desired size yet.
	postProcessCliques(cliqueList);

#ifdef OGDF_CLIQUE_FINDER_HEURISTIC_DEBUG
	realTime = ogdf::usedTime(realTime);

	int nodeNum2 = 0;
	for (List<node> *clique : cliqueList) {
		if (clique->size() > m_minDegree) {
			nodeNum2 += clique->size();
		}
	}
	if (nodeNum2 > nodeNum) {
		std::cout << "\nNumber cliques before PP: " << numC1
			<< "\nNumber cliques after PP: " << cliqueList.size()
			<< "\nNumber nodes in big cliques before PP: " << nodeNum
			<< "\nNumber nodes in big cliques after PP: " << nodeNum2;
	}
	std::cout << "\nUsed postprocessing time: " << realTime << std::endl;
#endif

#ifdef OGDF_DEBUG
	for (List<node> *clique : cliqueList) {
		OGDF_ASSERT(cliqueOK(*m_pCopy, clique, m_density));
	}
#endif

	// Assign clique numbers and get number of cliques.
	int numberOfCliques = 0;
	for (List<node> *pCand : cliqueList) {
		// The cliques must be big enough.
		if (pCand->size() > m_minDegree) {
			for (node u : *pCand) {
				OGDF_ASSERT(m_copyCliqueNumber[u] < 0);
				m_copyCliqueNumber[u] = numberOfCliques;
			}
			numberOfCliques++;
		}
	}

	// Free the allocated memory.
	for(List<node> *pCl : cliqueList) {
		delete pCl;
	}
	m_usedNode.init();
	delete m_adjOracle;
}

void CliqueFinderHeuristic::postProcessCliques(List<List<node>*> &cliqueList)
{
	if (!m_postProcess) {
		return;
	}

	// We run over all leftover nodes and try to find additional cliques.
	List<node> leftOver;
	List<List<node>*> cliqueAdd;

	// If dense subgraphs are searched, check whether nodes in cliques have high
	// enough connectivity. If not, move them from the clique to leftOver.
	// TODO Best would be to reinsert nodes immediatedly after each found
	// subgraph to allow reuse.
	if (OGDF_GEOM_ET.less(m_density, 1.0)) {
		// For every big clique:
		for (List<node> *pCand : cliqueList) {
			if (pCand->size() > m_minDegree) {
				// Clique nodes are inList.
				NodeArray<bool> inList(*m_pCopy, false);
				for (node u : *pCand) {
					inList[u] = true;
				}

				// For every clique node *itNode:
				ListIterator<node> itNode = pCand->begin();
				while (itNode.valid()) {
					// Get the number of nodes adjacent to *itNode in itCand.
					// adCount < current clique size is possible for m_density < 1.0.
					int adCount = 0;
					for (adjEntry adj : (*itNode)->adjEntries) {
						if (inList[adj->twinNode()]) {
							adCount++;
						}
					}

					// If the connectivity is too small, delete *itNode from
					// clique and insert it into leftOver.
					if (OGDF_GEOM_ET.less(adCount, int(ceil((pCand->size() - 1) * m_density)))) {
						leftOver.pushBack(*itNode);
						m_usedNode[*itNode] = false;
						inList[*itNode] = false;
						ListIterator<node> itDel = itNode;
						++itNode;
						pCand->del(itDel);
					} else {
						++itNode;
					}
				}
			} else {
				break;
			}
		}
	}

	// Delete cliques/dense subgraphs that are too small.
	auto itClique = cliqueList.begin();
	while (itClique.valid()) {
		if ((*itClique)->size() <= m_minDegree) {
			// Remove nodes from clique and put them into leftOver.
			while (!(*itClique)->empty()) {
				node v = (*itClique)->popFrontRet();
				leftOver.pushBack(v);
				OGDF_ASSERT(m_usedNode[v]);
				m_usedNode[v] = false;
			}

			// Delete the clique.
			ListIterator<List<node>*> itDel = itClique;
			delete *itDel;
			++itClique;
			cliqueList.del(itDel);
		} else {
			++itClique;
		}
	}

	// Now we have all left over nodes in list leftOver.
	// Sort leftOver by the heuristic evaluation function.
	leftOver.quicksort(GenericComparer<node, int, false>([&](node v) {
		return evaluate(v);
	}));

	// Go through leftOver nodes, starting at the most qualified ones.
	for (node v : leftOver) {
		// Nodes may be already assigned in earlier iterations.
		if (!m_usedNode[v]) {
			// TODO: This is the same loop as in evaluate and should not be run
			// twice. However, it's not efficient to save the neighbour degree
			// values for every run of evaluate.
			List<node> *neighbours = new List<node>();
			NodeArray<bool> neighbour(*m_pCopy, false);
			NodeArray<int> neighbourDegree(*m_pCopy, 0);

			for (adjEntry adj : v->adjEntries) {
				node w = adj->twinNode();

				if (!m_usedNode[w]) {
					neighbours->pushBack(w);
					neighbour[w] = true;
				}
			}

			// Get degree of each unused v-neighbour w inside the neighbourhood.
			for (node w : *neighbours) {
				OGDF_ASSERT(!m_usedNode[w]);
				OGDF_ASSERT(m_copyCliqueNumber[w] == -1);

				neighbourDegree[w]++; // connected to v

				for (adjEntry adj : w->adjEntries) {
					if (neighbour[adj->twinNode()]) {
						neighbourDegree[w]++;
					}
				}
			}

			// Now we have a (dense) set of nodes and we can delete nodes from
			// neighbours until the set conforms to m_minDegree and m_density.
			neighbours->quicksort(
				GenericComparer<node, int, false>(neighbourDegree)
			);
			findClique(v, *neighbours);

			// We found a dense subgraph!
			// Add v itself to the clique and set m_usedNode.
			if (neighbours->size() >= m_minDegree) {
				neighbours->pushFront(v);

				for (node vUsed : *neighbours) {
					OGDF_ASSERT(!m_usedNode[vUsed]);
					m_usedNode[vUsed] = true;
				}
				cliqueAdd.pushBack(neighbours);
#ifdef OGDF_DEBUG
				OGDF_ASSERT(cliqueOK(*m_pCopy, neighbours, m_density));
#endif
			} else {
				delete neighbours; // automatic clear()
			}
		}
	}

	cliqueList.conc(cliqueAdd);
}

int CliqueFinderHeuristic::evaluate(node v)
{
	// Get all unused neighbours of v.
	// TODO: Use something more efficient than the NodeArray?
	List<node> vNeighbours;
	NodeArray<bool> neighbour(*m_pCopy, false);

	for (adjEntry adj : v->adjEntries) {
		node w = adj->twinNode();

		if (!m_usedNode[w]) {
			vNeighbours.pushBack(w);
			neighbour[w] = true;
		}
	}

	// Run through the neighbourhood of v and try to find a triangle
	// path back (triangles over neighbours are mandatory for cliques).
	// Count every triangle (twice).
	int triangleCount = 0;
	for (node w : vNeighbours) {
		for (adjEntry adj : w->adjEntries) {
			if (neighbour[adj->twinNode()]) {
				triangleCount++;
			}
		}
	}

	return triangleCount;
}


void CliqueFinderHeuristic::findClique(node v, List<node> &neighbours)
{
	OGDF_ASSERT(!m_usedNode[v]);

	if (v->degree() < m_minDegree) {
		neighbours.clear();
	}

	List<node> clique; // Used to check clique criteria.
	clique.pushBack(v); // v must be in the clique.

	// {v, first neighbour} is a clique.
	ListIterator<node> itNode = neighbours.begin();
	if (itNode.valid()) {
		clique.pushBack(*itNode);
		++itNode;
	}

	while (itNode.valid()) {
		// Remove neighbours that cannot be part of the clique.
		if ((*itNode)->degree() < int(ceil(m_density * m_minDegree)) ||
			!allAdjacent(*itNode, &clique)) {
			ListIterator<node> itDel = itNode;
			++itNode;
			neighbours.del(itDel);
		} else {
			clique.pushBack(*itNode);
			++itNode;
		}
	}
}


inline bool CliqueFinderHeuristic::allAdjacent(node v, List<node>* vList) const
{
	if (vList->size() == 0) {
		return true;
	}

	// If v is not adjacent to enough nodes, return false.
	int threshold = int(ceil(max(1.0, vList->size() * m_density)));
	if (v->degree() < threshold) {
		return false;
	}

	// Count incident nodes of v that are in vList.
	int adjCount = 0;
	for (node inList : *vList) {
		if (m_adjOracle->adjacent(v, inList)) {
			adjCount++;
		}
	}

	return adjCount >= threshold;
}

void CliqueFinderHeuristic::preProcess()
{
	int relMinDegree = int(ceil(m_minDegree * m_density));

	// Get nodes with degree < relMinDegree.
	ArrayBuffer<node> lowDegNodes;
	for (node v : m_pCopy->nodes) {
		if (v->degree() < relMinDegree) {
			lowDegNodes.push(v);
		}
	}

	// For each low degree node v, push its low degree neighbours and delete it.
	while (!lowDegNodes.empty()) {
		node v = lowDegNodes.popRet();
		for (adjEntry adj : v->adjEntries) {
			node u = adj->twinNode();
			// Push u if removing (vu) lowers its degree below relMinDegree.
			// m_pCopy is simple, so we can ignore self-loops/multi-edges.
			if (u->degree() == relMinDegree) {
				lowDegNodes.push(u);
			}
		}

		m_pCopy->delNode(v);
	}
}

}
