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

#include <ogdf/lib/abacus/poolslot.h>
#include <ogdf/lib/abacus/pool.h>
#include <ogdf/lib/abacus/convar.h>

namespace abacus {

template<class BaseType, class CoType>
PoolSlot<BaseType, CoType>::PoolSlot(
	Master *master,
	Pool<BaseType, CoType> *pool,
	BaseType *convar) : master_(master), conVar_(convar), pool_(pool)
{
	version_ = (convar) ? 1 : 0;
}


template<class BaseType, class CoType>
PoolSlot<BaseType, CoType>::~PoolSlot()
{
#ifdef OGDF_DEBUG
	if (conVar_ && conVar_->nReferences()) {
		Logger::ifout() << "~PoolSlot(): it is not allowed to destruct objects of class PoolSlot with a constraint/variable with positive reference counter = " << conVar_->nReferences() << ".\n";
	}

#ifndef OGDF_USE_ASSERT_EXCEPTIONS // do not throw exceptions in destructor
	OGDF_ASSERT((conVar_ && conVar_->nReferences()) == false);
#endif
#endif

	delete conVar_;
}


template<class BaseType, class CoType>
void PoolSlot<BaseType, CoType>::insert(BaseType *convar)
{
	if (conVar_ != nullptr) {
		Logger::ifout() << "PoolSlot::insert(): insertion failed, the slot is not void\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Poolslot);
	}

	if (version_ == std::numeric_limits<unsigned long>::max()) {
		Logger::ifout() << "PoolSlot::insert(): insertion failed, maximum version number ULONG_MAX reached\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Poolslot);
	}

	conVar_ = convar;
	++version_;
}

}
