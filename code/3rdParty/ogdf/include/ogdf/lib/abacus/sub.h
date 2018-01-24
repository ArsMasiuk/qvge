/*!\file
 * \author Matthias Elf
 * brief the subproblem.
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
#include <ogdf/lib/abacus/fsvarstat.h>
#include <ogdf/lib/abacus/vartype.h>

#include <ogdf/basic/Stopwatch.h>


namespace abacus {

class LpSub;
class TailOff;
class BranchRule;
class LPVARSTAT;
class Variable;
class Master;
class InfeasCon;
class Constraint;

template<class BaseType, class CoType> class CutBuffer;
template<class BaseType, class CoType> class Active;
template<class BaseType, class CoType> class Pool;
template<class BaseType, class CoType> class PoolSlot;
template<class BaseType, class CoType> class LpSolution;


//! The subproblem.
/**
 * This class implements an abstract base class for a subproblem
 * of the enumeration, i.e., a node of the branch-and-bound tree.
 * The core of this class is the solution of the linear programming
 * relaxation. If a derived class provides methods for the generation
 * of cutting planes and/or variables, then the subproblem is
 * processed by a cutting plane and/or column generation algorithm.
 * Essential is that every subproblem has its own sets of active
 * constraints and variables, which provides a very high flexibility.
 */
class OGDF_EXPORT Sub : public AbacusRoot {

	friend class Master;
	friend class BoundBranchRule;
	friend class OpenSub;
	friend class LpSolution<Constraint, Variable>;
	friend class LpSolution<Variable, Constraint>;

public:

	//! A subproblem can have different statuses.
	enum STATUS {
		Unprocessed,	/*!< The status after generation, but before optimization of the subproblem. */
		ActiveSub,		/*!< The subproblem is currently processed. */
		Dormant,		/*!< The subproblem is partially processed and waiting in the set of open
						 *   subproblems for further optimization. */
		Processed,		/*!< The subproblem is completely processed but could not be fathomed. */
		Fathomed		/*!< The subproblem is fathomed. */
	};

	//! The optimization of the subproblem can be in one of the following phases.
	enum PHASE {
		Done,		/*!< The optimization is done. */
		Cutting,	/*!< The iterative solution of the LP-relaxation and the generation
					 *   of cutting planes and/or variables is currently performed. */
		Branching,	/*!< We try to generate further subproblems as sons of this subproblem. */
		Fathoming	/*!< The subproblem is currently being fathomed. */
	};

	//! Creates the root node of the enumeration tree.
	/**
	 * \param master  A pointer to the corresponding master of the optimization.
	 * \param conRes  The additional memory allocated for constraints.
	 * \param varRes  The additional memory allocated for variables.
	 * \param nnzRes  The additional memory allocated for nonzero elements of the constraint matrix.
	 * \param relativeRes If this argument is \a true, then reserve space for variables,
	 *                    constraints, and nonzeros given by the previous three arguments,
	 *                    is given in percent of the original numbers. Otherwise, the numbers
	 *                    are interpreted as absolute values (casted to integer). The
	 *                    default value is \a true.
	 * \param constraints The pool slots of the initial constraints. If the value is 0,
	 *                    then the constraints of the default constraint pool are taken.
	 *                    The default value is 0.
	 * \param variables   The pool slots of the initial variables. If the value is 0, then
	 *                    the variables of the default variable pool are taken.
	 *                    The default value is 0.
	 */
	Sub(
		Master *master,
		double conRes,
		double varRes,
		double nnzRes,
		bool relativeRes = true,
		ArrayBuffer<PoolSlot<Constraint, Variable> *> *constraints = nullptr,
		ArrayBuffer<PoolSlot<Variable, Constraint> *> *variables = nullptr);

	//! Creates a non-root node of the enumeration tree.
	/**
	 * \param master     A pointer to the corresponding master of the optimization.
	 * \param father     A pointer to the father in the enumeration tree.
	 * \param branchRule The rule defining the subspace of the
	 *                   solution space associated with this subproblem.
	 */
	Sub(Master *master, Sub *father, BranchRule *branchRule);

	//! The destructor only deletes the sons of the node.
	/**
	 * The deletion of allocated memory is already performed when the node
	 * is fathomed.
	 * We recursively call the destructors of all subproblems contained
	 * in the enumeration tree below this subproblem itself.
	 *
	 * If a subproblem has no sons and its status is either \a Unprocessed
	 * or \a Dormant, then it is still contained in the set of open
	 * subproblems, where it is removed from.
	 */
	virtual ~Sub();

	//! Returns whether using the exact solver is forced.
	bool forceExactSolver() const { return forceExactSolver_; }

	//! Returns the level of the subproblem in the branch-and-bound tree.
	int level() const { return level_; }

	//! Returns the identity number of the subproblem.
	int id() const { return id_; }

	//! Returns the status of the subproblem optimization.
	STATUS status() const { return status_; }

	//! Returns the number of active variables.
	int nVar() const;

	//! Returns the maximum number of variables which can be handled without reallocation.
	int maxVar() const;

	//! Returns the number of active constraints.
	int nCon() const;

	//! Returns the maximum number of constraints which can be handled without reallocation.
	int maxCon() const;

	//! Returns a lower bound on the optimal solution of the subproblem.
	double lowerBound() const;

	//! Returns an upper bound on the optimal solution of the subproblem.
	double upperBound() const;

	//! Returns a bound which is "better" than the optimal solution of the subproblem w.r.t. the sense of the optimization.
	/**
	 * I.e., it returns an upper for a maximization problem or
	 * a lower bound for a minimization problem, respectively.
	 */
	double dualBound() const { return dualBound_; }

	//! Sets the dual bound of the subproblem.
	/**
	 * If the subproblem is the root node of the enumeration tree and the new
	 * value is better than its dual bound, also the global dual bound is updated.
	 * It is an error if the dual bound gets worse.
	 *
	 * In normal applications it is not required to call this function
	 * explicitly. This is already done by ABACUS during the subproblem
	 * optimization.
	 *
	 * \param x The new value of the dual bound.
	 */
	void dualBound(double x);

	//! Returns a pointer to the father of the subproblem in the branch-and-bound tree.
	const Sub *father() const { return father_; }

	//! Returns a pointer to the linear program of the subproblem.
	LpSub *lp() const { return lp_; }

	//! Sets the maximal number of iterations in the cutting plane phase.
	/**
	 * Setting this value to 1 implies that no
	 * cuts are generated in the optimization process of the subproblem.
	 *
	 * \param max The maximal number of iterations.
	 */
	void maxIterations(int max);

	//! Returns a pointer to the currently active constraints.
	Active<Constraint, Variable> *actCon() const { return actCon_; }

	//! Returns a pointer to the currently active variables.
	Active<Variable, Constraint> *actVar() const { return actVar_; }

	//! Returns a pointer to the <i>i</i>-th active constraint.
	/**
	 * \param i The constraint being accessed.
	 */
	Constraint *constraint(int i) const;

	//! Returns a pointer to the status of the slack variable \a i in the last solved linear program.
	/**
	 * \param i The number of the slack variable.
	 */
	SlackStat *slackStat(int i) const { return (*slackStat_)[i]; }

	//! Returns a pointer to the \a i-th active variable.
	/**
	 * \param i The number of the variable being accessed.
	 */
	Variable *variable(int i) const;

	//! Can be used to access the lower of an active variable of the subproblem.
	/**
	 * \warning This is the lower bound of the variable within
	 * the current subproblem which can differ from its global lower bound.
	 *
	 * \param i The number of the variable.
	 *
	 * \return The lower bound of the \a i-th variable.
	 */
	double lBound(int i) const { return (*lBound_)[i]; }

	//! Sets the local lower bound of variable \a i to \a l.
	/**
	 * It does not change the global lower bound of the
	 * variable. The bound of a fixed or set variable should not be changed.
	 *
	 * \param i The number of the variable.
	 * \param l The new value of the lower bound.
	 */
	void lBound(int i, double l);

	//! Can be used to access the upper of an active variable of the subproblem.
	/**
	 * \warning This is the upper bound of the variable within
	 * the current subproblem which can differ from its global upper bound.
	 *
	 * \param i The number of the variable.
	 *
	 * \return The upper bound of the \a i-th variable.
	 */
	double uBound(int i) const { return (*uBound_)[i]; }

	//! Sets the local upper bound of variable \a i to \a u.
	/**
	 * This does not change the global lower bound of the
	 * variable. The bound of a fixed or set variable should not be changed.
	 *
	 * \param i The number of the variable.
	 * \param u The new value of the upper bound.
	 */
	void uBound(int i, double u);

