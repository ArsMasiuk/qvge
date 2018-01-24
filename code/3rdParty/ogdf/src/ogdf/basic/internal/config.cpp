/** \file
 * \brief Implementation of basic configuration utilities.
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

#include <ogdf/basic/internal/config.h>

namespace ogdf {

static string system_str[static_cast<int>(Configuration::System::STOP)+1] = {
	"unknown", "Windows", "Unix/linux", "Apple OSX", "STOP"
};

static string lpsolver_str[static_cast<int>(Configuration::LPSolver::STOP)+1] = {
	"N/A", "COIN-OR LP (Clp)", "Symphony", "CPLEX", "Gurobi", "STOP"
};

static string mm_str[static_cast<int>(Configuration::MemoryManager::STOP)+1] = {
	"pool allocator (thread-safe)", "pool allocator (not thread-safe)", "malloc", "STOP"
};


const string &Configuration::toString(System sys)
{
	return system_str[static_cast<int>(sys)];
}

const string &Configuration::toString(LPSolver lps)
{
	return lpsolver_str[static_cast<int>(lps)];
}

const string &Configuration::toString(MemoryManager mm)
{
	return mm_str[static_cast<int>(mm)];
}

}
