/*!\file
 * \author Matthias Elf
 * \brief poolslot.
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

#include <ogdf/lib/abacus/abacusroot.h>
#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/constraint.h>
#include <ogdf/lib/abacus/variable.h>

namespace abacus {

class Sub;

template<class BaseType, class CoType> class Pool;
template<class BaseType, class CoType> class PoolSlotRef;
template<class BaseType, class CoType> class StandardPool;
template<class BaseType, class CoType> class NonDuplPool;
template<class BaseType, class CoType> class CutBuffer;


//! Stores constraints and variables.
/**
 * Constraints or variables are not directly stored in a pool, but in pool
 * slots, which form again the basic building blocks of
 * a pool. The reason is that in order to save memory it can be necessary
 * that a constraint or variable in the pool has to be deleted although
 * it is still contained in the active formulation of an inactive subproblem.
 * Of course this deletion can be only done with constraints/variables which
 * can be regenerated or which are not required for the correctness of
 * the algorithm (e.g., for a cutting plane, but not for a variable or
 * constraint of the problem formulation of a general mixed integer optimization problem).
 *
 * Such that the deletion of a variable or constraint cannot cause a
 * run-time error, we store it in a pool slot. Together with the pointer
 * to the constraint/variable we store also its version number. The version
 * number is initially zero and incremented each time when a new item is
 * inserted in the pool slot. When we refer to a constraint/variable,
 * e.g., from the sets of active constraints or variables, then we
 * point to the slot and memorize the version number (class PoolSlotRef),
 * when this reference has been set up. Later by comparing the version
 * number of PoolSlotRef and the one of the corresponding PoolSlot
 * we can check if still the constraint/variable is contained in the slot
 * which is supposed to be there. Usually, if the expected constraint/variable
 * is missing, it is ignored.
 *
 * \warning A PoolSlot must not be deleted before the termination
 * of the optimization process, except that it can be guaranteed that
 * there is no reference to this slot from any other place of the program.
 */
template<class BaseType, class CoType> class OGDF_EXPORT PoolSlot : public AbacusRoot {

	friend class PoolSlotRef<BaseType,CoType>;
	friend class Pool<BaseType,CoType>;
	friend class StandardPool<BaseType,CoType>;
	friend class CutBuffer<BaseType,CoType>;

	friend class Sub;
	friend class PoolSlotRef<Constraint, Variable>;
	friend class PoolSlotRef<Variable, Constraint>;
	friend class Pool<Constraint, Variable>;
	friend class Pool<Variable, Constraint>;
	friend class StandardPool<Constraint, Variable>;
	friend class StandardPool<Variable, Constraint>;
	friend class NonDuplPool<Constraint, Variable>;
	friend class NonDuplPool<Variable, Constraint>;
	friend class CutBuffer<Constraint, Variable>;
	friend class CutBuffer<Variable, Constraint>;

public:

	//! Creates a pool slot and inserts \a convar.
	/**
	 * Sets the version number to 1, if a constraint has already been inserted
	 * in this slot, 0 otherwise.
	 *
	 * \param master A pointer to the corresponding master of the optimization.
	 * \param pool   The pool this slot belongs to.
	 * \param convar The constraint/variable inserted in this slot
	 *               if not 0. The default value is 0.
	 */
	PoolSlot(Master *master,
		Pool<BaseType, CoType> *pool,
		BaseType *convar = 0);

	~PoolSlot();

	//! Returns a pointer to the constraint/variable in the pool slot.
	BaseType *conVar() const { return conVar_; }


private:

	//! Inserts a constraint/variable in the slot and updates the version number.
	/**
	 * If the slot still contains a constraint, the program stops.
	 *
	 * \param convar The constraint/variable that is inserted.
	 */
	void insert(BaseType *convar);

	//! Tries to remove the item from the slot.
	/**
	 * This is possible if the function ConVar::deletable() returns \a true.
	 *
	 * \return 0 if the constraint/variable in the slot could be deleted, 1 otherwise.
	 */
	int softDelete() {
		if (conVar_ == nullptr)
			return 0;
		if (conVar_->deletable() == false)
			return 1;
		hardDelete();
		return 0;
	}

	//! Deletes the constraint/variable in the slot.
	/**
	 * \warning This function should be used very carefully.
	 */
	void hardDelete() {
		delete conVar_;
		conVar_ = nullptr;
	}

	//! Removes the constraint contained in this slot from its pool.
	void removeConVarFromPool() {
		pool_->removeConVar(this);
	}

	//! Return the version number of the constraint/variable in the slot.
	unsigned long version() const { return version_; }

	//! Returns a pointer to the corresponding master of the optimization.
	Master *master() { return master_; }

	//! Returns a const pointer to the corresponding master of the optimization.
	const Master *master() const { return master_; }

	Master *master_;	//!< A pointer to the corresponding master of the optimization.
	BaseType *conVar_;		//!< A pointer to the constraint/variable.
	unsigned long version_;	//!< The version of the constraint in the slot.
	Pool<BaseType, CoType> *pool_; //!< A pointer to the corresponding pool.


	PoolSlot(const PoolSlot<BaseType, CoType> &rhs);
	const PoolSlot<BaseType, CoType>
		&operator=(const PoolSlot<BaseType, CoType> &rhs);

	OGDF_NEW_DELETE
};

}

#include <ogdf/lib/abacus/poolslot.inc>