	//! Returns a pointer to the status of fixing/setting of the <i>i</i>-th variable.
	/**
	 * In a branch-and-cut-and-price algorithm we also would have to refer to the global variable
	 * status. While this subproblem is processed another subproblem could change
	 * the global status.
	 *
	 * \note This is the local status of fixing/setting that might differ
	 * from the global status of fixing/setting of the variable (\a variable(i)->fsVarStat()).
	 *
	 * \param i The number of the variable.
	 *
	 * \return A pointer to the status of fixing/setting of the \a i-th variable.
	 */
	FSVarStat *fsVarStat(int i) const { return (*fsVarStat_)[i]; }

	//! Returns a pointer to the status of the variable \a i in the last solved linear program.
	/**
	 * \param i The number of the variable.
	 */
	LPVARSTAT *lpVarStat(int i) const { return (*lpVarStat_)[i]; }

	//! Returns the value of the \a i-th variable in the last solved linear program.
	/**
	 * \param i The number of the variable under consideration.
	 */
	double xVal(int i) const { return xVal_[i]; }

	//! Returns the value of the \a i-th dual variable in the last solved linear program.
	/**
	 * \param i The number of the variable under consideration.
	 */
	double yVal(int i) const { return yVal_[i]; }

	//! Returns true if this subproblem is an ancestor of the subproblem \a sub, false otherwise.
	/**
	 * We define that a subproblem is also an ancestor of its own.
	 *
	 * \param sub A pointer to a subproblem.
	 */
	bool ancestor(const Sub *sub) const;

	//! Returns the master of the optimization.
	Master *master() { return master_; }

	//! Returns the const master of the optimization.
	const Master *master() const { return master_; }

	//! Removes the variables in \a remove from the set of active variables.
	/**
	 * The variables are not removed when this function
	 * is called, but are buffered and removed at the beginning of the next
	 * iteration.
	 *
	 * \param remove The variables which should be removed.
	 */
	void removeVars(ArrayBuffer<int> &remove);

	//! Remove variable \a i from the set of active variables.
	/**
	 * Like in the function \a removeVars()
	 * the variable is buffered and removed at the beginning of the next
	 * iteration.
	 *
	 * \param i The variable which should be removed.
	 */
	void removeVar(int i) { removeVarBuffer_->push(i); }

	//! Returns the additional space for nonzero elements of the constraint matrix when it is passed to the LP-solver.
	double nnzReserve() const { return nnzReserve_; }

	/**
	 * \return true If the reserve space for variables, constraints, and nonzeros
	 *         is given in percent of the original space, and \a false if its given as absolute value.
	 */
	bool relativeReserve() const { return relativeReserve_; }

	//! Returns a pointer to the branching rule of the subproblem.
	BranchRule *branchRule() const { return branchRule_; }

	//! Tests if all active variables and objective function coefficients are integer.
	/**
	 * If all variables are \a Binary or \a Integer and all objective function
	 * coefficients are integral, then all objective function values of
	 * feasible solutions are integral. The function \a objAllInteger() tests
	 * this condition for the current set of active variables.
	 *
	 * \note The result of this function can only be used to set the
	 * global parameter if \a actVar contains all variables of the problem
	 * formulation.
	 *
	 * \return true If this condition is satisfied by the currently active
	 *              variable set, false otherwise.
	 */
	bool objAllInteger() const;

	//! Adds constraints to the buffer of the removed constraints.
	/**
	 * These will be removed at the beginning of the next iteration of the cutting plane algorithm.
	 *
	 * \param remove The constraints which should be removed.
	 */
	virtual void removeCons(ArrayBuffer<int> &remove);

	//! Adds a single constraint to the set of constraints which are removed from the active set at the beginning of the next iteration.
	/**
	 * \param i The number of the constraint being removed.
	 */
	virtual void removeCon(int i);

	//! Can be used to determine the maximal number of the constraints which still can be added to the constraint buffer.
	/**
	 * A separation algorithm should stop as soon as the number of
	 * generated constraints reaches this number because further work is
	 * useless.
	 *
	 * \return The number of constraints which still can be inserted into the constraint buffer.
	 */
	int addConBufferSpace() const;

	//! Can be used to determine the maximal number of the variables which still can be added to the variable buffer.
	/**
	 * A pricing algorithm should stop as soon as the number of
	 * generated variables reaches this number because further work is
	 * useless.
	 *
	 * \return The number of variables which still can be inserted into the variable buffer.
	 */
	int addVarBufferSpace() const;

	//! \return The number of subproblem optimization the subproblem is already dormant.
	int nDormantRounds() const { return nDormantRounds_; }

	//! \brief Can be used to control better the tailing-off effect.
	/**
	 * If this function is called, the next
	 * LP-solution is ignored in the tailing-off control. Calling
	 * \a ignoreInTailingOff() can e.g.\ be considered in the following
	 * situation: If only constraints that are required for the integer
	 * programming formulation of the optimization problem are added then
	 * the next LP-value could be ignored in the tailing-off control. Only
	 * "real" cutting planes should be considered in the tailing-off
	 * control (this is only an example strategy that might not be
	 * practical in many situations, but sometimes turned out to be
	 * efficient).
	 */
	void ignoreInTailingOff();

	//! Adds a branching constraint to the constraint buffer.
	/**
	 * This constraint is automatically added at the beginning of the cutting plane algorithm.
	 * It should be used in definitions of the pure virtual function
	 * \a BRANCHRULE::extract().
	 *
	 * \return 0 If the constraint could be added, 1 otherwise.
	 *
	 * \param slot A pointer to the pool slot containing the branching constraint.
	 */
	virtual int addBranchingConstraint(PoolSlot<Constraint, Variable> *slot);

protected:

	//! Tries to add new constraints to the constraint buffer and a pool.
	/**
	 * The memory management of added constraints is passed to ABACUS by calling this function.
	 *
	 * \param constraints The new constraints.
	 * \param pool        The pool in which the new constraints are inserted.
	 *                    If the value of this argument is 0, then the
	 *                    cut pool of the master is selected. Its default value is 0.
	 * \param keepInPool  If \a (*keepInPool)[i] is \a true, then the constraint
	 *                    stays in the pool even if it is not activated.
	 *                    The default value is a 0-pointer.
	 * \param rank        If this pointer to a buffer is nonzero, this buffer
	 *                    should store a rank for each constraint.
	 *                    The greater the rank, the better the variable.
	 *                    The default value of \a rank is 0.
	 *
	 * \return The number of added constraints.
	 */
	virtual int addCons(ArrayBuffer<Constraint*> &constraints,
		Pool<Constraint, Variable> *pool = nullptr,
		ArrayBuffer<bool> *keepInPool = nullptr,
		ArrayBuffer<double> *rank = nullptr);

	//! Adds constraints to the active constraints and the linear program.
	/**
	 * \param newCons A buffer storing the pool slots of the new constraints.
	 *
	 * \return The number of added constraints.
	 */
	virtual int addCons(
		ArrayBuffer<PoolSlot<Constraint, Variable>*> &newCons);

	//! Tries to add new variables to the variable buffer and a pool.
	/**
	 * The memory management of added variables is passed to ABACUS by calling this function.
	 *
	 * \param variables  The new variables.
	 * \param pool       The pool in which the new variables are inserted.
	 *                   If the value of this argument is 0, then the default
	 *                   variable pool is taken. The default value is 0.
	 * \param keepInPool If \a (*keepInPool)[i] is \a true, then the variable
	 *                   stays in the pool even if it is not activated.
	 *                   The default value is a 0-pointer.
	 * \param rank       If this pointer to a buffer is nonzero, this buffer
	 *                   should store a rank for each variable.
	 *                   The greater the rank, the better the variable.
	 *                   The default value of \a rank is 0.
	 *
	 * \return The number of added variables.
	 */
	virtual int addVars(ArrayBuffer<Variable*> &variables,
		Pool<Variable, Constraint> *pool = nullptr,
		ArrayBuffer<bool> *keepInPool = nullptr,
		ArrayBuffer<double> *rank = nullptr);

	//! Adds both the variables in \a newVars to the set of active variables and to the linear program of the subproblem.
	/**
	 * If the new number of variables exceeds the maximal number of  variables
	 * an automatic reallocation is performed.
	 *
	 * We require this feature in derived classes if variables of \a newVars can be discarded
	 * if they are already active.
	 *
	 * \param newVars A buffer storing the pool slots of the new variables.
	 *
	 * \return The number of added variables.
	 */
	virtual int addVars(
		ArrayBuffer<PoolSlot<Variable, Constraint>*> &newVars);

