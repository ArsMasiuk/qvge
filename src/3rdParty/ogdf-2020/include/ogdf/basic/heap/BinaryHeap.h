/** \file
 * \brief Implementation of binary heap class that allows the
 * decreaseT operation.
 *
 * \author Karsten Klein, Tilo Wiedera
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

#include <ogdf/basic/heap/HeapBase.h>

namespace ogdf {

/**
 * \brief Heap realized by a data array.
 * This heap implementation does not support merge operations.
 *
 * @tparam T Denotes value type of inserted elements.
 * @tparam C Denotes comparison functor determining value ordering.
 */
template<
  typename T,
  typename C=std::less<T>
>
class BinaryHeap : public HeapBase<BinaryHeap<T, C>, int, T, C>
{

	using base_type = HeapBase<BinaryHeap<T, C>, int, T, C>;

public:

	/**
	 * Initializes an empty binary heap.
	 *
	 * @param comp Comparison functor determining value ordering.
	 * @param initialSize The intial capacity of this heap. Used to allocate an adequate amount of memory.
	 */
	explicit BinaryHeap(const C &comp = C(), int initialSize = 128);

	virtual ~BinaryHeap() {
		if (m_heapArray) {
			for(int i = 1; i <= m_size; i++) {
				delete m_heapArray[i].handle;
			}

			delete[] m_heapArray;
		}
	}

	/**
	 * Returns the topmost value in the heap.
	 *
	 * @return the topmost value
	 */
	const T &top() const override;

	/**
	 * Inserts a value into the heap.
	 *
	 * @param value The value to be inserted
	 * @return A handle to access and modify the value
	 */
	int *push(const T &value) override;

	/**
	 * Removes the topmost value from the heap.
	 */
	void pop() override;

	/**
	 * Decreases a single value.
	 *
	 * @param handle The handle of the value to be decreased
	 * @param value The decreased value. This must be less than the former value
	 */
	void decrease(int *handle, const T &value) override;

	/**
	 * Returns the value of that handle.
	 *
	 * @param handle The handle
	 * @return The value
	 */
	const T &value(int *handle) const override;

	//! Returns the current size.
	int capacity() const { return m_arraySize; }

	//! Returns the number of stored elements.
	int size() const { return m_size; }

	//! Returns true iff the heap is empty.
	bool empty() const { return m_size == 0; }

	//! Reinitializes the data structure.
	/**
	 * Deletes the array and reallocates it with size that was passed at
	 * construction time.
	 */
	void clear();

private:

	//! Establishes heap property by moving element up in heap if necessary.
	void siftUp(int pos);

	//! Establishes heap property by moving element down in heap if necessary.
	void siftDown(int pos);

	//! Array index of parent node.
	int parentIndex(int num)
	{
		OGDF_ASSERT(num > 0);
		return num / 2;
	}

	//! Array index of left child.
	int leftChildIndex(int num)
	{
		OGDF_ASSERT(num > 0);
		return 2 * num;
	}

	//! Array index of right child.
	int rightChildIndex(int num)
	{
		OGDF_ASSERT(num > 0);
		return 2*num + 1;
	}

	//! Returns true if left child exists.
	bool hasLeft(int num)
	{
		OGDF_ASSERT(num > 0);
		return leftChildIndex(num) <= m_size;
	}

	//! Returns true if right child exists.
	bool hasRight(int num)
	{
		OGDF_ASSERT(num > 0);
		return rightChildIndex(num) <= m_size;
	}

	// helper functions for internal maintenance

	int arrayBound(int arraySize) { return arraySize + 1; }
	int higherArrayBound(int arraySize) { return 2*arraySize + 1; }
	int higherArraySize(int arraySize) { return 2 * arraySize; }
	int lowerArrayBound(int arraySize) { return arraySize/2 + 1; }
	int lowerArraySize(int arraySize) { return arraySize / 2; }
	void init(int initialSize);

	struct HeapEntry {
		T value;
		int *handle = nullptr;
	};

	// dynamically maintained array of entries
	HeapEntry* m_heapArray;

	// number of elements stored in heap
	int m_size;

	// current capacity of the heap
	int m_arraySize;

	// used to check reallocation bound
	int m_initialSize;
};


template <typename T, typename C>
const T &BinaryHeap<T, C>::top() const {
	int firstIndex = 1;
	return value(&firstIndex);
}

template <typename T, typename C>
int *BinaryHeap<T, C>::push(const T &value) {
	OGDF_ASSERT((m_size) < m_arraySize);
	m_size++;

	// does the array size have to be adjusted ?
	if (m_size == m_arraySize) {
		HeapEntry *tempHeap = new HeapEntry[higherArrayBound(m_arraySize)];

		// last one is not occupied yet
		for (int i = 1; i <= m_arraySize; i++) {
			tempHeap[i] = m_heapArray[i];
		}

		delete[] m_heapArray;
		m_heapArray = tempHeap;
		m_arraySize = higherArraySize(m_arraySize);
	}

	// actually insert object and reestablish heap property
	HeapEntry &entry = m_heapArray[m_size];
	entry.value = value;
	entry.handle = new int;
	*(entry.handle) = m_size;

	int *result = entry.handle;

	OGDF_ASSERT(*result == *(m_heapArray[*result].handle));

	siftUp(m_size);

	OGDF_ASSERT(*result == *(m_heapArray[*result].handle));

	return result;
}

