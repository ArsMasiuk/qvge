/** \file
 * \brief Declaration of class PackingRowInfo.
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

#pragma once

#include <ogdf/basic/basic.h>

namespace ogdf {
namespace energybased {
namespace fmmm {

//! Helping data structure for MAARPacking.
class PackingRowInfo
{
	//! Outputstream for PackingRowInfo
	friend std::ostream &operator<< (std::ostream & output, const PackingRowInfo & A)
	{
		output <<" max_height "<<A.max_height<<" total_width "<<A.total_width<<" row_index "
			<< A.row_index;
		return output;
	}

	//! Inputstream for PackingRowInfo
	friend std::istream &operator>> (std::istream & input,  PackingRowInfo & A)
	{
		input >>A.max_height>>A.total_width>>A.row_index;
		return input;
	}

public:

	PackingRowInfo()      //!< constructor
	{
		total_width = 0;
		max_height = 0;
		row_index = 0;
	}

	void set_max_height(double h) { max_height = h; }
	void set_total_width(double w) { total_width = w; }
	void set_row_index(int i) { row_index = i; }

	double get_max_height() { return max_height; }
	double get_total_width() { return total_width; }
	int get_row_index() { return row_index; }

private:
	double max_height;  //!< the maximum height of a rectangle placed in this row
	double total_width; //!< the sum of the width of all rectsngles in this row
	int row_index;      //!< the index of the row (first row in packing has index 0)
};

}
}
}
