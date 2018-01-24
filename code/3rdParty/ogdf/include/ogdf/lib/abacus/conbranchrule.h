/*!\file
 * \author Matthias Elf
 *
 * \brief branching rule for constraints.
 *
 * This class implements the branching by adding a constraint to the
 * set of active constraints.
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
#include <ogdf/lib/abacus/poolslotref.h>
#include <ogdf/lib/abacus/constraint.h>
#include <ogdf/lib/abacus/variable.h>

namespace abacus {

//! Implements the branching by adding a constraint to the set of active constraints.
class OGDF_EXPORT ConBranchRule : public BranchRule {
public:

	//! Creates a branching constraint.
	/**
	 * \note The subproblem associated with the branching constraint will be
	 * modified in the constructor of the subproblem generated with this
	 * branching rule such that later the check for local validity of the
	 * branching constraint is performed correcly.
	 *
	 * \param master   A pointer to the corresponding master of the optimization.
	 * \param poolSlot A pointer to the pool slot of the branching constraint.
	 */
	ConBranchRule(Master *master, PoolSlot<Constraint, Variable> *poolSlot)
		: BranchRule(master), poolSlotRef_(poolSlot) { }


	virtual ~ConBranchRule() { }

	//! Output operator for branching constraints.
	/**
	 * \param out The output stream.
	 * \param rhs The branch rule being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend OGDF_EXPORT std::ostream &operator<<(std::ostream &out, const ConBranchRule &rhs);


	//! Adds the branching constraint to the subproblem.
	/**
	 * Instead of adding it directly to the set of active constraints it is
	 * added to the cut buffer.
	 *
	 * \param sub The subproblem being modified.
	 *
	 * \return Always 0, since there cannot be a contractiction.
	 */
	virtual int extract(Sub *sub) override;


	//! Overloaded to modify directly the linear programming relaxation.
	/**
	 * This required to evaluate the quality of a branching rule.
	 */
	virtual void extract(LpSub *lp) override;

	virtual void unExtract(LpSub *lp) override;


	//! Initializes the subproblem associated with the branching constraint.
	 /**
	 * \param sub A pointer to the subproblem that is associated with the branching constraint.
	 */
	virtual void initialize(Sub *sub) override;


	//! Returns a pointer to the branching constraint, or a 0-pointer if this constraint is not available.
	Constraint *constraint() {
		return poolSlotRef_.conVar();
	}

	//! Returns a const pointer to the branching constraint, or a 0-pointer if this constraint is not available.
	const Constraint *constraint() const {
		return poolSlotRef_.conVar();
	}

private:
	PoolSlotRef<Constraint, Variable>
		poolSlotRef_;  //!< A reference to the pool slot of the branching constraints.

	const ConBranchRule &operator=(const ConBranchRule &rhs);
};

}
