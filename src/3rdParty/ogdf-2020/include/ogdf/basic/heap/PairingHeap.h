/** \file
 * \brief Implementation of pairing heap data structure.
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

#include <ogdf/basic/heap/HeapBase.h>

namespace ogdf {


//! Pairing heap node.
template<typename T>
struct PairingHeapNode {
	template<typename, typename> friend class PairingHeap;
protected:
	T value; //!< Value contained in the node.

	PairingHeapNode<T> *prev; //!< Previous sibling of the node or parent.
	PairingHeapNode<T> *next; //!< Next sibling of the node.
	PairingHeapNode<T> *child; //!< First child of the node.

	//! Creates heap node with a given \p valueOfNode.
	explicit PairingHeapNode(const T &valueOfNode)
	: value(valueOfNode),
	  prev(nullptr), next(nullptr), child(nullptr)
	{
	}
};


//! Pairing heap implementation.
/**
 * @ingroup containers
 *
 * Code is mainly based on orginal paper "The Pairing Heap: A New Form of
 * Self-Adjusting Heap" by Fredman, Sedgewick, Sleator and Tarjan.
 *
 * @tparam T Denotes value type of inserted elements.
 * @tparam C Denotes comparison functor determining value ordering.
 */
template<typename T, typename C>
class PairingHeap : public HeapBase<PairingHeap<T, C>, PairingHeapNode<T>, T, C> {

	using base_type = HeapBase<PairingHeap<T, C>, PairingHeapNode<T>, T, C>;

public:
	/**
	 * Creates empty pairing heap.
	 *
	 * @param cmp Comparison functor determining value ordering.
	 * @param initialSize ignored by this implementation.
	 */
	explicit PairingHeap(const C &cmp = C(), int initialSize = -1);

	/**
	 * Destructs pairing heap.
	 *
	 * If the heap is not empty, destructors of contained elements are called
	 * and used storage is deallocated.
	 */
	virtual ~PairingHeap();

	//! Returns reference to the top element in the heap.
	const T &top() const override;

	/**
	 * Inserts a new node with given \p value into a heap.
	 *
	 * @param value A value to be inserted.
	 * @return Handle to the inserted node.
	 */
	PairingHeapNode<T> *push(const T &value) override;

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
	void decrease(PairingHeapNode<T> *heapNode, const T &value) override;

	/**
	 * Merges in values of \p other heap.
	 *
	 * After merge \p other heap becomes empty and is valid for further usage.
	 *
	 * @param other A heap to be merged in.
	 */
	void merge(PairingHeap<T, C> &other) override;

	/**
	 * Returns the value of the node
	 *
	 * @param heapNode The nodes handle
	 * @return the value of the node
	 */
	const T &value(PairingHeapNode<T> *heapNode) const override {
		return heapNode->value;
	}

private:
	PairingHeapNode<T> *m_root; //!< Root node of the heap.

	//! Pairs list of heaps given as \p heapNode. Returns resulting list.
	PairingHeapNode<T> *pair(PairingHeapNode<T> *heapNode);
	//! Merges lists of heaps \p a and \p b. Returns resulting list.
	PairingHeapNode<T> *merge(PairingHeapNode<T> *a, PairingHeapNode<T> *b);

	//! Makes \p child node a child of \p parent node.
	static void link(PairingHeapNode<T> *parent, PairingHeapNode<T> *child);
	//! Removes \p heapNode from its parent children list.
	static void unlink(PairingHeapNode<T> *heapNode);

	//! Releases memory occupied by list of heaps given as \p heapNode.
	static void release(PairingHeapNode<T> *heapNode);
};


template<typename T, typename C>
PairingHeap<T, C>::PairingHeap(const C &cmp, int /* unused parameter */)
: base_type(cmp), m_root(nullptr) {}


template<typename T, typename C>
PairingHeap<T, C>::~PairingHeap()
{
	release(m_root);
	m_root = nullptr;
}


template<typename T, typename C>
inline const T &PairingHeap<T, C>::top() const
{
	return m_root->value;
}


