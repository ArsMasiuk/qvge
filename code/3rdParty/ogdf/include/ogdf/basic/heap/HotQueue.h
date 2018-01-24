/** \file
 * \brief Implementation of Heap-on-Top data structure.
 *
 * \author Łukasz Hanuszczak
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

#include <functional>
#include <utility>
#include <vector>
#include <limits>
#include <cmath>


namespace ogdf {


//! Heap-on-Top bucket element.
template<typename V, typename P>
struct HotQueueNode {
	V value;
	P priority;

	HotQueueNode<V, P> *prev;
	HotQueueNode<V, P> *next;

	HotQueueNode()
	: prev(nullptr), next(nullptr)
	{
	}

	HotQueueNode(const V &val, const P &pr)
	: value(val), priority(pr),
	  prev(nullptr), next(nullptr)
	{
	}
};


//! Heap-on-Top handle to inserted items.
/**
 * This structure is essentially just an tagged union of either bucket handle
 * or native handle to underlying heap.
 *
 * @tparam V Denotes type of values of inserted elements.
 * @tparam P Denotes type of priorities of inserted elements.
 * @tparam HeapHandle Denotes type of handle of underlying heap.
 */
template<typename V, typename P, typename HeapHandle>
struct HotQueueHandle {
private:
	enum class Type {
		heap,
		bucket
	} type; //!< Union tag.

	union {
		//! Handle to underlying heap.
		HeapHandle heapHandle;
		//! Handle to bucket element (bucket index and list iterator).
		std::pair<std::size_t, HotQueueNode<V, P> *> bucketHandle;
	};

	//! Creates heap-type handle.
	HotQueueHandle(HeapHandle handle)
	: type(Type::heap), heapHandle(handle)
	{
	}

	//! Creates bucket-type handle.
	HotQueueHandle(std::size_t index, HotQueueNode<V, P> *queueNode)
	: type(Type::bucket), bucketHandle(index, queueNode)
	{
	}

	HotQueueHandle &operator=(const HotQueueHandle &other) {
		type = other.type;
		switch(type) {
		case Type::heap:
			heapHandle = other.heapHandle;
			break;
		case Type::bucket:
			bucketHandle = other.bucketHandle;
			break;
		}

		return *this;
	}

	template<
		typename V1, typename P1, template<typename T, typename C> class H1
	>
	friend class HotQueue;
};


//! Heap-on-Top queue implementation.
/**
 * @ingroup containers
 *
 * General idea of this implementation comes from short summary provided
 * here:
 * http://theory.stanford.edu/~amitp/GameProgramming/ImplementationNotes.html
 *
 * @tparam V Denotes type of values of inserted elements.
 * @tparam P Denotes type of priorities of inserted elements.
 * @tparam H Denotes class of underlying heap.
 */
template<typename V, typename P, template<typename T, typename C> class H>
class HotQueue {
private:
	struct HeapComparator;
	using HeapHandle = typename H<std::pair<V, P>, HeapComparator>::Handle;

	std::size_t m_size; //!< Number of total elements in the heap.

	H<std::pair<V, P>, HeapComparator> m_heap; //!< Underlying heap structure.
	std::size_t m_heapSize; //!< Size of underlying heap.

	std::vector<HotQueueNode<V, P> *> m_buckets; //!< Array of buckets.

public:
	using Handle = HotQueueHandle<V, P, HeapHandle>;

	//! Creates empty Heap-on-Top queue.
	/**
	 * @param change Maximum \em {event duration}.
	 * @param levels Number of buckets.
	 */
	HotQueue(const P &change, std::size_t levels);

	//! Releases all buckets on destruction
	~HotQueue();

	//! Returns reference to the top element in the heap.
	const V &top() const;

	//! Inserts a new node with given \p value and \p priority into a heap.
	/**
	 * @param value A value of inserted element.
	 * @param priority A priority of inserted element.
	 * @return Handle to the inserted node.
	 */
	Handle push(const V &value, const P &priority);

	//! Removes the top element from the heap.
	/**
	 * Behaviour of this function is undefined if the heap is empty.
	 */
	void pop();

	//! Decreases value of the given \p handle to \p priority.
	/**
	 * Behaviour of this function is undefined if referenced element does not
	 * belong to a the heap or new priority is incorrect.
	 *
	 * @param handle A handle to the element for which the priority is to be
	 *               decreased.
	 * @param priority A new priority for the element.
	 */
	void decrease(Handle &handle, const P &priority);

	//! Number of elements contained within the heap.
	std::size_t size() const {
		return m_size;
	}

	//! Checks whether the heap is empty.
	bool empty() const {
		return size() == 0;
	}

private:
	//! Comparator used by underlying heap.
	struct HeapComparator {
		bool operator()(const std::pair<V, P> &a, const std::pair<V, P> &b) const {
			return std::get<1>(a) < std::get<1>(b);
		}
	};