template <typename T, typename C>
void BinaryHeap<T, C>::pop() {
	OGDF_ASSERT(!empty());
	delete m_heapArray[1].handle;
	m_size--;

	if (m_size > 0) {
		// former last leaf
		m_heapArray[1] = m_heapArray[m_size+1];

		// check if reallocation is possible
		// TODO make reallocation threshold configurable?
		if (m_size < m_arraySize / 3 && m_arraySize > 2*m_initialSize - 1) {
			HeapEntry* tempHeap = new HeapEntry[lowerArrayBound(m_arraySize)];

			for (int i = 1; i <= m_size ; i++) {
				tempHeap[i] = m_heapArray[i];
			}

			delete[] m_heapArray;
			m_heapArray = tempHeap;
			m_arraySize = lowerArraySize(m_arraySize);
		}

		siftDown(1);
	}
}

template <typename T, typename C>
void BinaryHeap<T, C>::decrease(int *handle, const T &value) {
	HeapEntry& he = m_heapArray[*handle];
	OGDF_ASSERT(this->comparator()(value, he.value));

	he.value = value;
	siftUp(*handle);
}

template <typename T, typename C>
const T &BinaryHeap<T, C>::value(int *handle) const {
	OGDF_ASSERT(handle != nullptr);
	OGDF_ASSERT(*handle > 0);
	OGDF_ASSERT(*handle <= m_size);

	return m_heapArray[*handle].value;
}

template <typename T, typename C>
BinaryHeap<T, C>::BinaryHeap(const C &comp, int initialSize)
: base_type(comp)
{
	init(initialSize);
}

template <typename T, typename C>
void BinaryHeap<T, C>::init(int initialSize)
{
	m_arraySize = initialSize;
	m_initialSize = initialSize;
	m_size = 0;

	// create an array of HeapEntry Elements
	// start at 1
	m_heapArray = new HeapEntry[arrayBound(m_arraySize)];
}

template <typename T, typename C>
void BinaryHeap<T, C>::clear() {
	for(int i = 1; i <= m_size; i++) {
		delete m_heapArray[i].handle;
	}

	delete[] m_heapArray;
	init(m_initialSize);
}

template <typename T, typename C>
void BinaryHeap<T, C>::siftUp(int pos)
{
	OGDF_ASSERT(pos > 0);
	OGDF_ASSERT(pos <= m_size);

	if (pos == 1) {
		*(m_heapArray[1].handle) = 1;
	} else {
		HeapEntry tempEntry = m_heapArray[pos];
		int run = pos;

		while ((parentIndex(run) >= 1) &&
				this->comparator()(tempEntry.value, m_heapArray[parentIndex(run)].value)) {
			m_heapArray[run] = m_heapArray[parentIndex(run)];
			*(m_heapArray[run].handle) = run;

			run = parentIndex(run);
		}

		m_heapArray[run] = tempEntry;
		*(m_heapArray[run].handle) = run;
	}
}

template <typename T, typename C>
void BinaryHeap<T, C>::siftDown(int pos)
{
	OGDF_ASSERT(pos > 0);
	OGDF_ASSERT(pos <= m_size);

	// is leaf?
	if (pos >= m_size/2 + 1) {
		*(m_heapArray[pos].handle) = pos;
		// leafs cant move further down
	} else {
		T value = m_heapArray[pos].value;
		int sIndex = pos;
		C const &compare = this->comparator();

		// left child is smallest child?
		if (hasLeft(pos) && compare(m_heapArray[leftChildIndex(pos)].value, value)
			&& compare(m_heapArray[leftChildIndex(pos)].value, m_heapArray[rightChildIndex(pos)].value)) {
			sIndex = leftChildIndex(pos);
		// or is right child smaller?
		} else if (hasRight(pos) && compare(m_heapArray[rightChildIndex(pos)].value, value)) {
			sIndex = rightChildIndex(pos);
		}

		// need recursive sift?
		if (sIndex != pos) {
			HeapEntry tempEntry = m_heapArray[pos];
			m_heapArray[pos] = m_heapArray[sIndex];
			m_heapArray[sIndex] = tempEntry;

			// update both index entries
			*(m_heapArray[pos].handle) = pos;
			*(m_heapArray[sIndex].handle) = sIndex;

			siftDown(sIndex);
		} else {
			*(m_heapArray[pos].handle) = pos;
		}
	}
}

}
