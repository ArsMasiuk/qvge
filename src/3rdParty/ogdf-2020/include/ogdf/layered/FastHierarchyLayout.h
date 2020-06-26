/** \file
 * \brief declaration and implementation of the third phase of sugiyama
 *
 * \author Sebastian Leipert
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

#include <ogdf/layered/HierarchyLayoutModule.h>
#include <ogdf/basic/List.h>

namespace ogdf {

/**
 * \brief Coordinate assignment phase for the Sugiyama algorithm by Buchheim et al.
 *
 * @ingroup gd-hlm
 *
 * This class implements a hierarchy layout algorithm, i.e., it layouts
 * hierarchies with a given order of nodes on each layer. It is used as a third
 * phase of the Sugiyama algorithm.
 *
 * All edges of the layout will have at most two bends. Additionally,
 * for each edge having exactly two bends, the segment between them is
 * drawn vertically. This applies in particular to the long edges
 * arising in the first phase of the Sugiyama algorithm.
 *
 * The implementation is based on:
 *
 * Christoph Buchheim, Michael Jünger, Sebastian Leipert: <i>A Fast %Layout
 * Algorithm for k-Level Graphs</i>. LNCS 1984 (Proc. %Graph Drawing 2000),
 * pp. 229-240, 2001.
 *
 * <h3>Optional Parameters</h3>
 *
 * <table>
 *   <tr>
 *     <th>Option</th><th>Type</th><th>Default</th><th>Description</th>
 *   </tr><tr>
 *     <td><i>node distance</i></td><td>double</td><td>3.0</td>
 *     <td>the minimal horizontal distance between two nodes on the same layer</td>
 *   </tr><tr>
 *     <td><i>layer distance</i></td><td>double</td><td>3.0</td>
 *     <td>the minimal vertical distance between two nodes on neighbored layers</td>
 *   </tr><tr>
 *     <td><i>fixed layer distance</i></td><td>bool</td><td>false</td>
 *     <td>if true, the distance between neighbored layers is fixed, otherwise variable</td>
 *   </tr>
 * </table>
 */
class OGDF_EXPORT FastHierarchyLayout : public HierarchyLayoutModule
{
protected:
	virtual void doCall(const HierarchyLevelsBase &levels, GraphAttributes &AGC) override;

public:
	//! Creates an instance of fast hierarchy layout.
	FastHierarchyLayout();

	//! Copy constructor.
	FastHierarchyLayout(const FastHierarchyLayout &);

	// destructor
	virtual ~FastHierarchyLayout() { }


	//! Assignment operator
	FastHierarchyLayout &operator=(const FastHierarchyLayout &);


	//! Returns the option <i>node distance</i>.
	double nodeDistance() const {
		return m_minNodeDist;
	}

	//! Sets the option node distance to \p dist.
	void nodeDistance(double dist) {
		m_minNodeDist = dist;
	}

	//! Returns the option <i>layer distance</i>.
	double layerDistance() const {
		return m_minLayerDist;
	}

	//! Sets the option layer distance to \p dist.
	void layerDistance(double dist) {
		m_minLayerDist = dist;
	}

	//! Returns the option <i>fixed layer distance</i>.
	bool fixedLayerDistance() const {
		return m_fixedLayerDist;
	}

	//! Sets the option fixed layer distance to \p b.
	void fixedLayerDistance(bool b) {
		m_fixedLayerDist = b;
	}


private:

	int n;      //!< The number of nodes including virtual nodes.
	int m;      //!< The number edge sections.
	int k;      //!< The number of layers.
	int *layer; //!< Stores for every node its layer.
	int *first; //!< Stores for every layer the index of the first node.


	// nodes are numbered top down and from left to right.
	// Is called "internal numbering".
	// Nodes and Layeras are number 0 to n-1 and 0 to k-1, respectively.
	// For thechnical reasons we set first[k] to n.

	/**
	 * \brief The list of neighbors in previous / next layer.
	 *
	 * for every node : adj[0][node] list of neighbors in previous layer;
	 * for every node : adj[1][node] list of neighbors in next layer
	 */
	List<int> *adj[2];

	/**
	 * \brief The nodes belonging to a long edge.
	 *
	 * for every node : longEdge[node] is a pointer to a list containing all
	 * nodes that belong to the same long edge as node.
	 */
	List<int> **longEdge;

	double m_minNodeDist; //!< The minimal node distance on a layer.
	double m_minLayerDist;//!< The minimal distance between layers.
	double *breadth;      //!< for every node : breadth[node] = width of the node.
	double *height;       //!< for every layer : height[layer] = height of max{height of node on layer}.
	double *y;            //!< for every layer : y coordinate of layer.
	double *x;            //!< for every node : x coordinate of node.
	/**
	 * for every node : minimal possible distance between the center of a node
	 * and first[layer[node]].
	 */
	double *totalB;

	double *mDist; //!< Similar to totalB, used for temporary storage.

	bool m_fixedLayerDist; //!< 0 if distance between layers should be variable, 1 otherwise.
	bool *virt; //!< for every node : virt[node] = 1 if node is virtual, 0 otherwise.

	void incrTo(double& d, double t) {
		if(d < t) d = t;
	}

	void decrTo(double& d, double t) {
		if(d > t) d = t;
	}

	bool sameLayer(int n1, int n2) const {
		return n1 >= 0 && n1 < n
		    && n2 >= 0 && n2 < n
		    && layer[n1] == layer[n2];
	}

	bool isFirst(int actNode) const {
		return actNode < 0
		    || actNode >= n
		    || actNode == first[layer[actNode]];
	}

	bool isLast(int actNode) const {
		return actNode < 0
		    || actNode >= n
		    || actNode == first[layer[actNode] + 1] - 1;
	}

