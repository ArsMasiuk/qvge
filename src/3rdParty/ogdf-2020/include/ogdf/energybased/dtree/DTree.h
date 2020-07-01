/** \file
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

#include <algorithm>
#include <ogdf/energybased/dtree/utils.h>

namespace ogdf {
namespace energybased {
namespace dtree {

//! Implentation of the reduced quadtree for Dim dimensions
template<typename IntType, int Dim>
class DTree
{
public:

	//! the maximum number of children per node = 2^d
	static constexpr int MaxNumChildrenPerNode = (1 << Dim);

	//! constructor
	explicit DTree(int numPoints)
	: m_maxLevel((sizeof(IntType) << 3) + 1)
	, m_numPoints(0)
	, m_rootIndex(-1)
	{
		allocate(numPoints);
	}

	//! destructor
	~DTree()
	{
		deallocate();
	}

	//! The point class with integer coords
	struct Point
	{
		IntType x[Dim];
	};

	//! The entry in the sorted order for a point
	struct MortonEntry
	{
		IntType mortonNr[Dim]; //!< the morton number of the point
		int ref; //!< index in the original point order

		//! less comparator for sort
		bool operator<(const MortonEntry& other) const
		{
			return mortonComparerLess<IntType, Dim>(mortonNr, other.mortonNr);
		}

		//! equal comparer for the construction algorithm
		bool operator==(const MortonEntry& other) const
		{
			return mortonComparerEqual<IntType, Dim>(mortonNr, other.mortonNr);
		}
	};

	//! The node class
	struct Node
	{
		// quadtree related stuff
		int level; //!< the level of the node in a complete quadtree
		int next; //!< the next node on the same layer (leaf or inner node layer)
		int child[MaxNumChildrenPerNode]; //!< index of the children
		int numChilds; //!< number of children
		int firstPoint; //!< the first point in the sorted order covered by this subtree
		int numPoints; //!< the number of points covered by this subtree
	};

	//! Just to access the nodes a little bit easier
	inline const Node& node(int i) const { return m_nodes[i]; };

	//! Just to access the nodes a little bit easier
	inline Node& node(int i) { return m_nodes[i]; };

	//! returns the number of children of node i
	inline int numChilds(int i) const { return m_nodes[i].numChilds; };

	//! returns the index of the j th child of node i
	inline int child(int i, int j) const { return m_nodes[i].child[j]; };

	//! returns the number of points covered by this subtree
	inline int numPoints(int i) const { return m_nodes[i].numPoints; };

	//! returns the index of the jth point covered by i's subtree.
	inline int point(int i, int j) const { return m_mortonOrder[ m_nodes[i].firstPoint + j ].ref; };

	//! sets the point to the given grid coords
	void setPoint(int i, int d, IntType value);

	//! returns the number of points the quadtree contains
	inline int numPoints() const { return m_numPoints; };

	//! returns the maximum number of nodes (and the max index of a node)
	inline int maxNumNodes() const { return m_numPoints * 2; };

	//! returns the ith point in the input sequence
	const Point& point(int i) const { return m_points[i]; }

	//! Prepares the morton numbers for sorting
	void prepareMortonOrder();

	//! Sorts the points by morton number
	void sortMortonNumbers();

	//! Prepares both the leaf and inner node layer
	void prepareNodeLayer();

	//! Merges curr with next node in the chain (used by linkNodes)
	inline void mergeWithNext(int curr);

	//! used to update the first and numPoints of inner nodes by linkNodes
	inline void adjustPointInfo(int curr);

	//! The Recursive Bottom-Up Construction
	int linkNodes(int curr, int maxLevel);

	//! The Recursive Bottom-Up Construction (recursion start)
	void linkNodes();

	//! Does all required steps except the allocate, deallocate, randomPoints
	void build();

	//! Just for fun: traverse the tree and count the points in the leaves
	int countPoints(int curr) const;

	//! Just for fun: traverse the tree and count the points in the leaves
	int countPoints() const { return countPoints(m_rootIndex); };

	//! returns the index of the root node
	int rootIndex() const { return m_rootIndex; };

private:

	//! Allocates memory for n points
	void allocate(int n);

	//! Releases memory
	void deallocate();

	int m_maxLevel;
	Point* m_points = nullptr; //!< The input set
	int m_numPoints; //!< Total number of points
	MortonEntry* m_mortonOrder = nullptr; //!< The order to be sorted
	Node* m_nodes = nullptr; //!< Memory for all nodes
	int m_rootIndex; //!< The index of the root node
};

//! allocates memory for n points
template<typename IntType, int Dim>
void DTree<IntType, Dim>::allocate(int n)
{
	m_numPoints = n;
	m_points = new Point[m_numPoints];
	m_mortonOrder = new MortonEntry[m_numPoints];
	m_nodes = new Node[m_numPoints * 2];
};


//! releases memory
template<typename IntType, int Dim>
void DTree<IntType, Dim>::deallocate()
{
	delete[] m_points;
	delete[] m_mortonOrder;
	delete[] m_nodes;
};

template<typename IntType, int Dim>
void DTree<IntType, Dim>::setPoint(int i, int d, IntType value)
{
	// set i-th point d coord to value
	m_points[i].x[d] = value;
}

//! Prepares the morton numbers for sorting
template<typename IntType, int Dim>
void DTree<IntType, Dim>::prepareMortonOrder()
{
	// loop over the point order
	for (int i=0; i<m_numPoints; i++) {
		// set i's ref to i
		m_mortonOrder[i].ref = i;

		// generate the morton number by interleaving the bits
		interleaveBits<IntType, Dim>(m_points[i].x, m_mortonOrder[i].mortonNr);
	}
}

//! Sorts the points by morton number
template<typename IntType, int Dim>
void DTree<IntType, Dim>::sortMortonNumbers()
{
	// just sort them
	std::sort(m_mortonOrder, m_mortonOrder + m_numPoints);
}


//! Prepares both the leaf and inner node layer
template<typename IntType, int Dim>
void DTree<IntType, Dim>::prepareNodeLayer()
{
	Node* leafLayer	 = m_nodes;
	Node* innerLayer = m_nodes + m_numPoints;

#if 0
	int last = 0;
#endif
	for (int i = 0; i<m_numPoints;) {
		Node& leaf = leafLayer[i];
		Node& innerNode = innerLayer[i];
		// i represents the current node on both layers
		int j = i+1;

		// find the next morton number that differs or stop when j is equal to m_numPoints
		while ((j < m_numPoints) &&
			   (m_mortonOrder[i] == m_mortonOrder[j]))
			j++;
		// j is the index of the next cell (node)

		// init the node on the leaf layer
		leaf.firstPoint = i; //< node sits above the first point of the cell
		leaf.numPoints = j-i; //< number of points with equal morton numbers (number of points in grid cell)
		leaf.numChilds = 0; //< it's a leaf
		leaf.level = 0; //< it's a leaf
		leaf.next = j; //< this leaf hasnt been created yet but we use indices so its ok

		if (j < m_numPoints) {
			// Note: the n-th inner node is not needed because we only need n-1 inner nodes to cover n leaves
			// if we reach the n-1-th inner node, the variable last is set for the last time
#if 0
			last = i;
#endif
			// init the node on the inner node layer
			innerNode.child[0] = i; //< node sits above the first leaf
			innerNode.child[1] = j; //< this leaf hasnt been created yet but we use indices so its ok
			innerNode.numChilds = 2; //< every inner node covers two leaves in the beginning
			innerNode.level = lowestCommonAncestorLevel<IntType, Dim>(m_mortonOrder[i].mortonNr, m_mortonOrder[j].mortonNr);
			innerNode.next = m_numPoints + j; // the inner node layer is shifted by n
		} else {
			// no next for the last
			innerLayer[i].next = 0;
			// this is important to make the recursion stop
			innerLayer[i].level = m_maxLevel + 1;
		}

		// advance to the next cell
		i = j;
	};
	// here we set the successor of the n-1-th inner node to zero to avoid dealing with the n-th inner node
#if 0
	innerLayer[last].next = 0;
#endif
};

//! Merges curr with next node in the chain (used by linkNodes)
template<typename IntType, int Dim>
inline void DTree<IntType, Dim>::mergeWithNext(int curr)
{
	int next = node(curr).next;
	// Cool: since we never touched node(next) before
	// it is still linked to only two leaves,
	node(curr).child[node(curr).numChilds++] = node(next).child[1];

	// thus we don't need this ugly loop:
	//   for (int i=1; i<node(next).numChilds; i++)
	//      node(curr).child[node(curr).numChilds++] = node(next).child[i];
	node(curr).next = node(next).next;
};

template<typename IntType, int Dim>
inline void DTree<IntType, Dim>::adjustPointInfo(int curr)
{
	// adjust the first such that it matched the first child
	node(curr).firstPoint = node(node(curr).child[0]).firstPoint;

	// index of the last child
	const int lastChild = node(curr).child[node(curr).numChilds-1];

	// numPoints is lastPoint + 1 - firstPoint
	node(curr).numPoints = node(lastChild).firstPoint + node(lastChild).numPoints - node(curr).firstPoint;
}

//! The Recursive Bottom-Up Construction
template<typename IntType, int Dim>
int DTree<IntType, Dim>::linkNodes(int curr, int maxLevel)
{
	// while the subtree is smaller than maxLevel
	while (node(curr).next && node(node(curr).next).level < maxLevel) {
		// get next node in the chain
		int next = node(curr).next;
		// First case: same level => merge, discard next
		if (node(curr).level == node(next).level) {
			mergeWithNext(curr);
		} else // Second case: next is higher => become first child
		if (node(curr).level < node(next).level) {
			// set the first child of next to the current node
			node(next).child[0] = curr;

			// adjust the point info of the curr
			adjustPointInfo(curr);

			// this is the only case where we advance curr
			curr = next;
		} else { // Third case: next is smaller => construct a maximal subtree starting with next
			int r = linkNodes(next, node(curr).level);
			node(curr).child[node(curr).numChilds-1] = r;
			node(curr).next = node(r).next;
		};
	};
	// adjust the point info of the curr
	adjustPointInfo(curr);

	// we are done with this subtree, return the root
	return curr;
};


//! The Recursive Bottom-Up Construction (recursion start)
template<typename IntType, int Dim>
void DTree<IntType, Dim>::linkNodes()
{
	m_rootIndex = linkNodes(m_numPoints, m_maxLevel);
};

//! Just for fun: traverse the tree and count the points in the leaves
template<typename IntType, int Dim>
int DTree<IntType, Dim>::countPoints(int curr) const
{
	if (m_nodes[curr].numChilds) {
		int sum = 0;
		for (int i=0; i<m_nodes[curr].numChilds; i++) {
			sum += countPoints(m_nodes[curr].child[i]);
		};

		return sum;
	}
	else
		return m_nodes[curr].numPoints;
};

//! Does all required steps except the allocate, deallocate, randomPoints
template<typename IntType, int Dim>
void DTree<IntType, Dim>::build()
{
	// prepare the array with the morton numbers
	prepareMortonOrder();

	// sort the morton numbers
	sortMortonNumbers();

	// prepare the node layer
	prepareNodeLayer();

	//! link the inner nodes using the recursive bottom-up method
	linkNodes();
};

}}}
