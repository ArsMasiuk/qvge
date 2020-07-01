/** \file
 * \brief Implementation of the 2(1-1/l)-approximation algorithm for
 * 		  the minimum Steiner tree problem by Matsuyama and Takahashi
 *
 * \author Matthias Woste
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

#include <ogdf/basic/List.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>
#include <ogdf/graphalg/MinSteinerTreeModule.h>
#include <ogdf/basic/extended_graph_alg.h>

namespace ogdf {

/**
 * This class implements the minimum Steiner tree 2-approximation algorithm
 * by Takahashi and Matsuyama with improvements proposed by Poggi de Aragao et al.
 *
 * @ingroup ga-steiner
 *
 * This implementation is based on:
 *
 * (H. Takahashi and A. Matsuyama, An approximate solution for the Steiner problem in graphs, Math. Japonica,
 * volume 24, number 6, pages 573-577, 1980)
 *
 * (M. Poggi de Aragao, C. Riberiro, E. Uchoa, R. Werneck, Hybrid Local Search for the Steiner Problem in Graphs,
 * MIC 2001, pages 429-433, 2001)
 */
template<typename T>
class MinSteinerTreeTakahashi: public MinSteinerTreeModule<T> {
public:
	MinSteinerTreeTakahashi() { }

	virtual ~MinSteinerTreeTakahashi() { }

	/**
	 * An extended call method with specific start node.
	 *
	 * You should only call this method when there is more than one terminal.
	 *
	 * @see MinSteinerTreeModule::call
	 */
	virtual T call(const EdgeWeightedGraph<T> &G,
	               const List<node> &terminals,
	               const NodeArray<bool> &isTerminal,
	               EdgeWeightedGraphCopy<T> *&finalSteinerTree,
	               const node startNode)
	{
		return call(G, terminals, isTerminal, isTerminal, finalSteinerTree, startNode);
	}

	/**
	 * An extended call method with intermediate and final (original) terminals.
	 *
	 * You should only call this method when there is more than one terminal.
	 *
	 * @see MinSteinerTreeModule::call
	 */
	virtual T call(const EdgeWeightedGraph<T> &G,
	               const List<node> &terminals,
	               const NodeArray<bool> &isTerminal,
	               const NodeArray<bool> &isOriginalTerminal,
	               EdgeWeightedGraphCopy<T> *&finalSteinerTree)
	{
		return call(G, terminals, isTerminal, isOriginalTerminal, finalSteinerTree, terminals.front());
	}

	using MinSteinerTreeModule<T>::call;

	/*!
	 * An extended call method with intermediate and final (original) terminal nodes
	 * and a specific start node.
	 *
	 * You should only call this method when there is more than one terminal.
	 *
	 * @see MinSteinerTreeModule::call
	 */
	virtual T call(const EdgeWeightedGraph<T> &G,
	               const List<node> &terminals,
	               const NodeArray<bool> &isTerminal,
	               const NodeArray<bool> &isOriginalTerminal,
	               EdgeWeightedGraphCopy<T> *&finalSteinerTree,
	               const node startNode);

protected:
	virtual T computeSteinerTree(
		const EdgeWeightedGraph<T> &G,
		const List<node> &terminals,
		const NodeArray<bool> &isTerminal,
		EdgeWeightedGraphCopy<T> *&finalSteinerTree) override
	{
		return call(G, terminals, isTerminal, isTerminal, finalSteinerTree, terminals.front());
	}

	/*!
	 * Modified Dijkstra algorithm to solve the Minimum Steiner Tree problem
	 * @param wG the original graph
	 * @param intermediateTerminalSpanningTree intermediate terminal spanning tree
	 * @param s source node to start from
	 * @param numberOfTerminals number of terminal nodes
	 * @param isTerminal terminal incivende vector
	 * @return the weight of the intermediateTerminalSpanningTree
	 */
	T terminalDijkstra(const EdgeWeightedGraph<T> &wG,
	                   EdgeWeightedGraphCopy<T> &intermediateTerminalSpanningTree,
	                   const node s,
	                   int numberOfTerminals,
	                   const NodeArray<bool> &isTerminal);
};

