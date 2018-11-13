/** \file
 * \brief Implementation of std::ostream operator for ReturnType
 *
 * \author Ivo Hedtke
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

#include <ogdf/basic/Module.h>

namespace ogdf {

std::ostream & operator<<(std::ostream &os, const Module::ReturnType &r) {
	switch (r) {
		case Module::ReturnType::Feasible:           os << "Feasible";           break;
		case Module::ReturnType::Optimal:            os << "Optimal";            break;
		case Module::ReturnType::NoFeasibleSolution: os << "NoFeasibleSolution"; break;
		case Module::ReturnType::TimeoutFeasible:    os << "TimeoutFeasible";    break;
		case Module::ReturnType::TimeoutInfeasible:  os << "TimeoutInfeasible";  break;
		case Module::ReturnType::Error:              os << "Error";              break;
	}
	return os;
}

}
