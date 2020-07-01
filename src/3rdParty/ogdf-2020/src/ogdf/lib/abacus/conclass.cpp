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

#include <ogdf/lib/abacus/conclass.h>

namespace abacus {


std::ostream &operator<<(std::ostream &out, const ConClass &rhs)
{
	bool classified = false;

	if (rhs.discrete_) {
		out << "discrete/";
		classified = true;
	}
	if (rhs.allVarBinary_) {
		out << "allVarBinary/";
		classified = true;
	}
	if (rhs.trivial_) {
		out << "trivial/";
		classified = true;
	}
	if (rhs.bound_) {
		out << "bound/";
		classified = true;
	}
	if (rhs.varBound_) {
		out << "variable bound/";
		classified = true;
	}

	if (!classified) out << "no classification ";

	return out;
}
}
