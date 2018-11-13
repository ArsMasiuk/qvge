/*!\file
 * \author Matthias Elf
 * \brief cutbuffer.
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

#include <ogdf/lib/abacus/master.h>

namespace abacus {

template<class BaseType, class CoType> class PoolSlot;
template<class BaseType, class CoType> class PoolSlotRef;


//! Cut buffers.
/**
 * This template class implements a buffer for constraints and variables which are
 * generated during the cutting plane or column generation phase.
 * There are two reasons why constraints/variables are buffered instead
 * of being added immediately. First, the set of active constraints/variables
 * should not be falsified during the cut/variable generation. Second,
 * optionally a rank can be assigned to each buffered item. Then
 * not all, but only the best items according to this rank are actually
 * added.
 */
template<class BaseType, class CoType>
class CutBuffer : public AbacusRoot  {

	friend class Sub;

public:

	//! Creates a cut buffer with capacity \a size.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 * \param size   The maximal number of constraints/variables which can be buffered.
	 */
	CutBuffer(Master *master, int size) :
		master_(master),
		n_(0),
		psRef_(size),
		keepInPool_(size),
		rank_(size),
		ranking_(true)
	{ }

	//! \brief The destructor.
	/**
	 * If there are still items buffered when this object is destructed then
	 * we have to unset the locks of the buffered items. This can happen if
	 * in the feasibility test constraints are generated but for some reason
	 * (e.g., due to tailing off) the optimization of the subproblem is
	 * terminated.
	 */
	~CutBuffer();

	//! Returns the maximal number of items that can be buffered.
	int size() const { return psRef_.size(); }

	//! Returns the number of buffered items.
	int number() const { return n_; }

	//! Returns the number of items which can still be inserted into the buffer.
	int space() const { return size() - n_; }

	//! Adds a slot to the buffer.
	/**
	 * The member \a ranking_ has to be set to \a false, because since no rank
	 * is added together with this item a ranking of all items is impossible.
	 * Such that newly generated items cannot be removed immediately in
	 * a cleaning up process of the pool we set a lock which will be
	 * removed in the function \a extract().
	 *
	 * \param slot       The inserted pool slot.
	 * \param keepInPool If the flag \a keepInPool is \a true, then the item stored in the
	 *                   \a slot is not removed from the pool, even if it is discarded in
	 *                   \a extract(). Items regenerated from a pool should always have
	 *                   this flag set to \a true.
	 *
	 * \return 0 If the item can be inserted, 1 if the buffer is already full.
	 */
	int insert(PoolSlot<BaseType, CoType> *slot, bool keepInPool);

	//! Adds a slot with rank to the buffer.
	/**
	 * In addition to the previous version of the function \a insert()
	 * this version also adds a rank to the item such that all buffered
	 * items can be sorted with the function \a sort().
	 *
	 * \return 0 If the item can be inserted, 1 if the buffer is already full.
	 *
	 * \param slot       The inserted pool slot.
	 * \param keepInPool If the flag \a keepInPool is \a true, then the item stored in the
	 *                   \a slot is not removed from the pool, even if it is
	 *                   discarded in \a extract(). Items regenerated from a
	 *                   pool should always have this flag set to \a true.
	 * \param rank       A rank associated with the constraint/variable.
	 */
	int insert(PoolSlot<BaseType, CoType> *slot, bool keepInPool, double rank);

	//! Removes the elements specified by \a index from the buffer.
	/**
	 * \param index The numbers of the elements which should be removed.
	 */
	void remove(ArrayBuffer<int> &index);

	//! Returns a pointer to the <i>i</i>-th PoolSlot that is buffered.
	PoolSlot<BaseType, CoType> *slot(int i) {
		return psRef_[i]->slot();
	}

private:

	//! Takes the first \a max items from the buffer and clears the buffer.
	/**
	 * Constraints or variables stored in slots which
	 * are not extracted are also removed from their pools if
	 * \a keepInPool has not been set to \a true at insertion time.
	 *
	 * \param max      The maximal number of extracted items.
	 * \param newSlots The extracted items are inserted into this buffer.
	 */
	void extract(int max, ArrayBuffer<PoolSlot<BaseType, CoType>*> &newSlots);

	//! Sorts the items according to their ranks.
	/**
	 * \param threshold Only if more than \a threshold items are buffered,
	 *                  the sorting is performed.
	 */
	void sort(int threshold);


	Master *master_;	//!< A pointer to the corresponding master of the optimization.
	int         n_;			//!< The number of buffered items.

	//! References to the pool slots of the buffered constraints/variables.
	Array<PoolSlotRef<BaseType, CoType>*> psRef_;

	/**
	 * If \a keepInPool_[i] is \a true for a buffered constraint/variables,
	 * then it is not removed from its pool although it might be
	 * discarded in \a extract().
	 */
	Array<bool>   keepInPool_;

	//! This array stores optionally the rank of the buffered items.
	Array<double> rank_;

	//! This flag is \a true if a rank for each buffered item is available.
	/**
	 * As soon as an item without rank is inserted it becomes \a false.
	 */
	bool ranking_;

	CutBuffer(const CutBuffer<BaseType, CoType> &rhs);

	const CutBuffer<BaseType, CoType>
		&operator=(const CutBuffer<BaseType, CoType> &rhs);
};

}

#include <ogdf/lib/abacus/cutbuffer.inc>
