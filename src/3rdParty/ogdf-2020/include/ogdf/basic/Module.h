/** \file
 * \brief Declares base class for all module types.
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

#pragma once

#include <ogdf/basic/basic.h>

namespace ogdf {


/**
 * \brief Base class for modules.
 *
 * A module represents an algorithm that implements a certain interface.
 * There are various specific module types present in the OGDF, which all
 * inherit Module as a base class. These module types define the interface
 * implemented by the module.
 */
class OGDF_EXPORT Module
{
public:
	//! The return type of a module.
	enum class ReturnType {
		Feasible, //!< The solution is feasible.
		Optimal, //!< The solution is optimal
		NoFeasibleSolution, //!< There exists no feasible solution.
		TimeoutFeasible, //!< The solution is feasible, but there was a timeout.
		TimeoutInfeasible, //!< The solution is not feasible due to a timeout.
		Error //!< Computation was aborted due to an error.
	};

	//! Initializes a module.
	Module() { }

	virtual ~Module() { }

	//! Returns true iff \p ret indicates that the module returned a feasible solution.
	static bool isSolution(ReturnType ret) {
		return ret == ReturnType::Feasible || ret == ReturnType::Optimal || ret == ReturnType::TimeoutFeasible;
	}
};

OGDF_EXPORT std::ostream & operator<<(std::ostream &os, const Module::ReturnType &r);

}
