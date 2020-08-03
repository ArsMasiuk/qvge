/** \file
 * \brief Implementation of Fibonacci heap data structure.
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

#include <array>
#include <utility>
#include <functional>

#include <ogdf/basic/heap/HeapBase.h>

namespace ogdf {


//! Fibonacci heap node.
template<typename T>
struct FibonacciHeapNode {
	template<typename, typename> friend class FibonacciHeap;
protected:
	T value; //!< Value contained in the node.

	size_t rank; //!< Determines rank of a node.
	bool marked; //!< Indicates whether node is marked or not.

	FibonacciHeapNode<T> *parent; //!< Parent of the node.
	FibonacciHeapNode<T> *child; //!< First child of the node.
	FibonacciHeapNode<T> *prev; //!< Previous sibling of the node.
	FibonacciHeapNode<T> *next; //!< Next sibling of the node.

	//! Creates empty root node.
	FibonacciHeapNode()
	: rank(0), marked(false),
	  parent(nullptr), child(nullptr), prev(this), next(this)
	{
	}

	//! Creates heap node with a given \p nodeValue.
	explicit FibonacciHeapNode(const T &nodeValue)
	: value(nodeValue),
	  rank(0), marked(false),
	  parent(nullptr), child(nullptr), prev(this), next(this)
	{
	}
};


//! Fibonacci heap implementation.
/**
 * @ingroup containers
 *
 * This implementation is based on Wikipedia article, original paper by
 * Fredman and Tarjan and borrows some ideas from:
 * http://www.cs.princeton.edu/~wayne/cs423/fibonacci/FibonacciHeapAlgorithm.html
 *
 * @tparam T Denotes value type of inserted elements.
 * @tparam C Denotes comparison functor determining value ordering.
 */
template<typename T, typename C = std::less<T>>
class FibonacciHeap : public HeapBase<FibonacciHeap<T, C>, FibonacciHeapNode<T>, T, C>
{

	using base_type = HeapBase<FibonacciHeap<T, C>, FibonacciHeapNode<T>, T, C>;

public:

	/**
	 * Creates empty Fibonacci heap.
	 *
	 * @param cmp Comparison functor determining value ordering.
	 * @param initialSize ignored by this implementation.
	 */
	explicit FibonacciHeap(const C &cmp = C(), int initialSize = -1);

	/**
	 * Destructs the heap.
	 *
	 * If the heap is not empty, destructors of contained elements are called
	 * and used storage is deallocated.
	 */
	virtual ~FibonacciHeap();

	//! Returns reference to the top element in the heap.
	const T &top() const override;

	/**
	 * Inserts a new node with given \p value into a heap.
	 *
	 * @param value A value to be inserted.
	 * @return Handle to the inserted node.
	 */
	FibonacciHeapNode<T> *push(const T &value) override;

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
	void decrease(FibonacciHeapNode<T> *heapNode, const T &value) override;

	/**
	 * Merges in values of \p other heap.
	 *
	 * After merge \p other heap becomes empty and is valid for further usage.
	 *
	 * @param other A heap to be merged in.
	 */
	void merge(FibonacciHeap<T, C> &other) override;

	/**
	 * Returns the value of the node
	 *
	 * @param heapNode The nodes handle
	 * @return the value of the node
	 */
	const T &value(FibonacciHeapNode<T> *heapNode) const override {
		return heapNode->value;
	}

private:
	//! Handle to the tree with lowest root priority.
	FibonacciHeapNode<T> *m_minimal;
	//! Used for efficient tree list manipulation.
	FibonacciHeapNode<T> *m_knot;

	//! Used to compress trees.
	std::array<FibonacciHeapNode<T> *, sizeof(size_t) * 8> m_ranked;

	//! Removes minimal tree and moves its children to tree list.
	void remove();
	//! Reduces number of trees inside a heap by linking ones with same degree.
	void compress();

	//! Makes \p child node a child of \p root node.
	void link(FibonacciHeapNode<T> *root, FibonacciHeapNode<T> *child);
	//! Detaches given \p heapNode from its list and makes it self-circulate.
	void detach(FibonacciHeapNode<T> *heapNode);
	//! Merges \p other list into current heap list.
	void merge(FibonacciHeapNode<T> *other);
	//! Moves \p heapNode from its list to the \p target list.
	void splice(FibonacciHeapNode<T> *target, FibonacciHeapNode<T> *heapNode);
	//! Restores heap ordering in \p heapNode by making (cascade) cut.
	void restore(FibonacciHeapNode<T> *heapNode);

	//! Recursively releases memory starting at \p heapNode.
	void release(FibonacciHeapNode<T> *heapNode);
};


template<typename T, typename C>
FibonacciHeap<T, C>::FibonacciHeap(const C &cmp, int /* unused parameter */)
: base_type(cmp), m_minimal(nullptr), m_knot(new FibonacciHeapNode<T>())
{
	m_ranked.fill(nullptr);
}


template<typename T, typename C>
FibonacciHeap<T, C>::~FibonacciHeap()
{
	release(m_knot);
}


template<typename T, typename C>
void FibonacciHeap<T, C>::release(FibonacciHeapNode<T> *heapNode)
{
	if(heapNode == nullptr) {
		return;
	}

	FibonacciHeapNode<T> *end = heapNode;
	do {
		release(heapNode->child);

		FibonacciHeapNode<T> *next = heapNode->next;
		delete heapNode;
		heapNode = next;
	} while(heapNode != end);
}


