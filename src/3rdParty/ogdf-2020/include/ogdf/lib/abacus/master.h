/*!\file
 * \author Matthias Elf
 * \brief the master of the optimization.
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

#include <ogdf/lib/abacus/global.h>
#include <ogdf/lib/abacus/optsense.h>

#include <ogdf/lib/abacus/hash.h>
#include <ogdf/basic/Stopwatch.h>


class OsiSolverInterface;

namespace abacus {


class Sub;
class BranchRule;
class Variable;
class Constraint;

class History;
class OpenSub;
class FixCand;
class LpMasterOsi;

template<class BaseType, class CoType> class StandardPool;


//! The master of the optimization.
/**
 * As the name already indicates, the class Master is the central
 * object of the framework. The most important tasks of the class Master
 * is the management of the implicit enumeration. Moreover, it provides already
 * default implementations for constraints, cutting planes, and
 * variables pools. The class Master also stores various parameter
 * settings and compiles statistics about the solution process.
 *
 * The class Master is an abstract class from which a problem specific
 * master has to be derived.
 */
class OGDF_EXPORT Master : public AbacusGlobal {

	friend class Sub;
	friend class FixCand;

public:

	//! The various statuses of the optimization process.
	enum STATUS {
		Optimal,		/*!< The optimization terminated with an error and without reaching
						 *   one of the resource limits. If there is a feasible solution then the
						 *   optimal solution has been computed. */
		Error,			/*!< An error occurred during the optimization process. */
		OutOfMemory,
		Unprocessed,    /*!< The initial status, before the optimization starts. */
		Processing,		/*!< The status while the optimization is performed. */
		Guaranteed,		/*!< If the optimal solution is not determined but the required guarantee
						 *   is reached, then the status is \a Guaranteed. */
		MaxLevel,		/*!< The status, if subproblems are ignored since the maximum enumeration level is exceeded. */
		MaxCpuTime,		/*!< The status, if the optimization terminates since the maximum cpu time is exceeded. */
		MaxNSub,		/*!< The status, if the optimization terminates since the maximum number of subproblems is reached. */
		MaxCowTime,		/*!< The status, if the optimization terminates since the maximum wall-clock time is exceeded. */
		ExceptionFathom /*!< The status, if at least one subproblem has been fathomed according to a
						 *   problem specific criteria determined in the function Sub::exceptionFathom(). */
	};

	//! Literal values for the enumerators of the corresponding enumeration type.
	/**
	 * The order of the enumerators is preserved (e.g., <tt>STATUS_[0]=="Optimal"</tt>).
	 */
	static const char* STATUS_[];


	//! The enumeration defining the different enumeration strategies for the branch and bound algorithm.
	enum ENUMSTRAT {
		BestFirst,		/*!< Best-first search, i.e., select the subproblem with best dual
						 *   bound, i.e., the subproblem having minimal dual bound for a minimization
						 *   problem, or the subproblem having maximal dual bound for a maximization problem. */
		BreadthFirst,	/*!< Breadth-first search, i.e., select the subproblem with minimal level in the enumeration tree. */
		DepthFirst,		/*!< Depth-first search, i.e., select the subproblem with maximal level in the enumeration tree. */
		DiveAndBest		/*!< As long as no primal feasible solution is known the next subproblem is selected according
						 *   to the depth-first search strategy, otherwise the best-first search strategy is applied. */
	};

	//! Literal values for the enumerators of the corresponding enumeration type.
	/**
	 * The order of the enumerators is preserved (e.g., <tt>ENUMSTRAT_[0]=="BestFirst"</tt>).
	 */
	static const char *ENUMSTRAT_[];

	//! This enumeration defines the two currently implemented branching variable selection strategies.
	enum BRANCHINGSTRAT {
		CloseHalf,			/*!< Selects the variable with fractional part closest to 0.5. */
		CloseHalfExpensive	/*!< Selects the variable with fractional part close to 0.5 (within some
							 *   interval around 0.5) and has highest absolute objective function coefficient. */
	};

	//! Literal values for the enumerators of the corresponding enumeration type.
	/**
	 * The order of the enumerators is preserved (e.g., <tt>BRANCHINGSTRAT_[0]=="CloseHalf"</tt>).
	 */
	static const char *BRANCHINGSTRAT_[];

	//! This enumeration provides various methods for the initialization of the primal bound.
	/**
	 * The modes \a OptimalPrimalBound and \a OptimalOnePrimalBound can be useful in the testing phase.
	 * For these modes the value of an optimum solution must stored in the file given
	 * by the parameter <tt>OptimumFileName</tt> in the parameter file.
	 */
	enum PRIMALBOUNDMODE {
		NoPrimalBound,	/*!< The primal bound is initialized with \f$-\infty\f$ for maximization problems and
						 *   \f$\infty\f$ for minimization problems, respectively. */
		Optimum,		/*!< The primal bound is initialized with the value of the optimum solution. */
		OptimumOne		/*!< The primal bound is initialized with the value of optimum solution minus 1 for maximization problems
						 *   and with the value of the optimum solution plus one for minimization problems, respectively. */
	};

	//! Literal values for the enumerators of the corresponding enumeration type.
	/**
	 * The order of the enumerators is preserved (e.g., <tt>PRIMALBOUNDMODE_[0]=="None"</tt>).
	 */
	static const char* PRIMALBOUNDMODE_[];

	//! The way nodes are skipped for the generation of cuts.
	enum SKIPPINGMODE{
		SkipByNode,	/*!< Cuts are only generated in every <tt>SkipFactor</tt> subproblem, where
					 *   <tt>SkipFactor</tt> can be controlled with the parameter file <tt>.abacus</tt>. */
		SkipByLevel	/*!< Cuts are only generated in every <tt>SkipFactor</tt> level of the enumeration tree. */
	};

	//! Literal values for the enumerators of the corresponding enumeration type.
	/**
	 * The order of the enumerators is preserved (e.g., <tt>SKIPPINGMODE_[0]=="None"</tt>).
	 */
	static const char* SKIPPINGMODE_[];

	//! This enumeration defines the ways for automatic constraint elimination during the cutting plane phase.
	enum CONELIMMODE {
		NoConElim,	/*!< No constraints are eliminated. */
		NonBinding,	/*!< Nonbinding constraints are eliminated. */
		Basic		/*!< Constraints with basic slack variable are eliminated. */
	};


	//! Literal values for the enumerators of the corresponding enumeration type.
	/**
	 * The order of the enumerators is preserved (e.g., <tt>CONELIMMODE_[0]=="None"</tt>).
	 */
	static const char* CONELIMMODE_[];

	//! This enumeration defines the ways for automatic variable elimination during the column generation algorithm.
	enum VARELIMMODE {
		NoVarElim,	/*!< No variables are eliminated. */
		ReducedCost	/*!< Variables with high absolute reduced costs are eliminated. */
	};

