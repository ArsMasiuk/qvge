/** \file
 * \brief Data type for sorted sequences (based on skiplists)
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

#include <random>
#include <ogdf/basic/comparer.h>
#include <ogdf/basic/memory.h>
#include <ogdf/basic/Reverse.h>

namespace ogdf {

template<class KEY, class INFO, class CMP, bool isConst, bool isReverse>
class SortedSequenceIteratorBase;

template<class KEY, class INFO, class CMP>
using SortedSequenceIterator = SortedSequenceIteratorBase<KEY,INFO,CMP,false,false>;

template<class KEY, class INFO, class CMP>
using SortedSequenceConstIterator = SortedSequenceIteratorBase<KEY,INFO,CMP,true,false>;

template<class KEY, class INFO, class CMP>
using SortedSequenceReverseIterator = SortedSequenceIteratorBase<KEY,INFO,CMP,false,true>;

template<class KEY, class INFO, class CMP>
using SortedSequenceConstReverseIterator = SortedSequenceIteratorBase<KEY,INFO,CMP,true,true>;

//! Maintains a sequence of (key,info) pairs sorted by key.
/**
 * @ingroup containers
 *
 * Sorted sequences a implemented by doubly linked skiplists. Operations lookup, locate,
 * insert, del, delItem take expected time O(log \a n), where \a n is the current size of
 * the sequence.
 */
template<class KEY, class INFO, class CMP = StdComparer<KEY> >
class SortedSequence {

	friend class SortedSequenceIteratorBase<KEY,INFO,CMP,false,false>;
	friend class SortedSequenceIteratorBase<KEY,INFO,CMP,true,false>;
	friend class SortedSequenceIteratorBase<KEY,INFO,CMP,false,true>;
	friend class SortedSequenceIteratorBase<KEY,INFO,CMP,true,true>;

	//! Internal structure to hold the items and internal forward/backward pointers of the skiplist.
	struct Element {

		KEY  m_key; //!< stores the key.
		INFO m_info; //!< stores the associated information.
		int  m_height; //!< the height of the skiplist element.

		Element** m_next; //!< array of successor elements.
		Element** m_prev; //!< array of predecessor elements.

		//! Creates a skiplist element for(\p key,\p info) and given \p height.
		Element(const KEY &key, const INFO &info, int height)
			: m_key(key), m_info(info), m_height(height)
		{
			m_next = (Element**)malloc(height * sizeof(Element*));
			m_prev = (Element**)malloc(height * sizeof(Element*));
		}

		//! Creates a dummy (stop) element with given \p height.
		/**
		 * Stop elements are marked with height 0, although this is not their real height.
		 * It is not necessary to store that, as we use realloc to increase their height.
		 */
		Element(int height) : m_height(0)
		{
			m_next = (Element**)malloc(height * sizeof(Element*));
			m_prev = (Element**)malloc(height * sizeof(Element*));
		}

		Element(const Element &) = delete;

		//! Destructor.
		~Element() {
			free(m_prev);
			free(m_next);
		}

		//! Increases the element's height to \p newHeight.
		void grow(int newHeight) {
			Element **p = static_cast<Element**>( realloc(m_next, newHeight * sizeof(Element*)) );
			if (p == nullptr) OGDF_THROW(InsufficientMemoryException);
			m_next = p;

			p = static_cast<Element**>(realloc(m_prev, newHeight * sizeof(Element*)));
			if (p == nullptr) OGDF_THROW(InsufficientMemoryException);
			m_prev = p;
		}

		OGDF_NEW_DELETE
	};

public:

	//! The iterator type for sorted sequences (bidirectional iterator).
	using iterator = SortedSequenceIterator<KEY,INFO,CMP>;
	//! The const-iterator type for sorted sequences (bidirectional iterator).
	using const_iterator = SortedSequenceConstIterator<KEY,INFO,CMP>;
	//! The reverse iterator type for sorted sequences (bidirectional iterator).
	using reverse_iterator = SortedSequenceReverseIterator<KEY,INFO,CMP>;
	//! The const reverse iterator type for sorted sequences (bidirectional iterator).
	using const_reverse_iterator = SortedSequenceConstReverseIterator<KEY,INFO,CMP>;

