/** \file
 * \brief Declaration and implementation of AdjEntryArray class.
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

#include <ogdf/basic/Graph.h>


namespace ogdf {


//! Abstract base class for adjacency entry arrays.
/**
 * Defines the interface for event handling used by the Graph class.
 * Use the parameterized class AdjEntryArray for creating adjacency arrays.
 */
class AdjEntryArrayBase {
	/**
	 * Pointer to list element in the list of all registered adjacency
	 * entry arrays which references this array.
	 */
	ListIterator<AdjEntryArrayBase*> m_it;

public:
	const Graph *m_pGraph; //!< The associated graph.

	//! Initializes an adjacency entry array not associated with a graph.
	AdjEntryArrayBase() : m_pGraph(nullptr) { }

	//! Initializes an adjacency entry array associated with \p pG.
	explicit AdjEntryArrayBase(const Graph *pG) : m_pGraph(pG) {
		if(pG) m_it = pG->registerArray(this);
	}

	//! Moves adjacency entry array \p base to this adjacency entry array.
	AdjEntryArrayBase(AdjEntryArrayBase &base) : m_it(base.m_it), m_pGraph(base.m_pGraph) {
		if(m_pGraph) m_pGraph->moveRegisterArray(m_it, this);
		base.m_pGraph = nullptr;
		base.m_it     = ListIterator<AdjEntryArrayBase*>();
	}

	//! Destructor, unregisters the array
	virtual ~AdjEntryArrayBase() {
		if (m_pGraph) m_pGraph->unregisterArray(m_it);
	}

	// event interface used by Graph
	//! Virtual function called when table size has to be enlarged.
	virtual void enlargeTable(int newTableSize) = 0;
	//! Virtual function called when table has to be reinitialized.
	virtual void reinit(int initTableSize) = 0;
	//! Virtual function called when array is disconnected from the graph.
	virtual void disconnect() = 0;
	//! Virtual function called when the index of an adjacency entry is changed.
	virtual void resetIndex(int newIndex, int oldIndex) = 0;

	//! Associates the array with a new graph.
	void reregister(const Graph *pG) {
		if (m_pGraph) m_pGraph->unregisterArray(m_it);
		if ((m_pGraph = pG) != nullptr) m_it = pG->registerArray(this);
	}

	//! Moves array registration from \p base to this array.
	void moveRegister(AdjEntryArrayBase &base) {
		if (m_pGraph) m_pGraph->unregisterArray(m_it);
		m_pGraph = base.m_pGraph;
		m_it     = base.m_it;
		base.m_pGraph = nullptr;
		base.m_it     = ListIterator<AdjEntryArrayBase*>();
		if (m_pGraph != nullptr)
			m_pGraph->moveRegisterArray(m_it, this);
	}
};

//! Dynamic arrays indexed with adjacency entries.
/**
 * @ingroup graph-containers
 *
 * Adjacency entry arrays represent a mapping from adjacency entries to data of type \a T.
 * They adjust their table size automatically when the graph grows.
 *
 * @warn_undef_behavior_array
 *
 * @tparam T is the element type.
 */