	//! Literal values for the enumerators of the corresponding enumeration type.
	/**
	 * The order of the enumerators is preserved (e.g., <tt>VARELIMMODE_[0]=="None"</tt>).
	 */
	static const char* VARELIMMODE_[];

	//! This enumeration defines what kind of output can be generated for the VBCTOOL.
	enum VBCMODE {
		NoVbc,	/*!< No output for the tree interface. */
		File,	/*!< Output for the tree interface is written to a file. */
		Pipe	/*!< Output for the tree interface is pipeed to the standard output. */
	};

	//! Literal values for the enumerators of the corresponding enumeration type.
	/**
	 * The order of the enumerators is preserved (e.g., <tt>VBCMODE_[0]=="None"</tt>).
	 */
	static const char* VBCMODE_[];


	//! This enumeration defines which solvers can be used to solve the LP-relaxations.
	/**
	 * These are all solvers supported by OSI, see https://projects.coin-or.org/Osi .
	 */
	enum OSISOLVER { Cbc, Clp, CPLEX, DyLP, FortMP, GLPK, MOSEK, OSL, SoPlex, SYMPHONY, XPRESS_MP, Gurobi, Csdp };

	//! Array for the literal values for possible Osi solvers.
	static const char* OSISOLVER_[];

	//! The constructor.
	/**
	 * The members \a primalBound_ and \a dualBound_ stay uninitialized
	 * since this can only be done when the sense of optimization
	 * (minimization or maximization) is known. The initialization
	 * is performed automatically in the function \a optimize().
	 *
	 * \param problemName The name of the problem being solved. Must not be a 0-pointer.
	 * \param cutting     If \a true, then cutting planes can be generated if the function
	 *                    Sub::separate() is redefined.
	 * \param pricing     If \a true, then inactive variables are priced in, if the function
	 *                    Sub::pricing() is redefined.
	 * \param optSense    The sense of the optimization. The default value is OptSense::Unknown.
	 *                    If the sense is unknown when this constructor is called, e.g., if it is
	 *                    read from a file in the constructor of the derived class, then it must
	 *                    be initialized in the constructor of the derived class.
	 * \param eps         The zero-tolerance used within all member functions of objects which
	 *                    have a pointer to this master (default value \a 1.0e-4).
	 * \param machineEps  The machine dependent zero tolerance (default value \a 1.0e-7).
	 * \param infinity    All values greater than \a infinity are regarded as "infinite big",
	 *                    all values less than \a -infinity are regarded as "infinite small"
	 *                    (default value \a 1.0e30).
	 * \param readParamFromFile If true, then the parameter file <tt>.abacus</tt> is read,
	 *                    otherwise the parameters are initialized with default values (default \a true).
	 */
	Master(const char *problemName, bool cutting, bool pricing,
		OptSense::SENSE optSense = OptSense::Unknown,
		double eps = 1.0e-4, double machineEps = 1.0e-7,
		double infinity = 1.0e30,
		bool readParamFromFile = false);

	//! The destructor.
	virtual ~Master();

	//! Performs the optimization by branch-and-bound.
	/**
	 * \return The status of the optimization.
	 */
	STATUS optimize();

	/*! @name Bounds
	 *
	 * In order to embed both minimization and maximization problems in this
	 * system we work internally with primal bounds, i.e., a value which
	 * is worse than the best known solution (often a value of a feasible
	 * solution), and dual bounds, i.e., a bound
	 * which is better than the best known solution. Primal and dual bounds
	 * are then interpreted as lower or upper bounds according to the
	 * sense of the optimization.
	 *
	 */
	//@{

	//! Returns the value of the global lower bound.
	double lowerBound() const;

	//! Returns the value of the global upper bound.
	double upperBound() const;

	//! Returns the value of the primal bound.
	/**
	 * I.e., the \a lowerBound() for a maximization problem and the
	 * \a upperBound() for a minimization problem, respectively.
	 */
	double primalBound() const { return primalBound_; }

	//! Sets the primal bound to \a x and makes a new entry in the solution history.
	/**
	 * It is an error if the primal bound gets worse.
	 *
	 * \param x The new value of the primal bound.
	 */
	void primalBound(double x);

	//! Returns the value of the dual bound.
	/**
	 * I.e., the \a upperBound() for a maximization problem and the
	 * \a lowerBound() for a minimization problem, respectively.
	 */
	double dualBound() const { return dualBound_; }

	//! Sets the dual bound to \a x and makes a new entry in the solution history.
	/**
	 * It is an error if the dual bound gets worse.L
	 *
	 * \param x The new value of the dual bound.
	 */
	void dualBound(double x);

	//! Returns true if \a x is better than the best known dual bound; false otherwise.
	/**
	 * \param x The value being compared with the best know dual bound.
	 */
	bool betterDual(double x) const;

	//! Can be used to compare a value with the one of the best known primal bound.
	/**
	 * If the objective function values of all feasible solutions are
	 * integer, then we do not have to be so carefully.
	 *
	 * \param x The value being compared with the primal bound.
	 *
	 * \return true If \a x is not better than the best known primal bound, false otherwise.
	 */
	bool primalViolated(double x) const;

	//! Can be used to check if a value is better than the best know primal bound.
	/**
	 * \param x The value compared with the primal bound.
	 *
	 * \return true If \a x is better than the best known primal bound, false otherwise.
	 */
	bool betterPrimal(double x) const;

	//! Returns the dual bound at the root node.
	double rootDualBound() const { return rootDualBound_; }

	//! We use this function, e.g., to adapt the enumeration strategy in the \a DiveAndBest-Strategy.
	/***
	 * This function is only correct if any primal bound better than plus/minus infinity corresponds
	 * to a feasible solution.
	 *
	 * \return true If a feasible solution of the optimization problem has been found, false otherwise.
	 */
	bool feasibleFound() const;
	//@}

	//! Returns the enumeration strategy.
	ENUMSTRAT enumerationStrategy() const { return enumerationStrategy_; }

	//! Changes the enumeration strategy to \a strat.
	/**
	 * \param strat The new enumeration strategy.
	 */
	void enumerationStrategy(ENUMSTRAT strat) { enumerationStrategy_ = strat; }

	/**
	 * Analyzes the enumeration strategy set in the parameter file <tt>.abacus</tt>
	 * and calls the corresponding comparison function for the subproblems
	 * \a s1 and \a s2. This function should be redefined for application
	 * specific enumeration strategies.
	 *
	 * \return  1 If \a s1 has higher priority than \a s2;
	 * \return -1 if \a s2 has higher priority it returns -1; and
	 * \return  0 if both subproblems have equal priority.
	 *
	 * \param s1 A pointer to a subproblem.
	 * \param s2 A pointer to a subproblem.
	 */
	virtual int enumerationStrategy(const Sub *s1, const Sub *s2);

