/*!\file
 * \author Matthias Elf
 * \brief variable.
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

#include <ogdf/lib/abacus/convar.h>
#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/fsvarstat.h>
#include <ogdf/lib/abacus/vartype.h>
#include <ogdf/lib/abacus/constraint.h>

namespace abacus {


class Master;
class Sub;
#if 0
class VarType;
#endif
class Column;
class Constraint;

template<class BaseType, class CoType> class Active;


//! Forms the virtual base class for all possible variables given in pool format.
/**
 * Variables are one of the central items in a linear-programming based branch-and-bound algorithm.
 * This class forms the virtual base class for all possible variables
 * given in pool format and is derived from the common base class ConVar of
 * all constraints and variables.
 */
class OGDF_EXPORT Variable : public ConVar {
public:

	//! Initializes a variable.
	/**
	 * \param master  A pointer to the corresponding master of the optimization.
	 * \param sub     A pointer to the subproblem associated with the variable. This
	 *                can also be the 0-pointer.
	 * \param dynamic If this argument is \a true, then the variable can also be
	 *                removed again from the set of active variables after it is added once.
	 * \param local   If this argument is \a true, then the variable is only locally
	 *                valid, otherwise it is globally valid. As a locally valid variable is
	 *                always associated with a subproblem, the argument \a sub must
	 *                not be 0 if \a local is \a true.
	 * \param obj     The objective function coefficient.
	 * \param lBound  The lower bound of the variable.
	 * \param uBound  The upper bound of the variable.
	 * \param type    The type of the variable.
	 */
	Variable(
		Master *master,
		const Sub *sub,
		bool dynamic,
		bool local,
		double obj,
		double lBound,
		double uBound,
		VarType::TYPE type)
		  : ConVar(master, sub, dynamic, local),
			fsVarStat_(master), obj_(obj), lBound_(lBound), uBound_(uBound), type_(type)
	{ }


	virtual ~Variable() { }


	//! Returns the type of the variable.
	VarType::TYPE varType() const { return type_.type(); }


	//! Returns true if the type of the variable is \a Integer or \a Binary, false otherwise.
	bool discrete() const { return type_.discrete(); }


	//! Returns true If the type of the variable is \a Binary, false otherwise.
	bool binary() const { return type_.binary(); }


	//! Returns true If the type of the variable is \a Integer, false otherwise.
	bool integer() const { return type_.integer(); }


	//! Returns the objective function coefficient.
	virtual double obj() const { return obj_; }


	//! Returns the upper bound of the variable.
	double uBound() const { return uBound_; }


	//! Sets the upper bound of the variable to \a newBound.
	/**
	 * \param newBound The new value of the upper bound.
	 */
	void uBound(double newBound) { uBound_ = newBound; }


	//! Returns the lower bound of the variable.
	double lBound() const { return lBound_; }


	//! Sets the lower bound of the variable to \a newBound.
	/**
	 * \param newBound The new value of the lower bound.
	 */
	void lBound(double newBound) { lBound_ = newBound; }


	/**
	 * \note This is the global status of fixing/setting that might differ
	 * from the local status of fixing/setting a variable returned by
	 * the function Sub::fsVarStat().
	 *
	 * \return A pointer to the global status of fixing and setting of the variable.
	 */
	FSVarStat *fsVarStat() { return &fsVarStat_; }

	/**
	 * \note This is the global status of fixing/setting that might differ
	 * from the local status of fixing/setting a variable returned by
	 * the function Sub::fsVarStat().
	 *
	 * \return A const pointer to the global status of fixing and setting of the variable.
	 */
	const FSVarStat *fsVarStat() const { return &fsVarStat_; }

	//! Returns true if the variable is valid, false otherwise.
	/**
	 * \return true If the variable is globally valid, or the subproblem \a sub
	 *              is an ancestor in the enumeration tree of the subproblem
	 *              associated with the variable.
	 * \return false otherwise.
	 *
	 * \param sub The subproblem for which validity of the variable is checked.
	 */
	virtual bool valid(const Sub *sub) const;

