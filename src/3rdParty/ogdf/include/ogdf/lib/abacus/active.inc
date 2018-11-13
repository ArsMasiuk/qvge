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

#include <ogdf/lib/abacus/active.h>
#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/poolslotref.h>
#include <ogdf/lib/abacus/sparvec.h>
#include <ogdf/lib/abacus/convar.h>
#include <ogdf/lib/abacus/poolslot.h>

namespace abacus {

template <class BaseType, class CoType>
Active<BaseType, CoType>::Active(
	Master *master,
	Active<BaseType, CoType> *a,
	int max)
	:
master_(master),
	n_(0),
	active_(max),
	redundantAge_(0,max-1, 0)
{
	n_ = (max < a->number()) ? max : a->number();

	for (int i = 0; i < n_; i++)
		active_[i] = new PoolSlotRef<BaseType, CoType>(*(a->active_[i]));
}


template <class BaseType, class CoType>
Active<BaseType, CoType>::Active(const Active<BaseType, CoType> &rhs)
	:
	master_(rhs.master_),
	n_(rhs.n_),
	active_(/*rhs.master_,*/ rhs.max()),
	redundantAge_(/*master_*/0, rhs.max()-1, 0)
{
	for (int i = 0; i < n_; i++) {
		active_[i] = new PoolSlotRef<BaseType, CoType>(*(rhs.active_[i]));
		redundantAge_[i] = rhs.redundantAge_[i];
	}
}


template <class BaseType, class CoType>
Active<BaseType, CoType>::~Active()
{
	for (int i = 0; i < n_; i++)
		delete active_[i];
}


template <class BaseType, class CoType>
std::ostream &operator<<(std::ostream &out, const Active<BaseType, CoType> &rhs)
{
	for (int i = 0; i < rhs.n_; i++) {
		out << i << ": ";
		BaseType *cv = rhs.active_[i]->conVar();
		if (cv != 0)
			cv->print(out);
		else
			out << "void" << std::endl;
	}
	return out;
}


template <class BaseType, class CoType>
void Active<BaseType, CoType>::insert(PoolSlot<BaseType, CoType> *ps)
{
	OGDF_ASSERT(n_ != max()); // buffer is full

	active_[n_] = new PoolSlotRef<BaseType, CoType>(ps);
	redundantAge_[n_] = 0;
	n_++;
}


template <class BaseType, class CoType>
void Active<BaseType, CoType>::insert(
	ArrayBuffer<PoolSlot<BaseType, CoType> *> &ps)
{
	const int nPs = ps.size();

	for(int i = 0; i < nPs; i++)
		insert(ps[i]);
}


template <class BaseType, class CoType>
void Active<BaseType, CoType>::remove(ArrayBuffer<int> &del)
{
	const int nDel = del.size();

	for(int i = 0; i < nDel; i++)
		delete active_[del[i]];
	active_.leftShift(del);
	redundantAge_.leftShift(del);
	n_ -= nDel;
}


template <class BaseType, class CoType>
void Active<BaseType, CoType>::realloc(int newSize)
{
	active_.resize(newSize);
	redundantAge_.resize(newSize);
}

}
