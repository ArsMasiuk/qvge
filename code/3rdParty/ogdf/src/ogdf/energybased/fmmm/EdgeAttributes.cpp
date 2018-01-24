/** \file
 * \brief Implementation of class EdgeAttributes.
 *
 * \author Stefan Hachul
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

#include <ogdf/energybased/fmmm/EdgeAttributes.h>

namespace ogdf {
namespace energybased {
namespace fmmm {

std::ostream &operator<< (std::ostream & output, const EdgeAttributes & A)
{
	output <<"length: "<< A.length;
	output<<"  index of original edge ";
	if (A.e_original == nullptr)
		output <<"nullptr";
	else output<<A.e_original->index();
	output<<"  index of subgraph edge ";
	if (A.e_subgraph == nullptr)
		output <<"nullptr";
	if (A.moon_edge)
		output<<" is moon edge ";
	else
		output <<" no moon edge ";
	if (A.extra_edge)
		output<<" is extra edge ";
	else
		output <<" no extra edge ";
	return output;
}


std::istream &operator>> (std::istream & input,  EdgeAttributes & A)
{
	input >> A.length;
	return input;
}


EdgeAttributes::EdgeAttributes()
{
	length = 0;
	e_original = nullptr;
	e_subgraph = nullptr;
	moon_edge = false;
	extra_edge = false;
}

}
}
}
