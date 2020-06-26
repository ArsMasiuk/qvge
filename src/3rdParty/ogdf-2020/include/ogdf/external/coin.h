/** \file
 * \brief Definition of ogdf::CoinManager
 *
 * \author Markus Chimani, Stephan Beyer
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

#include <ogdf/basic/Logger.h>
#include <ogdf/lib/abacus/osiinclude.h>

#include <coin/CoinPackedVector.hpp> // not used here but always necessary when using COIN

namespace ogdf {

//! If you use COIN-OR, you should use this class
class OGDF_EXPORT CoinManager {
public:
	//! Get a new solver
	static OsiSolverInterface* createCorrectOsiSolverInterface() {
#ifdef COIN_OSI_CPX
		OsiCpxSolverInterface *ret = new OsiCpxSolverInterface(); // CPLEX
#elif defined(COIN_OSI_GRB)
		OsiGrbSolverInterface *ret = new OsiGrbSolverInterface(); // Gurobi
#elif defined(COIN_OSI_SYM)
		OsiSymSolverInterface *ret = new OsiSymSolverInterface(); // Symphony
		ret->setSymParam(OsiSymVerbosity, -2);
#else // COIN_OSI_CLP
		OsiClpSolverInterface *ret = new OsiClpSolverInterface(); // Coin-OR LP
#endif
		logging(ret, !Logger::globalStatisticMode() && Logger::globalLogLevel() <= Logger::Level::Minor);
		return ret;
	}

	//! Enable or disable logging for the given solver interface
	static void logging(OsiSolverInterface* osi, bool logMe) {
		osi->messageHandler()->setLogLevel(logMe ? 1 : 0);
	}
};

}
