/*!\file
 * \author Matthias Elf
 * \brief standard pool without constraint duplication.
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

#include <ogdf/lib/abacus/hash.h>
#include <ogdf/lib/abacus/standardpool.h>
#include <ogdf/lib/abacus/cutbuffer.h>
#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/poolslot.h>
#include <ogdf/lib/abacus/constraint.h>
#include <ogdf/lib/abacus/variable.h>
#include <ogdf/lib/abacus/sub.h>
#include <ogdf/lib/abacus/bheap.h>

namespace abacus {

//! Standard pools without constraint duplication.
/**
 * The class NonDuplPool provides an StandardPool with the additional feature
 * that the same constraint is at most stored once in the pool. For constraints
 * and variables inserted in this pool the virtual member functions
 * \a name(), \a hashKey(), and \a equal() of the base class ConVar have to be
 * defined. Using these three functions, we check at insertation time
 * if a constraint or variable is already stored in the pool.
 *
 * The implementation is unsafe in the sense that the data structure for
 * registering a constraint is corrupted if a constraint is removed directly
 * from the pool slot without using a function of this class.
 */
template<class BaseType, class CoType>
class  NonDuplPool :  public StandardPool<BaseType, CoType>  {
public:

	//! Creates an empty pool.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 * \param size   The maximal number of items which can be inserted in
	 *               the pool without reallocation.
	 * \param autoRealloc If this argument is \a true an automatic reallocation
	 *               is performed if the pool is full.
	 */
	NonDuplPool(Master *master, int size, bool autoRealloc = false)
		: StandardPool<BaseType, CoType>(master, size, autoRealloc), hash_(size), nDuplications_(0)
	{ }

	//! The destructor.
	virtual ~NonDuplPool() {
#ifdef OGDF_DEBUG
		Logger::ifout() << "Number of duplicated constraints: " <<
			nDuplications_ << std::endl;
#endif
	}

	//! Insert constraint/variable \a cv in the pool.
	/**
	 * Before the function \a insert() tries to insert a constraint/variable
	 * in the pool, it checks if the constraint/variable is already contained in the
	 * pool. If the constraint/variable \a cv is contained in the pool,
	 * it is deleted.
	 *
	 * \param cv The constraint/variable being inserted.
	 *
	 * \return A pointer to the pool slot where the item has been inserted, or a pointer
	 *         to the pool slot if the item is already contained in the pool, or 0 if the insertion failed.
	 */
	virtual PoolSlot<BaseType, CoType> *insert(BaseType *cv);

	//! Checks if constraint/variables \a cv is already contained in the pool.
	/**
	 * \param cv A pointer to a constraint/variable for which it should
	 *           be checked if an equivalent item is already contained in the pool.
	 *
	 * \return A pointer to the pool slot storing a constraint/variable that is equivalent
	 *         to \a cv according to the function ConVar::equal(). If there is no
	 *         such constraint/variable 0 is returned.
	 */
	virtual PoolSlot<BaseType, CoType> *present(BaseType *cv);

	//! Checks if constraint/variables \a cv is already contained in the pool.
	/**
	 * \param cv A (const) pointer to a constraint/variable for which it should
	 *           be checked if an equivalent item is already contained in the pool.
	 *
	 * \return A const pointer to the pool slot storing a constraint/variable that is equivalent
	 *         to \a cv according to the function ConVar::equal(). If there is no
	 *         such constraint/variable 0 is returned.
	 */
	virtual const PoolSlot<BaseType, CoType> *present(const BaseType *cv) const;

	//! Enlarges the pool to store up to \a size items.
	/**
	 * To avoid fatal errors we do not allow decreasing the size of the pool.
	 * This function redefines the virtual function of the base class
	 * StandardPool because we have to reallocate the hash table.
	 *
	 * \param size The new size of the pool.
	 */
	virtual void increase(int size) {
		StandardPool<BaseType, CoType>::increase(size);
		hash_.resize(size);
	}

	//! Returns some statistic information in \a nDuplicates and \a nCollisions.
	/**
	 * Determines the number of constraints that have not been inserted into
	 * the pool, because an equivalent was already present.
	 *
	 * Also the number of collisions in the hash table is computed.
	 * If this number is high, it might indicate that your hash function is not
	 * chosen very well.
	 *
	 * \param nDuplications The number of constraints that have not been inserted
	 *                      into the pool because an equivalent one was already
	 *                      present.
	 * \param nCollisions   The number of collisions in the hash table.
	 */
	void statistics(int &nDuplications, int &nCollisions) const {
		nDuplications = nDuplications_;
		nCollisions   = hash_.nCollisions();
}

private:

	/**
	 * Has to be redefined because the slot has to be removed from the hash table if the
	 * constraint/variable can be deleted.
	 *
	 * \return 0 If the constraint/variable could be deleted, 1 otherwise.
	 *
	 * \param slot A pointer to the pool slot from wich the constraint/variable
	 *             should be deleted.
	 */
	virtual int softDeleteConVar(PoolSlot<BaseType, CoType> *slot);

	/**
	 * Has to be redefined because the pool slot has to be removed from the hash table.
	 *
	 * \param slot A pointer to the pool slot from wich the constraint/variable
	 *             should be deleted.
	 */
	virtual void hardDeleteConVar(PoolSlot<BaseType, CoType> *slot);

	//! A hash table for a fast access to the pool slot storing a constraint/variable.
	AbaHash<unsigned, PoolSlot<BaseType, CoType>*> hash_;

	//! The number of insertions of constraints/variables that were rejected since the
	//! constraint/variable is stored already in the pool.
	int nDuplications_;


	NonDuplPool(const NonDuplPool &rhs);
	const NonDuplPool &operator=(const NonDuplPool &rhs);
};

}

#include <ogdf/lib/abacus/nonduplpool.inc>
