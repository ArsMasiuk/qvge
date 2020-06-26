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

#include <ogdf/lib/abacus/separator.h>
#include <ogdf/lib/abacus/constraint.h>
#include <ogdf/lib/abacus/variable.h>

namespace abacus {


template <class BaseType, class CoType>
Separator_CUTFOUND
	Separator<BaseType, CoType>::cutFound(BaseType *cv)
{
	if(newCons_.full()) {
		delete cv;
		return Full;
	}

	if(pool_&&pool_->present(cv)) {
		delete cv;
		nDuplications_++;
		return Duplication;
	}

	if(hash_&&find(cv)) {
		delete cv;
		nDuplications_++;
		return Duplication;
	}


	newCons_.push(cv);
	if(hash_)
		hash_->insert(cv->hashKey(),cv);
	return Added;
}


template <class BaseType, class CoType>
int Separator<BaseType, CoType>::nCollisions() const
{
	if(!hash_)
		return 0;
	return hash_->nCollisions();
}


template <class BaseType, class CoType>
bool Separator<BaseType, CoType>::find(BaseType *cv)
{
	int key = cv->hashKey();

	BaseType **cand = hash_->initializeIteration(key);

	while(cand) {
		if (cv->equal(*cand)) return true;
		cand = hash_->next(key);
	}
	return false;
}

}
