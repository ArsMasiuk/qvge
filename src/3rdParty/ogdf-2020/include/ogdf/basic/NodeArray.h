/** \file
 * \brief Declaration and implementation of NodeArray class
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

#include <ogdf/basic/Graph_d.h>


namespace ogdf {


//! Abstract base class for node arrays.
/**
 * Defines the interface for event handling used by the Graph class.
 * Use the parameterized class NodeArray for creating node arrays.
 */
class NodeArrayBase {
	/**
	 * Pointer to list element in the list of all registered node
	 * arrays which references this array.
	 */
	ListIterator<NodeArrayBase*> m_it;

public:
	const Graph *m_pGraph; //!< The associated graph.

	//! Initializes an node array not associated with a graph.
	NodeArrayBase() : m_pGraph(nullptr) { }

	//! Initializes an node array associated with \p pG.
	explicit NodeArrayBase(const Graph *pG) : m_pGraph(pG) {
		if(pG) m_it = pG->registerArray(this);
	}

	//! Moves node array \p base to this node array.
	NodeArrayBase(NodeArrayBase &base) : m_it(base.m_it), m_pGraph(base.m_pGraph) {
		if(m_pGraph) m_pGraph->moveRegisterArray(m_it, this);
		base.m_pGraph = nullptr;
		base.m_it     = ListIterator<NodeArrayBase*>();
	}

	// destructor, unregisters the array
	virtual ~NodeArrayBase() {
		if (m_pGraph) m_pGraph->unregisterArray(m_it);
	}

	// event interface used by Graph
	//! Virtual function called when table size has to be enlarged.
	virtual void enlargeTable(int newTableSize) = 0;
	//! Virtual function called when table has to be reinitialized.
	virtual void reinit(int initTableSize) = 0;
	//! Virtual function called when array is disconnected from the graph.
	virtual void disconnect() = 0;

	//! Associates the array with a new graph.
	void reregister(const Graph *pG) {
		if (m_pGraph) m_pGraph->unregisterArray(m_it);
		if ((m_pGraph = pG) != nullptr) m_it = pG->registerArray(this);
	}

	//! Moves array registration from \p base to this array.
	void moveRegister(NodeArrayBase &base) {
		if (m_pGraph) m_pGraph->unregisterArray(m_it);
		m_pGraph = base.m_pGraph;
		m_it     = base.m_it;
		base.m_pGraph = nullptr;
		base.m_it     = ListIterator<NodeArrayBase*>();
		if (m_pGraph != nullptr)
			m_pGraph->moveRegisterArray(m_it, this);
	}
};

//! Dynamic arrays indexed with nodes.
/**
 * @ingroup graph-containers
 *
 * NodeArrays represent a mapping from nodes to data of type \a T.
 * They adjust their table size automatically when the graph grows.
 *
 * @warn_undef_behavior_array
 *
 * @tparam T is the element type.
 */
