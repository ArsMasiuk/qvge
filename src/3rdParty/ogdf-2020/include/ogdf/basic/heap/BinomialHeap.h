/** \file
 * \brief Implementation of binomial heap data structure.
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

#include <ogdf/basic/heap/HeapBase.h>

namespace ogdf {


//! Binomial heap node.
template<typename T>
struct BinomialHeapNode {
	template<typename, typename> friend class BinomialHeap;
protected:
	T value; //!< Value contained in the node.

	size_t rank; //!< Determines rank of a node.

	BinomialHeapNode<T> *parent; //!< Parent of the node.
	BinomialHeapNode<T> *next; //!< Next sibling of the node.
	BinomialHeapNode<T> *child; //!< First child of the node.

	//! Creates heap node with a given \p nodeValue.
	explicit BinomialHeapNode(const T &nodeValue)
	: value(nodeValue),
	  rank(0), parent(nullptr), next(nullptr), child(nullptr)
	{
	}
};


//! Binomial heap implementation.
/**
 * @ingroup containers
 *
 * Code is mainly based on samples and ideas provided in "Introduction to
 * Algorithms" book (aka "Cormen").
 *
 * @tparam T Denotes value type of inserted elements.
 * @tparam C Denotes comparison functor determining value ordering.
 */
template<typename T, typename C = std::less<T>>
class BinomialHeap : public HeapBase<BinomialHeap<T, C>, BinomialHeapNode<T>, T, C>
{

	using base_type = HeapBase<BinomialHeap<T, C>, BinomialHeapNode<T>, T, C>;

public:

	/**
	 * Creates empty binomial heap.
	 *
	 * @param cmp Comparison functor determining value ordering.
	 * @param initialSize ignored by this implementation.
	 */
	explicit BinomialHeap(const C &cmp = C(), int initialSize = -1);

	/**
	 * Destructs the heap.
	 *
	 * If the heap is not empty, destructors of contained elements are called
	 * and used storage is deallocated.
	 */
	virtual ~BinomialHeap();

	//! Returns reference to the top element in the heap.
	const T &top() const override;

	/**
	 * Inserts a new node with given \p value into a heap.
	 *
	 * @param value A value to be inserted.
	 * @return Handle to the inserted node.
	 */
	BinomialHeapNode<T> *push(const T &value) override;

	/**
	 * Removes the top element from the heap.
	 *
	 * Behaviour of this function is undefined if the heap is empty.
	 */
	void pop() override;

	/**
	 * Decreases value of the given \p heapNode to \p value.
	 *
	 * Behaviour of this function is undefined if node does not belong to a the
	 * heap or new value is greater than old one.
	 *
	 * @param heapNode A node for which the value is to be decreased.
	 * @param value A new value for the node.
	 */
	void decrease(BinomialHeapNode<T> *heapNode, const T &value) override;

	/**
	 * Merges in values of \p other heap.
	 *
	 * After merge \p other heap becomes empty and is valid for further usage.
	 *
	 * @param other A heap to be merged in.
	 */
	void merge(BinomialHeap<T, C> &other) override;

	/**
	 * Returns the value of the node
	 *
	 * @param heapNode The nodes handle
	 * @return the value of the node
	 */
	const T &value(BinomialHeapNode<T> *heapNode) const override {
		return heapNode->value;
	}

private:
	BinomialHeapNode<T> *m_root; //!< Root node of the heap.

	//! Joins heap lists \p a and \p b into single list sorted by the ranks.
	BinomialHeapNode<T> *join(BinomialHeapNode<T> *a, BinomialHeapNode<T> *b);
	//! Merges in \p other heap list into the heap.
	void merge(BinomialHeapNode<T> *other);

	//! Makes \p child node a child of \p parent node.
	static void link(BinomialHeapNode<T> *parent, BinomialHeapNode<T> *child);

	//! Releases memory occupied by list of heaps given as \p heapNode.
	static void release(BinomialHeapNode<T> *heapNode);
};


template<typename T, typename C>
BinomialHeap<T, C>::BinomialHeap(const C &cmp, int /* unused parameter */)
: base_type(cmp), m_root(nullptr) {}


template<typename T, typename C>
BinomialHeap<T, C>::~BinomialHeap()
{
	release(m_root);
	m_root = nullptr;
}


