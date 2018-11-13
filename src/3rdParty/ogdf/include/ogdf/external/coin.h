/** \file
 * \brief Handles connection to the COIN library, by offering
 * helper classes.
 *
 * If you use Coin, you need to include this file.
 *
 * \todo Currently, there is only a single implementation of the
 * CoinCallback-class declared herein (necc. for userdefined cuts).
 * This implementation is CPLEX specific.
 * -- with current coin, it might not even work anymore!
 *
 * \author Markus Chimani
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

#pragma once

#include <ogdf/basic/basic.h>

#include <coin/OsiSolverInterface.hpp>
#include <coin/CoinPackedVector.hpp>

namespace ogdf {

class OGDF_EXPORT CoinCallbacks {
	friend class OGDF_EXPORT CoinManager;
public:
	enum class CallbackType { Cut = 1, Heuristic = 2, Incumbent = 4, Branch  = 8 };
	enum class CutReturn { Error, SolutionValid, AddCuts, DontAddCuts, NoCutsFound };
	enum class HeuristicReturn { Error, Ignore, Update };
	enum class IncumbentReturn { Error, Ignore, Update };
#if 0
	enum class BranchReturn { Error, ... };
#endif
	virtual CutReturn cutCallback(const double /* objValue */, const double* /* fracSolution */, OsiCuts* /* addThese */) { return CutReturn::Error; }
	virtual HeuristicReturn heuristicCallback(double& /* objValue */, double* /* solution */) { return HeuristicReturn::Error; }
	virtual IncumbentReturn incumbentCallback(const double /* objValue */, const double* /* solution */) { return IncumbentReturn::Error; }
#if 0
	virtual BranchReturn branchCallback() { return BR_Error; };
#endif

	virtual ~CoinCallbacks() {}
private:
	bool registerCallbacks(OsiSolverInterface* _posi, int callbackTypes);
};

class OGDF_EXPORT CoinManager {
public:
	static OsiSolverInterface* createCorrectOsiSolverInterface();
	static OsiSolverInterface* createCorrectOsiSolverInterface(CoinCallbacks* ccc, int callbackTypes) {
		OsiSolverInterface* posi = createCorrectOsiSolverInterface();
		if(ccc->registerCallbacks(posi, callbackTypes))
			return posi;
		delete posi;
		return nullptr;
	}
	static void logging(OsiSolverInterface* osi, bool logMe);
};

}
