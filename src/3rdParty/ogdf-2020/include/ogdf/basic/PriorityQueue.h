/** \file
 * \brief Priority queue interface wrapping various heaps.
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

#include <utility>
#include <functional>

#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/HashArray.h>

#include <ogdf/basic/heap/PairingHeap.h>

namespace ogdf {


//! Priority queue interface wrapper for heaps.
/**
 * @ingroup containers
 *
 * Priority queue offers interface similar to the STL's \c std::priority_queue
 * class but allows to choose from variety of different data structures to be
 * used as underlying implementation. Also, unlike STL's version it provides
 * extra methods for fast decreasing key of given element and merging-in other
 * priority queue.
 *
 * @tparam T Denotes value type of inserted elements.
 * @tparam C Denotes comparison functor determining value ordering.
 * @tparam Impl Denotes underlying heap class.
 */
template<
	typename T,
	class C=std::less<T>,
	template<typename, class> class Impl=PairingHeap
>
class PriorityQueue {
public:
	using size_type = std::size_t;

private:
	size_type m_size; //!< Number of entries.
	const C &m_cmp;
	using SpecImpl = Impl<T, C>;
	SpecImpl *m_impl; //!< Underlying heap data structure.

public:
	using value_type = T;
	using handle = typename SpecImpl::Handle;
	using reference = value_type &;
	using const_reference = const value_type &;

	//! Creates empty priority queue.
	/**
	 * @param cmp Comparison functor determining priority ordering.
	 * @param initialSize The initial capacity preference (ignored if not supported by underlying heap).
	 */
	explicit PriorityQueue(const C &cmp = C(), int initialSize = 128)
	: m_size(0), m_cmp(cmp), m_impl(new SpecImpl(cmp, initialSize))
	{
	}

	//! Copy constructor.
	PriorityQueue(const PriorityQueue &other)
	: m_size(other.m_size), m_impl(new SpecImpl(other.m_impl))
	{
	}

	//! Move constructor.
	PriorityQueue(PriorityQueue &&other)
	: PriorityQueue(other.m_impl->comparator())
	{
		other.swap(*this);
	}

	//! Creates priority queue with contents of the given range.
	/**
	 * @param first Begin iterator of the initializing range.
	 * @param last Past-the-end iterator of the initializing range.
	 * @param cmp Comparison functor determining priority ordering.
	 */
	template<class InputIt>
	PriorityQueue(InputIt first, InputIt last, const C &cmp = C())
	: PriorityQueue(cmp)
	{
		push(first, last);
	}

	//! Creates priority queue with contents of the given initializer list.
	/**
	 * @param init List whose elements should be used during initalization.
	 * @param cmp Comparison functor determining priority ordering.
	 */
	PriorityQueue(std::initializer_list<value_type> init, const C &cmp = C())
	: PriorityQueue(std::begin(init), std::end(init), cmp)
	{
	}

	//! Destroys the underlying data structure.
	~PriorityQueue() {
		delete m_impl;
	}

	//! Copy and move assignment.
	PriorityQueue &operator=(PriorityQueue other) {
		other.swap(*this);
		return *this;
	}

	//! Assigns the priority queue contents of the given initializer list.
	/**
	 * @param ilist List whose elements should be assigned.
	 * @return Reference to the assigned priority queue.
	 */
	PriorityQueue &operator=(std::initializer_list<value_type> ilist) {
		PriorityQueue tmp(ilist);
		tmp.swap(*this);
		return *this;
	}

	//! Swaps the contents.
	void swap(PriorityQueue &other) {
		std::swap(m_size, other.m_size);
		std::swap(m_impl, other.m_impl);
	}

	//! Swaps the contents.
	friend void swap(PriorityQueue &a, PriorityQueue &b) {
		a.swap(b);
	}

	//! Returns reference to the top element in the queue.
	const T &top() const {
		return m_impl->top();
	}

	//! Inserts a new element with given \p value into the queue.
	/*
	 * @param value A value to be inserted.
	 * @return Handle to the inserted node.
	 */
	handle push(const value_type &value) {
		m_size++;
		return m_impl->push(value);
	}