template<typename T, typename C>
void BinomialHeap<T, C>::release(BinomialHeapNode<T> *heapNode)
{
	while(heapNode != nullptr) {
		release(heapNode->child);

		BinomialHeapNode<T> *next = heapNode->next;
		delete heapNode;
		heapNode = next;
	}
}


template<typename T, typename C>
inline const T &BinomialHeap<T, C>::top() const
{
	BinomialHeapNode<T> *min = m_root;
	for(BinomialHeapNode<T> *it = m_root->next; it != nullptr; it = it->next) {
		if(this->comparator()(it->value, min->value)) {
			min = it;
		}
	}

	return min->value;
}


template<typename T, typename C>
BinomialHeapNode<T> *BinomialHeap<T, C>::push(const T &value)
{
	BinomialHeapNode<T> *heapNode = new BinomialHeapNode<T>(value);

	merge(heapNode);
	return heapNode;
}


template<typename T, typename C>
void BinomialHeap<T, C>::pop()
{
	BinomialHeapNode<T> *curr = m_root, *min = m_root, *minPrev = nullptr;

	while(curr->next != nullptr) {
		if(this->comparator()(curr->next->value, min->value)) {
			min = curr->next;
			minPrev = curr;
		}
		curr = curr->next;
	}

	if(min == m_root) {
		m_root = min->next;
	} else {
		minPrev->next = min->next;
	}

	// Children list has to be reversed before it can be merged.
	BinomialHeapNode<T> *reversed = nullptr, *child = min->child;
	while(child != nullptr) {
		BinomialHeapNode<T> *next = child->next;
		child->next = reversed;
		reversed = child;
		child = next;
	}
	merge(reversed);
	delete min;
}


template<typename T, typename C>
void BinomialHeap<T, C>::decrease(BinomialHeapNode<T> *heapNode, const T &value)
{
	// BinomialHeap::decrease is not supported
	OGDF_ASSERT(false);

	heapNode->value = value;

	while(heapNode->parent != nullptr &&
	      this->comparator()(heapNode->value, heapNode->parent->value))
	{
		std::swap(heapNode->value, heapNode->parent->value);
		heapNode = heapNode->parent;
	}
}


template<typename T, typename C>
void BinomialHeap<T, C>::merge(BinomialHeap<T, C> &other)
{
	merge(other.m_root);
	other.m_root = nullptr;
}


template<typename T, typename C>
inline BinomialHeapNode<T> *BinomialHeap<T, C>::join(
	BinomialHeapNode<T> *a, BinomialHeapNode<T> *b)
{
	if(a == nullptr) {
		return b;
	}
	if(b == nullptr) {
		return a;
	}

	if(b->rank < a->rank) {
		std::swap(a, b);
	}

	BinomialHeapNode<T> *head = a;
	while(b != nullptr) {
		if(a->next == nullptr) {
			a->next = b;
			break;
		}

		if(b->rank < a->next->rank) {
			BinomialHeapNode<T> *nextB = b->next;
			b->next = a->next;
			a->next = b;

			a = b;
			b = nextB;
		} else {
			a = a->next;
		}
	}

	return head;
}


template<typename T, typename C>
inline void BinomialHeap<T, C>::merge(BinomialHeapNode<T> *other)
{
	m_root = join(m_root, other);
	if(m_root == nullptr) {
		return;
	}

	BinomialHeapNode<T> *prev = nullptr, *curr = m_root, *next = m_root->next;
	while(next != nullptr) {
		if(curr->rank != next->rank || (next->next != nullptr &&
		                                next->next->rank == curr->rank))
		{
			prev = curr;
			curr = next;
			next = curr->next;
			continue;
		}

		if(this->comparator()(curr->value, next->value)) {
			curr->next = next->next;
			link(curr, next);
		} else if(prev == nullptr) {
			m_root = next;
			link(next, curr);
			curr = next;
		} else {
			prev->next = next;
			link(next, curr);
			curr = next;
		}
		next = curr->next;
	}
}


template<typename T, typename C>
inline void BinomialHeap<T, C>::link(
	BinomialHeapNode<T> *parent, BinomialHeapNode<T> *child)
{
	child->next = parent->child;
	child->parent = parent;
	parent->child = child;
	parent->rank++;
}

}
