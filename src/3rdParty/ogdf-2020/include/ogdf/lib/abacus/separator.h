/*!\file
 * \author Matthias Elf
 * \brief separator.
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

#include <ogdf/lib/abacus/lpsolution.h>
#include <ogdf/lib/abacus/nonduplpool.h>

namespace abacus {


enum Separator_CUTFOUND {Added, Duplication, Full};


//! Separators.
/**
 * This abstract template class can be used to implement a separation
 * routine. Using this class
 * is not mandatory, because separation can be implemented directly
 * in Sub::pricing() and Sub::separate(). However,
 * this class facilitates encapsulation of the code and provides
 * functions for checking for duplication of generated constraints/variables.
 *
 * If constraints are generated in the separation, then the \a BaseType
 * must be Constraint and the \a CoType must be Variable,
 * if variables are generated in separation this is vice versa.
 *
 * The user has to derive its specific separtor class in which the
 * separation algorithm should be implemented in function \a separate().
 * If a cutting plane is found,
 * the function \a cutFound should be called.
 *
 * The generated constraints/variables can be obtained by the member
 * function \a cutBuffer(). The return value of that function then can
 * serve as parameter to the functions Sub::addCons() and Sub::addVars().
 */
template <class BaseType, class CoType>
class  Separator :  public AbacusRoot  {
public:

	//! Creates a separator.
	/**
	 * \param lpSolution The LP solution to be separated.
	 * \param maxGen     The maximal number of cutting planes which are stored.
	 * \param nonDuplications If this flag is set, then the same
	 *                   constraint/variable is stored at most  once in the buffer. In this case
	 *                   for constraints/variables the virtual member functions
	 *                   \a name(), \a hashKey(), and \a equal() of the base class ConVar have to be
	 *                   defined. Using these three functions, we check in the function \a cutFound
	 *                   if a constraint or variable is already stored in the buffer.
	 */
	Separator(
		LpSolution<CoType,BaseType> *lpSolution,
		bool nonDuplications,
		int maxGen=300)
		:
		master_(lpSolution->master_),
		lpSol_(lpSolution),
		minAbsViolation_(master_->eps()),
		newCons_(master_,maxGen),
		hash_(0),
		nDuplications_(0),
		pool_(0)
	{
		if(nonDuplications)
			hash_=new AbaHash<unsigned, BaseType *>((AbacusGlobal*)master_, 3*maxGen);
	}


	//! \brief The destructor.
	virtual ~Separator() {
		delete hash_;
	}


	//! This function has to be redefined and should implement the separation routine.
	virtual void separate() = 0;

	//! Passes a cut (constraint or variable) to the buffer.
	/**
	 * If the buffer is full or the cut already exists, the cut is deleted.
	 *
	 * \param cv A pointer to a new constraint/variable found by the separation algorithm.
	 *
	 * \return ABA\_SEPARATOR\_CUTFOUND::Added       if the cut is added to the buffer;
	 * \return ABA\_SEPARATOR\_CUTFOUND::Duplication if the cut is already in the buffer;
	 * \return ABA\_SEPARATOR\_CUTFOUND::Full        if the buffer is full.
	 */
	Separator_CUTFOUND cutFound(BaseType *cv);

	//! Returns true if the separation should be terminated.
	/**
	 * In the default implementation, this is the case if \a maxGen constraints/variables
	 * are in the cutBuffer.
	 */
	virtual bool terminateSeparation() {
		return ( nGen() >= maxGen() );
	}

	//! Returns the buffer with the generated constraints/variable.
	ArrayBuffer<BaseType *> &cutBuffer() { return newCons_; }


	//! Returns the number of generated cutting planes.
	int nGen() const { return newCons_.number(); }


	//! Returns the number of duplicated constraints/variables which are discarded.
	int nDuplications() const { return nDuplications_; }


	//! Returns the number of collisions in the hash table.
	int nCollisions() const;

	//! Returns the maximal number of generated cutting planes.
	int maxGen() const { return newCons_.size(); }


	//! Returns the absolute value for considering a constraint/variable as violated.
	double minAbsViolation() const { return minAbsViolation_; }


	//! Sets a new value for \a minAbsViolation.
	void minAbsViolation(double minAbsVio) {
		minAbsViolation_=minAbsVio;
	}

	//! The lpSolution to be separated.
	LpSolution<CoType, BaseType> *lpSolution() {
		return lpSol_;
	}

	/**
	 * If the separator checks for duplication of cuts, the test is also done for
	 * constraints/variables that are in the pool passed as argument.
	 *
	 * This can be useful if already cuts are generated by performing constraint pool separation
	 * of this pool.
	 */
	void watchNonDuplPool(NonDuplPool<BaseType, CoType> *pool) {
		pool_ = pool;
	}

protected:

	/**
	 * \param cv A pointer to a constraint/variable for which it should
	 *           be checked if an equivalent item is already contained in the buffer.
	 *
	 * \return The function checks if a constraint/variable that is equivalent
	 * to \a cv according to the function ConVar::equal() is already stored in
	 * the buffer by using the hashtable.
	 */
	bool find(BaseType *cv);

	Master *master_; //!< A pointer to the corresponding master of the optimization.
	LpSolution<CoType, BaseType> *lpSol_; //!< The LP solution to be separated.

private:
	double minAbsViolation_;
	ArrayBuffer<BaseType*> newCons_;
	AbaHash<unsigned, BaseType*> *hash_;
	int nDuplications_;
	bool sendConstraints_;
	NonDuplPool<BaseType, CoType> *pool_;

	Separator(const Separator<BaseType, CoType> &rhs);
	const Separator<BaseType, CoType>
		&operator=(const Separator<BaseType, CoType> & rhs);
};

}

#include <ogdf/lib/abacus/separator.inc>
