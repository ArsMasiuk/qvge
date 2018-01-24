/*!\file
 * \author Matthias Elf
 * \brief status of variables.
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


//! status of variables.
/**
 * After the solution of a linear program by the simplex method
 * each variable receives a status indicating if the variable
 * is contained in the basis of the optimal solution, or is
 * nonbasic and has a value equal to its lower or upper bound,
 * or is a free variable not contained in the basis. We extend
 * this notion since later in the interface from a cutting plane
 * algorithm to the linear program variables might be eliminated.
 */
class OGDF_EXPORT LPVARSTAT : public AbacusRoot {
public:

	//! The enumeration of the statuses a variable gets from the linear program solver.
	enum STATUS {
		AtLowerBound,	/*!< The variable is at its lower bound, but not in the basis. */
		Basic,			/*!< The variable is in the basis. */
		AtUpperBound,	/*!< The variable is at its upper bound , but not in the basis. */
		NonBasicFree,	/*!< The variable is unbounded and not in the basis. */
		Eliminated,		/*!< The variable has been removed by our preprocessor
						 *   in the class LpSub. So, it is not present in the LP-solver. */
		Unknown			/*!< The LP-status of the variable is unknown since no
						 *   LP has been solved. This status is also assigned
						 *   to variables which are fixed or set, yet still
						 *   contained in the \a LP to avoid a wrong setting
						 *   or fixing  by reduced costs. */
	};


	//! This constructor initializes the status as \a Unknown.
	LPVARSTAT() : status_(Unknown) { }

	//! This constructor initializes the LPVARSTAT.
	/**
	 * \param status The initial status.
	 */
	LPVARSTAT(STATUS status) : status_(status) { }

	//! This constructor make a copy of \a *lpVarStat.
	/**
	 * \param lpVarStat A copy of this object is made.
	 */
	LPVARSTAT(LPVARSTAT *lpVarStat) :
		status_(lpVarStat->status_)
	{ }

	//! The output operator.
	/**
	 * Writes the \a STATUS to an output stream
	 * in the form <tt>AtLowerBound</tt>, <tt>Basic</tt>, <tt>AtUpperBound</tt>,
	 * <tt>NonBasicFree</tt>, <tt>Eliminated</tt>, <tt>Unknown</tt>.
	 *
	 * \param out The output stream.
	 * \param rhs The status being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend OGDF_EXPORT std::ostream &operator<<(std::ostream& out, const LPVARSTAT &rhs);

	//! Returns the LP-status.
	STATUS status() const { return status_; }

	//! Sets the status to \a stat.
	/**
	 * \param stat The new LP-status.
	 */
	void status(STATUS stat) { status_ = stat; }

	//! Sets the status to \a stat.
	/**
	 * \param stat The new LP-status.
	 */
	void status(const LPVARSTAT *stat) { status_ = stat->status_; }

	//! Returns true if the variable status is \a AtUpperBound or \a AtLowerBound, false otherwise.
	bool atBound() const {
		return (status_ == AtLowerBound || status_ == AtUpperBound);
	}

	//! Returns true If the status is \a Basic, false otherwise.
	bool basic() const {
		return (status_ == Basic);
	}


private:

	//! The LP-status.
	STATUS status_;

	OGDF_NEW_DELETE
};

}
