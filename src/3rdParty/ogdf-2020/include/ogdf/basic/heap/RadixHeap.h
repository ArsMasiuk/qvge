/** \file
 * \brief Implementation of radix heap data structure.
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
#include <array>
#include <vector>


namespace ogdf {


template<typename V, typename P>
class RadixHeapNode {
public:
	V value;
	P priority;

	RadixHeapNode<V, P> *prev;
	RadixHeapNode<V, P> *next;

	RadixHeapNode(const V &nodeValue, const P &nodePriority)
	: value(nodeValue), priority(nodePriority),
	  prev(nullptr), next(nullptr)
	{
	}
};


//! Radix heap data structure implementation.
/**
 * @ingroup containers
 *
 * It is just a simple implementation of the idea sketched here:
 * http://ssp.impulsetrain.com/radix-heap.html
 *
 * To achieve best performance it also uses low-level word functions
 * where avaliable.
 *
 * @tparam V Denotes type of values of inserted elements.
 * @tparam P Denotes type of integral priorities of inserted elements.
 */
template<typename V, typename P>
class RadixHeap {
public:
	//! Creates empty heap.
	RadixHeap();

	//! Destructs the heap.
	~RadixHeap();


	//! Inserts a new node with given \p value and \p priority into a heap.
	/**
	 * @param value A value of inserted element.
	 * @param priority A priority of inserted element.
	 * @return Handle to the inserted node.
	 */
	RadixHeapNode<V, P> *push(const V &value, const P &priority);

	//! Removes the top element from the heap and returns its value.
	/**
	 * Behaviour of this function is undefined if the heap is empty.
	 *
	 * @return Value of the popped node.
	 */
	V pop();

	//! Number of elements contained within the heap.
	std::size_t size() const {
		return m_size;
	}

	//! Checks whether the heap is empty.
	bool empty() const {
		return size() == 0;
	}

private:
	using Bucket = RadixHeapNode<V, P> *;
	static constexpr std::size_t BITS = sizeof(P) * 8;

	std::size_t m_size; //! Number of elements.

	P m_minimum; //! Priority of the lowest element so far.
	P m_bucketMask; //! Mask describing state of buckets (for fast lookup).
	std::array<Bucket, BITS + 1> m_buckets; //! Buckets with values.

	//! Inserts \p heapNode to the appropriate bucket.
	void insert(RadixHeapNode<V, P> *heapNode);

	//! Returns most significant bit set on given mask.
	template<typename T>
	static std::size_t msbSet(T mask) {
		std::size_t i = 0;
		while(mask != 0) {
			mask >>= 1;
			i++;
		}
		return i;
	}

#ifdef __has_builtin
#if __has_builtin(__builtin_clz)
	static std::size_t msbSet(unsigned int mask) {
		return mask == 0 ? 0 : (BITS - __builtin_clz(mask));
	}

	static std::size_t msbSet(unsigned long mask) {
		return mask == 0 ? 0 : (BITS - __builtin_clzl(mask));
	}

	static std::size_t msbSet(unsigned long long mask) {
		return mask == 0 ? 0 : (BITS - __builtin_clzll(mask));
	}
#endif
#endif

};


template<typename V, typename P>
RadixHeap<V, P>::RadixHeap()
: m_size(0), m_minimum(0), m_bucketMask(0)
{
	m_buckets.fill(nullptr);
}


template<typename V, typename P>
RadixHeap<V, P>::~RadixHeap()
{
	for(Bucket &bucket : m_buckets) {
		auto it = bucket;
		while(it != nullptr) {
			auto next = it->next;
			delete it;
			it = next;
		}
	}
}


template<typename V, typename P>
RadixHeapNode<V, P> *RadixHeap<V, P>::push(const V &value, const P &priority)
{
	m_size++;

	RadixHeapNode<V, P> *heapNode = new RadixHeapNode<V, P>(value, priority);
	insert(heapNode);
	return heapNode;
}


template<typename V, typename P>
V RadixHeap<V, P>::pop()
{
	m_size--;

	if(m_buckets[0] != nullptr) {
		V result = std::move(m_buckets[0]->value);

		Bucket tmp = m_buckets[0]->next;
		delete m_buckets[0];
		m_buckets[0] = tmp;

		if(m_buckets[0] != nullptr) {
			m_buckets[0]->prev = nullptr;
		}
		return result;
	}

	std::size_t ind = BITS + 1 - msbSet(m_bucketMask);

	Bucket bucket = m_buckets[ind];
	m_buckets[ind] = nullptr;
	if(ind != 0) {
		m_bucketMask ^= (P(1) << (8 * sizeof(P) - ind));
	}


	Bucket min = bucket;
	for(Bucket it = bucket; it != nullptr; it = it->next) {
		if(it->priority < min->priority) {
			min = it;
		}
	}

	if(min->prev != nullptr) {
		min->prev->next = min->next;
	} else {
		bucket = min->next;
	}

	if(min->next != nullptr) {
		min->next->prev = min->prev;
	}

	V result = std::move(min->value);
	m_minimum = std::move(min->priority);
	delete min;

	while(bucket != nullptr) {
		Bucket next = bucket->next;
		bucket->prev = nullptr;
		insert(bucket);

		bucket = next;
	}

	return result;
}


template<typename V, typename P>
inline void RadixHeap<V, P>::insert(RadixHeapNode<V, P> *heapNode)
{
	std::size_t ind = msbSet(heapNode->priority ^ m_minimum);

	heapNode->next = m_buckets[ind];
	if(m_buckets[ind] != nullptr) {
		m_buckets[ind]->prev = heapNode;
	}
	m_buckets[ind] = heapNode;

	if(ind != 0) {
		m_bucketMask |= (P(1) << (BITS - ind));
	}
}

}
