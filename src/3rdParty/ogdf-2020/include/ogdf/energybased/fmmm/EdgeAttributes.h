/** \file
 * \brief Declaration of class EdgeAttributes.
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

#include <ogdf/basic/geometry.h>
#include <ogdf/basic/Graph.h>

namespace ogdf {
namespace energybased {
namespace fmmm {

//! helping data structure that stores the graphical attributes of an edge
//! that are needed for the force-directed  algorithms.
class OGDF_EXPORT EdgeAttributes
{
	//! outputstream for EdgeAttributes
	friend OGDF_EXPORT std::ostream &operator<< (std::ostream &, const EdgeAttributes &);

	//! inputstream for EdgeAttributes
	friend OGDF_EXPORT std::istream &operator>> (std::istream &, EdgeAttributes &);

public:
	//! Constructor
	EdgeAttributes();

	void set_EdgeAttributes(double len, edge e_orig, edge e_sub)
	{
		length = len;
		e_original = e_orig;
		e_subgraph = e_sub;
	}

	void set_length(double len) { length = len; }
	double get_length() const { return length; }

	//! \name for the divide et impera step in FMMM @{

	void set_original_edge (edge e) { e_original = e; }
	void set_subgraph_edge (edge e) { e_subgraph = e; }
	edge get_original_edge() const { return e_original; }
	edge get_subgraph_edge() const { return e_subgraph; }

	//! @}
	//! \name for the preprocessing step in FMMM (set/get_original_edge are needed, too) @{

	void set_copy_edge (edge e) {e_subgraph = e;}
	edge get_copy_edge() const {return e_subgraph;}

	//! @}
	//! \name for multilevel step @{

	void set_higher_level_edge (edge e) { e_subgraph = e; }
	edge get_higher_level_edge() const { return e_subgraph; }
	bool is_moon_edge() const { return moon_edge; }
	void make_moon_edge() { moon_edge = true; }
	bool is_extra_edge() const { return extra_edge; }
	void make_extra_edge() { extra_edge = true; }
	void mark_as_normal_edge() { extra_edge = false; }
	void init_mult_values() { e_subgraph = nullptr; moon_edge = false; }

	//! @}

private:
	double length;
	edge e_original;
	edge e_subgraph;

	//! indicates if this edge is associasted with a moon node
	bool moon_edge;

	//! indicates if this edge is an extra edge that is added to
	//! enforce few edge crossings
	bool extra_edge;
};

}
}
}
