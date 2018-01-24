/*!\file
 * \author Matthias Elf
 * \brief candidates for fixing.
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
class FSVarStat;
class Variable;
class Constraint;

template<class BaseType, class CoType> class CutBuffer;
template<class BaseType, class CoType> class PoolSlotRef;


//! Candidates for fixing.
/**
 * Variables can be only fixed according to the reduced costs and
 * statuses of variables of the root of the remaining branch-and-bound tree.
 * However, if we store these values, we can repeat the fixing process
 * also in any other node of the enumeration tree when we find
 * a better global lower bound.
 *
 * Possible candidates for fixing are all variables which have
 * the status \a AtLowerBound or \a AtUpperBound. We store all
 * these candidates together with their values in this class.
 *
 * If we try to fix variables according to reduced cost criteria
 * in nodes which are not the root of the remaining branch-and-cut tree,
 * we always have to take the candidates and values from this class.
 */
class OGDF_EXPORT FixCand : public AbacusRoot {

	friend class Sub;
	friend class Master;

public:

	//! Creates an empty set of candidates for fixing.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 */
	FixCand(Master *master) :
		master_(master),
		candidates_(nullptr),
		fsVarStat_(nullptr),
		lhs_(nullptr)
	{ }

	//! The destructor.
	~FixCand() { deleteAll(); }

private:

	//! Memorizes suitable variables for fixing.
	/**
	 * \param sub A pointer to the root node of the remaining branch-and-cut tree.
	 */
	void saveCandidates(Sub *sub);

	//! Tries to fix as many candidates as possible.
	/**
	 * The new variable status is both stored in the global variable status
	 * of the class Master and in the local variable status of Sub.
	 * Candidates which are fixed are removed from the candidate set.
	 *
	 * We do not used the function Master::primalViolated() for checking of a
	 * variable can be fixed, because here we also have to be careful for
	 * integer objective function.
	 *
	 * \param addVarBuffer Inactive variables which are fixed to a nonzero
	 *                     value are added to \a addVarBuffer to be activated
	 *                     in the next iteration.
	 *
	 * \return 1 If contradictions to the variables statuses of \a sub are detected; 0 otherwise.
	 */
	void fixByRedCost(CutBuffer<Variable, Constraint> *addVarBuffer);

	//! Deletes all allocated memory of members.
	/**
	 * The member pointers are set to 0 that multiple deletion cannot cause any error.
	 */
	void deleteAll();

	//! Allocates memory to store \a nCand candidates for fixing.
	void allocate(int nCand);


	Master *master_; //!< A pointer to the corresponding master of the optimization.

	ArrayBuffer<PoolSlotRef<Variable, Constraint>*> *candidates_; //!< The candidates for fixing.

	ArrayBuffer<FSVarStat*> *fsVarStat_; //!< The fixing status of the candidates.

	ArrayBuffer<double> *lhs_; //!< The left hand side of the expression evaluated for fixing.

	FixCand(const FixCand &rhs);
	const FixCand &operator=(const FixCand &rhs);
};

}
