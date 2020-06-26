/*!\file
 * \author Matthias Elf
 *
 * \brief csense.
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

class AbacusGlobal;


//! Sense of constraints.
/**
 * The most important objects in a cutting plane algorithm are
 * constraints, which can be equations (\a Equal) or
 * inequalities with the sense \a Less or \a Greater.
 * We implement the sense of optimization as a class
 * since we require it both in the classes Constraint and Row.
 */
class OGDF_EXPORT CSense : public AbacusRoot {
public:

	enum SENSE { Less, Equal, Greater };

	//! Default constructor, sense is undefined.
	CSense() { }

	//! Initializes the sense to \a s.
	/**
	 * \param s  The sense.
	 */
	CSense(const SENSE s) : sense_(s) { }

	//! Initializes the sense of the constraint specified with a single letter.
	/**
	 * \param s  A character representing the sense:
	 *           <tt>E</tt> or <tt>e</tt> stand for \a Equal,
	 *           <tt>G</tt> and <tt>g</tt> stand for \a Greater, and
	 *           <tt>L</tt> or <tt>l</tt> stand for \a Less.
	 */
	CSense(char s);

	//! Output operator for constraint senses.
	/**
	 * The output operator writes the sense on an output stream in the form \a <=, \a =, or \a >=.
	 *
	 * \param out The output stream.
	 * \param rhs The sense being output.
	 *
	 * \return The output stream.
	 */
	friend OGDF_EXPORT std::ostream& operator<<(std::ostream &out, const CSense &rhs);

	//! Assignment operator.
	/**
	 * The default assignment operator is overloaded such that also the
	 * enumeration \a SENSE can be used on the right hand side.
	 *
	 * \param rhs The new sense.
	 *
	 * \return A reference to the sense.
	 */
	const CSense &operator=(SENSE rhs) {
		sense_ = rhs;
		return *this;
	}

	//! Returns the sense of the constraint.
	SENSE sense() const { return sense_; }

	//! Changes the sense of the constraint.
	/**
	 * \param s The new sense.
	 */
	void sense(SENSE s) { sense_ = s; }

	//! Changes the sense of the constraint given a letter \a s.
	/**
	 * \param s The new sense.
	 */
	void sense(char s);

private:

	SENSE sense_; //!< Stores the sense of a constraint.
};

}
