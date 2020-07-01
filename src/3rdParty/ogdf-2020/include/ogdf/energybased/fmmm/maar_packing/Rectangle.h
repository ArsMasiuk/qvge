/** \file
 * \brief Declaration of class Rectangle.
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
#include <iostream>

namespace ogdf {
namespace energybased {
namespace fmmm {

//! Helping data structure for packing rectangles; The width, height and the position
//! of the down left corner of the tight surroundig rectangle is represented for each
//! connected component of the graph.
class Rectangle
{
	//! Outputstream for Rectangle.
	friend std::ostream &operator<< (std::ostream & output, const Rectangle & A)
	{
		output <<"width: "<< A.width<<" height: "<<A.height<<" old dlc_position: "
			<<A.old_down_left_corner_position<<" new dlc_position: "
			<<A.new_down_left_corner_position<<" coponenet_index: "<<A.component_index;
		if (A.tipped_over) {
			output << " is tipped_over";
		}
		return output;
	}

	//! Inputstream for Rectangle.
	friend std::istream &operator>> (std::istream & input,  Rectangle & A)
	{
		input >>A.width;
		return input;
	}

public:

	Rectangle() //!< constructor
	{
		old_down_left_corner_position.m_x = 0;
		old_down_left_corner_position.m_y = 0;
		new_down_left_corner_position.m_x = 0;
		new_down_left_corner_position.m_y = 0;
		width = 0;
		height = 0;
		component_index = -1;
		tipped_over = false;
	}

	void set_rectangle (double w, double h, double old_dlc_x_pos,double
		old_dlc_y_pos,int comp_index)
	{
		width = w;
		height = h;
		old_down_left_corner_position.m_x = old_dlc_x_pos;
		old_down_left_corner_position.m_y = old_dlc_y_pos;
		component_index = comp_index;
		tipped_over = false;
	}

	void set_old_dlc_position(DPoint dlc_pos){old_down_left_corner_position = dlc_pos;}
	void set_new_dlc_position(DPoint dlc_pos){new_down_left_corner_position = dlc_pos;}
	void set_width(double w) {width = w;}
	void set_height(double h) {height = h;}
	void set_component_index (int comp_index) {component_index = comp_index;}
	void tipp_over() { tipped_over = !tipped_over; }

	DPoint get_old_dlc_position() const { return old_down_left_corner_position; }
	DPoint get_new_dlc_position() const { return new_down_left_corner_position; }
	double get_width() const {return width;}
	double get_height() const {return height;}
	int get_component_index() const {return component_index;}
	bool is_tipped_over() const {return tipped_over;}

private:
	DPoint old_down_left_corner_position;//!< down left corner of the tight surround. rect.
	DPoint new_down_left_corner_position;//!< new calculated down left corner of ...
	double width;                     //!< width of the surround. rect.
	double height;                    //!< height of the surround. rect.
	int component_index;  //!< the index of the related connected component
	bool tipped_over;     //!< indicates if this rectangle has been tipped over in the
	//! packing step

};

}
}
}
