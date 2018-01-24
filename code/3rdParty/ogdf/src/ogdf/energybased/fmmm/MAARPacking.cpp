/** \file
 * \brief Declaration of class MAARPacking (used by FMMMLayout).
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

#include <ogdf/energybased/fmmm/MAARPacking.h>
#include <ogdf/energybased/fmmm/numexcept.h>

namespace ogdf {
namespace energybased {
namespace fmmm {

MAARPacking::MAARPacking()
{
	area_width = 0;
	area_height = 0;
}


MAARPacking::~MAARPacking() { }


void MAARPacking::pack_rectangles_using_Best_Fit_strategy(
	List<Rectangle>& R,
	double aspect_ratio,
	FMMMOptions::PreSort presort,
	FMMMOptions::TipOver allow_tipping_over,
	double& aspect_ratio_area,
	double& bounding_rectangles_area)
{
	List<PackingRowInfo> P; //represents the packing of the rectangles
	List<ListIterator <PackingRowInfo> > row_of_rectangle; //stores for each rectangle
	//r at pos. i in R the ListIterator of the row in P
	//where r is placed (at pos i in row_of_rectangle)
	List<ListIterator<Rectangle> > rectangle_order;//holds the order in which the
	//rectangles are touched
	PQueue total_width_of_row; //stores for each row the ListIterator of the corresp. list
	//in R and its total width

	switch (presort) {
	case FMMMOptions::PreSort::DecreasingWidth:
		presort_rectangles_by_width(R);
		break;
	case FMMMOptions::PreSort::DecreasingHeight:
		presort_rectangles_by_height(R);
		break;
	case FMMMOptions::PreSort::None:
		break;
	}

	//init rectangle_order
	ListIterator<Rectangle> rect_item;
	for(rect_item = R.begin(); rect_item.valid(); ++rect_item)
		rectangle_order.pushBack(rect_item);

	for (rect_item = R.begin(); rect_item.valid(); ++rect_item)
	{
		if (P.empty())
		{
			Rectangle r = *rect_item;
			double area;
			if (better_tipp_rectangle_in_new_row(r, aspect_ratio, allow_tipping_over, area))
				r = tipp_over(rect_item);
			B_F_insert_rectangle_in_new_row(r, P, row_of_rectangle, total_width_of_row);
			aspect_ratio_area = calculate_aspect_ratio_area(r.get_width(), r.get_height(),
				aspect_ratio);
		}
		else
		{
			ListIterator<PackingRowInfo> B_F_item =
				find_Best_Fit_insert_position(rect_item, allow_tipping_over, aspect_ratio, aspect_ratio_area, total_width_of_row);

			B_F_insert_rectangle(*rect_item, P, row_of_rectangle, B_F_item, total_width_of_row);
		}
	}
	export_new_rectangle_positions(P, row_of_rectangle, rectangle_order);
	bounding_rectangles_area = calculate_bounding_rectangles_area(R);
}


inline void MAARPacking::presort_rectangles_by_height(List<Rectangle>& R)
{
	R.quicksort(GenericComparer<Rectangle, double>([&](const Rectangle& x) { return -x.get_height(); }));
}


inline void MAARPacking::presort_rectangles_by_width(List<Rectangle>& R)
{
	R.quicksort(GenericComparer<Rectangle, double>([&](const Rectangle& x) { return -x.get_width(); }));
}


void MAARPacking::B_F_insert_rectangle_in_new_row(
	Rectangle r,
	List<PackingRowInfo>& P,
	List <ListIterator<PackingRowInfo> >&row_of_rectangle,
	PQueue& total_width_of_row)
{
	PackingRowInfo p;

	//create new empty row and insert r into this row of P
	p.set_max_height(r.get_height());
	p.set_total_width(r.get_width());
	p.set_row_index(P.size());
	P.pushBack(p);

	//remember in which row of P r is placed by updating row_of_rectangle
	row_of_rectangle.pushBack(P.rbegin());

	//update area_height,area_width
	Math::updateMax(area_width, r.get_width());
	area_height += r.get_height();


	//update total_width_of_row
	total_width_of_row.push(P.rbegin(), r.get_width());
}


ListIterator<PackingRowInfo> MAARPacking::find_Best_Fit_insert_position(
	ListIterator<Rectangle> rect_item,
	FMMMOptions::TipOver allow_tipping_over,
	double aspect_ratio,
	double& aspect_ratio_area,
	PQueue& total_width_of_row)
{
	double area_2;
	int best_try_index,index_2;
	Rectangle r = *rect_item;

	if(better_tipp_rectangle_in_new_row(r,aspect_ratio,allow_tipping_over,
		aspect_ratio_area))
		best_try_index = 2;
	else
		best_try_index = 1;

	ListIterator<PackingRowInfo> B_F_item = total_width_of_row.topElement();
	PackingRowInfo B_F_row = *B_F_item;
	if(better_tipp_rectangle_in_this_row(r,aspect_ratio,allow_tipping_over,B_F_row,area_2))
		index_2 = 4;
	else
		index_2 = 3;

	if((area_2 <= aspect_ratio_area) || numexcept::nearly_equal(aspect_ratio_area,area_2))
	{
		aspect_ratio_area = area_2;
		best_try_index = index_2;
	}

	//return the row and eventually tipp the rectangle with ListIterator rect_item
	if(best_try_index == 1)
		return nullptr;
	else if(best_try_index == 2)
	{
		tipp_over(rect_item);
		return nullptr;
	}
	else if(best_try_index == 3)
		return B_F_item;
	else //best_try_index == 4
	{
		tipp_over(rect_item);
		return B_F_item;
	}
}


void MAARPacking::B_F_insert_rectangle(
	Rectangle r,
	List<PackingRowInfo>& P,
	List<ListIterator<PackingRowInfo> >&row_of_rectangle,
	ListIterator<PackingRowInfo> B_F_item,
	PQueue& total_width_of_row)
{
	ListIterator<PackingRowInfo> null = nullptr;
	if (B_F_item == null) //insert into a new row
		B_F_insert_rectangle_in_new_row(r,P,row_of_rectangle,total_width_of_row);
	else //insert into an existing row
	{
		double old_max_height;

		//update P[B_F_item]
		PackingRowInfo p = *B_F_item;
		old_max_height = p.get_max_height();
		p.set_max_height(max(old_max_height,r.get_height()));
		p.set_total_width(p.get_total_width()+r.get_width());
		*B_F_item = p;

		//updating row_of_rectangle
		row_of_rectangle.pushBack(B_F_item);

		//update area_height,area_width
		Math::updateMax(area_width, p.get_total_width());
		Math::updateMax(area_height, area_height - old_max_height + r.get_height());

		//update total_width_of_row

		total_width_of_row.pop();
		total_width_of_row.push(B_F_item, p.get_total_width());

	}
}



void MAARPacking::export_new_rectangle_positions(
	List<PackingRowInfo>& P,
	List<ListIterator<PackingRowInfo> >& row_of_rectangle,
	List<ListIterator<Rectangle> >& rectangle_order)
{
	int i;
	Rectangle r;
	PackingRowInfo p, p_pred;
	DPoint new_dlc_pos;
	Array<double> row_y_min(P.size()); //stores the min. y-coordinates for each row in P
	Array<double> act_row_x_max(P.size()); //stores the actual rightmost x-coordinate
	//for each row in P
#if 0
	ListIterator< ListIterator<PackingRowInfo> > row_item;
#endif
	ListIterator<PackingRowInfo>  row_item;
	ListIterator<ListIterator<PackingRowInfo> > Rrow_item;

	//init act_row_x_max
	for (i = 0; i < P.size(); i++)
		act_row_x_max[i] = 0;

	//calculate minimum heights of each row
	for (row_item = P.begin(); row_item.valid(); ++row_item)
	{
		if (row_item == P.begin())
			row_y_min[0] = 0;
		else
		{
			p = *row_item;
			p_pred = *(P.cyclicPred(row_item));
			row_y_min[p.get_row_index()] = row_y_min[p.get_row_index() - 1] +
				p_pred.get_max_height();
		}
	}

	//calculate for each rectangle its new down left corner coordinate
	Rrow_item = row_of_rectangle.begin();

	for (ListIterator<Rectangle> R_item : rectangle_order)
	{
		r = *R_item;
		row_item = *Rrow_item;
		p = *row_item;
		double new_x = act_row_x_max[p.get_row_index()];
		act_row_x_max[p.get_row_index()] += r.get_width();
		double new_y = row_y_min[p.get_row_index()] + (p.get_max_height() - r.get_height()) / 2;

		new_dlc_pos.m_x = new_x;
		new_dlc_pos.m_y = new_y;
		r.set_new_dlc_position(new_dlc_pos);
		*R_item = r;

		if (Rrow_item != row_of_rectangle.rbegin())
			Rrow_item = row_of_rectangle.cyclicSucc(Rrow_item);
	}
}


inline double MAARPacking::calculate_bounding_rectangles_area(List<Rectangle>& R)
{
	double area = 0;

	for(const Rectangle &r : R)
		area += r.get_width() * r.get_height();

	return area;
}


inline double MAARPacking::calculate_aspect_ratio_area(
	double width,
	double height,
	double aspect_ratio)
{
	double ratio = width/height;

	if(ratio < aspect_ratio) //scale width
		return width * height * (aspect_ratio/ratio);
	else //scale height
		return width * height * (ratio/aspect_ratio);
}


bool MAARPacking::better_tipp_rectangle_in_new_row(
	Rectangle r,
	double aspect_ratio,
	FMMMOptions::TipOver allow_tipping_over,
	double& best_area)
{
	bool rotate = false;

	//first try: new row insert position
	double width = max(area_width, r.get_width());
	double height = area_height + r.get_height();
	best_area = calculate_aspect_ratio_area(width, height, aspect_ratio);
	double act_area;

	//second try: new row insert position with tipping r over
	switch (allow_tipping_over) {
	case FMMMOptions::TipOver::NoGrowingRow:
	case FMMMOptions::TipOver::Always:
		width = max(area_width, r.get_height());
		height = area_height + r.get_width();
		act_area = calculate_aspect_ratio_area(width, height, aspect_ratio);
		if (act_area < 0.99999 * best_area) {
			best_area = act_area;
			rotate = true;
		}
	case FMMMOptions::TipOver::None:
		break;
	}
	return rotate;
}


bool MAARPacking::better_tipp_rectangle_in_this_row(
	Rectangle r,
	double aspect_ratio,
	FMMMOptions::TipOver allow_tipping_over,
	PackingRowInfo B_F_row,
	double& best_area)
{
	bool rotate = false;

	//first try: BEST_FIT insert position
	double width = max(area_width, B_F_row.get_total_width() + r.get_width());
	double height = max(area_height, area_height - B_F_row.get_max_height() + r.get_height());
	best_area = calculate_aspect_ratio_area(width, height, aspect_ratio);
	double act_area;

	//second try: BEST_FIT insert position  with skipping r over
	switch (allow_tipping_over) {
	case FMMMOptions::TipOver::NoGrowingRow:
		if (r.get_width() > B_F_row.get_max_height()) {
			break;
		}
		OGDF_CASE_FALLTHROUGH;
	case FMMMOptions::TipOver::Always:
		width = max(area_width, B_F_row.get_total_width() + r.get_height());
		height = max(area_height, area_height - B_F_row.get_max_height() + r.get_width());
		act_area = calculate_aspect_ratio_area(width, height, aspect_ratio);
		if (act_area < 0.99999 * best_area) {
			best_area = act_area;
			rotate = true;
		}
	case FMMMOptions::TipOver::None:
		break;
	}
	return rotate;
}


inline Rectangle MAARPacking::tipp_over(ListIterator<Rectangle> rect_item)
{
	Rectangle r = *rect_item;
	Rectangle r_tipped_over = r;
	DPoint tipped_dlc;

	if(!r.is_tipped_over())
	{//tipp old_dlc over
		tipped_dlc.m_x = r.get_old_dlc_position().m_y*(-1)-r.get_height();
		tipped_dlc.m_y = r.get_old_dlc_position().m_x;
	}
	else
	{//tipp old_dlc back;
		tipped_dlc.m_x = r.get_old_dlc_position().m_y;
		tipped_dlc.m_y = r.get_old_dlc_position().m_x*(-1)-r.get_width();
	}
	r_tipped_over.set_old_dlc_position(tipped_dlc);
	r_tipped_over.set_width(r.get_height());
	r_tipped_over.set_height(r.get_width());
	r_tipped_over.tipp_over();
	*rect_item = r_tipped_over;

	return r_tipped_over;
}

}
}
}