	//! Can be used to check if the guarantee requirements are fulfilled.
	/**
	 * I.e., the difference between upper bound and the lower bound in respect to the lowerBound
	 * is less than this guarantee value in percent.
	 *
	 * If the lower bound is zero, but the upper bound is nonzero,
	 * we cannot give any guarantee.
	 *
	 * \warning A guarantee for a solution can only be given if the
	 * pricing problem is solved exactly or no column generation is performed
	 * at all.
	 *
	 * \return true If the guarantee requirements are fulfilled, false otherwise.
	 */
	bool guaranteed() const;

	//! Can be used to access the guarantee which can be given for the best known feasible solution.
	/**
	 * It is an error to call this function if the lower bound is zero, but the upper bound
	 * is nonzero.
	 *
	 * \return The guarantee for best known feasible solution in percent.
	 */
	double guarantee() const;

	//! Writes the guarantee nicely formated on the output stream associated with this object.
	/**
	 * If no bounds are available, or the lower bound is zero, but the
	 * upper bound is nonzero, then we cannot give any guarantee.
	 */
	void printGuarantee() const;

	//! Can be used to control the correctness of the optimization if the value of the optimum solution has been loaded.
	/**
	 * This is done, if a file storing the optimum value is specified with
	 * the parameter <tt>OptimumFileName</tt> in the configuration file
	 * <tt>.abacus</tt>.
	 *
	 * \return true If the optimum solution of the problem is known and equals the primal bound,
	 *         false otherwise.
	 */
	bool check() const;

	/**
	 * Opens the file specified with the parameter <tt>OptimumFileName</tt> in the configuration
	 * file <tt>.abacus</tt> and tries to find a line
	 * with the name of the problem instance (as specified in the
	 * constructor of Master) as first string.
	 *
	 * \param optVal If the return value is \a true, then \a optVal holds the
	 *               optimum value found in the line with the name of the problem instance
	 *               as first string. Otherwise, \a optVal is undefined.
	 *
	 * \return true If a line with \a problemName_ has been found, false otherwise.
	 */
	bool knownOptimum(double &optVal) const;

	//! Does nothing but can be redefined in derived classes for output before the timing statistics.
	virtual void output() const { }

	/**
	 * \return true If \a cutting has been set to \a true in the call of the
	 *         constructor of the class Master, i.e., if cutting
	 *         planes should be generated in the subproblem optimization;
	 *         false otherwise.
	 */
	bool cutting() const { return cutting_; }

	/**
	 * \return true If \a pricing has been set to true in the call of the
	 *         constructor of the class Master, i.e., if a columns
	 *         should be generated in the subproblem optimization;
	 *         false otherwise.
	 */
	bool pricing() const { return pricing_; }

	//! Returns a pointer to the object holding the optimization sense of the problem.
	const OptSense *optSense() const { return &optSense_; }

	//! Returns a pointer to the object storing the solution history of this branch and cut problem.
	History *history() const { return history_; }

	//! Returns a pointer to the set of open subproblems.
	OpenSub *openSub() const { return openSub_; }

	//! Returns a pointer to the default pool storing the constraints of the problem formulation.
	StandardPool<Constraint, Variable> *conPool() const { return conPool_; }

	//! Returns a pointer to the default pool for the generated cutting planes.
	StandardPool<Constraint, Variable> *cutPool() const { return cutPool_; }

	//! Returns a pointer to the default pool storing the variables.
	StandardPool<Variable, Constraint> *varPool() const { return varPool_; }

	//! Can be used to access the root node of the branch-and-bound tree.
	/**
	 * \return  A pointer to the root node of the enumeration tree.
	 */
	Sub *root() const { return root_; }

	/**
	 * \return A pointer to the root of the remaining branch-and-bound tree,
	 *   i.e., the subproblem which is an ancestor of all open
	 *   subproblems and has highest level in the tree.
	 */
	Sub *rRoot() const { return rRoot_; }

	//! Returns the status of the Master.
	STATUS status() const { return status_; }

	//! Returns the name of the instance being optimized (as specified in the constructor of this class).
	const string &problemName() const { return problemName_; }

	//! Returns a pointer to the timer measuring the total wall clock time.
	const ogdf::StopwatchWallClock *totalCowTime() const { return &totalCowTime_; }

	//! True, if an approximative solver should be used.
	inline bool solveApprox() const { return solveApprox_; }

	//! returns a pointer to the timer measuring the total cpu time for the optimization.
	const ogdf::StopwatchCPU *totalTime() const { return &totalTime_; }

	//! Returns a pointer to the timer measuring the cpu time spent in members of the LP-interface.
	const ogdf::StopwatchCPU *lpTime() const { return &lpTime_; }

	//! Return a pointer to the timer measuring  the cpu time required by the LP solver.
	const ogdf::StopwatchCPU *lpSolverTime() const { return &lpSolverTime_; }

	//! Returns a pointer to the timer measuring the cpu time spent in the separation of cutting planes.
	const ogdf::StopwatchCPU *separationTime() const { return &separationTime_; }

	//! Returns a pointer to the timer measuring the cpu time spent in the heuristics for the computation of feasible solutions.
	const ogdf::StopwatchCPU *improveTime() const { return &improveTime_; }

	//! Returns a pointer to the timer measuring the cpu time spent in pricing.
	const ogdf::StopwatchCPU *pricingTime() const { return &pricingTime_; }

	//! Returns a pointer to the timer measuring the cpu time spent in finding and selecting the branching rules.
	const ogdf::StopwatchCPU *branchingTime() const { return &branchingTime_; }

	//! returns the number of generated subproblems.
	int nSub() const { return nSub_; }

	//! Returns the number of optimized linear programs (only LP-relaxations).
	int nLp() const { return nLp_; }

	//! Returns the highest level in the tree which has been reached during the implicit enumeration.
	int highestLevel() const { return highestLevel_; }

	//! Returns the number of root changes of the remaining branch-and-cut tree.
	int nNewRoot() const { return nNewRoot_; }

	//! Returns the number of subproblems which have already been selected from the set of open subproblems.
	int nSubSelected() const { return nSubSelected_; }

	//! Writes all parameters of the class Master together with their values to the global output stream.
	void printParameters() const;

	//! Returns the branching strategy.
	BRANCHINGSTRAT branchingStrategy() const { return branchingStrategy_; }

	//! Changes the branching strategy to \a strat.
	/**
	 * \param strat The new branching strategy.
	 */
	void branchingStrategy(BRANCHINGSTRAT strat) { branchingStrategy_ = strat; }

	//! returns the Lp Solver.
	OSISOLVER defaultLpSolver() const { return defaultLpSolver_; }

	//! Changes the default Lp solver to \a osiSolver.
	/**
	 * \param osiSolver The new solver.
	 */
	void defaultLpSolver(OSISOLVER osiSolver) { defaultLpSolver_ = osiSolver; }

	LpMasterOsi *lpMasterOsi() const { return lpMasterOsi_; }

