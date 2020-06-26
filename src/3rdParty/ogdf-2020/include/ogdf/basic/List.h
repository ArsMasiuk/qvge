/** \file
 * \brief Declaration of doubly linked lists and iterators
 *
 * \author Carsten Gutwenger, Sebastian Leipert, Tilo Wiedera
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

#include <ogdf/basic/internal/list_templates.h>
#include <ogdf/basic/Reverse.h>
#include <random>

namespace ogdf {

template<class E> class List;
template<class E> class ListPure;
template<class E, bool isConst, bool isReverse> class ListIteratorBase;
template<class E> using ListConstIterator = ListIteratorBase<E, true, false>;
template<class E> using ListIterator = ListIteratorBase<E, false, false>;
template<class E> using ListConstReverseIterator = ListIteratorBase<E, true, true>;
template<class E> using ListReverseIterator = ListIteratorBase<E, false, true>;

//! Structure for elements of doubly linked lists.
template<class E>
class ListElement {
	friend class ListPure<E>;
	friend class List<E>;
	friend class ListIteratorBase<E, true, true>;
	friend class ListIteratorBase<E, false, true>;
	friend class ListIteratorBase<E, true, false>;
	friend class ListIteratorBase<E, false, false>;

	ListElement<E> *m_next; //!< Pointer to successor element.
	ListElement<E> *m_prev; //!< Pointer to predecessor element.
	E m_x; //!< Stores the content.
#ifdef OGDF_DEBUG
	ListPure<E> *m_list; //!< List object that the element belongs to.
#endif

	//! Constructs a ListElement.
	ListElement(ListPure<E> *list, const E &x, ListElement<E> *next, ListElement<E> *prev)
			: m_next(next), m_prev(prev), m_x(x) {
#ifdef OGDF_DEBUG
		m_list = list;
#endif
	}
	//! Constructs a ListElement.
	ListElement(ListPure<E> *list, const E &x) : ListElement(list, x, nullptr, nullptr) { }
	//! Constructs a ListElement with given arguments \p args for constructor of element type.
	template<class ... Args>
	ListElement(ListPure<E> *list, ListElement<E> *next, ListElement<E> *prev, Args && ... args)
			: ListElement(list, E(std::forward<Args>(args)...), next, prev) { }

	OGDF_NEW_DELETE
};

//! Encapsulates a pointer to a list element.
/**
 * It is used in order to iterate over doubly linked lists,
 * and to specify a position in a doubly linked list. It is possible that
 * an iterator encapsulates a null pointer.
 *
 * @tparam E The type of element.
 * @tparam isConst True iff this iterator allows only const-access to the element.
 * @tparam isReverse True iff this iterator is a reverse iterator.
 */
template<class E, bool isConst, bool isReverse> class ListIteratorBase {
	friend class ListIteratorBase<E, !isConst, isReverse>;
	friend class ListIteratorBase<E, isConst, !isReverse>;
	friend class ListIteratorBase<E, !isConst, !isReverse>;
	friend class ListPure<E>;
	//! The underlying list element, depending on isConst
	using ListElem = typename std::conditional<isConst, const ListElement<E>, ListElement<E>>::type;
	//! The underlying type, depending on isConst
	using Elem = typename std::conditional<isConst, const E, E>::type;

	//! pointer to list element
	ListElem *m_pX;

	//! Conversion to pointer to list element.
	operator ListElem *() { return m_pX; }

public:
	//! Constructs an iterator that points to \p pX.
	ListIteratorBase(ListElem *pX) : m_pX(pX) { }

	//! Constructs an invalid iterator.
	ListIteratorBase() : ListIteratorBase(nullptr) { }

	//! Constructs an iterator that is a copy of \p it.
	template<bool isArgConst, typename std::enable_if<isConst || !isArgConst, int>::type = 0, bool isArgReverse>
	ListIteratorBase(const ListIteratorBase<E,isArgConst,isArgReverse> &it) : ListIteratorBase(it.m_pX) { }

	//! Copy constructor.
	// gcc9 complains since it cannot match the templated constructor above (-Werror=deprecated-copy).
	ListIteratorBase(const ListIteratorBase<E, isConst, isReverse> &it) : ListIteratorBase(it.m_pX) { }

	//! Returns true iff the iterator points to an element.
	bool valid() const { return m_pX != nullptr; }

#ifdef OGDF_DEBUG
	//! Returns the list that this iterator belongs to.
	//! \pre This iterator is valid.
	ListPure<E> *listOf() {
		OGDF_ASSERT(valid());
		return m_pX->m_list;
	}
#endif
	//! Equality operator.
	bool operator==(const ListIteratorBase<E, isConst, isReverse> &it) const {
		return m_pX == it.m_pX;
	}

	//! Inequality operator.
	bool operator!=(const ListIteratorBase<E, isConst, isReverse> &it) const {
		return m_pX != it.m_pX;
	}

	//! Returns successor iterator.
	ListIteratorBase<E, isConst, isReverse> succ() const {
		return isReverse ? m_pX->m_prev : m_pX->m_next;
	}

	//! Returns predecessor iterator.
	ListIteratorBase<E, isConst, isReverse> pred() const {
		return isReverse ? m_pX->m_next : m_pX->m_prev;
	}

	//! Returns a reference to the element content.
	Elem &operator*() const { return m_pX->m_x; }

	//! Assignment operator.
	ListIteratorBase<E, isConst, isReverse> &operator=(const ListIteratorBase<E, isConst, isReverse> &it) {
		m_pX = it.m_pX;
		return *this;
	}

	//! Increment operator (prefix).
	ListIteratorBase<E, isConst, isReverse> &operator++() {
		m_pX = isReverse ? m_pX->m_prev : m_pX->m_next;
		return *this;
	}

	//! Increment operator (postfix).
	ListIteratorBase<E, isConst, isReverse> operator++(int) {
		ListIteratorBase<E, isConst, isReverse> it = *this;
		m_pX = isReverse ? m_pX->m_prev : m_pX->m_next;
		return it;
	}

	//! Decrement operator (prefix).
	ListIteratorBase<E, isConst, isReverse> &operator--() {
		m_pX = isReverse ? m_pX->m_next : m_pX->m_prev;
		return *this;
	}

	//! Decrement operator (postfix).
	ListIteratorBase<E, isConst, isReverse> operator--(int) {
		ListIteratorBase<E, isConst, isReverse> it = *this;
		m_pX = isReverse ? m_pX->m_next : m_pX->m_prev;
		return it;
	}

	OGDF_NEW_DELETE
};

