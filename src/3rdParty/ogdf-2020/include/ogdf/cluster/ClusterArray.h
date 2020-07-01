/** \file
 * \brief Declaration and implementation of ClusterArray class.
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

#include <ogdf/basic/Array.h>
#include <ogdf/cluster/ClusterGraph.h>


namespace ogdf {

//! Abstract base class for cluster arrays.
/**
 * @ingroup graph-containers
 *
 * Defines the interface for event handling used by the ClusterGraph class.
 * Use the paramiterized class ClusterArray for creating edge arrays.
 */
class ClusterArrayBase {
	/**
	 * Pointer to list element in the list of all registered cluster
	 * arrays which references this array.
	 */
	ListIterator<ClusterArrayBase*> m_it;

public:
	const ClusterGraph *m_pClusterGraph; //!< The associated cluster graph.

	//! Initializes a cluster array not associated with a cluster graph.
	ClusterArrayBase() : m_pClusterGraph(nullptr) { }

	//! Initializes a cluster array associated with \p pC.
	explicit ClusterArrayBase(const ClusterGraph *pC) : m_pClusterGraph(pC) {
		if(pC) m_it = pC->registerArray(this);
	}

	//! Moves cluster array \p base to this cluster array.
	ClusterArrayBase(ClusterArrayBase &base) : m_it(base.m_it), m_pClusterGraph(base.m_pClusterGraph) {
		if(m_pClusterGraph) m_pClusterGraph->moveRegisterArray(m_it, this);
		base.m_pClusterGraph = nullptr;
		base.m_it            = ListIterator<ClusterArrayBase*>();
	}

	// destructor, unregisters the array
	virtual ~ClusterArrayBase() {
		if (m_pClusterGraph) m_pClusterGraph->unregisterArray(m_it);
	}

	// event interface used by Graph
	//! Virtual function called when table size has to be enlarged.
	virtual void enlargeTable(int newTableSize) = 0;
	//! Virtual function called when table has to be reinitialized.
	virtual void reinit(int initTableSize) = 0;
	//! Virtual function called when array is disconnected from the cluster graph.
	virtual void disconnect() = 0;

	//! Associates the array with a new cluster graph.
	void reregister(const ClusterGraph *pC) {
		if (m_pClusterGraph) m_pClusterGraph->unregisterArray(m_it);
		if ((m_pClusterGraph = pC) != nullptr) m_it = pC->registerArray(this);
	}

	//! Moves array registration from \p base to this array.
	void moveRegister(ClusterArrayBase &base) {
		if (m_pClusterGraph) m_pClusterGraph->unregisterArray(m_it);
		m_pClusterGraph = base.m_pClusterGraph;
		m_it            = base.m_it;
		base.m_pClusterGraph = nullptr;
		base.m_it            = ListIterator<ClusterArrayBase*>();
		if (m_pClusterGraph != nullptr)
			m_pClusterGraph->moveRegisterArray(m_it, this);
	}
};

//! Dynamic arrays indexed with clusters.
/**
 * Cluster arrays adjust their table size automatically
 * when the cluster graph grows.
 *
 * @warn_undef_behavior_array
 */