	//! Inserts new elements specified by the given range.
	/**
	 * @param first Begin iterator of the range being inserted.
	 * @param last Past-the-end iterator of the range being inserted.
	 */
	template<class InputIt>
	void push(InputIt first, InputIt last) {
		while(first != last) {
			push(*(first++));
		}
	}

	//! Inserts new elements specified by the given initializer list.
	/**
	 * @param ilist List whose elements should be used during insertion.
	 */
	void push(std::initializer_list<value_type> ilist) {
		push(std::begin(ilist), std::end(ilist));
	}

	//! Removes the top element from the heap.
	/**
	 * Behaviour of this function is undefined if the heap is empty.
	 */
	void pop() {
		m_size--;
		m_impl->pop();
	}

	//! Decreases value of the element specified by \p handle to \p value.
	/**
	 * Behaviour of this function is undefined if handle does not belong to
	 * a the queue or new value is greater than old one.
	 *
	 * @param pos A element for which the value is to be decreased.
	 * @param value A new value for the node.
	 */
	void decrease(handle pos, const T &value) {
		m_impl->decrease(pos, value);
	}

	//! Merges in enqueued values of \p other queue.
	/**
	 * After merge \p other queue becomes empty and is valid for further usage.
	 *
	 * @param other A queue to be merged in.
	 */
	void merge(PriorityQueue &other) {
		m_impl->merge(*other.m_impl);
		m_size += other.m_size;
		other.m_size = 0;
	}

	//! Removes all the entries from the queue.
	void clear() {
		PriorityQueue tmp(m_cmp);
		tmp.swap(*this);
	}

	//! Checks whether the queue is empty.
	bool empty() const {
		return size() == 0;
	}

	//! Returns the number of enqueued elements.
	size_type size() const {
		return m_size;
	}

	/**
	* Returns the priority of that handle.
	*
	* @param pos The handle
	* @return The priority
	*/
	const T &value(handle pos) const {
		return m_impl->value(pos);
	}
};

/**
 * This namespace contains helper classes to keep the code dry.
 * It should not be accessed from outside this file.
 */
namespace pq_internal {

//! Used to compare elements with assigned priorities.
template<
	typename T,
	typename C
>
class Compare
{
public:
	Compare(const C &compare = C()) : m_compare(compare) {}

	Compare(const Compare& other) : m_compare(other.m_compare) {}

	bool operator()(const T &x, const T &y) const {
		return m_compare(x.priority(), y.priority());
	}
private:
	C m_compare;
};

//! Pair for storing an element and a priority.
template<
	typename E,
	typename P
>
class PairTemplate
{
public:
	PairTemplate()
		: m_priority(-1)
	{
	}

	PairTemplate(const E &element, const P &priority)
		: m_element(element), m_priority(priority)
	{
	}

	const E &element() const {
		return m_element;
	}

	const P &priority() const {
		return m_priority;
	}

private:
	E m_element;
	P m_priority;
};

//! Shortcut for the base class of ::PrioritizedQueue.
template<
	typename E,
	typename P,
	class C,
	template<typename, class> class Impl
>
using SuperQueueTemplate = PriorityQueue<PairTemplate<E, P>, Compare<PairTemplate<E, P>, C>, Impl>;

//! Defines a queue for handling prioritized elements.
template<
	typename E,
	typename P,
	class C,
	template<typename, class> class Impl
>
class PrioritizedQueue : public SuperQueueTemplate<E, P, C, Impl>
{
private:
	using SuperQueue = SuperQueueTemplate<E, P, C, Impl>;
	using Pair = PairTemplate<E, P>;

	C m_comp;

public:
	//! The type of handle for accessing the elements of this queue.
	using Handle = typename SuperQueue::handle;

	PrioritizedQueue(const C &cmp = C(), int initialSize = 128)
		: SuperQueue(Compare<PairTemplate<E, P>, C>(cmp), initialSize), m_comp(cmp)
	{
	}

	//! Returns the topmost element in the queue.
	const E &topElement() const {
		return SuperQueue::top().element();
	}