//! Doubly linked lists.
/**
 * @ingroup containers
 *
 * Use ogdf::ListConstIterator or ogdf::ListIterator in order to iterate over the list.
 *
 * In contrast to ogdf::List, instances of ogdf::ListPure do not store the length of the list.
 *
 * @tparam E is the data type stored in list elements.
 */
template<class E> class ListPure {
protected:
	ListElement<E> *m_head; //!< Pointer to first element.
	ListElement<E> *m_tail; //!< Pointer to last element.

public:
	//! Represents the data type stored in a list element.
	using value_type = E;
	//! Provides a reference to an element stored in a list.
	using reference = E&;
	//! Provides a reference to a const element stored in a list for reading and performing const operations.
	using const_reference = const E&;
	//! Provides a bidirectional iterator that can read a const element in a list.
	using const_iterator = ListConstIterator<E>;
	//! Provides a bidirectional iterator that can read or modify any element in a list.
	using iterator = ListIterator<E>;
	//! Provides a bidirectional reverse iterator that can read a const element in a list.
	using const_reverse_iterator = ListConstReverseIterator<E>;
	//! Provides a bidirectional reverse iterator that can read or modify any element in a list.
	using reverse_iterator = ListReverseIterator<E>;

	//! Constructs an empty doubly linked list.
	ListPure() : m_head(nullptr), m_tail(nullptr) { }

	//! Constructs a doubly linked list containing the elements in \p init.
	ListPure(std::initializer_list<E> init) : m_head(nullptr), m_tail(nullptr) {
		for (const E &x : init)
			pushBack(x);
	}

	//! Constructs a doubly linked list that is a copy of \p L.
	ListPure(const ListPure<E> &L) : m_head(nullptr), m_tail(nullptr) {
		copy(L);
	}

	//! Constructs a doubly linked list containing the elements of \p L (move semantics).
	/**
	 * The list \p L is empty afterwards.
	 */
	ListPure(ListPure<E> &&L) : m_head(L.m_head), m_tail(L.m_tail) {
		L.m_head = L.m_tail = nullptr;
	}

	//! Destructor.
	virtual ~ListPure() { clear(); }


	/**
	 * @name Access methods
	 * These methods provide simple access without changing the list.
	 */
	//@{

	//! Returns true iff the list is empty.
	bool empty() const { return m_head == nullptr; }

	//! Returns the number of elements in the list.
	/**
	 * Notice that this method requires to iterate over the whole list and takes linear running time!
	 * If you require frequent access to the size of the list, consider using ogdf::List instead of ogdf::ListPure.
	 */
	virtual int size() const {
		int count = 0;
		for (ListElement<E> *pX = m_head; pX; pX = pX->m_next)
			++count;
		return count;
	}

	//! Returns a const reference to the first element.
	/**
	 * \pre The list is not empty!
	 */
	const_reference front() const {
		OGDF_ASSERT(m_head != nullptr);
		return m_head->m_x;
	}

	//! Returns a reference to the first element.
	/**
	 * \pre The list is not empty!
	 */
	reference front() {
		OGDF_ASSERT(m_head != nullptr);
		return m_head->m_x;
	}

	//! Returns a const reference to the last element.
	/**
	 * \pre The list is not empty!
	 */
	const_reference back() const {
		OGDF_ASSERT(m_tail != nullptr);
		return m_tail->m_x;
	}

	//! Returns a reference to the last element.
	/**
	 * \pre The list is not empty!
	 */
	reference back() {
		OGDF_ASSERT(m_tail != nullptr);
		return m_tail->m_x;
	}

	//! Returns a const iterator pointing to the element at position \p pos.
	/**
	 * The running time of this method is linear in \p pos.
	 */
	const_iterator get(int pos) const {
		ListElement<E> *pX;
		for(pX = m_head; pX != nullptr; pX = pX->m_next)
			if (pos-- == 0) break;
		return pX;
	}

	//! Returns an iterator pointing to the element at position \p pos.
	/**
	 * The running time of this method is linear in \p pos.
	 */
	iterator get(int pos) {
		ListElement<E> *pX;
		for(pX = m_head; pX != nullptr; pX = pX->m_next)
			if (pos-- == 0) break;
		return pX;
	}

	//! Returns the position (starting with 0) of iterator \p it in the list.
	/**
	 * \pre \p it is a valid iterator pointing to an element in this list!
	 */
	int pos(const_iterator it) const {
		OGDF_ASSERT(it.listOf() == this);
		int p = 0;
		for(ListElement<E> *pX = m_head; pX != nullptr; pX = pX->m_next, ++p)
			if (pX == it) break;
		return p;
	}


	//@}
	/**
	 * @name Iterators
	 * These methods return bidirectional iterators to elements in the list and allow to iterate over the elements in linear or cyclic order.
	 */
	//@{

	//! Returns an iterator to the first element of the list.
	/**
	 * If the list is empty, a null pointer iterator is returned.
	 */
	iterator begin() { return m_head; }

	//! Returns a const iterator to the first element of the list.
	/**
	 * If the list is empty, a null pointer iterator is returned.
	 */
	const_iterator begin() const { return m_head; }

	//! Returns a const iterator to the first element of the list.
	/**
	 * If the list is empty, a null pointer iterator is returned.
	 */
	const_iterator cbegin() const { return m_head; }

	//! Returns an iterator to one-past-last element of the list.
	/**
	 * This is always a null pointer iterator.
	 */
	iterator end() { return iterator(); }

	//! Returns a const iterator to one-past-last element of the list.
	/**
	 * This is always a null pointer iterator.
	 */
	const_iterator end() const { return const_iterator(); }

	//! Returns a const iterator to one-past-last element of the list.
	/**
	 * This is always a null pointer iterator.
	 */
	const_iterator cend() const { return const_iterator(); }

	//! Returns an iterator to the last element of the list.
	/**
	 * If the list is empty, a null pointer iterator is returned.
	 */
	reverse_iterator rbegin() { return m_tail; }

	//! Returns a const iterator to the last element of the list.
	/**
	 * If the list is empty, a null pointer iterator is returned.
	 */
	const_reverse_iterator rbegin() const { return m_tail; }

	//! Returns a const iterator to the last element of the list.
	/**
	 * If the list is empty, a null pointer iterator is returned.
	 */
	const_reverse_iterator crbegin() const { return m_tail; }

	//! Returns an iterator to one-before-first element of the list.
	/**
	 * This is always a null pointer iterator.
	 */
	reverse_iterator rend() { return reverse_iterator(); }

	//! Returns a const iterator to one-before-first element of the list.
	/**
	 * This is always a null pointer iterator.
	 */
	const_reverse_iterator rend() const { return const_reverse_iterator(); }

	//! Returns a const iterator to one-before-first element of the list.
	/**
	 * This is always a null pointer iterator.
	 */
	const_reverse_iterator crend() const { return const_reverse_iterator(); }

	//! Returns a const iterator to the cyclic successor of \p it.
	/**
	 * \pre \p it points to an element in this list or to nullptr!
	 */
	const_iterator cyclicSucc(const_iterator it) const {
		OGDF_ASSERT(!it.valid() || it.listOf() == this);
		const ListElement<E> *pX = it;
		return (pX && pX->m_next) ? pX->m_next : m_head;
	}

	//! Returns an iterator to the cyclic successor of \p it.
	/**
	 * \pre \p it points to an element in this list or to nullptr!
	 */
	iterator cyclicSucc(iterator it) {
		OGDF_ASSERT(!it.valid() || it.listOf() == this);
		ListElement<E> *pX = it;
		return (pX && pX->m_next) ? pX->m_next : m_head;
	}

	/**
	 * @copydoc ogdf::List::cyclicSucc
	 */
	const_reverse_iterator cyclicSucc(const_reverse_iterator it) const {
		OGDF_ASSERT(!it.valid() || it.listOf() == this);
		const ListElement<E> *pX = it;
		return (pX && pX->m_prev) ? pX->m_prev : m_tail;
	}

	/**
	 * @copydoc ogdf::List::cyclicSucc
	 */
	reverse_iterator cyclicSucc(reverse_iterator it) {
		OGDF_ASSERT(!it.valid() || it.listOf() == this);
		ListElement<E> *pX = it;
		return (pX && pX->m_prev) ? pX->m_prev : m_tail;
	}

	//! Returns a const iterator to the cyclic predecessor of \p it.
	/**
	 * \pre \p it points to an element in this list or to nullptr!
	 */
	const_iterator cyclicPred(const_iterator it) const {
		OGDF_ASSERT(!it.valid() || it.listOf() == this);
		const ListElement<E> *pX = it;
		return (pX && pX->m_prev) ? pX->m_prev : m_tail;
	}

	//! Returns an iterator to the cyclic predecessor of \p it.
	/**
	 * \pre \p it points to an element in this list or to nullptr!
	 */
	iterator cyclicPred(iterator it) {
		OGDF_ASSERT(!it.valid() || it.listOf() == this);
		ListElement<E> *pX = it;
		return (pX && pX->m_prev) ? pX->m_prev : m_tail;
	}

	/**
	 * @copydoc ogdf::List::cyclicPred
	 */
	const_reverse_iterator cyclicPred(const_reverse_iterator it) const {
		OGDF_ASSERT(!it.valid() || it.listOf() == this);
		const ListElement<E> *pX = it;
		return (pX && pX->m_next) ? pX->m_next : m_head;
	}

	/**
	 * @copydoc ogdf::List::cyclicPred
	 */
	reverse_iterator cyclicPred(reverse_iterator it) {
		OGDF_ASSERT(!it.valid() || it.listOf() == this);
		ListElement<E> *pX = it;
		return (pX && pX->m_next) ? pX->m_next : m_head;
	}

	//@}
	/**
	 * @name Operators
	 * The following operators are provided by lists.
	 */
	//@{

	//! Assignment operator.
	ListPure<E> &operator=(const ListPure<E> &L) {
		clear(); copy(L);
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * The list \p L is empty afterwards.
	 */
	ListPure<E> &operator=(ListPure<E> &&L) {
		clear();
		m_head = L.m_head;
		m_tail = L.m_tail;
		L.m_head = L.m_tail = nullptr;
		reassignListRefs();
		return *this;
	}

	//! Equality operator.
	bool operator==(const ListPure<E> &L) const {
		ListElement<E> *pX = m_head, *pY = L.m_head;
		while(pX != nullptr && pY != nullptr) {
			if(pX->m_x != pY->m_x)
				return false;
			pX = pX->m_next;
			pY = pY->m_next;
		}
		return pX == nullptr && pY == nullptr;
	}

	//! Inequality operator.
	bool operator!=(const ListPure<E> &L) const {
		return !operator==(L);
	}

	//@}
	/**
	 * @name Adding elements
	 * These method add elements to the list.
	 */
	//@{

	//! Adds element \p x at the beginning of the list.
	iterator pushFront(const E &x) {
		ListElement<E> *pX = new ListElement<E>(this, x, m_head, nullptr);
		if (m_head)
			m_head = m_head->m_prev = pX;
		else
			m_head = m_tail = pX;
		return m_head;
	}

	//! Adds a new element at the beginning of the list.
	/**
	 * The element is constructed in-place with exactly the same arguments \p args as supplied to the function.
	 */
	template<class ... Args>
	iterator emplaceFront(Args && ... args) {
		ListElement<E> *pX = new ListElement<E>(this, m_head, nullptr, std::forward<Args>(args)...);
		if (m_head)
			m_head = m_head->m_prev = pX;
		else
			m_head = m_tail = pX;
		return m_head;
	}

	//! Adds element \p x at the end of the list.
	iterator pushBack(const E &x) {
		ListElement<E> *pX = new ListElement<E>(this, x, nullptr, m_tail);
		if (m_head)
			m_tail = m_tail->m_next = pX;
		else
			m_tail = m_head = pX;
		return m_tail;
	}

	//! Adds a new element at the end of the list.
	/**
	* The element is constructed in-place with exactly the same arguments \p args as supplied to the function.
	*/
	template<class ... Args>
	iterator emplaceBack(Args && ... args) {
		ListElement<E> *pX = new ListElement<E>(this, nullptr, m_tail, std::forward<Args>(args)...);
		if (m_head)
			m_tail = m_tail->m_next = pX;
		else
			m_tail = m_head = pX;
		return m_tail;
	}

	//! Inserts element \p x before or after \p it.
	/**
	 * @param x is the element to be inserted.
	 * @param it is a list iterator in this list.
	 * @param dir determines if \p x is inserted before or after \p it.
	 *   Possible values are ogdf::before and ogdf::after.
	 * \pre \p it points to an element in this list.
	 */
	iterator insert(const E &x, iterator it, Direction dir = Direction::after) {
		OGDF_ASSERT(it.listOf() == this);
		ListElement<E> *pY = it, *pX;
		if (dir == Direction::after) {
			ListElement<E> *pYnext = pY->m_next;
			pY->m_next = pX = new ListElement<E>(this, x, pYnext, pY);
			if (pYnext) pYnext->m_prev = pX;
			else m_tail = pX;
		} else {
			ListElement<E> *pYprev = pY->m_prev;
			pY->m_prev = pX = new ListElement<E>(this, x, pY, pYprev);
			if (pYprev) pYprev->m_next = pX;
			else m_head = pX;
		}
		return pX;
	}

	//! Inserts element \p x before \p it.
	/**
	 * \pre \p it points to an element in this list.
	 */
	iterator insertBefore(const E &x, iterator it) {
		OGDF_ASSERT(it.listOf() == this);
		ListElement<E> *pY = it, *pX;
		ListElement<E> *pYprev = pY->m_prev;
		pY->m_prev = pX = new ListElement<E>(this, x, pY, pYprev);
		if (pYprev) pYprev->m_next = pX;
		else m_head = pX;
		return pX;
	}

	//! Inserts element \p x after \p it.
	/**
	 * \pre \p it points to an element in this list.
	 */
	iterator insertAfter(const E &x, iterator it) {
		OGDF_ASSERT(it.listOf() == this);
		ListElement<E> *pY = it, *pX;
		ListElement<E> *pYnext = pY->m_next;
		pY->m_next = pX = new ListElement<E>(this, x, pYnext, pY);
		if (pYnext) pYnext->m_prev = pX;
		else m_tail = pX;
		return pX;
	}

	//@}
	/**
	 * @name Removing elements
	 * These method remove elements from the list.
	 */
	//@{

	//! Removes the first element from the list.
	/**
	 * \pre The list is not empty!
	 */
	void popFront() {
		OGDF_ASSERT(m_head != nullptr);
		ListElement<E> *pX = m_head;
		m_head = m_head->m_next;
		delete pX;
		if (m_head) m_head->m_prev = nullptr;
		else m_tail = nullptr;
	}

	//! Removes the first element from the list and returns it.
	/**
	 * \pre The list is not empty!
	 */
	E popFrontRet() {
		E el = front();
		popFront();
		return el;
	}

	//! Removes the last element from the list.
	/**
	 * \pre The list is not empty!
	 */
	void popBack() {
		OGDF_ASSERT(m_tail != nullptr);
		ListElement<E> *pX = m_tail;
		m_tail = m_tail->m_prev;
		delete pX;
		if (m_tail) m_tail->m_next = nullptr;
		else m_head = nullptr;
	}

	//! Removes the last element from the list and returns it.
	/**
	 * \pre The list is not empty!
	 */
	E popBackRet() {
		E el = back();
		popBack();
		return el;
	}

	//! Removes \p it from the list.
	/**
	 * \pre \p it points to an element in this list.
	 */
	void del(iterator it) {
		OGDF_ASSERT(it.listOf() == this);
		ListElement<E> *pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
		delete pX;
		if (pPrev) pPrev->m_next = pNext;
		else m_head = pNext;
		if (pNext) pNext->m_prev = pPrev;
		else m_tail = pPrev;
	}

	//! Removes the first occurrence of \p x (if any) from the list.
	/**
	 * If the list contains \p x several times, only the first element
	 * containing \p x is removed.
	 *
	 * \return true if one element has been removed, false otherwise.
	 */
	bool removeFirst(const E &x) {
		for(ListElement<E> *pX = m_head; pX != nullptr; pX = pX->m_next)
			if(pX->m_x == x) {
				del(pX); return true;
			}
		return false;
	}

	//! Removes all elements from the list.
	void clear() {
		if (m_head == nullptr) return;

		if (!std::is_trivially_destructible<E>::value) {
			for(ListElement<E> *pX = m_head; pX != nullptr; pX = pX->m_next)
				pX->m_x.~E();
		}
		OGDF_ALLOCATOR::deallocateList(sizeof(ListElement<E>),m_head,m_tail);

		m_head = m_tail = nullptr;
	}

	//@}
	/**
	 * @name Moving elements
	 * The method allow to change the order of elements within the list, or to move elements to another list.
	 */
	//@{

	//! Exchanges the positions of \p it1 and \p it2 in the list.
	/**
	 * \pre \p it1 and \p it2 point to elements in this list.
	 */
	void exchange(iterator it1, iterator it2) {
		OGDF_ASSERT(it1.valid());
		OGDF_ASSERT(it2.valid());
		OGDF_ASSERT(it1 != it2);
		ListElement<E> *pX = it1, *pY = it2;

		std::swap(pX->m_next,pY->m_next);
		std::swap(pX->m_prev,pY->m_prev);
#ifdef OGDF_DEBUG
		std::swap(pX->m_list,pY->m_list);
#endif

		if(pX->m_next == pX) {
			pX->m_next = pY; pY->m_prev = pX;
		}
		if(pX->m_prev == pX) {
			pX->m_prev = pY; pY->m_next = pX;
		}

		if(pX->m_prev) pX->m_prev->m_next = pX;
		else m_head = pX;

		if(pY->m_prev) pY->m_prev->m_next = pY;
		else m_head = pY;

		if(pX->m_next) pX->m_next->m_prev = pX;
		else m_tail = pX;

		if(pY->m_next) pY->m_next->m_prev = pY;
		else m_tail = pY;
	}

	//! Moves \p it to the begin of the list.
	/**
	 * \pre \p it points to an element in this list.
	 */
	void moveToFront(iterator it) {
		OGDF_ASSERT(it.listOf() == this);
		// remove it
		ListElement<E> *pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
		//already at front
		if (!pPrev) return;

		//update old position
		if (pPrev) pPrev->m_next = pNext;
		if (pNext) pNext->m_prev = pPrev;
		else m_tail = pPrev;
		// insert it at front
		pX->m_prev = nullptr;
		pX->m_next = m_head;
		m_head = m_head->m_prev = pX;
	}

	//! Moves \p it to the end of the list.
	/**
	 * \pre \p it points to an element in this list.
	 */
	void moveToBack(iterator it) {
		OGDF_ASSERT(it.listOf() == this);
		// remove it
		ListElement<E> *pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
		//already at back
		if (!pNext) return;

		//update old position
		if (pPrev) pPrev->m_next = pNext;
		else m_head = pNext;
		if (pNext) pNext->m_prev = pPrev;
		// insert it at back
		pX->m_prev = m_tail;
		pX->m_next = nullptr;
		m_tail = m_tail->m_next = pX;
	}

	//! Moves \p it after \p itBefore.
	/**
	 * \pre \p it and \p itBefore point to elements in this list.
	 */
	void moveToSucc(iterator it, iterator itBefore) {
		OGDF_ASSERT(it.listOf() == this);
		OGDF_ASSERT(itBefore.listOf() == this);
		// move it
		ListElement<E> *pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
		//the same of already in place
		ListElement<E> *pY = itBefore;
		if(pX == pY || pPrev == pY) return;

		// update old position
		if (pPrev) pPrev->m_next = pNext;
		else m_head = pNext;
		if (pNext) pNext->m_prev = pPrev;
		else m_tail = pPrev;
		// move it after itBefore
		ListElement<E> *pYnext = pX->m_next = pY->m_next;
		(pX->m_prev = pY)->m_next = pX;
		if (pYnext) pYnext->m_prev = pX;
		else m_tail = pX;
	}

	//! Moves \p it before \p itAfter.
	/**
	 * \pre \p it and \p itAfter point to elements in this list.
	 */
	void moveToPrec(iterator it, iterator itAfter) {
		OGDF_ASSERT(it.listOf() == this);
		OGDF_ASSERT(itAfter.listOf() == this);
		// move it
		ListElement<E> *pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
		//the same of already in place
		ListElement<E> *pY = itAfter;
		if(pX == pY || pNext == pY) return;

		// update old position
		if (pPrev) pPrev->m_next = pNext;
		else m_head = pNext;
		if (pNext) pNext->m_prev = pPrev;
		else m_tail = pPrev;
		// move it before itAfter
		ListElement<E> *pYprev = pX->m_prev = pY->m_prev;
		(pX->m_next = pY)->m_prev = pX;
		if (pYprev) pYprev->m_next = pX;
		else m_head = pX;
	}

	//! Moves \p it to the begin of \p L2.
	/**
	 * \pre \p it points to an element in this list.
	 */
	void moveToFront(iterator it, ListPure<E> &L2) {
		OGDF_ASSERT(it.listOf() == this);
		OGDF_ASSERT(this != &L2);
		// remove it
		ListElement<E> *pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
		if (pPrev) pPrev->m_next = pNext;
		else m_head = pNext;
		if (pNext) pNext->m_prev = pPrev;
		else m_tail = pPrev;
		// insert it at front of L2
		pX->m_prev = nullptr;
		if ((pX->m_next = L2.m_head) != nullptr)
			L2.m_head = L2.m_head->m_prev = pX;
		else
			L2.m_head = L2.m_tail = pX;

#ifdef OGDF_DEBUG
		pX->m_list = &L2;
#endif
	}

	//! Moves \p it to the end of \p L2.
	/**
	 * \pre \p it points to an element in this list.
	 */
	void moveToBack(iterator it, ListPure<E> &L2) {
		OGDF_ASSERT(it.listOf() == this);
		OGDF_ASSERT(this != &L2);
		// remove it
		ListElement<E> *pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
		if (pPrev) pPrev->m_next = pNext;
		else m_head = pNext;
		if (pNext) pNext->m_prev = pPrev;
		else m_tail = pPrev;
		// insert it at back of L2
		pX->m_next = nullptr;
		if ((pX->m_prev = L2.m_tail) != nullptr)
			L2.m_tail = L2.m_tail->m_next = pX;
		else
			L2.m_head = L2.m_tail = pX;

#ifdef OGDF_DEBUG
		pX->m_list = this;
#endif
	}

	//! Moves \p it to list \p L2 and inserts it after \p itBefore.
	/**
	 * \pre \p it points to an element in this list, and \p itBefore
	 *      points to an element in \p L2.
	 */
	void moveToSucc(iterator it, ListPure<E> &L2, iterator itBefore) {
		OGDF_ASSERT(it.listOf() == this);
		OGDF_ASSERT(itBefore.listOf() == &L2);
		OGDF_ASSERT(this != &L2);
		// remove it
		ListElement<E> *pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
		if (pPrev) pPrev->m_next = pNext;
		else m_head = pNext;
		if (pNext) pNext->m_prev = pPrev;
		else m_tail = pPrev;
		// insert it in list L2 after itBefore
		ListElement<E> *pY = itBefore;
		ListElement<E> *pYnext = pX->m_next = pY->m_next;
		(pX->m_prev = pY)->m_next = pX;
		if (pYnext) pYnext->m_prev = pX;
		else L2.m_tail = pX;

#ifdef OGDF_DEBUG
		pX->m_list = &L2;
#endif
	}

	//! Moves \p it to list \p L2 and inserts it before \p itAfter.
	/**
	 * \pre \p it points to an element in this list, and \p itAfter
	 *      points to an element in \p L2.
	 */
	void moveToPrec(iterator it, ListPure<E> &L2, iterator itAfter) {
		OGDF_ASSERT(it.listOf() == this);
		OGDF_ASSERT(itAfter.listOf() == &L2);
		OGDF_ASSERT(this != &L2);
		// remove it
		ListElement<E> *pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
		if (pPrev) pPrev->m_next = pNext;
		else m_head = pNext;
		if (pNext) pNext->m_prev = pPrev;
		else m_tail = pPrev;
		// insert it in list L2 after itBefore
		ListElement<E> *pY = itAfter;
		ListElement<E> *pYprev = pX->m_prev = pY->m_prev;
		(pX->m_next = pY)->m_prev = pX;
		if (pYprev) pYprev->m_next = pX;
		else L2.m_head = pX;

#ifdef OGDF_DEBUG
		pX->m_list = &L2;
#endif
	}

	//! Appends \p L2 to this list and makes \p L2 empty.
	void conc(ListPure<E> &L2) {
		OGDF_ASSERT(this != &L2);
		if (m_head)
			m_tail->m_next = L2.m_head;
		else
			m_head = L2.m_head;
		if (L2.m_head) {
			L2.m_head->m_prev = m_tail;
			m_tail = L2.m_tail;
		}

		reassignListRefs(L2.m_head);

		L2.m_head = L2.m_tail = nullptr;
	}

	//! Prepends \p L2 to this list and makes \p L2 empty.
	void concFront(ListPure<E> &L2) {
		OGDF_ASSERT(this != &L2);
		if (m_head)
			m_head->m_prev = L2.m_tail;
		else
			m_tail = L2.m_tail;
		if (L2.m_head) {
			L2.m_tail->m_next = m_head;
			m_head = L2.m_head;
		}

		reassignListRefs(L2.m_head);

		L2.m_head = L2.m_tail = nullptr;
	}

	//! Exchanges the contents of this list and \p other in constant time.
	void swap(ListPure<E> &other) {
		std::swap(m_head, other.m_head);
		std::swap(m_tail, other.m_tail);

		reassignListRefs();
		other.reassignListRefs();
	}

	//! Splits the list at element \p it into lists \p L1 and \p L2.
	/**
	 * If \p it is not a null pointer and \a L = x1,...,x{k-1}, \p it,x_{k+1},xn, then
	 * \p L1 = x1,...,x{k-1} and \p L2 = \p it,x{k+1},...,xn if \p dir = Direction::before.
	 * If \p it is a null pointer, then \p L1 is made empty and \p L2 = \a L. Finally
	 * \a L is made empty if it is not identical to \p L1 or \p L2.
	 *
	 * \pre \p it points to an element in this list.
	 */

	void split(iterator it,ListPure<E> &L1,ListPure<E> &L2,Direction dir = Direction::before) {
		OGDF_ASSERT(!it.valid() || it.listOf() == this);
		if (&L1 != this) L1.clear();
		if (&L2 != this) L2.clear();

		if (it.valid()){
			L1.m_head = m_head;
			L2.m_tail = m_tail;
			if (dir == Direction::before){
				L2.m_head = it;
				L1.m_tail = L2.m_head->m_prev;
			}
			else {
				L1.m_tail = it;
				L2.m_head = L1.m_tail->m_next;
			}
			L2.m_head->m_prev = L1.m_tail->m_next = nullptr;

		} else {
			L1.m_head = L1.m_tail = nullptr;
			L2.m_head = m_head;
			L2.m_tail = m_tail;
		}

		if (this != &L1 && this != &L2) {
			m_head = m_tail = nullptr;
		}

		L1.reassignListRefs();
		L2.reassignListRefs();
	}

	//! Splits the list after \p it.
	void splitAfter(iterator it, ListPure<E> &L2) {
		OGDF_ASSERT(it.listOf() == this);
		OGDF_ASSERT(this != &L2);
		L2.clear();
		ListElement<E> *pX = it;
		if (pX != m_tail) {
			(L2.m_head = pX->m_next)->m_prev = nullptr;
			pX->m_next = nullptr;
			L2.m_tail = m_tail;
			m_tail = pX;
		}

		L2.reassignListRefs();
	}

	//! Splits the list before \p it.
	void splitBefore(iterator it, ListPure<E> &L2) {
		OGDF_ASSERT(it.listOf() == this);
		OGDF_ASSERT(this != &L2);
		L2.clear();
		ListElement<E> *pX = it;
		L2.m_head = pX; L2.m_tail = m_tail;
		if ((m_tail = pX->m_prev) == nullptr)
			m_head = nullptr;
		else
			m_tail->m_next = nullptr;
		pX->m_prev = nullptr;

		L2.reassignListRefs();
	}

	//! Reverses the order of the list elements.
	void reverse() {
		ListElement<E> *pX = m_head;
		m_head = m_tail;
		m_tail = pX;
		while(pX) {
			ListElement<E> *pY = pX->m_next;
			pX->m_next = pX->m_prev;
			pX = pX->m_prev = pY;
		}
	}

	//@}
	/**
	 * @name Searching and sorting
	 * These methods provide searching for values and sorting the list.
	 */
	//@{

	//! Scans the list for the specified element and returns an iterator to the first occurrence in the list, or an invalid iterator if not found.
	ListConstIterator<E> search(const E& e) const {
		ListConstIterator<E> i;
		for (i = begin(); i.valid(); ++i)
			if (*i == e) return i;
		return i;
	}

	//! Scans the list for the specified element and returns an iterator to the first occurrence in the list, or an invalid iterator if not found.
	ListIterator<E> search(const E& e) {
		ListIterator<E> i;
		for (i = begin(); i.valid(); ++i)
			if (*i == e) return i;
		return i;
	}

	//! Scans the list for the specified element (using the user-defined comparer) and returns an iterator to the first occurrence in the list, or an invalid iterator if not found.
	template<class COMPARER>
	ListConstIterator<E> search(const E &e, const COMPARER &comp) const {
		ListConstIterator<E> i;
		for (i = begin(); i.valid(); ++i)
			if (comp.equal(*i, e)) return i;
		return i;
	}

	//! Scans the list for the specified element (using the user-defined comparer) and returns an iterator to the first occurrence in the list, or an invalid iterator if not found.
	template<class COMPARER>
	ListIterator<E> search(const E &e, const COMPARER &comp) {
		ListIterator<E> i;
		for (i = begin(); i.valid(); ++i)
			if (comp.equal(*i, e)) return i;
		return i;
	}

	//! Sorts the list using Quicksort.
	void quicksort() {
		ogdf::quicksortTemplate(*this);
	}

	//! Sorts the list using Quicksort and comparer \p comp.
	template<class COMPARER>
	void quicksort(const COMPARER &comp) {
		ogdf::quicksortTemplate(*this,comp);
	}

	//! Sorts the list using bucket sort.
	/**
	 * @param l is the lowest bucket that will occur.
	 * @param h is the highest bucket that will occur.
	 * @param f returns the bucket for each element.
	 * \pre The bucket function \p f will only return bucket values between \p l
	 * and \p h for this list.
	 */
	void bucketSort(int l, int h, BucketFunc<E> &f);

	//@}
	/**
	 * @name Random elements and permutations
	 * These methods allow to select a random element in the list, or to randomly permute the list.
	 */
	//@{

	/**
	 * Returns an iterator to a random element.
	 *
	 * Takes linear time.
	 * An invalid iterator is returned iff no feasible element exists.
	 *
	 * @see chooseIteratorFrom
	 */
	const_iterator chooseIterator(
			std::function<bool(const E&)> includeElement = [](const E&) { return true; },
			bool isFastTest = true) const {
		return chooseIteratorFrom(*this, includeElement, isFastTest);
	}

	//! @copydoc #chooseIterator
	iterator chooseIterator(
			std::function<bool(const E&)> includeElement = [](const E&) { return true; },
			bool isFastTest = true) {
		return chooseIteratorFrom(*this, includeElement, isFastTest);
	}

	/**
	 * Returns a random element.
	 *
	 * Takes linear time.
	 * \pre Requires at least one feasible element to exist.
	 *
	 * @see chooseIteratorFrom
	 */
	const_reference chooseElement(
			std::function<bool(const E&)> includeElement = [](const E&) { return true; },
			bool isFastTest = true) const {
		const_iterator result = chooseIterator(includeElement, isFastTest);
		OGDF_ASSERT(result.valid());
		return *result;
	}

	//! @copydoc #chooseElement
	reference chooseElement(
			std::function<bool(const E&)> includeElement = [](const E&) { return true; },
			bool isFastTest = true) {
		iterator result = chooseIterator(includeElement, isFastTest);
		OGDF_ASSERT(result.valid());
		return *result;
	}

	//! Randomly permutes the elements in the list.
	void permute() {
		std::minstd_rand rng(randomSeed());
		permute(size(), rng);
	}

	//! Randomly permutes the elements in the list using random number generator \p rng.
	template<class RNG>
	void permute(RNG &rng) {
		permute(size(), rng);
	}

	//@}

protected:
	void copy(const ListPure<E> &L) {
		for(ListElement<E> *pX = L.m_head; pX != nullptr; pX = pX->m_next)
			pushBack(pX->m_x);
	}

	template<class RNG>

	//! permutes elements in list randomly; n is the length of the list
	void permute(const int n, RNG &rng);

	//! Sets the debug reference of all list elements starting at \p start to \c this.
	inline void reassignListRefs(ListElement<E> *start = nullptr) {
#ifdef OGDF_DEBUG
		for(auto e = start == nullptr ? m_head : start; e != nullptr; e = e->m_next) {
			e->m_list = this;
		}
#endif
	}

	OGDF_NEW_DELETE
};

//! Doubly linked lists (maintaining the length of the list).
/**
 * @ingroup containers
 *
 * Use ogdf::ListConstIterator or ogdf::ListIterator in order to iterate over the list.
 *
 * In contrast to ogdf::ListPure, instances of ogdf::List store the length of the list.
 *
 * @tparam E is the data type stored in list elements.
 */
template<class E>
class List : private ListPure<E> {
	int m_count; //!< The length of the list.

public:
	using typename ListPure<E>::value_type;
	using typename ListPure<E>::reference;
	using typename ListPure<E>::const_reference;
	using typename ListPure<E>::const_iterator;
	using typename ListPure<E>::iterator;
	using typename ListPure<E>::const_reverse_iterator;
	using typename ListPure<E>::reverse_iterator;

	//! Constructs an empty doubly linked list.
	List() : m_count(0) { }

	//! Constructs a doubly linked list containing the elements in \p init.
	List(std::initializer_list<E> init) : ListPure<E>(init), m_count((int)init.size()) { }

	//! Constructs a doubly linked list that is a copy of \p L.
	List(const List<E> &L) : ListPure<E>(L), m_count(L.m_count) { }

	//! Constructs a doubly linked list containing the elements of \p L (move semantics).
	/**
	 * The list \p L is empty afterwards.
	 */
	List(List<E> &&L) : ListPure<E>(std::move(L)), m_count(L.m_count) {
		L.m_count = 0;
	}

	/**
	 * @name Access methods
	 * These methods provide simple access without changing the list.
	 */
	//@{

	//! Returns the number of elements in the list.
	/**
	 * This method has constant runtime (in contrast to ListPure::size()), since the list maintains the current size.
	 */
	int size() const { return m_count; }

	//! Conversion to const ListPure.
	const ListPure<E> &getListPure() const { return *this; }

	//@}
	/**
	 * @name Operators
	 * The following operators are provided by lists.
	 */
	//@{

	//! Assignment operator.
	List<E> &operator=(const List<E> &L) {
		ListPure<E>::operator=(L);
		m_count = L.m_count;
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * The list \p L is empty afterwards.
	 */
	List<E> &operator=(List<E> &&L) {
		m_count = L.m_count;
		ListPure<E>::operator=(std::move(L));
		L.m_count = 0;
		return *this;
	}

	//! @copydoc ogdf::ListPure::operator==
	bool operator==(const List<E> &L) const {
		return (m_count == L.m_count) && ListPure<E>::operator==(L);
	}

	//! @copydoc ogdf::ListPure::operator!=
	bool operator!=(const List<E> &L) const {
		return !operator==(L);
	}

	//@}
	/**
	 * @name Adding elements
	 * These method add elements to the list.
	 */
	//@{

	//! @copydoc ogdf::ListPure::pushFront
	iterator pushFront(const E &x) {
		++m_count;
		return ListPure<E>::pushFront(x);
	}

	//! @copydoc ogdf::ListPure::emplaceFront
	template<class ... Args>
	iterator emplaceFront(Args && ... args) {
		++m_count;
		return ListPure<E>::emplaceFront(std::forward<Args>(args)...);
	}

	//! @copydoc ogdf::ListPure::pushBack
	iterator pushBack(const E &x) {
		++m_count;
		return ListPure<E>::pushBack(x);
	}

	//! @copydoc ogdf::ListPure::emplaceBack
	template<class ... Args>
	iterator emplaceBack(Args && ... args) {
		++m_count;
		return ListPure<E>::emplaceBack(std::forward<Args>(args)...);
	}

	//! @copydoc ogdf::ListPure::insert
	iterator insert(const E &x, iterator it, Direction dir = Direction::after) {
		++m_count;
		return ListPure<E>::insert(x,it,dir);
	}

	//! @copydoc ogdf::ListPure::insertBefore
	iterator insertBefore(const E &x, iterator it) {
		++m_count;
		return ListPure<E>::insertBefore(x,it);
	}

	//! @copydoc ogdf::ListPure::insertAfter
	iterator insertAfter(const E &x, iterator it) {
		++m_count;
		return ListPure<E>::insertAfter(x,it);
	}

	//@}
	/**
	 * @name Removing elements
	 * These method remove elements from the list.
	 */
	//@{

	//! @copydoc ogdf::ListPure::popFront
	void popFront() {
		--m_count;
		ListPure<E>::popFront();
	}

	//! @copydoc ogdf::ListPure::popFrontRet
	E popFrontRet() {
		E el = front();
		popFront();
		return el;
	}

	//! @copydoc ogdf::ListPure::popBack
	void popBack() {
		--m_count;
		ListPure<E>::popBack();
	}

	//! @copydoc ogdf::ListPure::popBackRet
	E popBackRet() {
		E el = back();
		popBack();
		return el;
	}

	//! @copydoc ogdf::ListPure::del
	void del(iterator it) {
		--m_count;
		ListPure<E>::del(it);
	}

	//! @copydoc ogdf::ListPure::removeFirst
	bool removeFirst(const E &x) {
		bool hasRemoved = ListPure<E>::removeFirst(x);
		if(hasRemoved)
			--m_count;
		return hasRemoved;
	}

	//! @copydoc ogdf::ListPure::clear
	void clear() {
		m_count = 0;
		ListPure<E>::clear();
	}

	//@}
	/**
	 * @name Moving elements
	 * The method allow to change the order of elements within the list, or to move elements to another list.
	 */
	//@{

	//! @copydoc ogdf::ListPure::moveToFront
	void moveToFront(iterator it, List<E> &L2) {
		ListPure<E>::moveToFront(it,L2);
		--m_count; ++L2.m_count;
	}

	//! @copydoc ogdf::ListPure::moveToBack
	void moveToBack(iterator it, List<E> &L2) {
		ListPure<E>::moveToBack(it,L2);
		--m_count; ++L2.m_count;
	}

	//! @copydoc ogdf::ListPure::moveToSucc
	void moveToSucc(iterator it, List<E> &L2, iterator itBefore) {
		ListPure<E>::moveToSucc(it,L2,itBefore);
		--m_count; ++L2.m_count;
	}

	//! @copydoc ogdf::ListPure::moveToPrec
	void moveToPrec(iterator it, List<E> &L2, iterator itAfter) {
		ListPure<E>::moveToPrec(it,L2,itAfter);
		--m_count; ++L2.m_count;
	}

	//! @copydoc ogdf::ListPure::conc
	void conc(List<E> &L2) {
		ListPure<E>::conc(L2);
		m_count += L2.m_count;
		L2.m_count = 0;
	}

	//! @copydoc ogdf::ListPure::concFront
	void concFront(List<E> &L2) {
		ListPure<E>::concFront(L2);
		m_count += L2.m_count;
		L2.m_count = 0;
	}

	//! @copydoc ogdf::ListPure::swap
	void swap(List<E> &other) {
		ListPure<E>::swap(other);
		std::swap(m_count, other.m_count);
	}

	//! @copydoc ogdf::ListPure::split
	void split(iterator it, List<E> &L1, List<E> &L2, Direction dir = Direction::before) {
		ListPure<E>::split(it,L1,L2,dir);
		int countL = m_count, countL1 = 0;
		for(ListElement<E> *pX = L1.m_head; pX != nullptr; pX = pX->m_next)
			++countL1;

		L1.m_count = countL1;
		L2.m_count = countL - countL1;
		if (this->m_head == nullptr) m_count = 0;
	}

	using ListPure<E>::empty;
	using ListPure<E>::front;
	using ListPure<E>::back;
	using ListPure<E>::get;
	using ListPure<E>::pos;
	using ListPure<E>::begin;
	using ListPure<E>::cbegin;
	using ListPure<E>::end;
	using ListPure<E>::cend;
	using ListPure<E>::rbegin;
	using ListPure<E>::crbegin;
	using ListPure<E>::rend;
	using ListPure<E>::crend;
	using ListPure<E>::cyclicSucc;
	using ListPure<E>::cyclicPred;
	using ListPure<E>::exchange;
	using ListPure<E>::moveToFront;
	using ListPure<E>::moveToBack;
	using ListPure<E>::moveToSucc;
	using ListPure<E>::moveToPrec;
	using ListPure<E>::reverse;
	using ListPure<E>::search;
	using ListPure<E>::quicksort;
	using ListPure<E>::bucketSort;
	using ListPure<E>::chooseIterator;
	using ListPure<E>::chooseElement;
	using ListPure<E>::permute;

	OGDF_NEW_DELETE
};

template<class E>
void ListPure<E>::bucketSort(int l, int h, BucketFunc<E> &f)
{
	if (m_head == m_tail) return;

	Array<ListElement<E> *> head(l,h,nullptr), tail(l,h);

	ListElement<E> *pX;
	for (pX = m_head; pX; pX = pX->m_next) {
		int i = f.getBucket(pX->m_x);
		if (head[i])
			tail[i] = ((pX->m_prev = tail[i])->m_next = pX);
		else
			head[i] = tail[i] = pX;
	}

	ListElement<E> *pY = nullptr;
	for (int i = l; i <= h; i++) {
		pX = head[i];
		if (pX) {
			if (pY) {
				(pY->m_next = pX)->m_prev = pY;
			} else
				(m_head = pX)->m_prev = nullptr;
			pY = tail[i];
		}
	}

	m_tail = pY;
	pY->m_next = nullptr;
}

template<class E>
template<class RNG>
void ListPure<E>::permute(const int n, RNG &rng)
{
	//if n==0 do nothing
	if (n == 0) { return; }

	Array<ListElement<E> *> A(n+2);
	A[0] = A[n+1] = nullptr;

	int i = 1;
	ListElement<E> *pX;
	for (pX = m_head; pX; pX = pX->m_next)
		A[i++] = pX;

	A.permute(1,n,rng);

	for (i = 1; i <= n; i++) {
		pX = A[i];
		pX->m_next = A[i+1];
		pX->m_prev = A[i-1];
	}

	m_head = A[1];
	m_tail = A[n];
}

//! Prints list \p L to output stream \p os using delimiter \p delim.
template<class E>
void print(std::ostream &os, const ListPure<E> &L, char delim = ' ')
{
	typename ListPure<E>::const_iterator pX = L.begin();
	if (pX.valid()) {
		os << *pX;
		for(++pX; pX.valid(); ++pX)
			os << delim << *pX;
	}
}

//! Prints list \p L to output stream \p os using delimiter \p delim.
template<class E>
void print(std::ostream &os, const List<E> &L, char delim = ' ')
{
	print(os, L.getListPure(), delim);
}

//! Prints list \p L to output stream \p os.
template<class E>
std::ostream &operator<<(std::ostream &os, const ListPure<E> &L)
{
	print(os,L);
	return os;
}

//! Prints list \p L to output stream \p os.
template<class E>
std::ostream &operator<<(std::ostream &os, const List<E> &L)
{
	return os << L.getListPure();
}

template<class E, class Master>
class ListContainer : public List<E> {
	friend Master;

public:
	//! Provides a bidirectional iterator to an object in the container.
	using iterator = typename List<E>::const_iterator;
	//! Provides a bidirectional reverse iterator to an object in the container.
	using reverse_iterator = typename List<E>::const_reverse_iterator;

	//! Returns an iterator to the first element in the container.
	iterator begin() const { return List<E>::cbegin(); }

	//! Returns an iterator to the one-past-last element in the container.
	iterator end() const { return List<E>::cend(); }

	//! Returns a reverse iterator to the last element in the container.
	reverse_iterator rbegin() const { return List<E>::crbegin(); }

	//! Returns a reverse iterator to the one-before-first element in the container.
	reverse_iterator rend() const { return List<E>::crend(); }

	//! Returns the number of elements in the container.
	int size() const { return List<E>::size(); }
};

}
