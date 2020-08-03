/** \file
 * \brief Implementation of randomized meldable heap data structure.
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
#include <random>

#include <ogdf/basic/heap/HeapBase.h>

namespace ogdf {


//! Randomized meldable heap node.
template<typename T>
struct RMHeapNode {
	template<typename, typename> friend class RMHeap;
protected:
	T value; //!< Value contained in the node.

	RMHeapNode<T> *parent; //!< Parent of the node.
	RMHeapNode<T> *left; //!< Left child of the node.
	RMHeapNode<T> *right; //!< Right child of the node.

	//! Creates heap node with a given \p nodeValue.
	explicit RMHeapNode(const T &nodeValue)
	: value(nodeValue),
	  parent(nullptr), left(nullptr), right(nullptr)
	{
	}
};


//! Randomized meldable heap implementation.
/**
 * @ingroup containers
 *
 * Code of meld (also known as merge) operation is solely based on Wikipedia
 * article. Other methods are based on my intuitions and make use of melding.
 *
 * For random number generation it uses default generator provided by the C++11
 * standard. In the future, it should be possible to provide custom random
 * device, generator and seed.
 *
 * @tparam T Denotes value type of inserted elements.
 * @tparam C Denotes comparison functor determining value ordering.
 */
template<typename T, typename C=std::less<T>>
class RMHeap : public HeapBase<RMHeap<T, C>, RMHeapNode<T>, T, C>
{

	using base_type = HeapBase<RMHeap<T, C>, RMHeapNode<T>, T, C>;

public:

	/**
	 * Creates empty randomized meldable heap.
	 *
	 * @param cmp Comparison functor determining value ordering.
	 * @param initialSize ignored by this implementation.
	 */
	explicit RMHeap(const C &cmp = C(), int initialSize = -1);

	/**
	 * Destructs the heap.
	 *
	 * If the heap is not empty, destructors of contained elements are called
	 * and used storage is deallocated.
	 */
	virtual ~RMHeap();

	//! Returns reference to the top element in the heap.
	const T &top() const override;

	/**
	 * Inserts a new node with given \p value into a heap.
	 *
	 * @param value A value to be inserted.
	 * @return Handle to the inserted node.
	 */
	RMHeapNode<T> *push(const T &value) override;

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
	void decrease(RMHeapNode<T> *heapNode, const T &value) override;

	/**
	 * Merges in values of \p other heap.
	 *
	 * After merge \p other heap becomes empty and is valid for further usage.
	 *
	 * @param other A heap to be merged in.
	 */
	void merge(RMHeap<T, C> &other) override;

	/**
	 * Returns the value of the node
	 *
	 * @param heapNode The nodes handle
	 * @return the value of the node
	 */
	const T &value(RMHeapNode<T> *heapNode) const override {
		return heapNode->value;
	}

private:
	std::default_random_engine m_rand; //!< Random values generator.
	RMHeapNode<T> *m_root; //!< Root node of the heap.

	//! Recursively merges heaps \p a and \p b. Returns resulting heap.
	RMHeapNode<T> *merge(RMHeapNode<T> *a, RMHeapNode<T> *b);
	//! Removes given \p heapNode from the main tree (does not free memory!).
	void remove(RMHeapNode<T> *heapNode);

	//! Recursively releases memory occupied by heap pointed by \p heapNode.
	static void release(RMHeapNode<T> *heapNode);
};


template<typename T, typename C>
RMHeap<T, C>::RMHeap(const C &cmp, int initialSize)
: base_type(cmp), m_rand((std::random_device())()), m_root(nullptr) {}


template<typename T, typename C>
RMHeap<T, C>::~RMHeap()
{
	release(m_root);
}


template<typename T, typename C>
const T &RMHeap<T, C>::top() const
{
	return m_root->value;
}


template<typename T, typename C>
RMHeapNode<T> *RMHeap<T, C>::push(const T &value)
{
	RMHeapNode<T> *heapNode = new RMHeapNode<T>(value);
	m_root = merge(m_root, heapNode);

	return heapNode;
}


template<typename T, typename C>
void RMHeap<T, C>::pop()
{
	RMHeapNode<T> *root = m_root;
	m_root = merge(m_root->left, m_root->right);
	if(m_root != nullptr) {
		m_root->parent = nullptr;
	}
	delete root;
}


template<typename T, typename C>
void RMHeap<T, C>::decrease(RMHeapNode<T> *heapNode, const T &value)
{
	heapNode->value = value;
	if(heapNode == m_root) {
		return;
	}

	remove(heapNode);
	heapNode->left = nullptr;
	heapNode->right = nullptr;
	heapNode->parent = nullptr;

	m_root = merge(m_root, heapNode);
}


template<typename T, typename C>
void RMHeap<T, C>::merge(RMHeap<T, C> &other)
{
	m_root = merge(m_root, other.m_root);
	other.m_root = nullptr;
}


template<typename T, typename C>
RMHeapNode<T> *RMHeap<T, C>::merge(RMHeapNode<T> *a, RMHeapNode<T> *b)
{
	if(a == nullptr) {
		return b;
	}
	if(b == nullptr) {
		return a;
	}

	if(this->comparator()(a->value, b->value)) {
		if(m_rand() % 2 == 0) {
			a->left = merge(a->left, b);
			if(a->left != nullptr) {
				a->left->parent = a;
			}
		} else {
			a->right = merge(a->right, b);
			if(a->right != nullptr) {
				a->right->parent = a;
			}
		}
		return a;
	} else {
		if(m_rand() % 2 == 0) {
			b->left = merge(b->left, a);
			if(b->left != nullptr) {
				b->left->parent = b;
			}
		} else {
			b->right = merge(b->right, a);
			if(b->right != nullptr) {
				b->right->parent = b;
			}
		}
		return b;
	}
}


template<typename T, typename C>
void RMHeap<T, C>::remove(RMHeapNode<T> *heapNode)
{
	RMHeapNode<T> *merged = merge(heapNode->left, heapNode->right);
	if(heapNode == heapNode->parent->left) {
		heapNode->parent->left = merged;
	} else {
		heapNode->parent->right = merged;
	}
	if(merged != nullptr) {
		merged->parent = heapNode->parent;
	}
}


template<typename T, typename C>
void RMHeap<T, C>::release(RMHeapNode<T> *heapNode)
{
	if(heapNode == nullptr) {
		return;
	}

	release(heapNode->left);
	release(heapNode->right);
	delete heapNode;
}

}
