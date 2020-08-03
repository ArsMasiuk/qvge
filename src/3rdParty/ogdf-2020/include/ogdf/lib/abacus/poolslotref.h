/*!\file
 * \author Matthias Elf
 * \brief poolslotref
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
#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/constraint.h>
#include <ogdf/lib/abacus/variable.h>

namespace abacus {

template<class BaseType, class CoType>
class  PoolSlotRef;

template<class BaseType, class CoType>
std::ostream &operator<< (std::ostream &out, const PoolSlotRef<BaseType, CoType> &slot);


//! Stores a pointer to a pool slot with version number.
/**
 * As already explained in the class PoolSlot we do not refer directly
 * to constraints/variables but store a pointer to a pool slot and
 * memorize the version number of the slot at initialization time of the
 * class PoolSlotRef.
 */
template<class BaseType, class CoType>
class  PoolSlotRef :  public AbacusRoot {
public:

	//! Creates an object referencing no pool slot.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 */
	PoolSlotRef(Master *master) : master_(master), slot_(0), version_(0) { }

	//! Creates an object referencing a pool slot \a slot.
	/**
	 * Also the constraint/variable contained in this slot receives a
	 * message that a new references to it is created.
	 *
	 * \param slot The pool slot that is referenced now.
	 */
	PoolSlotRef(PoolSlot<BaseType, CoType> *slot) : master_(slot->master()), slot_(slot), version_(slot->version())
	{
		ConVar *cv = slot_->conVar();
		if(cv) cv->addReference();
	}

	//! Copy constructor
	/**
	 * May increment the reference counter of the
	 * constraint/variable only if version number of the slot and
	 * version number of the reference are equal, since otherwise
	 * this is not a correct reference to \a slot_->conVar().
	 *
	 * \param rhs The pool slot that is copied in the initialization process.
	 */
	PoolSlotRef(const PoolSlotRef<BaseType, CoType> &rhs) : master_(rhs.master_), slot_(rhs.slot_), version_(rhs.version_)
	{
		ConVar *cv = slot_->conVar();
		if (version_ == slot_->version() && cv)
			cv->addReference();
	}

	//! The destructor
	/**
	 * Sends a message to the constraint that it
	 * will no longer be referred from this place in the program.
	 *
	 * If the version number of the reference and the version number
	 * of the slot do not equal, we must not decrement the
	 * reference counter of \a slot_->conVar() because this is
	 * not a correct reference to this constraint/variable.
	 */
	~PoolSlotRef() {
		if(slot_) {
			ConVar *cv = slot_->conVar();
			if (cv && version_ == slot_->version())
				cv->removeReference();
		}
	}

	//! Output operator for pool slot references.
	/**
	 * The output operator writes the constraint/variable stored in the
	 * referenced slot to an output stream.
	 *
	 * \param out  The output stream.
	 * \param slot The reference to a pool slot being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<< <> (std::ostream &out, const PoolSlotRef<BaseType, CoType> &slot);

	//! Returns a pointer to the constraint/variable stored in the referenced slot.
	/**
	 * If the version number of the slot differs from the version number at construction/initialization time
	 * of this slot 0 is returned.
	 */
	BaseType *conVar() const {
		if(version_ == slot_->version())
			return slot_->conVar();
		printDifferentVersionError();

		return nullptr;
	}

	//! Returns the version number of the constraint/variable stored in the referenced slot at construction time.
	unsigned long version() const { return version_; }

	//! Returns a pointer to the referenced slot.
	PoolSlot<BaseType, CoType> *slot() const { return slot_; }


	//! Initializes the referenced pool slot with \a s.
	/**
	 * The function \a slot() may decrement the reference counter
	 * of \a slot_->conVar() only if the version number of the reference
	 * and the version number of the slot are equal since otherwise this
	 * is not a valid reference.
	 *
	 * \param s The new slot that is referenced. This must not be a 0-pointer.
	 */
	void slot(PoolSlot<BaseType, CoType> *s);

private:

	Master *master_; //!< A pointer to the corresponding master of the optimization.

	PoolSlot<BaseType, CoType> *slot_; //!< A pointer to the referenced pool slot.

	//! The version number of the slot at construction/initialization time of this reference.
	unsigned long version_;

	void printDifferentVersionError() const;


	const PoolSlotRef<BaseType, CoType>
		&operator=(const PoolSlotRef<BaseType, CoType> &rhs);

	OGDF_NEW_DELETE
};

}

#include <ogdf/lib/abacus/poolslotref.inc>
