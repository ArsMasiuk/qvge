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
 */

#include <ogdf/lib/abacus/lpvarstat.h>
#include <ogdf/lib/abacus/global.h>

namespace abacus {


std::ostream &operator<<(std::ostream& out, const LPVARSTAT &rhs)
{
	switch (rhs.status_)
	{
	case LPVARSTAT::AtLowerBound:
		out << "AtLowerBound";
		break;
	case LPVARSTAT::Basic:
		out << "Basic";
		break;
	case LPVARSTAT::AtUpperBound:
		out << "AtUpperBound";
		break;
	case LPVARSTAT::NonBasicFree:
		out << "NonBasicFree";
		break;
	case LPVARSTAT::Eliminated:
		out << "Eliminated";
		break;
	case LPVARSTAT::Unknown:
		out << "Unknown";
		break;
	default:
		Logger::ifout() << "LPVARSTAT: unknonw status\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::LpVarStat);
	}
	return out;
}
}