	//! Returns the priority of the topmost element in this queue.
	const P &topPriority() const {
		return SuperQueue::top().priority();
	}

	/**
	 * Pushes a new element with the respective priority to the queue.
	 *
	 * @param element the element to be added
	 * @param priority the priority of that element
	 */
	Handle push(const E &element, const P &priority) {
		Pair pair(element, priority);
		Handle result = SuperQueue::push(pair);
		return result;
	}

	void decrease(Handle pos, const P &priority) {
		OGDF_ASSERT(m_comp(priority, this->value(pos).priority()));
		Pair pair(this->value(pos).element(), priority);
		SuperQueue::decrease(pos, pair);
	}
};

template<
	typename E,
	typename P,
	class C,
	template<typename, class> class Impl,
	typename Map
>
class PrioritizedArrayQueueBase : public PrioritizedQueue<E, P, C, Impl>
{
protected:
	using SuperQueue = PrioritizedQueue<E, P, C, Impl>;
	using Handle = typename PrioritizedQueue<E, P, C, Impl>::Handle;

public:

	//! Returns whether this queue contains that key
	bool contains(const E &element) const
	{
		return m_handles[element] != nullptr;
	}

	/*
	 * Returns the priority of the key.
	 * Note, that the behaviour is undefined if the key is not present.
	 */
	const P &priority(const E &element) const
	{
		OGDF_ASSERT(contains(element));
		return this->value(m_handles[element]).priority();
	}

	/**
	 * Adds a new element to the queue.
	 * Note, that the behaviour is undefined if the key is already present.
	 */
	void push(const E &element, const P &priority) {
		OGDF_ASSERT(m_handles[element] == nullptr);
		m_handles[element] = SuperQueue::push(element, priority);
	}

	/**
	 * Removes the topmost element from the queue.
	 */
	void pop() {
		m_handles[SuperQueue::topElement()] = nullptr;
		SuperQueue::pop();
	}

	/**
	 * Decreases the priority of the given element.
	 * Note, that the behaviour is undefined if the key is not present
	 * or the given priority is greater than the former one.
	 */
	void decrease(const E &element, const P &priority) {
		Handle pos = m_handles[element];
		SuperQueue::decrease(pos, priority);
	}

	//! Removes all elements from this queue.
	void clear() {
		SuperQueue::clear();
		m_handles.clear();
	}

protected:
	PrioritizedArrayQueueBase(const C &cmp, int initialSize, const Map &map)
	: SuperQueue(cmp, initialSize), m_handles(map)
	{
	}

	Map m_handles;
};

}

//! Prioritized queue interface wrapper for heaps.
/**
 * @ingroup containers
 *
 * Extends the default priority queue interface by adding
 * the functionality to store elements with assigned priorities.
 * The stored elements do not have to be unique.
 *
 *
 * @tparam E Denotes value type of inserted elements.
 * @tparam P Denotes the type of priority.
 * @tparam C Denotes the comparison functor for comparing priorities.
 * @tparam Impl Denotes the underlying heap class.
 */
template<
	typename E,
	typename P,
	class C=std::less<P>,
	template<typename, class> class Impl=PairingHeap
>
using PrioritizedQueue = pq_internal::PrioritizedQueue<E, P, C, Impl>;

//! Prioritized queue interface wrapper for heaps.
/**
 * @ingroup containers
 *
 * Much like PrioritizedQueue but each inserted element is treated as a key.
 * As such, the same element might not be inserted twice.
 * This interface does not require the user to keep track of handlers
 * for decreasing the priority of the respective elements.
 *
 * If ::node (or ::edge) is specified as the type of keys to be stored, a
 * ::ogdf::NodeArray (or ::ogdf::EdgeArray) is used internally.
 * These types should be chosen whenever possible.
 *
 * This queue does not support merge operations.
 *
 * @tparam E Denotes value type of inserted elements.
 * @tparam P Denotes the type of priority.
 * @tparam C Denotes the comparison functor for comparing priorities.
 * @tparam Impl Denotes the underlying heap class.
 * @tparam HashFunc The hashing function to be used if a ::ogdf::HashArray is used internally.
 */
