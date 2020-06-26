/*!\file
 * \author Matthias Elf
 *
 * \brief Base class for branching rules.
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
class LpSub;


//! Abstract base class for all branching rules.
/**
 * Branching should be very flexible within a branch-and-cut framework.
 * Therefore in a branching step each generated subproblem
 * receives an object of the class BranchRule. When the subproblem
 * is activated, it copies the active variables, their bounds and statuses,
 * and the active constraints from the father, and then modifies
 * the subproblem according to the branching rule.
 *
 * This class is an abstract base class for all branching rules within
 * this framework. We provide by non-abstract derived classes standard branching
 * schemes for setting a binary variable to its lower or upper
 * bound (SetBranchRule),
 * for setting an integer variable to a certain value (ValBranchRule),
 * for changing the bounds of an integer variable (ConBranchRule), and
 * for adding a branching constraint (ConBranchRule).
 */
class OGDF_EXPORT BranchRule : public AbacusRoot {
public:

	//! Initializes a branching rule.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 */
	BranchRule(Master *master) : master_(master) { }


	virtual ~BranchRule() { }


	//! Modifies a subproblem by setting the branching variable.
	/**
	 * \param sub The subproblem being modified.
	 *
	 * \return 0 If the subproblem can be modified according to the branching rule.
	 * \return 1 If a contradiction occurs.
	 */
	virtual int extract(Sub *sub) = 0;


	//! Should modify the linear programming relaxation |lp| in order to determine the quality of the branching rule in an LP-based branching rule selection.
	/**
	 * The default implementation does nothing except writing a warning to the error stream.
	 * If a derived concrete branching rule should be used in LP-based branching
	 * rule selection then this function has to be redefined.
	 *
	 * \param lp A pointer to a the linear programming relaxtion of a subproblem.
	 */
	virtual void extract(LpSub *lp);


	//! Should undo the modifictions of the linear programming relaxtion |lp|.
	/**
	 * This function has to be redefined in a derived class if extract(LpSub*) is redefined there.
	 *
	 * \param lp A pointer to a the linear programming relaxtion of a subproblem.
	 */
	virtual void unExtract(LpSub *lp);


	//! Should indicate if the branching is performed by setting a binary variable.
	/**
	 * This is only required as in the current version of the GNU-compiler run time
	 * type information is not satisfactorily implemented.
	 *
	 * This function is currently required to determine global validity
	 * of Gomory cuts for general \a s.
	 *
	 * \return The default implementation returns always false.
	 *         This function must be redefined in SetBranchRule, where it has
	 *         to return \a true.
	 */
	virtual bool branchOnSetVar() {
		return false;
	}


	//! Called from the constructor of a subproblem.
	/**
	 * It can be used to perform initializations of the branching rule that can only be
	 * done after the generation of the subproblem.
	 *
	 * The default implementation does nothing.
	 *
	 * \param sub A pointer to the subproblem that should be used for the initialization.
	 */
	virtual void initialize(Sub* sub) { }

protected:
	Master *master_;  //!< A pointer to the corresponding master of the optimization.

	OGDF_NEW_DELETE
};

}