	//! Constructs an initially empty sorted sequence.
	SortedSequence(const CMP &comparer = CMP()) : m_comparer(comparer), m_rng(randomSeed()), m_randomBits(0,1) {
		initEmpty();
	}

	//! Constructs a sorted sequence containing the elements in \p initList.
	SortedSequence(std::initializer_list < std::pair < KEY, INFO >> initList);

	//! Copy constructor.
	SortedSequence(const SortedSequence<KEY,INFO,CMP> &S);

	//! Copy constructor (move semantics).
	/**
	 * The sequence \p S is empty afterwards.
	 */
	SortedSequence(SortedSequence<KEY,INFO,CMP> &&S);


	//! Destructor
	~SortedSequence() {
		clear();
		delete m_dummy;
	}


	//@}
	/**
	 * @name General information and standard iterators
	 * These methods provide basic information like the number of elements in the list, as well as
	 * iterators to the begin and end of the sequence allowing forward and backward iteration over the sequence.
	 */
	//@{

	//! Returns the current size of the sequence, i.e., the number of stored elements.
	int size() const { return m_size; }

	//! Returns true if the sequence is empty, false otherwise.
	bool empty() const { return m_size == 0; }

	//! Returns an iterator pointing to the first element.
	iterator begin();

	//! Returns a const-iterator pointing to the first element.
	const_iterator begin() const;

	//! Returns a const-iterator pointing to the first element.
	const_iterator cbegin() const { return begin(); }

	//! Returns an iterator pointing to one past the last element.
	iterator end();

	//! Returns a const-iterator pointing to one past the last element.
	const_iterator end() const;

	//! Returns a const-iterator pointing to one past the last element.
	const_iterator cend() const { return end(); }

	//! Returns a reverse iterator pointing to the last element.
	reverse_iterator rbegin();

	//! Returns a const reverse iterator pointing to the last element.
	const_reverse_iterator rbegin() const;

	//! Returns a const reverse iterator pointing to the last element.
	const_reverse_iterator crbegin() const { return rbegin(); }

	//! Returns a reverse iterator pointing to one before the first element.
	reverse_iterator rend();

	//! Returns a const reverse iterator pointing to one before the first element.
	const_reverse_iterator rend() const;

	//! Returns a const reverse iterator pointing to one before the first element.
	const_reverse_iterator crend() const { return rend(); }


	/**
	 * @name Lookup operations
	 * These methods can be used to find elements by key. They return iterators pointing to the respective element in the sequence.
	 */
	//@{

	//! Returns an iterator to the element with key \p key, or a null iterator if no such element exists.
	iterator lookup(const KEY &key);

	//! Returns a const-iterator to the element with key \p key, or a null iterator if no such element exists.
	const_iterator lookup(const KEY &key) const;

	//! Returns an iterator to the element < \a k1, \a i1 > such that \a k1 is minimal with \a k1 &ge; \p key, or a null iterator if no such element exists.
	iterator locate(const KEY &key);

	//! Returns a const-iterator to the element < \a k1, \a i1 > such that \a k1 is minimal with \a k1 &ge; \p key, or a null iterator if no such element exists.
	const_iterator locate(const KEY &key) const;

	//! Returns an iterator to the element with minimal key if the sequence is not empty, an invalid iterator otherwise.
	/**
	 * Calling this method is equivalent to calling begin(), but has a more intuitive name.
	 */
	iterator minItem() { return begin(); }

	//! Returns a const-iterator to the element with minimal key if the sequence is not empty, an invalid const-iterator otherwise.
	/**
	 * Calling this method is equivalent to calling begin(), but has a more intuitive name.
	 */
	const_iterator minItem() const { return begin(); }

	//! Returns a reverse iterator to the element with maximal key if the sequence is not empty, an invalid reverse iterator otherwise.
	/**
	 * Calling this method is equivalent to calling rbegin(), but has a more intuitive name.
	 */
	reverse_iterator maxItem() { return rbegin(); }

