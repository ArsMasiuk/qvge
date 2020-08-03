/*!\file
 * \author Matthias Elf
 * \brief branching rule for setting.
 *
 * This class implements a branching rule for setting a binary variable
 * to its lower or upper bound.
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

#include <ogdf/lib/abacus/branchrule.h>
#include <ogdf/lib/abacus/fsvarstat.h>

namespace abacus {


//! Implements a branching rule for setting a binary variable to its lower or upper bound.
class  SetBranchRule :  public BranchRule  {
public:

	//! Creates a branching rule for setting binary \a variable according to \a status.
	/**
	 * \param master   A pointer to the corresponding master of the optimization.
	 * \param variable The branching variable.
	 * \param status   The status the variable is set to (SetToLowerBound or SetToUpperBound).
	 */
	SetBranchRule(Master *master, int variable, FSVarStat::STATUS status)
		: BranchRule(master), variable_(variable), status_(status) { }


	virtual ~SetBranchRule() { }


	//! Output operator for set branching rules.
	/**
	 * Writes the number of the branching variable and its status to the output stream \a out.
	 *
	 * \param out The output stream.
	 * \param rhs The branching rule being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<<(std::ostream &out, const SetBranchRule &rhs);


	//! Modifies a subproblem by setting the branching variable.
	/*!*
	 * \return 0 If the subproblem can be modified according to the branching rule.
	 * \return 1 If a contradiction occurs.
	 *
	 * \param sub The subproblem being modified.
	 */
	virtual int extract(Sub *sub) override;


	//! Overloaded to modify directly the linear programming relaxation.
	/**
	 * This required to evaluate the quality of a branching rule with linear
	 * programming methods. The changes have to be undone with the function
	 * unextract() before the next linear program is solved.
	 *
	 * \param lp A pointer to the linear programming relaxation of a subproblem.
	 */
	virtual void extract(LpSub *lp) override;


	virtual void unExtract(LpSub *lp) override;


	//! Redefined for returning true, as this branching rule is setting a binary variable.
	/**
	 * \return Always true.
	 */
	virtual bool branchOnSetVar() override {
		return true;
	}


	//! Returns true if the branching variable is set to the upper bound, false otherwise.
	bool setToUpperBound() const {
		return (status_ == FSVarStat::SetToUpperBound);
	}


	//! Returns the number of the branching variable.
	int variable() const {
		return variable_;
	}


private:
	int                   variable_;	//!< The branching variable.
	FSVarStat::STATUS status_;		//!< The status of the branching variable.

	/**
	 * The bound of the branching variable in the LP before it is temporarily
	 * modified for testing the quality of this branching rule.
	 */
	double oldLpBound_;  //!< The previous LP bound.
};

}