	std::size_t m_heapedBucket; //!< Index of currently heaped bucket.
	std::size_t m_lastBucket; //!< Index of highest, non-empty bucket.

	const P m_bucketSpan; //!< Length of the interval covered by each bucket.

	//! Computes bucket index of given \p priority.
	std::size_t bucketIndex(const P &priority) {
		return (std::size_t) std::round(priority / m_bucketSpan);
	}

	//! Provides access to bucket at given \p index.
	HotQueueNode<V, P> *&bucketAt(std::size_t index) {
		return m_buckets[index % m_buckets.size()];
	}

	static constexpr std::size_t NONE = std::numeric_limits<size_t>::max();
};


template<typename V, typename P, template<typename T, typename C> class H>
HotQueue<V, P, H>::HotQueue(const P &change, std::size_t levels)
: m_size(0), m_heapSize(0),
  m_buckets(levels, nullptr),
  m_heapedBucket(NONE), m_lastBucket(0),
  m_bucketSpan((int) std::round(change / (levels - 1)))
{
}

template<typename V, typename P, template<typename T, typename C> class H>
HotQueue<V, P, H>::~HotQueue()
{
	if (empty()) {
		return;
	}
	for (auto &bucket : m_buckets) {
		if (bucket != nullptr) {
			for (HotQueueNode<V, P> *it = bucket; it != nullptr;) {
				HotQueueNode<V, P> *next = it->next;
				delete it;
				it = next;
			}
		}
	}
}

template<typename V, typename P, template<typename T, typename C> class H>
inline const V &HotQueue<V, P, H>::top() const
{
	return std::get<0>(m_heap.top());
}


template<typename V, typename P, template<typename T, typename C> class H>
typename HotQueue<V, P, H>::Handle HotQueue<V, P, H>::push(
	const V &value, const P &priority)
{
	m_size++;

	std::size_t ind = bucketIndex(priority);

	if(m_heapedBucket == NONE) {
		m_heapedBucket = ind;
	}

	if(ind == m_heapedBucket) {
		m_heapSize++;
		HeapHandle handle = m_heap.push(std::make_pair(value, priority));
		return Handle(handle);
	} else {
		HotQueueNode<V, P> *queueNode = new HotQueueNode<V, P>(value, priority);

		if(bucketAt(ind) != nullptr) {
			bucketAt(ind)->prev = queueNode;
		}
		queueNode->next = bucketAt(ind);
		bucketAt(ind) = queueNode;

		m_lastBucket = std::max(m_lastBucket, ind);
		return Handle(ind, queueNode);
	}
}


template<typename V, typename P, template<typename T, typename C> class H>
void HotQueue<V, P, H>::pop()
{
	m_size--;

	m_heap.pop();
	m_heapSize--;

	/*
	 * If the heap became empty and there is non-empty bucket afterwards
	 * we need to heapify first non-empty bucket. If the heap became empty
	 * but there is no non-empty bucket afterwards it may be the case that
	 * element will be inserted into the current heap (therefore, do nothing).
	 */
	if(!(m_heapSize == 0 && m_heapedBucket != m_lastBucket)) {
		return;
	}

	// Find first non-empty bucket.
	do {
		m_heapedBucket++;
	} while(bucketAt(m_heapedBucket) == nullptr);

	// Move bucket contents into a heap.
	for(HotQueueNode<V, P> *it = bucketAt(m_heapedBucket); it != nullptr;)	{
		m_heapSize++;
		m_heap.push(std::make_pair(it->value, it->priority));

		HotQueueNode<V, P> *next = it->next;
		delete it;
		it = next;
	}
	bucketAt(m_heapedBucket) = nullptr;
}


template<typename V, typename P, template<typename T, typename C> class H>
void HotQueue<V, P, H>::decrease(
	typename HotQueue<V, P, H>::Handle &handle,
	const P &priority)
{
	switch(handle.type) {
	case Handle::Type::heap: {
		// Simple case, just use native heap decrease key.
		const HeapHandle &elem = handle.heapHandle;
		m_heap.decrease(elem, std::make_pair(
			std::get<0>(m_heap.value(elem)), priority)
		);
		break;
	}
	case Handle::Type::bucket:
		// Remove element from bucket (as with ordinary linked-list)...
		HotQueueNode<V, P> *queueNode = std::get<1>(handle.bucketHandle);
		if(queueNode->next != nullptr) {
			queueNode->next->prev = queueNode->prev;
		}
		if(queueNode->prev != nullptr) {
			queueNode->prev->next = queueNode->next;
		} else {
			m_buckets[std::get<0>(handle.bucketHandle)] = queueNode->next;
		}

		// ... and push it again with new priority, releasing the memory.
		m_size--;
		handle = push(queueNode->value, priority);
		delete queueNode;
		break;
	}
}

}