	//! Returns a const reverse iterator to the element with maximal key if the sequence is not empty, an invalid const reverse iterator otherwise.
	/**
	 * Calling this method is equivalent to calling rbegin(), but has a more intuitive name.
	 */
	const_reverse_iterator maxItem() const { return rbegin(); }


	//@}
	/**
	 * @name Insertion and deletion
	 * These method provide basic modification methods, like inserting new elements or removing elements from the sequence.
	 */
	//@{

	//! Updates information for \p key if contained in sequence, or adds a new element <\p key, \p info>.
	iterator insert(const KEY &key, const INFO &info);

	//! Removes the element with key \p key (if such an element exists).
	void del(const KEY &key);

	//! Removes the element to which \p it points from the sequence.
	void delItem(iterator it);

	//! Removes all elements from the sorted sequence.
	void clear() {
		Element* item = m_dummy->m_next[0];
		while(item != m_dummy) {
			Element* old = item;
			item = item->m_next[0];
			delete old;
		}
		m_size = 0;
		m_height = 1;
		m_dummy->m_next[0] = m_dummy->m_prev[0] = m_dummy;
	}

	//@}
	/**
	 * @name Operators
	 * The following operators are overloeded for sorted sequences.
	 */
	//@{

	//! Assignment operator.
	SortedSequence<KEY,INFO,CMP> &operator=(const SortedSequence<KEY,INFO,CMP> &S);

	//! Assignment operator (move semantics).
	/**
	 * The sequence \p S is empty afterwards.
	 */
	SortedSequence<KEY,INFO,CMP> &operator=(SortedSequence<KEY,INFO,CMP> &&S);

	//! Returns true if the keys stored in this sequence equal the keys in \p S, false otherwise.
	/**
	 * Uses the given comparer object to compare keys.
	 */
	bool operator==(const SortedSequence<KEY,INFO,CMP> &S);

	//! Returns false if the keys stored in this sequence equal the keys in \p S, true otherwise.
	/**
	 * Uses the given comparer object to compare keys.
	 */
	bool operator!=(const SortedSequence<KEY,INFO,CMP> &S) {
		return !operator==(S);
	}

	//@}
	/**
	 * @name Special modification methods
	 * These methods must be handled with care; they are only useful in very specific scenarios. First read their documentation carefully!
	 */
	//@{

	//! Adds a new element <\p key, \p info> after element \p it.
	/**
	 * \pre \p it points to an element in the sequence that shall appear before <\p key, \p info> in the sorted sequence,
	 * and its current successor \a itSucc shall appear after <\p key, \p info>, i.e., \p it's key is smaller than \p key
	 * and \a itSucc's key is greater than \p key.
	 */
	iterator insertAfter(iterator it, const KEY &key, const INFO &info);

	//! Reverses the items in the subsequence from \p itBegin to \p itEnd (inclusive).
	/**
	 * \pre \p itBegin appears before \p itEnd in the sequence.
	 *
	 * \warning
	 * Usually, you do not need this method as the sequence is always sorted.
	 * It only makes sense if you use a special compare function that changes the underlying linear ordering,
	 * and you have to adjust the sorting manually.
	 * Do not use this method unless you know exactly what you are doing! After applying
	 * this method the subsequence should be ordered correctly according to the compare function.
	 */
	void reverseItems(iterator itBegin, iterator itEnd) {
		reverseElements(itBegin.m_pElement, itEnd.m_pElement);
	}

	//@}

private:
	CMP m_comparer;
	int m_size; //!< number of elements stored in the sequence.
	Element *m_dummy; //!< dummy element representing the head and tail of the skiplist.
	int m_height; //!< current height of head/tail.
	int m_realHeight; //!< actual height of head/tail arrays.

	std::minstd_rand m_rng;  //!< Random number generator
	std::uniform_int_distribution<> m_randomBits;


