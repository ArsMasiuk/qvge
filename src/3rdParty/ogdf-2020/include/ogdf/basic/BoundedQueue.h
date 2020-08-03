/** \file
 * \brief Declaration and implementation of bounded queue class.
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

#include <ogdf/basic/exceptions.h>

namespace ogdf {

//! The parameterized class BoundedQueue implements queues with bounded size.
/**
 * @ingroup containers
 *
 * @tparam E     is the element type.
 * @tparam INDEX is the index type. The default index type is \c int, other possible types
 *               are \c short and <code>long long</code> (on 64-bit systems).
 */
template<class E, class INDEX = int> class BoundedQueue {

	E *m_pStart; //! Pointer to first element of current sequence.
	E *m_pEnd;   //! Pointer to one past last element of current sequence.
	E *m_pStop;  //! Pointer to one past last element of total array.
	E *m_pFirst; //! Pointer to  first element of total array.

public:
	//! Creates a non-valid bounded queue. Needs to be reinitialized first.
	BoundedQueue() {
		m_pStart = m_pEnd = m_pFirst = m_pStop = nullptr;
	}

	//! Constructs an empty bounded queue for at most \p n elements.
	explicit BoundedQueue(INDEX n) {
		OGDF_ASSERT(n >= 1);
		m_pStart = m_pEnd = m_pFirst = new E[n+1];
		if (m_pFirst == nullptr) OGDF_THROW(InsufficientMemoryException);

		m_pStop = m_pFirst+n+1;
	}

	//! Constructs a bounded queue that is a copy of \p Q.
	BoundedQueue(const BoundedQueue<E> &Q) {
		copy(Q);
	}

	//! Constructs a bounded queue containing the elements of \p Q (move semantics).
	/**
	 * The queue \p Q is non valid afterwards, i.e., its capacity is zero.
	 * It has to be reinitialized if new elements shall be appended.
	 */
	BoundedQueue(BoundedQueue<E> &&Q) {
		m_pStart = Q.m_pStart;
		m_pEnd   = Q.m_pEnd;
		m_pStop  = Q.m_pStop;
		m_pFirst = Q.m_pFirst;
		Q.m_pStart = Q.m_pEnd = Q.m_pFirst = Q.m_pStop = nullptr;
	}

	//! Destruction
	~BoundedQueue() { delete[] m_pFirst; }

	//! Reinitializes the bounded queue to a non-valid bounded queue.
	void init() {
		delete[] m_pFirst;
		m_pStart = m_pEnd = m_pFirst = m_pStop = nullptr;
	}

	//! Reinitializes the bounded queue to a bounded queue for at most \p n elements.
	void init(INDEX n) {
		delete[] m_pFirst;

		OGDF_ASSERT(n >= 1);
		m_pStart = m_pEnd = m_pFirst = new E[n+1];
		if (m_pFirst == nullptr) OGDF_THROW(InsufficientMemoryException);

		m_pStop = m_pFirst+n+1;
	}

	//! Returns front element.
	const E &top() const {
		OGDF_ASSERT(m_pStart != m_pEnd);
		return *m_pStart;
	}

	//! Returns front element.
	E &top() {
		OGDF_ASSERT(m_pStart != m_pEnd);
		return *m_pStart;
	}

	//! Returns back element.
	const E &bottom() const {
		OGDF_ASSERT(m_pStart != m_pEnd);
		if (m_pEnd == m_pFirst) return *(m_pStop-1);
		else return *(m_pEnd-1);
	}

	//! Returns back element.
	E &bottom() {
		OGDF_ASSERT(m_pStart != m_pEnd);
		if (m_pEnd == m_pFirst) return *(m_pStop-1);
		else return *(m_pEnd-1);
	}

	//! Returns current size of the queue.
	INDEX size() const {
		return (m_pEnd >= m_pStart) ?
			(INDEX)(m_pEnd - m_pStart) :
			(INDEX)((m_pEnd-m_pFirst)+(m_pStop-m_pStart));
	}

	//! Returns the capacity of the bounded queue.
	INDEX capacity() const { return (INDEX)((m_pStop - m_pFirst)-1); }

	//! Returns true iff the queue is empty.
	bool empty() { return m_pStart == m_pEnd; }

	//! Returns true iff the queue is full.
	bool full() {
		INDEX h = m_pEnd-m_pStart;
		return h >= 0 ? h == m_pStop - m_pFirst - 1 : h == -1;
	}

	//! Assignment operator.
	BoundedQueue<E> &operator=(const BoundedQueue<E> &Q) {
		delete[] m_pFirst;
		copy(Q);
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * The queue \p Q is non valid afterwards, i.e., its capacity is zero.
	 * It has to be reinitialized if new elements shall be appended.
	 */
	BoundedQueue<E> &operator=(BoundedQueue<E> &&Q) {
		delete[] m_pFirst;

		m_pStart = Q.m_pStart;
		m_pEnd   = Q.m_pEnd;
		m_pStop  = Q.m_pStop;
		m_pFirst = Q.m_pFirst;
		Q.m_pStart = Q.m_pEnd = Q.m_pFirst = Q.m_pStop = nullptr;

		return *this;
	}


	//! Adds \p x at the end of queue.
	void append(const E &x) {
		*m_pEnd++ = x;
		if (m_pEnd == m_pStop) m_pEnd = m_pFirst;
		OGDF_ASSERT(m_pStart != m_pEnd);
	}

	//! Removes front element and returns it.
	E pop() {
		OGDF_ASSERT(m_pStart != m_pEnd);
		E x = *m_pStart++;
		if (m_pStart == m_pStop) m_pStart = m_pFirst;
		return x;
	}

	//! Makes the queue empty.
	void clear() { m_pStart = m_pEnd = m_pFirst; }

	//! Prints the queue to output stream \p os with the seperator \p delim.
	void print(std::ostream &os, char delim = ' ') const
	{
		for (const E *pX = m_pStart; pX != m_pEnd; ) {
			if (pX != m_pStart) os << delim;
			os << *pX;
			if (++pX == m_pStop) pX = m_pFirst;
		}
	}

private:
	void copy(const BoundedQueue<E> &Q) {
		int n = Q.size()+1;
		m_pEnd = m_pStart = m_pFirst = new E[n];
		if (m_pFirst == nullptr) OGDF_THROW(InsufficientMemoryException);

		m_pStop = m_pStart + n;
		for (E *pX = Q.m_pStart; pX != Q.m_pEnd; ) {
			*m_pEnd++ = *pX++;
			if (pX == Q.m_pStop) pX = Q.m_pFirst;
		}
	}
};

//! Prints BoundedQueue \p Q to output stream \p os.
template<class E, class INDEX>
std::ostream &operator<<(std::ostream &os, const BoundedQueue<E,INDEX> &Q)
{
	Q.print(os);
	return os;
}

}
