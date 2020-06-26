/*!\file
 * \author Matthias Elf
 * \brief linear program of a subproblem.
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

#include <ogdf/lib/abacus/lp.h>


namespace abacus {

class InfeasCon;
class Sub;
class Master;
class FSVarStat;
class Constraint;
class Variable;


//! The linear program of a subproblem.
/**
 * This class is derived from the class \a LP to implement the
 * linear programming relaxations of a subproblem. We require
 * this class as the Constraint/Variable format of the
 * constraints/variables has to be transformed to the Row/Column format
 * required by the class \a LP. Moreover the class LpSub is also
 * a preprocessor for the linear programs. Currently we only
 * provide the elimination of (nonbasic) fixed and set variables.
 * Future extensions should be considered.
 *
 * The class LpSub is still an abstract class independent of the
 * used LP-solver. The class for solving LP-relaxation with the
 * LP-solvers supported by the Open Solver Interface (OSI) is the class
 * LpSubOsi.
 */
class OGDF_EXPORT LpSub : public virtual LP {

	friend class Sub;
	friend class SetBranchRule;
	friend class BoundBranchRule;
	friend class ValBranchRule;
	friend class ConBranchRule;
	friend class COPBRANCHRULE;

public:

	//! The constructor.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 * \param sub    The subproblem of which the LP-relaxation is solved.
	 */
	LpSub (Master *master, const Sub *sub);

	//! The destructor
	/**
	 *   deletes the components of \a infeasCons_ since they might
	 *   have been allocated in the constructor and Sub::initializeLp() deletes
	 *   after having tried to add variables restoring feasibility immediately
	 *   LpSub. Afterwards the constructor of LpSub is called again.
	 */
	virtual ~LpSub();

	const Sub *sub() const { return sub_; }

	//! Returns the number of columns which are passed to the LP-solver.
	/**
	 * I.e., the number of active variables
	 * of the subproblem minus the number of eliminated variables.
	 */
	int trueNCol() const { return LP::nCol(); }

	//! Returns the number of nonzeros which are currently present in the constraint matrix of the LP-solver.
	int trueNnz() const { return LP::nnz(); }

	//! We have to redefine the function \a lBound(i) since variables may have been eliminated.
	/**
	 * \param i The number of a variable.
	 *
	 * \return The lower bound of variable \a i. If a variable is eliminated, we
	 *         return the value the eliminated variable is fixed or set to.
	 */
	double lBound(int i)  const;

	//! We have to redefine the function \a uBound(i) since variables may have been eliminated.
	/***
	 * \param i The number of a variable.
	 *
	 * \return The upper bound of variable \a i. If a variable is eliminated, we
	 *         return the value the eliminated variable is fixed or set to.
	 */
	double uBound(int i)  const;

	//! Returns the objective function value of the linear program.
	/**
	 * Since variables might be eliminated we have to add to the solution value
	 * of the LP-solver the objective function part of the eliminated
	 * variables, to get the right value of \a value().
	 */
	virtual double value() const override { return LP::value() + valueAdd_; }

	//! We have to redefine the function \a xVal(i) since variables may have been eliminated.
	/**
	 * \return The x-value of variable \a i after the solution of the linear program.
	 */
	virtual double xVal(int i) const override;

	//! We have to redefine the function \a barXVal(i) since variables may have been eliminated.
	/**
	 * \return The x-value of variable \a i after the solution of the linear
	 *         program before crossing over to a basic solution.
	 */
	virtual double barXVal(int i) const override;

	//! We define the reduced costs of eliminated variables as 0.
	/**
	 * \return The reduced cost of variable \a i.
	 */
	virtual double reco(int i) const override;

	//! Returns the status of the variable in the linear program.
	/**
	 * If the variable \a i is eliminated, then LPVARSTAT::Eliminated is returned.
	 */
	virtual LPVARSTAT::STATUS lpVarStat(int i) const override;


	//! Is called if the last linear program has been solved with the dual simplex method and is infeasible.
	/**
	 * In this case it computes the infeasible basic variable or constraint
	 * and the corresponding row of the basis inverse.
	 *
	 * \return 0 If no error occurs, 1 otherwise.
	 *
	 * \param infeasCon If nonnegative, this is the number of the infeasible slack variable.
	 * \param infeasVar If nonnegative, this is the number of the infeasible structural
	 *                  variable. Note, either \a infeasCon or \a infeasVar is nonnegative.
	 * \param bInvRow   An array containing the corresponding row of the basis inverse.
	 */
	virtual int getInfeas(int &infeasCon, int &infeasVar, double *bInvRow) const override;

	/**
	 * \return true If the \a LP turned out to be
	 *              infeasible either if the base class \a LP detected an infeasibility
	 *              during the solution of the linear program
	 *              or infeasible constraints have been memorized during the
	 *              construction of the LP or during the addition of constraints,
	 * \return false otherwise.
	 */
	virtual bool infeasible() const override {
		return (LP::infeasible() || infeasCons_.size());
	}

	//! Returns a pointer to the buffer holding the infeasible constraints.
	ArrayBuffer<InfeasCon*> *infeasCon() { return &infeasCons_; }

	//! Loads a new basis for the linear program.
	/**
	 * The function redefines a virtual function of the base class \a LP.
	 * Eliminated variables have to be considered when the basis is loaded.
	 *
	 * \param lpVarStat An array storing the status of the columns.
	 * \param slackStat An array storing the status of the slack variables.
	 */
	virtual void loadBasis(Array<LPVARSTAT::STATUS> &lpVarStat,
		Array<SlackStat::STATUS> &slackStat) override;

protected:

	//! This function will pass the linear program of the associated subproblem to the solver.
	/**
	 * The function \a initialize() has to be called in the constructor
	 * of the class derived from this class and from a class implementing an LP-solver.
	 */
	virtual void initialize();

private:

	//! Performs the optimization of the linear program with method \a method.
	/**
	 * This function redefines a virtual function of the base class \a LP.
	 *
	 * We have to reimplement \a optimize() since there might be infeasible
	 * constraints.
	 * If a linear program turns out to be infeasible but has not been
	 * solved with the dual simplex method we solve it again to
	 * find a dual feasible basis which can be used to determine
	 * inactive variables restoring feasibility.
	 * Before the optimization can be performed the infeasible constraints
	 * must be removed with the function \a _initMakeFeas(), then the
	 * \a LP should be deleted and reconstructed. This is done by the function
	 * \a solveLp() in the cutting plane algorithm of the class Sub.
	 */
	virtual OPTSTAT optimize(METHOD method) override;

	//! Removes all constraints listed in the buffer \a ind from the linear program.
	virtual void removeCons(ArrayBuffer<int> &ind) {
		LP::remRows(ind);
	}

	//! Removes the variables with names given in \a vars from the linear program.
	virtual void removeVars(ArrayBuffer<int> &vars);

	//! Adds the constraints \a newCons to the linear program.
	virtual void addCons(ArrayBuffer<Constraint*> &newCons);

	/**
	 * \param vars      The new variables which are added to the linear program.
	 * \param fsVarStat The status of fixing/setting of the new variables.
	 * \param lb        The lower bounds of the new variables.
	 * \param ub        The upper bounds of the new variables.
	 */
	virtual void addVars(
		ArrayBuffer<Variable*> &vars,
		ArrayBuffer<FSVarStat*> &fsVarStat,
		ArrayBuffer<double> &lb,
		ArrayBuffer<double> &ub);

	//! Sets the lower bound of variable \a i to \a newLb.
	/**
	 * It is not allowed to change the lower bound of an
	 * eliminated variable. This will cause a run-time error.
	 */
	virtual void changeLBound(int i, double newLb) override;

	//! Sets the upper bound of variable \a i to \a newUb.
	/**
	 * It is not allowed to change the upper bound of
	 * an eliminated variable. This will cause a run-time error.
	 */
	virtual void changeUBound(int i, double newUb) override;

	//! Sets the maximal number of variables to \a newSize.
	virtual void varRealloc(int newSize);

	//! Sets the maximal number of constraints to \a newSize.
	virtual void conRealloc(int newSize);

	//! Generates the row format of the constraint \a cons and stores it in \a rows.
	void constraint2row(ArrayBuffer<Constraint*> &newCons,
		ArrayBuffer<Row*> &newRows);

	//! Returns \a true if the function can be eliminated.
	/**
	 * This function may be only applied to variables which are fixed or set!
	 * It is sufficient for turning off any variable elimination to return always
	 * \a false by this function.
	 */
	bool eliminable(int i) const;

	//! Returns \a true if the variable \a i is actually eliminated from the \a LP.
	/**
	 * This function can give different results than the function \a eliminate(i) since
	 * the condition to eliminate a variable might have become \a true
	 * after the \a LP has been set up.
	 */
	bool eliminated(int i) const {
		return (orig2lp_[i] == -1);
	}

	//! \brief Returns the value the variable \a i to which it is fixed or set to.
	/**
	 * The value of an eliminated variable is defined by the bound to which
	 * it is fixed or set.
	 * There is no reason to distinguish between \a sub_ and \a master_
	 * in the \a switch statement, since both values should be equal.
	 */
	virtual double elimVal(int i) const;

	//! \brief Returns the value a variable is fixed or set to.
	/**
	 * \param stat A pointer to the status of the variable.
	 * \param lb   The lower bound of the variable.
	 * \param ub   The upper bound of the variable.
	 */
	virtual double elimVal(FSVarStat *stat, double lb, double ub) const;

	void initialize(
		OptSense sense,
		int nRow,
		int maxRow,
		int nCol,
		int maxCol,
		Array<double> &obj,
		Array<double> &lBound,
		Array<double> &uBound,
		Array<Row*> &rows);

	void initialize(
		OptSense sense,
		int nRow,
		int maxRow,
		int nCol,
		int maxCol,
		Array<double> &obj,
		Array<double> &lBound,
		Array<double> &uBound,
		Array<Row*> &rows,
		Array<LPVARSTAT::STATUS> &lpVarStat,
		Array<SlackStat::STATUS> &slackStat);

	int    nCol()     const;
	int    maxCol()   const;
	int    nnz()      const;
	double obj(int i) const;

	void rowRealloc(int newSize);
	void colRealloc(int newSize);


	//! A pointer to the corresponding subproblem.
	const Sub *sub_;

	/**
	 * After the elimination of variables the internal variables are
	 * again numbered consecutively starting with 0. \a orig2lp_[i] is
	 * the internal number of the variable \a i. This is \a -1 if the variable
	 * is eliminated.
	 */
	Array<int> orig2lp_;

	//! Original number of a (non-eliminated) variable.
	Array<int> lp2orig_;

	//! Buffer storing the infeasible constraints found be the constructor.
	ArrayBuffer<InfeasCon*> infeasCons_;

	//! The constant which has been added to the objective function value due to the elimination of  variables.
	double valueAdd_;

	//! The number of original variables of the linear program.
	int nOrigVar_;

	LpSub(const LpSub &rhs);
	const LpSub &operator=(const LpSub &rhs);
};

}

#include <ogdf/lib/abacus/sub.h>

namespace abacus {

inline LpSub::LpSub (Master *master, const Sub *sub)
	:
	LP(master),
	sub_(sub),
	orig2lp_(sub->maxVar()),
	lp2orig_(sub->maxVar()),
	infeasCons_(sub->maxCon(),false)
{ }

}
