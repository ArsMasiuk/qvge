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

#include <ogdf/lib/abacus/fsvarstat.h>
#include <ogdf/lib/abacus/global.h>

namespace abacus {


std::ostream &operator<<(std::ostream& out, const FSVarStat &rhs)
{
	switch (rhs.status_)
	{
	case FSVarStat::Free:
		out << "Free";
		break;
	case FSVarStat::SetToLowerBound:
		out << "SetToLowerBound";
		break;
	case FSVarStat::Set:
		out << "Set to " << rhs.value_;
		break;
	case FSVarStat::SetToUpperBound:
		out << "SetToUpperBound";
		break;
	case FSVarStat::FixedToLowerBound:
		out << "FixedToLowerBound";
		break;
	case FSVarStat::Fixed:
		out << "Fixed to "<< rhs.value_;
		break;
	case FSVarStat::FixedToUpperBound:
		out << "FixedToUpperBound";
		break;
	default:
		Logger::ifout() << "FSVarStat: unknonw status\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::FsVarStat);
	}
	return out;
}


bool FSVarStat::fixed() const
{
	switch (status_)
	{
	case FixedToLowerBound:
	case FixedToUpperBound:
	case Fixed:
		return true;
	default:
		return false;
	}
}


bool FSVarStat::set() const
{
	switch (status_)
	{
	case SetToLowerBound:
	case SetToUpperBound:
	case Set:
		return true;
	default:
		return false;
	}
}


bool FSVarStat::contradiction(FSVarStat *fsVarStat) const
{
	STATUS status = fsVarStat->status();

	switch (status)
	{
	case Set:
	case Fixed:
		return contradiction(status, fsVarStat->value());
	default:
		return contradiction(status);
	}
}


bool FSVarStat::contradiction(STATUS status, double value) const
{
	switch (status_)
	{
	case SetToLowerBound:
	case FixedToLowerBound:
		switch (status) {
		case SetToUpperBound:
		case FixedToUpperBound:
		case Set:
		case Fixed:
			return true;
		default:
			return false;
		}

	case SetToUpperBound:
	case FixedToUpperBound:
		switch (status) {
		case SetToLowerBound:
		case FixedToLowerBound:
		case Set:
		case Fixed:
			return true;
		default:
			return false;
		}

	case Fixed:
	case Set:
		switch (status) {
		case Fixed:
		case Set:
			if (glob_->equal(value_, value))
				return true;
			else
				return false;
		default:
			return false;
		}

	default:
		return false;
	}
}
}
