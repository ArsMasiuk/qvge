/** \file
 * \brief Implements front-end for LP solver
 *
 * \author Carsten Gutwenger
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */


#include <ogdf/basic/basic.h>

#include <ogdf/lpsolver/LPSolver.h>

namespace ogdf {

LPSolver::LPSolver()
{
	osi = CoinManager::createCorrectOsiSolverInterface();
}


double LPSolver::infinity() const
{
	return osi->getInfinity();
}

bool LPSolver::checkFeasibility(
	const Array<int>    &matrixBegin,   // matrixBegin[i] = begin of column i
	const Array<int>    &matrixCount,   // matrixCount[i] = number of nonzeroes in column i
	const Array<int>    &matrixIndex,   // matrixIndex[n] = index of matrixValue[n] in its column
	const Array<double> &matrixValue,	  // matrixValue[n] = non-zero value in matrix
	const Array<double> &rightHandSide, // right-hand side of LP constraints
	const Array<char>   &equationSense, // 'E' ==   'G' >=   'L' <=
	const Array<double> &lowerBound,    // lower bound of x[i]
	const Array<double> &upperBound,    // upper bound of x[i]
	const Array<double> &x              // x-vector of optimal solution (if result is Optimal)
) const
{
	const int numRows = rightHandSide.size();
	const int numCols = x.size();

	double eps;
	osi->getDblParam(OsiPrimalTolerance, eps);

	for(int i = 0; i < numCols; ++i) {
		if(x[i]+eps < lowerBound[i] || x[i]-eps > upperBound[i]) {
			std::cerr << "column " << i << " out of range" << std::endl;
			return false;
		}
	}

	for(int i = 0; i < numRows; ++i) {
		double leftHandSide = 0.0;

		for(int c = 0; c < numCols; ++c) {
			for(int j = matrixBegin[c]; j < matrixBegin[c]+matrixCount[c]; ++j)
				if(matrixIndex[j] == i) {
					leftHandSide += matrixValue[j] * x[c];
				}
		}

		switch(equationSense[i]) {
			case 'G':
				if(leftHandSide+eps < rightHandSide[i]) {
					std::cerr << "row " << i << " violated " << std::endl;
					std::cerr << leftHandSide << " > " << rightHandSide[i] << std::endl;
					return false;
				}
				break;
			case 'L':
				if(leftHandSide-eps > rightHandSide[i]) {
					std::cerr << "row " << i << " violated " << std::endl;
					std::cerr << leftHandSide << " < " << rightHandSide[i] << std::endl;
					return false;
				}
				break;
			case 'E':
				if(leftHandSide+eps < rightHandSide[i] || leftHandSide-eps > rightHandSide[i]) {
					std::cerr << "row " << i << " violated " << std::endl;
					std::cerr << leftHandSide << " = " << rightHandSide[i] << std::endl;
					return false;
				}
				break;
			default:
				std::cerr << "unexpected equation sense " << equationSense[i] << std::endl;
				return false;
		}
	}
	return true;
}

LPSolver::Status LPSolver::optimize(
	OptimizationGoal goal,        // goal of optimization (minimize or maximize)
	Array<double> &obj,           // objective function vector
	Array<int>    &matrixBegin,   // matrixBegin[i] = begin of column i
	Array<int>    &matrixCount,   // matrixCount[i] = number of nonzeroes in column i
	Array<int>    &matrixIndex,   // matrixIndex[n] = index of matrixValue[n] in its column
	Array<double> &matrixValue,	  // matrixValue[n] = non-zero value in matrix
	Array<double> &rightHandSide, // right-hand side of LP constraints
	Array<char>   &equationSense, // 'E' ==   'G' >=   'L' <=
	Array<double> &lowerBound,    // lower bound of x[i]
	Array<double> &upperBound,    // upper bound of x[i]
	double &optimum,              // optimum value of objective function (if result is Optimal)
	Array<double> &x              // x-vector of optimal solution (if result is Optimal)
)
{
	if(osi->getNumCols()>0) { // get a fresh one if necessary
		delete osi;
		osi = CoinManager::createCorrectOsiSolverInterface();
	}

	const int numRows = rightHandSide.size();
	const int numCols = obj.size();
#ifdef OGDF_DEBUG
	const int numNonzeroes = matrixIndex.size();
#endif

	// assert correctness of array boundaries
	OGDF_ASSERT(obj          .low()  == 0);
	OGDF_ASSERT(obj          .size() == numCols);
	OGDF_ASSERT(matrixBegin  .low()  == 0);
	OGDF_ASSERT(matrixBegin  .size() == numCols);
	OGDF_ASSERT(matrixCount  .low()  == 0);
	OGDF_ASSERT(matrixCount  .size() == numCols);
	OGDF_ASSERT(matrixIndex  .low()  == 0);
	OGDF_ASSERT(matrixIndex  .size() == numNonzeroes);
	OGDF_ASSERT(matrixValue  .low()  == 0);
	OGDF_ASSERT(matrixValue  .size() == numNonzeroes);
	OGDF_ASSERT(rightHandSide.low()  == 0);
	OGDF_ASSERT(rightHandSide.size() == numRows);
	OGDF_ASSERT(equationSense.low()  == 0);
	OGDF_ASSERT(equationSense.size() == numRows);
	OGDF_ASSERT(lowerBound   .low()  == 0);
	OGDF_ASSERT(lowerBound   .size() == numCols);
	OGDF_ASSERT(upperBound   .low()  == 0);
	OGDF_ASSERT(upperBound   .size() == numCols);
	OGDF_ASSERT(x            .low()  == 0);
	OGDF_ASSERT(x            .size() == numCols);

	osi->setObjSense(goal==OptimizationGoal::Minimize ? 1 : -1);

	int i;

	CoinPackedVector zero;
	for(i = 0; i < numRows; ++i) {
		osi->addRow(zero,equationSense[i],rightHandSide[i],0);
	}
	for(int colNo = 0; colNo < numCols; ++colNo) {
		CoinPackedVector cpv;
		for(i = matrixBegin[colNo]; i<matrixBegin[colNo]+matrixCount[colNo]; ++i) {
			cpv.insert(matrixIndex[i],matrixValue[i]);
		}
		osi->addCol(cpv,lowerBound[colNo],upperBound[colNo],obj[colNo]);
	}


	osi->initialSolve();

	Status status;
	if(osi->isProvenOptimal()) {
		optimum = osi->getObjValue();
		const double* sol = osi->getColSolution();
		for(i = numCols; i-- > 0;)
			x[i]=sol[i];
		status = Status::Optimal;
		OGDF_HEAVY_ASSERT(checkFeasibility(matrixBegin,matrixCount,matrixIndex,matrixValue,
			rightHandSide,equationSense,lowerBound,upperBound,x));

	} else if(osi->isProvenPrimalInfeasible())
		status = Status::Infeasible;
	else if(osi->isProvenDualInfeasible())
		status = Status::Unbounded;
	else
		OGDF_THROW_PARAM(AlgorithmFailureException, AlgorithmFailureCode::NoSolutionFound);

	return status;
}

}