	//! Returns the number of variables that should be tested for the selection of the branching variable.
	int nBranchingVariableCandidates() const { return nBranchingVariableCandidates_; }

	//! Sets the number of tested branching variable candidates to \a n.
	/**
	 * \param n The new value of the number of tested variables for
	 *          becoming branching variable.
	 */
	void nBranchingVariableCandidates(int n);

	//! The number of simplex iterations that are performed when testing candidates for branching variables within strong branching.
	int nStrongBranchingIterations() const { return nStrongBranchingIterations_; }

	//! Sets the number of simplex iterations that are performed when testing candidates for branching variables within strong branching.
	/**
	 * \param n The new value of the number of simplex iterations.
	 */
	void nStrongBranchingIterations(int n);

	//! The guarantee specification for the optimization.
	double requiredGuarantee() const { return requiredGuarantee_; }

	//! Changes the guarantee specification tp \a g.
	/**
	 * \param g The new guarantee specification (in percent).
	 *          This must be a nonnative value. Note, if the guarantee specification is changed
	 *          after a single node of the enumeration tree has been
	 *          fathomed, then the overall guarantee might differ from
	 *          the new value.
	 */
	void requiredGuarantee(double g);

	//! Returns the maximal depth up to which the enumeration should be performed.
	/**
	 *  By default the maximal enumeration depth is \a INT\_MAX.
	 */
	int maxLevel() const { return maxLevel_; }

	//! This version of the function \a maxLevel() changes the maximal enumeration depth.
	/**
	 * If it is set to 1 the branch-and-cut algorithm becomes a pure cutting plane algorithm.
	 *
	 * \param ml The new value of the maximal enumeration level.
	 */
	void maxLevel(int ml);

	//! Returns the maximal number of subproblems to be processed.
	/**
	 * By default this number is \a INT\_MAX.
	 */
	int maxNSub() const { return maxNSub_; }

	//! Changes the maximal number of subproblems to \a ml.
	/**
	 * If it is set to 1 the branch-and-cut algorithm becomes a pure cutting plane algorithm.
	 *
	 * \param ml The new value of the maximal enumeration level.
	 */
	void maxNSub(int ml);

	//! Returns the maximal cpu time (in seconds) which can be used by the optimization.
	int64_t maxCpuTime() const { return maxCpuTime_; }

	//! Returns the maximal cpu time (as string <tt>hh:mm:ss</tt>) which can be used by the optimization.
	string maxCpuTimeAsString() const;

	//! Sets the maximally allowed cpu time for the optimization to \a t.
	/**
	 * \param t The new value of the maximal cpu time in the form <tt>hh:mm:ss</tt>.
	 */
	void maxCpuTime(const string &t);

	//! Sets the maximally allowed cpu time to \a seconds.
	void maxCpuTime(int64_t seconds) { maxCpuTime_ = seconds; }

	//! Sets the maximally allowed cpu time for the optimization to \a hour, \a min, \a sec.
	void maxCpuTime(int hour, int min, int sec);

	//! Returns the maximal wall-clock time (in seconds) which can be used by the optimization.
	int64_t maxCowTime() const { return maxCowTime_; }

	//! Returns the maximal wall-clock time (as string <tt>hh:mm:ss</tt>) which can be used by the optimization.
	string maxCowTimeAsString() const;

	//! Sets the maximally allowed wall-clock time to \a seconds.
	void maxCowTime(int64_t seconds) { maxCowTime_ = seconds; }

	//! Sets the maximally allowed wall-clock time for the optimization to \a t.
	/**
	 * \param t The new value of the maximal cpu time in the form <tt>hh:mm:ss</tt>.
	 */
	void maxCowTime(const string &t);

	//! If true then we assume that all feasible solutions have integral objective function values.
	bool objInteger() const { return objInteger_; }

	//! Sets the assumption that the objective function values of all feasible solutions are integer.
	/**
	 * \param b The new value of the assumption.
	 */
	void objInteger(bool b) { objInteger_ = b; }

	//! Returns the number of linear programs considered in the tailing off analysis.
	int tailOffNLp() const { return tailOffNLp_; }

	//! Sets the number of linear programs considered in the tailing off analysis to \a n.
	/**
	 * This new value is only
	 * relevant for subproblems activated <b>after</b> the change of this value.
	 *
	 * \param n The new number of LPs for the tailing off analysis.
	 */
	void tailOffNLp(int n) { tailOffNLp_ = n; }

	//! Returns the minimal change of the dual bound for the tailing off analysis in percent.
	double tailOffPercent() const { return tailOffPercent_; }

	//! Sets the minimal change of the dual bound for the tailing off analysis to \a p.
	/**
	 * This change is only
	 * relevant for subproblems activated <b>after</b> calling this function.
	 *
	 * \param p The new value for the tailing off analysis.
	 */
	void tailOffPercent(double p);

	//! Returns true if the number of optimizations \a nOpt of a subproblem exceeds the delayed branching threshold, false otherwise.
	/**
	 * \param nOpt The number of optimizations of a subproblem.
	 */
	bool delayedBranching(int nOpt) const;

	//! Sets the number of optimizations of a subproblem until sons are created in Sub::branching().
	/**
	 * If this value is 0, then a branching step is performed at the
	 * end of the subproblem optimization as usually if the subproblem
	 * can be fathomed. Otherwise, if this value is strictly positive,
	 * the subproblem is put back for a later optimization. This can be
	 * advantageous if in the meantime good cutting planes or primal bounds
	 * can be generated. The number of times the subproblem is put back
	 * without branching is indicated by this value.
	 *
	 * \param threshold The new value of the delayed branching threshold.
	 */
	void dbThreshold(int threshold) { dbThreshold_ = threshold; }

	//! Returns the number of optimizations of a subproblem until sons are created.
	/**
	 * For further detatails we refer to \a dbThreshold(int).
	 */
	int dbThreshold() const { return dbThreshold_; }

	//! Returns the maximal number of rounds, i.e., number of subproblem optimizations, a subproblem is dormant.
	/**
	 * I.e., it is not selected from the set
	 * of open subproblem if its status is \a Dormant, if possible.
	 */
	int minDormantRounds() const { return minDormantRounds_; }

	//! Sets the number of rounds a subproblem should stay dormant to \a nRounds.
	/**
	 * \param nRounds The new minimal number of dormant rounds.
	 */
	void minDormantRounds(int nRounds) { minDormantRounds_ = nRounds; }

	//! Returns the mode of the primal bound initialization.
	PRIMALBOUNDMODE pbMode() const { return pbMode_; }

	//! Sets the mode of the primal bound initialization to \a mode.
	/**
	 * \param mode The new mode of the primal bound initialization.
	 */
	void pbMode(PRIMALBOUNDMODE mode) { pbMode_ = mode; }

