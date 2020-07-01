/*!\file
 * \author Matthias Elf
 * \brief ring.
 *
 * \par License:
 * This file is part of ABACUS - A Branch And CUt System
 * Copyright (C) 1995 - 2003
 * University of Cologne, Germany
 *
 * \par
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * \par
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * \see http://www.gnu.org/copyleft/gpl.html
 */

#pragma once

#include <ogdf/lib/abacus/abacusroot.h>


namespace abacus {

template <class Type>
class  AbaRing;

template <class Type>
std::ostream &operator<< (std::ostream &out, const AbaRing<Type> &ring);


//! Bounded circular lists.
/**
 * The template AbaRing implements a bounded circular list with
 * the property that if the list is full and an element is inserted
 * the oldest element of the ring is removed. With this implementation
 * single elements cannot be removed, but the whole AbaRing can be cleared.
 */
template <class Type> class  AbaRing :  public AbacusRoot  {
public:

	//! The constructor.
	/**
	 * \param size The length of the ring.
	 */
	explicit AbaRing(int size);

	//! The destructor.
	virtual ~AbaRing() { }

	//! \brief The output operator
	/**
	 * Writes the elements of the ring to an output
	 * stream starting with the oldest element in the ring.
	 *
	 * \param out  The output stream.
	 * \param ring The ring being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<< <> (std::ostream &out, const AbaRing<Type> &ring);

	//! Returns the <i>i</i>-th element of the ring.
	/**
	 * The operation is undefined if no element has been inserted in the <i>i</i>-th position so far.
	 *
	 * \param i The element being accessed.
	 */
	Type& operator[](int i);

	//! The operator [] is overloaded for constant use.
	const Type& operator[](int i) const;

	//! Inserts a new element into the ring.
	/**
	 * If the ring is already full, this operation overwrites the oldest
	 * element in the ring.
	 *
	 * \param elem The element being inserted.
	 */
	void insert(Type elem);

	//! Empties the ring.
	void clear();

	//! Returns the size of the ring.
	int  size() const;

	//! Returns the current number of elements in the ring.
	int  number() const;

	//! Returns the oldest element in the ring.
	/**
	 * The result is undefined, if the ring is empty.
	 */
	Type oldest() const;

	//! Returns the index of the oldest element in the ring.
	/**
	 * The result is undefined, if the ring is empty.
	 */
	int  oldestIndex() const;

	//! Returns the newest element in the ring.
	/**
	 * The result is undefined if the ring is empty.
	 */
	Type newest() const;

	//! Returns the index of the newest element in the ring.
	/**
	 * The result is undefined if the ring is empty.
	 */
	int  newestIndex() const;

	//! Can be used to access any element between the oldest and newest inserted element.
	/**
	 * \param i The element \a i elements before the newest element is
	 *          retrieved. If \a i is 0, then the function retrieves the newest element.
	 * \param p Contains the \a i-th element before the newest one in
	 *          a successful call.
	 *
	 * \return 0 If there are enough elements in the ring such that
	 *           the element \a i entries before the newest one could be accessed,
	 * \return 1 otherwise.
	 */
	int  previous(int i, Type &p) const;

	//! Returns true if no element is contained in the ring, false otherwise.
	bool empty() const;

	//! Returns true If the ring is completely filled up, false otherwise.
	bool filled() const;

	//! Changes the length of the ring.
	/**
	 * \param newSize The new length of the ring.
	 *                If the ring decreases below the current number of elements in the
	 *                ring, then the \a newSize newest elements stay in the ring.
	 */
	void realloc(int newSize);

private:

	//! An array storing the elements of the ring.
	Array<Type> ring_;

	//! The position in the array \a ring_ where the next element will be inserted.
	int head_;

	//! This member becomes \a true if ring is completely filled up.
	bool filled_;
};

}

#include <ogdf/lib/abacus/ring.inc>
