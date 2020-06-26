/*!\file
 * \author Matthias Elf
 *
 * \brief constraints and variables.
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
class Sub;
class Variable;
class Constraint;

template<class BaseType, class CoType> class PoolSlot;
template<class BaseType, class CoType> class PoolSlotRef;
template<class BaseType, class CoType> class StandardPool;
template<class BaseType, class CoType> class CutBuffer;


//! Common base class for constraints (Constraint) and variables (Variable).
/**
 * ConVar is the common base class for constraints and variables, which
 * are implemented in the derived classes Constraint and Variable,
 * respectively.
 * It might seem a bit strange to implement a common base class for
 * these two objects. Besides several technical reasons, there is
 * linear programming duality which motivates this point of view.
 * E.g., the separation problem for the primal problem is equivalent
 * to the pricing problem for the dual problem.
 *
 * ConVar is <b>not</b> the base class for constraints and variables
 * as they are used in the interface to the linear programming solver.
 * There are the classes Row and Column for this purpose. ConVar is
 * the father of a class hierarchy for abstract constraints and
 * variables which are used in the branch-and-bound algorithm.
 */
class OGDF_EXPORT ConVar : public AbacusRoot {

	friend class PoolSlot<Constraint, Variable>;
	friend class PoolSlot<Variable, Constraint>;
	friend class PoolSlotRef<Constraint, Variable>;
	friend class PoolSlotRef<Variable, Constraint>;
	friend class StandardPool<Constraint, Variable>;
	friend class StandardPool<Variable, Constraint>;
	friend class CutBuffer<Constraint, Variable>;
	friend class CutBuffer<Variable, Constraint>;
	friend class Sub;

public:

	//! Creates an instance of type ConVar.
	/**
	 * \param master  A pointer to the corresponding master of the optimization.
	 * \param sub     A pointer the subproblem the constraint/variable is associated with.
	 *                If the item is not associated with any subproblem, then this
	 *                can also be the 0-pointer.
	 * \param dynamic If this paramument is \a true, then the constraint/variable can also be
	 *                removed again from the set of active constraints/variables after it is
	 *                added once.
	 * \param local   If \a local is \a true, then the constraint/variable is only locally
	 *                valid.
	 */
	ConVar (Master *master, const Sub *sub, bool dynamic, bool local) :
		master_(master),
		sub_(sub),
		expanded_(false),
		nReferences_(0),
		dynamic_(dynamic),
		nActive_(0),
		nLocks_(0),
		local_(local)
	{ }

	virtual ~ConVar();


	//! Checks if the constraint/variable is active in at least one active subproblem.
	/**
	 * \return true if the constraint/variable is active, false otherwise.
	 */
	bool active() const { return (nActive_ != 0); }


	//! Returns true if the constraint/variable is only locally valid, false otherwise.
	bool local() const { return local_; }

	//! Returns true if the constraint/variable is globally valid, false otherwise.
	bool global() const { return !local_; }


	//! Return true if the constraint/variable is dynamic.
	/**
	 * \return true if the constraint/variable can be also removed from the set of active
	 * constraints/variables after it has been activated, false otherwise.
	 */
	virtual bool dynamic() const { return dynamic_; }


	/**
	 * @name Expand and Compress
	 *
	 * Constraints/Variables often have to be stored in a format different
	 * from the format used in the linear program. One reason is to save
	 * memory and the other reason is that if constraints and/or variable
	 * sets are dynamic, then we require a format to compute the coefficients
	 * of later activated variables/constraints.
	 *
	 * The disadvantage of such a constraint format is that the computation
	 * of a single constraint coefficient can be very time consuming. Often
	 * it cannot be done in constant time. Hence we provide a mechanism
	 * which converts a constraint/variable to a format enabling efficient
	 * computation of coefficients. The following functions provide this
	 * feature.
	 */
	//@{

	//! Returns true if the expanded format of a constraint/variable is available, false otherwise.
	bool expanded() const { return expanded_; }


	//! Expands a constraint/variable.
	/**
	 * The default implementation does nothing. It should be redefined in derived classes.
	 * Attention: Data that is compacted/compressed needs to be marked as mutable, as
	 * this function is supposed to be applicable for const objects! Compressing/expanding
	 * is NOT expected to change the "outer" status of this class, and compression/expansion
	 * is done automatically on the fly on an as-needed basis.
	 */
	virtual void expand() const { }