	//! Returns the number of linear programs being solved between two additional pricing steps.
	/**
	 * If no additional pricing steps should be
	 * executed this parameter has to be set to 0.
	 * The default value of the pricing frequency is 0. This parameter
	 * does not influence the execution of pricing steps which are
	 * required for the correctness of the algorithm.
	 */
	int pricingFreq() const { return pricingFreq_; }

	//! Sets the number of linear programs being solved between two additional pricing steps to \a f.
	/**
	 * \param f The pricing frequency.
	 */
	void pricingFreq(int f);

	//! Returns the frequency of subproblems in which constraints or variables should be generated.
	int skipFactor() const { return skipFactor_; }

	//! Sets the frequency for constraint and variable generation to \a f.
	/**
	 * \param f The new value of the frequency.
	 */
	void skipFactor(int f);

	//! Sets the skipping strategy to \a mode.
	/**
	 * \param mode The new skipping strategy.
	 */
	void skippingMode(SKIPPINGMODE mode) { skippingMode_ = mode; }

	//! Returns the skipping strategy.
	SKIPPINGMODE skippingMode() const { return skippingMode_; }

	//! Returns the mode for the elimination of constraints.
	CONELIMMODE conElimMode() const { return conElimMode_; }

	//! Changes the constraint elimination mode to \a mode.
	/**
	 * \param mode The new constraint elimination mode.
	 */
	void conElimMode(CONELIMMODE mode) { conElimMode_ = mode; }

	//! Returns the mode for the elimination of variables.
	VARELIMMODE varElimMode() const { return varElimMode_; }

	//! Changes the variable elimination mode to \a mode.
	/**
	 * \param mode The new variable elimination mode.
	 */
	void varElimMode(VARELIMMODE mode) { varElimMode_ = mode; }

	//! Returns the zero tolerance for the elimination of constraints by the slack criterion.
	double conElimEps() const { return conElimEps_; }

	//! Changes the tolerance for the elimination of constraints by the slack criterion to \a eps.
	/**
	 * \param eps The new tolerance.
	 */
	void conElimEps(double eps) { conElimEps_ = eps; }

	//! Returns the zero tolerance for the elimination of variables by the reduced cost criterion.
	double varElimEps() const { return varElimEps_; }

	//! Changes the tolerance for the elimination of variables by the reduced cost criterion to \a eps.
	/**
	 * \param eps The new tolerance.
	 */
	void varElimEps(double eps) { varElimEps_ = eps; }

	//! Returns the age for the elimination of variables by the reduced cost criterion.
	int varElimAge() const { return varElimAge_; }

	//! Changes the age for the elimination of variables by the reduced cost criterion to \a age.
	/**
	 * \param age The new age.
	 */
	void varElimAge(int age) { varElimAge_ = age; }

	//! Returns the age for the elimination of constraints.
	int conElimAge() const { return conElimAge_; }

	//! Changes the age for the elimination of constraints to \a age.
	/**
	 * \param age The new age.
	 */
	void conElimAge(int age) { conElimAge_ = age; }

	/**
	 * \return true  Then variables are fixed and set by reduced cost criteria.
	 * \return false Then no variables are fixed or set by reduced cost criteria.
	 */
	bool fixSetByRedCost() const { return fixSetByRedCost_; }

	//! Turns fixing and setting variables by reduced cost on or off.
	/**
	 * \param on If \a true, then variable fixing and setting by reduced
	 *           cost is turned on. Otherwise it is turned of.
	 */
	void fixSetByRedCost(bool on) { fixSetByRedCost_ = on; }

	/**
	 * \return true  Then the linear program is output every iteration of the subproblem optimization;
	 * \return false The linear program is not output.
	 */
	bool printLP() const { return printLP_; }

	//! Turns the output of the linear program in  every iteration on or off.
	/**
	 * \param on If \a true, then the linear program is output,
	 *           otherwise it is not output.
	 */
	void printLP(bool on) { printLP_ = on; }

	//! Returns the maximal number of constraints which should be added in every iteration of the cutting plane algorithm.
	int maxConAdd() const { return maxConAdd_; }

	//! Sets the maximal number of constraints that are added in an iteration of the cutting plane algorithm.
	/***
	 * \param max The maximal number of constraints.
	 */
	void maxConAdd(int max) { maxConAdd_ = max; }

	//! Returns the size of the buffer for generated constraints in the cutting plane algorithm.
	int maxConBuffered() const { return maxConBuffered_; }

	//! Changes the maximal number of constraints that are buffered in an iteration of the cutting plane algorithm.
	/**
	 * \note This function changes only the default value for subproblems
	 * that are activated after its call.
	 *
	 * \param max The new maximal number of buffered constraints.
	 */
	void maxConBuffered(int max) { maxConBuffered_ = max; }

	//! Returns the maximal number of variables which should be added in the column generation algorithm.
	int maxVarAdd() const { return maxVarAdd_; }

	//! Changes the maximal number of variables that are added in an iteration of the subproblem optimization.
	/**
	 * \param max The new maximal number of added variables.
	 */
	void maxVarAdd(int max) { maxVarAdd_ = max; }

	//! Returns the size of the buffer for the variables generated in the column generation algorithm.
	int maxVarBuffered() const { return maxVarBuffered_; }

	//! Changes the maximal number of variables that are buffered in an iteration of the subproblem optimization.
	/**
	 * \note This function changes only the default value for subproblems
	 * that are activated after its call.
	 *
	 * \param max The new maximal number of buffered variables.
	 */
	void maxVarBuffered(int max) { maxVarBuffered_ = max; }

	//! Returns the maximal number of iterations per subproblem optimization (-1 means no iteration limit).
	int maxIterations() const { return maxIterations_; }

	//! Changes the default value for the maximal number of iterations of the optimization of a subproblem.
	/**
	 * \note This function changes only this value for subproblems that
	 * are constructed after this function call. For already constructed
	 * objects the value can be changed with the function
	 * Sub::maxIterations().
	 *
	 * \param max The new maximal number of iterations of the subproblem
	 *            optimization (-1 means no limit).
	 */
	void maxIterations(int max) { maxIterations_ = max; }

	/**
	 * \return true  Then we try to eliminate fixed and set variables from the linear program;
	 * \return false Fixed or set variables are not eliminated.
	 */
	bool eliminateFixedSet() const { return eliminateFixedSet_; }

	//! Turns the elimination of fixed and set variables on or off.
	/**
	 * \param turnOn The elimination is turned on if \a turnOn is \a true,
	 *               otherwise it is turned off.
	 */
	void eliminateFixedSet(bool turnOn) { eliminateFixedSet_ = turnOn; }

	/**
	 * \return true  Then a new root of the remaining branch-and-bound tree is reoptimized
	 *               such that the associated reduced costs can be used for the fixing of variables;
	 * \return false A new root is not reoptimized.
	 */
	bool newRootReOptimize() const { return newRootReOptimize_; }

