/*!\file
 * \author Matthias Elf
 * \brief constraint defined by a number.
 *
 * Like the class NumVar for variables we provide the class NumCon
 * for constraints which are uniquely defined by an integer number.
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

#include <ogdf/lib/abacus/constraint.h>


namespace abacus {


//! Constraints defined by a number.
/**
 * Like the class NumVar for variables we provide the class NumCon
 * for constraints which are uniquely defined by an integer number.
 */
class OGDF_EXPORT NumCon : public Constraint {
public:

	//! Creates a constraint defined by \a number.
	/**
	 * \param master   A pointer to the corresponding master of the optimization.
	 * \param sub      A pointer to the subproblem associated with the constraint.
	 *                 This can be also the 0-pointer.
	 * \param sense    The sense of the constraint.
	 * \param dynamic  If this argument is \a true, then the constraint can be removed
	 *                 from the active constraint set during the cutting plane phase
	 *                 of the subproblem optimization.
	 * \param local    If this argument is \a true, then the constraint is considered
	 *                 to be only locally valid. As a local constraint is associated
	 *                 with a subproblem, \a sub must not be 0 if \a local is \a true.
	 * \param liftable If this argument is \a true, then a lifting procedure must be
	 *                 available, i.e., that the coefficients of variables which have
	 *                 not been active at generation time of the constraint can be computed.
	 * \param number   The identification number of the constraint.
	 * \param rhs      The right hand side of the constraint.
	 */
	NumCon(Master *master,
		const Sub *sub,
		CSense::SENSE sense,
		bool dynamic,
		bool local,
		bool liftable,
		int number,
		double rhs)
		: Constraint(master, sub,  sense, rhs, dynamic, local, liftable), number_(number)
	{ }

	//! The destructor.
	virtual ~NumCon() { }

	//! The output operator writes the identification number and the right hand side to an output stream.
	/**
	 * \param out The output stream.
	 * \param rhs The variable being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<<(std::ostream &out, const NumCon &rhs) {
		return out << "number = " << rhs.number_ << "  rhs = " << rhs.rhs_ << std::endl;
	}

	//! Returns the coefficient of the variable \a v.
	/**
	 * \param v The variable of which the coefficient is determined.
	 *          It must point to an object of the class ColVar.
	 */
	virtual double coeff(const Variable *v) const;

	//! Writes the row format of the constraint on  an output stream.
	/**
	 * It redefines the virtual function \a print() of the base class ConVar.
	 *
	 * \param out The output stream.
	 */
	virtual void print(std::ostream &out) const {
		out << *this;
	}

	//! Returns the identification number of the constraint.
	int number() const { return number_; }


private:
	int number_; //!< The identification number of the constraint.
};

}

#include <ogdf/lib/abacus/colvar.h>

namespace abacus {

inline double NumCon::coeff(const Variable *v) const
{
	const ColVar *colvar = (const ColVar *) v;
	return colvar->coeff(number_);
}

}
