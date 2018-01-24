/*!\file
 * \author Matthias Elf
 * \brief status of fixed and set variables.
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


//! Status of fixed and set variables.
/**
 * If a variable is fixed to a value, then this means that it keeps
 * this value "forever". If it is set, then the variable keeps the
 * value in the subproblem where the setting is performed and in the
 * subproblems of the subtree rooted at this subproblem.
 */
class OGDF_EXPORT FSVarStat : public AbacusRoot {
public:

	//! The enumeration defining the different statuses of variables from the point of view of fixing and setting.
	enum STATUS {
		Free,				//!< The variable is neither fixed nor set.
		SetToLowerBound,	//!< The variable is set to its lower bound.
		Set,				//!< The variable is set to a value which can be accessed with the member function value().
		SetToUpperBound,	//!< The variable is set to its upper bound.
		FixedToLowerBound,	//!< The variable is fixed to its lower bound.
		Fixed,				//!< The variable is fixed to a value which can be accessed with the member function value().
		FixedToUpperBound	//!< The variable is fixed to its upper bound.
	};

	//! Initializes the status to \a Free.
	/**
	 *  \param glob A pointer to a global object.
	 */
	FSVarStat(AbacusGlobal *glob) : glob_(glob), status_(Free) { }

	//! Initializes the status to \a status.
	/**
	 * \param glob   A pointer to a global object.
	 * \param status The initial status that must neither be \a Fixed nor \a Set.
	 *               For these two statuses the next constructor has to be used.
	 */
	FSVarStat(AbacusGlobal *glob, STATUS status) : glob_(glob), status_(status), value_(0.0)
	{
		if (status == Fixed || status == Set) {
			Logger::ifout() << "FSVarStat::FSVarStat(): value to set/fix missing\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::FsVarStat);
		}
	}

	//! Initializes the status explicitely to \a Fixed or \a Set.
	/**
	 * \param glob   A pointer to a global object.
	 * \param status The initial status that must be \a Fixed or \a Set.
	 * \param value  The value associated with the status \a Fixed or \a Set.
	 */
	FSVarStat(AbacusGlobal *glob, STATUS status, double value) : glob_(glob), status_(status), value_(value)
	{
		if (status != Fixed && status != Set) {
			Logger::ifout() << "FSVarStat::FSVarStat(): wrong status for this constructor\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::FsVarStat);
		}
	}

	//! Initializes the status as in \a fsVarStat.
	/**
	 * \param fsVarStat The status is initialized with a copy of \a *fsVarStat.
	 */
	FSVarStat(FSVarStat *fsVarStat) : glob_(fsVarStat->glob_), status_(fsVarStat->status_), value_(fsVarStat->value_) { }


	//! Output operator.
	/**
	 * The output operator writes the status and, if the status is
	 * \a Fixed or \a Set, also its value on an output stream.
	 *
	 * \param out The output stream.
	 * \param rhs The variable status being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend OGDF_EXPORT std::ostream &operator<<(std::ostream& out, const FSVarStat &rhs);

	//! Returns the status of fixing or setting.
	STATUS status() const { return status_; }

	//! Assigns a new status.
	/**
	 * For specifying also a value in case of the statuses \a Fixed or \a Set the
	 * next version of this function can be use.
	 *
	 * \param stat The new status.
	 */
	void status(STATUS stat) { status_ = stat; }

	//! Assigns a new status also for the statuses \a Fixed and \a Set.
	/**
	 * \param stat The new status.
	 * \param val  A value associated with the new status.
	 */
	void status(STATUS stat, double val) {
		status_ = stat;
		value_  = val;
	}

	//! Assigns the status as in \a stat.
	/**
	 * \param stat A pointer to the object that status and value is copied.
	 */
	void status(const FSVarStat *stat) {
		status_ = stat->status_;
		value_  = stat->value_;
	}

	//! Returns the value of fixing or setting if the variable has status \a Fixed or \a Set.
	double value() const { return value_; }

	//! Assigns a new value of fixing or setting.
	/**
	 * \param val The new value.
	 */
	void value(double val) { value_ = val; }

	//! Returns true if the status is \a FixedToLowerBound, \a Fixed, or\a FixedToUpperBound; false otherwise.
	bool fixed() const;

	//! Returns true if the status is \a SetToLowerBound, \a Set, or \a SetToUpperBound; false otherwise.
	bool set() const;

	//! Returns false if the status is \a Free, true otherwise.
	bool fixedOrSet() const {
		return (status_ != Free);
	}

	//! Returns whether there is a contradiction.
	/**
	 * We say there is a contradiction between two status if they are
	 * fixed/set to different bounds or values. However, two statuses are not
	 * contradiction if one of them is "fixed" and the other one is
	 * "set", if this fixing/setting refers to the same bound or value.
	 *
	 * \param fsVarStat A pointer to the status with which contradiction is is tested.
	 *
	 * \return true If there is a contradiction between the status of this
	 *              object and \a fsVarStat, false otherwise.
	 */
	bool contradiction(FSVarStat *fsVarStat) const;

	//! Returns whether there is a contradiction.
	/**
	 * \param status The status with which contradiction is checked.
	 * \param value  The value with which contradiction is checked.
	 *               The default value of \a value is 0.
	 *
	 * \return true If there is a contradiction between the status of this
	 *              object and (\a status, \a value), false otherwise.
	 */
	bool contradiction(STATUS status, double value = 0) const;

private:

	AbacusGlobal *glob_; //!< A pointer to the corresponding global object.

	STATUS      status_; //!< The status of the variable.

	//! The value the variable is fixed/set to.
	/**
	 *  This member is only used for the statuses \a Fixed and \a Set.
	 */
	double      value_;

	OGDF_NEW_DELETE
};

}
