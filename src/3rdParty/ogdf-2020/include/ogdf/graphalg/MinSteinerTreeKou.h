/** \file
 * \brief Declaration and implementation of the class computing a
 * 		  2(1-1/l) minimum Steiner tree approximation according
 * 		  to the algorithm of Kou et al.
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
#include <ogdf/graphalg/MinSteinerTreeModule.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>

#include <ogdf/graphalg/Dijkstra.h>

namespace ogdf {

/*!
 * \brief This class implements the Minimum Steiner Tree 2-approximation algorithm by Kou et al.
 *
 * @ingroup ga-steiner
 *
 * This implementation is based on:
 *
 * (L. Kou, G. Markowsky, L. Berman, A fast algorithm for Steiner trees, Acta Informatica,
 * volumne 15, number 2, pages 141-145, 1981)
 */
template<typename T>
class MinSteinerTreeKou : public MinSteinerTreeModule<T> {
public:
	MinSteinerTreeKou() { }

	virtual ~MinSteinerTreeKou() { }

protected:
	/*!
	 * \brief Builds a minimum Steiner tree given a weighted graph and a list of terminals
	 * @param G The weighted input graph
	 * @param terminals The list of terminal nodes
	 * @param isTerminal A bool array of terminals
	 * @param finalSteinerTree The final Steiner tree
	 * @return The objective value (sum of edge costs) of the final Steiner tree
	 */
	virtual T computeSteinerTree(
		const EdgeWeightedGraph<T> &G,
		const List<node> &terminals,
		const NodeArray<bool> &isTerminal,
		EdgeWeightedGraphCopy<T> *&finalSteinerTree) override;

	/*!
	 * \brief Builds a complete terminal graph
	 * @param wG the original graph
	 * @param terminals list of terminals
	 * @param predecessor stores for each edge in the complete terminal graph the according path in the original graph
	 * @param completeTerminalGraph the resulting complete terminal graph
	 */
	void calculateCompleteGraph(const EdgeWeightedGraph<T> &wG, const List<node> &terminals, EdgeArray<List<edge>> &predecessor,
			EdgeWeightedGraphCopy<T> &completeTerminalGraph);

	/*!
	 * \brief Swaps an edge in the complete terminal graph with the corresponding shortest path in the original graph
	 * @param completeTerminalGraph the complete terminal graph
	 * @param ssspPred contains for each edge in the complete terminal graph the corresponding path in the original graph
	 * @param mstPred predecessor data structure of a minimum terminal spanning tree
	 * @param finalSteinerTree the resulting Steiner tree
	 * @param wG the original graph
	 */
	void reinsertShortestPaths(const EdgeWeightedGraphCopy<T> &completeTerminalGraph, const EdgeArray<List<edge>> &ssspPred,
			const NodeArray<edge> &mstPred, EdgeWeightedGraphCopy<T> &finalSteinerTree, const EdgeWeightedGraph<T> &wG);

	/*!
	 * \brief Inserts a shortest path corresponding to an edge in the complete terminal graph
	 * @param ssspPred contains for each edge in the complete terminal graph the corresponding path in the original graph
	 * @param finalSteinerTree the resulting Steiner tree
	 * @param wG the original graph
	 */
	void insertPath(const List<edge> &ssspPred, EdgeWeightedGraphCopy<T> &finalSteinerTree, const EdgeWeightedGraph<T> &wG);
};

template<typename T>
T MinSteinerTreeKou<T>::computeSteinerTree(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, EdgeWeightedGraphCopy<T> *&finalSteinerTree)
{
	EdgeWeightedGraphCopy<T> completeTerminalGraph;
	completeTerminalGraph.createEmpty(G);

	for (node v : terminals) {
		completeTerminalGraph.newNode(v);
	}

	List<edge> steinerTreeEdges;
	EdgeArray<List<edge>> ssspPred(completeTerminalGraph);

	calculateCompleteGraph(G, terminals, ssspPred, completeTerminalGraph);

	NodeArray<edge> mstPred(completeTerminalGraph);
	computeMinST(completeTerminalGraph, completeTerminalGraph.edgeWeights(), mstPred);

	finalSteinerTree = new EdgeWeightedGraphCopy<T>();
	finalSteinerTree->createEmpty(G);

	reinsertShortestPaths(completeTerminalGraph, ssspPred, mstPred, *finalSteinerTree, G);

	T mstWeight = makeMinimumSpanningTree(*finalSteinerTree, finalSteinerTree->edgeWeights());
	mstWeight -= MinSteinerTreeModule<T>::pruneAllDanglingSteinerPaths(*finalSteinerTree, isTerminal);

	return mstWeight;
}

template<typename T>
void MinSteinerTreeKou<T>::calculateCompleteGraph(const EdgeWeightedGraph<T> &wG, const List<node> &terminals, EdgeArray<List<edge>> &predecessor, EdgeWeightedGraphCopy<T> &completeTerminalGraph)
{
	Dijkstra<T> sssp;
	for (node u = completeTerminalGraph.firstNode(); u->succ(); u = u->succ()) {
		NodeArray<T> d;
		NodeArray<edge> pi;
		sssp.call(wG, wG.edgeWeights(), completeTerminalGraph.original(u), pi, d);
		for (node v = u->succ(); v; v = v->succ()) {
			edge e = completeTerminalGraph.newEdge(u, v, d[completeTerminalGraph.original(v)]);
			predecessor[e].clear();
			for (node t = completeTerminalGraph.original(v); pi[t]; t = pi[t]->opposite(t)) {
				predecessor[e].pushBack(pi[t]);
			}
		}
	}
}

template<typename T>
void MinSteinerTreeKou<T>::reinsertShortestPaths(const EdgeWeightedGraphCopy<T> &completeTerminalGraph,
                                                 const EdgeArray<List<edge>> &ssspPred,
                                                 const NodeArray<edge> &mstPred,
                                                 EdgeWeightedGraphCopy<T> &finalSteinerTree,
                                                 const EdgeWeightedGraph<T> &wG)
{
	for(node u : completeTerminalGraph.nodes) {
		if (mstPred[u]) {
			insertPath(ssspPred[mstPred[u]], finalSteinerTree, wG);
		}
	}
}

template<typename T>
void MinSteinerTreeKou<T>::insertPath(const List<edge> &ssspPred, EdgeWeightedGraphCopy<T> &finalSteinerTree, const EdgeWeightedGraph<T> &wG)
{
	for (edge e : ssspPred)
	{
		if (e != nullptr && finalSteinerTree.chain(e).size() == 0)
		{
			node edgeSource = e->source();
			node edgeTarget = e->target();

			node stSource = finalSteinerTree.copy(edgeSource);
			if (stSource == nullptr) {
				stSource = finalSteinerTree.newNode(edgeSource);
			}

			node stTarget = finalSteinerTree.copy(edgeTarget);
			if (stTarget == nullptr) {
				stTarget = finalSteinerTree.newNode(edgeTarget);
			}

			if (e->source() == finalSteinerTree.original(stSource)) {
				edge newE = finalSteinerTree.newEdge(stSource, stTarget, wG.weight(e));
				finalSteinerTree.setEdge(e, newE);
			}
		}
	}
}

}
