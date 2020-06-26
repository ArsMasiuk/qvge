/*! \file
 * \brief Definition of the ogdf::steiner_tree:HeavyPathDecomposition class template
 *
 * \author Mihai Popa
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

#include <vector>
#include <ogdf/basic/Math.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>

namespace ogdf {
namespace steiner_tree {

/**
 * An implementation of the heavy path decomposition on trees.
 * It contains very specific queries used by reductions.
 */
template<typename T>
class HeavyPathDecomposition {
private:
	const EdgeWeightedGraphCopy<T> &tree; ///< constant ref to the tree to be decomposed
	List<node> terminals; ///< list of terminals of the desc tree
	const NodeArray<bool> isTerminal; ///< isTerminal of the desc tree

	std::vector<std::vector<node>> chains; ///< list of chains of nodes corresponding to the decomposition
	std::vector<std::vector<node>> chainsOfTerminals; ///< list of chains only of terminals corresponding to the decomposition
	NodeArray<int> chainOfNode; ///< the index of a node's chain
	NodeArray<int> positionOnChain; ///< position of a node on his chain
	NodeArray<int> weightOfSubtree; ///< weight of the subtree rooted in one node
	NodeArray<int> nodeLevel; ///< the level of a node in the tree
	NodeArray<T> distanceToRoot; ///< the length of the edge to his father
	NodeArray<node> closestSteinerAncestor; ///< the highest-level Steiner ancestor of the current node
	std::vector<node> fatherOfChain; ///< the first node from bottom up that does not belong to the chain

	std::vector<std::vector<T>> longestDistToSteinerAncestorOnChain; ///< the max of the interval 0..i for every i on chains
	std::vector<std::vector<T>> longestDistToSteinerAncestorSegTree; ///< segment tree for segment maxs on every chain

	/*!
	 * \brief builds a max segment tree on the baseArray
	 * - Time: O(n)
	 */
	void buildMaxSegmentTree(std::vector<T> &segmentTree, const int nodeIndex, const int left, const int right, const std::vector<T> &baseArray) const
	{
		if (left == right) { // leaf
			segmentTree[nodeIndex] = baseArray[left];
			return;
		}

		int middle = (left+right)>>1;
		int leftNodeIndex = nodeIndex+nodeIndex+1;
		int rightNodeIndex = leftNodeIndex + 1;

		buildMaxSegmentTree(segmentTree, leftNodeIndex, left, middle, baseArray);
		buildMaxSegmentTree(segmentTree, rightNodeIndex, middle+1, right, baseArray);

		segmentTree[nodeIndex] = max(segmentTree[leftNodeIndex], segmentTree[rightNodeIndex]);
	}
	/*!
	 * \brief extracts the maximum on the required interval
	 * - Time: O(log n)
	 */
	T getMaxSegmentTree(const std::vector<T> &segmentTree, const int nodeIndex, const int left, const int right, const int queryLeft, const int queryRight) const
	{
		if (queryLeft > queryRight
		 || left > queryRight
		 || queryLeft > right) {
			return 0;
		}

		if (queryLeft <= left
		 && right <= queryRight) { // included
			return segmentTree[nodeIndex];
		}

		int middleIndex = (left + right)>>1;
		int leftNodeIndex = nodeIndex+nodeIndex+1;
		int rightNodeIndex = leftNodeIndex + 1;

		T maxValue(0);
		if (queryLeft <= middleIndex) {
			Math::updateMax(maxValue, getMaxSegmentTree(segmentTree, leftNodeIndex, left, middleIndex, queryLeft, queryRight));
		}
		if (queryRight > middleIndex) {
			Math::updateMax(maxValue, getMaxSegmentTree(segmentTree, rightNodeIndex, middleIndex+1, right, queryLeft, queryRight));
		}

		return maxValue;
	}

	/*!
	 * computes the sum of all edges on the path from v to ancestor
	 * v must be in ancestor's subtree
	 */
	T distanceToAncestor(node v, node ancestor) const
	{
		if (ancestor == nullptr) {
			return distanceToRoot[v];
		}
		return distanceToRoot[v] - distanceToRoot[ancestor];
	}