template<
	typename E,
	typename P,
	class C=std::less<P>,
	template<typename, class> class Impl=PairingHeap,
	template<typename> class HashFunc=DefHashFunc
>
class PrioritizedMapQueue : public pq_internal::PrioritizedArrayQueueBase<E, P, C, Impl, HashArray<E, typename PrioritizedQueue<E, P, C, Impl>::Handle, HashFunc<E>>>
{
private:
	using CustomHashArray = HashArray<E, typename PrioritizedQueue<E, P, C, Impl>::Handle, HashFunc<E>>;
	using SuperQueue = pq_internal::PrioritizedArrayQueueBase<E, P, C, Impl, CustomHashArray>;

public:

	/**
	 * Creates a new queue with the given comparer.
	 *
	 * @param cmp The comparer to be used.
	 * @param initialSize The initial capacity preference (ignored if not supported by underlying heap).
	 */
	PrioritizedMapQueue(const C &cmp = C(), int initialSize = 128)
	: SuperQueue(cmp, initialSize, CustomHashArray(nullptr))
	{
	}
};

//! Specialization for ::node elements.
template<
	typename P,
	class C,
	template<typename, class> class Impl,
	template<typename> class HashFunc
>
class PrioritizedMapQueue<node, P, C, Impl, HashFunc> : public pq_internal::PrioritizedArrayQueueBase<node, P, C, Impl, NodeArray<typename PrioritizedQueue<node, P, C, Impl>::Handle>>
{
private:
	using Handle = typename PrioritizedQueue<node, P, C, Impl>::Handle;
	using SuperQueue = pq_internal::PrioritizedArrayQueueBase<node, P, C, Impl, NodeArray<Handle>>;

public:

	/**
	 * Creates a new queue with the given comparer.
	 *
	 * @param G The graph containing the nodes that will be inserted.
	 * @param cmp The comparer to be used.
	 * @param initialSize The initial capacity preference (ignored if not supported by underlying heap).
	 *        Defaults to the initial number of nodes in the given Graph.
	 */
	PrioritizedMapQueue(const Graph &G, const C &cmp = C(), int initialSize = -1)
	: SuperQueue(cmp, initialSize == -1 ? G.numberOfNodes() : initialSize, NodeArray<Handle>(G, nullptr))
	{
	}

	//! Removes all elements from this queue.
	void clear() {
		SuperQueue::SuperQueue::clear();
		this->m_handles.init(*this->m_handles.graphOf(), nullptr);
	}
};

//! Specialization for ::edge elements.
template<
	typename P,
	class C,
	template<typename, class> class Impl,
	template<typename> class HashFunc
>
class PrioritizedMapQueue<edge, P, C, Impl, HashFunc> : public pq_internal::PrioritizedArrayQueueBase<edge, P, C, Impl, EdgeArray<typename PrioritizedQueue<edge, P, C, Impl>::Handle>>
{
private:
	using Handle = typename PrioritizedQueue<edge, P, C, Impl>::Handle;
	using SuperQueue = pq_internal::PrioritizedArrayQueueBase<edge, P, C, Impl, EdgeArray<typename PrioritizedQueue<edge, P, C, Impl>::Handle>>;

public:

	/**
	 * Creates a new queue with the given comparer.
	 *
	 * @param G The graph containing the edges that will be inserted.
	 * @param cmp The comparer to be used.
	 * @param initialSize The initial capacity preference (ignored if not supported by underlying heap).
	 *        Defaults to the initial number of edges in the given Graph.
	 */
	PrioritizedMapQueue(const Graph &G, const C &cmp = C(), int initialSize = -1)
	: SuperQueue(cmp, initialSize == -1 ? G.numberOfEdges() : initialSize, EdgeArray<Handle>(G, nullptr))
	{
	}

	//! Removes all elements from this queue.
	void clear() {
		SuperQueue::SuperQueue::clear();
		this->m_handles.init(*this->m_handles.graphOf(), nullptr);
	}
};


}