	//! Tries to generate inactive variables from a pool.
	/**
	 * \param ranking This parameter indicates how the ranks of geneated variables
	 *                should be computed (0: no ranking; 1: violation is rank,
	 *                2: absolute value of violation is rank 3: rank determined by
	 *                ConVar::rank()). The default value is 0.
	 * \param pool    The pool the variables are generated from.
	 *                If \a pool is 0, then the default variable pool
	 *                is used. The default value of \a pool is 0.
	 * \param minViolation A violated constraint/variable is only
	 *                     added if the absolute value of its violation is at least
	 *                     \a minAbsViolation. The default value is 0.001.
	 *
	 * \return The number of generated variables.
	 */
	virtual int variablePoolSeparation(
		int ranking = 0,
		Pool<Variable, Constraint> *pool = nullptr,
		double minViolation = 0.001);

	//! Tries to generate inactive constraints from a pool.
	/**
	 * \param ranking This parameter indicates how the ranks of
	 *                violated constraints should be computed
	 *                (0: no ranking; 1: violation is rank,
	 *                2: absolute value of violation is rank,
	 *                3: rank determined by ConVar::rank()).
	 *                The default value is 0.
	 * \param pool    The pool the constraints are generated from.
	 *                If \a pool is 0, then the default constraint pool
	 *                is used. The default value of \a pool is 0.
	 * \param minViolation A violated constraint/variable is only
	 *                     added if the absolute value of its violation is at least
	 *                     \a minAbsViolation. The default value is \a 0.001.
	 *
	 * \return The number of generated constraints.
	 */
	virtual int constraintPoolSeparation(
		int ranking = 0,
		Pool<Constraint, Variable> *pool = nullptr,
		double minViolation = 0.001);

	//! Can be used as an entrance point for problem specific activations.
	/**
	 * The default implementation does nothing.
	 */
	virtual void activate() { }

	//! Can be used as entrance point for problem specific deactivations after the subproblem optimization.
	/**
	 * The default version of this function does nothing. This function is
	 * only called if the function \a activate() for the subproblem has been
	 * executed. This function is called from \a _deactivate().
	 */
	virtual void deactivate () { }

	//! Tries to find rules for splitting the current subproblem in further subproblems.
	/**
	 * Per default we generate rules for branching on variables (\a branchingOnVariable()).
	 * But by redefining this function in a derived class
	 * any other branching strategy can be implemented.
	 *
	 * \return 0 If branching rules could be found, 1 otherwise.
	 *
	 * \param rules If branching rules are found, then they are stored in this buffer.
	 */
	virtual int generateBranchRules(ArrayBuffer<BranchRule*> &rules) {
		return branchingOnVariable(rules);
	}

	//! Generates branching rules for two new subproblems by selecting a branching variable with the function \a selectBranchingVariable().
	/**
	 * If a new branching variable selection strategy should be used the function
	 * \a selectBranchingVariable() should be redefined.
	 *
	 * \param rules If branching rules are found, then they are stored in this buffer.
	 *              The length of this buffer is the number of active variables of the subproblem.
	 *              If more branching rules are generated a reallocation has to be performed.
	 *
	 * \return 0 If branching rules could be found, 1 otherwise
	 */
	virtual int branchingOnVariable(ArrayBuffer<BranchRule*> &rules);

	//! Chooses a branching variable.
	/**
	 * The function \a selectBranchingVariableCandidates() is asked to generate
	 * depending in the parameter <tt>NBranchingVariableCandidates</tt> of the
	 * file <tt>.abacus</tt> candidates for branching variables. If only
	 * one candidate is generated, this one becomes the branching variable.
	 * Otherwise, the pairs of branching rules are generated for all
	 * candidates and the "best" branching variables is determined with the
	 * function \a selectBestBranchingSample().
	 *
	 * \param variable Holds the branching variable if one is found.
	 *
	 * \return 0 If a branching variable is found, 1 otherwise.
	 */
	virtual int selectBranchingVariable(int &variable);

	//! Selects candidates for branching variables.
	/**
	 * Candidates are selected depending on the branching variable strategy given by the parameter
	 * <tt>BranchingStrategy</tt> in the file <tt>.abacus</tt> candidates that
	 * for branching variables.
	 *
	 * Currently two branching variable selection strategies are
	 * implemented. The first one (\a CloseHalf) first searches the binary
	 * variables with fractional part closest to \f$0.5\f$. If there is
	 * no fractional binary variable it repeats this process with the
	 * integer variables.
	 *
	 * The second strategy (\a CloseHalfExpensive) first tries to find binary
	 * variables with fraction close to \f$0.5\f$ and high absolute objective function
	 * coefficient. If this fails, it tries to find an integer variable with
	 * fractional part close to \f$0.5\f$ and high absolute objective function
	 * coefficient.
	 *
	 * If neither a binary nor an integer variable with fractional value
	 * is found then for both strategies we try to find non-fixed
	 * and non-set binary variables. If this fails we repeat this process with
	 * the integer variables.
	 *
	 * Other branching variable selection strategies can be implemented by
	 * redefining this virtual function in a derived class.
	 *
	 * \param candidates The candidates for branching variables are stored in this buffer.
	 *                   We try to find as many variables as fit into the buffer.
	 *
	 * \return 0 If a candidate is found, 1 otherwise.
	 */
	virtual int selectBranchingVariableCandidates(ArrayBuffer<int> &candidates);

	//! Evaluates branching samples.
	/**
	 * We denote a branching sample the set of rules defining
	 * all sons of a subproblem in the enumeration tree). For each sample
	 * the ranks are determined with the function \a rankBranchingSample().
	 * The ranks of the various samples are compared with the function
	 * \a compareBranchingSample().
	 *
	 * \param nSamples The number of branching samples.
	 * \param samples An array of pointer to buffers storing the branching rules of each sample.
	 *
	 * \return The number of the best branching sample, or \a -1 in case of an internal error.
	 */
	virtual int selectBestBranchingSample(int nSamples,
		ArrayBuffer<BranchRule*> **samples);

	//! Computes for each branching rule of a branching sample a rank with the function \a rankBranchingRule().
	/**
	 * \param sample A branching sample.
	 * \param rank   An array storing the rank for each branching rule in the
	 *               sample after the function call.
	 */
	virtual void rankBranchingSample(ArrayBuffer<BranchRule*> &sample,
		Array<double> &rank);

	//! Computes the rank of a branching rule.
	/**
	 * This default implementation computes the rank
	 * with the function \a lpRankBranchingRule(). By redefining this virtual
	 * function the rank for a branching rule can be computed differently.
	 *
	 * \param branchRule A pointer to a branching rule.
	 *
	 * \return The rank of the branching rule.
	 */
	virtual double rankBranchingRule(BranchRule *branchRule);

	//! Computes the rank of a branching rule by modifying the linear programming relaxation of the subproblem according to the branching rule and solving it.
	/**
	 * This modifiction is undone after the solution of the linear program.
	 *
	 * It is useless, but no error, to call this function for branching rules
	 * for which the virtual dummy functions \a extract(LpSub*)  and
	 * \a unExtract(LpSub*) of the base class BranchRule are not redefined.
	 *
	 * \param branchRule A pointer to a branching rule.
	 * \param iterLimit  The maximal number of iterations that should be performed by the simplex method.
	 *                   If this number is negative there is no  iteration limit
	 *                   (besides internal limits of the LP-solver). The default value is \a -1.
	 *
	 * \return The value of the linear programming relaxation of the subproblem modified by the branching rule.
	 */
	double lpRankBranchingRule(BranchRule *branchRule, int iterLimit = -1);

	//! Compares the ranks of two branching samples.
	/**
	 * For maximimization problem that rank
	 * is better for which the maximal rank of a rule is minimal, while for
	 * minimization problem the rank is better for which the minimal rank of
	 * a rule is maximal. If this value equals for both ranks we continue
	 * with the secand greatest value, etc.
	 *
	 * \return 1  If \a rank1 is better.
	 * \return 0  If both ranks are equal.
	 * \return -1 If \a rank2 is better.
	 */
	virtual int compareBranchingSampleRanks(Array<double> &rank1,
		Array<double> &rank2);

