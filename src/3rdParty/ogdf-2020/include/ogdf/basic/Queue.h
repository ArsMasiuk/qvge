/** \file
 * \brief Declaration and implementation of list-based queues
 *        (classes QueuePure<E> and Queue<E>).
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

#include <ogdf/basic/SList.h>


namespace ogdf {


//! Implementation of list-based queues.
/**
 * @ingroup containers
 *
 * In contrast to Queue<E>, instances of QueuePure<E> do not store the
 * number of elements contained in the queue.
 *
 * @tparam E is the element type.
 */
template<class E> class QueuePure : private SListPure<E> {
public:
	//! Represents the data type stored in a queue element.
	using value_type = E;
	//! Provides a reference to an element stored in a queue.
	using reference = E&;
	//! Provides a reference to a const element stored in a queue for reading and performing const operations.
	using const_reference = const E&;
	//! Provides a forward iterator that can read a const element in a queue.
	using const_iterator = SListConstIterator<E>;
	//! Provides a forward iterator that can read or modify any element in a queue.
	using iterator = SListIterator<E>;

	//! Constructs an empty queue.
	QueuePure() { }

	//! Constructs a queue and appends the elements in \p initList to it.
	QueuePure(std::initializer_list<E> initList) : SListPure<E>(initList) { }

	//! Constructs a queue that is a copy of \p Q.
	QueuePure(const QueuePure<E> &Q) : SListPure<E>(Q) { }

	//! Constructs a queue containing the elements of \p Q (move semantics).
	/**
	 * Queue \p Q is empty afterwards.
	 */
	QueuePure(QueuePure<E> &&Q) : SListPure<E>(std::move(Q)) { }

	//! Destruction
	~QueuePure() { }

	/**
	* @name Access methods
	* These methods provide simple access without changing the list.
	*/
	//@{

	//! Returns true iff the queue is empty.
	bool empty() const { return SListPure<E>::empty(); }

	//! Returns a reference to the front element.
	const_reference top() const {
		return SListPure<E>::front();
	}

	//! Returns a reference to the front element.
	reference top() {
		return SListPure<E>::front();
	}

	//! Returns a reference to the back element.
	const_reference bottom() const {
		return SListPure<E>::back();
	}

	//! Returns a reference to the back element.
	reference bottom() {
		return SListPure<E>::back();
	}

	//@}
	/**
	* @name Iterators
	* These methods return forward iterators to elements in the queue.
	*/
	//@{

	//! Returns an iterator to the first element of the queue.
	iterator begin() { return SListPure<E>::begin(); }

	//! Returns a const iterator to the first element of the queue.
	const_iterator begin() const { return SListPure<E>::begin(); }

	//! Returns a const iterator to the first element of the queue.
	const_iterator cbegin() const { return SListPure<E>::cbegin(); }

	//! Returns an iterator to one-past-last element of the queue.
	iterator end() { return SListPure<E>::end(); }

	//! Returns a const iterator to one-past-last element of the queue.
	const_iterator end() const { return SListPure<E>::end(); }

	//! Returns a const iterator to one-past-last element of the queue.
	const_iterator cend() const { return SListPure<E>::cend(); }

	//! Returns an iterator to the last element of the queue.
	iterator backIterator() { return SListPure<E>::backIterator(); }

	//! Returns a const iterator to the last element of the queue.
	const_iterator backIterator() const { return SListPure<E>::backIterator(); }

	//@}
	/**
	* @name Operators
	* The following operators are provided by lists.
	*/
	//@{

	//! Assignment operator.
	QueuePure<E> &operator=(const QueuePure<E> &Q) {
		SListPure<E>::operator=(Q);
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * Queue \p Q is empty afterwards.
	 */
	QueuePure<E> &operator=(QueuePure<E> &&Q) {
		SListPure<E>::operator=(std::move(Q));
		return *this;
	}

	//! Conversion to const SListPure.
	const SListPure<E> &getListPure() const { return *this; }

	//@}
	/**
	* @name Adding and removing elements
	* These method add elements to the list and remove elements from the list.
	*/
	//@{

	//! Adds \p x at the end of queue.
	iterator append(const E &x) {
		return SListPure<E>::pushBack(x);
	}

	//! Adds a new element at the end of the queue.
	/**
	* The element is constructed in-place with exactly the same arguments \p args as supplied to the function.
	*/
	template<class ... Args>
	iterator emplace(Args && ... args) {
		return SListPure<E>::emplaceBack(std::forward<Args>(args)...);
	}

	//! Removes front element and returns it.
	E pop() {
		E x = top();
		SListPure<E>::popFront();
		return x;
	}

	//! Makes the queue empty.
	void clear() { SListPure<E>::clear(); }

	OGDF_NEW_DELETE
};

//! The parameterized class Queue<E> implements list-based queues.
/**
 * @ingroup containers
 *
 * In contrast to QueuePure<E>, instances of Queue<E> store the
 * number of elements contained in the queue.
 *
 * @tparam E is the element type.
 */
template<class E> class Queue : private SList<E> {
public:
	//! Represents the data type stored in a queue element.
	using value_type = E;
	//! Provides a reference to an element stored in a queue.
	using reference = E&;
	//! Provides a reference to a const element stored in a queue for reading and performing const operations.
	using const_reference = const E &;
	//! Provides a forward iterator that can read a const element in a queue.
	using const_iterator = SListConstIterator<E>;
	//! Provides a forward iterator that can read or modify any element in a queue.
	using iterator = SListIterator<E>;

	//! Constructs an empty queue.
	Queue() { }

	//! Constructs a queue and appends the elements in \p initList to it.
	Queue(std::initializer_list<E> initList) : SList<E>(initList) { }

	//! Constructs a queue that is a copy of \p Q.
	Queue(const Queue<E> &Q) : SList<E>(Q) { }

	//! Constructs a queue containing the elements of \p Q (move semantics).
	/**
	 * Queue \p Q is empty afterwards.
	 */
	Queue(Queue<E> &&Q) : SList<E>(std::move(Q)) { }

	//! Destruction
	~Queue() { }

	/**
	* @name Access methods
	* These methods provide simple access without changing the list.
	*/
	//@{

	//! Returns true iff the queue is empty.
	bool empty() const { return SList<E>::empty(); }

	//! Returns the number of elements in the queue.
	int size() const { return SList<E>::size(); }

	//! Returns a reference to the front element.
	const_reference top() const {
		return SList<E>::front();
	}

	//! Returns a reference to the front element.
	reference top() {
		return SList<E>::front();
	}

	//! Returns a reference to the back element.
	const_reference bottom() const {
		return SListPure<E>::back();
	}

	//! Returns a reference to the back element.
	reference bottom() {
		return SListPure<E>::back();
	}

	//@}
	/**
	* @name Iterators
	* These methods return forward iterators to elements in the queue.
	*/
	//@{

	//! Returns an iterator to the first element of the queue.
	iterator begin() { return SList<E>::begin(); }

	//! Returns a const iterator to the first element of the queue.
	const_iterator begin() const { return SList<E>::begin(); }

	//! Returns a const iterator to the first element of the queue.
	const_iterator cbegin() const { return SList<E>::cbegin(); }

	//! Returns an iterator to one-past-last element of the queue.
	iterator end() { return SList<E>::end(); }

	//! Returns a const iterator to one-past-last element of the queue.
	const_iterator end() const { return SList<E>::end(); }

	//! Returns a const iterator to one-past-last element of the queue.
	const_iterator cend() const { return SList<E>::cend(); }

	//! Returns an iterator to the last element of the queue.
	iterator backIterator() { return SList<E>::backIterator(); }

	//! Returns a const iterator to the last element of the queue.
	const_iterator backIterator() const { return SList<E>::backIterator(); }

	//@}
	/**
	* @name Operators
	* The following operators are provided by lists.
	*/
	//@{

	//! Assignment operator.
	Queue<E> &operator=(const Queue<E> &Q) {
		SList<E>::operator=(Q);
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * Queue \p Q is empty afterwards.
	 */
	Queue<E> &operator=(Queue<E> &&Q) {
		SList<E>::operator=(std::move(Q));
		return *this;
	}

	//! Conversion to const SList.
	const SList<E> &getList() const { return *this; }

	//@}
	/**
	* @name Adding and removing elements
	* These method add elements to the list and remove elements from the list.
	*/
	//@{

	//! Adds \p x at the end of queue.
	iterator append(const E &x) {
		return SList<E>::pushBack(x);
	}

	//! Adds a new element at the end of the queue.
	/**
	* The element is constructed in-place with exactly the same arguments \p args as supplied to the function.
	*/
	template<class ... Args>
	iterator emplace(Args && ... args) {
		return SList<E>::emplaceBack(std::forward<Args>(args)...);
	}

	//! Removes front element and returns it.
	E pop() {
		E x = top();
		SList<E>::popFront();
		return x;
	}

	//! Makes the queue empty.
	void clear() { SList<E>::clear(); }

	//@{

	OGDF_NEW_DELETE
};

// prints queue to output stream os using delimiter delim
template<class E>
void print(std::ostream &os, const QueuePure<E> &Q, char delim = ' ')
{ print(os,Q.getListPure(),delim); }

// prints queue to output stream os using delimiter delim
template<class E>
void print(std::ostream &os, const Queue<E> &Q, char delim = ' ')
{ print(os,Q.getList(),delim); }


// output operator
template<class E>
std::ostream &operator<<(std::ostream &os, const QueuePure<E> &Q)
{
	print(os,Q); return os;
}

template<class E>
std::ostream &operator<<(std::ostream &os, const Queue<E> &Q)
{
	print(os,Q); return os;
}

}