	/**
	 * Places the node actNode as far as possible to the left (if \p dir = 1) or to
	 * the right (if \p dir = -1) within a block.
	 *
	 * A proper definition of blocks is given in Techreport zpr99-368, pp 5, where
	 * blocks are named classes. If actNode is virtual (and thus belongs to a long
	 * edge), the function sortLongEdges places the actNode as far as possible to
	 * the left such that the corresponding long edge will be vertical.
	 *
	 * @param actNode the placed node
	 * @param dir
	 *   Stores the direction of placement: 1 for placing long edges to the left and
	 *   -1 for placing them to the right.
	 * @param pos
	 *   Array for all nodes.
	 *   Stores the computed position.
	 * @param marked
	 *   Array for all nodes.
	 *   Stores for every node, whether sortLongEdges has already been applied to it.
	 * @param block
	 *   Array for all nodes.
	 *   Stores for every node the block it belongs to.
	 * @param exD
	 *   is 1 if there exists a node w on the longEdge of actNode,
	 *   that has a direct right sibling (if moving to the left (depending on
	 *   the direction)) on the same layer which belongs to a different block.
	 * @param dist
	 *   If exD is 1, it gives the minimal distance between any w of long
	 *   edge (see exD) and its direct right (left) sibling if the sibling
	 *   belongs to ANOTHER block. if exD is 0, dist is not relevant.
	 */
	void sortLongEdges(int actNode, int dir, double *pos, bool &exD, double &dist, int *block, bool *marked);

	/**
	 * Places a sequence of nonvirtual nodes containing exactly one node.
	 *
	 * @param leftBnd
	 *   contains the number of the next virtual sibling to the left of actNode, if it exists;
	 *   -1 otherwise. Observe that between leftBnd and actNode there may be other nonvirtual nodes.
	 * @param rightBnd
	 *   contains the number of the next virtual sibling to the right of actNode, if it exists;
	 *   -1 otherwise. Observe that between rightBnd and actNode there may be other nonvirtual nodes.
	 * @param actNode an nonvirtual node that has to be placed.
	 * @param best the position that is computed for actNode by placeSingleNode.
	 * @param d is the direction of traversal. If 0 we traverse the graph top to bottom, 1 otherwise.
	 *
	 * The total length of all edges of actnode to the previous layer (if d = 0) or
	 * next layer (if d = 1) is minimized observing the bounds given by leftBnd and
	 * rightBnd. The optimal position is the median of its neighbours adapted to
	 * leftBnd and rightBnd. The position of the neighbours is given by the global
	 * variable x.
	 *
	 * The funcion returns 0 if actNode does not have neighbours on the previous
	 * (next) layer, 1 otherwise.
	 */
	bool placeSingleNode(int leftBnd, int rightBnd, int actNode, double &best, int d);

	/**
	 * Places a sequence of nonvirtual nodes.
	 *
	 * The function partitions the sequence, applying a divide and conquer strategy
	 * using recursive calls on the two subsequences.
	 *
	 * @param leftBnd
	 *   contains the number of the next virtual sibling to the left of the sequence, if it exists;
	 *   -1 otherwise. Observe that between leftBnd and actNode there may be other nonvirtual nodes.
	 * @param rightBnd
	 *   contains the number of the next virtual sibling to the right of the sequence, if it exists;
	 *   -1 otherwise. Observe that between rightBnd and actNode there may be other nonvirtual nodes.
	 * @param left the leftmost nonvirtual node of the sequence that has to be placed.
	 * @param right the rightmost nonvirtual node of the sequence that has to be placed.
	 * @param d the direction of traversal. If 0 we traverse the graph top to bottom. 1 otherwise.
	 *
	 * The total length of all edges of the sequence to the previous layer (if d = 0)
	 * or next layer (if d = 1) is minimized observing the bounds given by leftBnd
	 * and rightBnd.
	 *
	 * The position that is computed for every node of the sequence is stored in the
	 * global variable x. The position of the neighbours is given by the global
	 * variable x.
	 */
	void placeNodes(int leftBnd, int rightBnd, int left, int right, int d);

	/**
	 * Used for postprocessing the layout.
	 *
	 * If the two nonvirtual ndoes of the long edge are both to the left (right) of
	 * the virtual nodes, the function moveLongEdge tries to reduce the length of the
	 * two outermost segments by moving the virtual nodes simultaneously as far as
	 * possible to the left (right). If both non virtual nodes are on different sides
	 * of the virtual nodes, moveLongEdge tries to remove one of the edge bends by
	 * moving the virtual nodes.
	 *
	 * If there exists a conflict with another long edge on the left (right) side of
	 * the current long edge, the function moveLongEdge is first applied recursively
	 * to this long edge.
	 *
	 * @param actNode a representative node of the long edge
	 * @param dir
	 *   is -1 if it is preferred to move the long edge to the left,
	 *   1 if it is preferred to move the long edge to the right,
	 *   0 if there is no preference
	 * @param marked Array for all nodes. Stores for every node, whether moveLongEdge has already been applied to it.
	*/
	void moveLongEdge(int actNode, int dir, bool *marked);

	/**
	 * Tries to remove a bend at the position of the virtual node by straightening the edge.
	 *
	 * The method is applied to long edges with exactly one virtual node.
	 *
	 * @param actNode the virtual  representative node of the long edge
	 * @param marked array for all nodes. Stores for every node, whether straightenEdge has already been applied to it.
	 *
	 * If there exists a conflict with a direct sibling to the left (right) side of
	 * the current node, the function straightenEdge is first applied recursively to
	 * this node.
	 */
	void straightenEdge(int actNode, bool *marked);

	//! Computes the layout of an embedded layered graph.
	void findPlacement();
};

}
