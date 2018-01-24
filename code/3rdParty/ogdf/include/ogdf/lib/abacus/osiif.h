/*!\file
 * \author Frank Baumann
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
#include <ogdf/lib/abacus/optsense.h>
#include <ogdf/lib/abacus/lpvarstat.h>
#include <ogdf/lib/abacus/slackstat.h>
#include <ogdf/lib/abacus/csense.h>
#include <ogdf/lib/abacus/osiinclude.h>
#include <coin/CoinPackedMatrix.hpp>
#include <coin/CoinPackedVector.hpp>
#include <coin/CoinWarmStartBasis.hpp>
#include <coin/CoinBuild.hpp>

namespace abacus {

class LpMasterOsi;


class  OsiIF :  public virtual LP  {
public:

	//! Constructor without initialization.
	/**
	 * This constructor does not initialize the problem data of the linear
	 * program. It must be loaded later with the function \a initialize().
	 *
	 * \param master A pointer to the corresponding master of the optimization.
	 */
	OsiIF(Master *master);

	//! A constructor with initialization.
	/**
	 * \param master A pointer to the corresponding master of the optimization.
	 * \param sense  The sense of the objective function.
	 * \param nCol   The number of columns (variables).
	 * \param maxCol The maximal number of columns.
	 * \param nRow   The number of rows.
	 * \param maxRow The maximal number of rows.
	 * \param obj    An array with the objective function coefficients.
	 * \param lb     An array with the lower bounds of the columns.
	 * \param ub     An array with the upper bounds of the columns.
	 * \param rows   An array storing the rows of the problem.
	 */
	OsiIF(Master *master,
		OptSense sense,
		int nRow,
		int maxRow,
		int nCol,
		int maxCol,
		Array<double> &obj,
		Array<double> &lb,
		Array<double> &ub,
		Array<Row*> &rows);

	//! The destructor.
	virtual ~OsiIF();

	//! The enumeration of possible solver types.
	enum SOLVERTYPE { Exact, Approx };

	SOLVERTYPE currentSolverType() const { return currentSolverType_; }

	OsiSolverInterface* osiLP() { return osiLP_; }