	/*!
	 * \brief creates for every chain an array with the maximum of every prefix of a fictive array
	 * which keeps for every node the sum of the edges on the path from it to its closest
	 * terminal ancestor
	 */
	void computeLongestDistToSteinerAncestorOnChain()
	{
		longestDistToSteinerAncestorOnChain.resize((int)chains.size());
		for (int i = 0; i < (int)chains.size(); ++i) {
			longestDistToSteinerAncestorOnChain[i].resize((int)chains[i].size());
			for (int j = 0; j < (int)chains[i].size(); ++j) {
				longestDistToSteinerAncestorOnChain[i][j] = distanceToAncestor(chains[i][j], closestSteinerAncestor[chains[i][j]]);
				if (j > 0) {
					Math::updateMax(longestDistToSteinerAncestorOnChain[i][j], longestDistToSteinerAncestorOnChain[i][j-1]);
				}
			}
		}
	}

	/*!
	 * \brief creates for every chain a segment tree built on a fictive array
	 * which keeps for every node the sum of the edges on the path from it to its closest
	 * terminal ancestor
	 */
	void computeLongestDistToSteinerAncestorSegTree()
	{
		longestDistToSteinerAncestorSegTree.resize((int)chains.size());
		for (int i = 0; i < (int)chains.size(); ++i) {
			longestDistToSteinerAncestorSegTree[i].resize(4*(int)chains[i].size());

			std::vector<T> distanceToSteinerAncestor_byPosition;
			distanceToSteinerAncestor_byPosition.resize(chains[i].size());
			for (int j = 0; j < (int)chains[i].size(); ++j) {
				distanceToSteinerAncestor_byPosition[j] = distanceToAncestor(chains[i][j], closestSteinerAncestor[chains[i][j]]);
			}

			buildMaxSegmentTree(longestDistToSteinerAncestorSegTree[i], 0, 0, (int)chains[i].size()-1, distanceToSteinerAncestor_byPosition);
		}
	}

	/*!
	 * \brief performs the heavy path decomposition on the tree belonging to the class
	 * updates the fields of the class
	 */
	void dfsHeavyPathDecomposition(node v, node closestSteinerUpNode)
	{
		weightOfSubtree[v] = 1;
		node heaviestSon = nullptr;
		closestSteinerAncestor[v] = closestSteinerUpNode;

		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node son = adj->twinNode();

			if (weightOfSubtree[son] != 0) { // the parent
				continue;
			}

			nodeLevel[son] = nodeLevel[v] + 1;
			distanceToRoot[son] = distanceToRoot[v] + tree.weight(e);

			dfsHeavyPathDecomposition(son, v);

			fatherOfChain[chainOfNode[son]] = v;
			weightOfSubtree[v] += weightOfSubtree[son];
			if (heaviestSon == nullptr
			 || weightOfSubtree[heaviestSon] < weightOfSubtree[son]) {
				heaviestSon = son;
			}
		}

		if (heaviestSon == nullptr) { // it's leaf => new chain
			OGDF_ASSERT((v->degree() <= 1));

			std::vector<node>new_chain;
			chains.push_back(new_chain);
			chainsOfTerminals.push_back(new_chain);

			fatherOfChain.push_back(nullptr);
			chainOfNode[v] = (int)chains.size()-1;
		} else {
			chainOfNode[v] = chainOfNode[heaviestSon];
		}

		chains[chainOfNode[v]].push_back(v);
		chainsOfTerminals[chainOfNode[v]].push_back(v);
		positionOnChain[v] = (int)chains[chainOfNode[v]].size()-1;
	}

	/*!
	 * \brief performs a binary search on chainOfTerminals,
	 * searches for the closest node to the root on chainOfTerminals that sits below v
	 * - Time: O(log n)
	 */
	node binarySearchUpmostTerminal(node v, const std::vector<node> &chainOfTerminals) const
	{
		int left = 0, middle, right = (int)chainOfTerminals.size()-1;
		while (left <= right) {
			middle = (left+right)>>1;

			if (nodeLevel[chainOfTerminals[middle]] >= nodeLevel[v]) {
				right = middle-1;
			} else {
				left = middle+1;
			}
		}

		if (left == (int)chainOfTerminals.size()) {
			return nullptr;
		}
		return chainOfTerminals[left];
	}

	/*!
	 * \brief computes for the path from x to ancestor
	 * the maximum distance (sum of edges) between any two consecutive terminals using the hpd
	 * updates it in longestPathDistance
	 * also puts in fromLowestToAncestor the sum of edges from the last terminal found to the ancestor
	 * (the last terminal is special since its Steiner ancestor is ancestor for ancestor as well, so we would consider
	 * a wrong part of the path
	 * - Time: O(log n)
	 */
	void computeBottleneckOnBranch(node x, node ancestor, T &longestPathDistance, T &fromLowestToAncestor) const
	{
		node upmostTerminal = x;
		while (closestSteinerAncestor[chains[chainOfNode[x]][0]] != nullptr
		    && nodeLevel[closestSteinerAncestor[chains[chainOfNode[x]][0]]] >= nodeLevel[ancestor]) {
			Math::updateMax(longestPathDistance, longestDistToSteinerAncestorOnChain[chainOfNode[x]][positionOnChain[x]]);

			if (!chainsOfTerminals[chainOfNode[x]].empty()
			 && nodeLevel[chainsOfTerminals[chainOfNode[x]][0]] <= nodeLevel[x]) {
				upmostTerminal = chainsOfTerminals[chainOfNode[x]][0];
			}
			x = fatherOfChain[chainOfNode[x]];
		}

		// search the upmost terminal on the current chain that has level >= level[ancestor]
		node upmostTerminalLastChain = binarySearchUpmostTerminal(ancestor, chainsOfTerminals[chainOfNode[x]]);
		if (nodeLevel[upmostTerminalLastChain] > nodeLevel[x]) {
			upmostTerminalLastChain = nullptr;
		}
		if (upmostTerminalLastChain != nullptr) {
			upmostTerminal = upmostTerminalLastChain;
		}

		if (upmostTerminalLastChain != nullptr) {
			Math::updateMax(longestPathDistance,
			  getMaxSegmentTree(longestDistToSteinerAncestorSegTree[chainOfNode[x]], 0, 0,
			                    static_cast<int>(chains[chainOfNode[x]].size()) - 1,
			                    positionOnChain[upmostTerminalLastChain] + 1,
			                    positionOnChain[x]));
		}

		fromLowestToAncestor = distanceToAncestor(upmostTerminal, ancestor);
	}

