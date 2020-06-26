/*!\file
 * \author Matthias Elf
 * \brief colvar.
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

// must be included first here
#include <ogdf/lib/abacus/numcon.h>

#pragma once

#include <ogdf/lib/abacus/variable.h>
#include <ogdf/lib/abacus/vartype.h>
#include <ogdf/lib/abacus/column.h>
#include <ogdf/lib/abacus/bheap.h>

namespace abacus {

class SparVec;


/**
 * Some optimization problems, in particular column generation problems, are
 * better described from a variable point of view instead of a constraint
 * point of view. For such context we provide the class ColVar which
 * similar to the class RowCon stores the nonzero coefficient explicitly
 * in an object of the class Column.
 *
 * The constraint class which is associated with this variables class
 * is the class NumCon which identifies constraints only by a unique
 * integer number. NumCon is an abstract class.
 */
class OGDF_EXPORT ColVar : public Variable {
public:

	//! The constructor.
	/**
	 * \param master  A pointer to the corresponding master of the optimization.
	 * \param sub     A pointer to the subproblem associated with the variable.
	 *                This can be also the 0-pointer.
	 * \param dynamic If this argument is \a true, then the variable can be removed
	 *                from the active variable set during the subproblem optimization.
	 * \param local   If this argument is \a true, then the constraint is considered
	 *                to be only locally valid. As a local variable is associated with
	 *                a subproblem, \a sub must not be 0 if local is \a true.
	 * \param lBound  The lower bound of the variable.
	 * \param uBound  The upper bound of the variable.
	 * \param varType The type of the variable.
	 * \param obj     The objective function coefficient of the variable.
	 * \param nnz     The number of nonzero elements of the variable.
	 * \param support The array storing the constraints with the nonzero coefficients.
	 * \param coeff   The nonzero coefficients of the constraints stored in \a support.
	 */
	ColVar(Master *master,
		const Sub *sub,
		bool dynamic,
		bool local,
		double lBound,
		double uBound,
		VarType::TYPE varType,
		double obj,
		int nnz,
		Array<int> &support,
		Array<double> &coeff)
	:
		Variable(master, sub, dynamic, local, obj, lBound, uBound, varType),
		column_(master, obj, lBound, uBound, nnz, support, coeff)
	{ }

	//! Constructor using a sparse vector \a vector.
	/**
	 * \param master  A pointer to the corresponding master of the optimization.
	 * \param sub     A pointer to the subproblem associated with the variable.
	 *                This can be also the 0-pointer.
	 * \param dynamic If this argument is \a true, then the variable can be removed
	 *                from the active variable set during the subproblem optimization.
	 * \param local   If this argument is \a true, then the constraint is considered
	 *                to be only locally valid. As a local variable is associated with
	 *                a subproblem, \a sub must not be 0 if local is \a true.
	 * \param lBound  The lower bound of the variable.
	 * \param uBound  The upper bound of the variable.
	 * \param varType The type of the variable.
	 * \param obj     The objective function coefficient of the variable.
	 * \param vector  The constraints.
	 */
	ColVar(Master *master,
		const Sub *sub,
		bool dynamic,
		bool local,
		double lBound,
		double uBound,
		VarType::TYPE varType,
		double obj, SparVec &vector)
	:
		Variable(master, sub, dynamic, local, obj, lBound, uBound, varType),
		column_(master, obj, lBound, uBound, vector)
	{ }


	virtual ~ColVar() { }

	//! Output operator for column variables.
	/**
	 * The output operator writes the column representing the variable to an output stream.
	 *
	 * \param out The output stream.
	 * \param rhs The variable being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<<(std::ostream &out, const ColVar &rhs);


	//! Writes the column representing the variable to output stream \a out.
	/**
	 * It redefines the virtual function \a print() of the base class ConVar.
	 *
	 * \param out The output stream.
	 */
	virtual void print(std::ostream &out) const {
		out << *this;
	}

	//! Returns the coefficient of the constraint \a con.
	/**
	 *\param con The constraint of which the coefficient is computed.
	 *           This must be a pointer to the class NumCon.
	 */
	virtual double coeff(const Constraint *con) const {
		return column_.origCoeff(((const NumCon*) con)->number());
	}

	//! Computes the coefficient of a constraint with given index \a i.
	/**
	 * \param i The number of the constraint.
	 *
	 * \return The coefficient of constraint \a i.
	 */
	double coeff(int i) const { return column_.origCoeff(i); }


	//! Returns a pointer to the column representing the variable.
	Column *column() { return &column_; }

	//! Returns a const pointer to the column representing the variable.
	const Column *column() const { return &column_; }

protected:

	Column   column_;  //!> The column representing the variable.
};


inline std::ostream &operator<<(std::ostream &out, const ColVar &rhs)
{
	return out << rhs.column_;
}

}
