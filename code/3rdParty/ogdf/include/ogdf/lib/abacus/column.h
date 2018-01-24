/*!\file
 * \author Matthias Elf
 * \brief column.
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

#include <ogdf/lib/abacus/sparvec.h>

namespace abacus {

//! Representation of variables in column format.
/**
 * The class Column refines SparVec for the representation of variables in column format.
 * In the same way as the class Row refines the class SparVec for the
 * representation of constraints in row format, the class Column refines
 * SparVec for the representation of variables in column format.
 * This class should not be confused with the class Variable for the
 * abstract representation of variables within the framework. Again, there
 * is a class ColVar derived from Variable having a member of type
 * Column, but there are also other classes derived from Variable.
 */
class OGDF_EXPORT Column : public SparVec {
public:

	//! Creates and initializes a column.
	/**
	 * \param glob A pointer to the corresponding global object.
	 * \param obj  The objective function coefficient.
	 * \param lb   The lower bound.
	 * \param ub   The upper bound.
	 * \param nnz  The number of nonzero elements stored in the arrays \a s and \a c.
	 * \param s    An array of the nonzero elements of the column.
	 * \param c    An array of the nonzero coefficients associated with the elements of \a s.
	 */
	Column(AbacusGlobal *glob,
		double obj,
		double lb,
		double ub,
		int nnz,
		Array<int> &s,
		Array<double> &c)
		: SparVec(glob, nnz, s, c), obj_(obj), lBound_(lb), uBound_(ub) { }

	//! Creates an uninitialized column.
	/**
	 * \param glob   A pointer to the corresponding global object.
	 * \param maxNnz The maximal number of nonzero elements that can be stored in the row.
	 */
	Column(AbacusGlobal *glob, int maxNnz) : SparVec(glob, maxNnz) { }

	//! Creates a column initialized by a sparse vector \a vec.
	/**
	 * \param glob  A pointer to the corresponding global object.
	 * \param obj   The objective function coefficient.
	 * \param lb    The lower bound.
	 * \param ub    The upper bound.
	 * \param vec   A sparse vector storing the support and the coefficients of the column.
	 */
	Column(AbacusGlobal *glob,
		double obj,
		double lb,
		double ub,
		SparVec &vec)
		: SparVec(vec), obj_(obj), lBound_(lb), uBound_(ub) { }

	~Column() { }

	//! The output operator.
	/**
	 * \param out The output stream.
	 * \param rhs The column being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend OGDF_EXPORT std::ostream& operator<<(std::ostream &out, const Column &rhs);

	//! Returns the objective function coefficient of the column.
	double obj() const { return obj_; }


	//! Sets the objective function coefficient of the column to \p c.
	/**
	 * \param c The new value of the objective function coefficient.
	 */
	void obj(double c) { obj_ = c; }


	//! Returns the lower bound of the column.
	double lBound() const { return lBound_; }


	//! Sets the lower bound of the column to \a l.
	/**
	 * \param l The new value of the lower bound.
	 */
	void lBound(double l) { lBound_ = l; }


	//! Returns the upper bound of the column.
	double uBound() const { return uBound_; }


	//! Sets the upper bound of the column to \a u.
	/**
	 * \param u The new value of the upper bound.
	 */
	void uBound(double u) { uBound_ = u; }


	//! Copies column \a col.
	/**
	 * Is very similar to the assignment operator, yet the columns do not have
	 * to be of equal size. A reallocation is performed if required.
	 *
	 * \param col The column that is copied.
	 */
	void copy(const Column &col);

private:

	double obj_;     //!< The objective function coefficient of the column.
	double lBound_;  //!< The lower bound of the column.
	double uBound_;  //!< The upper bound of the column.
};

}
