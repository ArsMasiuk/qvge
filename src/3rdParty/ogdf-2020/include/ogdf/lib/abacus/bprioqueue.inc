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
AbaPrioQueue<Type, Key>::AbaPrioQueue(int size) : heap_(size)
{ }


template<class Type, class Key>
void AbaPrioQueue<Type, Key>::insert(Type elem, Key key)
{
	heap_.insert(elem, key);
}


template<class Type, class Key>
int AbaPrioQueue<Type, Key>::getMin(Type &min) const
{
	//! AbaPrioQueue: is the priority queue empty?
	if (heap_.empty()) return 1;

	min = heap_.getMin();
	return 0;
}


template<class Type, class Key>
int AbaPrioQueue<Type, Key>::getMinKey(Key &minKey) const
{
	//! AbaPrioQueue: is the priority queue empty?
	if (heap_.empty()) return 1;

	minKey = heap_.getMinKey();
	return 0;
}


template<class Type, class Key>
int AbaPrioQueue<Type, Key>::extractMin(Type& min)
{
	//! AbaPrioQueue: is the priority queue empty?
	if (heap_.empty()) return 1;

	min = heap_.extractMin();
	return 0;
}


template<class Type, class Key>
void AbaPrioQueue<Type, Key>::clear()
{
	heap_.clear();
}


template<class Type, class Key>
inline int AbaPrioQueue<Type, Key>::size() const
{
	return heap_.size();
}


template<class Type, class Key>
inline int AbaPrioQueue<Type, Key>::number() const
{
	return heap_.number();
}


template<class Type, class Key>
void AbaPrioQueue<Type, Key>::realloc(int newSize)
{
	if (newSize < size()) {
		Logger::ifout() << "AbaPrioQueue::realloc : priority queue cannot be decreased\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::BPrioQueue);
	}

	heap_.realloc(newSize);
}

}
