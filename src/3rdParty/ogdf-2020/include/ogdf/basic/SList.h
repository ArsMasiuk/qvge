/** \file
 * \brief Declaration of singly linked lists and iterators.
 *
 * \author Carsten Gutwenger, Tilo Wiedera
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

namespace ogdf {

template<class E> class SListPure;
template<class E, bool isConst> class SListIteratorBase;
template<class E> using SListConstIterator = SListIteratorBase<E, true>;
template<class E> using SListIterator = SListIteratorBase<E, false>;

//! Structure for elements of singly linked lists.
template<class E>
class SListElement {
	friend class SListPure<E>;
	friend class SListIteratorBase<E, true>;
	friend class SListIteratorBase<E, false>;

	SListElement<E> *m_next; //!< Pointer to successor element.
	E m_x; //!< Stores the content.
#ifdef OGDF_DEBUG
	SListPure<E> *m_list; //!< List object that the element belongs to.
#endif

	//! Constructs an SListElement.
	SListElement(SListPure<E> *list, const E &x, SListElement<E> *next)
			: m_next(next), m_x(x) {
#ifdef OGDF_DEBUG
		m_list = list;
#endif
	}
	//! Constructs an SListElement.
	SListElement(SListPure<E> *list, const E &x) : SListElement(list, x, nullptr) { }
	//! Constructs an SListElement.
	SListElement() : SListElement(nullptr, E()) { }
	//! Constructs an SListElement with given arguments \p args for constructor of element type.
	template<class ... Args>
	SListElement(SListPure<E> *list, SListElement<E> *next, Args && ... args)
			: SListElement(list, E(std::forward<Args>(args)...), next) { }

	OGDF_NEW_DELETE
};

//! Encapsulates a pointer to an ogdf::SList element.
/**
 * It is used in order to iterate over singly linked lists,
 * and to specify a position in a singly linked list. It is possible that
 * an iterator encapsulates a null pointer.
 *
 * @tparam E The type of element.
 * @tparam isConst True iff this iterator allows only const-access to the element.
 */

template<class E, bool isConst> class SListIteratorBase {
	friend class SListIteratorBase<E, !isConst>;
	friend class SListPure<E>;

	//! The underlying list element, depending on isConst
	using ListElem = typename std::conditional<isConst, const SListElement<E>, SListElement<E>>::type;
	//! The underlying type, depending on isConst
	using Elem = typename std::conditional<isConst, const E, E>::type;

	ListElem *m_pX; //!< Pointer to slist element.

	//! Conversion to pointer to slist element.
	operator ListElem *() { return m_pX; }

public:
	//! Constructs an iterator that points to \p pX.
	SListIteratorBase(ListElem *pX) : m_pX(pX) { }

	//! Constructs an invalid iterator.
	SListIteratorBase() : SListIteratorBase(nullptr) { }

	//! Constructs an iterator that is a copy of \p it.
	template<bool isArgConst, typename std::enable_if<isConst || !isArgConst, int>::type = 0>
	SListIteratorBase(const SListIteratorBase<E, isArgConst> &it) : SListIteratorBase(it.m_pX) { }

	//! Copy constructor.
	// gcc9 complains since it cannot match the templated constructor above (-Werror=deprecated-copy).
	SListIteratorBase(const SListIterator<E> &it) : SListIteratorBase(it.m_pX) { }

	//! Returns true iff the iterator points to an element.
	bool valid() const { return m_pX != nullptr; }

#ifdef OGDF_DEBUG
	//! Returns the list that this iterator belongs to.
	//! \pre This iterator is valid.
	SListPure<E> *listOf() {
		OGDF_ASSERT(valid());
		return m_pX->m_list;
	}
#endif

	//! Equality operator.
	bool operator==(const SListIteratorBase<E, isConst> &it) const {
		return m_pX == it.m_pX;
	}

	//! Inequality operator.
	bool operator!=(const SListIteratorBase<E, isConst> &it) const {
		return m_pX != it.m_pX;
	}

	//! Returns successor iterator.
	SListIteratorBase<E, isConst> succ() const { return m_pX->m_next; }

	//! Returns a reference to the element content.
	Elem &operator*() const { return m_pX->m_x; }

	//! Assignment operator.
	SListIteratorBase<E, isConst> &operator=(const SListIterator<E> &it) {
		m_pX = it.m_pX;
		return *this;
	}

	//! Increment operator (prefix).
	SListIteratorBase<E, isConst> &operator++() {
		m_pX = m_pX->m_next;
		return *this;
	}

	//! Increment operator (postfix).
	SListIteratorBase<E, isConst> operator++(int) {
		SListIteratorBase<E, isConst> it = *this;
		m_pX = m_pX->m_next;
		return it;
	}

	OGDF_NEW_DELETE
};

