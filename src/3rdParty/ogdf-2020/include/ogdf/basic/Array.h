/** \file
 * \brief Declaration and implementation of Array class and
 * Array algorithms
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

#include <ogdf/basic/comparer.h>
#include <ogdf/basic/memory.h>
#include <ogdf/basic/exceptions.h>
#include <ogdf/basic/Reverse.h>
#include <random>
#include <type_traits>


namespace ogdf {

template<class E, class INDEX> class ArrayBuffer;

template<class E, bool isConst> class ArrayReverseIteratorBase;
template<class E> using ArrayConstIterator = const E*;
template<class E> using ArrayIterator = E*;
template<class E> using ArrayConstReverseIterator = ArrayReverseIteratorBase<E, true>;
template<class E> using ArrayReverseIterator = ArrayReverseIteratorBase<E, false>;

//! Random-access reverse iterator based on a pointer to an array element.
/**
 * Swaps all operations involving an increment of the pointer by operations
 * involving a decrement, and vice versa. Moreover, the relational operators are
 * swapped as well. It is possible that an iterator encapsulates a null pointer.
 *
 * @tparam E The type of element.
 * @tparam isConst True iff this iterator allows only const-access to the element.
 */
template<class E, bool isConst> class ArrayReverseIteratorBase {
	friend class ArrayReverseIteratorBase<E, !isConst>;

	//! The underlying element, depending on isConst.
	using Elem = typename std::conditional<isConst, const E, E>::type;

	//! The pointer to the array element.
	Elem *m_pX;

public:
	//! Constructs an iterator that points to E* \p pX.
	ArrayReverseIteratorBase(E *pX) : m_pX(pX) { }

	//! Constructs an iterator that points to const E* \p pX.
	template<bool isConstSFINAE = isConst, typename std::enable_if<isConstSFINAE, int>::type = 0>
	ArrayReverseIteratorBase(const E *pX) : m_pX(pX) { }

	//! Constructs an invalid iterator.
	ArrayReverseIteratorBase() : ArrayReverseIteratorBase(nullptr) { }

	//! Constructs an iterator that is a copy of \p it.
	template<bool isArgConst, typename std::enable_if<isConst || !isArgConst, int>::type = 0>
	ArrayReverseIteratorBase(const ArrayReverseIteratorBase<E,isArgConst> &it) : ArrayReverseIteratorBase(it.m_pX) { }

	//! Implicit cast to (const) E*.
	operator std::conditional<isConst, const E, E> *() const { return m_pX; }

	//! Equality operator.
	bool operator==(const ArrayReverseIteratorBase<E, isConst> &it) const {
		return m_pX == it.m_pX;
	}

	//! Inequality operator.
	bool operator!=(const ArrayReverseIteratorBase<E, isConst> &it) const {
		return m_pX != it.m_pX;
	}

	//! Returns the element this iterator points to.
	Elem &operator*() const { return *m_pX; }

	//! Assignment operator.
	ArrayReverseIteratorBase<E, isConst> &operator=(const ArrayReverseIteratorBase<E, isConst> &it) {
		m_pX = it.m_pX;
		return *this;
	}

	//! Increment operator (prefix).
	ArrayReverseIteratorBase<E, isConst> &operator++() {
		m_pX--;
		return *this;
	}

	//! Increment operator (postfix).
	ArrayReverseIteratorBase<E, isConst> operator++(int) {
		ArrayReverseIteratorBase<E, isConst> it = *this;
		m_pX--;
		return it;
	}

	//! Decrement operator (prefix).
	ArrayReverseIteratorBase<E, isConst> &operator--() {
		m_pX++;
		return *this;
	}

	//! Decrement operator (postfix).
	ArrayReverseIteratorBase<E, isConst> operator--(int) {
		ArrayReverseIteratorBase<E, isConst> it = *this;
		m_pX++;
		return it;
	}

	//! Compound assignment operator (+).
	ArrayReverseIteratorBase<E, isConst>& operator+=(const int &rhs) {
		m_pX -= rhs;
		return *this;
	}

	//! Compound assignment operator (-).
	ArrayReverseIteratorBase<E, isConst>& operator-=(const int &rhs) {
		m_pX += rhs;
		return *this;
	}

	//! Addition operator with int on the right-hand side.
	ArrayReverseIteratorBase<E, isConst> operator+(const int &rhs) {
		return ArrayReverseIteratorBase<E, isConst>(m_pX - rhs);
	}

	//! Addition operator with int on the left-hand side.
	//! Returns the same result as addition with int on the right-hand side.
	friend ArrayReverseIteratorBase<E, isConst> operator+(
			const int &lhs, ArrayReverseIteratorBase<E, isConst> rhs) {
		return ArrayReverseIteratorBase<E, isConst>(rhs.m_pX - lhs);
	}

	//! Subtraction operator with int on the right-hand side.
	ArrayReverseIteratorBase<E, isConst> operator-(const int &rhs) {
		return ArrayReverseIteratorBase<E, isConst>(m_pX + rhs);
	}

	//! Subtraction operator.
	template<bool isArgConst>
	int operator-(ArrayReverseIteratorBase<E, isArgConst> &rhs) {
		return rhs.m_pX - m_pX;
	}

	//! Less-than operator.
	bool operator< (ArrayReverseIteratorBase<E, isConst> &it) const { return m_pX > it.m_pX; }

	//! Greater-than operator.
	bool operator> (ArrayReverseIteratorBase<E, isConst> &it) const { return m_pX < it.m_pX; }

	//! Less-than-or-equals operator.
	bool operator<=(ArrayReverseIteratorBase<E, isConst> &it) const { return m_pX >= it.m_pX; }

	//! Greater-than-or-equals operator.
	bool operator>=(ArrayReverseIteratorBase<E, isConst> &it) const { return m_pX <= it.m_pX; }

	//! Member access operator.
	Elem &operator[](std::size_t idx) { return m_pX[-idx]; }

	//! Const member access operator.
	const Elem &operator[](std::size_t idx) const { return m_pX[-idx]; }

	OGDF_NEW_DELETE
};

