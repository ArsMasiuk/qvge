/*!\file
 * \author Matthias Elf
 * \brief constraint.
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

#include <ogdf/lib/abacus/infeascon.h>
#include <ogdf/lib/abacus/csense.h>
#include <ogdf/lib/abacus/conclass.h>


namespace abacus {

class Row;
class Master;
class Variable;
class LpSub;

template<class BaseType, class CoType> class Active;


//! Forms the virtual base class for all possible constraints given in pool format.
/**
 * Constraints are one of the central items in a linear-programming based branch-and-bound algorithm.
 * This class forms the virtual base class for all possible constraints
 * given in pool format and is derived from the common base class ConVar of
 * all constraints and variables.
 */
class OGDF_EXPORT Constraint : public ConVar {

	friend class LpSub;

public:

	//! Initializes a constraint.
	/**
	 * \param master  A pointer to the corresponding master of the optimization.
	 * \param sub     A pointer to the subproblem associated with the constraint.
	 *                This can be also a 0-pointer.
	 * \param sense   The sense of the constraint.
	 * \param rhs     The right hand side of the constraint.
	 * \param dynamic If this parameter is true, then the constraint can be removed
	 *                from the active constraint set during the cutting plane phase
	 *                of the subproblem optimization.
	 * \param local   If this parameter is true, then the constraint is considered
	 *                to be only locally valid. In this case the paramument sub must
	 *                not be 0 as each locally valid constraint is associated with a subproblem.
	 * \param liftable If this parameter is true, then a lifting procedure must be available,
	 *                i.e., that the coefficients of variables which have not been
	 *                active at generation time of the constraint can be computed.
	 */
	Constraint (
		Master *master,
		const Sub *sub,
		CSense::SENSE sense,
		double rhs,
		bool dynamic,
		bool local,
		bool liftable);


	//! Initializes an empty constraint.
	/**
	 * This constructor is, e.g., useful if parallel separation is applied.
	 * In this case the constraint can be constructed and receive later its
	 * data by message passing.
	 *
	 * \param master A pointer to the corresponding master of the optimization.
	 */
	Constraint (Master *master);


	//! Copy constructor
	/**
	 * \param rhs The constraint being copied.
	 */
	Constraint(const Constraint &rhs)
		: ConVar(rhs), sense_(rhs.sense_), rhs_(rhs.rhs_), conClass_(nullptr), liftable_(rhs.liftable_)
	{
		if (rhs.conClass_) conClass_ = new ConClass(*(rhs.conClass_));
	}


	virtual ~Constraint() { delete conClass_; }


	//! Returns a pointer to the sense of the constraint.
	CSense *sense() { return &sense_; }

	//! Returns a const pointer to the sense of the constraint.
	const CSense *sense() const { return &sense_; }


	//! Returns the coefficient of the variable \a v in the constraint.
	/**
	 * \param v  A pointer to a variable.
	 *
	 * \return   The coefficient of the variable \a v in the constraint.
	 */
	virtual double coeff(const Variable *v) const = 0;

	//! Returns the right hand side of the constraint.
	virtual double rhs() const { return rhs_; }


	//! Checks if the constraint is liftable.
	/**
	 * I.e., if the coefficients of variables inactive at generation time of the constraint
	 * can be computed later.
	 *
	 * \return true If the constraint can be lifted, false otherwise.
	 */
	bool liftable() const { return liftable_; }


	//! Checks if the constraint is valid for the subproblem \a sub.
	/**
	 * Per default, this is the case if the constraint is globally valid, or the subproblem
	 * associated with the constraint is an ancestor of the subproblem \a sub in the
	 * enumeration tree.
	 *
	 *\param sub The subproblem for which the validity is checked.
	 *
	 *\return true If the constraint is valid for the subproblem sub, false otherwise.
	 */
	virtual bool valid(Sub *sub) const;


	//! Generates the row format of the constraint associated with the variable set \a var.
	/**
	 * This function is declared virtual since faster constraint specific
	 * implementations might be desirable.
	 *
	 * All nonzero coefficients are added to the row format. Before
	 * we generate the coefficients we try to expand the constraint,
	 * afterwards it is compressed again.
	 *
	 * \param var The variable set for which the row format should be computed.
	 * \param row Stores the row format after calling this function.
	 *
	 * \return The number of nonzero elements in the row format \a row.
	 */
	virtual int genRow(Active<Variable, Constraint> *var,
		Row &row) const;

	//! Computes the slack of the vector \a x associated with the variable set \a variables.
	/**
	 *
	 * \param variables The variable set associated with the vector \a x.
	 * \param x         The values of the variables.
	 * \return The slack induced by the vector \a x.
	 */
	virtual double slack(Active<Variable, Constraint> *variables,
		double *x) const;