template<class T> class AdjEntryArray : private Array<T>, protected AdjEntryArrayBase {
	T m_x; //!< The default value for array elements.

public:
	//! The type for array keys.
	using key_type = adjEntry;
	//! The type for array entries.
	using value_type = T;

	//! The type for adjEntry array iterators.
	using iterator = internal::GraphArrayIterator<AdjEntryArray<T>>;
	//! The type for adjEntry array const iterators.
	using const_iterator = internal::GraphArrayConstIterator<AdjEntryArray<T>>;


	//! Constructs an empty adjacency entry array associated with no graph.
	AdjEntryArray() : Array<T>(), AdjEntryArrayBase() { }

	//! Constructs an adjacency entry array associated with \p G.
	explicit AdjEntryArray(const Graph &G) : Array<T>(G.adjEntryArrayTableSize()), AdjEntryArrayBase(&G) { }

	//! Constructs an adjacency entry array associated with \p G.
	/**
	 * @param G is the associated graph.
	 * @param x is the default value for all array elements.
	 */
	AdjEntryArray(const Graph &G, const T &x) :
		Array<T>(0,G.adjEntryArrayTableSize()-1,x), AdjEntryArrayBase(&G), m_x(x) { }

	//! Constructs an adjacency entry array that is a copy of \p A.
	/**
	 * Associates the array with the same graph as \p A and copies all elements.
	 */
	AdjEntryArray(const AdjEntryArray<T> &A) : Array<T>(A), AdjEntryArrayBase(A.m_pGraph), m_x(A.m_x) { }

	//! Constructs an adjacency entry array containing the elements of \p A (move semantics).
	/**
	 * Adjacency entry array \p A is empty afterwards and not associated with any graph.
	 */
	AdjEntryArray(AdjEntryArray<T> &&A) : Array<T>(std::move(A)), AdjEntryArrayBase(A), m_x(A.m_x) { }


	/**
	 * @name Access methods
	 * These methods provide access to elements, size, and corresponding graph.
	 */
	//@{

	//! Returns true iff the array is associated with a graph.
	bool valid() const { return Array<T>::low() <= Array<T>::high(); }

	//! Returns a pointer to the associated graph.
	const Graph *graphOf() const {
		return m_pGraph;
	}

	//! Returns a reference to the element with index \p adj.
	const T &operator[](adjEntry adj) const {
		OGDF_ASSERT(adj != nullptr);
		OGDF_ASSERT(adj->graphOf() == m_pGraph);
		return Array<T>::operator [](adj->index());
	}

	//! Returns a reference to the element with index \p adj.
	T &operator[](adjEntry adj) {
		OGDF_ASSERT(adj != nullptr);
		OGDF_ASSERT(adj->graphOf() == m_pGraph);
		return Array<T>::operator [](adj->index());
	}

	//! Returns a reference to the element with index \p adj.
	const T &operator()(adjEntry adj) const {
		OGDF_ASSERT(adj != nullptr);
		OGDF_ASSERT(adj->graphOf() == m_pGraph);
		return Array<T>::operator [](adj->index());
	}

	//! Returns a reference to the element with index \p adj.
	T &operator()(adjEntry adj) {
		OGDF_ASSERT(adj != nullptr);
		OGDF_ASSERT(adj->graphOf() == m_pGraph);
		return Array<T>::operator [](adj->index());
	}


	//@}
	/**
	 * @name Iterators
	 * These methods return bidirectional iterators to elements in the array.
	 */
	//@{

	//! Returns an iterator to the first entry in the array.
	/**
	 * If the array is empty, a null pointer iterator is returned.
	 */
	iterator begin() {
		return iterator(findFirstKey(), this);
	}

	//! Returns a const iterator to the first entry in the array.
	/**
	 * If the array is empty, a null pointer iterator is returned.
	 */
	const_iterator begin() const { return const_iterator(findFirstKey(), this); }

	//! Returns a const iterator to the first entry in the array.
	/**
	 * If the array is empty, a null pointer iterator is returned.
	 */
	const_iterator cbegin() const { return const_iterator(findFirstKey(), this); }

	//! Returns an iterator to one-past-last entry in the array.
	/**
	 * This is always a null pointer iterator.
	 */
	iterator end() { return iterator(nullptr, this); }

	//! Returns a const iterator to one-past-last entry in the array.
	/**
	 * This is always a null pointer iterator.
	 */
	const_iterator end() const { return const_iterator(nullptr, this); }

	//! Returns a const iterator to one-past-last entry in the array.
	/**
	 * This is always a null pointer iterator.
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
		Array<T>::init(G.adjEntryArrayTableSize()); reregister(&G);
	}

	//! Reinitializes the array. Associates the array with \p G.
	/**
	 * @param G is the associated graph.
	 * @param x is the default value.
	 */
	void init(const Graph &G, const T &x) {
		Array<T>::init(0,G.adjEntryArrayTableSize()-1, m_x = x); reregister(&G);
	}

	//! Sets all array elements to \p x.
	void fill(const T &x) {
		int high = m_pGraph->maxAdjEntryIndex();
		if(high >= 0)
			Array<T>::fill(0,high,x);
	}

	//! Assignment operator.
	AdjEntryArray<T> &operator=(const AdjEntryArray<T> &A) {
		Array<T>::operator =(A);
		m_x = A.m_x;
		reregister(A.m_pGraph);
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * Adjacency entry array \p a is empty afterwards and not associated with any graph.
	 */
	AdjEntryArray<T> &operator=(AdjEntryArray<T> &&a) {
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

	//! Returns the key succeeding \p adj.
	static adjEntry findSuccKey(adjEntry adj) {
		if(adj->succ() != nullptr)
			return adj->succ();
		node v = adj->theNode();
		for(v = v->succ(); v != nullptr && v->firstAdj() == nullptr; v = v->succ())
			;
		return (v != nullptr) ? v->firstAdj() : nullptr;
	}

	//! Returns the key preceeding \p adj.
	static adjEntry findPredKey(adjEntry adj) {
		if(adj->pred() != nullptr)
			return adj->pred();
		node v = adj->theNode();
		for(v = v->pred(); v != nullptr && v->lastAdj() == nullptr; v = v->pred())
			;
		return (v != nullptr) ? v->lastAdj() : nullptr;
	}

	//@}

private:
	//! Returns the first key (adjacency entry in the graph).
	adjEntry findFirstKey() const {
		node v = m_pGraph->firstNode();
		while(v != nullptr && v->firstAdj() == nullptr)
			v = v->succ();
		adjEntry key = (v != nullptr) ? v->firstAdj() : nullptr;
		return key;
	}

	//! Returns the last key (adjacency entry in the graph).
	adjEntry findLastKey() const {
		node v = m_pGraph->lastNode();
		while(v != nullptr && v->lastAdj() == nullptr)
			v = v->pred();
		adjEntry key = (v != nullptr) ? v->lastAdj() : nullptr;
		return key;
	}

	virtual void enlargeTable(int newTableSize) {
		Array<T>::grow(newTableSize-Array<T>::size(),m_x);
	}

	virtual void reinit(int initTableSize) {
		Array<T>::init(0,initTableSize-1,m_x);
	}

	virtual void resetIndex(int newIndex, int oldIndex) {
		Array<T>::operator [](newIndex) = Array<T>::operator [](oldIndex);
	}

	virtual void disconnect() {
		Array<T>::init();
		m_pGraph = nullptr;
	}

	OGDF_NEW_DELETE
};

}