	//! Compresses a constraint/variable.
	/**
	 * The default implementation does nothing. It should be redefined in derived classes.
	 * Attention: Data that is compacted/compressed needs to be marked as mutable, as
	 * this function is supposed to be applicable for const objects! Compressing/expanding
	 * is NOT expected to change the "outer" status of this class, and compression/expansion
	 * is done automatically on the fly on an as-needed basis.
	 */
	virtual void compress() const { }

	//! Returns true if the constraint/variable can be destructed.
	/**
	 * This is per default only possible if the reference counter is 0 and no lock is set.
	 * The function is declared virtual such that problem specific implementations are possible.
	 */
	virtual bool deletable() const {
		return !(nReferences_ || nLocks_);
	}

	//@}

	//! Writes the constraint/variable to the output stream \a out.
	/**
	 * This function is used since the output operator cannot be declared
	 * virtual. The default implementation only writes
	 * <tt>"ConVar::print() is only a dummy."</tt>.
	 * We do not declare this function pure virtual since it is not really
	 * required, mainly only for debugging. In this case a constraint/variable
	 * specific redefinition is strongly recommended.
	 *
	 * Normally, the implementation <tt>out << *this</tt> should be sufficient.
	 *
	 * \param out The output stream.
	 */
	virtual void print(std::ostream &out) const;

	//! Returns a const pointer to the subproblem associated with the constraint/variable.
	/**
	 * Note, this can also be the 0-pointer.
	 */
	const Sub *sub() const { return sub_; }


	//! Associates a new subproblem with the constraint/variable.
	/**
	 * \param sub The new subproblem associated with the constraint/variable.
	 */
	void sub(Sub *sub) { sub_ = sub; }


	//! Should provide a key for the constraint/variable that can be used to insert it into a hash table.
	/**
	 * As usual for hashing, it is not required that any two items have
	 * different keys.
	 *
	 * This function is required if the constraint/variable is stored in a
	 * pool of the class NonDuplPool.
	 *
	 * The default implementation just throws an exception.
	 * This function is not a pure virtual function because in the default
	 * version of ABACUS it is not required.
	 *
	 * We do not use \a double as result type because typical problems in
	 * floating point arithmetic might give slightly different hash keys
	 * for two constraints that are equal from a mathematical point of
	 * view.
	 *
	 * \return An integer providing a hash key for the constraint/variable.
	 */
	virtual unsigned hashKey() const;

	//! Should return the name of the constraint/variable.
	/**
	 * This function is required to emulate a simple
	 * run time type information (RTTI) that is still missing in g++.
	 * This function will be removed as soon as RTTI is supported sufficiently.
	 *
	 * A user must take care that for each redefined version of this
	 * function in a derived class a unique name is returned. Otherwise
	 * fatal run time errors can occur. Therefore, we recommend to return
	 * always the name of the class.
	 *
	 * This function is required if the constraint/variable is stored in a
	 * pool of the class NonDuplPool.
	 *
	 * The default implementation shows a warning and throws an exception.
	 * This function is not a pure virtual function because in the default
	 * version of ABACUS it is not required.
	 *
	 * \return The name of the constraint/variable.
	 */
	virtual const char *name() const;

	//! Should compare if the constraint/variable is identical (in a mathematical sense) with the constraint/variable \a cv.
	/**
	 * Using RTTI or its emulation provided by the function name()
	 * it is sufficient to implement this functions for
	 * constraints/variables of the same type.
	 *
	 * This function is required if the constraint/variable is stored in a
	 * pool of the class NonDuplPool.
	 *
	 * The default implementation shows a warning and throws an exception.
	 * This function is not a pure virtual function because in the default
	 * version of ABACUS it is not required.
	 *
	 * \param cv The constraint/variable that should be compared with this object.
	 *
	 * \return true If the constraint/variable represented by this object
	 *              represents the same item as the constraint/variable \a cv,
	 *              false otherwise.
	 */
	virtual bool equal(const ConVar *cv) const;

	//! The function should return a rank associated with the constraint/variable.
	/**
	 * The default implementation returns 0.
	 *
	 * \return The rank of the constraint/variable.
	 */
	virtual double rank() const { return 0; }

protected:

