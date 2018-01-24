/*!\file
 * \author Matthias Elf
 *
 * \brief bounded priority queue.
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

#include <ogdf/lib/abacus/bheap.h>

namespace abacus {

//! Bounded priority queues.
/**
 * A priority queue is a data structure storing a set of elements.
 * Each element has a key which must be an ordered data type.
 * The most important operations are the insertion
 * of an element, the determination of the element having the minimal
 * key, and the deletion of the element having minimal key.
 *
 * Since the priority queue is implemented by a heap (class AbaBHeap)
 * the insertion of a new element and the deletion of the minimal
 * element require O(log n) time if n elements are
 * stored in the priority queue. The element having minimal key
 * can be determined in constant time.
 *
 * To provide an efficient implementation the priority queue is
 * bounded, i.e., the maximal number of elements is an argument
 * of the constructor. However, if required, later a reallocation
 * can be performed.
 */
template<class Type, class Key>
class  AbaPrioQueue :  public AbacusRoot  {
public:

	//! The constructor of an empty priority queue.
	/**
	 * \param size The maximal number of elements the priority queue
	 *             can hold without reallocation.
	 */
	AbaPrioQueue(int size);

	//! Inserts an element in the priority queue.
	/**
	 * \param elem The element being inserted.
	 * \param key  The key of the element.
	 */
	void insert(Type elem, Key key);

	//! Retrieves the element with minimal key from the priority queue.
	/**
	 * \param min If the priority queue is non-empty the minimal element is
	 *            assigned to \a min.
	 *
	 * \return 0 If the priority queue is non-empty, 1 otherwise.
	 */
	int getMin(Type &min) const;

	//! Retrieves the key of the minimal element in the priority queue.
	/**
	 * \param minKey Holds after the call the key of the minimal element in
	 *               the priority queue, if the queue is non-emtpy.
	 *
	 * \return 0 If the priority queue is non-empty, 1 otherwise.
	 */
	int getMinKey(Key &minKey) const;

	//! Retrieves and removes the minimal element from the priority queue.
	/**
	 * \return 0 If the priority queue is non-empty, 1 otherwise.
	 *
	 * \param min If the priority queue is non-empty the minimal element is
	 *            assigned to \a min.
	 */
	int extractMin(Type &min);

	//! Makes the priority queue empty.
	void clear();

	//! Returns the maximal number of elements which can be stored in the priority queue.
	int size() const;

	//! Returns the number of elements stored in the priority queue.
	int number() const;

	//! Increases the size of the priority queue.
	/**
	 * It is not allowed to decrease the size of the priority queue.
	 * In this case an error message is output and the program stops.
	 *
	 * \param newSize The new size of the priority queue.
	 */
	void realloc(int newSize);

private:

	//! The heap implementing the priority queue.
	AbaBHeap<Type, Key>  heap_;
};

}

#include <ogdf/lib/abacus/bprioqueue.inc>