private:

	//! Pointer to the Osi solver interface.
	/**
	 * It is later typecast to a pointer to an object of a solver specific derived class.
	 * See osiif.cc, OsiIF::_initialize.
	 */
	OsiSolverInterface *osiLP_;

	/***************************************************
	 * Helper functions to free allocated memory
	 **************************************************/

	void freeDouble(const double* &ptr) {
		delete [] ptr;
		ptr = nullptr;
	}

	void freeDouble(double* &ptr) {
		delete [] ptr;
		ptr = nullptr;
	}

	void freeInt(int* &ptr) {
		delete [] ptr;
		ptr = nullptr;
	}

	void freeChar(char* &ptr) {
		delete [] ptr;
		ptr = nullptr;
	}

	void freeChar(const char* &ptr) {
		delete [] ptr;
		ptr = nullptr;
	}

	void freeStatus(CoinWarmStartBasis::Status*&);

	//! Loads the linear program defined by the following arguments to the solver.
	/**
	 * \param sense  The sense of the objective function.
	 * \param nRow   The number of rows.
	 * \param maxRow The maximal number of rows.
	 * \param nCol   The number of columns (variables).
	 * \param maxCol The maximal number of columns.
	 * \param obj    An array with the objective function coefficients.
	 * \param lBound An array with the lower bounds of the columns.
	 * \param uBound An array with the upper bounds of the columns.
	 * \param rows   An array storing the rows of the problem.
	 */
	virtual void _initialize(
		OptSense sense,
		int nRow, int maxRow,
		int nCol, int maxCol,
		Array<double> &obj,
		Array<double> &lBound,
		Array<double> &uBound,
		Array<Row*> &rows) override;

	//! Loads a basis to the solver
	/**
	 * \param lpVarStat An array storing the status of the columns.
	 * \param slackStat An array storing the status of the slack variables.
	 */
	virtual void _loadBasis(Array<LPVARSTAT::STATUS> &lpVarStat,
		Array<SlackStat::STATUS> &slackStat) override;

	//! Returns the sense of the optimization.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual OptSense _sense() const override;

	//! Changes the sense of the optimization to \a newSense.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual void _sense(const OptSense &newSense) override;

	//! Returns the number of rows of the linear program.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual int  _nRow()  const override { return numRows_; }

	//! Returns the maximal number of rows of the linear program.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual int  _maxRow() const override {
		return numRows_;  // Size management is completely done by Osi!
	}

	//! Returns the number of columns of the linear program.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual int  _nCol() const override { return numCols_; }

	//! Returns the maximal number of columns of the linear program.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual int  _maxCol() const override {
		return numCols_;  // Size management is completely done by Osi!
	}

	//! Returns the objective function coefficient of column \a i.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual double _obj(int i) const override { return objcoeff_[i]; }

	//! Returns the lower bound of column \a i.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual double _lBound(int i) const override { return collower_[i]; }

	//! Returns the upper bound of column \a i.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual double _uBound(int i) const override { return colupper_[i]; }

	//! Returns the right hand side of row \a i.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual double _rhs(int i) const override { return rhs_[i]; }

	//! Stores a copy of row \a i in \a r.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual void _row(int i, Row &r) const override;

	//! Returns the number of nonzero elements in the constraint matrix (not including the right hand side).
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual int _nnz() const override { return osiLP_->getNumElements(); }

	//! Calls the primal simplex method.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual OPTSTAT _primalSimplex() override;

	//! Calls the dual simplex method.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual OPTSTAT _dualSimplex() override;

	//! Calls the barrier method.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual OPTSTAT _barrier(bool doCrossover) override;

	//! Calls an approximate method.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual OPTSTAT _approx() override;

	//! Returns the optimum value of the linear program.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual double _value() const override { return value_; }

	//! Returns the value of the column \a i.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual double _xVal(int i) const override { return xVal_[i]; }

	//! Returns the value of the column \a i.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual double _barXVal(int i) const override;

	//! Returns the reduced cost of the column \a i.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual double _reco(int i) const override { return reco_[i]; }

	//! Returns the value of the slack column of the row \a i.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual double _slack(int i) const override;

	//! Returns the value of the dual column of the row \a i.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual double _yVal(int i) const override { return yVal_[i]; }

	//! Returns the status of the column \a i.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual LPVARSTAT::STATUS _lpVarStat(int i) const override;

	//! Returns the status of the slack column \a i.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual SlackStat::STATUS _slackStat(int i) const override;

	//! Can be called if the last linear program has been solved with the dual simplex method and is infeasible.
	/**
	 * This function is currently not supported by the interface.
	 *
	 * In this case it computes the infeasible basic variable or constraint
	 * and the corresponding row \a nInvRow of the basis inverse.
	 * Either \a infeasRow or \a infeasCol is nonnegative. Then this number
	 * refers to an infeasible variable or slack variable, respectively.
	 * The function returns 0 if it is successful, 1 otherwise.
	 *
	 * Currently this featureis not supported by the Open Solver Interface,
	 * therefore a call to this function always returns an error status.
	 *
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual int  _getInfeas(int &infeasRow, int &infeasCol, double *bInvRow) const override;

	//! Removes the rows listed in \a ind.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual void _remRows(ArrayBuffer<int> &ind) override;

	//! Adds the \a rows to the linear program.
	/**
	 *  It implements the pure virtual function of the base class LP.
	 */
	virtual void _addRows(ArrayBuffer<Row*> &newRows) override ;

	//! Removes the columns listed in \a vars.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual void _remCols(ArrayBuffer<int> &vars) override;

	//! Adds the columns \a newCols to the linear program.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual void _addCols(ArrayBuffer<Column*> &newVars) override;

	//! Sets the right hand side of the linear program to \a newRhs.
	/**
	 * This array must have at least length of the number of rows.
	 * This function implements the pure virtual function of the base class LP.
	 */
	virtual void _changeRhs(Array<double> &newRhs) override;

	//! Sets the lower bound of column \a i to \a newLb.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual void _changeLBound(int i, double newLb) override;

	//! Sets the upper bound of column \a i to \a newLb.
	/**
	 * It implements the pure virtual function of the base class LP.
	 */
	virtual void _changeUBound(int i, double newUb) override;

	//! Pivots the slack variables stored in the buffer \a rows into the basis.
	/**
	 * This function defines the pure virtual function of the base class LP.
	 * This function is currently not supported by the interface.
	 *
	 * \return 0 All variables could be pivoted in, 1 otherwise.
	 *
	 * \param rows The numbers of the slack variables that should be pivoted in.
	 */
	virtual int _pivotSlackVariableIn(ArrayBuffer<int> &rows) override;

	//! Extracts the solution.
	/**
	 * I.e., the value, the status, the values of the variables, slack variables, and dual
	 * variables, the reduced costs, and the statuses of the variables
	 * and slack variables form the internal solver data structure.
	 */
	void getSol();

	//! Converts the ABACUS representation of the row sense to the Osi representation.
	char csense2osi(CSense *sense) const;

	//! Converts the OSI representation of the row sense to the ABACUS representation.
	CSense::SENSE osi2csense(char sense) const;

	//! Converts the ABACUS variable status to OSI format.
	CoinWarmStartBasis::Status lpVarStat2osi(LPVARSTAT::STATUS stat) const;

	//! Converts the OSI variable status to ABACUS format.
	LPVARSTAT::STATUS osi2lpVarStat(CoinWarmStartBasis::Status stat) const;

	//! Converts the ABACUS slack status to OSI format.
	CoinWarmStartBasis::Status slackStat2osi(SlackStat::STATUS stat) const;

	//! Converts the OSI slack status to ABACUS format.
	SlackStat::STATUS osi2slackStat(CoinWarmStartBasis::Status stat) const;

	//! Allocates an Open Solver Interface of type defaultOsiSolver.
	OsiSolverInterface* getDefaultInterface();

	//! Switches between exact and approximate solvers.
	OsiSolverInterface* switchInterfaces(SOLVERTYPE newMethod);

	//! Initializes the problem with a dummy row.
	/**
	 * To be used with CPLEX if there are no rows.
	 */
	void loadDummyRow(OsiSolverInterface* s2, const double* lbounds, const double* ubounds, const double* objectives);

	//! Reallocates the internal memory such that newSize rows can be stored.
	 /**
	 * This function is obsolete, as memory management is completely handled by Osi.
	 *
	 * It implements the corresponding pure virtual function of the base class LP.
	 * If a reallocation is performed in the base class LP, we
	 * reinitialize the internal data structure. Actually this
	 * reinitialization is redundant since it would be performed
	 * automatically if \a addRows() or \a addCols() fail. However,
	 * to be consistent, and if a reallocation is performed to
	 * decrease the size of the arrays we call \a reinitialize().
	 */
	void _rowRealloc(int newSize) override;

	//! Reallocates the internal memory such that \a newSize columns can be stored.
	/**
	 * This function is obsolete, as memory management is completely handled by Osi.
	 *
	 * It implements the corresponding pure virtual function of the base
	 * class LP.
	 */
	void _colRealloc(int newSize) override;

	//! Changes the iteration limit of the Simplex algorithm.
	/**
	 * This function defines a pure virtual function of the base class LP.
	 *
	 * \param limit The new value of the iteration limit.
	 *
	 * \return 0 If the iteration limit could be set, 1 otherwise.
	 */
	virtual int _setSimplexIterationLimit(int limit) override {
		return(!osiLP_->setIntParam(OsiMaxNumIteration, limit));
	}

	//! Defines a pure virtual function of the base class LP.
	/***
	 *  \param limit Stores the iteration limit if the return value is 0.
	 *
	 *  \return 0 If the iteration limit could be retrieved, \return 1 otherwise.
	 */
	virtual int _getSimplexIterationLimit(int &limit) const override {
		return(!osiLP_->getIntParam(OsiMaxNumIteration, limit));
	}

	void convertSenseToBound(
		double inf,
		const char sense,
		const double right,
		const double range,
		double& lower,
		double& upper) const
	{
		switch (sense) {
		case 'E':
			lower = upper = right;
			break;
		case 'L':
			lower = -inf;
			upper = right;
			break;
		case 'G':
			lower = right;
			upper = inf;
			break;
		case 'R':
			lower = right - range;
			upper = right;
			break;
		case 'N':
			lower = -inf;
			upper = inf;
			break;
		}
	}

	LpMasterOsi *lpMasterOsi_;

	//! The value of the optimal solution.
	double  value_;

	//! An array storing the values of the variables after the linear program has been optimized.
	double *xVal_;

	const double *barXVal_;

	//! An array storing the values of the reduced costs after the linear program has been optimized.
	double *reco_;

	//! An array storing the values of the dual variables after the linear program has been optimized.
	double *yVal_;

	//! An array storing the statuses of the variables after the linear program has been optimized.
	char *cStat_;

	//! The number of columns currently used in the LP.
	int numCols_;

	//! The number of rows currently used in the LP.
	int numRows_;

	//! An array storing the statuses of the slack variables after the linear program has been optimized.
	char *rStat_;

	//! An array storing the right hand sides of the linear program.
	const double *rhs_;

	//! An array storing the row activity of the linear program.
	double *rowactivity_;

	//! An array storing the row senses of the linear program.
	const char *rowsense_;

	//! An array storing the column upper bounds of the linear program.
	const double *colupper_;

	//! An array storing the column lower bounds of the linear program.
	const double *collower_;

	//! An array storing the objective function coefficients of the linear program.
	const double *objcoeff_;

	//! A warm start object storing information about a basis of the linear program.
	CoinWarmStartBasis *ws_;

	//! The type of the current solver interface.
	SOLVERTYPE currentSolverType_;

	OsiIF(const OsiIF &rhs);
	const OsiIF &operator=(const OsiIF &rhs);
};

}
