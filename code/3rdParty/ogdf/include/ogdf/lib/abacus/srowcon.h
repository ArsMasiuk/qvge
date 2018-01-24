/*!\file
 * \author Matthias Elf
 * \brief constraint using row with static variable set.
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

#include <ogdf/lib/abacus/rowcon.h>

namespace abacus {

//! Constraints using row with static variable set.
/**
 * The member functions \a genRow() and \a slack() of the class
 * RowCon can be significantly improved if the variable set
 * is static, i.e., no variables are added or removed during
 * the optimization. Therefore we implement the class SRowCon.
 */
class  SRowCon :  public RowCon  {
public:

	//! Creates a row constraint.
	/**
	* \param master   A pointer to the corresponding master of the optimization.
	* \param sub      A pointer to the subproblem associated with the constraint.
	*                 This can be also the 0-pointer.
	* \param sense    The sense of the constraint.
	* \param nnz      The number of nonzero elements of the constraint.
	* \param support  The array storing the variables with nonzero coefficients.
	* \param coeff    The nonzero coefficients of the variables stored in \a support.
	* \param rhs      The right hand side of the constraint.
	* \param dynamic  If this argument is \a true, then the constraint can be removed
	*                 from the active constraint set during the cutting plane phase
	*                 of the subproblem optimization.
	* \param local    If this argument is \a true, then the constraint is considered
	*                 to be only locally valid. As a locally valid constraint is
	*                 associated with a subproblem, \a sub must not be 0 if \a local is \a true.
	* \param liftable If this argument is \a true, then a lifting procedure must be
	*                 available, i.e., that the coefficients of variables which have not been
	*                 active at generation time of the constraint can be computed.
	*/
	SRowCon(Master *master,
		const Sub *sub,
		CSense::SENSE sense,
		int nnz,
		const Array<int> &support,
		const Array<double> &coeff,
		double rhs,
		bool dynamic,
		bool local,
		bool liftable)
		: RowCon(master, sub, sense, nnz, support, coeff, rhs, dynamic, local, liftable)
	{ }

	//! Creates a row constraint.
	/**
	* \param master   A pointer to the corresponding master of the optimization.
	* \param sub      A pointer to the subproblem associated with the constraint.
	*                 This can be also the 0-pointer.
	* \param sense    The sense of the constraint.
	* \param nnz      The number of nonzero elements of the constraint.
	* \param support  The array storing the variables with nonzero coefficients.
	* \param coeff    The nonzero coefficients of the variables stored in \a support.
	* \param rhs      The right hand side of the constraint.
	* \param dynamic  If this argument is \a true, then the constraint can be removed
	*                 from the active constraint set during the cutting plane phase
	*                 of the subproblem optimization.
	* \param local    If this argument is \a true, then the constraint is considered
	*                 to be only locally valid. As a locally valid constraint is
	*                 associated with a subproblem, \a sub must not be 0 if \a local is \a true.
	* \param liftable If this argument is \a true, then a lifting procedure must be
	*                 available, i.e., that the coefficients of variables which have not been
	*                 active at generation time of the constraint can be computed.
	*/
	SRowCon(Master *master,
		const Sub *sub,
		CSense::SENSE sense,
		int nnz,
		int *support,
		double *coeff,
		double rhs,
		bool dynamic,
		bool local,
		bool liftable)
		: RowCon(master, sub, sense, nnz, support, coeff, rhs, dynamic, local, liftable)
	{ }

	//! The destructor.
	virtual ~SRowCon() { }

	//! Generates the row format of the constraint associated with the variable set \a var.
	/**
	 * This function redefines a virtual function of the base class
	 * RowCon.
	 *
	 * \return It returns the number of nonzero elements in the row format.
	 *
	 * \param var The variable set for which the row format is generated is only a dummy
	 *            since the the variable set is assumed to be fixed for this constraint class.
	 * \param row Holds the row format of the constraint after the execution of this function.
	 */
	virtual int genRow(Active<Variable, Constraint> *var, Row &row) const override {
		row.copy(row_);
		return row_.nnz();
	}

	//! Computes the slack of a vector associated with the variable set \a variables.
	/**
	 * This function redefines a virtual function of the base class RowCon.
	 *
	 * \param variables The variable set for which the row format is generated
	 *                  is only a dummy since the the variable set is assumed
	 *                  to be fixed for this constraint class.
	 * \param x         An array of length equal to the number of variables.
	 *
	 * \return The slack of the vector \a x.
	 */
	virtual double slack(Active<Variable, Constraint> *variables,
		double *x) const override;
};

}
