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

#include <ogdf/lib/abacus/csense.h>
#include <ogdf/lib/abacus/global.h>

namespace abacus {


CSense::CSense(char s)
{
	switch (s) {
	case 'l':
	case 'L':
		sense_ = Less;
		break;
	case 'e':
	case 'E':
		sense_ = Equal;
		break;
	case 'g':
	case 'G':
		sense_ = Greater;
		break;
	default:
		Logger::ifout() << "CSense::CSense(): unknown argument " << s << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Csense);
	}
}


std::ostream &operator<<(std::ostream &out, const CSense &rhs)
{
	switch (rhs.sense_) {
	case CSense::Less:
		out << "<=";
		break;
	case CSense::Equal:
		out << '=';
		break;
	case CSense::Greater:
		out << ">=";
		break;
	}
	return out;
}


void CSense::sense(char s)
{
	switch (s) {
	case 'l':
	case 'L':
		sense_ = Less;
		break;
	case 'e':
	case 'E':
		sense_ = Equal;
		break;
	case 'g':
	case 'G':
		sense_ = Greater;
		break;
	default:
		Logger::ifout() << "CSense::sense(): unknown argument " << s << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Csense);
		break;
	}
}
}
