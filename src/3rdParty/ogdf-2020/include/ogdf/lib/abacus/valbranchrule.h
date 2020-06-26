/*!\file
 * \author Matthias Elf
 * \brief branching rule for values.
 *
 * This class implements a branching rule for setting a variable to a certain
 * value.
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

namespace abacus {


//! Implements a branching rule for setting a variable to a certain value.
class  ValBranchRule :  public BranchRule  {
public:

	//! Creates a branching rule for setting \a variable to \a value.
	/**
	 * \param master   The corresponding master of the optimization.
	 * \param variable The branching variable.
	 * \param value    The value the branching variable is set to.
	 */
	ValBranchRule(Master *master, int variable, double value)
		: BranchRule(master), variable_(variable), value_(value) { }


	virtual ~ValBranchRule() { }


	//! Output operator for val branching rules.
	/**
	 * Writes the branching variable together with its value to output stream \a out.
	 *
	 * \param out The output stream.
	 * \param rhs The branching rule being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<<(std::ostream &out, const ValBranchRule &rhs);


	//! Modifies a subproblem by setting the branching variable.
	/**
	 * \param sub The subproblem being modified.
	 *
	 * \return 0 If the subproblem can be modified according to the branching rule.
	 * \return 1 If a contradiction occurs.
	 */
	virtual int extract(Sub *sub) override;


	//! Overloaded to modify directly the linear programming relaxation.
	/**
	 * This required to evaluate the quality of a branching rule.
	 */
	virtual void extract(LpSub *lp) override;


	virtual void unExtract(LpSub *lp) override;


	//! Returns the number of the branching variable.
	int variable() const {
		return variable_;
	}


	//! Returns the value of the branching variable.
	double value() const {
		return value_;
	}


private:
	int    variable_;		//!< The branching variable.
	double value_;			//!< The value the branching variable is set to.
	double oldLpLBound_;
	double oldLpUBound_;
};

}
