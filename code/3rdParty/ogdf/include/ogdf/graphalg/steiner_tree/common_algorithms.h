/** \file
 * \brief Algorithms used by at least two functions of Steiner tree
 *  code or its internal helpers.
 *
 * \author Stephan Beyer
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

#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>
#include <ogdf/graphalg/MinSteinerTreeTakahashi.h>

//#define OGDF_COMMON_ALG_FIND_BEST_TAKAHASHI_ROOT


namespace ogdf {
namespace steiner_tree {

/*!
 * \brief Given an edge-weighted tree, builds an auxiliary arborescence
 *  where each arc of the input tree
 *  is a node in the arborescence. The weight of each node is at
 *  least the weight of its children.
 *  The construction algorithm takes time O(n log n).
 * @return root node of the arborescence
 */
template<typename T>
node buildHeaviestEdgeInComponentTree(
		const EdgeWeightedGraphCopy<T> &inputTree, //!< the input tree
		NodeArray<node> &externalNodes, //!< the resulting mapping from input nodes to arborescence leaves (must be nullptr'ed in input!)
		NodeArray<edge> &treeEdge, //!< the resulting mapping from each (inner) node of the arborescence to an edge in the input tree
		Graph &outputTree) //!< the output arborescence
{
	// sort edges by weight
	Array<Prioritized<edge, T>> sortEdges(inputTree.numberOfEdges());
	int i = 0;
	for(edge e : inputTree.edges) {
		sortEdges[i] = Prioritized<edge,T>(e, inputTree.weight(e));
		++i;
	}
	sortEdges.quicksort();

	// insert edges into forest (which in the end makes up a tree)
	NodeArray<node *> root(outputTree);
	List<node *> garbage;
	node edgeNode = nullptr;
	for (i = 0; i < inputTree.numberOfEdges(); ++i) {
		edgeNode = outputTree.newNode();
		edge e = sortEdges[i].item();
		treeEdge[edgeNode] = e;

		node u = e->source();
		node v = e->target();
		if (externalNodes[u]) {
			node *uRoot = root[externalNodes[u]];
			OGDF_ASSERT(uRoot);
			while (root[*uRoot] != uRoot) {
				*uRoot = *root[*uRoot];
				uRoot = root[*uRoot];
				OGDF_ASSERT(uRoot);
			}
			outputTree.newEdge(edgeNode, *uRoot);
			root[edgeNode] = uRoot;
			if (externalNodes[v]) {
				node *vRoot = root[externalNodes[v]];
				OGDF_ASSERT(vRoot);
				while (root[*vRoot] != vRoot) {
					*vRoot = *root[*vRoot];
					vRoot = root[*vRoot];
					OGDF_ASSERT(vRoot);
				}
				outputTree.newEdge(edgeNode, *vRoot);
				*vRoot = edgeNode;
			} else {
				externalNodes[v] = edgeNode;
			}
		} else {
			externalNodes[u] = edgeNode;
			if (externalNodes[v]) {
				node *vRoot = root[externalNodes[v]];
				OGDF_ASSERT(vRoot);
				while (root[*vRoot] != vRoot) {
					*vRoot = *root[*vRoot];
					vRoot = root[*vRoot];
					OGDF_ASSERT(vRoot);
				}
				outputTree.newEdge(edgeNode, *vRoot);
				root[edgeNode] = vRoot;
			} else {
				root[edgeNode] = new node;
				garbage.pushBack(root[edgeNode]);
				externalNodes[v] = edgeNode;
			}
		}
		*root[edgeNode] = edgeNode;
	}
	OGDF_ASSERT(edgeNode);

	// garbage collect
	for(node *p : garbage) {
		delete p;
	}

	return edgeNode;
}


/*!
 * \brief Updates the Steiner tree by deleting save edges, removing all direct connections
 * between the terminals of the contracted triple and connecting them through 0-cost edges
 * @param t The contracted triple
 * @param st The Steiner Tree
 * @param save0 One of the three save edges of the triple
 * @param save1 One of the three save edges of the triple
 * @param save2 One of the three save edges of the triple
 * @param ne0 One of the new zero-weight edges
 * @param ne1 One of the new zero-weight edges
 */
template <typename T>
void contractTripleInSteinerTree(const Triple<T> &t, EdgeWeightedGraphCopy<T> &st, edge save0, edge save1, edge save2, edge &ne0, edge &ne1)
{
	if (save0 == save1) {
		st.delEdge(save1);
		st.delEdge(save2);
	} else {
		st.delEdge(save0);
		st.delEdge(save1);
	}
	ne0 = st.newEdge(st.copy(t.s0()), st.copy(t.s1()), 0);
	ne1 = st.newEdge(st.copy(t.s0()), st.copy(t.s2()), 0);
}

template <typename T>
inline void contractTripleInSteinerTree(const Triple<T> &t, EdgeWeightedGraphCopy<T> &st, edge e0, edge e1, edge e2)
{
	edge ne0, ne1;
	contractTripleInSteinerTree(t, st, e0, e1, e2, ne0, ne1);
}


template<typename T>
T obtainFinalSteinerTree(const EdgeWeightedGraph<T> &G, const NodeArray<bool> &isTerminal, const NodeArray<bool> &isOriginalTerminal, EdgeWeightedGraphCopy<T> *&finalSteinerTree)
{
	List<node> terminals;
	MinSteinerTreeModule<T>::getTerminals(terminals, G, isTerminal);

	finalSteinerTree = nullptr;
	MinSteinerTreeTakahashi<T> mstt;
#ifdef OGDF_COMMON_ALG_FIND_BEST_TAKAHASHI_ROOT
	// find minimum Steiner tree of G among Takahashi approximations for each start node
	T bestMstWeight = std::numeric_limits<T>::max();
	for (node v : terminals) {
		EdgeWeightedGraphCopy<T> *tmpSteinerTree;
		T tmpMstWeight = mstt.call(G, terminals, isTerminal, isOriginalTerminal, tmpSteinerTree, v);
		if (tmpMstWeight < bestMstWeight) {
			bestMstWeight = tmpMstWeight;
			delete finalSteinerTree;
			finalSteinerTree = tmpSteinerTree;
		} else {
			delete tmpSteinerTree;
		}
	}
	return bestMstWeight;
#else
	return mstt.call(G, terminals, isTerminal, isOriginalTerminal, finalSteinerTree);
#endif
}

}
}
