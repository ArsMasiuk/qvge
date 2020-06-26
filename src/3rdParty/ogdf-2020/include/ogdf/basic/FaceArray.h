/** \file
 * \brief declaration and implementation of FaceArray class
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

#include <ogdf/basic/Array.h>
#include <ogdf/basic/CombinatorialEmbedding.h>


namespace ogdf {


//! Abstract base class for face arrays.
/**
 * Defines the interface for event handling used by the
 * CombinatorialEmbedding class.
 * Use the parameterized class FaceArray for creating face arrays.
 */
class FaceArrayBase {
	/**
	 * Pointer to list element in the list of all registered face
	 * arrays which references this array.
	 */
	ListIterator<FaceArrayBase*> m_it;

public:
	const ConstCombinatorialEmbedding *m_pEmbedding; //!< The associated combinatorial embedding.

	//! Initializes a face array not associated with a combinatorial embedding.
	FaceArrayBase() : m_pEmbedding(nullptr) { }

	//! Initializes a face array associated with \p pE.
	explicit FaceArrayBase(const ConstCombinatorialEmbedding *pE) : m_pEmbedding(pE) {
		if(pE) m_it = pE->registerArray(this);
	}

	//! Moves face array \p base to this face array.
	FaceArrayBase(FaceArrayBase &base) : m_it(base.m_it), m_pEmbedding(base.m_pEmbedding) {
		if(m_pEmbedding) m_pEmbedding->moveRegisterArray(m_it, this);
		base.m_pEmbedding = nullptr;
		base.m_it         = ListIterator<FaceArrayBase*>();
	}

	// destructor, unregisters the array
	virtual ~FaceArrayBase() {
		if (m_pEmbedding) m_pEmbedding->unregisterArray(m_it);
	}

	// event interface used by CombinatorialEmbedding
	//! Virtual function called when table size has to be enlarged.
	virtual void enlargeTable(int newTableSize) = 0;
	//! Virtual function called when table has to be reinitialized.
	virtual void reinit(int initTableSize) = 0;

	//! Associates the array with a new combinatorial embedding.
	void reregister(const ConstCombinatorialEmbedding *pE) {
		if (m_pEmbedding) m_pEmbedding->unregisterArray(m_it);
		if ((m_pEmbedding = pE) != nullptr) m_it = pE->registerArray(this);
	}

	//! Moves array registration from \p base to this array.
	void moveRegister(FaceArrayBase &base) {
		if (m_pEmbedding) m_pEmbedding->unregisterArray(m_it);
		m_pEmbedding = base.m_pEmbedding;
		m_it         = base.m_it;
		base.m_pEmbedding = nullptr;
		base.m_it         = ListIterator<FaceArrayBase*>();
		if (m_pEmbedding != nullptr)
			m_pEmbedding->moveRegisterArray(m_it, this);
	}
};

//! Dynamic arrays indexed with faces of a combinatorial embedding.
/**
 * @ingroup graph-containers
 *
 * Face arrays represent a mapping from faces to data of type \a T.
 * They adjust their table size automatically when the number of faces in the
 * corresponding combinatorial embedding increases.
 *
 * @warn_undef_behavior_array
 *
 * @tparam T is the element type.
 */