	//! Checks if a constraint is violated by a vector \a x associated with a variable set.
	/**
	 *
	 * \param variables The variables associated with the vector \a x.
	 * \param x         The vector for which the violation is checked.
	 * \param sl        If \a sl is nonzero, then \a *sl will store the value of the
	 *                  violation, i.e., the slack.
	 * \return true If the constraint is violated, false otherwise.
	 */
	virtual bool violated(
		Active<Variable, Constraint> *variables, double *x, double *sl = nullptr) const;

	//! Checks if a constraint is violated given the \a slack of a vector.
	/**
	 * \param slack The slack of a vector.
	 *
	 * \return true  if the constraint is an equation and the \a slack is nonzero.
	 * \return true  if the constraint is a \f$\le\f$-inequality and the slack is negative.
	 * \return true  if the constraint is a \f$\ge\f$-inequality and the slack is positive.
	 * \return false otherwise.
	 */
	virtual bool violated(double slack) const;

	//! Writes the row format of the constraint associated with the variable set \a var to output stream \a out.
	/**
	 * \param out The output stream.
	 * \param var The variables for which the row format should be written.
	 */
	void printRow(std::ostream &out, Active<Variable, Constraint> *var) const;

	//! Returns the Euclidean distance of \a x associated with variable set \a actVar to the hyperplane induced by the constraint.
	/**
	 * The distance of a point \f$\overline{x}\f$ and a hyperplane
	 * \f$a^T x = \beta\f$ can be computed in the following way:
	 * Let \f$y\f$ be the intersection of the hyperplane \f$a^T x = \beta\f$
	 * and the line defined by \f$\overline{x}\f$ and the vector \f$a\f$.
	 * Then the distance \f$d\f$ is the length of the vector \f$||\overline{x} - y||\f$.
	 *
	 * \param x      The point for which the distance should be computed.
	 * \param actVar The variables associated with \a x.
	 *
	 * \return The Euclidean distance of the vector \a x associated with the variable
	 * set \a actVar to the hyperplane induced by the constraint.
	 */
	virtual double distance(double *x, Active<Variable, Constraint> *actVar) const;

	//Constraint *duplicate() { return 0; }

	//! Returns a pointer to the classification of the constraint.
	/**
	 * If no classification is available then we try to classify the constraint.
	 * In this case \a var must not be a 0-pointer.
	 *
	 * A constraint classification can only be generated if the function
	 * classify() is redefined in a derived class.
	 */
	ConClass *classification(Active<Variable, Constraint> *var = nullptr) const;

protected:

	/**
	 * Can be called if after variable elimination the left hand side of the constraint
	 * has become void and the right hand side has been adapted to \a newRhs.
	 *
	 * Then this function checks if the constraint is violated.
	 *
	 * \param newRhs The right hand side of the constraint after the elimination of the variables.
	 *
	 * \return TooLarge or TooSmall if the \a newRhs violates the sense of the constraint, i.e,
	 *         it is < / > / != 0 and the sense of the constraint is >= / <= / =,
	 * \return Feasible otherwise.
	 */
	virtual InfeasCon::INFEAS voidLhsViolated(double newRhs) const;

	/**
	 * The default implementation returns a 0 pointer.
	 */
	virtual ConClass *classify(Active<Variable, Constraint> *var) const {
		return nullptr;
	}


	CSense sense_; //!< The sense of the constraint.

	double rhs_; //!< The right hand side of the constraint.

	mutable ConClass *conClass_; //this is just s cached-value, and hence can be set in const classify-call (it's nullptr initially, and computed upon first call

	/**
	 * This member is  \a true if also coefficients of variables which have been
	 * inactive at generation time can be computed, \a false otherwise.
	 */
	bool liftable_;

private:
	const Constraint &operator=(const Constraint &rhs);
};

}

#include <ogdf/lib/abacus/sub.h>
#include <ogdf/lib/abacus/master.h>

namespace abacus {

inline Constraint::Constraint (
	Master *master,
	const Sub *sub,
	CSense::SENSE sense,
	double rhs,
	bool dynamic,
	bool local,
	bool liftable)
	: ConVar(master, sub, dynamic, local), sense_(sense), rhs_(rhs), conClass_(nullptr), liftable_(liftable)
{
	if (local && sub == nullptr) {
		Logger::ifout() << "Constraint::Constraint(): subtree of local item must not be 0\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Constraint);
	}
}


inline Constraint::Constraint (Master *master)
	: ConVar(master, nullptr, true, true), conClass_(nullptr) { }


inline bool Constraint::valid(Sub *sub) const {
	OGDF_ASSERT(!local_ || sub != nullptr);

	return (!local_ || sub_->ancestor(sub));
}

}