	//! Computes the column \a col of the variable associated with the active constraints \a *actCon.
	/**
	 * \note The upper and lower bound of the column are initialized
	 * with the global upper and lower bound of the variable.
	 * Therefore, an adaption with the local bounds might be required.
	 *
	 * \param actCon The constraints for which the column of the variable should be computed.
	 * \param col    Stores the column when the function terminates.
	 *
	 * \return The number of nonzero entries in \a col.
	 */
	virtual int genColumn(Active<Constraint, Variable> *actCon, Column &col) const;

	//! Computes the coefficient of the variable in the constraint \a con.
	/**
	 * Per default the coefficient of a variable iscomputed indirectly via the coefficient
	 * of a constraint. Problem specific redefinitions might be required.
	 *
	 * \param con The constraint of which the coefficient should be computed.
	 *
	 * \return The coefficient of the variable in the constraint \a con.
	 */
	virtual double coeff(const Constraint *con) const {
		return con->coeff(this);
	}


	//!  Checks, if a variable does not price out correctly.
	/**
	 * I.e., if the reduced cost \a rc is positive fora maximization problem and
	 * negative for a minimization problem, respectively.
	 *
	 * \param rc The reduced cost of the variable.
	 *
	 * \return true If the variable does not price out correctly, false otherwise.
	 */
	virtual bool violated(double rc) const;

	//!  Checks, if a variable does not price out correctly.
	/**
	 * I.e., if the reduced cost of the variable associated with the constraint set
	 * \a constraints and the dual variables \a y are positive for a maximization problem
	 * and negative for a minimization problem, respectively.
	 *
	 * \param constraints The constraints associated with the dual variables \a y.
	 * \param y           The dual variables of the constraint.
	 * \param slack       If \a r is not the 0-pointer, it will store the reduced cost after the
	 *                    function call. Per default \a r is 0.
	 *
	 * \return true if the variable does not price out correctly, false otherwise.
	 */
	virtual bool violated(Active<Constraint, Variable> *constraints,
		double *y, double *slack = nullptr) const;

	//! Computes the reduced cost of the variable corresponding the constraint set \a actCon and the dual variables \a y.
	/**
	 * Given the dual variables \f$y\f$, then the reduced cost of a variable
	 * with objective function coefficient \f$c_e\f$, column \f$a_{.e}\f$ are defined
	 * as \f$c_e - y^{\mathrm{T}} a_{.e}\f$.
	 *
	 * \param actCon The constraints associated with the dual variables \a y.
	 * \param y      The dual variables of the constraint.
	 *
	 * \return The reduced cost of the variable.
	 */
	virtual double redCost(Active<Constraint, Variable> *actCon,
		double *y) const;

	//! Returns whether an (inactive) discrete variable is useful.
	/**
	 * An (inactive) discrete variable is considered as \a useful() if its
	 * activation might not produce only solutions worse than the
	 * best known feasible solution.
	 *
	 * This is the same criterion for
	 * fixing inactive variables by reduced cost criteria.
	 *
	 * \param actCon The active constraints.
	 * \param y      The dual variables of these constraints.
	 * \param lpVal  The value of the linear program.
	 *
	 * \return true If the variable is considered as useful, false otherwise.
	 */
	virtual bool useful(Active<Constraint, Variable> *actCon,
		double *y,
		double lpVal) const;

	//! Writes the column of the variable corresponding to the \a constraints to output stream \a out.
	/**
	 * \param out         The output stream.
	 * \param constraints The constraints for which the column should be written.
	 */
	void printCol(std::ostream &out,
		Active<Constraint, Variable> *constraints) const;

protected:

	FSVarStat fsVarStat_;	//!< The global status of fixing and setting of the variable.
	double obj_;				//!< The objective function coefficient of the variable.
	double lBound_;				//!< The lower bound of the variable.
	double uBound_;				//!< The upper bound of the variable.
	VarType type_;			//!< The type of the variable.
};

inline bool Variable::valid(const Sub *sub) const
{
	OGDF_ASSERT(!local_ || sub != nullptr);

	return (!local_ || sub->ancestor(sub_));
}



}