	//! Turns the reoptimization of new root nodes of the remaining branch and bound tree on or off.
	/**
	 * \param on If \a true, new root nodes are reoptimized.
	 */
	void newRootReOptimize(bool on) { newRootReOptimize_ = on; }

	//! Returns the name of the file that stores the optimum solutions.
	const string &optimumFileName() const { return optimumFileName_; }

	//! Changes the name of the file in which the value of the optimum solution is searched.
	/**
	 * \param name The new name of the file.
	 */
	void optimumFileName(const char *name) { optimumFileName_ = name; }

	/**
	 * \return true  Then the average distance of the fractional solution
	 *               from all added cutting planes is output every iteration
	 *               of the subproblem optimization.
	 * \return false The average cut distance is not output.
	 */
	bool showAverageCutDistance() const { return showAverageCutDistance_; }

	//! Turns the output of the average distance of the added cuts from the fractional solution on or off.
	/**
	 * \param on If \a true the output is turned on, otherwise it is turned off.
	 */
	void showAverageCutDistance(bool on) { showAverageCutDistance_ = on; }

	//! Returns the mode of output for the Vbc-Tool.
	VBCMODE vbcLog() const { return VbcLog_; }

	//! Changes the mode of output for the Vbc-Tool to \a mode.
	/**
	 * This function should only be called before the optimization is
	 * started with the function Master::optimize().
	 *
	 * \param mode The new mode.
	 */
	void vbcLog(VBCMODE mode) { VbcLog_ = mode; }

	//! Sets solver specific parameters.
	/**
	 * The default does nothing.
	 *
	 * \return true if an error has occured, false otherwise.
	 */
	virtual bool setSolverParameters(OsiSolverInterface* interface, bool solverIsApprox);

protected:

	//! Sets up the default pools for variables, constraints, and cutting planes.
	/**
	 * \param constraints The constraints of the problem formulation
	 *                    are inserted in the constraint pool. The size
	 *                    of the constraint pool equals the number of
	 *                    \a constraints.
	 * \param variables The variables of the problem formulation are
	 *                  inserted in the variable pool.
	 * \param varPoolSize The size of the pool for the variables. If
	 *                    more variables are added the variable pool
	 *                    is automatically reallocated.
	 * \param cutPoolSize The size of the pool for cutting planes.
	 * \param dynamicCutPool If this argument is true, then the cut
	 *                       is automatically reallocated if more
	 *                       constraints are inserted than \a cutPoolSize.
	 *                       Otherwise, non-active constraints are removed
	 *                       if the pool becomes full.
	 *                       The default value is false.
	 */
	virtual void initializePools(
		ArrayBuffer<Constraint*> &constraints,
		ArrayBuffer<Variable*> &variables,
		int varPoolSize,
		int cutPoolSize,
		bool dynamicCutPool = false);

	//! Is overloaded such that also a first set of cutting planes can be inserted into the cutting plane pool.
	/**
	 * \param constraints The constraints of the problem formulation
	 *                    are inserted in the constraint pool. The size
	 *                    of the constraint pool equals the number of
	 *                    \a constraints.
	 * \param cuts The constraints that are inserted in the cutting
	 *             plane pool. The number of constraints in the buffer
	 *             must be less or equal than the size of the cutting
	 *             plane pool \a cutPoolSize.
	 * \param variables The variables of the problem formulation are
	 *                  inserted in the variable pool.
	 * \param varPoolSize The size of the pool for the variables. If
	 *                    more variables are added the variable pool
	 *                    is automatically reallocated.
	 * \param cutPoolSize The size of the pool for cutting planes.
	 * \param dynamicCutPool If this argument is true, then the cut
	 *                       is automatically reallocated if more
	 *                       constraints are inserted than \a cutPoolSize.
	 *                       Otherwise, non-active constraints are removed
	 *                       if the pool becomes full.
	 *                       The default value is false.
	 */
	virtual void initializePools(
		ArrayBuffer<Constraint*> &constraints,
		ArrayBuffer<Constraint*> &cuts,
		ArrayBuffer<Variable*> &variables,
		int varPoolSize,
		int cutPoolSize,
		bool dynamicCutPool = false);

	/**
	 * Can be used to initialize the sense
	 * of the optimization in derived classes, if this has not been already
	 * performed when the constructor of Master has been called.
	 *
	 * \param sense The sense of the optimization (OptSense::Min or OptSense::Max).
	 */
	void initializeOptSense(OptSense::SENSE sense) { optSense_.sense(sense); }

	//! Implements the best first search enumeration.
	/**
	 * If the bounds of both subproblems are equal, then
	 * the subproblems are compared with the function \a equalSubCompare().
	 *
	 * \return -1 If subproblem \a s1 has a worse dual bound than \a s2,
	 *            i.e., if it has a smaller dual bound for minimization or
	 *            a larger dual bound for maximization problems.
	 * \return 1  If subproblem \a s2 has a worse dual bound than \a s1.
	 * \return 0  If both subproblems have the same priority in the enumeration strategy.
	 *
	 * \param s1 A subproblem.
	 * \param s2 A subproblem.
	 */
	int bestFirstSearch(const Sub* s1, const Sub* s2) const;

	/**
	 * Is called from the function
	 * \a bestFirstSearch() and from the function \a depthFirstSearch()
	 * if the subproblems \a s1 and \a s2 have the same priority.
	 *
	 * If both subproblems were generated by setting a binary variable, then
	 * that subproblem has higher priority of which the branching variable is
	 * set to upper bound.
	 *
	 * This function can be redefined to resolve equal subproblems according
	 * to problem specific criteria.
	 * As the root node is compared with itself and has no branching rule,
	 * we have to insert the first line of this function.
	 *
	 * \param s1 A subproblem.
	 * \param s2 A subproblem.
	 *
	 * \return  0 If both subproblems were not generated by setting a
	 *            variable, or the branching variable of both subproblems
	 *            is set to the same bound.
	 * \return  1 If the branching variable of the first subproblem
	 *            is set to the upper bound.
	 * \return -1 If the branching variable of the second subproblem
	 *            is set to the upper bound.
	 */
	virtual int equalSubCompare(const Sub *s1, const Sub *s2) const;

	//! Implements the depth first search enumeration strategy, i.e., the subproblem with maximum \a level is selected.
	/**
	 * If the level of both subproblems are equal, then
	 * the subproblems are compared with the function \a equalSubCompare().
	 *
	 * \return -1 If subproblem \a s1 has higher priority,
	 * \return 0  if both subproblems have equal priority,
	 * \return 1  otherwise.
	 *
	 * \param s1 The first subproblem.
	 * \param s2 The second subproblem.
	 */
	int depthFirstSearch(const Sub* s1, const Sub* s2) const;