template<class T> class ClusterArray : private Array<T>, protected ClusterArrayBase {
	T m_x; //!< The default value for array elements.

public:
	//! The type for array keys.
	using key_type = cluster;
	//! The type for array entries.
	using value_type = T;

	//! The type for cluster array iterators.
	using iterator = internal::GraphArrayIterator<ClusterArray<T>>;
	//! The type for cluster array const iterators.
	using const_iterator = internal::GraphArrayConstIterator<ClusterArray<T>>;


	//! Constructs an empty cluster array associated with no graph.
	ClusterArray() : Array<T>(), ClusterArrayBase() { }

	//! Constructs a cluster array associated with \p C.
	ClusterArray(const ClusterGraph &C) :
		Array<T>(C.clusterArrayTableSize()),
		ClusterArrayBase(&C) { }

	//! Constructs a cluster array associated with \p C.
	/**
	 * @param C is the associated cluster graph.
	 * @param x is the default value for all array elements.
	 */
	ClusterArray(const ClusterGraph &C, const T &x) :
		Array<T>(0,C.clusterArrayTableSize()-1,x),
		ClusterArrayBase(&C), m_x(x) { }

	//! Constructs a cluster array associated with \p C and a given
	//! size (for static use).
	/**
	 * @param C is the associated cluster graph.
	 * @param x is the default value for all array elements.
	 * @param size is the size of the array.
	 */
	ClusterArray(const ClusterGraph &C, const T &x, int size) :
		Array<T>(0,size-1,x),
		ClusterArrayBase(&C), m_x(x) { }

	//! Constructs a cluster array that is a copy of \p A.
	/**
	 * Associates the array with the same cluster graph as \p A and copies all elements.
	 */
	ClusterArray(const ClusterArray<T> &A) :
		Array<T>(A),
		ClusterArrayBase(A.m_pClusterGraph), m_x(A.m_x) { }

	//! Constructs a cluster array containing the elements of \p A (move semantics).
	/**
	 * Cluster array \p A is empty afterwards and not associated with any cluster graph.
	 */
	ClusterArray(ClusterArray<T> &&A) : Array<T>(std::move(A)), ClusterArrayBase(A), m_x(A.m_x) { }


	/**
	 * @name Access methods
	 * These methods provide access to elements, size, and corresponding embedding.
	 */
	//@{

	//! Returns true iff the array is associated with a graph.
	bool valid() const { return Array<T>::low() <= Array<T>::high(); }

	//! Returns a pointer to the associated cluster graph.
	const ClusterGraph *graphOf() const {
		return m_pClusterGraph;
	}

	//! Returns a reference to the element with index \p c.
	const T &operator[](cluster c) const {
		OGDF_ASSERT(c != nullptr);
		OGDF_ASSERT(c->graphOf() == m_pClusterGraph);
		return Array<T>::operator [](c->index());
	}

	//! Returns a reference to the element with index \p c.
	T &operator[](cluster c) {
		OGDF_ASSERT(c != nullptr);
		OGDF_ASSERT(c->graphOf() == m_pClusterGraph);
		return Array<T>::operator [](c->index());
	}

	//! Returns a reference to the element with index \p c.
	const T &operator()(cluster c) const {
		OGDF_ASSERT(c != nullptr);
		OGDF_ASSERT(c->graphOf() == m_pClusterGraph);
		return Array<T>::operator [](c->index());
	}

	//! Returns a reference to the element with index \p c.
	T &operator()(cluster c) {
		OGDF_ASSERT(c != nullptr);
		OGDF_ASSERT(c->graphOf() == m_pClusterGraph);
		return Array<T>::operator [](c->index());
	}

	//! Returns a reference to the element with index \p index.
	//! \attention Make sure that \p index is a valid index for a cluster in the associated cluster graph!
	OGDF_DEPRECATED("Cluster arrays should be indexed by a cluster, not an integer index.")
	const T &operator[](int index) const
		{ return Array<T>::operator [](index); }

	//! Returns a reference to the element with index \p index.
	//!\attention Make sure that \p index is a valid index for a cluster in the associated cluster graph!
	OGDF_DEPRECATED("Cluster arrays should be indexed by a cluster, not an integer index.")
	T &operator[](int index)
		{ return Array<T>::operator [](index); }

	//@}
	/**
	 * @name Iterators
	 * These methods return bidirectional iterators to elements in the array.
	 */
	//@{

	//! Returns an iterator to the first entry in the cluster array.
	/**
	 * If the cluster array is empty, a null pointer iterator is returned.
	 */
	iterator begin() { return iterator(m_pClusterGraph->firstCluster(), this); }

	//! Returns a const iterator to the first entry in the cluster array.
	/**
	 * If the cluster array is empty, a null pointer iterator is returned.
	 */
	const_iterator begin() const { return const_iterator(m_pClusterGraph->firstCluster(), this); }

	//! Returns a const iterator to the first entry in the cluster array.
	/**
	 * If the cluster array is empty, a null pointer iterator is returned.
	 */
	const_iterator cbegin() const { return const_iterator(m_pClusterGraph->firstCluster(), this); }

	//! Returns an iterator to one-past-last entry in the cluster array.
	/**
	 * This is always a null pointer iterator.
	 */
	iterator end() { return iterator(nullptr, this); }

	//! Returns a const iterator to one-past-last entry in the cluster array.
	/**
	 * This is always a null pointer iterator.
	 */
	const_iterator end() const { return const_iterator(nullptr, this); }

	//! Returns a const iterator to one-past-last entry in the cluster array.
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

	//! Reinitializes the array. Associates the array with no cluster graph.
	void init() {
		Array<T>::init(); reregister(nullptr);
	}

	//! Reinitializes the array. Associates the array with \p C.
	void init(const ClusterGraph &C) {
		Array<T>::init( C.clusterArrayTableSize() ); reregister(&C);
	}

	//! Reinitializes the array. Associates the array with \p C.
	/**
	 * @param C is the associated cluster graph.
	 * @param x is the default value.
	 */
	void init(const ClusterGraph &C, const T &x) {
		Array<T>::init(0,C.clusterArrayTableSize()-1, m_x = x); reregister(&C);
	}

	//! Sets all array elements to \p x.
	void fill(const T &x) {
		int high = m_pClusterGraph->maxClusterIndex();
		if(high >= 0)
			Array<T>::fill(0,high,x);
	}

	//! Assignment operator.
	ClusterArray<T> &operator=(const ClusterArray<T> &a) {
		Array<T>::operator =(a);
		m_x = a.m_x;
		reregister(a.m_pClusterGraph);
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * Cluster array \p a is empty afterwards and not associated with any cluster graph.
	 */
	ClusterArray<T> &operator=(ClusterArray<T> &&a) {
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
		m_pClusterGraph = nullptr;
	}

	OGDF_NEW_DELETE

};

}