public:
	HeavyPathDecomposition(const EdgeWeightedGraphCopy<T> &treeEWGraphCopy)
	  : tree{treeEWGraphCopy},
	    chainOfNode{tree, -1},
	    positionOnChain{tree, -1},
	    weightOfSubtree{tree, 0},
	    nodeLevel{tree, 0},
	    distanceToRoot{tree, 0},
	    closestSteinerAncestor{tree, nullptr} {
		OGDF_ASSERT(!tree.empty());
		node root = tree.firstNode();

		dfsHeavyPathDecomposition(root, nullptr);
		fatherOfChain[chainOfNode[root]] = nullptr;

		// reverse the obtained chains
		int numberOfChains = static_cast<int>(chains.size());
		for (int i = 0; i < numberOfChains; ++i) {
			reverse(chains[i].begin(), chains[i].end());
			reverse(chainsOfTerminals[i].begin(), chainsOfTerminals[i].end());
			for (node v : chains[i]) {
				positionOnChain[v] = (int)chains[i].size()-1-positionOnChain[v];
			}
		}

		computeLongestDistToSteinerAncestorOnChain();
		computeLongestDistToSteinerAncestorSegTree();
	}

	/*!
	 * \brief computes the lowest common ancestor of nodes x and y using the hpd
	 * - Time: O(log n)
	 */
	node lowestCommonAncestor(node x, node y) const
	{
		while (chainOfNode[x] != chainOfNode[y]) {
			int xlevelOfFatherOfChain = fatherOfChain[chainOfNode[x]] != nullptr ? nodeLevel[fatherOfChain[chainOfNode[x]]] : -1;
			int ylevelOfFatherOfChain = fatherOfChain[chainOfNode[y]] != nullptr ? nodeLevel[fatherOfChain[chainOfNode[y]]] : -1;

			if (xlevelOfFatherOfChain >= ylevelOfFatherOfChain) {
				x = fatherOfChain[chainOfNode[x]];
			} else {
				y = fatherOfChain[chainOfNode[y]];
			}
		}

		if (nodeLevel[x] <= nodeLevel[y]) {
			return x;
		}
		return y;
	}

	/*!
	 * \brief computes in the bottleneck distance between terminals x and y
	 * - Time: O(log n)
	 */
	T getBottleneckSteinerDistance(node x, node y) const
	{
		T xLongestPathDistance = 0, yLongestPathDistance = 0;
		T xFromLowestToLCA = 0, yFromLowestToLCA = 0;
		node xyLowestCommonAncestor = lowestCommonAncestor(x, y);

		computeBottleneckOnBranch(x, xyLowestCommonAncestor, xLongestPathDistance, xFromLowestToLCA);
		computeBottleneckOnBranch(y, xyLowestCommonAncestor, yLongestPathDistance, yFromLowestToLCA);

		T maxValue = max(xLongestPathDistance, yLongestPathDistance);
		Math::updateMax(maxValue, xFromLowestToLCA+yFromLowestToLCA);

		return maxValue;
	}
};

}
}
