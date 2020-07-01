/*!\file
 * \author Matthias Elf
 * \brief standard pool.
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

#include <ogdf/basic/SList.h>
#include <ogdf/lib/abacus/pool.h>

namespace abacus {


template<class BaseType, class CoType>
class StandardPool;

template<class BaseType, class CoType>
std::ostream& operator<< (std::ostream &out, const StandardPool<BaseType,CoType> &rhs);


//! Standard pools.
/**
 * This class is derived from the class Pool and provides a very
 * simple implementation of a pool which is sufficient for a large
 * class of applications. The pool slots are stored in an array
 * and the set of free slots is managed by a linear list.
 *
 * A standard pool can be static or dynamic. A static standard pool
 * has a fixed size, whereas a dynamic standard pool is automatically
 * enlarged by ten percent if it is full and an item is inserted.
 */
template<class BaseType, class CoType>
class StandardPool:public Pool<BaseType,CoType> {
public:

	//! Creates an empty pool.
	/**
	 * All slots are inserted in the linked list of free slots.
	 *
	 * \param master      A pointer to the corresponding master of the optimization.
	 * \param size        The maximal number of items which can be inserted in
	 *                    the pool without reallocation.
	 * \param autoRealloc If this argument is \a true an automatic reallocation
	 *                    is performed if the pool is full.
	 */
	StandardPool(Master*master,int size,bool autoRealloc= false);

	//! The destructor.
	/**
	 * Deletes all slots. The destructor of a pool slot
	 * deletes then also the respective constraint or variable.
	 */
	virtual ~StandardPool();

	//! Output operator for standard pools.
	/**
	 * The output operator calls the output operator of each item of a non-void pool slot.
	 *
	 * \param out The output stream.
	 * \param rhs The pool being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream& operator<< <> (std::ostream &out, const StandardPool &rhs);

	//! Tries to insert a constraint/variable in the pool.
	/**
	 * If there is no free slot available, we try to generate free slots by removing redundant
	 * items, i.e., items which have no reference to them.
	 * If this fails, we either perform an automatic reallocation of the pool or remove non-active items.
	 *
	 * \param cv The constraint/variable being inserted.
	 *
	 * \return A pointer to the pool slot where the item has been
	 *         inserted, or 0 if the insertion failed.
	 */
	virtual PoolSlot<BaseType,CoType> *insert(BaseType*cv);

	//! Enlarges the pool to store up to \a size items.
	/**
	 * To avoid fatal errors we do not allow decreasing the size of the pool.
	 *
	 * \param size The new size of the pool.
	 */
	virtual void increase(int size);

	//! Cleans-up the pool.
	/**
	 * Scans the pool, removes all deletable items, i.e., those items without having references, and
	 * adds the corresponding slots to the list of free slots.
	 *
	 * \return The number of "cleaned" slots.
	 */
	int cleanup();

	//! Return the maximal number of constraints/variables that can be inserted in the pool.
	int size() const { return pool_.size(); }

	//! Returns a pointer to the <i>i</i>-th slot in the pool.
	/**
	 * \param i The number of the slot being accessed.
	 */
	PoolSlot<BaseType,CoType> *slot(int i) { return pool_[i]; }

	//! Checks if a pair of a vector and an active constraint/variable set violates any item in the pool.
	/**
	 * If the pool is a constraint pool, then the vector is an LP-solution and the active set
	 * the set of active variables. Otherwise, if the pool is a variable pool, then
	 * the vector stores the values of the dual variables and the active set
	 * the associated active constraints.
	 *
	 * Before a constraint or variable is generated we check if it is valid for the subproblem \a sub.
	 *
	 * The function defines the pure virtual function of the base class Pool.
	 *
	 * This is a very simple version of the pool separation. Future versions
	 * might scan a priority queue of the available constraints until
	 * a limited number of constraints is tested or separated.
	 *
	 * \param x         The vector for which violation is checked.
	 * \param active    The constraint/variable set associated with \a z.
	 * \param sub       The subproblem for which validity of the violated item is required.
	 * \param cutBuffer The violated constraints/variables are added to this buffer.
	 * \param minAbsViolation A violated constraint/variable is only added to the \a cutBuffer if the absolute
	 *                  value of its violation is at least \a minAbsViolation.
	 *                  The default value is \a 0.001.
	 * \param ranking   If 1, the violation is associated with a rank of item in the buffer,
	 *                  if 2 the absolute violation is used, if 3 the function
	 *                  ConVar::rank() is used, if 0 no rank is associated with the item.
	 *
	 * \return The number of violated items.
	 */
	virtual int separate(
		double *x,
		Active<CoType,BaseType> *active,
		Sub*sub,
		CutBuffer<BaseType,CoType> *cutBuffer,
		double minAbsViolation = 0.001,
		int ranking = 0);

protected:

	//! Tries to remove at most \a maxRemove  inactive items from the pool.
	/**
	 * A minimum heap of the items with the reference counter as
	 * key is built up and items are removed in this order.
	 */
	int removeNonActive(int maxRemove);

	//! Returns a free slot, or 0 if no free slot is available.
	/**
	 * A returned slot is removed from the list of free slots.
	 *
	 * This function defines the pure virtual function of the base class Pool.
	 */
	virtual PoolSlot<BaseType,CoType> *getSlot() {
		return (freeSlots_.empty()) ? nullptr : freeSlots_.popFrontRet();
	}

	//! Inserts the \a slot in the list of free slots.
	/**
	 * It is an error to insert a slot which is not empty.
	 *
	 * This function defines the pure virtual function of the base class Pool.
	 */
	virtual void putSlot(PoolSlot<BaseType,CoType> *slot) {
		if (slot->conVar()) {
			Logger::ifout() << "StandardPool::putSlot(): you cannot put a non-void slot.\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::StandardPool);
		}
		freeSlots_.pushFront(slot);
	}

	Array<PoolSlot<BaseType,CoType> *> pool_; //!< The array with the pool slots.
	ogdf::SListPure<PoolSlot<BaseType,CoType> *> freeSlots_; //!< The linked lists of unused slots.

	/**
	 * If the pool becomes full and this member is \a true, then an automatic reallocation is performed.
	 */
	bool autoRealloc_;

private:

	StandardPool(const StandardPool&rhs);
	const StandardPool&operator= (const StandardPool&rhs);
};

}

#include <ogdf/lib/abacus/standardpool.inc>