	void initEmpty() {
		m_size = 0;
		m_realHeight = 5;
		m_height = 1;

		m_dummy = new Element(m_realHeight);
		m_dummy->m_next[0] = m_dummy;
		m_dummy->m_prev[0] = m_dummy;
	}

	int randomHeightAndGrow();
	void grow(int newHeight);

	const Element *_lookup(const KEY &key) const;
	const Element *_locate(const KEY &key) const;

	void removeElement(Element *p);
	void insertElementAfterElement(Element *p, Element *q);
	void reverseElements(Element *p, Element *q);
};


//! Iterators for sorted sequences.
template<class KEY, class INFO, class CMP, bool isConst, bool isReverse>
class SortedSequenceIteratorBase {
	friend class SortedSequence<KEY,INFO,CMP>;
	friend class SortedSequenceIteratorBase<KEY,INFO,CMP,!isConst,isReverse>;
	friend class SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,!isReverse>;
	friend class SortedSequenceIteratorBase<KEY,INFO,CMP,!isConst,!isReverse>;

	using Element = typename std::conditional<isConst,
		  const typename SortedSequence<KEY,INFO,CMP>::Element,
		  typename SortedSequence<KEY,INFO,CMP>::Element>::type;
	Element *m_pElement;

	//! Creates an iterator pointing to \p pElement.
	SortedSequenceIteratorBase(Element *pElement) : m_pElement(pElement) { }

public:
	//! Creates an invalid (null-) iterator.
	SortedSequenceIteratorBase() : m_pElement(nullptr) { }

	//! Copy constructor.
	template<bool isArgConst, typename std::enable_if<isConst || !isArgConst, int>::type = 0, bool isArgReverse>
	SortedSequenceIteratorBase(const SortedSequenceIteratorBase<KEY,INFO,CMP,isArgConst,isArgReverse> &it)
		: m_pElement(it.m_pElement) { }

	//! Copy constructor.
	// gcc9 complains since it cannot match the templated constructor above (-Werror=deprecated-copy).
	SortedSequenceIteratorBase(const SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> &it)
		: m_pElement(it.m_pElement) { }

	//! Returns the key of the sequence element pointed to.
	const KEY &key() const {
		OGDF_ASSERT(valid());
		return m_pElement->m_key;
	}

	//! Returns the info of the sequence element pointed to.
	typename std::conditional<isConst, const INFO, INFO>::type &info() const {
		OGDF_ASSERT(valid());
		return m_pElement->m_info;
	}

	//! Returns true if the iterator points to an element.
	bool valid() const { return m_pElement != nullptr; }

	//! Move the iterator one item forward (prefix notation)
	SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> &operator++() {
		m_pElement = isReverse ? predElement() : succElement();
		return *this;
	}

	//! Moves the iterator one item forward (postfix notation)
	SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> operator++(int) {
		SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> it = *this;
		m_pElement = isReverse ? predElement() : succElement();
		return it;
	}

	//! Moves the iterator one item backward (prefix notation)
	SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> &operator--() {
		m_pElement = isReverse ? succElement() : predElement();
		return *this;
	}

	//! Moves the iterator one item backward (postfix notation)
	SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> operator--(int) {
		SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> it = *this;
		m_pElement = isReverse ? succElement() : predElement();
		return it;
	}

	//! Returns an iterator pointing to the next element in the sequence.
	SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> succ() const {
		return isReverse ? predElement() : succElement();
	}

	//! Returns an iterator pointing to the previous element in the sequence.
	SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> pred() const {
		return isReverse ? succElement() : predElement();
	}

	//! Assignment operator
	SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> &operator=(const SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> &it) {
		m_pElement = it.m_pElement;
		return *this;
	}

	//! Equality operator.
	bool operator==(const SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> &it) const {
		return m_pElement == it.m_pElement;
	}

	//! Inequality operator.
	bool operator!=(const SortedSequenceIteratorBase<KEY,INFO,CMP,isConst,isReverse> &it) const {
		return m_pElement != it.m_pElement;
	}

private:
	typename SortedSequence<KEY,INFO,CMP>::Element *succElement() const {
		OGDF_ASSERT(valid());
		return (m_pElement->m_next[0]->m_height > 0) ? m_pElement->m_next[0] : nullptr;
	}

