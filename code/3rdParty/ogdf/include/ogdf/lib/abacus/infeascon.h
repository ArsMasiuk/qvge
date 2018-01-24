/*!\file
 * \author Matthias Elf
 * \brief infeasible constraints.
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

class Constraint;
class Variable;
class Master;


//! Infeasible constraints.
/**
 * If a constraint is transformed from its pool to the row format
 * it may turn out that the constraint is infeasible since variables
 * are fixed or set such that all nonzero coefficients of the left
 * hand side are eliminated and the right hand side has to be updated.
 * The enumeration \a INFEAS indicates if the
 * constraint's left hand side, which is implicitly zero,
 * is either \a TooLarge, \a Feasible, or \a TooSmall.
 */
class OGDF_EXPORT InfeasCon : public AbacusRoot {
public:

	//! The different ways of infeasibility of a constraint.
	enum INFEAS {
		TooSmall = -1,	//!< The left hand side is too small for the right hand side.
		Feasible,		//!< The constraint is not infeasible.
		TooLarge		//!< The left hand side is too large for the right hand side.
	};

	//! The constructor.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 * \param con    The infeasible constraint.
	 * \param inf    The way of infeasibility.
	 */
	InfeasCon(Master *master, Constraint *con, INFEAS inf) :
		master_(master),
		constraint_(con),
		infeas_(inf)
	{ }

	//! Returns a pointer to the infeasible constraint.
	Constraint *constraint() const { return constraint_; }


	//! Returns the way of infeasibility of the constraint.
	INFEAS infeas() const { return infeas_; }

	//! Returns true if the variable \a v might reduce the infeasibility, false otherwise.
	/**
	 * \param v A variable for which we test if its addition might reduce infeasibility.
	 */
	bool goodVar(const Variable *v) const;

private:

	//! A pointer to the corresponding master of the optimization.
	Master *master_;

	//! A pointer to the infeasible constraint.
	Constraint *constraint_;

	//! The way of infeasibility.
	INFEAS infeas_;
};

}