template<typename T, typename C>
inline const T &FibonacciHeap<T, C>::top() const
{
	return m_minimal->value;
}


template<typename T, typename C>
FibonacciHeapNode<T> *FibonacciHeap<T, C>::push(const T &value)
{
	FibonacciHeapNode<T> *heapNode = new FibonacciHeapNode<T>(value);
	splice(m_knot, heapNode);

	if(m_minimal == nullptr || this->comparator()(heapNode->value, m_minimal->value)) {
		m_minimal = heapNode;
	}

	return heapNode;
}


template<typename T, typename C>
void FibonacciHeap<T, C>::pop()
{
	// Special case for tree with only one node.
	if(m_knot->next->next == m_knot &&
	   m_knot->next->child == nullptr)
	{
		m_knot->prev = m_knot->next = m_knot;
		delete m_minimal;
		m_minimal = nullptr;
		return;
	}

	remove();
	compress();

	// Find new minimal node in compressed tree list.
	m_minimal = m_knot->next;
	for(auto it = m_knot->next->next; it != m_knot; it = it->next) {
		if(this->comparator()(it->value, m_minimal->value)) {
			m_minimal = it;
		}
	}
}


template<typename T, typename C>
void FibonacciHeap<T, C>::decrease(FibonacciHeapNode<T> *heapNode, const T &value)
{
	heapNode->value = value;
	if(this->comparator()(heapNode->value, m_minimal->value)) {
		m_minimal = heapNode;
	}

	restore(heapNode);
}


template<typename T, typename C>
void FibonacciHeap<T, C>::merge(FibonacciHeap<T, C> &other)
{
	if(other.m_minimal == nullptr) {
		return;
	}

	FibonacciHeapNode<T> *next = other.m_knot->next;
	detach(other.m_knot);
	merge(next);

	if(this->comparator()(other.m_minimal->value, m_minimal->value)) {
		m_minimal = other.m_minimal;
	}
	other.m_minimal = nullptr;
}


template<typename T, typename C>
inline void FibonacciHeap<T, C>::remove()
{
	if(m_minimal->child) {
		FibonacciHeapNode<T> *it = m_minimal->child;
		do {
			FibonacciHeapNode<T> *next = it->next;
			it->parent = nullptr;
			splice(m_knot, it);

			it = next;
		} while(it != m_minimal->child);
	}
	detach(m_minimal);
	delete m_minimal;
}


template<typename T, typename C>
inline void FibonacciHeap<T, C>::compress()
{
	size_t maxr = 0;

	for(auto it = m_knot->next; it != m_knot;) {
		FibonacciHeapNode<T> *next = it->next;

		size_t r = it->rank;
		maxr = std::max(r, maxr);
		while(m_ranked[r]) {
			if(this->comparator()(m_ranked[r]->value, it->value)) {
				link(m_ranked[r], it);
				it = m_ranked[r];
			} else {
				link(it, m_ranked[r]);
			}
			m_ranked[r] = nullptr;
			r++;
			maxr = std::max(maxr, r);
		}
		m_ranked[r] = it;

		it = next;
	}

	for(size_t i = 0; i <= maxr; i++) {
		m_ranked[i] = nullptr;
	}
}


template<typename T, typename C>
inline void FibonacciHeap<T, C>::link(
	FibonacciHeapNode<T> *root, FibonacciHeapNode<T> *child)
{
	child->marked = false;
	child->parent = root;
	root->rank++;

	if(root->child) {
		splice(root->child, child);
	} else {
		detach(child);
		root->child = child;
	}
}


template<typename T, typename C>
inline void FibonacciHeap<T, C>::detach(FibonacciHeapNode<T> *heapNode)
{
	heapNode->prev->next = heapNode->next;
	heapNode->next->prev = heapNode->prev;
	heapNode->next = heapNode;
	heapNode->prev = heapNode;
}


template<typename T, typename C>
inline void FibonacciHeap<T, C>::merge(FibonacciHeapNode<T> *other)
{
	m_knot->next->prev = other->prev;
	other->prev->next = m_knot->next;
	m_knot->next = other;
	other->prev = m_knot;
}


template<typename T, typename C>
inline void FibonacciHeap<T, C>::splice(
	FibonacciHeapNode<T> *target, FibonacciHeapNode<T> *heapNode)
{
	detach(heapNode);
	target->next->prev = heapNode;
	heapNode->next = target->next;
	target->next = heapNode;
	heapNode->prev = target;
}


template<typename T, typename C>
inline void FibonacciHeap<T, C>::restore(FibonacciHeapNode<T> *heapNode)
{
	for(;;) {
		FibonacciHeapNode<T> *parent = heapNode->parent;
		if(parent == nullptr) {
			return;
		}

		// We need to make sure parent has valid children after the splice.
		parent->rank--;
		if(parent->rank == 0) {
			parent->child = nullptr;
		} else if(parent->child == heapNode) {
			parent->child = heapNode->next;
		}

		heapNode->parent = nullptr;
		splice(m_knot, heapNode);

		// If parent is unmarked we can stop cut cascade.
		if(!parent->marked) {
			parent->marked = true;
			return;
		}

		heapNode = parent;
	}
}

}