	typename SortedSequence<KEY,INFO,CMP>::Element *predElement() const {
		OGDF_ASSERT(valid());
		return (m_pElement->m_prev[0]->m_height > 0) ? m_pElement->m_prev[0] : nullptr;
	}
};

template<class KEY, class INFO, class CMP>
SortedSequence<KEY, INFO, CMP>::SortedSequence(std::initializer_list < std::pair < KEY, INFO >> initList)
	: SortedSequence()
{
	for (const auto &p : initList)
		insert(p.first, p.second);
}


template<class KEY, class INFO, class CMP>
SortedSequence<KEY,INFO,CMP>::SortedSequence(const SortedSequence<KEY,INFO,CMP> &S)
	: m_comparer(S.m_comparer), m_rng(randomSeed()), m_randomBits(0,1)
{
	initEmpty();

	iterator it = m_dummy;
	for(Element *pS = S.m_dummy->m_next[0]; pS != S.m_dummy; pS = pS->m_next[0])
		it = insertAfter(it, pS->m_key, pS->m_info);
}


template<class KEY, class INFO, class CMP>
SortedSequence<KEY,INFO,CMP>::SortedSequence(SortedSequence<KEY,INFO,CMP> &&S)
	: m_comparer(S.m_comparer), m_size(S.m_size), m_height(S.m_height), m_realHeight(S.m_realHeight), m_rng(randomSeed()), m_randomBits(0,1)
{
	// move all elements to this sequence
	m_dummy = S.m_dummy;

	// initalize S with an empty sequence
	S.initEmpty();
}


template<class KEY, class INFO, class CMP>
SortedSequence<KEY,INFO,CMP> &SortedSequence<KEY,INFO,CMP>::operator=(const SortedSequence<KEY,INFO,CMP> &S)
{
	clear();

	iterator it = m_dummy;
	for(Element *pS = S.m_dummy->m_next[0]; pS != S.m_dummy; pS = pS->m_next[0])
		it = insertAfter(it, pS->m_key, pS->m_info);

	return *this;
}


template<class KEY, class INFO, class CMP>
SortedSequence<KEY,INFO,CMP> &SortedSequence<KEY,INFO,CMP>::operator=(SortedSequence<KEY,INFO,CMP> &&S)
{
	// clear this sequence
	Element* item = m_dummy->m_next[0];
	while(item != m_dummy) {
		Element* old = item;
		item = item->m_next[0];
		delete old;
	}
	delete m_dummy;

	// move elements from S to this sequence
	m_comparer   = S.m_comparer;
	m_size       = S.m_size;
	m_height     = S.m_height;
	m_realHeight = S.m_realHeight;
	m_dummy      = S.m_dummy;

	// make S the empty sequence
	S.initEmpty();

	return *this;
}


template<class KEY, class INFO, class CMP>
bool SortedSequence<KEY,INFO,CMP>::operator==(const SortedSequence<KEY,INFO,CMP> &S)
{
	if(m_size != S.m_size)
		return false;

	Element *p = m_dummy->m_next[0], *pS = S.m_dummy->m_next[0];
	while(p != m_dummy) {
		if (!m_comparer.equal(p->m_key,pS->m_key))
			return false;
		p  = p ->m_next[0];
		pS = pS->m_next[0];
	}

	return true;
}


template<class KEY, class INFO, class CMP>
const typename SortedSequence<KEY,INFO,CMP>::Element *SortedSequence<KEY,INFO,CMP>::_lookup(const KEY &key) const
{
	int h = m_height - 1;
	Element** pElement = m_dummy->m_next;

	while(true)	{
		if( pElement[h] != m_dummy && m_comparer.less(pElement[h]->m_key, key) )
			pElement = pElement[h]->m_next;
		else if(--h < 0) {
			if( pElement[0] != m_dummy && m_comparer.equal(pElement[0]->m_key, key) )
				return pElement[0];
			return nullptr;
		}
	}
}