	//! Selects a single branching variable of type \a branchVarType, with fractional part close to \f$0.5\f$ and high absolute objective function coefficient.
	/**
	 * This is the default strategy from the TSP project.
	 *
	 * \param branchVar     Holds the number of the branching variable if one is found.
	 * \param branchVarType The type of the branching variable can be
	 *                      restricted either to VarType::Binary or VarType::Integer.
	 *
	 * \return 0 If a branching variable is found, 1 otherwise.
	 */
	int closeHalfExpensive(int &branchVar, VarType::TYPE branchVarType);

	//! Selects several candidates for branching variables of type \a branchVarType.
	/***
	 * Those variables with fractional part close to \f$0.5\f$ and high absolute objective function
	 * coefficient are selected..
	 *
	 * \param variables     Holds the numbers of possible branching variables if
	 *                      at least one is found. We try to find as many
	 *                      candidates as fit into this buffer. We abort
	 *                      the function with a fatal error if the size
	 *                      of the buffer is 0.
	 * \param branchVarType The type of the branching variable can be restricted either
	 *                      to VarType::Binary or VarType::Integer.
	 *
	 * \return 0 If at least one branching variable is found, 1 otherwise.
	 */
	int closeHalfExpensive(ArrayBuffer<int> &variables,
		VarType::TYPE branchVarType);

	//! Searches a branching variable of type \a branchVarType, with fraction as close to \f$0.5\f$ as possible.
	/**
	 * \param branchVar     Holds the branching variable if one is found.
	 * \param branchVarType The type of the branching variable can be
	 *                      restricted either to VarType::Binary or VarType::Integer.
	 *
	 * \return 0 If a branching variable is found, 1 otherwise.
	 */
	int closeHalf(int &branchVar, VarType::TYPE branchVarType);

	//! Searches searches several possible branching variable of type \a branchVarType, with fraction as close to \f$0.5\f$ as possible.
	/**
	 * \param branchVar     Stores the possible branching variables.
	 * \param branchVarType The type of the branching variable can berestricted either
	 *                      to VarType::Binary or VarType::Integer.
	 *
	 * \return 0 If at least one branching variable is found, 1 otherwise.
	 */
	int closeHalf(ArrayBuffer<int> &branchVar, VarType::TYPE branchVarType);

	//! Selects the first variables that are neither fixed nor set.
	/**
	 * \param branchVar     Holds the number of the possible branching variables if one is found.
	 * \param branchVarType The type of the branching variable can be
	 *                      restricted either to VarType::Binary or VarType::Integer.
	 *
	 * \return 0 If at least one variable neither fixed nor set is found, 1 otherwise.
	 */
	int findNonFixedSet(ArrayBuffer<int> &branchVar,
		VarType::TYPE branchVarType);

	//! Selects the first variable that is neither fixed nor set.
	/**
	 * \return 0 If a variable neither fixed nor set is found, 1 otherwise.
	 *
	 * \param branchVar     Holds the number of the branching variable if one is found.
	 * \param branchVarType The type of the branching have (VarType::Binary or
	 *                      VarType::Integer).
	 */
	int findNonFixedSet(int &branchVar, VarType::TYPE branchVarType);

	//! The default implementation of the virtual \a initMakeFeas() does nothing.
	/**
	 * A reimplementation of this function should generate inactive
	 * variables until at least one variable \a v which satisfies the function
	 * InfeasCon::goodVar(v) for each infeasible constraint
	 * is found.
	 *
	 * \param infeasCon The infeasible constraints.
	 * \param newVars   The variables that might restore feasibility should be
	 *                  added here.
	 * \param pool      A pointer to the pool to which the new variables should
	 *                  be added. If this is a 0-pointer the variables are added
	 *                  to the default variable pool. The default value is 0.
	 *
	 * \return 0 if the feasibility might have been restored, 1 otherwise.
	 */
	virtual int initMakeFeas(
		ArrayBuffer<InfeasCon*> &infeasCon,
		ArrayBuffer<Variable*> &newVars,
		Pool<Variable, Constraint> **pool)
	{
		return 1;
	}

	//! \brief The default implementation of \a makeFeasible()does nothing.
	/**
	 * If there is an infeasible structural variable then it is stored
	 * in \a infeasVar_, otherwise \a infeasVar_ is \a -1. If there is
	 * an infeasible slack variable, it is stored in \a infeasCon_, otherwise
	 * it is \a -1. At most one of the two members \a infeasVar_ and \a infeasCon_
	 * can be nonnegative.
	 * A reimplementation in a derived class should  generate variables
	 * to restore feasibility or confirm that the subproblem is infeasible.
	 *
	 * The strategy for the generation of inactive variables is completely
	 * problem and user specific.
	 * For testing if a variable might restore again the feasibility the functions
	 * Variable::useful() and Sub::goodCol() might be helpful.
	 *
	 * \return 0 If feasibility can be restored, 1 otherwise.
	 */
	virtual int makeFeasible() { return 1; }

	/**
	 * \param col The column of the variable.
	 * \param row The row of the basis inverse associated with the infeasible variable.
	 * \param x   The LP-value of the infeasible variable.
	 * \param lb  The lower bound of the infeasible variable.
	 * \param ub  The upper bound of the infeasible variable.
	 *
	 * \return true If the column \a col might restore feasibiblity if
	 *              the variable with value \a x turns out to be infeasible,
	 *              false otherwise.
	 */
	virtual bool goodCol(Column &col, Array<double> &row,
		double x, double lb, double ub);

	//! The default implementation of \a setByLogImp() does nothing.
	/**
	 * In derived classes this function can be reimplemented.
	 *
	 * \param variable The variables which should be set have to be inserted in this buffer.
	 * \param status   The status of the set variables.
	 */
	virtual void setByLogImp(ArrayBuffer<int> &variable,
		ArrayBuffer<FSVarStat*> &status) { }

	//! Must check the feasibility of a solution of the LP-relaxation.
	/**
	 * If the function returns \a true and
	 * the value of the primal bound is worse than the value of this feasible
	 * solution, the value of the primal bound is updated automatically.
	 *
	 * \return true If the LP-solution is feasible, false otherwise.
	 */
	virtual bool feasible() = 0;

	//! Can be used to check if the solution of the LP-relaxation is primally feasible if integrality is suficinet.
	/**
	 * Checks if all binary and integer variables have an integral value.
	 * This function can be called from the function \a feasible() in derived
	 * classes.
	 *
	 * \return true If the LP-value of all binary and integer variables is integral, false otherwise.
	 */
	bool integerFeasible();

	//! Controls if during the cutting plane phase a (primal) separation step or a pricing step (dual separation) should be performed.
	/**
	 * Per default a pure cutting plane algorithm performs always a primal
	 * separation step, a pure column generation algorithm never performs
	 * a primal separation, and a hybrid algorithm generates usually cutting
	 * planes but from time to time also inactive variables are priced
	 * out depending on the \a pricingFrequency().
	 *
	 * \return true  Then cutting planes are generated in this iteration.
	 * \return false Then columns are generated in this iteration.
	 */
	virtual bool primalSeparation();

	//! Must be redefined in derived classes for the generation of cutting planes.
	/**
	 * The default implementation does nothing.
	 *
	 * \return The number of generated cutting planes.
	 */
	virtual int separate();

	//! Can be used as an entry point for application specific elimination of constraints.
	/**
	 * The default implementation of this function calls either the function
	 * \a nonBindingConEliminate() or the function \a basicConEliminate() depending
	 * on the constraint elimination mode of the master that is initialized
	 * via the parameter file.
	 *
	 * \param remove The constraints that should be eliminated must be
	 *               inserted in this buffer.
	 */
	virtual void conEliminate(ArrayBuffer<int> &remove);

	//! Retrieves the dynamic constraints with slack exceeding the value given by the parameter <tt>ConElimEps</tt>.
	/**
	 * \param remove Stores the nonbinding constraints.
	 */
	virtual void nonBindingConEliminate(ArrayBuffer<int> &remove);

	//! Retrieves all dynamic constraints having basic slack variable.
	/**
	 * \param remove Stores the nonbinding constraints.
	 */
	virtual void basicConEliminate(ArrayBuffer<int> &remove);

	//! Entry point for application specific variable elimination.
	/**
	 * The default implementation selects the variables
	 * with the function \a redCostVarEliminate().
	 *
	 * \param remove The variables that should be removed have to be stored in this buffer.
	 */
	virtual void varEliminate(ArrayBuffer<int> &remove);

	//! Retrieves all variables with "wrong" reduced costs.
	/**
	 * \param remove The variables with "wrong" reduced cost are stored in this buffer.
	 */
	void redCostVarEliminate(ArrayBuffer<int> &remove);

	//! Should generate inactive variables which do not price out correctly.
	/**
	 * The default implementation does nothing and returns 0.
	 *
	 * \return The number of new variables.
	 */
	virtual int pricing() { return 0; }

