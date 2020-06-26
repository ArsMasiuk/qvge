/** \file
 * \brief Decralation of graph iterators.
 *
 * \author Carsten Gutwenger, Ivo Hedtke
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

#include <functional>
#include <ogdf/basic/basic.h>
#include <ogdf/basic/Reverse.h>

namespace ogdf {

namespace internal {

template<class GraphObjectPtr, bool isReverse> class GraphIteratorBase;
template<class GraphObjectPtr> using GraphIterator = GraphIteratorBase<GraphObjectPtr,false>;
template<class GraphObjectPtr> using GraphReverseIterator = GraphIteratorBase<GraphObjectPtr,true>;

template<class GraphObjectPtr, bool isReverse>
class GraphIteratorBase {
	friend class GraphIteratorBase<GraphObjectPtr, !isReverse>;

	GraphObjectPtr m_ptr;

public:
	GraphIteratorBase() : m_ptr(nullptr) { }
	GraphIteratorBase(GraphObjectPtr ptr) : m_ptr(ptr) { }

	template<bool isArgReverse>
	GraphIteratorBase(GraphIteratorBase<GraphObjectPtr, isArgReverse> &it) : m_ptr(it.m_ptr) { }

	bool operator==(const GraphIteratorBase<GraphObjectPtr, isReverse> &other) const {
		return m_ptr == other.m_ptr;
	}

	bool operator!=(const GraphIteratorBase<GraphObjectPtr, isReverse> &other) const {
		return m_ptr != other.m_ptr;
	}

	GraphObjectPtr operator*() {
		return m_ptr;
	}

	//! Increment operator (prefix).
	GraphIteratorBase<GraphObjectPtr, isReverse> &operator++() {
		OGDF_ASSERT(m_ptr != nullptr);
		m_ptr = isReverse ? m_ptr->pred() : m_ptr->succ();
		return *this;
	}

	//! Increment operator (postfix).
	GraphIteratorBase<GraphObjectPtr, isReverse> operator++(int) {
		OGDF_ASSERT(m_ptr != nullptr);
		GraphObjectPtr ptr = m_ptr;
		m_ptr = isReverse ? m_ptr->pred() : m_ptr->succ();
		return ptr;
	}

	//! Decrement operator (prefix).
	GraphIteratorBase<GraphObjectPtr, isReverse> &operator--() {
		OGDF_ASSERT(m_ptr != nullptr);
		m_ptr = isReverse ? m_ptr->succ() : m_ptr->pred();
		return *this;
	}

	//! Decrement operator (postfix).
	GraphIteratorBase<GraphObjectPtr, isReverse> operator--(int) {
		OGDF_ASSERT(m_ptr != nullptr);
		GraphObjectPtr ptr = m_ptr;
		m_ptr = isReverse ? m_ptr->succ() : m_ptr->pred();
		return ptr;
	}
};

template<class ArrayType, bool isConst> class GraphArrayIteratorBase;
template<class ArrayType> using GraphArrayIterator = GraphArrayIteratorBase<ArrayType, false>;
template<class ArrayType> using GraphArrayConstIterator = GraphArrayIteratorBase<ArrayType, true>;

template<class ArrayType, bool isConst> class GraphArrayIteratorBase {
	friend class GraphArrayIteratorBase<ArrayType, true>;

public:
	//! Index type of the associated array.
	using key_type = typename ArrayType::key_type;
	//! Value type of the associated array.
	using value_type = typename std::conditional<isConst, const typename ArrayType::value_type, typename ArrayType::value_type>::type;
	//! Type of the array.
	using array_pointer_type = typename std::conditional<isConst, const ArrayType*, ArrayType*>::type;

	//! Constructor.
	GraphArrayIteratorBase()
	: m_key(nullptr), m_array(nullptr) {}

	//! Constructor.
	GraphArrayIteratorBase(key_type key, array_pointer_type a)
	: m_key(key), m_array(a) { }

	//! Constructor.
	template<bool isArgConst, typename std::enable_if<isConst || !isArgConst, int>::type = 0>
	GraphArrayIteratorBase(const GraphArrayIteratorBase<ArrayType, isArgConst> &iter)
	: GraphArrayIteratorBase(iter.m_key, iter.m_array) { }

	//! Copy constructor.
	// gcc9 complains since it cannot match the templated constructor above (-Werror=deprecated-copy).
	GraphArrayIteratorBase(const GraphArrayIteratorBase<ArrayType, isConst> &iter)
	: GraphArrayIteratorBase(iter.m_key, iter.m_array) { }

	//! Copy assignment operator.
	GraphArrayIteratorBase<ArrayType, isConst> &operator=(const GraphArrayIteratorBase<ArrayType, isConst> &iter) {
		m_key = iter.m_key;
		m_array = iter.m_array;
		return *this;
	}

	//! Index in #m_array.
	key_type key() const { return m_key; }

	//! Value of #m_array at index #m_key.
	value_type &value() const { return (*m_array)[m_key]; }

	//! Value of #m_array at index #m_key.
	value_type &operator*() const { return (*m_array)[m_key]; }

	//! Equality operator.
	bool operator==(const GraphArrayIteratorBase<ArrayType, isConst> &iter) const {
		return m_key == iter.m_key && m_array == iter.m_array;
	}

	//! Inequality operator.
	bool operator!=(const GraphArrayIteratorBase<ArrayType, isConst> &iter) const {
		return !operator==(iter);
	}

	//! Increment operator (prefix).
	GraphArrayIteratorBase<ArrayType, isConst> &operator++() {
		OGDF_ASSERT(m_key != nullptr);
		m_key = ArrayType::findSuccKey(m_key);
		return *this;
	}

	//! Increment operator (postfix).
	GraphArrayIteratorBase<ArrayType, isConst> operator++(int) {
		GraphArrayIteratorBase<ArrayType, isConst> iter = *this;
		m_key = ArrayType::findSuccKey(m_key);
		return iter;
	}

	//! Decrement operator (prefix).
	GraphArrayIteratorBase<ArrayType, isConst> &operator--() {
		OGDF_ASSERT(m_key != nullptr);
		m_key = ArrayType::findPredKey(m_key);
		return *this;
	}

	//! Decrement operator (postfix).
	GraphArrayIteratorBase<ArrayType, isConst> operator--(int) {
		GraphArrayIteratorBase<ArrayType, isConst> iter = *this;
		m_key = ArrayType::findPredKey(m_key);
		return iter;
	}

private:
	key_type m_key; //!< Index in #m_array.
	array_pointer_type m_array; //!< The array.
};

}
}
