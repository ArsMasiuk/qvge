/*!\file
 * \author Matthias Elf
 * \brief solution history
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


//! Solution histories.
/**
 * This class implements the storage of the solution history. Each time
 * when a better feasible solution or globally valid dual bound
 * is found, it should be memorized in this class.
 */
class OGDF_EXPORT History : public AbacusRoot {
public:

	//! Creates a history table with 100 possible entries.
	/**
	 * If this number is exceeded an automatic reallocation is performed.
	 *
	 * \param master A pointer to the corresponding master of the optimization.
	 */
	History(Master *master) :
		master_(master),
		primalBound_(100),
		dualBound_(100),
		time_(100),
		n_(0)
	{ }

	virtual ~History() { }

	//! The output operator.
	/**
	 * \param out The output stream.
	 * \param rhs The solution history being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend OGDF_EXPORT std::ostream& operator<<(std::ostream& out, const History &rhs);

	//! Adds an additional line to the history table.
	/**
	 * Primal bound, dual bound, and the time are taken from the
	 * corresponding master object.
	 * The history table is automatically reallocated if necessary.
	 *
	 * Usually an explicit call to this function from an application class
	 * is not required since \a update() is automatically called if
	 * a new global primal or dual bound is found.
	 */
	void update();

private:

	//! Returns the length of the history table.
	int size() const { return primalBound_.size(); }

	//! The function \a realloc() enlarges the history table by 100 components.
	void realloc();


	//! A pointer to corresponding master of the optimization.
	Master *master_;

	//! The array storing the value of the best primal solution.
	Array<double> primalBound_;

	//! The array storing the value of the best dual solution.
	Array<double> dualBound_;

	//! The CPU time in seconds, when the entry in the table was made.
	Array<int64_t> time_;

	//! The number of entries in the history table.
	int n_;
};

}