	//! Can be redefined in order to implement primal heuristics for finding feasible solutions.
	/**
	 * The default implementation does nothing.
	 *
	 * \return 0 If no better solution could be found, 1 otherwise.
	 *
	 * \param primalValue Should hold the value of the feasible solution,
	 *                    if a better one is found.
	 */
	virtual int improve(double &primalValue);

	//! Returns a pointer to an object of a problem specific subproblem, which is generated from the current subproblem by branching rule \a rule.
	/**
	 * \param rule The branching rule with which the subproblem is generated.
	 */
	virtual Sub *generateSon(BranchRule *rule) = 0;

	//! Returns true if the dual bound is worse than the best known primal bound, false otherwise.
	bool boundCrash()    const;

	/**
	 * Sometimes it is appropriate to put a subproblem back into the list
	 * of open subproblems. This is called \a pausing.
	 * In the default implementation the virtual function \a pausing()
	 * always returns \a false.
	 *
	 * It could be useful to enforce pausing a node if a tailing off
	 * effect is observed during its first optimization.
	 *
	 * \return true The function \a pausing() should return \a true if this
	 *              condition is satisfied, false otherwise.
	 */
	virtual bool pausing() { return false; }

	//! Returns true if the subproblem does not contain a feasible solution, false otherwise.
	bool infeasible();

	//! Reallocates memory that at most \a newSize variables can be handled in the subproblem.
	/**
	 * \param newSize The new maximal number of variables in the subproblem.
	 */
	virtual void varRealloc(int newSize);

	//! Reallocates memory that at most \a newSize constraints can be handled in the subproblem.
	/**
	 * \param newSize The new maximal number of constraints of the subproblem.
	 */
	virtual void conRealloc(int newSize);

	//! \brief Controls the method used to solve a linear programming relaxation.
	/**
	 * The default implementation
	 * chooses the barrier method for the first linear program of the root
	 * node and for all other linear programs it tries to choose a method
	 * such that phase 1 of the simplex method is not required.
	 *
	 * \param nVarRemoved The number of removed variables.
	 * \param nConRemoved The number of removed constraints.
	 * \param nVarAdded   The number of added variables.
	 * \param nConAdded   The number of added constraint.
	 *
	 * \return The method the next linear programming relaxation is solved with.
	 */
	virtual LP::METHOD chooseLpMethod(int nVarRemoved, int nConRemoved,
		int nVarAdded, int nConAdded);

	//! Called when a tailing off effect according to the parameters <tt>TailOffPercent</tt> and <tt>TailOffNLps</tt> is observed.
	/**
	 * This function can be redefined in derived classes in order to
	 * perform actions to resolve the tailing off (e.g., switching on
	 * an enhanced separation strategy).
	 *
	 * \return true  If a branching step should be enforced. But before
	 *               branching a pricing operation is perfored. The
	 *               branching step is only performed if no variables are
	 *               added. Otherwise, the cutting plane algorithm is continued.
	 * \return false If the cutting plane algorithm should be continued.
	 */
	virtual bool tailingOff() { return true; }

	//! Returns true if \a x is better than the best known dual bound of the subproblem, false otherwise.
	bool betterDual(double x) const;

	//! Is called before variables are selected from the variable buffer.
	/**
	 * It can be redefined in a derived class e.g., to remove multiply inserted
	 * variables from the buffer.
	 */
	virtual void selectVars() { }

	//! Is called before constraint are selected from the constraint buffer.
	/**
	 * It can be redefined in a derived class e.g., to remove multiply inserted
	 * constraints from the buffer.
	 */
	virtual void selectCons() { }

	//! Tries to fix variables according to the reduced cost criterion.
	/**
	 * \param newValues If variables are fixed to different values as
	 *                  in the last solved linear program, then
	 *                  \a newValues becomes true.
	 * \param saveCand  If \a saveCand is \a true, then a new list of
	 *                  candidates for later calls is compiled. This is
	 *                  only possible when the root of the remaining
	 *                  branch-and-bound is processed.
	 *
	 * \return 1 If a contradiction is found, 0 otherwise.
	 */
	virtual int fixByRedCost(bool &newValues, bool saveCand);

	//! Should collect the numbers of the variables to be fixed in \a variable and the respective statuses in \a status.
	/***
	 * The default implementation of \a fixByLogImp() does nothing. This
	 * function has to be redefined if variables should be fixed by logical
	 * implications in derived classes.
	 *
	 * \param variables The variables which should be fixed.
	 * \param status    The statuses these variables should be fixed to.
	 */
	virtual void fixByLogImp(ArrayBuffer<int> &variables,
		ArrayBuffer<FSVarStat*> &status) { }

	//! Tries to fix and set variables both by logical implications and reduced cost criteria.
	/**
	 * Actually, variables fixed or set to 0 could be eliminated. However,
	 * this could lead to a loss of important structural information
	 * for fixing and setting further variables later, for the computation
	 * of feasible solutions, for the separation and for detecting contradictions.
	 * Therefore, we do not eliminate these variables per default.
	 *
	 * \return 1 If a contradiction is found, 0 otherwise.
	 *
	 * \param newValues If a variables is set or fixed to a value different
	 *                  from the last LP-solution, \a newValues is set
	 *                  to \a true, otherwise it is set to \a false.
	 */
	virtual int fixAndSet(bool &newValues);

	//! Tries to fix variables by reduced cost criteria and logical implications.
	/**
	 * \param newValues The parameter \a newValues becomes \a true if variables are fixed
	 *                  to other values as in the current LP-solution.
	 * \param saveCand  If the parameter \a saveCand is \a true a new candidate list of variables
	 *                  for fixing is generated.
	 *                  The default value of \a saveCand is false. Candidates should not be
	 *                  saved if fixing is performed after the addition of variables.
	 *
	 * \return 1 If a contradiction is found, 0 otherwise.
	 */
	virtual int fixing(bool &newValues, bool saveCand = false);

	//! Tries to set variables by reduced cost criteria and logical implications like \a fixing().
	/**
	 * But instead of global conditions only locally valid conditions have to be satisfied.
	 *
	 * \param newValues The parameter \a newValues becomes \a true if variables are fixed
	 *                  to other values as in the current LP-solution (\a setByRedCost()
	 *                  cannot set variables to new values).
	 *
	 * \return 1 If a contradiction has been found, 0 otherwise.
	 */
	virtual int setting(bool &newValues);

	//! Tries to set variables according to the reduced cost criterion.
	/**
	 * \return 1 If a contradiction is found, 0 otherwise.
	 */
	virtual int setByRedCost();

	//! Fathoms a node and recursively tries to fathom its father.
	/**
	 * If the root of the remaining branch-and-cut tree is fathomed we are done
	 * since the optimization problem has been solved.
	 *
	 * Otherwise, we count the number of unfathomed sons of the father
	 * of the subproblem being fathomed. If all sons of the father
	 * are fathomed it is recursively fathomed, too. If the father
	 * is the root of the remaining branch-and-cut tree and only one of its
	 * sons is unfathomed, then this unfathomed son becomes the new root
	 * of the remaining branch-and-cut tree.
	 *
	 * We could stop the recursive fathoming already at the root of
	 * the remaining branch-and-cut tree. But, we proceed until the root
	 * of the complete tree was visited to be really correct.
	 *
	 * \note Use the function \a ExceptionFathom() for specifying problem
	 * specific fathoming criteria.
	 *
	 * \param reoptimize If \a reoptimize is \a true, then we perform a reoptimization in
	 *                   the new root. This is controlled via a parameter since it might not be
	 *                   desirable when we find a new root during the fathoming
	 *                   of a complete subtree with the function \a fathomTheSubTree().
	 */
	virtual void fathom(bool reoptimize);

	//! Controls if variables should be fixed or set when all variables price out correctly.
	/**
	 * The default implementation always returns \a true.
	 *
	 * \return true If variables should be fixed and set, false otherwise.
	 */
	virtual bool fixAndSetTime() { return true; }

	//! \brief Fixes a variable.
	/**
	 * If the variable which is currently fixed is already set then
	 * we must not change its bounds in the LP since it might be
	 * eliminated.
	 *
	 * \param i        The number of the variable being fixed.
	 * \param newStat  A pointer to an object storing the new status of
	 *                 the variable.
	 * \param newValue If the variable is fixed to a value different
	 *                 from the one of the last LP-solution, the
	 *                 argument \a newValue is set to \a true. Otherwise,
	 *                 it is set to \a false.
	 *
	 * \return 1 If a contradiction is found, 0 otherwise.
	 */
	virtual int fix(int i, FSVarStat *newStat, bool &newValue);

