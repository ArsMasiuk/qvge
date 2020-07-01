/*!\file
 * \author Matthias Elf
 * \brief variable identified by a number.
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

#include <ogdf/lib/abacus/variable.h>

namespace abacus {


//! Variables identified by a number.
/**
 * This class is derived from the class Variable and implements
 * a variable which is uniquely defined by a number.
 */
class OGDF_EXPORT NumVar : public Variable {
public:

	//! The constructor.
	/**
	 * \param master  A pointer to the corresponding master of the optimization.
	 * \param sub     A pointer to the subproblem associated with variable. This
	 *                can also be the 0-pointer.
	 * \param number  The number of the column associated with the variable.
	 * \param dynamic If this argument is \a true, then the variable can also be
	 *                removed again from the set of active variables after it is
	 *                added once.
	 * \param local   If this argument is \a true, then the variable is only locally
	 *                valid, otherwise it is globally valid. As a locally valid variable
	 *                is associated with a subproblem, \a sub must not be 0, if \a local
	 *                is \a true.
	 * \param obj     The objective function coefficient of the variable.
	 * \param lBound  The lower bound of the variable.
	 * \param uBound  The upper Bound of the variable.
	 * \param type    The type of the variable.
	 */
	NumVar(Master *master,
		const Sub *sub,
		int number,
		bool dynamic,
		bool local,
		double obj,
		double lBound,
		double uBound,
		VarType::TYPE type)
		: Variable(master, sub, dynamic, local, obj, lBound, uBound, type), number_(number)
	{ }

	//! The destructor.
	virtual ~NumVar() { }

	//! Writes the number of the variable to an output stream.
	/**
	 * \param out  The output stream.
	 * \param rhs  The variable being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<<(std::ostream &out, const NumVar &rhs) {
		return out << '(' << rhs.number_  << ')' << std::endl;
	}

	//! Returns the number of the variable.
	int number() const { return number_; }

protected:

	//! The identification number of the variable.
	int number_;
};

}
