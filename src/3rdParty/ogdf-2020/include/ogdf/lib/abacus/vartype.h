/*!\file
 * \author Matthias Elf
 * \brief vartype.
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


//! Variable types.
/**
 * Variables can be of three different types: \a Continuous,
 * \a Integer or \a Binary. We distinguish \a Integer and \a Binary variables
 * since some operations are performed differently (e.g., branching).
 */
class  VarType :  public AbacusRoot  {
public:

	//! The enumeration with the different variable types.
	enum TYPE {
		Continuous,	//!< A continuous variable.
		Integer,	//!< A general integer variable.
		Binary		//!< A variable having value 0 or 1.
	};

	//! The default constructor: Lets the type of the variable uninitialized.
	VarType() { }

	//! Creates a variable type \a t.
	/**
	 * \param t The variable type.
	 */
	VarType(TYPE t) : type_(t) { }

	//! Output operator for variable types.
	/**
	 * Writes the variable type to an output stream in
	 * the format <tt>Continuous</tt>, <tt>Integer</tt>, or <tt>Binary</tt>.
	 *
	 * \param out The output stream.
	 * \param rhs The variable type being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<<(std::ostream &out, const VarType &rhs);

	//! Returns the type of the variable.
	TYPE type() const { return type_; }


	//! Sets the variable type to \a t.
	/**
	 * \param t The new type of the variable.
	 */
	void type(TYPE t) { type_ = t; }


	//! Returns true if the type of the variable is \a Integer or \a Binary, false otherwise.
	bool discrete() const { return (type_ != Continuous); }


	//! Returns true if the type of the variable is \a Binary, false otherwise.
	bool binary() const { return (type_ == Binary); }


	//! Returns true if the type of the variable is \a Integer, false otherwise.
	bool integer() const { return (type_ == Integer); }

private:

	TYPE type_; //!< The type of the variable.
};

}