template<typename T, typename C>
PairingHeapNode<T> *PairingHeap<T, C>::push(const T &value)
{
	PairingHeapNode<T> *heapNode = new PairingHeapNode<T>(value);

	m_root = m_root == nullptr ? heapNode : merge(m_root, heapNode);
	return heapNode;
}


template<typename T, typename C>
void PairingHeap<T, C>::pop()
{
	PairingHeapNode<T> *children = m_root->child;
	delete m_root;

	m_root = pair(children);
}


template<typename T, typename C>
void PairingHeap<T, C>::decrease(PairingHeapNode<T> *heapNode, const T &value)
{
	heapNode->value = value;
	if (heapNode->prev != nullptr) {
		unlink(heapNode);
		m_root = merge(m_root, heapNode);
	}
}


template<typename T, typename C>
void PairingHeap<T, C>::merge(PairingHeap<T, C> &other)
{
	m_root = merge(m_root, other.m_root);
	other.m_root = nullptr;
}


template<typename T, typename C>
inline PairingHeapNode<T> *PairingHeap<T, C>::pair(
	PairingHeapNode<T> *heapNode)
{
	if(heapNode == nullptr) {
		return nullptr;
	}

	/*
	 * We move towards the end of list counting elements along the way. We do
	 * this for two reasons: to know whether the list has even or odd number of
	 * elements and for possible speed up of going-back through the list (loop
	 * unrolling is not applicable for iterators but works for integers).
	 */
	size_t children = 1;
	PairingHeapNode<T> *it = heapNode;
	while (it->next != nullptr) {
		it = it->next;
		children++;
	}

	PairingHeapNode<T> *result;

	if (children % 2 == 1) {
		PairingHeapNode<T> *a = it;
		it = it->prev;
		a->prev = a->next = nullptr;
		result = a;
	} else {
		PairingHeapNode<T> *a = it, *b = it->prev;
		it = it->prev->prev;
		a->prev = a->next = b->prev = b->next = nullptr;
		result = merge(a, b);
	}

	for (size_t i = 0; i < (children - 1) / 2; i++) {
		PairingHeapNode<T> *a = it, *b = it->prev;
		it = it->prev->prev;
		a->prev = a->next = b->prev = b->next = nullptr;
		result = merge(merge(a, b), result);
	}

	return result;
}


template<typename T, typename C>
inline PairingHeapNode<T> *PairingHeap<T, C>::merge(
	PairingHeapNode<T> *a, PairingHeapNode<T> *b)
{
	if(this->comparator()(a->value, b->value)) {
		link(a, b);
		return a;
	} else {
		link(b, a);
		return b;
	}
}


template<typename T, typename C>
inline void PairingHeap<T, C>::link(
	PairingHeapNode<T> *root, PairingHeapNode<T> *child)
{
	if(root->child != nullptr) {
		child->next = root->child;
		root->child->prev = child;
	}
	child->prev = root;
	root->child = child;
}


template<typename T, typename C>
inline void PairingHeap<T, C>::unlink(
	PairingHeapNode<T> *heapNode)
{
	if(heapNode->prev->child == heapNode) {
		heapNode->prev->child = heapNode->next;
	} else {
		heapNode->prev->next = heapNode->next;
	}
	if(heapNode->next != nullptr) {
		heapNode->next->prev = heapNode->prev;
	}
	heapNode->prev = nullptr;
	heapNode->next = nullptr;
}


template<typename T, typename C>
inline void PairingHeap<T, C>::release(PairingHeapNode<T> *heapNode)
{
	/*
	 * Recursive version of this function is infinitely prettier than that
	 * abomination. Please, make it prettier if you can.
	 */
	PairingHeapNode<T> *it = heapNode;

	if(it == nullptr) {
		return;
	}

	for(;;) {
		// Slide down as long as possible.
		if(it->child != nullptr) {
			it = it->child;
			continue;
		}
		if(it->next != nullptr) {
			it = it->next;
			continue;
		}

		// Climb up until you find first non-visited node.
		for(;;) {
			PairingHeapNode<T> *curr = it, *prev = it->prev;
			delete it;

			if(prev == nullptr) {
				return;
			}
			if(curr == prev->child && prev->next != nullptr) {
				it = prev->next;
				break;
			} else {
				it = prev;
			}
		}
	}
}

}