template<class T> class NodeArray : private Array<T>, protected NodeArrayBase {
	T m_x; //!< The default value for array elements.

public:
	using key_type = node;    //!< The type for array keys.
	using value_type =  T;  //!< The type for array entries.

	using iterator = internal::GraphArrayIterator<NodeArray<T>>;  //!< The type for node array iterators.
	using const_iterator = internal::GraphArrayConstIterator<NodeArray<T>>;  //!< The type for node array const iterators.


	//! Constructs an empty node array associated with no graph.
	NodeArray() : Array<T>(), NodeArrayBase() { }

	//! Constructs a node array associated with \p G.
	NodeArray(const Graph &G) : Array<T>(G.nodeArrayTableSize()), NodeArrayBase(&G) { }

	//! Constructs a node array associated with \p G.
	/**
	 * @param G is the associated graph.
	 * @param x is the default value for all array elements.
	 */
	NodeArray(const Graph &G, const T &x) :
		Array<T>(0,G.nodeArrayTableSize()-1,x), NodeArrayBase(&G), m_x(x) { }

	//! Constructs a node array that is a copy of \p A.
	/**
	 * Associates the array with the same graph as \p A and copies all elements.
	 */
	NodeArray(const NodeArray<T> &A) : Array<T>(A), NodeArrayBase(A.m_pGraph), m_x(A.m_x) { }

	//! Constructs a node array containing the elements of \p A (move semantics).
	/**
	 * NodeArray \p A is empty afterwards and not associated with any graph.
	 */
	NodeArray(NodeArray<T> &&A) : Array<T>(std::move(A)), NodeArrayBase(A), m_x(A.m_x) { }


	/**
	 * @name Access methods
	 * These methods provide access to elements and the corresponding graph.
	 */
	//@{

	//! Returns true iff the array is associated with a graph.
	bool valid() const { return Array<T>::low() <= Array<T>::high(); }

	//! Returns a pointer to the associated graph.
	const Graph *graphOf() const {
		return m_pGraph;
	}

	//! Returns a reference to the element with index \p v.
	const T &operator[](node v) const {
		OGDF_ASSERT(v != nullptr);
		OGDF_ASSERT(v->graphOf() == m_pGraph);
		return Array<T>::operator [](v->index());
	}

	//! Returns a reference to the element with index \p v.
	T &operator[](node v) {
		OGDF_ASSERT(v != nullptr);
		OGDF_ASSERT(v->graphOf() == m_pGraph);
		return Array<T>::operator [](v->index());
	}

	//! Returns a reference to the element with index \p v.
	const T &operator()(node v) const {
		OGDF_ASSERT(v != nullptr);
		OGDF_ASSERT(v->graphOf() == m_pGraph);
		return Array<T>::operator [](v->index());
	}

	//! Returns a reference to the element with index \p v.
	T &operator()(node v) {
		OGDF_ASSERT(v != nullptr);
		OGDF_ASSERT(v->graphOf() == m_pGraph);
		return Array<T>::operator [](v->index());
	}

	//! Returns a reference to the element with index \p index.
	//! \attention Make sure that \p index is a valid index for a node in the associated graph!
	OGDF_DEPRECATED("NodeArrays should be indexed by a node, not an integer index.")
	const T &operator[](int index) const
		{ return Array<T>::operator [](index); }

	//! Returns a reference to the element with index \p index.
	//! \attention Make sure that \p index is a valid index for a node in the associated graph!
	OGDF_DEPRECATED("NodeArrays should be indexed by a node, not an integer index.")
	T &operator[](int index)
		{ return Array<T>::operator [](index); }

	//@}
	/**
	 * @name Iterators
	 * These methods return bidirectional iterators to elements in the array.
	 */
	//@{

	//! Returns an iterator to the first entry in the node array.
	/**
	 * If the node array is empty, an invalid iterator is returned.
	 */
	iterator begin() { return iterator(m_pGraph->firstNode(), this); }

	//! Returns a const iterator to the first entry in the node array.
	/**
	 * If the node array is empty, an invalid iterator is returned.
	 */
	const_iterator begin() const { return const_iterator(m_pGraph->firstNode(), this); }

	//! Returns a const iterator to the first entry in the node array.
	/**
	 * If the node array is empty, an invalid iterator is returned.
	 */
	const_iterator cbegin() const { return const_iterator(m_pGraph->firstNode(), this); }

	//! Returns an iterator to one-past-last entry in the node array.
	/**
	 * This is always an invalid iterator.
	 */
	iterator end() { return iterator(nullptr, this); }

	//! Returns a const iterator to one-past-last entry in the node array.
	/**
	 * This is always an invalid iterator.
	 */
	const_iterator end() const { return const_iterator(nullptr, this); }

	//! Returns a const iterator to one-past-last entry in the node array.
	/**
	 * This is always an invalid iterator.
	 */
	const_iterator cend() const { return const_iterator(nullptr, this); }

	//@}
	/**
	 * @name Initialization and assignment
	 * These methods can be used to reinitialize the array, or to initialize all elements with a given value.
	 */
	//@{

	//! Reinitializes the array. Associates the array with no graph.
	void init() {
		Array<T>::init(); reregister(nullptr);
	}

	//! Reinitializes the array. Associates the array with \p G.
	void init(const Graph &G) {
		Array<T>::init(G.nodeArrayTableSize()); reregister(&G);
	}

	//! Reinitializes the array. Associates the array with \p G.
	/**
	 * @param G is the associated graph.
	 * @param x is the default value.
	 */
	void init(const Graph &G, const T &x) {
		Array<T>::init(0,G.nodeArrayTableSize()-1, m_x = x); reregister(&G);
	}

	//! Sets all array elements to \p x.
	void fill(const T &x) {
		int high = m_pGraph->maxNodeIndex();
		if(high >= 0)
			Array<T>::fill(0,high,x);
	}

	//! Assignment operator.
	NodeArray<T> &operator=(const NodeArray<T> &a) {
		Array<T>::operator =(a);
		m_x = a.m_x;
		reregister(a.m_pGraph);
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * Nodearray \p a is empty afterwards and not associated with any graph.
	 */
	NodeArray<T> &operator=(NodeArray<T> &&a) {
		Array<T>::operator=(std::move(a));
		m_x = a.m_x;
		moveRegister(a);
		return *this;
	}


	//@}
	/**
	 * @name Helper functions
	 * These methods are mainly intended for internal use.
	 */
	//@{

	static key_type findSuccKey(key_type key) { return key->succ(); }
	static key_type findPredKey(key_type key) { return key->pred(); }

	//@}

private:
	virtual void enlargeTable(int newTableSize) {
		Array<T>::resize(newTableSize,m_x);
	}

	virtual void reinit(int initTableSize) {
		Array<T>::init(0,initTableSize-1,m_x);
	}

	virtual void disconnect() {
		Array<T>::init();
		m_pGraph = nullptr;
	}

	OGDF_NEW_DELETE
};

}
