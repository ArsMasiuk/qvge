/*!\file
 * \author Matthias Elf
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

namespace abacus {


template<class Type, class Key>
AbaBHeap<Type, Key>::AbaBHeap(int size)
	:
	heap_(size),
	keys_(size),
	n_(0)
{ }


template<class Type, class Key>
AbaBHeap<Type, Key>::AbaBHeap(
	const ArrayBuffer<Type> &elems,
	const ArrayBuffer<Key> &keys)
	:
	heap_(elems),
	keys_(keys),
	n_(keys.size())
{
	for (int i = father(n_-1); i>=0; --i)
		heapify(i);
}


template<class Type, class Key>
std::ostream& operator<<(std::ostream& out, const AbaBHeap<Type, Key>& rhs)
{
	for(int i = 0; i < rhs.n_; i ++)
		out << rhs.heap_[i] << " (" << rhs.keys_[i] << ")  ";
	out << std::endl;
	return out;
}


template<class Type, class Key>
void AbaBHeap<Type, Key>::insert(Type elem, Key key)
{
	//! check if the heap is full
	OGDF_ASSERT(n_ < size());

	// go up towards the root and insert \a elem
	/* The position \a n_ of the array representing the heap becomes the
	* new leaf of corresponding binary tree. However, inserting the new
	* element at this position could destroy the heap property.
	* Therefore, we go up to the root until we find the position
	* for inserting the new element. While going up to this position
	* we move earlier inserted elements already to their new position.
	*/
	int i = n_;
	int f = father(i);

	while (i > 0 && keys_[f] > key) {
		heap_[i] = heap_[f];
		keys_[i] = keys_[f];
		i        = f;
		f        = father(i);
	}
	heap_[i] = elem;
	keys_[i] = key;
	++n_;
}


template<class Type, class Key>
Type AbaBHeap<Type, Key>::getMin() const
{
	// is the heap empty?
	/* The check on an empty heap is only performed if is the code
	* is compiled with the precompiler flag <tt>OGDF_DEBUG</tt>.
	*/
	OGDF_ASSERT(!empty());

	return heap_[0];
}


template<class Type, class Key>
Key AbaBHeap<Type, Key>::getMinKey() const
{
	// is the heap empty?
	/* The check on an empty heap is only performed if is the code
	* is compiled with the precompiler flag <tt>OGDF_DEBUG</tt>.
	*/
	OGDF_ASSERT(!empty());

	return keys_[0];
}


template<class Type, class Key>
Type AbaBHeap<Type, Key>::extractMin()
{
	Type min = getMin();

	--n_;

	if (n_ != 0) {
		heap_[0] = heap_[n_];
		keys_[0] = keys_[n_];
		heapify(0);
	}

	return min;
}


template<class Type, class Key>
void AbaBHeap<Type, Key>::clear()
{
	n_ = 0;
}


template<class Type, class Key>
inline int AbaBHeap<Type, Key>::size() const
{
	return heap_.size();
}


template <class Type, class Key>
inline int AbaBHeap<Type, Key>::number() const
{
	return n_;
}


template<class Type, class Key>
inline bool AbaBHeap<Type, Key>::empty() const
{
	if(n_ == 0) return true;
	return false;
}


template<class Type, class Key>
void AbaBHeap<Type, Key>::realloc(int newSize)
{
	heap_.realloc(newSize);
	keys_.realloc(newSize);
}


template<class Type, class Key>
void AbaBHeap<Type, Key>::check() const
{
	for(int i = 0; i < n_/2; i++)
		if (keys_[i] > keys_[leftSon(i)] || (rightSon(i) < n_ && heap_[i] > heap_[rightSon(i)])) {
			Logger::ifout() << "AbaBHeap:check(): " << i << " violates heap\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::BHeap);
		}
}


template <class Type, class Key>
inline int AbaBHeap<Type, Key>::leftSon(int i) const
{
	return 2*i + 1;
}


template <class Type, class Key>
int AbaBHeap<Type, Key>::rightSon(int i) const
{
	return 2*i + 2;
}


template <class Type, class Key>
inline int AbaBHeap<Type, Key>::father(int i) const
{
	return (i-1)/2;
}


template <class Type, class Key>
void AbaBHeap<Type, Key>::heapify(int i)
{
	//! AbaBHeap::heapify(): variables
	int  smallest; // smallest heap element of i,l, and r
	Type tmp;      // temporary object for swapping elements of the heap
	Key  ktmp;     // temporary object for swapping the keys

	// restore the heap property
	/* Node \a i violates the heap property if it has a son with
	* a smaller key. If we swap the element and the key of node \a i
	* with the element and the key of the smaller one of its two sons,
	* then the heap property is restored for node \a i. However, it
	* might be now destroyed for node \a smallest. So we
	* iterate this process until no swap is performed.
	*/
	while (i < n_) {
		//! find the \a smallest element of \a i, and its two sons
		int l = leftSon(i);
		int r = rightSon(i);
		if (l < n_ && keys_[i] > keys_[l])        smallest = l;
		else                                      smallest = i;
		if (r < n_ && keys_[smallest] > keys_[r]) smallest = r;

		if (smallest != i) {
			//! swap \a i and \a smallest
			tmp            = heap_[i];
			heap_[i]        = heap_[smallest];
			heap_[smallest] = tmp;

			ktmp           = keys_[i];
			keys_[i]        = keys_[smallest];
			keys_[smallest] = ktmp;

			i = smallest;
		}
		else break;
	}
}

}