//! Singly linked lists.
/**
 * @ingroup containers
 *
 * Use ogdf::SListConstIterator or ogdf::SListIterator in order to iterate over the list.
 *
 * In contrast to ogdf::SList, instances of ogdf::SListPure do not store the length of the list.
 *
 * @tparam E is the data type stored in list elements.
 */

template<class E> class SListPure {
	SListElement<E> *m_head; //!< Pointer to first element.
	SListElement<E> *m_tail; //!< Pointer to last element.

public:
	//! Represents the data type stored in a list element.
	using value_type = E;
	//! Provides a reference to an element stored in a list.
	using reference = E&;
	//! Provides a reference to a const element stored in a list for reading and performing const operations.
	using const_reference = const E&;
	//! Provides a forward iterator that can read a const element in a list.
	using const_iterator = SListConstIterator<E>;
	//! Provides a forward iterator that can read or modify any element in a list.
	using iterator = SListIterator<E>;

	//! Constructs an empty singly linked list.
	SListPure() : m_head(nullptr), m_tail(nullptr) { }

	//! Constructs a singly linked list containing the elements in \p init.
	SListPure(std::initializer_list<E> init) : m_head(nullptr), m_tail(nullptr) {
		for (const E &x : init)
			pushBack(x);
	}
	//! Constructs a singly linked list that is a copy of \p L.
	SListPure(const SListPure<E> &L) : m_head(nullptr), m_tail(nullptr) {
		copy(L);
	}

	//! Constructs a singly linked list containing the elements of \p L (move semantics).
	/**
	 * The list \p L is empty afterwards.
	 */
	SListPure(SListPure<E> &&L) : m_head(L.m_head), m_tail(L.m_tail) {
		L.m_head = L.m_tail = nullptr;
	}

	//! Destructor.
	virtual ~SListPure() { clear(); }

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
	 * If you require frequent access to the size of the list, consider using SList instead of SListPure.
	 */
	virtual int size() const {
		int count = 0;
		for (SListElement<E> *pX = m_head; pX != nullptr; pX = pX->m_next)
			++count;
		return count;
	}

	//! Returns a reference to the first element.
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

	//! Returns a reference to the last element.
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

	//! Returns an iterator pointing to the element at position \p pos.
	/**
	 * The running time of this method is linear in \p pos.
	 */
	const_iterator get(int pos) const {
		SListElement<E> *pX;
		for(pX = m_head; pX != nullptr; pX = pX->m_next)
			if (pos-- == 0) break;
		return pX;
	}

	//! Returns an iterator pointing to the element at position \p pos.
	/**
	 * The running time of this method is linear in \p pos.
	 */
	iterator get(int pos) {
		SListElement<E> *pX;
		for(pX = m_head; pX != nullptr; pX = pX->m_next)
			if (pos-- == 0) break;
		return pX;
	}

	//! Returns the position (starting with 0) of \p it in the list.
	/**
	 * Positions are numbered 0,1,...
	 * \pre \p it is an iterator pointing to an element in this list.
	 */
	int pos(const_iterator it) const {
		OGDF_ASSERT(it.listOf() == this);
		int p = 0;
		for(SListElement<E> *pX = m_head; pX != nullptr; pX = pX->m_next, ++p)
			if (pX == it) break;
		return p;
	}

	//@}
	/**
	 * @name Iterators
	 * These methods return forward iterators to elements in the list and allow to iterate over the elements in linear or cyclic order.
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
	iterator end() { return SListIterator<E>(); }

	//! Returns a const iterator to one-past-last element of the list.
	/**
	 * This is always a null pointer iterator.
	 */
	const_iterator end() const { return SListConstIterator<E>(); }

	//! Returns a const iterator to one-past-last element of the list.
	/**
	 * This is always a null pointer iterator.
	 */
	const_iterator cend() const { return SListConstIterator<E>(); }

	//! Returns an iterator to the last element of the list.
	/**
	 * If the list is empty, an invalid iterator is returned.
	 */
	iterator backIterator() { return m_tail; }

	//! Returns a const iterator to the last element of the list.
	/**
	 * If the list is empty, an invalid iterator is returned.
	 */
	const_iterator backIterator() const { return m_tail; }

	//! Returns an iterator to the cyclic successor of \p it.
	/**
	 * \pre \p it points to an element in this list!
	 */
	const_iterator cyclicSucc(const_iterator it) const {
		OGDF_ASSERT(it.listOf() == this);
		const SListElement<E> *pX = it;
		return (pX->m_next) ? pX->m_next : m_head;
	}

	//! Returns an iterator to the cyclic successor of \p it.
	/**
	 * \pre \p it points to an element in this list!
	 */
	iterator cyclicSucc(iterator it) {
		OGDF_ASSERT(it.listOf() == this);
		SListElement<E> *pX = it;
		return (pX->m_next) ? pX->m_next : m_head;
	}

	//@}
	/**
	 * @name Operators
	 * The following operators are provided by lists.
	 */
	//@{

	//! Assignment operator.
	SListPure<E> &operator=(const SListPure<E> &L) {
		clear(); copy(L);
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * The list \p L is empty afterwards.
	 */
	SListPure<E> &operator=(SListPure<E> &&L) {
		clear();
		m_head = L.m_head;
		m_tail = L.m_tail;
		L.m_head = L.m_tail = nullptr;
		reassignListRefs();
		return *this;
	}

	//! Equality operator.
	bool operator==(const SListPure<E> &L) const {
		SListElement<E> *pX = m_head, *pY = L.m_head;
		while(pX != nullptr && pY != nullptr) {
			if(pX->m_x != pY->m_x)
				return false;
			pX = pX->m_next;
			pY = pY->m_next;
		}
		return pX == nullptr && pY == nullptr;
	}

	//! Inequality operator.
	bool operator!=(const SListPure<E> &L) const {
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
		m_head = new SListElement<E>(this, x, m_head);
		if (m_tail == nullptr) m_tail = m_head;
		return m_head;
	}

	//! Adds a new element at the beginning of the list.
	/**
	* The element is constructed in-place with exactly the same arguments \p args as supplied to the function.
	*/
	template<class ... Args>
	iterator emplaceFront(Args && ... args) {
		m_head = new SListElement<E>(this, m_head, std::forward<Args>(args)...);
		if (m_tail == nullptr) m_tail = m_head;
		return m_head;
	}

	//! Adds element \p x at the end of the list.
	iterator pushBack(const E &x) {
		SListElement<E> *pNew = new SListElement<E>(this, x);
		if (m_head == nullptr)
			m_head = m_tail = pNew;
		else
			m_tail = m_tail->m_next = pNew;
		return m_tail;
	}

	//! Adds a new element at the end of the list.
	/**
	* The element is constructed in-place with exactly the same arguments \p args as supplied to the function.
	*/
	template<class ... Args>
	iterator emplaceBack(Args && ... args) {
		SListElement<E> *pNew = new SListElement<E>(this, nullptr, std::forward<Args>(args)...);
		if (m_head == nullptr)
			m_head = m_tail = pNew;
		else
			m_tail = m_tail->m_next = pNew;
		return m_tail;
	}

	//! Inserts element \p x after \p itBefore.
	/**
	 * \pre \p itBefore references an element in this list.
	 */
	iterator insertAfter(const E &x, iterator itBefore) {
		OGDF_ASSERT(itBefore.listOf() == this);
		SListElement<E> *pBefore = itBefore;
		SListElement<E> *pNew = new SListElement<E>(this, x, pBefore->m_next);
		if (pBefore == m_tail) m_tail = pNew;
		return (pBefore->m_next = pNew);
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
		SListElement<E> *pX = m_head;
		if ((m_head = m_head->m_next) == nullptr) m_tail = nullptr;
		delete pX;
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

	//! Removes the succesor of \p itBefore.
	/**
	 * \pre \p itBefore points to an element in this list.
	 */
	void delSucc(iterator itBefore) {
		OGDF_ASSERT(itBefore.listOf() == this);
		SListElement<E> *pBefore = itBefore;
		OGDF_ASSERT(pBefore != nullptr);
		SListElement<E> *pDel = pBefore->m_next;
		OGDF_ASSERT(pDel != nullptr);
		if ((pBefore->m_next = pDel->m_next) == nullptr) m_tail = pBefore;
		delete pDel;
	}

	//! Removes all elements from the list.
	void clear() {
		if (m_head == nullptr) return;

		if (!std::is_trivially_destructible<E>::value) {
			for(SListElement<E> *pX = m_head; pX != nullptr; pX = pX->m_next)
				pX->m_x.~E();
		}
		OGDF_ALLOCATOR::deallocateList(sizeof(SListElement<E>),m_head,m_tail);

		m_head = m_tail = nullptr;
	}

	//@}
	/**
	 * @name Moving elements
	 * The method allow to change the order of elements within the list, or to move elements to another list.
	 */
	//@{

	//! Moves the first element of this list to the begin of list \p L2.
	void moveFrontToFront(SListPure<E> &L2) {
		OGDF_ASSERT(m_head != nullptr);
		OGDF_ASSERT(this != &L2);

		SListElement<E> *pX = m_head;
		if ((m_head = m_head->m_next) == nullptr) m_tail = nullptr;
		pX->m_next = L2.m_head;
		L2.m_head = pX;
		if (L2.m_tail == nullptr) L2.m_tail = L2.m_head;

		L2.m_head->m_list = &L2;
	}

	//! Moves the first element of this list to the end of list \p L2.
	void moveFrontToBack(SListPure<E> &L2) {
		OGDF_ASSERT(m_head != nullptr);
		OGDF_ASSERT(this != &L2);

		SListElement<E> *pX = m_head;
		if ((m_head = m_head->m_next) == nullptr) m_tail = nullptr;
		pX->m_next = nullptr;
		if (L2.m_head == nullptr)
			L2.m_head = L2.m_tail = pX;
		else
			L2.m_tail = L2.m_tail->m_next = pX;

		L2.m_tail->m_list = &L2;
	}

	//! Moves the first element of this list to list \p L2 inserted after \p itBefore.
	/**
	 * \pre \p itBefore points to an element in \p L2.
	 */
	void moveFrontToSucc(SListPure<E> &L2, iterator itBefore) {
		OGDF_ASSERT(m_head != nullptr);
		OGDF_ASSERT(this != &L2);
		OGDF_ASSERT(itBefore.listOf() == &L2);

		SListElement<E> *pBefore = itBefore;
		SListElement<E> *pX = m_head;
		if ((m_head = m_head->m_next) == nullptr) m_tail = nullptr;
		pX->m_next = pBefore->m_next;
		pBefore->m_next = pX;
		if (pBefore == L2.m_tail) L2.m_tail = pX;

		pX->m_list = &L2;
	}

	//! Appends \p L2 to this list and makes \p L2 empty.
	void conc(SListPure<E> &L2) {
		if (m_head)
			m_tail->m_next = L2.m_head;
		else
			m_head = L2.m_head;
		if (L2.m_tail != nullptr) m_tail = L2.m_tail;

		reassignListRefs(L2.m_head);

		L2.m_head = L2.m_tail = nullptr;
	}

	//! Reverses the order of the list elements.
	void reverse() {
		SListElement<E> *p, *pNext, *pPred = nullptr;
		for(p = m_head; p; p = pNext) {
			pNext = p->m_next;
			p->m_next = pPred;
			pPred = p;
		}
		std::swap(m_head,m_tail);
	}

	//@}
	/**
	 * @name Searching and sorting
	 * These methods provide searching for values and sorting the list.
	 */
	//@{

	//! Scans the list for the specified element and returns an iterator to the first occurrence in the list, or an invalid iterator if not found.
	SListConstIterator<E> search(const E &e) const {
		SListConstIterator<E> i;
		for (i = begin(); i.valid(); ++i)
			if (*i == e) return i;
		return i;
	}

	//! Scans the list for the specified element and returns an iterator to the first occurrence in the list, or an invalid iterator if not found.
	SListIterator<E> search(const E &e) {
		SListIterator<E> i;
		for (i = begin(); i.valid(); ++i)
			if (*i == e) return i;
		return i;
	}

	//! Scans the list for the specified element (using the user-defined comparer) and returns an iterator to the first occurrence in the list, or an invalid iterator if not found.
	template<class COMPARER>
	SListConstIterator<E> search(const E &e, const COMPARER &comp) const {
		SListConstIterator<E> i;
		for (i = begin(); i.valid(); ++i)
			if (comp.equal(*i, e)) return i;
		return i;
	}

	//! Scans the list for the specified element (using the user-defined comparer) and returns an iterator to the first occurrence in the list, or an invalid iterator if not found.
	template<class COMPARER>
	SListIterator<E> search(const E &e, const COMPARER &comp) {
		SListIterator<E> i;
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

	//! Sorts the list using bucket sort.
	void bucketSort(BucketFunc<E> &f);

	//@}
	/**
	 * @name Random elements and permutations
	 * These methods allow to select a random element in the list, or to randomly permute the list.
	 */
	//@{

	//! @copydoc ogdf::List#chooseIterator
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

	//! @copydoc ogdf::List#chooseElement
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
	void copy(const SListPure<E> &L) {
		for(SListElement<E> *pX = L.m_head; pX != nullptr; pX = pX->m_next)
			pushBack(pX->m_x);
	}

	//! Permutes elements in list randomly; \p n is the length of the list.
	template<class RNG>
	void permute(const int n, RNG &rng);

	//! Sets the debug reference of all list elements starting at \p start to \c this.
	inline void reassignListRefs(SListElement<E> *start = nullptr) {
#ifdef OGDF_DEBUG
		for(auto e = start == nullptr ? m_head : start; e != nullptr; e = e->m_next) {
			e->m_list = this;
		}
#endif
	}

	OGDF_NEW_DELETE
};

//! Singly linked lists (maintaining the length of the list).
/**
 * @ingroup containers
 *
 * Use ogdf::SListConstIterator or ogdf::SListIterator in order to iterate over the list.
 * In contrast to ogdf::SListPure, instances of ogdf::SList store the length of the list
 * and thus allow constant time access to the length.
 *
 * @tparam E is the data type stored in list elements.
 */

template<class E>
class SList : private SListPure<E> {

	int m_count; //!< The length of the list.

public:
	using typename SListPure<E>::value_type;
	using typename SListPure<E>::reference;
	using typename SListPure<E>::const_reference;
	using typename SListPure<E>::const_iterator;
	using typename SListPure<E>::iterator;

	//! Constructs an empty singly linked list.
	SList() : m_count(0) { }

	//! Constructs a singly linked list containing the elements in \p init.
	SList(std::initializer_list<E> init) : SListPure<E>(init), m_count((int) init.size()) { }

	//! Constructs a singly linked list that is a copy of \p L.
	SList(const SList<E> &L) : SListPure<E>(L), m_count(L.m_count) { }

	//! Constructs a singly linked list containing the elements of \p L (move semantics).
	/**
	 * The list \p L is empty afterwards.
	 */
	SList(SList<E> &&L) : SListPure<E>(std::move(L)), m_count(L.m_count) {
		L.m_count = 0;
	}

	/**
	 * @name Access methods
	 * These methods provide simple access without changing the list.
	 */
	//@{

	//! Returns the number of elements in the list.
	/**
	 * This method has constant runtime (in contrast to SListPure::size()), since the list maintains the current size.
	 */
	int size() const { return m_count; }

	//! Conversion to const SListPure.
	const SListPure<E> &getSListPure() const { return *this; }

	//@}
	/**
	 * @name Operators
	 * The following operators are provided by lists.
	 */
	//@{

	//! Assignment operator.
	SList<E> &operator=(const SList<E> &L) {
		SListPure<E>::operator=(L);
		m_count = L.m_count;
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * The list \p L is empty afterwards.
	 */
	SList<E> &operator=(SList<E> &&L) {
		m_count = L.m_count;
		SListPure<E>::operator=(std::move(L));
		L.m_count = 0;
		return *this;
	}

	//! @copydoc ogdf::SListPure::operator==
	bool operator==(const SList<E> &L) const {
		return (m_count == L.m_count) && SListPure<E>::operator==(L);
	}

	//! @copydoc ogdf::SListPure::operator!=
	bool operator!=(const SList<E> &L) const {
		return !operator==(L);
	}

	//@}
	/**
	 * @name Adding elements
	 * These method add elements to the list.
	 */
	//@{

	//! @copydoc ogdf::SListPure::pushFront
	SListIterator<E> pushFront(const E &x) {
		++m_count;
		return SListPure<E>::pushFront(x);
	}

	//! @copydoc ogdf::SListPure::emplaceFront
	template<class ... Args>
	iterator emplaceFront(Args && ... args) {
		++m_count;
		return SListPure<E>::emplaceFront(std::forward<Args>(args)...);
	}

	//! @copydoc ogdf::SListPure::pushBack
	SListIterator<E> pushBack(const E &x) {
		++m_count;
		return SListPure<E>::pushBack(x);
	}

	//! @copydoc ogdf::SListPure::emplaceBack
	template<class ... Args>
	iterator emplaceBack(Args && ... args) {
		++m_count;
		return SListPure<E>::emplaceBack(std::forward<Args>(args)...);
	}

	//! @copydoc ogdf::SListPure::insertAfter
	SListIterator<E> insertAfter(const E &x, SListIterator<E> itBefore) {
		++m_count;
		return SListPure<E>::insertAfter(x, itBefore);
	}

	//@}
	/**
	 * @name Removing elements
	 * These method remove elements from the list.
	 */
	//@{

	//! @copydoc ogdf::SListPure::popFront
	void popFront() {
		--m_count;
		SListPure<E>::popFront();
	}

	//! @copydoc ogdf::SListPure::popFrontRet
	E popFrontRet() {
		E el = front();
		popFront();
		return el;
	}

	//! @copydoc ogdf::SListPure::delSucc
	void delSucc(SListIterator<E> itBefore) {
		--m_count;
		SListPure<E>::delSucc(itBefore);
	}

	//! @copydoc ogdf::SListPure::clear
	void clear() {
		m_count = 0;
		SListPure<E>::clear();
	}

	//@}
	/**
	 * @name Moving elements
	 * The method allow to change the order of elements within the list, or to move elements to another list.
	 */
	//@{

	//! @copydoc ogdf::SListPure::moveFrontToFront
	void moveFrontToFront(SList<E> &L2) {
		SListPure<E>::moveFrontToFront(L2);
		--m_count; ++L2.m_count;
	}

	//! @copydoc ogdf::SListPure::moveFrontToBack
	void moveFrontToBack(SList<E> &L2) {
		SListPure<E>::moveFrontToBack(L2);
		--m_count; ++L2.m_count;
	}

	//! @copydoc ogdf::SListPure::moveFrontToSucc
	void moveFrontToSucc(SList<E> &L2, SListIterator<E> itBefore) {
		SListPure<E>::moveFrontToSucc(L2,itBefore);
		--m_count; ++L2.m_count;
	}

	//! @copydoc ogdf::SListPure::conc
	void conc(SList<E> &L2) {
		SListPure<E>::conc(L2);
		m_count += L2.m_count;
		L2.m_count = 0;
	}

	using SListPure<E>::empty;
	using SListPure<E>::front;
	using SListPure<E>::back;
	using SListPure<E>::get;
	using SListPure<E>::pos;
	using SListPure<E>::begin;
	using SListPure<E>::cbegin;
	using SListPure<E>::end;
	using SListPure<E>::cend;
	using SListPure<E>::backIterator;
	using SListPure<E>::cyclicSucc;
	using SListPure<E>::reverse;
	using SListPure<E>::search;
	using SListPure<E>::quicksort;
	using SListPure<E>::bucketSort;
	using SListPure<E>::chooseIterator;
	using SListPure<E>::chooseElement;
	using SListPure<E>::permute;

	OGDF_NEW_DELETE
};

template<class E>
void SListPure<E>::bucketSort(BucketFunc<E> &f)
{
	// if less than two elements, nothing to do
	if (m_head == m_tail) return;

	int l, h;
	l = h = f.getBucket(m_head->m_x);

	SListElement<E> *pX;
	for(pX = m_head->m_next; pX; pX = pX->m_next)
	{
		int i = f.getBucket(pX->m_x);
		if (i < l) l = i;
		if (i > h) h = i;
	}

	bucketSort(l,h,f);
}

template<class E>
void SListPure<E>::bucketSort(int l, int h, BucketFunc<E> &f)
{
	// if less than two elements, nothing to do
	if (m_head == m_tail) return;

	Array<SListElement<E> *> head(l,h,nullptr), tail(l,h);

	SListElement<E> *pX;
	for (pX = m_head; pX; pX = pX->m_next) {
		int i = f.getBucket(pX->m_x);
		if (head[i])
			tail[i] = (tail[i]->m_next = pX);
		else
			head[i] = tail[i] = pX;
	}

	SListElement<E> *pY = nullptr;
	for (int i = l; i <= h; i++) {
		pX = head[i];
		if (pX) {
			if (pY)
				pY->m_next = pX;
			else
				m_head = pX;
			pY = tail[i];
		}
	}

	m_tail = pY;
	pY->m_next = nullptr;
}

template<class E>
template<class RNG>
void SListPure<E>::permute(const int n, RNG &rng)
{
	if (n == 0) {
		return;
	}

	Array<SListElement<E> *> A(n+1);
	A[n] = nullptr;

	int i = 0;
	SListElement<E> *pX;
	for (pX = m_head; pX; pX = pX->m_next)
		A[i++] = pX;

	A.permute(0,n-1,rng);

	for (i = 0; i < n; i++) {
		A[i]->m_next = A[i+1];
	}

	m_head = A[0];
	m_tail = A[n-1];
}

//! Prints list \p L to output stream \p os using delimiter \p delim.
template<class E>
void print(std::ostream &os, const SListPure<E> &L, char delim = ' ')
{
	SListConstIterator<E> pX = L.begin();
	if (pX.valid()) {
		os << *pX;
		for(++pX; pX.valid(); ++pX)
			os << delim << *pX;
	}
}

//! Prints list \p L to output stream \p os using delimiter \p delim.
template<class E>
void print(std::ostream &os, const SList<E> &L, char delim = ' ')
{
	print(os, L.getSListPure(), delim);
}

//! Output operator.
template<class E>
std::ostream &operator<<(std::ostream &os, const SListPure<E> &L)
{
	print(os,L);
	return os;
}

//! Output operator.
template<class E>
std::ostream &operator<<(std::ostream &os, const SList<E> &L)
{
	return operator<<(os,L.getSListPure());
}

//! Bucket-sort array \p a using bucket assignment \p f;
//! the values of \p f must be in the interval [\p min,\p max].
template<class E>
void bucketSort(Array<E> &a, int min, int max, BucketFunc<E> &f)
{
	if (a.low() >= a.high()) return;

	Array<SListPure<E> > bucket(min,max);

	int i;
	for(i = a.low(); i <= a.high(); ++i)
		bucket[f.getBucket(a[i])].pushBack(a[i]);

	i = a.low();
	for(int j = min; j <= max; ++j) {
		SListConstIterator<E> it = bucket[j].begin();
		for(; it.valid(); ++it)
			a[i++] = *it;
	}
}

}
