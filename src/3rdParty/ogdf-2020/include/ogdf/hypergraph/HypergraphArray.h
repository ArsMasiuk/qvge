/** \file
 * \brief Declaration and implementation of hyperraph array classes
 *        based on Node/EdgeArray classes written by Carsten Gutwenger,
 *        but slightly modified (base class is common for both arrays).
 *
 * \author Ondrej Moris
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

#include <ogdf/hypergraph/Hypergraph.h>

namespace ogdf {

//! Abstract base class for hypergraph arrays.
class HypergraphArrayBase {

public:

	/**
	 * Pointer to list element in the list of all registered hypergraph
	 * arrays which references this array.
	 */
	ListIterator<HypergraphArrayBase *> m_it;

	//! The associated hypergraph.
	const Hypergraph *m_hypergraph;

	//! Initializes an array not associated with a hypergraph.
	HypergraphArrayBase()
		: m_hypergraph(nullptr)
	{
	}

	//! Initializes an array associated with \p pH.
	explicit HypergraphArrayBase(const Hypergraph *pH)
	{
	m_hypergraph = pH;
	}

	//! Destructor, unregisters the array.
	virtual ~HypergraphArrayBase()
	{
	}

	//! Returns a pointer to the associated hypergraph.
	const Hypergraph *hypergraphOf() const {
		return m_hypergraph;
	}

	//! Table re-initialization.
	virtual void reinit(int initTableSize) = 0;

	//! Associates the array with a new hypergraph.
	virtual void reregister(const Hypergraph *H) = 0;

	//! Table size enlargement.
	virtual void enlargeTable(int newTableSize) = 0;

	//! Disconnetion from the hypergraph.
	virtual void disconnect() = 0;
};

//! Dynamic arrays indexed with hypernodes.
/**
 * @warn_undef_behavior_array
 */
template<class T> class HypernodeArray :
private Array<T>, protected HypergraphArrayBase {

	//! The default value for array elements.
	T m_x;

public:

	//! Constructs an empty hypernode array associated with no hypergraph.
HypernodeArray() : Array<T>(), HypergraphArrayBase() { }

	//! Constructs a hypernode array associated with \p H.
	HypernodeArray(const Hypergraph &H, const T &x)
		: Array<T>(0,H.hypernodeArrayTableSize() - 1, x),
		  HypergraphArrayBase(&H), m_x(x)
	{
		m_it = H.registerHypernodeArray(this);
	}

	virtual ~HypernodeArray()
	{
		if (m_hypergraph)
			m_hypergraph->unregisterHypernodeArray(m_it);
	}

	//! Returns a reference to the element with index \p v.
	T &operator[](hypernode v)
	{
		return Array<T>::operator [](v->index());
	}

	//! Returns a reference to the element with index \p index.
	const T &operator[](int index) const
	{
		return Array<T>::operator [](index);
	}

	//! Returns a reference to the element with index \p index.
	T &operator[](int index)
	{
		return Array<T>::operator [](index);
	}

	//! Assignment operator.
	HypernodeArray<T> &operator=(const HypernodeArray<T> &a)
	{
		Array<T>::operator =(a);
		m_x = a.m_x;
		reregister(a.m_hypergraph);
		return *this;
	}

	//! Reinitializes the array. Associates the array with \p H.
	void init(const Hypergraph &H)
	{
		Array<T>::init(H.hypernodeArrayTableSize()); reregister(&H);
	}

	//! Reinitializes the array. Associates the array with \p H.
	void init(const Hypergraph &H, const T &x)
	{
		Array<T>::init(0, H.hypernodeArrayTableSize() - 1, m_x = 0);
		reregister(&H);
	}

	virtual void reregister(const Hypergraph *H)
	{
		if (m_hypergraph)
			m_hypergraph->unregisterHypernodeArray(m_it);

		if ((m_hypergraph = H) != nullptr)
			m_it = H->registerHypernodeArray(this);
	}

private:

	virtual void enlargeTable(int newTableSize)
	{
		Array<T>::resize(newTableSize, m_x);
	}

	virtual void reinit(int initTableSize)
	{
		Array<T>::init(0, initTableSize - 1, m_x);
	}

	virtual void disconnect()
	{
		Array<T>::init();
		m_hypergraph = nullptr;
	}

	OGDF_NEW_DELETE;
};

//! Dynamic arrays indexed with nodes.
/**
 * @warn_undef_behavior_array
 */
template<class T> class HyperedgeArray :
private Array<T>, protected HypergraphArrayBase {

	//! The default value for array elements.
	T m_x;

public:

	//! Constructs an empty hypernode array associated with no graph.
	HyperedgeArray()
		: Array<T>(), HypergraphArrayBase()
	{
	}

	//! Constructs a hypernode array associated with \p H.
	HyperedgeArray(const Hypergraph &H, const T &x)
		: Array<T>(0, H.hyperedgeArrayTableSize() - 1, x),
		  HypergraphArrayBase(&H), m_x(x)
	{
		m_it = H.registerHyperedgeArray(this);
	}

	//! Destructor.
	virtual ~HyperedgeArray()
	{
		if (m_hypergraph)
			m_hypergraph->unregisterHyperedgeArray(m_it);
	}

	//! Returns true iff the array is associated with a hypergraph.
	bool valid() const
	{
		return Array<T>::low() <= Array<T>::high();
	}

	//! Returns a reference to the element with the index of \p e.
	T &operator[](hyperedge e)
	{
		return Array<T>::operator [](e->index());
	}

	//! Returns a reference to the element with index \p index.
	const T &operator[](int index) const
	{
		return Array<T>::operator [](index);
	}

	//! Returns a reference to the element with index \p index.
	T &operator[](int index)
	{
		return Array<T>::operator [](index);
	}

	//! Assignment operator.
	HyperedgeArray<T> &operator=(const HyperedgeArray<T> &a)
	{
		Array<T>::operator =(a);
		m_x = a.m_x;
		reregister(a.m_hypergraph);
		return *this;
	}

	//! Reinitializes the array. Associates the array with \p H.
	void init(const Hypergraph &H)
	{
		Array<T>::init(H.hyperedgeArrayTableSize()); reregister(&H);
	}

	//! Reinitializes the array. Associates the array with \p H.
	void init(const Hypergraph &H, const T &x)
	{
		Array<T>::init(0, H.hyperedgeArrayTableSize() - 1, m_x = 0);
		reregister(&H);
	}

	virtual void reregister(const Hypergraph *H)
	{
		if (m_hypergraph)
			m_hypergraph->unregisterHyperedgeArray(m_it);

		if ((m_hypergraph = H) != nullptr)
			m_it = H->registerHyperedgeArray(this);
	}

private:

	virtual void enlargeTable(int newTableSize)
	{
		Array<T>::resize(newTableSize, m_x);
	}

	virtual void reinit(int initTableSize)
	{
		Array<T>::init(0, initTableSize-1, m_x);
	}

	virtual void disconnect()
	{
		Array<T>::init();
		m_hypergraph = nullptr;
	}

	OGDF_NEW_DELETE;
};

}