	//! Implements the breadth first search enumeration strategy, i.e., the subproblem with minimum \a level is selected.
	/**
	 * If both subproblems have the same \a level, the smaller
	 * one is the one which has been generated earlier, i.e., the one with
	 * the smaller \a id.
	 *
	 * \return -1 If subproblem \a s1 has higher priority,
	 * \return  0 if both subproblems have equal priority,
	 * \return  1 otherwise.
	 *
	 * \param s1 The first subproblem.
	 * \param s2 The second subproblem.
	 */
	int breadthFirstSearch(const Sub* s1, const Sub* s2) const;


	//! Performs depth-first search until a feasible solution is found, then the search process is continued with best-first search.
	/**
	 * \return -1 If subproblem \a s1 has higher priority,
	 * \return  0 if both subproblems have equal priority,
	 * \return  1 otherwise.
	 *
	 * \param s1 The first subproblem.
	 * \param s2 The second subproblem.
	 */
	int diveAndBestFirstSearch(const Sub *s1, const Sub* s2) const;

	/**
	 * Is only a dummy. This function can be used to initialize parameters of derived classes
	 * and to overwrite parameters read from the file <tt>.abacus</tt> by the
	 * function \a \_initializeParameters().
	 */
	virtual void initializeParameters() { }

	//! Should return a pointer to the first subproblem of the optimization, i.e., the root node of the enumeration tree.
	/**
	 * This is a pure virtual function since
	 * a pointer to a problem specific subproblem should be returned,
	 * which is derived from the class Sub.
	 */
	virtual Sub *firstSub() = 0;

	/**
	 * The default implementation of \a initializeOptimization() does nothing.
	 *
	 * This virtual function can be used as an entrance point to perform
	 * some initializations after \a optimize() is called.
	 */
	virtual void initializeOptimization() { }

	/**
	 * The default implementation of \a terminateOptimization() does nothing.
	 *
	 * This virtual function can be used as an entrance point
	 * after the optimization process is finished.
	 */
	virtual void terminateOptimization() { }

	//! Assigns the parameters that were read from a file to the member variables of the master.
	virtual void assignParameters();

private:

	//! \brief Reads the parameter-file <tt>.abacus</tt>.
	/**
	 * This file is searched in the directory given by the
	 * environment variable ABACUS_DIR, then the virtual function \a initializeParameters()
	 * is called which can initialize parameters of derived classes and overwrite
	 * parameters of this class.
	 *
	 * All parameters are first inserted together
	 * with their values in a parameter table in the function \a readParameters().
	 * If the virtual dummy function \a initializeParameters() is redefined
	 * in a derived class and also reads a parameter file with the function
	 * \a readParameters(), then already inserted parameters can be overwritten.
	 *
	 * After all parameters are input we extract with the function \a assignParameter()
	 * all parameters. Problem specific parameters should be extracted in
	 * a redefined version of \a initializeParameters().
	 * extracted from this table
	 */
	void _initializeParameters();

	void _createLpMasters();
	void _deleteLpMasters();
	void _initializeLpParameters();

	//! Initializes the LP solver specific default parameters if they are not read from <tt>.abacus</tt>.
	/**
	 * This function is implemented in the file \a lpif.cc.
	 */
	void _setDefaultLpParameters();

	//! \brief Prints the LP solver specific parameters.
	/**
	 * This function is implemented in the file \a lpif.cc.
	 */
	void _printLpParameters() const;

	//! \brief Prints the LP solver specific statistics.
	/**
	 * This function is implemented in the file \a lpif.cc.
	 */
	void _outputLpStatistics() const;

	//! \brief Returns a pointer to an open subproblem for further processing.
	/**
	 * If the set of open subproblems is empty or
	 * one of the criteria for early termination of the optimization
	 * (maximal cpu time, maximal elapsed time, guarantee) is
	 * fulfilled 0 is returned.
	 */
	Sub   *select();

	int initLP();

	//! Writes the string \a info to the stream associated with the Tree Interface.
	/**
	 * A \$ is preceded if
	 * the output is written to standard out for further pipelining.
	 * If \a time is true a time string is written in front of the
	 * information. The default value of \a time is \a true.
	 */
	void writeTreeInterface(const string &info, bool time = true) const;

	/**
	 * Adds the subproblem \a sub to the
	 * stream storing information for graphical output of the enumeration
	 * tree if this logging is turned on.
	 */
	void treeInterfaceNewNode(Sub *sub) const;

	//! Assigns the \a color to the subproblem \a sub in the Tree Interface.
	void treeInterfacePaintNode(int id, int color) const;

	//! Passes the new lower bound \a lb to the Tree Interface.
	void treeInterfaceLowerBound(double lb) const;

	//! Passes the new upper bound \a ub to the Tree Interface.
	void treeInterfaceUpperBound(double ub) const;

	//! Updates the node information in the node with number \a id by writing the lower bound \a lb and the upper bound \a ub to the node.
	void treeInterfaceNodeBounds(int id, double lb, double ub);

	//! Registers a new subproblem which is on level \a level in enumeration tree.
	/**
	 * It is called each time a new subproblem is generated.
	 */
	void newSub(int level);

	//! Increments the counter for linear programs and should be called in each optimization call of the LP-relaxation.
	void countLp() { ++nLp_; }

	//! Increments the counter of the number of fixed variables by \a n.
	void newFixed(int n) { nFixed_ += n; }

	//! Increments the counter for the total number of added constraints by \a n.
	void addCons(int n) { nAddCons_ += n; }

	//! Increments the counter for the total number of removed constraints by \a n.
	void removeCons(int n) { nRemCons_ += n; }

	//! Increments the counter for the total number of added variables by \a n.
	void addVars(int n) { nAddVars_ += n; }

	//! Increments the counter for the total number of removed variables by \a n.
	void removeVars(int n) { nRemVars_ += n; }

	//! Returns a pointer to the object storing the variables which are candidates for being fixed.
	FixCand *fixCand() const { return fixCand_; }

	//! Sets the root of the remaining branch-and-cut tree to \a newRoot.
	/**
	 * If \a reoptimize is \a true a reoptimization of the
	 * subproblem \a *newRoot is performed.
	 * This is controlled via a function argument since it might not be
	 * desirable when we find a new \a rRoot_ during the fathoming
	 * of a complete subtree Sub::FathomTheSubtree().
	 */
	void rRoot(Sub *newRoot, bool reoptimize);

	//! Sets the status of the Master.
	void status(STATUS stat) { status_ = stat; }

	//! Updates the final dual bound of the root node.
	/**
	 * This function should be only called at the end of the root node optimization.
	 */
	void rootDualBound(double x);

	void theFuture();

	//! The name of the optimized problem.
	string problemName_;

	bool readParamFromFile_;

	//! The sense of the objective function.
	OptSense optSense_;

	//! The root node of the enumeration tree.
	Sub *root_;

	//! The root node of the remaining enumeration tree.
	Sub *rRoot_;

	//! The set of open subproblems.
	OpenSub *openSub_;

	//! The solution history.
	History *history_;

	//! The enumeration strategy.
	ENUMSTRAT enumerationStrategy_;

