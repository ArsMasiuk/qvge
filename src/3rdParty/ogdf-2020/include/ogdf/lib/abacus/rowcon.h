/*!\file
 * \author Matthias Elf
 * \brief constraint using row.
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

#include <ogdf/lib/abacus/row.h>
#include <ogdf/lib/abacus/constraint.h>
#include <ogdf/lib/abacus/numvar.h>

namespace abacus {


//! Implements constraints stored in the class Row.
/**
 * Earlier we explained that we distinguish between the constraint
 * and the row format. We have seen already that a constraint is
 * transformed to the row format when it is added to the linear program.
 * However, for some constraints of certain optimization problems
 * the row format itself is the most suitable representation.
 * Therefore the class RowCon implements constraints stored in the
 * class Row.
 */
class  RowCon :  public Constraint  {
public:

	//! Creates a row constraint.
	/**
	 * \param master   A pointer to the corresponding master of the optimization.
	 * \param sub      A pointer to the subproblem associated with the
	 *                 constraint. This can also be the 0-pointer.
	 * \param sense    The sense of the constraint.
	 * \param nnz      The number of nonzero elements of the constraint.
	 * \param support  The array storing the variables with nonzero coefficients.
	 * \param coeff    The nonzero coefficients of the variables stored in \a support.
	 * \param rhs      The right hand side of the constraint.
	 * \param dynamic  If this argument is \a true, then the constraint can be removed
	 *                 from the active constraint set during the cutting plane phase
	 *                 of the subproblem optimization.
	 * \param local    If this argument is \a true, then the constraint is considered
	 *                 to be only locally valid. As a locally valid constraint is associated
	 *                 with a subproblem, \a sub must not be 0 if \a local is \a true.
	 * \param liftable If this argument is \a true, then a lifting procedure must be
	 *                 available, i.e., that the coefficients of variables which have not been
	 *                 active at generation time of the constraint can be computed.
	 */
	RowCon(Master *master,
		const Sub *sub,
		CSense::SENSE sense,
		int nnz,
		const Array<int> &support,
		const Array<double> &coeff,
		double rhs,
		bool dynamic,
		bool local,
		bool liftable)
		:
		Constraint(master, sub, sense, rhs, dynamic, local, liftable),
		row_(master, nnz, support, coeff, sense, rhs)
	{ }

	//! Creates a row constraint.
	/**
	 * \param master   A pointer to the corresponding master of the optimization.
	 * \param sub      A pointer to the subproblem associated with the
	 *                 constraint. This can also be the 0-pointer.
	 * \param sense    The sense of the constraint.
	 * \param nnz      The number of nonzero elements of the constraint.
	 * \param support  The array storing the variables with nonzero coefficients.
	 * \param coeff    The nonzero coefficients of the variables stored in \a support.
	 * \param rhs      The right hand side of the constraint.
	 * \param dynamic  If this argument is \a true, then the constraint can be removed
	 *                 from the active constraint set during the cutting plane phase
	 *                 of the subproblem optimization.
	 * \param local    If this argument is \a true, then the constraint is considered
	 *                 to be only locally valid. As a locally valid constraint is associated
	 *                 with a subproblem, \a sub must not be 0 if \a local is \a true.
	 * \param liftable If this argument is \a true, then a lifting procedure must be
	 *                 available, i.e., that the coefficients of variables which have not been
	 *                 active at generation time of the constraint can be computed.
	 */
	RowCon(Master *master,
		const Sub *sub,
		CSense::SENSE sense,
		int nnz,
		int *support,
		double *coeff,
		double rhs,
		bool dynamic,
		bool local,
		bool liftable)
		:
		Constraint(master, sub, sense, rhs, dynamic, local, liftable),
		row_(master, nnz, support, coeff, sense, rhs)
	{ }

	//! The destructor.
	virtual ~RowCon() { }

	//! Computes the coefficient of a variable which must be of type NumVar.
	/**
	 * It redefines the virtual function \a coeff() of the base class Constraint.
	 *
	 * \warning The worst case complexity of the call of this function is
	 * the number of nonzero elements of the constraint.
	 *
	 * \param v The variable of which the coefficient is determined.
	 *
	 * \return The coefficient of the variable \a v.
	 */
	virtual double coeff(const Variable *v) const override {
		const NumVar *numVar = static_cast<const NumVar *>(v);
		return row_.origCoeff(numVar->number());
	}

	//! Writes the row format of the constraint on an output stream.
	/**
	 * It redefines the virtual function \a print() of the base class ConVar.
	 *
	 * \param out The output stream.
	 */
	virtual void print(std::ostream &out) const override {
		out << row_;
	}

	//! Returns a pointer to the object of the class Row representing the constraint.
	Row *row() { return &row_; }

	//! Returns a const pointer to the object of the class Row representing the constraint.
	const Row *row() const { return &row_; }

protected:

	Row row_; //!< The representation of the constraint.
};

}
