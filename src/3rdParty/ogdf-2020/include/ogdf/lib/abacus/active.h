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

#include <ogdf/lib/abacus/abacusroot.h>


namespace abacus {

class Master;
class SparVec;

template<class BaseType, class CoType> class PoolSlot;
template<class BaseType, class CoType> class PoolSlotRef;

template<class BaseType,class CoType>
class Active;

template<class BaseType,class CoType>
std::ostream&operator<< (std::ostream &out, const Active<BaseType, CoType> &rhs);


//! Implements the sets of active constraints and variables which are associated with each subproblem.
/**
 * This parameterized class implements the sets of active constraints and variables
 * which are associated with each subproblem. Note, also an inactive
 * subproblem can have an active set of constraints and variables, e.g.,
 * the sets with which its unprocessed sons in the enumeration tree
 * are initialized.
 *
 * If an active set of constraints is instantiated then the \a BaseType
 * should be Constraint and the \a CoType should be Variable,
 * for an active set of variables this is vice versa.
 */
template <class BaseType, class CoType>
class  Active : public AbacusRoot  {
public:

	//! Creates an empty set of active items.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 * \param max    The maximal number of active constraints/variables.
	 */
	Active(Master *master, int max)
		: master_(master), n_(0), active_(max), redundantAge_(0,max-1, 0)
	{ }

	//! Creates a set of active items, initialized to at most \a max items from \a a.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 * \param a      At most \a max active constraints/variables are taken from this set.
	 * \param max    The maximal number of active constraints/variables.
	 */
	Active(Master *master, Active *a, int max);

	//! Copy constructor.
	/**
	 * \param rhs The active set that is copied.
	 */
	Active(const Active<BaseType, CoType> &rhs);

	~Active();

	//! Output operator for active sets.
	/**
	 * The output operator writes all active constraints and variables
	 * to an output stream.
	 *
	 * If an associated pool slot is void, or the item
	 * is newer than the one we refer to, then <tt>"void"</tt> is written.
	 *
	 * \param out The output stream.
	 * \param rhs The active set being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<< <> (std::ostream &out, const Active<BaseType, CoType> &rhs);

	//! Returns the current number of active items.
	int number() const { return n_; }


	//! Returns the maximum number of storable active items (without reallocation).
	int max() const { return active_.size(); }


	//! Access to the <i>i</i>-th active item.
	/**
	 * \param i The number of the active item.
	 *
	 * \return A pointer to the \a i-th active item, or 0 if this item has been removed in the meantime.
	 */
	BaseType* operator[](int i) {
#ifdef OGDF_DEBUG
		if (i > n_) {
			Logger::ifout() << "Active::operator[] : no active item in slot " << i << ".\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Active);
		}
#endif
		return (active_[i]) ? active_[i]->conVar() : nullptr;
	}

	//! Access to the <i>i</i>-th active item.
	/**
	 * \param i The number of the active item.
	 *
	 * \return A const pointer to the \a i-th active item, or 0 if this item has been removed in the meantime.
	 */
	const BaseType* operator[](int i) const {
#ifdef OGDF_DEBUG
		if (i > n_) {
			Logger::ifout() << "Active::operator[] : no active item in slot " << i << ".\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Active);
		}
#endif
		return (active_[i]) ? active_[i]->conVar() : 0;
	}

	//! Returns the <i>i</i>-th entry in the Array \a active.
	/**
	 * \param i The index of the active item.
	 */
	PoolSlotRef<BaseType, CoType>* poolSlotRef(int i) {
		return active_[i];
	}

	//! Returns the <i>i</i>-th entry in the Array \a active.
	/**
	 * \param i The index of the active item.
	 */
	const PoolSlotRef<BaseType, CoType>* poolSlotRef(int i) const {
		return active_[i];
	}

	//! Adds a constraint/variable to the active items set.
	/**
	 * \param ps The pool slot storing the constraint/variable being added.
	 */
	void insert(PoolSlot<BaseType, CoType> *ps);

	//! Adds constraints/variables to the active items set.
	/**
	 * \param ps The buffer storing the pool slots of all constraints/variables that are added.
	 */
	void insert(ArrayBuffer<PoolSlot<BaseType, CoType> *> &ps);

	//! Removes items from the list of active items.
	/**
	 * \param del The numbers of the items that should be removed. These numbers must be upward sorted.
	 */
	void remove(ArrayBuffer<int> &del);

	//! Changes the maximum number of active items which can be stored.
	/**
	 * \param newSize The new maximal number of active items.
	 */
	void realloc(int newSize);

	//! Returns the number of iterations a constraint/variable is already redundant.
	int redundantAge(int i) const {
		return redundantAge_[i];
	}

	//! Increments the number ofiterations the item \a i is already redundant by 1.
	/**
	 * \param i The index of the constraint/variable.
	 */
	void incrementRedundantAge(int i) {
		redundantAge_[i]++;
	}

	//! Sets the number of iterations item \a i is redundant to 0.
	/**
	 * \param i The index of the constraint/variable.
	 */
	void resetRedundantAge(int i) {
		redundantAge_[i] = 0;
	}

private:
	Master *master_;  //!< A pointer to corresponding master of the optimization.

	int n_;  //!< The number of active items.
	Array<PoolSlotRef<BaseType, CoType> *>  active_;  //!< The array storing references to the pool slots of the active items.
	Array<int> redundantAge_;  //!< The number of iterations a constraint is already redundant.

	const Active<BaseType, CoType>
		&operator=(const Active<BaseType, CoType> & rhs);

	OGDF_NEW_DELETE
};

}

#include <ogdf/lib/abacus/active.inc>
