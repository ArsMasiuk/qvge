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

#include <ogdf/lib/abacus/pool.h>
#include <ogdf/lib/abacus/poolslot.h>
#include <ogdf/lib/abacus/master.h>

namespace abacus {


template <class BaseType, class CoType> class  Active;


//! Base class for constraint/variabe pools.
/**
 * Every constraint and variable has to to be stored in a pool. This class
 * implements an abstract template class for a pool, which can be
 * used to store objects of the class Variable or of the class Constraint.
 * A constraint or variable is not directly stored in the pool, but in
 * an PoolSlot. Hence, a pool is a collection of pool slots.
 *
 * A pool has two template arguments: the \a BaseType and the \a CoType.
 * Only two scenarios make sense in the current context. For a pool
 * storing constraints the \a BaseType is Constraint and the \a CoType
 * is Variable. For a pool storing variables the \a BaseType is
 * Variable and the corresponding \a CoType is Constraint.
 *
 * The class Pool is an abstract class from which concrete classes
 * have to be derived, implementing the data structures for the storage
 * of pool slots. We provide already in the class StandardPool
 * a simple but convenient implementation of a pool.
 * We refer to all constraints and variables via the class PoolSlotRef.
 */
template<class BaseType, class CoType>
class  Pool :  public AbacusRoot  {
public:

	//! Determines how th rank of a constraint/variable is computed.
	/**
	 * The enumeration \a RANKING indicates how the rank of a
	 * constraint/variable in a pool separation is determined.
	 * This enumeration is not used at the moment because
	 * we cannot use the enumeration as the type of a function
	 * parameter in a derived class.
	 */
	enum RANKING {
		//! No rank is computed.
		NO_RANK,
		//! The violation computed by the function \a violated() of the classes
		//! Constraint or Variable is used as rank.
		RANK,
		//! The absolute value of the violation is taken as rank.
		ABS_RANK
	};

	//! Initializes an empty pool.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 */
	Pool(Master *master) : master_(master), number_(0) { }

	virtual ~Pool() { }

	/*!\brief Tries to insert a constraint/variable in the pool.
	 *
	 * \return A pointer to the pool slot where the item has been
	 * inserted, or 0 if the insertion failed.
	 *
	 * \param cv The constraint/variable being inserted.
	 */
	virtual PoolSlot<BaseType, CoType> *insert(BaseType *cv) = 0;

	/*! \brief Removes the constraint/variable stored in
	 * a pool slot and adds the slot to the list of free slots.
	 *
	 * \param slot The pool slot from which the constraint/variable is removed.
	 */
	void removeConVar(PoolSlot<BaseType, CoType> *slot) {
		if (softDeleteConVar(slot)) {
			Logger::ifout() << "removeConVar(): removing constraint from slot failed\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Pool);
		}
	}

	//! Returns the current number of items in the pool.
	int number() const { return number_; }

	//! Checks if a pair of a vector and an active constraint/variable set violates any item in the pool.
	/**
	 * If the pool is a constraint pool, then the vector is an LP-solution and the active set
	 * is the set of active variables. Otherwise, if the pool is a variable pool, then
	 * the vector contains the dual variables and the active set
	 * is the set of associated active constraints.
	 *
	 * \param z         The vector for which violation is checked.
	 * \param active    The constraint/variable set associated with \a z.
	 * \param sub       The subproblem for which validity of the violated
	 *                  item is required.
	 * \param cutBuffer The violated constraints/variables are added to this buffer.
	 * \param minAbsViolation A violated constraint/variable is only added to the \a cutBuffer
	 *                  if the absolute value of its violation is at least
	 *                  \a minAbsViolation. The default value is \a 0.001.
	 * \param ranking   If 1, the violation is associated with a rank of item in the buffer,
	 *                  if 2 the absolute violation is used, if 0 no rank is associated with the item.
	 *
	 * \return The number of violated items.
	 */
	virtual int separate(
		double *z,
		Active<CoType, BaseType> *active,
		Sub *sub,
		CutBuffer<BaseType, CoType> *cutBuffer,
		double minAbsViolation = 0.001,
		int ranking = 0) = 0;

protected:

	//! Removes the constraint/variable stored in \a slot from the pool if it can be deleted.
	/**
	 * If the constraint/variable can be removed, the slot is added to the set of free slots.
	 *
	 * \param slot A pointer to the pool slot from wich the constraint/variable should be deleted.
	 *
	 * \return 0 if the constraint/variable could be deleted.
	 * \return 1 otherwise.
	 */
	virtual int softDeleteConVar(PoolSlot<BaseType, CoType> *slot) {
		if (slot->softDelete() == 0) {
			putSlot(slot);
			--number_;
			return 0;
		}
		return 1;
	}

	//! Removes a constraint/variable from the pool and adds the slot to the set of free slots.
	/**
	 * \param slot A pointer to the pool slot from wich the constraint/variable
	 *             should be deleted.
	 */
	virtual void hardDeleteConVar(PoolSlot<BaseType, CoType> *slot) {
		--number_;
		slot->hardDelete();
		putSlot(slot);
	}

	//! Tries to find a free slot in the pool.
	/**
	 * This function is protected since it should only be used
	 * by \a insert(). The data structure managing the free poolslots
	 * can be individually defined for each derived pool class.
	 *
	 * \return A pointer to a free PoolSlot where a constraint/variable can be inserted.
	 *         If no pool slot is available \a getSlot() returns 0.
	 */
	virtual PoolSlot<BaseType, CoType> *getSlot() = 0;

	//! Makes an PoolSlot again available for later calls of \a getSlot().
	/**
	 * If somebody else refers to this constraint the function should abort with an error message.
	 *
	 * \param slot The slot made available for further use.
	 */
	virtual void putSlot(PoolSlot<BaseType, CoType> *slot) = 0;

	Master *master_; //!<  A pointer to the corresponding master of the optimization.
	int number_; //!< The current number of constraints in the pool.
};

}