template<class KEY, class INFO, class CMP>
typename SortedSequence<KEY,INFO,CMP>::iterator SortedSequence<KEY,INFO,CMP>::lookup(const KEY &key)
{
	return iterator(const_cast<Element *>(_lookup(key)));
}

template<class KEY, class INFO, class CMP>
typename SortedSequence<KEY,INFO,CMP>::const_iterator SortedSequence<KEY,INFO,CMP>::lookup(const KEY &key) const
{
	return const_iterator(_lookup(key));
}


template<class KEY, class INFO, class CMP>
const typename SortedSequence<KEY,INFO,CMP>::Element *SortedSequence<KEY,INFO,CMP>::_locate(const KEY &key) const
{
	int h = m_height - 1;
	Element** pElement = m_dummy->m_next;

	while(true)	{
		if( pElement[h] != m_dummy && m_comparer.less(pElement[h]->m_key, key) )
			pElement = pElement[h]->m_next;
		else if(--h < 0) {
			Element *p = (pElement[0] != m_dummy) ? pElement[0] : nullptr;
			return p;
		}
	}
}

template<class KEY, class INFO, class CMP>
typename SortedSequence<KEY,INFO,CMP>::iterator SortedSequence<KEY,INFO,CMP>::locate(const KEY &key)
{
	return iterator(const_cast<Element *>(_locate(key)));
}

template<class KEY, class INFO, class CMP>
typename SortedSequence<KEY,INFO,CMP>::const_iterator SortedSequence<KEY,INFO,CMP>::locate(const KEY &key) const
{
	return const_iterator(_locate(key));
}


template<class KEY, class INFO, class CMP>
void SortedSequence<KEY,INFO,CMP>::grow(int newHeight)
{
	if(newHeight > m_realHeight) {
		m_realHeight = newHeight;
		m_dummy->grow(newHeight);
	}

	for(int i = newHeight; i-- > m_height; ) {
		m_dummy->m_next[i] = m_dummy;
		m_dummy->m_prev[i] = m_dummy;
	}
	m_height = newHeight;
}

template<class KEY, class INFO, class CMP>
int SortedSequence<KEY,INFO,CMP>::randomHeightAndGrow()
{
	int h = 1;
	while(m_randomBits(m_rng) == 1)
		h++;

	if(h > m_height)
		grow(h);

	return h;
}


template<class KEY, class INFO, class CMP>
typename SortedSequence<KEY,INFO,CMP>::iterator SortedSequence<KEY,INFO,CMP>::insert(const KEY &key, const INFO &info)
{
	int h = m_height - 1;
	Element *pCurrent = m_dummy;

	while(true)	{
		if( pCurrent->m_next[h] != m_dummy && m_comparer.less(pCurrent->m_next[h]->m_key, key) ) {
			pCurrent = pCurrent->m_next[h];

		} else {
			if(--h < 0) {
				// found element?
				if( pCurrent->m_next[0] != m_dummy && m_comparer.equal(pCurrent->m_next[0]->m_key, key) ) {
					pCurrent->m_next[0]->m_info = info;
					return iterator(pCurrent->m_next[0]);
				}
				break;
			}
		}
	}

	// add new element (key,inf)
	m_size++;

	int nh = randomHeightAndGrow();

	Element* pNewElement = new Element(key, info, nh);
	insertElementAfterElement(pNewElement, pCurrent);

	return iterator(pNewElement);
}


template<class KEY, class INFO, class CMP>
void SortedSequence<KEY,INFO,CMP>::del(const KEY &key)
{
	iterator it = lookup(key);
	if(it.valid())
		delItem(it);
}


template<class KEY, class INFO, class CMP>
typename SortedSequence<KEY,INFO,CMP>::iterator SortedSequence<KEY,INFO,CMP>::insertAfter(iterator it, const KEY &key, const INFO &info)
{
	m_size++;

	int nh = randomHeightAndGrow();

	Element* pNewElement = new Element(key, info, nh);
	insertElementAfterElement(pNewElement, it.m_pElement);

	return pNewElement;
}