	//! The branching strategy.
	BRANCHINGSTRAT branchingStrategy_;

	//! The number of candidates that are evaluated for branching on variables.
	int nBranchingVariableCandidates_;

	//! The number of simplex iterations that are performed when testing a branching variable candidate within strong branching.
	int nStrongBranchingIterations_;

	//! The default LP-Solver.
	OSISOLVER defaultLpSolver_;

	LpMasterOsi *lpMasterOsi_;

	//! The default pool with the constraints of the problem formulation.
	StandardPool<Constraint, Variable> *conPool_;


	//! The default pool of dynamically generated constraints.
	StandardPool<Constraint, Variable> *cutPool_;

	//! The default pool with the variables of the problem formulation.
	StandardPool<Variable, Constraint> *varPool_;

	//! The best known primal bound.
	double primalBound_;

	//! The best known dual bound.
	double dualBound_;

	//! The best known dual bound at the end of the optimization of the root node.
	double rootDualBound_;

	//! The variables which are candidates for being fixed.
	FixCand *fixCand_;

	//! If \a true, then constraints are generated in the optimization.
	bool cutting_;

	//! If \a true, then variables are generated in the optimization.
	bool pricing_;

	//! If \a true, then an approximative solver is used to solve linear programs
	bool solveApprox_;

	//! The number of subproblems already selected from the list of open subproblems.
	int nSubSelected_;

	//! Ouput for the Tree Interface is generated depending on the value of this variable.
	VBCMODE VbcLog_;

	//! A pointer to the log stream for the VBC-Tool.
	std::ostream *treeStream_;

	//! The guarantee in percent which should be reached when the optimization stops.
	/**
	 * If this value is 0.0, then the optimum solution is determined.
	 */
	double requiredGuarantee_;

	//! The maximal level in enumeration tree.
	/**
	 * Up to this level subproblems are considered in the enumeration.
	 */
	int maxLevel_;

	//! The maximal number of subproblems to be processed.
	/**
	 * Up to this number subproblems are considered in the enumeration.
	 */
	int maxNSub_;

	//! The maximal available cpu time.
	int64_t maxCpuTime_;

	//! The maximal available wall-clock time.
	int64_t maxCowTime_;

	//! \a true, if all objective function values of feasible solutions are assumed to be integer.
	bool objInteger_;

	//! The number of LP-iterations for the tailing off analysis.
	int tailOffNLp_;

	//! The minimal change of the LP-value on the tailing off analysis.
	double tailOffPercent_;

	//! The number of optimizations of an Sub until branching is performed.
	int dbThreshold_;

	/**
	 * The minimal number of rounds, i.e., number of subproblem optimizations,
	 * a subproblem is dormant, i.e., it is not selected from the set
	 * of open subproblem if its status is \a Dormant, if possible.
	 */
	int minDormantRounds_;

	//! The mode of the primal bound initialization.
	PRIMALBOUNDMODE pbMode_;

	//! The number of solved LPs between two additional pricing steps.
	int pricingFreq_;

	//! The frequency constraints or variables are generated depending on the skipping mode.
	int skipFactor_;

	/**
	 * Either constraints are generated only every \a skipFactor_ subproblem
	 * (\a SkipByNode) only every \a skipFactor_ level (\a SkipByLevel).
	 */
	SKIPPINGMODE skippingMode_;

	//! If \a true, then variables are fixed and set by reduced cost criteria.
	bool fixSetByRedCost_;

	//! If \a true, then the linear program is output every iteration.
	bool printLP_;

	//! The maximal number of added constraints per iteration of the cutting plane algorithm.
	int maxConAdd_;

	//! The size of the buffer for generated cutting planes.
	int maxConBuffered_;

	//! The maximal number of added variables per iteration of the column generation algorithm.
	int maxVarAdd_;

	//! The size of the buffer for generated variables.
	int maxVarBuffered_;

	//! The maximal number of iterations of the cutting plane/column generation algorithm in the subproblem.
	int maxIterations_;

	//! If \a true, then nonbasic fixed and set variables are eliminated.
	bool eliminateFixedSet_;

	/**
	 * If \a true, then an already earlier processed node is reoptimized
	 * if it becomes the new root of the remaining branch-and-bound tree.
	 */
	bool newRootReOptimize_;

	//! The name of a file storing a list of optimum solutions of problem instances.
	string optimumFileName_;

	/**
	 * If \a true then the average distance of the added cutting planes
	 * is output every iteration of the cutting plane algorithm.
	 */
	bool showAverageCutDistance_;

	//! The way constraints are automatically eliminated in the cutting plane algorithm.
	CONELIMMODE conElimMode_;

	//! The way variables are automatically eliminated in the column generation algorithm.
	VARELIMMODE varElimMode_;

	//! The tolerance for the elimination of constraints by the mode \a NonBinding/
	double conElimEps_;

	//! The tolerance for the elimination of variables by the mode \a ReducedCost.
	double varElimEps_;

	//! The number of iterations an elimination criterion must be satisfied until a constraint can be removed.
	int conElimAge_;

	//! The number of iterations an elimination criterion must be satisfied until a variable can be removed.
	int varElimAge_;

	//! The current status of the optimization.
	STATUS status_;

	//! The timer for the total elapsed time.
	ogdf::StopwatchWallClock totalCowTime_;

	//! The timer for the total cpu time for the optimization.
	ogdf::StopwatchCPU totalTime_;

	//! The timer for the cpu time spent in the LP-interface.
	ogdf::StopwatchCPU lpTime_;

	ogdf::StopwatchCPU lpSolverTime_;

	//! The timer for the cpu time spent in the separation
	ogdf::StopwatchCPU separationTime_;

	//! The timer for the cpu time spent in the heuristics for the computation of feasible solutions.
	ogdf::StopwatchCPU improveTime_;

	//! The timer for the cpu time spent in pricing.
	ogdf::StopwatchCPU pricingTime_;

	//! The timer for the cpu time spent in determining the branching rules.
	ogdf::StopwatchCPU branchingTime_;

	//! The number of generated subproblems.
	int nSub_;

	//! The number of solved LPs.
	int nLp_;

	//! The highest level which has been reached in the enumeration tree.
	int highestLevel_;

	//! The total number of fixed variables.
	int nFixed_;

	//! The total number of added constraints.
	int nAddCons_;

	//! The total number of removed constraints.
	int nRemCons_;

	//! The total number of added variables.
	int nAddVars_;

	//! The total number of removed variables.
	int nRemVars_;

	//! The number of changes of the root of the remaining branch-and-bound tree.
	int nNewRoot_;

	Master(const Master &rhs);
	const Master &operator=(const Master& rhs);
};


inline double Master::lowerBound() const
{
	if (optSense_.max()) return primalBound_;
	else                 return dualBound_;
}

inline double Master::upperBound() const
{
	if (optSense_.max()) return dualBound_;
	else                 return primalBound_;
}

}