template<typename T>
T MinSteinerTreeTakahashi<T>::call(const EdgeWeightedGraph<T> &G,
                                   const List<node> &terminals,
                                   const NodeArray<bool> &isTerminal,
                                   const NodeArray<bool> &isOriginalTerminal,
                                   EdgeWeightedGraphCopy<T> *&finalSteinerTree,
                                   const node startNode)
{
	OGDF_ASSERT(isConnected(G));

	EdgeWeightedGraphCopy<T> terminalSpanningTree;
	terminalSpanningTree.createEmpty(G);
	terminalDijkstra(G, terminalSpanningTree, startNode, terminals.size(), isTerminal);

	finalSteinerTree = new EdgeWeightedGraphCopy<T>(G);
	for(node u : G.nodes) {
		if (!terminalSpanningTree.copy(u)) {
			finalSteinerTree->delNode(finalSteinerTree->copy(u));
		}
	}

	T mstWeight = makeMinimumSpanningTree(*finalSteinerTree, finalSteinerTree->edgeWeights());
	mstWeight -= MinSteinerTreeModule<T>::pruneAllDanglingSteinerPaths(*finalSteinerTree, isOriginalTerminal);

	return mstWeight;
}

template<typename T>
T MinSteinerTreeTakahashi<T>::terminalDijkstra(const EdgeWeightedGraph<T> &wG,
		EdgeWeightedGraphCopy<T> &intermediateTerminalSpanningTree, const node s, int numberOfTerminals,
		const NodeArray<bool> &isTerminal)
{
	NodeArray<edge> predecessor(wG, nullptr);
	NodeArray<T> distance(wG, std::numeric_limits<T>::max());
	distance[s] = 0;
	NodeArray<T> bestDistance(wG, std::numeric_limits<T>::max());
	bestDistance[s] = 0;
	NodeArray<bool> isInQueue(wG, true);

	PrioritizedMapQueue<node, T> queue(wG); //priority queue
	for (node v : wG.nodes) {
		queue.push(v, distance[v]);
	}

	T mstWeight = 0;
	int terminalsFound = 1;
	while (!queue.empty() && terminalsFound < numberOfTerminals) {
		node v = queue.topElement();
		queue.pop();
		isInQueue[v] = false;
		bestDistance[v] = distance[v];
		if (isTerminal[v]
		 && distance[v] > 0) {
			++terminalsFound;
			// insert path from new node to old tree
			node tmpT = intermediateTerminalSpanningTree.newNode(v);
			while (distance[v] > 0) {
				distance[v] = 0;
				queue.push(v, distance[v]);
				isInQueue[v] = true;
				const edge e = predecessor[v];
				OGDF_ASSERT(e);
				const node w = e->opposite(v);
				node tmpS = intermediateTerminalSpanningTree.copy(w);
				if (!tmpS) {
					tmpS = intermediateTerminalSpanningTree.newNode(w);
				}
				edge tmpE;
				if (e->target() == v) {
					tmpE = intermediateTerminalSpanningTree.newEdge(tmpS, tmpT, wG.weight(e));
				} else {
					tmpE = intermediateTerminalSpanningTree.newEdge(tmpT, tmpS, wG.weight(e));
				}
				mstWeight += wG.weight(e);
				intermediateTerminalSpanningTree.setEdge(e, tmpE);
				tmpT = tmpS;
				v = w;
			}
		} else { // !isTerminal[v] || distance[v] == 0
			for(adjEntry adj : v->adjEntries) {
				const node w = adj->twinNode();
				const edge e = adj->theEdge();
				if (distance[w] > distance[v] + wG.weight(e)
				 && bestDistance[w] >= distance[w]) {
					distance[w] = distance[v] + wG.weight(e);
					if (!isInQueue[w]) {
						queue.push(w, distance[w]);
						isInQueue[w] = true;
					} else {
						queue.decrease(w, distance[w]);
					}
					predecessor[w] = e;
				}
			}
		}
	}
	return mstWeight;
}

}