//! The parameterized class Array implements dynamic arrays of type \a E.
/**
 * @ingroup containers
 *
 * @tparam E     denotes the element type.
 * @tparam INDEX denotes the index type. The index type must be chosen such that it can
 *               express the whole index range of the array instance, as well as its size.
 *               The default index type is \c int, other possible types are \c short and
 *               <code>long long</code> (on 64-bit systems).
 */
template<class E, class INDEX = int> class Array {
public:
	//! Threshold used by #quicksort() such that insertion sort is
	//! called for instances smaller than #maxSizeInsertionSort.
	static const int maxSizeInsertionSort = 40;

	//! Represents the data type stored in an array element.
	using value_type = E;
	//! Provides a reference to an element stored in an array.
	using reference = E&;
	//! Provides a reference to a const element stored in an array for reading and performing const operations.
	using const_reference = const E&;
	//! Provides a random-access iterator that can read a const element in an array.
	using const_iterator = ArrayConstIterator<E>;
	//! Provides a random-access iterator that can read or modify any element in an array.
	using iterator = ArrayIterator<E>;
	//! Provides a reverse random-access iterator that can read a const element in an array.
	using const_reverse_iterator = ArrayConstReverseIterator<E>;
	//! Provides a reverse random-access iterator that can read or modify any element in an array.
	using reverse_iterator = ArrayReverseIterator<E>;

	//! Creates an array with empty index set.
	Array() { construct(0,-1); }

	//! Creates an array with index set [0..\p s-1].
	explicit Array(INDEX s) : Array(0, s - 1) { }

	//! Creates an array with index set [\p a..\p b].
	Array(INDEX a, INDEX b) {
		construct(a,b); initialize();
	}

	//! Creates an array with index set [\p a..\p b] and initializes each element with \p x.
	Array(INDEX a, INDEX b, const E &x) {
		construct(a,b); initialize(x);
	}

	//! Creates an array containing the elements in the initializer list \p initList.
	/**
	 * The index set of the array is set to 0, ..., number of elements in \p initList - 1.
	 */
	Array(std::initializer_list<E> initList) {
		construct(0, ((INDEX) initList.size()) - 1);
		initialize(initList);
	}

	//! Creates an array that is a copy of \p A.
	Array(const Array<E,INDEX> &A) {
		copy(A);
	}

	//! Creates an array containing the elements of \p A (move semantics).
	/**
	 * The array \p A is empty afterwards.
	 */
	Array(Array<E,INDEX> &&A)
		: m_vpStart(A.m_vpStart), m_pStart(A.m_pStart), m_pStop(A.m_pStop), m_low(A.m_low), m_high(A.m_high)
	{
		A.construct(0,-1);
	}

	//! Creates an array that is a copy of \p A. The array-size is set to be the number of elements (not the capacity) of the buffer.
	Array(const ArrayBuffer<E,INDEX> &A);

	//! Destruction
	~Array() {
		deconstruct();
	}

	/**
	 * @name Access methods
	 * These methods provide access to elements, size, and index range.
	 */
	//@{

	//! Returns the minimal array index.
	INDEX low() const { return m_low; }

	//! Returns the maximal array index.
	INDEX high() const { return m_high; }

	//! Returns the size (number of elements) of the array.
	INDEX size() const { return m_high - m_low + 1; }

	//! Returns \c true iff there are no elements in the array.
	bool empty() const { return size() == 0; }

	//! Returns a reference to the element at position \p i.
	const_reference operator[](INDEX i) const {
		OGDF_ASSERT(m_low <= i);
		OGDF_ASSERT(i <= m_high);
		return m_vpStart[i];
	}

	//! Returns a reference to the element at position \p i.
	reference operator[](INDEX i) {
		OGDF_ASSERT(m_low <= i);
		OGDF_ASSERT(i <= m_high);
		return m_vpStart[i];
	}


	//@}
	/**
	 * @name Iterators
	 * These methods return random-access iterators to elements in the array.
	 */
	//@{

	//! Returns an iterator to the first element.
	iterator begin() { return m_pStart; }

	//! Returns a const iterator to the first element.
	const_iterator begin() const { return m_pStart; }

	//! Returns a const iterator to the first element.
	const_iterator cbegin() const { return m_pStart; }

	//! Returns an iterator to one past the last element.
	iterator end() { return m_pStop; }

	//! Returns a const iterator to one past the last element.
	const_iterator end() const { return m_pStop; }

	//! Returns a const iterator to one past the last element.
	const_iterator cend() const { return m_pStop; }

	//! Returns an reverse iterator to the last element.
	reverse_iterator rbegin() { return m_pStop-1; }

	//! Returns a const reverse iterator to the last element.
	const_reverse_iterator rbegin() const { return m_pStop-1; }

	//! Returns a const reverse iterator to the last element.
	const_reverse_iterator crbegin() const { return m_pStop-1; }

	//! Returns an reverse iterator to one before the first element.
	reverse_iterator rend() { return m_pStart-1; }

	//! Returns a const reverse iterator to one before the first element.
	const_reverse_iterator rend() const { return m_pStart-1; }

	//! Returns a const reverse iterator to one before the first element.
	const_reverse_iterator crend() const { return m_pStart-1; }


	//@}
	/**
	 * @name Initialization and assignment
	 * These methods can be used to reinitialize or resize the array, or to initialize all elements with a given value.
	 */
	//@{

	//! Reinitializes the array to an array with empty index set.
	void init() {
		deconstruct();
		construct(0,-1);
	}

	//! Reinitializes the array to an array with index set [0..\p s-1].
	/**
	 * Notice that the elements contained in the array get discarded!
	 */
	void init(INDEX s) { init(0,s-1); }

	//! Reinitializes the array to an array with index set [\p a..\p b].
	/**
	 * Notice that the elements contained in the array get discarded!
	 */
	void init(INDEX a, INDEX b) {
		deconstruct();
		construct(a,b);
		initialize();
	}

	//! Reinitializes the array to an array with index set [\p a..\p b] and sets all entries to \p x.
	void init(INDEX a, INDEX b, const E &x) {
		deconstruct();
		construct(a,b);
		initialize(x);
	}

	//! Sets all elements to \p x.
	void fill(const E &x) {
		E *pDest = m_pStop;
		while(pDest > m_pStart)
			*--pDest = x;
	}

	//! Sets elements in the intervall [\p i..\p j] to \p x.
	void fill(INDEX i, INDEX j, const E &x) {
		OGDF_ASSERT(m_low <= i);
		OGDF_ASSERT(i <= m_high);
		OGDF_ASSERT(m_low <= j);
		OGDF_ASSERT(j <= m_high);

		E *pI = m_vpStart + i, *pJ = m_vpStart + j+1;
		while(pJ > pI)
			*--pJ = x;
	}

	//! Enlarges the array by \p add elements and sets new elements to \p x.
	/**
	 *  Note: address of array entries in memory may change!
	 * @param add is the number of additional elements; \p add can be negative in order to shrink the array.
	 * @param x is the inital value of all new elements.
	 */
	void grow(INDEX add, const E &x);

	//! Enlarges the array by \p add elements.
	/**
	 *  Note: address of array entries in memory may change!
	 * @param add is the number of additional elements; \p add can be negative in order to shrink the array.
	 */
	void grow(INDEX add);

	//! Resizes (enlarges or shrinks) the array to hold \p newSize elements and sets new elements to \p x.
	/**
	 *  Note: address of array entries in memory may change!
	 * @param newSize is new size of the array
	 * @param x is the inital value of all new elements.
	 */
	void resize(INDEX newSize, const E &x) { grow(newSize - size(), x); }

	//! Resizes (enlarges or shrinks) the array to hold \p newSize elements.
	/**
	 *  Note: address of array entries in memory may change!
	 * @param newSize is new size of the array
	 */
	void resize(INDEX newSize) { grow(newSize - size()); }

	//! Assignment operator.
	Array<E,INDEX> &operator=(const Array<E,INDEX> &A) {
		deconstruct();
		copy(A);
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * Array \p A is empty afterwards.
	 */
	Array<E,INDEX> &operator=(Array<E,INDEX> &&A) {
		deconstruct();

		m_vpStart = A.m_vpStart;
		m_pStart  = A.m_pStart;
		m_pStop   = A.m_pStop;
		m_low     = A.m_low;
		m_high    = A.m_high;

		A.construct(0,-1);
		return *this;
	}

	//! @}
	//! @name Comparison
	//! @{

	//! Equality operator.
	bool operator==(const Array<E, INDEX> &L) const {
		if (size() != L.size()) {
			return false;
		}

		auto thisIterator = begin();
		auto thatIterator = L.begin();

		while (thisIterator != end() && thatIterator != L.end()) {
			if (*thisIterator != *thatIterator) {
				return false;
			}
			++thisIterator;
			++thatIterator;
		}

		OGDF_ASSERT(thisIterator == end() && thatIterator == L.end());
		return true;
	}

	//! Inequality operator.
	bool operator!=(const Array<E, INDEX> &L) const {
		return !operator==(L);
	}

	//! @}
	/**
	 * @name Reordering
	 * These following methods change the order of elements in the array.
	 */
	//@{

	//! Swaps the elements at position \p i and \p j.
	void swap(INDEX i, INDEX j) {
		OGDF_ASSERT(m_low <= i);
		OGDF_ASSERT(i <= m_high);
		OGDF_ASSERT(m_low <= j);
		OGDF_ASSERT(j <= m_high);

		std::swap(m_vpStart[i], m_vpStart[j]);
	}

	//! Randomly permutes the subarray with index set [\p l..\p r].
	void permute(INDEX l, INDEX r) {
		std::minstd_rand rng(randomSeed());
		permute(l, r, rng);
	}

	//! Randomly permutes the array.
	void permute() {
		permute(low(), high());
	}

	/**
	 * Randomly permutes the subarray with index set [\p l..\p r] using random number generator \p rng.
	 * @param l left border
	 * @param r right border
	 * @param rng random number generator
	 */
	template<class RNG>
	void permute(INDEX l, INDEX r, RNG &rng);

	/**
	 * Randomly permutes the array using random number generator \p rng.
	 * @param rng random number generator
	 */
	template<class RNG>
	void permute(RNG &rng) {
		if(!empty()) {
			permute(low(), high(), rng);
		}
	}


	//@}
	/**
	 * @name Searching and sorting
	 * These methods provide searching for values and sorting the array.
	 */
	//@{

	//! Performs a binary search for element \p e.
	/**
	 * \pre The array must be sorted!
	 * \return the index of the found element, and low()-1 if not found.
	 */
	inline int binarySearch (const E& e) const {
		return binarySearch(low(), high(), e, StdComparer<E>());
	}

	//! Performs a binary search for element \p e within the array section [\p l, ..., \p r] .
	/**
	 * \pre The array must be sorted!
	 * \return the index of the found element, and low()-1 if not found.
	 */
	inline int binarySearch (INDEX l, INDEX r, const E& e) const {
		return binarySearch(l, r, e, StdComparer<E>());
	}

	//! Performs a binary search for element \p e with comparer \p comp.
	/**
	 * \pre The array must be sorted according to \p comp!
	 * \return the index of the found element, and low()-1 if not found.
	 */
	template<class COMPARER>
	inline int binarySearch(const E& e, const COMPARER &comp) const {
		return binarySearch(low(), high(), e, comp);
	}

	//! Performs a binary search for element \p e within the array section [\p l, ..., \p r] with comparer \p comp.
	/**
	 * \pre The array must be sorted according to \p comp!
	 * \return the index of the found element, and low()-1 if not found.
	 */
	template<class COMPARER>
	INDEX binarySearch(INDEX l, INDEX r, const E& e, const COMPARER &comp) const {
		if(r<l) return low()-1;
		while(r>l) {
			INDEX m = (r + l)/2;
			if(comp.greater(e, m_vpStart[m]))
				l = m+1;
			else
				r = m;
		}
		return comp.equal(e, m_vpStart[l]) ? l : low()-1;
	}
	//! Performs a linear search for element \p e.
	/**
	 * Warning: This method has linear running time!
	 * Note that the linear search runs from back to front.
	 * \return the index of the found element, and low()-1 if not found.
	 */
	inline INDEX linearSearch (const E& e) const {
		int i;
		for(i = size(); i-- > 0; )
			if(e == m_pStart[i]) break;
		return i+low();	}

	//! Performs a linear search for element \p e with comparer \p comp.
	/**
	 * Warning: This method has linear running time!
	 * Note that the linear search runs from back to front.
	 * \return the index of the found element, and low()-1 if not found.
	 */
	template<class COMPARER>
	INDEX linearSearch(const E& e, const COMPARER &comp) const {
		int i;
		for(i = size(); i-- > 0; )
			if(comp.equal(e, m_pStart[i])) break;
		return i+low();
	}

	//! Sorts array using Quicksort.
	inline void quicksort() {
		quicksort(StdComparer<E>());
	}

	//! Sorts subarray with index set [\p l, ..., \p r] using Quicksort.
	inline void quicksort(INDEX l, INDEX r) {
		quicksort(l, r, StdComparer<E>());
	}

	//! Sorts array using Quicksort and a user-defined comparer \p comp.
	/**
	 * @param comp is a user-defined comparer; it must be a class providing a \c less(x,y) method.
	 */
	template<class COMPARER>
	inline void quicksort(const COMPARER &comp) {
		if(low() < high())
			quicksortInt(m_pStart,m_pStop-1,comp);
	}

	//! Sorts the subarray with index set [\p l, ..., \p r] using Quicksort and a user-defined comparer \p comp.
	/**
	 * @param l is the left-most position in the range to be sorted.
	 * @param r is the right-most position in the range to be sorted.
	 * @param comp is a user-defined comparer; it must be a class providing a \c less(x,y) method.
	 */
	template<class COMPARER>
	void quicksort(INDEX l, INDEX r, const COMPARER &comp) {
		OGDF_ASSERT(low() <= l);
		OGDF_ASSERT(l <= high());
		OGDF_ASSERT(low() <= r);
		OGDF_ASSERT(r <= high());
		if(l < r)
			quicksortInt(m_vpStart+l,m_vpStart+r,comp);
	}

	//! Removes the components listed in \p ind by shifting the remaining components to the left.
	/**
	 * The "free" positions in the array at the end remain as they are.
	 *
	 * This function is mainly used by Abacus. Other uses are supposed to be rare.
	 *
	 * Memory management of the removed components must be
	 * carefully implemented by the user of this function to avoid
	 * memory leaks.
	 *
	 * @param ind The compenents being removed from the array.
	 */
	void leftShift(ArrayBuffer<INDEX, INDEX> &ind);

	//! Removes the components listed in \p ind by shifting the remaining components to the left.
	/**
	 * The "free" positions in the array at the end are filled with \p val.
	 *
	 * Memory management of the removed components must be
	 * carefully implemented by the user of this function to avoid
	 * memory leaks.
	 *
	 * @param ind specifies the components that are removed from the array.
	 * @param val is the value used to fill the positions that become free.
	 */
	void leftShift(ArrayBuffer<INDEX, INDEX> &ind, const E& val) {
		leftShift(ind);
		fill(high()-ind.size(),high(),val);
	}

	//@}

	template<class F, class I> friend class ArrayBuffer; // for efficient ArrayBuffer::compact-method

private:
	E *m_vpStart; //!< The virtual start of the array (address of A[0]).
	E *m_pStart;  //!< The real start of the array (address of A[m_low]).
	E *m_pStop;   //!< Successor of last element (address of A[m_high+1]).
	INDEX m_low;    //!< The lowest index.
	INDEX m_high;   //!< The highest index.

	//! Allocates new array with index set [\p a, ..., \p b].
	void construct(INDEX a, INDEX b);

	//! Initializes elements with default constructor.
	void initialize();

	//! Initializes elements with \p x.
	void initialize(const E &x);

	//! Initializes elements from given initializer list \p initList.
	void initialize(std::initializer_list<E> initList);

	//! Deallocates array.
	void deconstruct();

	//! Constructs a new array which is a copy of \p A.
	void copy(const Array<E,INDEX> &A);

	//! Used by grow() to enlarge the array.
	void expandArray(INDEX add);

	//! Internal Quicksort implementation with comparer template.
	template<class COMPARER>
	static void quicksortInt(E *pL, E *pR, const COMPARER &comp) {
		size_t s = pR-pL;

		// use insertion sort for small instances
		if (s < maxSizeInsertionSort) {
			for (E *pI = pL+1; pI <= pR; pI++) {
				E v = *pI;
				E *pJ = pI;
				while (--pJ >= pL && comp.less(v,*pJ)) {
					*(pJ+1) = *pJ;
				}
				*(pJ+1) = v;
			}
			return;
		}

		E *pI = pL, *pJ = pR;
		E x = *(pL+(s>>1));

		do {
			while (comp.less(*pI,x)) pI++;
			while (comp.less(x,*pJ)) pJ--;
			if (pI <= pJ) std::swap(*pI++,*pJ--);
		} while (pI <= pJ);

		if (pL < pJ) quicksortInt(pL,pJ,comp);
		if (pI < pR) quicksortInt(pI,pR,comp);
	}

	OGDF_NEW_DELETE
};

// enlarges storage for array and moves old entries
template<class E, class INDEX>
void Array<E, INDEX>::expandArray(INDEX add)
{
	INDEX sOld = size(), sNew = sOld + add;

	// expand allocated memory block
	if (m_pStart != nullptr) {
		// if the element type is trivially copiable, just use realloc
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 5
		// g++ 4.8/4.9 does not have is_trivially_copyable, but
		// clang 3.5 (which is also __GNUC__ < 5) has it
		if (std::has_trivial_copy_assign<E>::value) {
#else
		if (std::is_trivially_copyable<E>::value) {
#endif
			E *p = static_cast<E *>( realloc(m_pStart, sNew*sizeof(E)) );
			if (p == nullptr) OGDF_THROW(InsufficientMemoryException);
			m_pStart = p;

		// otherwise allocate new block, move elements, and free old block
		} else {
			E *p = static_cast<E *>( malloc(sNew*sizeof(E)) );
			if (p == nullptr) OGDF_THROW(InsufficientMemoryException);

			for (int i = 0; i < min(sOld,sNew); ++i) {
				new (&p[i]) E(std::move(m_pStart[i]));
			}

			deconstruct();
			m_pStart = p;
		}

	} else {
		m_pStart = static_cast<E *>( malloc(sNew*sizeof(E)) );
		if (m_pStart == nullptr) OGDF_THROW(InsufficientMemoryException);
	}

	m_vpStart = m_pStart - m_low;
	m_pStop = m_pStart + sNew;
	m_high += add;
}

// enlarges array by add elements and sets new elements to x
template<class E, class INDEX>
void Array<E,INDEX>::grow(INDEX add, const E &x)
{
	if(add == 0) return;

	INDEX sOld = size();
	expandArray(add);

	// initialize new array entries
	for (E *pDest = m_pStart+sOld; pDest < m_pStop; pDest++)
		new (pDest) E(x);
}

// enlarges array by add elements (initialized with default constructor)
template<class E, class INDEX>
void Array<E,INDEX>::grow(INDEX add)
{
	if(add == 0) return;

	INDEX sOld = size();
	expandArray(add);

	// initialize new array entries
	for (E *pDest = m_pStart+sOld; pDest < m_pStop; pDest++)
		new (pDest) E;
}

template<class E, class INDEX>
void Array<E,INDEX>::construct(INDEX a, INDEX b)
{
	m_low = a; m_high = b;
	INDEX s = b-a+1;

	if (s < 1) {
		m_pStart = m_vpStart = m_pStop = nullptr;

	} else {
		m_pStart = static_cast<E *>( malloc(s*sizeof(E)) );
		if (m_pStart == nullptr) OGDF_THROW(InsufficientMemoryException);

		m_vpStart = m_pStart - a;
		m_pStop = m_pStart + s;
	}
}


template<class E, class INDEX>
void Array<E,INDEX>::initialize()
{
	E *pDest = m_pStart;
	try {
		for (; pDest < m_pStop; pDest++)
			new(pDest) E;
	} catch (...) {
		while(--pDest >= m_pStart)
			pDest->~E();
		free(m_pStart);
		throw;
	}
}


template<class E, class INDEX>
void Array<E,INDEX>::initialize(const E &x)
{
	E *pDest = m_pStart;
	try {
		for (; pDest < m_pStop; pDest++)
			new(pDest) E(x);
	} catch (...) {
		while(--pDest >= m_pStart)
			pDest->~E();
		free(m_pStart);
		throw;
	}
}


template<class E, class INDEX>
void Array<E, INDEX>::initialize(std::initializer_list<E> initList)
{
	E *pDest = m_pStart;
	try {
		for (const E &x : initList)
			new(pDest++) E(x);
	}
	catch (...) {
		while (--pDest >= m_pStart)
			pDest->~E();
		free(m_pStart);
		throw;
	}
}


template<class E, class INDEX>
void Array<E,INDEX>::deconstruct()
{
	if (!std::is_trivially_destructible<E>::value) {
		for (E *pDest = m_pStart; pDest < m_pStop; pDest++)
			pDest->~E();
	}
	free(m_pStart);
}


template<class E, class INDEX>
void Array<E,INDEX>::copy(const Array<E,INDEX> &array2)
{
	construct(array2.m_low, array2.m_high);

	if (m_pStart != nullptr) {
		E *pSrc = array2.m_pStop;
		E *pDest = m_pStop;
		while(pDest > m_pStart)
#if 0
			*--pDest = *--pSrc;
#endif
			new (--pDest) E(*--pSrc);
	}
}


// permutes array a from a[l] to a[r] randomly
template<class E, class INDEX>
template<class RNG>
void Array<E,INDEX>::permute (INDEX l, INDEX r, RNG &rng)
{
	OGDF_ASSERT(low() <= l);
	OGDF_ASSERT(l <= high());
	OGDF_ASSERT(low() <= r);
	OGDF_ASSERT(r <= high());

	std::uniform_int_distribution<int> dist(0,r-l);

	E *pI = m_vpStart+l, *pStart = m_vpStart+l, *pStop = m_vpStart+r;
	while(pI <= pStop)
		std::swap( *pI++, *(pStart + dist(rng)) );
}


 //! Prints array \p a to output stream \p os using delimiter \p delim.
template<class E, class INDEX>
void print(std::ostream &os, const Array<E,INDEX> &a, char delim = ' ')
{
	for (int i = a.low(); i <= a.high(); i++) {
		if (i > a.low()) os << delim;
		os << a[i];
	}
}


//! Prints array \p a to output stream \p os.
template<class E, class INDEX>
std::ostream &operator<<(std::ostream &os, const ogdf::Array<E,INDEX> &a)
{
	print(os,a);
	return os;
}

}

#include <ogdf/basic/ArrayBuffer.h>

namespace ogdf {

//! shift all items up to the last element of \p ind to the left
template<class E, class INDEX>
void Array<E,INDEX>::leftShift(ArrayBuffer<INDEX, INDEX> &ind) {
	const INDEX nInd = ind.size();
	if (nInd == 0) return;

	OGDF_ASSERT(ind[0] >= low());
	OGDF_ASSERT(ind[0] <= high());

	INDEX j, current = ind[0];
	for (INDEX i = 0; i < nInd - 1; i++) {
		OGDF_ASSERT(ind[i+1] >= low());
		OGDF_ASSERT(ind[i+1] <= high());

		const INDEX last = ind[i+1];
		for(j = ind[i]+1; j < last; j++)
			m_vpStart[current++] = m_vpStart[j];
	}

	//! copy the rest of the buffer
	for (j = ind[nInd - 1]+1; j < size(); j++)
		m_vpStart[current++] = m_vpStart[j];
}

template<class E, class INDEX>
Array<E,INDEX>::Array(const ArrayBuffer<E, INDEX> &A) {
	construct(0,-1);
	A.compactCopy(*this);
}

}