	//! \brief Sets a variable.
	/**
	 * \param i        The number of the variable being set.
	 * \param newStat  A pointer to the object storing the new status of the
	 *                 the variable.
	 * \param newValue If the variable is set to a value different from
	 *                 the one of the last LP-solution, \a newValue
	 *                 is set to \a true. Otherwise, it is set to \a false.
	 *
	 * \return 1 If a contradiction is found, 0 otherwise.
	 */
	virtual int set(int i, FSVarStat *newStat, bool &newValue);

	//! \brief Sets a variable.
	/**
	 * \param i        The number of the variable being set.
	 * \param newStat  The new status of the variable.
	 * \param newValue If the variable is set to a value different from
	 *                 the one of the last LP-solution, \a newValue
	 *                 is set to \a true. Otherwise, it is set to \a false.
	 *
	 * \return 1 If a contradiction is found, 0 otherwise.
	 */
	virtual int set(int i, FSVarStat::STATUS newStat, bool &newValue);

	//! \brief Sets a variable.
	/**
	 * \param i        The number of the variable being set.
	 * \param newStat  The new status of the variable.
	 * \param value    The value the variable is set to.
	 * \param newValue If the variable is set to a value different from
	 *                 the one of the last LP-solution, \a newValue
	 *                 is set to \a true. Otherwise, it is set to \a false.
	 *
	 * \return 1 If a contradiction is found, 0 otherwise.
	 */
	virtual int set(int i, FSVarStat::STATUS newStat, double value,
		bool &newValue);

	/**
	 * \param x The value that should be rounded if possible.
	 *
	 * \return If all objective function values of feasible solutions are integer
	 *         the function \a dualRound() returns \a x rounded up to the next integer
	 *         if this is a minimization problem, or \a x rounded down to the next integer
	 *         if this is a maximization problem, respectively. Otherwise, the return
	 *         value is \a x.
	 */
	virtual double dualRound(double x);

	//! May not be called if  the lower bound is 0 and upper bound not equal to 0.
	/**
	 * The guarantee that can be given for the subproblem.
	 */
	virtual double guarantee() const;

	/**
	 * \return true If the lower and the upper bound of the subproblem
	 *              satisfies the guarantee requirements, false otherwise.
	 */
	virtual bool guaranteed() const;

	/**
	 * \return true If all active constraints can be lifted.
	 * \return false otherwise.
	 *         In this case the non-liftable constraints are removed and
	 *         \a genNonLiftCons_ is set to \a false to avoid the generation
	 *         of non-liftable constraints in the next cutting plane iterations.
	 */
	virtual bool removeNonLiftableCons();

	//! Is called before a branching step to remove constraints.
	/**
	 * \return 1 If constraints have been removed, 0 otherwise.
	 *
	 * \param lastIteration This argument is always set to \a true in the
	 *                      function call.
	 */
	virtual int prepareBranching(bool &lastIteration);

	//! \brief Fathoms all nodes in the subtree rooted at this subproblem.
	/**
	 * \a Dormant and \a Unprocessed nodes are also
	 * removed from the set of open subproblems.
	 *
	 * If the subproblem is already \a Fathomed we do not have to
	 * proceed in this branch. Otherwise, we fathom the node and
	 * continue with all its sons.
	 * The actual fathoming starts at the unfathomed
	 * leaves of the subtree and recursively goes up through
	 * the tree.
	 */
	virtual void fathomTheSubTree();

	//! \brief Performs the optimization of the subproblem.
	/**
	 * After activating the subproblem, i.e., allocating and initializing
	 * memory, and initializing  the \a LP, the optimization process
	 * constitutes of the three phases
	 * \a Cutting, \a Branching, and \a Fathoming, which are alternately processed.
	 * The function \a fathoming() always returns \a Done. However, we think
	 * that the code is better readable instead of taking it out of the
	 * \a while loop.
	 * The optimization stops if
	 * the \a PHASE \a Done is reached. Note, \a Done does not necessarily mean
	 * that the subproblem is solved to optimality!
	 *
	 * After the node is processed we deallocate memory, which is not required
	 * for further computations or of which the corresponding data can be
	 * easily reconstructed. This is performed in \a _deactivate().
	 *
	 * \return 0 If the optimization has been performed without error, 1 otherwise.
	 */
	virtual int optimize();

	//! \brief Repeats the optimization of an already optimized subproblem.
	/**
	 * This function is used to determine the
	 * reduced costs for fixing variables of a new root of the remaining
	 * branch-and-bound tree.
	 *
	 * As the subproblem has been processed already earlier it is sufficient
	 * to perform the cutting plane algorithm. If the subproblem is
	 * fathomed the complete subtree rooted at this subproblem can be fathomed,
	 * too. Since this function is usually only called for the root of the
	 * remaining branch-and-bound tree, we are done in this case.
	 *
	 * It is not guaranteed that all constraints and variables of this subproblem
	 * can be regenerated in \a _activate(). Therefore, the result of a call
	 * to \a reoptimize() can differ from the result obtained by the cutting
	 * plane algorithm in \a optimize().
	 */
	virtual void reoptimize();

	//! Initializes the active variable set.
	/**
	 * \param maxVar The maximal number of variables of the subproblem.
	 */
	virtual void initializeVars(int maxVar);

	//! Initializes the active constraint set.
	/**
	 * \param maxCon The maximal number of constraints of the subproblem.
	 */
	virtual void initializeCons(int maxCon);

	//! Performs branching.
	/**
	 * Is called if the global lower bound of a
	 * branch-and-cut node is still strictly less than the local upper bound, but
	 * either no violated cutting planes or variables are found, or we abort the
	 * cutting phase for some other strategic reason (e.g.,
	 * observation of a tailing off effect, or branch pausing).
	 *
	 * Usually, two new subproblems are generated. However,
	 * our implementation of \a branching() is more sophisticated that
	 * allows different branching. Moreover,
	 * we also check if this node is only paused. If this is the
	 * case the node is put back into the list of open branch-and-cut
	 * nodes without generating sons of this node.
	 *
	 * Finally if none of the previous conditions is satisfied
	 * we generate new subproblems.
	 *
	 * \return Done      If sons of the subproblem could be generated,
	 * \return Fathoming otherwise.
	 */
	virtual PHASE branching();

	//! Fathoms the node, and if certain conditions are satisfied, also its ancestor.
	/**
	 * The third central phase of the optimization of a subproblem is the
	 * \a Fathoming of a subproblem. A subproblem is fathomed if it can be
	 * guaranteed that this subproblem cannot contain a better solution
	 * than the best known one. This is the case if the global upper bound
	 * does not exceed the local lower bound (maximization problem assumed)
	 * or the subproblem cannot
	 * contain a feasible solution either if there is a fixing/setting
	 * contradiction or the \a LP-relaxation turns out to be infeasible.
	 *
	 * \note Use the function \a ExceptionFathom() for specifying problem
	 * specific fathoming criteria.
	 *
	 * The called function \a fathom() fathoms the subproblem itself
	 * and recursively also tries to fathom its father in the enumeration
	 * tree. The argument of \a fathom() is \a true as a possibly detected
	 * new root should be reoptimized in order to receive better criteria
	 * for fixing variables by reduced costs.
	 *
	 * In the parallel version, only the subproblem itself is fathomed.
	 * No processed unfathomed nodes are kept in memory (father_=0).
	 *
	 * \return The function always returns \a Done.
	 */
	virtual PHASE fathoming();

	//! Iteratively solves the LP-relaxation, generates constraints and/or variables.
	/**
	 * Also generating variables can be regarded as "cutting",
	 * namely as generating cuts for the dual problem.
	 * A reader even studying these lines has been very brave! Therefore,
	 * the first reader of these lines is invited to a beer from the
	 * author.
	 *
	 * \return Fathoming If one of the conditions for fathoming the subproblem is satisfied.
	 * \return Branching If the subproblem should be splitted in further subproblems.
	 */
	virtual PHASE cutting();

	//! Instantiates an \a LP for the solution of the \a LP-relaxation in this subproblem.
	/**
	 * This function is redefined in a derived class for a specific
	 * \a LP-solver interface
	 *
	 * This function is defined in the file <tt>lpif.cpp</tt>.
	 *
	 * \return A pointer to an object of type LpSub.
	 */
	virtual LpSub *generateLp();

