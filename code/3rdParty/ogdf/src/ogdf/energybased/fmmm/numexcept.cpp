/** \file
 * \brief Implementation of class numexcept (handling of numeric problems).
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


#include <ogdf/energybased/fmmm/numexcept.h>

#define epsilon 0.1
#define POS_SMALL_DOUBLE 1e-300
#define POS_BIG_DOUBLE   1e+300

namespace ogdf {
namespace energybased {
namespace fmmm {

DPoint numexcept::choose_distinct_random_point_in_disque(DPoint old_point,
	double xmin,double xmax,double ymin,double ymax)
{
	const int BILLION = 1000000000;
	double mindist;//minimal distance from old_point to the boundaries of the disc
	double mindist_to_xmin,mindist_to_xmax,mindist_to_ymin,mindist_to_ymax;
	double rand_x,rand_y;
	DPoint new_point;

	mindist_to_xmin = old_point.m_x - xmin;
	mindist_to_xmax = xmax -  old_point.m_x;
	mindist_to_ymin = old_point.m_y - ymin;
	mindist_to_ymax = ymax -  old_point.m_y;

	mindist = min(min(mindist_to_xmin,mindist_to_xmax), min(mindist_to_ymin,mindist_to_ymax));

	if (mindist > 0) {
		do {
			//assign random double values in range (-1,1)
			rand_x = 2*(double(randomNumber(1,BILLION)+1)/(BILLION+2)-0.5);
			rand_y = 2*(double(randomNumber(1,BILLION)+1)/(BILLION+2)-0.5);
			new_point.m_x = old_point.m_x+mindist*rand_x*epsilon;
			new_point.m_y = old_point.m_y+mindist*rand_y*epsilon;
		} while ((old_point == new_point)||((old_point-new_point).norm() >= mindist*epsilon));
	} else if (mindist == 0) { //old_point lies at the boundaries
		double mindist_x =0;
		double mindist_y =0;

		if (mindist_to_xmin > 0)
			mindist_x = (-1)* mindist_to_xmin;
		else if (mindist_to_xmax > 0)
			mindist_x = mindist_to_xmax;
		if (mindist_to_ymin > 0)
			mindist_y = (-1)* mindist_to_ymin;
		else if (mindist_to_ymax > 0)
			mindist_y = mindist_to_ymax;

		if (mindist_x != 0 || mindist_y != 0) {
			do {
				//assign random double values in range (0,1)
				rand_x = double(randomNumber(1,BILLION)+1)/(BILLION+2);
				rand_y = double(randomNumber(1,BILLION)+1)/(BILLION+2);
				new_point.m_x = old_point.m_x+mindist_x*rand_x*epsilon;
				new_point.m_y = old_point.m_y+mindist_y*rand_y*epsilon;
			} while (old_point == new_point);
		} else {
			std::cout<<"Error DIM2:: box is equal to old_pos"<<std::endl;
		}
	} else {
		std::cout<<"Error DIM2:: choose_distinct_random_point_in_disque: old_point not ";
		std::cout<<"in box"<<std::endl;
	}

	return new_point;
}

DPoint numexcept::choose_distinct_random_point_in_radius_epsilon(DPoint old_pos)
{
	double xmin = old_pos.m_x-1*epsilon;
	double xmax = old_pos.m_x+1*epsilon;
	double ymin = old_pos.m_y-1*epsilon;
	double ymax = old_pos.m_y+1*epsilon;

	return choose_distinct_random_point_in_disque(old_pos,xmin,xmax,ymin,ymax);
}

static double random_precision_number(double shift)
{
	const int BILLION = 1000000000;
	double rand = shift + double(randomNumber(1, BILLION) + 1) / (BILLION + 2);
	return randomNumber(0, 1) == 0 ? rand : -rand;
}

bool numexcept::f_rep_near_machine_precision(double distance, DPoint& force)
{
	const double POS_BIG_LIMIT = POS_BIG_DOUBLE * 1e-190;
	const double POS_SMALL_LIMIT = POS_SMALL_DOUBLE * 1e190;

	if (distance > POS_BIG_LIMIT) {
		force = DPoint(POS_SMALL_LIMIT * random_precision_number(1),
		               POS_SMALL_LIMIT * random_precision_number(1));
		return true;
	} else if (distance < POS_SMALL_LIMIT) {
		force = DPoint(POS_BIG_LIMIT * random_precision_number(0),
		               POS_BIG_LIMIT * random_precision_number(0));
		return true;
	}
	return false;
}

bool numexcept::f_near_machine_precision(double distance, DPoint& force)
{
	const double POS_BIG_LIMIT = POS_BIG_DOUBLE * 1e-190;
	const double POS_SMALL_LIMIT = POS_SMALL_DOUBLE * 1e190;

	if (distance < POS_SMALL_LIMIT) {
		force = DPoint(POS_SMALL_LIMIT * random_precision_number(1),
		               POS_SMALL_LIMIT * random_precision_number(1));
		return true;
	} else if (distance > POS_BIG_LIMIT) {
		force = DPoint(POS_BIG_LIMIT * random_precision_number(0),
		               POS_BIG_LIMIT * random_precision_number(0));
		return true;
	}
	return false;
}


bool numexcept::nearly_equal(double a,double b)
{
	double delta = 1e-10;
	double small_b,big_b;

	if(b > 0) {
		small_b = b*(1-delta);
		big_b   = b*(1+delta);

	} else //b <= 0
	{
		small_b = b*(1+delta);
		big_b   = b*(1-delta);
	}

	return (small_b <= a) && (a <= big_b);
}

DPoint numexcept::f_rep_u_on_v(DPoint pos_u, DPoint pos_v)
{
	if (pos_u == pos_v) {
		// Exception handling if two nodes have the same position
		pos_u = choose_distinct_random_point_in_radius_epsilon(pos_u);
	}
	DPoint vector_v_minus_u = pos_v - pos_u;
	double norm_v_minus_u = vector_v_minus_u.norm();
	DPoint f_rep_u_on_v;
	if (!f_rep_near_machine_precision(norm_v_minus_u, f_rep_u_on_v)) {
		double scalar = f_rep_scalar(norm_v_minus_u) / norm_v_minus_u;
		f_rep_u_on_v = scalar * vector_v_minus_u;
	}
	return f_rep_u_on_v;
}

}
}
}
