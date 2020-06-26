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

#include <ogdf/lib/abacus/cutbuffer.h>
#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/poolslot.h>
#include <ogdf/lib/abacus/constraint.h>
#include <ogdf/lib/abacus/variable.h>
#include <ogdf/lib/abacus/sub.h>
#include <ogdf/lib/abacus/bheap.h>

namespace abacus {

template<class BaseType, class CoType>
StandardPool<BaseType, CoType>::StandardPool(
	Master *master,
	int size,
	bool autoRealloc)
	:
Pool<BaseType, CoType>(master),
	pool_(size),
	autoRealloc_(autoRealloc)
{
	for (int i = 0; i < size; i++) {
		pool_[i] = new PoolSlot<BaseType, CoType>(master, this);
		freeSlots_.pushBack(pool_[i]);
	}
}


template<class BaseType, class CoType>
StandardPool<BaseType, CoType>::~StandardPool()
{
	const int s = size();

	for (int i = 0; i < s; i++)
		delete pool_[i];
}


template<class BaseType, class CoType>
std::ostream &operator<<(std::ostream &out, const StandardPool<BaseType, CoType> &rhs)
{
	const int s = rhs.size();

	for (int i = 0; i < s; i++) {
		if (rhs.pool_[i]->conVar()) {
			out << i << ": ";
			rhs.pool_[i]->conVar()->print(out);
			out << std::endl;
		}
	}

	return out;
}


template<class BaseType, class CoType>
PoolSlot<BaseType, CoType> * StandardPool<BaseType, CoType>::insert(
	BaseType *cv)
{
	PoolSlot<BaseType, CoType>* slot = getSlot();
	if (slot == nullptr) {
		if(cleanup() == 0) {
			if (autoRealloc_)
				increase((int) (size()*1.1 + 1));
			else {
				if (removeNonActive(size()/10 + 1) == 0)
					return nullptr;
			}
		}
		slot = getSlot();
	}

	slot->insert(cv);
	++Pool<BaseType, CoType>::number_;
	return slot;
}


template<class BaseType, class CoType>
void StandardPool<BaseType, CoType>::increase(int size)
{
	int oldSize = pool_.size();

	if (size < oldSize) {
		Logger::ifout() << "StandardPool::increase(): the pool size cannot be decreased.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::StandardPool);
	}

	pool_.resize(size);

	for(int i = oldSize; i < size; i++) {
		pool_[i] = new PoolSlot<BaseType, CoType>(Pool<BaseType, CoType>::master_, this);
		freeSlots_.pushBack(pool_[i]);
	}
}


template<class BaseType, class CoType>
int StandardPool<BaseType, CoType>::cleanup()
{
	int nDeleted = 0;

	for(int i = 0; i < Pool<BaseType, CoType>::number(); i++)
	{
		if(this->softDeleteConVar(pool_[i]) == 0)
		{
			nDeleted++;
			// consider the case that a slot has been deleted although it was empty
			// in softDeleteConVar(), number_ was decreased by 1
			if (i != Pool<BaseType, CoType>::number())
			{
				//exchange the current slot with the last slot of the pool
				PoolSlot<BaseType, CoType> *CMslot = pool_[i];
				pool_[i] = pool_[Pool<BaseType, CoType>::number()];
				pool_[Pool<BaseType, CoType>::number()] = CMslot;
				i--; // decrease i in order to consider the new current slot
			}
		}
	}

	Logger::ilout(Logger::Level::Minor) << "StandardPool::cleanup(): " << nDeleted << " items removed." << std::endl;
	return nDeleted;

}

template<class BaseType, class CoType>
int StandardPool<BaseType, CoType>::removeNonActive(int maxRemove)
{
	//! prepare the heap storing the candidates
	ArrayBuffer<int> elems(size(),false);
	ArrayBuffer<int> keys(size(),false);

	const int s = size();

	for (int i = 0; i < s; i++) {
		BaseType *cv = pool_[i]->conVar();
		if (cv && !cv->active() && !cv->locked()) {
			elems.push(i);
			keys.push(cv->nReferences());
		}
	}

	AbaBHeap<int, int> candidates(elems, keys);

	//! remove the items with minimal reference counter from the pool
	/*!  Only those items in the pool are candidates which are neither active nor
	*  locked.
	*/
	int nRemoved = 0;

	while(nRemoved < maxRemove && !candidates.empty()) {
		int c = candidates.extractMin();
		this->hardDeleteConVar(pool_[c]);
		nRemoved++;
	}

	Logger::ilout(Logger::Level::Minor) << nRemoved << " inactive items removed from pool." << std::endl;

	return nRemoved;
}


template<class BaseType, class CoType>
int StandardPool<BaseType, CoType>::separate(
	double *z,
	Active<CoType, BaseType> *active,
	Sub *sub,
	CutBuffer<BaseType, CoType> *cutBuffer,
	double minAbsViolation,
	int ranking)
{
	double    violation;
	int       oldSep = cutBuffer->number();

	Logger::ilout(Logger::Level::Minor) << "StandardPool::separate(): " << "size = " << size() << " n = " << Pool<BaseType, CoType>::number_;

	PoolSlot<BaseType, CoType> *slot;
	const int s = size();

	for (int i = 0; i < s; i++) {
		slot = pool_[i];
		BaseType *cv = slot->conVar();
		if (cv && !cv->active() && (cv->global() || cv->valid(sub)))
			if (cv->violated(active, z, &violation) && fabs(violation) > minAbsViolation) {
				if (ranking == 0) {
					if (cutBuffer->insert(slot, true))
						break;
				}
				else if (ranking == 1) {
					if (cutBuffer->insert(slot, true, violation))
						break;
				}
				else if (ranking == 2) {
					if (cutBuffer->insert(slot, true, fabs(violation)))
						break;
				}
				else if (ranking == 3) {
					if (cutBuffer->insert(slot, true, cv->rank()))
						break;
				}
			}
	}

	Logger::ilout(Logger::Level::Minor) << " generated = " << cutBuffer->number() - oldSep << std::endl;
	return cutBuffer->number() - oldSep;
}

}
