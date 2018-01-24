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

#include <ogdf/lib/abacus/poolslotref.h>
#include <ogdf/lib/abacus/poolslot.h>
#include <ogdf/lib/abacus/cutbuffer.h>
#include <ogdf/lib/abacus/variable.h>
#include <ogdf/lib/abacus/constraint.h>
//#include <ogdf/lib/abacus/sorter.h>

#include <ogdf/basic/comparer.h>

namespace abacus {


template<class BaseType, class CoType>
CutBuffer<BaseType, CoType>::~CutBuffer()
{
	for (int i = 0; i < n_; i++) {
		psRef_[i]->conVar()->unlock();
		delete psRef_[i];
	}
}


template<class BaseType, class CoType>
int CutBuffer<BaseType, CoType>::insert(
	PoolSlot<BaseType, CoType> *slot,
	bool keepInPool)
{
	if (n_ == size())
		return 1;
	else {
		psRef_[n_]      = new PoolSlotRef<BaseType, CoType>(slot);
		keepInPool_[n_] = keepInPool;
		ranking_        = false;
		slot->conVar()->lock();
		++n_;
		return 0;
	}
}


template<class BaseType, class CoType>
int CutBuffer<BaseType, CoType>::insert(
	PoolSlot<BaseType, CoType> *slot,
	bool keepInPool,
	double rank)
{
	if (n_ == size())
		return 1;
	else {
		psRef_[n_]      = new PoolSlotRef<BaseType, CoType>(slot);
		keepInPool_[n_] = keepInPool;
		rank_[n_]       = rank;
		++n_;
		slot->conVar()->lock();
		return 0;
	}
}


template<class BaseType, class CoType>
void CutBuffer<BaseType, CoType>::remove(ArrayBuffer<int> &index)
{
	PoolSlotRef<BaseType, CoType> *psr;

	const int nIndex = index.size();

	for (int i = 0; i < nIndex; i++) {
		psr = psRef_[index[i]];
		psr->conVar()->unlock();
		PoolSlot<BaseType, CoType> *ps = psr->slot();
		delete psr;
		if (ps->conVar()->deletable())
			ps->removeConVarFromPool();
	}
	psRef_.leftShift(index);
	keepInPool_.leftShift(index);
	rank_.leftShift(index);

	n_ -= nIndex;
}


template<class BaseType, class CoType>
void CutBuffer<BaseType, CoType>::sort(int threshold)
{
	if (ranking_) {
		if (n_ > threshold) {
			// sort the buffered items
		/*	AbaSorter<int, double> sorter;
			Array<int>          index(n_);
			Array<double>       keys(n_);

			for (int i = 0; i < n_; i++) {
				index[i] = i;
				keys[i]  = -rank_[i];
			}

			sorter.quickSort(n_, index, keys);
		*/
			Array< ogdf::Prioritized<int> > things(n_);
			for (int i = 0; i < n_; i++) {
				things[i].setItem(i);
				things[i].setPriority(-rank_[i]);
			}
			things.quicksort();


			// reorder the buffered items
			Array<PoolSlotRef<BaseType, CoType>*> psRefSorted(n_);
			Array<bool> keepInPoolSorted(n_);

			for (int i = 0; i < n_; i++) {
				psRefSorted[i]      = psRef_[things[i].item()];
				keepInPoolSorted[i] = keepInPool_[things[i].item()];
			}

			for (int i = 0; i < n_; i++) {
				psRef_[i]      = psRefSorted[i];
				keepInPool_[i] = keepInPoolSorted[i];
			}

			Logger::ilout(Logger::Level::Minor) << "\titems ranked: accepted in " << -things[0].priority() << " ... "
			 << -things[threshold - 1].priority() << ", rejected in "
			 << -things[threshold].priority() << " ... " << -things[n_ - 1].priority() << std::endl;

		}
		else
			Logger::ilout(Logger::Level::Minor) << "\tnot enough items, no ranking required" << std::endl;
	}
	else
		Logger::ilout(Logger::Level::Minor) << "\tranking of buffered items not possible" << std::endl;
}


template<class BaseType, class CoType>
void CutBuffer<BaseType, CoType>::extract(
	int max,
	ArrayBuffer<PoolSlot<BaseType, CoType>*> &newSlots)
{
	// unlock the buffered items
	for (int i = 0; i < n_; i++)
		psRef_[i]->conVar()->unlock();

	// determine the number of items to extract
	int nExtract;

	if (n_ < max) nExtract = n_;
	else          nExtract = max;

	// delete the nonextracted items
	/* We have to check if the constraint/variable can be deleted, because the
	* pool slot might be shared with another constraint/variable that is not
	* deleted.
	*
	* The deletion of the extracted items must be performed before the
	* deletion of the non-extracted ones. Otherwise if a \a NONDUPLPOOL
	* is used, it can happen that a constraint is removed from the pool
	* that is the duplicate of an extracted one.
	*/
	PoolSlot<BaseType, CoType> *s;

	for (int i = nExtract; i < n_; i++) {
		if (!keepInPool_[i]) {
			s = psRef_[i]->slot();
			delete psRef_[i];
			if (s->conVar()->deletable())
				s->removeConVarFromPool();
		}
		else delete psRef_[i];
	}

	n_ = 0;

	// extract the items
	for (int i = 0; i < nExtract; i++) {
		newSlots.push(psRef_[i]->slot());
		delete psRef_[i];
	}

	// allow ranking in next iteration
	ranking_ = true;
}

}