	//! Initializes the linear program.
	/**
	 * Since not all variables might be active we
	 * first have to try making the \a LP feasible again by
	 * the addition of variables. If this fails, i.e., \a _initMakeFeas()
	 * has a nonzero return value, we return 1 in order to
	 * indicate that the corresponding subproblem can be fathomed.
	 * Otherwise, we continue with the initialization of the \a LP.
	 *
	 * \return 0 If the linear program could be initialized successfully.
	 * \return 1 If the linear program turns out to be infeasible.
	 */
	virtual int initializeLp();

	//! Solves the LP-relaxation of the subproblem.
	/**
	 * As soon as the \a LP-relaxation becomes infeasible in a static
	 * branch and cut algorithm the respective subproblem can be fathomed.
	 * Otherwise, we memorize the value of the LP-solution to control
	 * the tailing off effect.
	 *
	 * We assume that the \a LP is never primal unbounded.
	 *
	 * \return 0 The linear program has an optimimal solution.
	 * \return 1 If the linear program is infeasible.
	 * \return 2 If the linear program is infeasible for the current
	 *           variable set, but non-liftable constraints have to be
	 *           removed before a pricing step can be performed.
	 */
	virtual int solveLp();

	//! Can be used to specify a problem specific fathoming criterium that is checked before the separation or pricing.
	/**
	 * The default implementation always returns \a false.
	 *
	 * \return true If the subproblem should be fathomed, false otherwise.
	 */
	virtual bool exceptionFathom() { return false; }

	//! Can be used to specify a problem specific criteria for enforcing a branching step.
	/**
	 * This criterium is checked before the separation or
	 * pricing. The default implementation always returns \a false.
	 *
	 * \return true If the subproblem should be fathomed, false otherwise.
	 */
	virtual bool exceptionBranch() { return false; }

	/**
	 * The default implementation always returns false.
	 *
	 * \return True, if the approximative solver should be used to solve
	 *         the next linear program, false otherwise.
	 */
	virtual bool solveApproxNow() { return false; }


	//! A pointer to the corresponding master of the optimization.
	Master *master_;

	//! The active constraints of the subproblem.
	Active<Constraint, Variable> *actCon_;

	//! The active variables of the subproblem.
	Active<Variable, Constraint> *actVar_;

	//! A pointer to the father in the branch-and-cut tree.
	Sub *father_;

	//! A pointer to the corresponding linear program.
	LpSub *lp_;

	//! A pointer to an array storing the status of fixing and setting of the active variables.
	/**
	 * Although fixed and set variables are already kept
	 * at their value by the adaption of the lower and upper bounds, we store
	 * this information, since, e.g., a fixed or set variable should
	 * not be removed, but a variable with an upper bound equal to
	 * the lower bound can be removed.
	 */
	Array<FSVarStat*> *fsVarStat_;

	//! A pointer to an array storing the status of each active variable in the linear program.
	Array<LPVARSTAT*> *lpVarStat_;

	//! A pointer to an array with the local lower bound of the active variables.
	Array<double> *lBound_;

	//! A pointer to an array with the local upper bounds of the active variables.
	Array<double> *uBound_;

	//! A pointer to an array storing the statuses of the slack variables of the last solved linear program.
	Array<SlackStat*> *slackStat_;

	//! A pointer to the tailing off manager.
	TailOff *tailOff_;

	//! The dual bound of the subproblem.
	double dualBound_;

	//! The number of iterations in the cutting plane phase.
	int nIter_;

	//! The last iteration in which constraints have been added.
	int lastIterConAdd_;

	//! The last iteration in which variables have been added.
	int lastIterVarAdd_;

	//! The branching rule for the subproblem.
	BranchRule *branchRule_;

	/**
	 * If \a true, then the branching rule of the subproblem and of all
	 * ancestor on the path to the root node are branching on a binary variable.
	 */
	bool allBranchOnSetVars_;

	//! The solution method for the next linear program.
	LP::METHOD lpMethod_;

	//! The buffer of the newly generated variables.
	CutBuffer<Variable, Constraint> *addVarBuffer_;

	//! The buffer of the newly generated constraints.
	CutBuffer<Constraint, Variable> *addConBuffer_;

	//! The buffer of the variables which are removed at the beginning of the next iteration.
	ArrayBuffer<int> *removeVarBuffer_;

	//! The buffer of the constraints which are removed at the beginning of the next iteration.
	ArrayBuffer<int> *removeConBuffer_;

	//! The last LP-solution.
	double *xVal_;

	//! The dual variables of the last linear program.
	double *yVal_;

	//! A row of the basis inverse associated with the infeasible variable \a infeasVar_ or slack variable \a infeasCon_
	double *bInvRow_;

	//! The number of an infeasible constraint.
	int infeasCon_;

	//! The number of an infeasible variable.
	int infeasVar_;

	//! If \a true, then the management of non-liftable constraints is performed.
	bool genNonLiftCons_;

private:

	//! Returns the number of generated cutting planes.
	virtual int _separate();

	//! Returns the number of eliminated constraints.
	/**
	 * Only dynamic constraints are eliminated from the \a LP.
	 *
	 * It might be worth to implement problem specific versions of this function.
	 */
	virtual int _conEliminate();

	//! Returns the number of eliminated variables.
	/**
	 * Only dynamic variables can be eliminated.
	 */
	virtual int _varEliminate();

	/**
	 * If \a doFixSet is \a true, then we try to fix and set variables, if
	 * all inactive variables price out correctly. In this case \a newValues
	 * becomes \a true of a variable is set or fixed to a value different
	 * from its value in the last linear program.
	 *
	 * In a pricing step the reduced costs of inactive variables are
	 * computed and variables with positive (negative) reduced costs
	 * in a maximization (minimization) problem
	 * are activated.
	 *
	 * The function \a _pricing() returns the
	 * 1 if no global optimality can be guaranteed, since variables have
	 * negative reduced costs, it returns 2 if before a pricing step can
	 * be performed, non-liftable constraints have to be removed,
	 * and 0 if the LP-solution is global dual feasible.
	 *
	 * Also if there are no inactive variables, this function is called
	 * since it will also try to fix and set variables.
	 *
	 * \a true is the default value
	 * of \a doFixSet. No variables should be fixed or set if \a _pricing()
	 * is called from \a _makeFeasible().
	 */
	virtual int _pricing(bool &newValues, bool doFixSet = true);

	//! Tries to find a better feasible solution.
	/**
	 * If a better solution is found its value is stored in \a primalValue
	 * and we return 1, otherwise we return 0.
	 *
	 * If the upper bound has been initialized with the optimum solution
	 * or with the optimum solution plus/minus one these primal heuristics
	 * are skipped.
	 *
	 * The primal bound, if improved, is either updated in the function
	 * \a cutting(), from which \a _improved() is called, are can be
	 * updated in the function \a improve() of an application in a derived class.
	 */
	virtual int _improve(double &primalValue);

	//! Returns 1, if a contradiction has been found, 0 otherwise.
	/**
	 * The parameter \a newValues is set to \a true if a variable
	 * is fixed to value different from its value in the last solved linear program.
	 */
	virtual int _fixByLogImp(bool &newValues);

	//! Adapts the bound of a fixed or set variable \a i also in the linear program.
	/**
	 * This can be only done if a linear
	 * program is available and the variable is not eliminated.
	 */
	virtual void updateBoundInLp(int i);

	//! Returns the value which the upper and lower bounds of a variable should take after it is fixed or set.
	virtual double fixSetNewBound(int i);

	//! Increments the counter for the number of rounds the subproblem is dormant.
	/**
	 * This function is called, when the set of open subproblems is scanned for
	 * the selection of the next subproblem.
	 */
	virtual void newDormantRound() { ++nDormantRounds_; }

	//! Allocates and initializes memory of the subproblem at the beginning of the optimization.
	/**
	 * The function
	 * returns the next phase of the optimization. This is either \a Cutting
	 * or \a Fathoming if the subproblem immediately turns out to be infeasible.
	 *
	 * Since many objects of the class Sub can exist at the
	 * same time, yet in a sequential algorithm only one problem is active,
	 * a lot of memory can be saved if some memory is dynamically allocated
	 * when the subproblem becomes active and other information is
	 * stored in a compressed format for dormant problems.
	 *
	 * These allocations and decompressions are performed by the function
	 * \a _activate(), the respective deallocations and compression of
	 * data is executed by the function \a _deactivate().
	 *
	 * Currently for all subproblems which have not been processed already
	 * (except for the root) we initialize
	 * the active constraints and variables with the respective data from the
	 * father node adapted by the branching information since so
	 * we can make sure that all fixed and set variables are active.
	 * A more flexible strategy might be desirable but also dangerous.
	 *
	 * The virtual function \a activate() can perform problem specific
	 * activations. It is
	 * called before variables are fixed by
	 * logical implications, because, e.g., for problems on graphs,
	 * the graph associated with the subproblem might have to be activated.
	 *
	 * Moreover, the function \a _activate() is redundant in the sense that
	 * it is called only once and could be substituted by a function. However,
	 * having a future generalization to non linear-programming based branch-and-bound in mind, we keep this
	 * function.
	 */
	virtual PHASE _activate();

