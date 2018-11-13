/*!\file
 * \author Matthias Elf
 * \brief row.
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
#include <ogdf/lib/abacus/csense.h>

namespace abacus {


//! Representation of constraints in the row format.
/**
 * This class refines its base class SparVec for the representation of
 * constraints in the row format. This class plays an essential role in the
 * interface with the LP-solver.
 *
 * This class should not be confused with the class Constraint, which
 * is an abstract class for the representation of constraints within
 * the framework.
 * Moreover, the class RowCon derived from the class Constraint provides
 * a constraint representation in row format, but there are also other
 * representations of constraints.
 */
class  Row :  public SparVec  {
public:

	//! Creates a row and initializes it.
	/**
	 * \param glob  A pointer to the corresponding global object.
	 * \param nnz   The number of nonzero elements of the row.
	 * \param s     The array storing the nonzero elements.
	 * \param c     The array storing the nonzero coefficients of the elements of \a s.
	 * \param sense The sense of the row.
	 * \param r     The right hand side of the row.
	 */
	Row(AbacusGlobal *glob,
		int nnz,
		const Array<int> &s,
		const Array<double> &c,
		const CSense &sense,
		double r)
		: SparVec(glob, nnz, s, c), sense_(sense), rhs_(r) { }

	//! Creates a row and initializes it.
	/**
	 * \param glob  A pointer to the corresponding global object.
	 * \param nnz   The number of nonzero elements of the row.
	 * \param s     The array storing the nonzero elements.
	 * \param c     The array storing the nonzero coefficients of the elements of \a s.
	 * \param sense The sense of the row.
	 * \param r     The right hand side of the row.
	 */
	Row(AbacusGlobal *glob,
		int nnz,
		const Array<int> &s,
		const Array<double> &c,
		const CSense::SENSE sense,
		double r)
		: SparVec(glob, nnz, s, c), sense_(sense), rhs_(r) { }

	//! Creates a row and initializes it using C-style arrays.
	/**
	 * \param glob  A pointer to the corresponding global object.
	 * \param nnz   The number of nonzero elements of the row.
	 * \param s     The array storing the nonzero elements.
	 * \param c     The array storing the nonzero coefficients of the elements of \a s.
	 * \param sense The sense of the row.
	 * \param r     The right hand side of the row.
	 */
	Row(AbacusGlobal *glob,
		int nnz,
		int *s,
		double *c,
		CSense::SENSE sense,
		double r)
		: SparVec(glob, nnz, s, c), sense_(sense), rhs_(r) { }

	//! Creates a row without initialization of the nonzeros of the row.
	/**
	 * \param glob A pointer to the corresponding global object.
	 * \param size The maximal numbers of nonzeros.
	 */
	Row(AbacusGlobal *glob, int size) : SparVec(glob, size) { }

	//! The destructor.
	~Row() { }

	//! \brief The output operator
	/**
	 * Writes the row on an output stream in format like <tt>-2.5 x1 + 3 x3 <= 7</tt>.
	 *
	 * Only variables with nonzero coefficients are output.
	 * The output operator does neither output a \a '+' before the first
	 * coefficient of a row, if it is positive, nor outputs coefficients
	 * with absolute value 1.
	 *
	 * \param out The output stream.
	 * \param rhs The row being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<<(std::ostream& out, const Row &rhs);

	//! Returns the right hand side stored in the row format.
	double rhs() const { return rhs_; }

	//! Sets the right hand side of the row to \a r.
	/**
	 * \param r The new value of the right hand side.
	 */
	void rhs(double r) { rhs_ = r; }

	//! Returns a pointer to the sense of the row.
	CSense *sense() { return &sense_; }

	//! Returns a const pointer to the sense of the row.
	const CSense *sense() const { return &sense_; }

	//! Sets the sense of the row to \a s.
	/**
	 * \param s The new sense of the row.
	 */
	void sense(CSense &s) { sense_ = s; }

	//! Sets the sense of the row to \a s.
	/**
	 * \param s The new sense of the row.
	 */
	void sense(CSense::SENSE s) { sense_.sense(s); }

	//! Copies \p row.
	/**
	 * Behaves like an assignment operator, however, the maximal number of
	 * the elements of this row only has to be at least the number of nonzeros of \a row.
	 *
	 * \param row The row that is copied.
	 */
	void copy(const Row &row);

	//! Removes the indices listed in \a buf from the support of the row and subtracts \a rhsDelta from its right hand side.
	/**
	 * \param buf      The components being removed from the row.
	 * \param rhsDelta The correction of the right hand side of the row.
	 */
	void delInd(ArrayBuffer<int> &buf, double rhsDelta) {
		leftShift(buf);
		rhs_ -= rhsDelta;
	}

protected:

	CSense sense_;  //!< The sense of the row.
	double rhs_;        //!< The right hand side of the row.
};


}