	Master *master_; //!< A pointer to the corresponding master of the optimization.

	//! A pointer to the subproblem associated with the constraint/variable.
	/**
	 * This may be also the 0-pointer.
	 */
	const Sub *sub_;

	mutable bool expanded_; //!< true, if expanded version of constraint/variables available. [mutable in const-objects to allow compress/expand on const]

	int nReferences_; //!< The number of references to the pool slot the constraint is stored in.

	/**
	 * If this member is \a true then the constraint/variable can be also removed from the active formulation
	 * after it is added the first time. For constraints/variables which should be
	 * never removed from the active formulation this member should be set to \a false.
	 */
	bool dynamic_;

	//! The number of active subproblems of which the constraint/variable belongs to the set of active constraints/variables.
	/**
	 * This value is always 0 after construction and has to be set and reset during the subproblem
	 * optimization. This member is mainly used to accelerate pool separation and to control that the same variable
	 * is not multiply included into a set of active variables.
	 */
	int nActive_;

	int nLocks_; //!< The number of locks which have been set on the constraint/variable.

	bool local_; //!< true if the constraint/variable is only locally valid

private:

	//! Tries to generate the expanded format of the constraint/variable.
	/**
	 * This will be only possible if the virtual
	 * function expand() is redefined for the specific constraint/variable.
	 */
	void _expand() const;


	//! Removes the expanded format of the constraint/variable.
	/**
	 * This will be only possible if the virtual
	 * function \a compress() is redefined for the specific constraint/variable.
	 */
	void _compress() const;

	//! Must be called if the constraint/variable is added to the active formulation of an active subproblem.
	/**
	 * This function is only called within member functions of the class Sub.
	 */
	void activate() { ++nActive_; }


	//! Counterpart of activate().
	/**
	 * Is also called within members of the class Sub to indicate
	 * that the constraint/variable does not belong any more to the active
	 * formulation of an active subproblem.
	 */
	void deactivate();

	//! Returns the number of references to the pool slot PoolSlotRef storing this constraint/variable.
	/**
	 * We require the bookkeeping of the references in order to determine
	 * if a constraint/variable can be deleted without causing any harm.
	 */
	int nReferences() const { return nReferences_; }


	//! Indicates that there is a new reference to the pool slot storing this constraint/variable.
	/**
	 * The function is only called from members of the class PoolSlotRef.
	 */
	void addReference() { ++nReferences_; }


	//! Is the counterpart of the function addReference() and indicates the removal of a reference to this constraint.
	/**
	 * It is only called from members of the class PoolSlotRef.
	 */
	void removeReference();

	/** @name Locking
	 *
	 * If a constraint/variable has just been separated and added to
	 * the buffer of currently separated constraints/variables, then
	 * this item should not be removed before the buffer is emptied
	 * at the beginning of the next iteration. Hence, we provide a
	 * locking mechanism for constraints/variables by the following
	 * three functions.
	 */
	//@{

	//! Returns true if at least one lock is set on the constraint/variable, false otherwise.
	bool locked() const { return (nLocks_ != 0); }


	//! Adds an additional lock to the constraint/variable.
	void lock() { ++nLocks_; }


	//! Removes one lock from the constraint/variable.
	void unlock();

	//@}
};


inline ConVar::~ConVar()
{
#ifdef OGDF_DEBUG
	if (nActive_) {
		Logger::ifout() << "ConVar::~ConVar(): constraint/variable still active: \ncounter = " << nActive_ << "\n";
	}

	if (nLocks_) {
		Logger::ifout() << "ConVar::~ConVar(): constraint/variable has still " << nLocks_ << " locks\n";
	}

#ifndef OGDF_USE_ASSERT_EXCEPTIONS // do not throw exceptions in destructor
	OGDF_ASSERT(nActive_ == 0);
	OGDF_ASSERT(nLocks_ == 0);
#endif
#endif
}


inline void ConVar::deactivate()
{
	OGDF_ASSERT(nActive_ != 0);
	--nActive_;
}


inline void ConVar::removeReference()
{
	if(--nReferences_ < 0) {
		Logger::ifout() << "ConVar::removeReference : reference counter negative\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Convar);
	}
}


inline void ConVar::unlock()
{
	OGDF_ASSERT(nLocks_ != 0);
	--nLocks_;
}


}