template<class KEY, class INFO, class CMP>
void SortedSequence<KEY,INFO,CMP>::insertElementAfterElement(Element *p, Element *q)
{
	OGDF_ASSERT(p->m_height <= m_height);

	for(int h = 0; h < p->m_height; ++h) {
		while(q != m_dummy && q->m_height <= h)
			q = q->m_prev[h-1];

		Element *r = q->m_next[h];
		p->m_next[h] = r;
		p->m_prev[h] = q;
		q->m_next[h] = r->m_prev[h] = p;
	}
}


template<class KEY, class INFO, class CMP>
void SortedSequence<KEY,INFO,CMP>::reverseElements(Element *p, Element *q)
{
	while(p != q) {
		Element *r = p;
		p = p->m_next[0];
		removeElement(r);
		insertElementAfterElement(r,q);
	}
}


template<class KEY, class INFO, class CMP>
void SortedSequence<KEY,INFO,CMP>::removeElement(Element *p)
{
	OGDF_ASSERT(p != nullptr);
	OGDF_ASSERT(p != m_dummy);

	for(int h = 0; h < p->m_height; ++h) {
		Element *pPred = p->m_prev[h], *pSucc = p->m_next[h];
		pPred->m_next[h] = pSucc;
		pSucc->m_prev[h] = pPred;
	}
}


template<class KEY, class INFO, class CMP>
void SortedSequence<KEY,INFO,CMP>::delItem(typename SortedSequence<KEY,INFO,CMP>::iterator it)
{
	Element *p = it.m_pElement;
	removeElement(p);

	m_size--;
	delete p;
}


template<class KEY, class INFO, class CMP>
inline typename SortedSequence<KEY,INFO,CMP>::iterator
	SortedSequence<KEY,INFO,CMP>::begin()
{
	return (m_dummy->m_next[0] != m_dummy) ? m_dummy->m_next[0] : nullptr;
}

template<class KEY, class INFO, class CMP>
inline typename SortedSequence<KEY,INFO,CMP>::const_iterator
	SortedSequence<KEY,INFO,CMP>::begin() const
{
	return (m_dummy->m_next[0] != m_dummy) ? m_dummy->m_next[0] : 0;
}


template<class KEY, class INFO, class CMP>
inline typename SortedSequence<KEY,INFO,CMP>::iterator
	SortedSequence<KEY,INFO,CMP>::end()
{
	return SortedSequence<KEY,INFO,CMP>::iterator();
}

template<class KEY, class INFO, class CMP>
inline typename SortedSequence<KEY,INFO,CMP>::const_iterator
	SortedSequence<KEY,INFO,CMP>::end() const
{
	return SortedSequence<KEY,INFO,CMP>::const_iterator();
}


template<class KEY, class INFO, class CMP>
inline typename SortedSequence<KEY,INFO,CMP>::reverse_iterator
	SortedSequence<KEY,INFO,CMP>::rbegin()
{
	return (m_dummy->m_prev[0] != m_dummy) ? m_dummy->m_prev[0] : 0;
}

template<class KEY, class INFO, class CMP>
inline typename SortedSequence<KEY,INFO,CMP>::const_reverse_iterator
	SortedSequence<KEY,INFO,CMP>::rbegin() const
{
	return (m_dummy->m_prev[0] != m_dummy) ? m_dummy->m_prev[0] : 0;
}


template<class KEY, class INFO, class CMP>
inline typename SortedSequence<KEY,INFO,CMP>::reverse_iterator
	SortedSequence<KEY,INFO,CMP>::rend()
{
	return SortedSequence<KEY,INFO,CMP>::iterator();
}

template<class KEY, class INFO, class CMP>
inline typename SortedSequence<KEY,INFO,CMP>::const_reverse_iterator
	SortedSequence<KEY,INFO,CMP>::rend() const
{
	return SortedSequence<KEY,INFO,CMP>::const_iterator();
}

}
