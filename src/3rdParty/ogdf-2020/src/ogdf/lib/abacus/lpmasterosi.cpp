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

#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/lpmasterosi.h>
#include <ogdf/lib/abacus/osiif.h>

namespace abacus {


LpMasterOsi::~LpMasterOsi()
{
#ifdef OSI_CPLEX
#ifdef OSI_CPLEX_HACK
	//close the CPLEX environment that was opened in
	//initializeLpParameters() and decrement the OSI
	//instance counter, if necessary
	if( master_->defaultLpSolver() == Master::CPLEX ) {
		if( OsiCpxSolverInterface::getNumInstances() > 0 )
			OsiCpxSolverInterface::decrementInstanceCounter();
	}
#endif
#endif //OSI_CPLEX
}


void LpMasterOsi::initializeLpParameters()
{
#ifdef OSI_CPLEX
#ifdef OSI_CPLEX_HACK
	//get a CPLEX license and prevent OSI from getting
	//a new one for each LP
	//FIXME CPLEX environment
	if( master_->defaultLpSolver() == Master::CPLEX ) {
		int err = 0;
		if( OsiCpxSolverInterface::getNumInstances() < 1 )
			OsiCpxSolverInterface::incrementInstanceCounter();
	}
#endif
#endif //OSI_CPLEX
}
}