template<class T> class FaceArray : private Array<T>, protected FaceArrayBase {
	T m_x; //!< The default value for array elements.

public:
	//! The type for array keys.
	using key_type = face;
	//! The type for array entries.
	using value_type = T;

	//! The type for face array iterators.
	using iterator = internal::GraphArrayIterator<FaceArray<T>>;
	//! The type for face array const iterators.
	using const_iterator = internal::GraphArrayConstIterator<FaceArray<T>>;


	//! Constructs an empty face array associated with no combinatorial embedding.
	FaceArray() : Array<T>(), FaceArrayBase() { }

	//! Constructs a face array associated with \p E.
	FaceArray(const ConstCombinatorialEmbedding &E) :
		Array<T>(E.faceArrayTableSize()), FaceArrayBase(&E) { }

	//! Constructs a face array associated with \p E.
	/**
	 * @param E is the associated combinatorial embedding.
	 * @param x is the default value for all array elements.
	 */
	FaceArray(const ConstCombinatorialEmbedding &E, const T &x) :
		Array<T>(0,E.faceArrayTableSize()-1,x), FaceArrayBase(&E), m_x(x) { }

	//! Constructs an face array that is a copy of \p A.
	/**
	 * Associates the array with the same combinatorial embedding as
	 * \p A and copies all elements.
	 */
	FaceArray(const FaceArray<T> &A) : Array<T>(A), FaceArrayBase(A.m_pEmbedding), m_x(A.m_x) { }

	//! Constructs a face array containing the elements of \p A (move semantics).
	/**
	 * Face array \p A is empty afterwards and not associated with any combinatorial embedding.
	 */
	FaceArray(FaceArray<T> &&A) : Array<T>(std::move(A)), FaceArrayBase(A), m_x(A.m_x) { }


	/**
	 * @name Access methods
	 * These methods provide access to elements, size, and corresponding graph.
	 */
	//@{

	//! Returns true iff the array is associated with a combinatorial embedding.
	bool valid() const { return Array<T>::low() <= Array<T>::high(); }

	//! Returns a pointer to the associated combinatorial embedding.
	const ConstCombinatorialEmbedding *embeddingOf() const {
		return m_pEmbedding;
	}

	//! Returns a reference to the element with index \p f.
	const T &operator[](face f) const {
		OGDF_ASSERT(f != nullptr);
		OGDF_ASSERT(f->embeddingOf() == m_pEmbedding);
		return Array<T>::operator [](f->index());
	}

	//! Returns a reference to the element with index \p f.
	T &operator[](face f) {
		OGDF_ASSERT(f != nullptr);
		OGDF_ASSERT(f->embeddingOf() == m_pEmbedding);
		return Array<T>::operator [](f->index());
	}

	//! Returns a reference to the element with index \p f.
	const T &operator()(face f) const {
		OGDF_ASSERT(f != nullptr);
		OGDF_ASSERT(f->embeddingOf() == m_pEmbedding);
		return Array<T>::operator [](f->index());
	}

	//! Returns a reference to the element with index \p f.
	T &operator()(face f) {
		OGDF_ASSERT(f != nullptr);
		OGDF_ASSERT(f->embeddingOf() == m_pEmbedding);
		return Array<T>::operator [](f->index());
	}


	//@}
	/**
	 * @name Iterators
	 * These methods return bidirectional iterators to elements in the array.
	 */
	//@{

	//! Returns an iterator to the first entry in the face array.
	/**
	 * If the face array is empty, a null pointer iterator is returned.
	 */
	iterator begin() { return iterator(m_pEmbedding->firstFace(), this); }

	//! Returns a const iterator to the first entry in the face array.
	/**
	 * If the face array is empty, a null pointer iterator is returned.
	 */
	const_iterator begin() const { return const_iterator(m_pEmbedding->firstFace(), this); }

	//! Returns a const iterator to the first entry in the face array.
	/**
	 * If the face array is empty, a null pointer iterator is returned.
	 */
	const_iterator cbegin() const { return const_iterator(m_pEmbedding->firstFace(), this); }

	//! Returns an iterator to one-past-last entry in the face array.
	/**
	 * This is always a null pointer iterator.
	 */
	iterator end() { return iterator(nullptr, this); }

	//! Returns a const iterator to one-past-last entry in the face array.
	/**
	 * This is always a null pointer iterator.
	 */
	const_iterator end() const { return const_iterator(nullptr, this); }

	//! Returns a const iterator to one-past-last entry in the face array.
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

	//! Reinitializes the array. Associates the array with no combinatorial embedding.
	void init() {
		Array<T>::init(); reregister(nullptr);
	}

	//! Reinitializes the array. Associates the array with \p E.
	void init(const ConstCombinatorialEmbedding &E) {
		Array<T>::init(E.faceArrayTableSize()); reregister(&E);
	}

	//! Reinitializes the array. Associates the array with \p E.
	/**
	 * @param E is the associated combinatorial embedding.
	 * @param x is the default value.
	 */
	void init(const ConstCombinatorialEmbedding &E, const T &x) {
		Array<T>::init(0,E.faceArrayTableSize()-1, m_x = x); reregister(&E);
	}

	//! Sets all array elements to \p x.
	void fill(const T &x) {
		int high = m_pEmbedding->maxFaceIndex();
		if(high >= 0)
			Array<T>::fill(0,high,x);
	}

	//! Assignment operator.
	FaceArray<T> &operator=(const FaceArray<T> &a) {
		Array<T>::operator =(a);
		m_x = a.m_x;
		reregister(a.m_pEmbedding);
		return *this;
	}

	//! Assignment operator (move semantics).
	/**
	 * Face array \p a is empty afterwards and not associated with any combinatorial embedding.
	 */
	FaceArray<T> &operator=(FaceArray<T> &&a) {
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

	OGDF_NEW_DELETE
};

}
