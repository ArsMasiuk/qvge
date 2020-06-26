/*!\file
 * \author Matthias Elf
 * \brief constraint classification.
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

#include <ogdf/lib/abacus/master.h>

namespace abacus {


//! Constraint classification.
/**
 * For the generation of knapsack cuts in mixed integer optimization problem, a constraint
 * requires certain attributes about the types of its variables
 * and the structure of the constraint. A rather rudimentary
 * classification is implemented in the class ConClass.
 */
class OGDF_EXPORT ConClass : public AbacusRoot {
public:

	/*! \brief The constructor initializes the constraint classification with
	 *
	 * \param master A pointer to the corresponding master of the optimization.
	 * \param discrete
	 * \param allVarBinary \a true if all variables are binary
	 * \param trivial \a true if it is a bound or a variable upper bound
	 * \param bound \a true if the constraint is a bound of the variable
	 * \param varBound \a true if the constraint is a variable upper bound
	 */
	ConClass(
		const Master *master,
		bool discrete,
		bool allVarBinary,
		bool trivial,
		bool bound,
		bool varBound)
	:
		discrete_(discrete),
		allVarBinary_(allVarBinary),
		trivial_(trivial),
		bound_(bound),
		varBound_(varBound)
	{ }

	//! Output operator for constraint classifications.
	friend OGDF_EXPORT std::ostream &operator<<(std::ostream &out, const ConClass &rhs);

	//! Returns \a true if all variables with nonzero coefficients of the constraint are binary.
	bool allVarBinary() const { return allVarBinary_; }

	//! Returns \a true if the constraint is a bound or a variable upper bound.
	bool trivial() const { return trivial_; }


private:

	bool  discrete_;

	//! \a true if all variables are binary.
	bool  allVarBinary_;

	//! \a true if it is a bound or a variable lower/upper bound.
	bool  trivial_;

	//! \a true if the constraint is a bound of the variable.
	bool  bound_;

	//! \a true if the constraint is a variable lower/upper bound.
	bool  varBound_;
};

}
