/*!\file
* \author Matthias Elf
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
*
* \par
*   The file \a lpif.cc contains all code fragments of the basic
*   \ABACUS\ library which depend on the preprocessor definition of the
*   supported LP solvers. Only this file needs to be recompiled
*   if the LP solver definitions change.
*
* \par
*   Currently the following definitions are supported:
*
* \par
*   For OSI:
*   ABACUS_LP_OSI.
*
* $Id: lpif.cc,v 2.9 2009-05-13 14:11:06 baumann Exp $
*/

#include <ogdf/lib/abacus/master.h>
#include <ogdf/lib/abacus/sub.h>

#define ABACUS_LP_OSI
#ifdef ABACUS_LP_OSI
#include <ogdf/lib/abacus/lpmasterosi.h>
#include <ogdf/lib/abacus/lpsubosi.h>
#endif

namespace abacus {

//! The function Sub::generateLp().

LpSub *Sub::generateLp()
{
	switch (master_->defaultLpSolver()) {
#ifdef ABACUS_LP_OSI
	case Master::Cbc:
	case Master::Clp:
	case Master::CPLEX:
	case Master::DyLP:
	case Master::FortMP:
	case Master::GLPK:
	case Master::MOSEK:
	case Master::OSL:
	case Master::SoPlex:
	case Master::SYMPHONY:
	case Master::XPRESS_MP:
	case Master::Gurobi:
	case Master::Csdp:
		return new LpSubOsi(master_, this);
#endif
	default:
		Logger::ifout() << "Error: ABACUS library not compiled for\nselected LP-Solver " << Master::OSISOLVER_[master_->defaultLpSolver()] << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::LpIf);
	}
}


//! The function Master::_createLpMasters().

void Master::_createLpMasters()
{
#ifdef ABACUS_LP_OSI
	lpMasterOsi_ = new LpMasterOsi(this);
#endif
}

//! The function Master::_deleteLpMasters().

void Master::_deleteLpMasters()
{
#ifdef ABACUS_LP_OSI
	delete lpMasterOsi_;
#endif
}

//! The function Master::_initializeLpParameters().

void Master::_initializeLpParameters()
{
#ifdef ABACUS_LP_OSI
	lpMasterOsi_->initializeLpParameters();
#endif
}

//! The function Master::_setDefaultLpParameters().

void Master::_setDefaultLpParameters()
{
#ifdef ABACUS_LP_OSI
	lpMasterOsi_->setDefaultLpParameters();
#endif
}

//! The function Master::_printLpParameters().

void Master::_printLpParameters() const
{
#ifdef ABACUS_LP_OSI
	lpMasterOsi_->printLpParameters();
#endif
}

//! The function Master::_outputLpStatistics().

void Master::_outputLpStatistics() const
{
#ifdef ABACUS_LP_OSI
	lpMasterOsi_->outputLpStatistics();
#endif
}
}
