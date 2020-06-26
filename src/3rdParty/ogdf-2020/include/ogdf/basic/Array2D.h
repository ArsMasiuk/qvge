/** \file
 * \brief Declaration and implementation of class Array2D which implements
 * dynamic two dimensional arrays.
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


//! The parameterized class Array2D implements dynamic two-dimensional arrays.
/**
 * @ingroup containers
 *
 * @tparam E denotes the element type.
 */
template<class E> class Array2D
{
public:
	// constructors

	//! Creates a two-dimensional array with empty index set.
	Array2D() { construct(0,-1,0,-1); }

	//! Creates a two-dimensional array with index set [\p a, ..., \p b]*[\p c, ..., \p d].
	Array2D(int a, int b, int c, int d) {
		construct(a,b,c,d); initialize();
	}

	//! Creates a two-dimensional array with index set [\p a, ..., \p b]*[\p c, ..., \p d] and initailizes all elements with \p x.
	Array2D(int a, int b, int c, int d, const E &x) {
		construct(a,b,c,d); initialize(x);
	}

	//! Creates a two-dimensional array that is a copy of \p A.
	Array2D(const Array2D<E> &A) {
		copy(A);
	}

	//! Creates a two-dimensional array containing the elements of \p A (move semantics).
	/**
	 * The array \p A is empty afterwards.
	 */
	Array2D(Array2D<E> &&A)
		: m_vpStart(A.m_vpStart), m_lenDim2(A.m_lenDim2), m_pStart(A.m_pStart), m_pStop(A.m_pStop),
		  m_a(A.m_a), m_b(A.m_b), m_c(A.m_c), m_d(A.m_d)
	{
		A.construct(0,-1,0,-1);
	}

	//! Destructor
	~Array2D() {
		deconstruct();
	}

	//! Returns the minimal array index in dimension 1.
	int low1() const { return m_a; }

	//! Returns the maximal array index in dimension 1.
	int high1() const { return m_b; }

	//! Returns the minimal array index in dimension 2.
	int low2() const { return m_c; }

	//! Returns the maximal array index in dimension 2.
	int high2() const { return m_d; }

	//! Returns the size (number of elements) of the array.
	int size() const { return size1() * size2(); }

	//! Returns the length of the index interval (number of entries) in dimension 1.
	int size1() const { return m_b - m_a + 1; }

	//! Returns the length of the index interval (number of entries) in dimension 2.
	int size2() const { return m_lenDim2; }

	//! Returns the determinant of the matrix
	/*! \note use only for square matrices and floating point values */
	float det() const;

	//! Returns a reference to the element with index (\p i,\p j).
	const E &operator()(int i, int j) const {
		OGDF_ASSERT(m_a <= i);
		OGDF_ASSERT(i <= m_b);
		OGDF_ASSERT(m_c <= j);
		OGDF_ASSERT(j <= m_d);
		return m_vpStart[size_t(i-m_a)*m_lenDim2+j];
	}

	//! Returns a reference to the element with index (\p i,\p j).
	E &operator()(int i, int j) {
		OGDF_ASSERT(m_a <= i);
		OGDF_ASSERT(i <= m_b);
		OGDF_ASSERT(m_c <= j);
		OGDF_ASSERT(j <= m_d);
		return m_vpStart[size_t(i-m_a)*m_lenDim2+j];
	}

	//! Reinitializes the array to an array with empty index set.
	void init() { init(0,-1,0,-1); }

	//! Reinitializes the array to an array with index set [\p a, ..., \p b]*[\p c, ..., \p d].
	void init(int a, int b, int c, int d) {
		deconstruct();
		construct(a,b,c,d);
		initialize();
	}

	//! Reinitializes the array to an array with index set [\p a, ..., \p b]*[\p c, ..., \p d] and initializes all entries with \p x.
	void init(int a, int b, int c, int d, const E &x) {
		deconstruct();
		construct(a,b,c,d);
		initialize(x);
	}

	//! Assignment operator.
	Array2D<E> &operator=(const Array2D<E> &array2) {
		deconstruct();
		copy(array2);
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * Array \p A is empty afterwards.
	 */
	Array2D<E> &operator=(Array2D<E> &&A) {
		deconstruct();

		m_vpStart = A.m_vpStart;
		m_pStart  = A.m_pStart;
		m_pStop   = A.m_pStop;
		m_lenDim2 = A.m_lenDim2;
		m_a       = A.m_a;
		m_b       = A.m_b;
		m_c       = A.m_c;
		m_d       = A.m_d;

		A.construct(0,-1,0,-1);
		return *this;
	}

	//! Sets all elements to \p x.
	void fill(const E &x) {
		E *pDest = m_pStop;
		while(pDest > m_pStart)
			*--pDest = x;
	}

private:
	E   *m_vpStart; //!< The virtual start of the array (address of A[0,0]).
	int  m_lenDim2; //!< The  number of elements in dimension 2.
	E   *m_pStart; //!< The real start of the array (address of A[low1,low2]).
	E   *m_pStop; //!< Successor of last element (address of A[high1,high2+1]).

	int  m_a; //!< The lowest index in dimension 1.
	int  m_b; //!< The highest index in dimension 1.
	int  m_c; //!< The lowest index in dimension 2.
	int  m_d; //!< The highest index in dimension 2.

	void construct(int a, int b, int c, int d);

	void initialize();
	void initialize(const E &x);

	void deconstruct();

	void copy(const Array2D<E> &array2);
};


//! Constructs the array with index set [\p a, ..., \p b]*[\p c, ..., \p d].
template<class E>
void Array2D<E>::construct(int a, int b, int c, int d)
{
	m_a = a;
	m_b = b;
	m_c = c;
	m_d = d;

	size_t lenDim1 = b-a+1;
	m_lenDim2   = d-c+1;

	if (lenDim1 < 1 || m_lenDim2 < 1) {
		m_pStart = m_vpStart = m_pStop = nullptr;

	} else {
		size_t len = lenDim1*m_lenDim2;
		m_pStart = static_cast<E *>( malloc(len*sizeof(E)) );
		if (m_pStart == nullptr)
			OGDF_THROW(InsufficientMemoryException);

		m_vpStart = m_pStart - c;
		m_pStop   = m_pStart + len;
	}
}


//! Initializes the array with default constructor of \a E.
template<class E>
void Array2D<E>::initialize()
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


//! Initializes the array with \p x.
template<class E>
void Array2D<E>::initialize(const E &x)
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


//! Call destructor of all elements.
template<class E>
void Array2D<E>::deconstruct()
{
	if (!std::is_trivially_destructible<E>::value) {
		for (E *pDest = m_pStart; pDest < m_pStop; pDest++)
			pDest->~E();
	}
	free(m_pStart);
}

//! Copy \p array2.
template<class E>
void Array2D<E>::copy(const Array2D<E> &array2)
{
	construct(array2.m_a, array2.m_b, array2.m_c, array2.m_d);

	if (m_pStart != 0) {
		E *pSrc  = array2.m_pStop;
		E *pDest = m_pStop;
		while(pDest > m_pStart)
			new (--pDest) E(*--pSrc);
	}
}


//! Computes the determinant via row expansion.
template<class E>
float Array2D<E>::det() const
{
	// matrix must be quadratic
	OGDF_ASSERT(size1() == size2());

	int a = m_a;
	int b = m_b;
	int c = m_c;
	int d = m_d;
	int n = m_lenDim2;

	int i, j;
	int column;

	float determinant = 0.0;

	switch(n) {
	case 0:
		break;
	case 1:
		determinant = (float)((*this)(a, c));
		break;
	case 2:
		determinant = (float)((*this)(a, c) * (*this)(b, d) - (*this)(a, d) * (*this)(b, c));
		break;

		// Expanding along the first row (Laplace's Formula)
	default:
		Array2D<E> remMatrix(0, n-2, 0, n-2);             // the remaining matrix
		for(column = c; column <= d; column++) {
			int rem_i = 0;
			int rem_j = 0;
			for(i = a; i <= b; i++) {
				for(j = c; j <= d; j++) {
					if(i != a && j != column) {
						remMatrix(rem_i, rem_j) = (*this)(i, j);
						if(rem_j < n-2) {
							rem_j++;
						}
						else {
							rem_i++;
							rem_j = 0;
						}
					}
				}
			}
			determinant += pow(-1.0,(a+column)) * (*this)(a,column) * remMatrix.det();
		}
	}

	return determinant;
}

}
