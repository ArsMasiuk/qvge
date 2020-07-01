/** \file
 * \brief Declares classes ShellingOrderSet and ShellingOrder.
 *
 * \author Carsten Gutwenger
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

#include <ogdf/basic/NodeArray.h>


namespace ogdf {


/**
 * \brief The node set in a shelling order of a graph.
 */
class ShellingOrderSet : public Array<node>
{
public:
	//! Creates an empty shelling order set.
	ShellingOrderSet() : Array<node>()
	{
		m_leftVertex = m_rightVertex = nullptr;
		m_leftAdj    = m_rightAdj    = nullptr;
	}

	//! Creates a shelling order set for \p n nodes.
	/**
	 * @param n is the number of nodes in the set.
	 * @param adjL points to the left-node of the set.
	 * @param adjR points to the right-node of the set.
	 */
	ShellingOrderSet(int n, adjEntry adjL = nullptr, adjEntry adjR = nullptr) : Array<node>(1, n)
	{
		m_leftVertex  = (adjL != nullptr) ? adjL->twinNode() : nullptr;
		m_rightVertex = (adjR != nullptr) ? adjR->twinNode() : nullptr;
		m_leftAdj     = adjL;
		m_rightAdj    = adjR;
	}


	//! Returns the left-node of the set.
	node left() const {
		return m_leftVertex;
	}

	//! Returns the right-node of the set.
	node right() const {
		return m_rightVertex;
	}

	//! Returns the adjacency entry pointing from <I>z</I><SUB>1</SUB> to the left node (or 0 if no such node).
	adjEntry leftAdj() const {
		return m_leftAdj;
	}

	//! Returns the adjacency entry pointing from <I>z<SUB>p</SUB></I> to the right node (or 0 if no such node).
	adjEntry rightAdj() const {
		return m_rightAdj;
	}

	//! Returns true iff the adjacency entry to the left-node exists.
	bool hasLeft() const {
		return m_leftAdj != nullptr;
	}

	//! Returns true iff the adjacency entry to the right-node exists.
	bool hasRight() const {
		return m_rightAdj != nullptr;
	}

	//! Sets the left-node to \p cl.
	void left(node cl) {
		m_leftVertex = cl;
	}

	//! Sets the right-node to \p cr.
	void right (node cr) {
		m_rightVertex = cr;
	}

	//! Sets the adjacency entry pointing to the left-node to \p adjL.
	void leftAdj(adjEntry adjL) {
		m_leftAdj = adjL;
	}

	//! Sets the adjacency entry pointing to the right-node to \p adjR.
	void rightAdj(adjEntry adjR) {
		m_rightAdj = adjR;
	}

	//! Returns the length of the order set, i.e., the number of contained nodes.
	int len() const {
		return high();
	}

	//! Returns the i-th node in the order set from left (the leftmost node has index 1).
	node operator[] (const int i) const {
		return Array<node>::operator[](i);
	}

	//! Returns the i-th node in the order set from left (the leftmost node has index 1).
	node& operator[] (const int i) {
		return Array<node>::operator[](i);
	}


private:
	node m_leftVertex;   //!< the left-node of the set.
	node m_rightVertex;	 //!< the right-node of the set.
	adjEntry m_leftAdj;  //!< the adjacency entry pointing to the left-node.
	adjEntry m_rightAdj; //!< the adjacency entry pointing to the right-node.
};

/**
 * \brief The shelling order of a graph.
 */
class OGDF_EXPORT ShellingOrder
{
public:

	//! Creates an empty shelling order.
	ShellingOrder() {
		m_pGraph = nullptr;
	}

#if 0
	ShellingOrder(const Graph &G, const List<ShellingOrderSet> &partition);
#endif

	~ShellingOrder() { }

	//! Returns the graph associated with the shelling order.
	const Graph& getGraph() const {
		return *m_pGraph;
	}

	//! Returns the number of sets in the node partition.
	int length() const {
		return m_V.high();
	}

	//! Returns the length of the <I>i</I>-th order set <I>V<SUB>i</SUB></I>.
	int len(int i) const {
		return m_V[i].len();
	}

	//! Returns the <I>j</I>-th node of the <I>i</I>-th order set <I>V<SUB>i</SUB></I>.
	node operator() (int i, int j) const {
		return (m_V[i])[j];
	}

	//! Returns the <I>i</I>-th set <I>V_i</I>
	const ShellingOrderSet& operator[](int i) const {
		return m_V[i];
	}

	//! Returns the left-node of the <I>i</I>-th set <I>V<SUB>i</SUB></I>.
	node left (int i) const {
		return m_V[i].left();
	}

	//! Returns the right-node of the <I>i</I>-th set <I>V<SUB>i</SUB></I>.
	node right (int i) const {
		return m_V[i].right();
	}

	//! Returns the rank of node <I>v</I>, where rank(<I>v</I>) = <I>i</I> iff <I>v</I> is contained in <I>V<SUB>i</SUB></I>.
	int rank (node v) const {
		return m_rank[v];
	}


	/**
	 * \brief Initializes the shelling order for graph \p G with a given node partition.
	 * @param G is the associated graph.
	 * @param partition is the node partition.
	 */
	void init(const Graph &G, const List<ShellingOrderSet> &partition);

	/**
	 * \brief Initializes the shelling order for graph \p G with a given node partition and transforms it into a leftmost order.
	 * @param G is the associated graph.
	 * @param partition is the node partition.
	 */
	void initLeftmost(const Graph &G, const List<ShellingOrderSet> &partition);


	void push(int k, node v, node tgt);

	friend class CompOrderBic;

private:
	const Graph            *m_pGraph; //!< the associated graph.
	Array<ShellingOrderSet> m_V;      //!< the node partition.
	NodeArray<int>          m_rank;   //!< the rank of nodes.
};

}