	//! Deallocates the memory which is not required after the optimization of the subproblem.
	/**
	 * The virtual dummy function \a deactivate() can perform problem specific deactivations.
	 *
	 * As the function \a _activate(),
	 * the function \a _deactivate() is redundant in the sense that
	 * it is called only once and could be substituted by a function. However,
	 * having a future generalization to non linear-programming based branch-and-bound in mind, we keep this
	 * function.
	 */
	virtual void _deactivate();

	//! Tries to add variables to restore infeasibilities detected at initialization time.
	/**
	 * It returns 0 if variables could be activated which might
	 * restore feasibility, otherwise it returns 1.
	 *
	 * The function should analyse the constraints stored in
	 * LpSub::infeasCons_ and try to add inactive variables
	 * which could restore the infeasibility.
	 *
	 * The new variables are only added to the set of active variables
	 * but not to the linear program since no linear program exists
	 * when this function is called.
	 */
	virtual int _initMakeFeas();

	/**
	 * Is called if the \a LP is infeasible and adds inactive variables, which can
	 * make the \a LP feasible again, to the set of active variables.
	 *
	 * The function returns \a 0 if feasibility might have been
	 * restored and \a 1 if it is guaranteed that the linear program
	 * is infeasible on the complete variable set.
	 */
	virtual int _makeFeasible();

	//! Tries to set variables according to logical implications of already set and fixed variables.
	/**
	 * Since logical implications are problem specific
	 * the virtual function \a setByLogImp() is called to find
	 * variables which can be set. If a variable is set to a new value,
	 * i.e., a value different from the one in the last solved LP,
	 * \a newValues is set to \a true.
	 * If such a setting implies a contradiction, \a _setByLogImp() returns
	 * 1, otherwise it returns 0.
	 */
	virtual int _setByLogImp(bool &newValues);

	//! Should be called if a subproblem turns out to be infeasible.
	/**
	 * It sets the dual bound of the subproblem correctly.
	 */
	virtual void infeasibleSub();

	//! Updates the status of the variables and the slack variables.
	virtual void getBase();

	//! Adds the variables stored in the pool slots of \a newVars to the set of active variables, but not to the linear program.
	/**
	 * If the new number of variables exceeds the maximal number of  variables
	 * an automatic reallocation is performed.
	 */
	virtual void activateVars(ArrayBuffer<PoolSlot<Variable, Constraint>*> &newVars);

	//! Adds the variables stored in the pool slots of \a newVars to the linear program. \a localStatus can specify a local status of fixing and setting.
	/**
	 * If the local FSVarStat of the added variables differs from their
	 * global status, then this local status can be stated in \a localStatus.
	 * Per default the value of \a localStatus is 0.
	 */
	virtual void addVarsToLp(ArrayBuffer<PoolSlot<Variable, Constraint>*> &newVars,
		ArrayBuffer<FSVarStat*> *localStatus = nullptr);

	//! Selects the \a master_->maxVarAdd() best variables from the buffered variables.
	/**
	 * \param newVars Holds the selected variables after the call.
	 */
	virtual void _selectVars(ArrayBuffer<PoolSlot<Variable, Constraint>*> &newVars);

	//! Selects the \a master_->maxConAdd() best constraints from the buffered constraints and stores them in \a newCons.
	virtual void _selectCons(ArrayBuffer<PoolSlot<Constraint, Variable>*> &newCons);

	//! Removes the variables with numbers \a remove from the set of active variables.
	virtual int _removeVars(ArrayBuffer<int> &remove);

	//! Removes the constraints with numbers \a remove from the set of active constraints.
	virtual int _removeCons(ArrayBuffer<int> &remove);

	//! Returns true iff the current value of variable i in the primal lp is equal to its upper bound.
	bool _atUpperBound(int i);

	//! Returns true iff the current value of variable i in the primal lp is equal to its lower bound.
	bool _atLowerBound(int i);


	//! The level of the subproblem in the enumeration tree.
	int level_;

	//! The number of the subproblem.
	int id_;

	//! The status of the subproblem.
	STATUS status_;

	//! The sons of the node in the branch-and-cut tree.
	ArrayBuffer<Sub*> *sons_;

	//! The maximum number of iterations in the cutting plane phase.
	int maxIterations_;

	//! The number of optimizations of the subproblem.
	int nOpt_;

	/**
	 * If this member is \a true then the
	 * space reserve of the following three members \a varReserve_,
	 * \a conReserve_, and \a nnzReserve_ is relative to the initial
	 * numbers of constraints, variables, and nonzeros, respectively.
	 * Otherwise, the values are casted to integers and regarded as
	 * absolute values.
	 */
	bool relativeReserve_;

	//! The additional space for variables.
	double varReserve_;

	//! The  additional space for constraints.
	double conReserve_;

	//! The  additional space for nonzeros.
	double nnzReserve_;

	//! The number of subproblem optimizations the subproblem has already the status \a Dormant.
	int nDormantRounds_;

	//! The variable is \a true if the function \a activate() has been called from the function \a _activate().
	/**
	 * This memorization is required such that a \a deactivate() is only called when
	 * \a activate() has been called.
	 */
	bool activated_;

	//! If this flag is set to \a true then the next LP-solution is ignored in the tailing-off control.
	/**
	 * The default value of the variable is \a false.
	 * It can be set to \a true by the function \a ignoreInTailingOff().
	 */
	bool ignoreInTailingOff_;

	//! \brief The method that was used to solve the last LP
	LP::METHOD lastLP_;

	ogdf::StopwatchCPU localTimer_;

	//! Indicates whether to force the use of an exact solver to prepare branching etc.
	bool forceExactSolver_;

	Sub(const Sub &rhs);
	const Sub &operator=(const Sub &rhs);
};

}

// NOW declaration of sub is complete. its definitions below need full declarations of the below types...

#include <ogdf/lib/abacus/variable.h>
#include <ogdf/lib/abacus/constraint.h>
#include <ogdf/lib/abacus/active.h>
#include <ogdf/lib/abacus/cutbuffer.h>
#include <ogdf/lib/abacus/lpsub.h>

namespace abacus {

inline int Sub::addBranchingConstraint(PoolSlot<Constraint, Variable> *slot)
{
	return addConBuffer_->insert(slot, true);
}

inline int Sub::addConBufferSpace() const
{
	return addConBuffer_->space();
}

inline int Sub::addVarBufferSpace() const
{
	return addVarBuffer_->space();
}

inline int Sub::nVar() const
{
	return actVar_->number();
}

inline int Sub::nCon() const
{
	return actCon_->number();
}

inline int Sub::maxVar() const
{
	return actVar_->max();
}

inline int Sub::maxCon() const
{
	return actCon_->max();
}

inline Constraint *Sub::constraint(int i) const
{
	return (*actCon_)[i];
}

inline Variable *Sub::variable(int i) const
{
	return (*actVar_)[i];
}

inline double Sub::rankBranchingRule(BranchRule *branchRule)
{
	return lpRankBranchingRule(branchRule, master_->nStrongBranchingIterations());
}

inline double Sub::lowerBound() const
{
	if (master_->optSense()->max())
		return master_->primalBound();
	else
		return dualBound_;
}

inline double Sub::upperBound() const
{
	if (master_->optSense()->min())
		return master_->primalBound();
	else
		return dualBound_;
}

inline bool Sub::betterDual(double x) const
{
	if (master_->optSense()->max())
		return x < dualBound_ ? true : false;
	else
		return x > dualBound_ ? true : false;
}

inline bool Sub::boundCrash() const
{
	return(master_->primalViolated(dualBound_));
}

inline void Sub::removeCon(int i)
{
	removeConBuffer_->push(i);
}

inline void Sub::lBound(int i, double l)
{
	(*lBound_)[i] = l;
	if (lp_)
		lp_->changeLBound(i, l);
}

inline void Sub::uBound(int i, double u)
{
	(*uBound_)[i] = u;
	if (lp_)
		lp_->changeUBound(i, u);
}

}
