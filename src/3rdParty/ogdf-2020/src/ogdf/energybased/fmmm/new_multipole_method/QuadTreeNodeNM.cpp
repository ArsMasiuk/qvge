/** \file
 * \brief Implementation of class QuadTreeNodeNM.
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


#include <ogdf/energybased/fmmm/new_multipole_method/QuadTreeNodeNM.h>


namespace ogdf {
namespace energybased {
namespace fmmm {

using std::complex;

std::ostream &operator<< (std::ostream & output, const QuadTreeNodeNM & A)
{
	output <<" Sm_level: "<<A.Sm_level<<" Sm_downleftcorner: "<<A.Sm_downleftcorner
		<<" Sm boxlength: "<<A.Sm_boxlength<<" Sm_center: "<<A.Sm_center
		<<"spnumber: "<<A.subtreeparticlenumber;
	if(A.father_ptr == nullptr)
		output <<" is root ";
	if((A.child_lt_ptr == nullptr) ||(A.child_rt_ptr == nullptr) || (A.child_lb_ptr == nullptr)||
		(A.child_rb_ptr == nullptr))
	{
		output <<" (no child in ";
		if(A.child_lt_ptr == nullptr)
			output <<" lt";
		if(A.child_rt_ptr == nullptr)
			output <<" rt";
		if(A.child_lb_ptr == nullptr)
			output <<" lb";
		if(A.child_rb_ptr == nullptr)
			output <<" rb";
		output<<" quad) ";
	}

	output<<" L_x: ";
	if(A.L_x_ptr == nullptr)
		output<<"no list specified";
	else if(A.L_x_ptr->empty())
		output <<"is empty";
	else
	{
		for(const ParticleInfo &pi : *A.L_x_ptr)
			output << "  " << pi;
	}

	output<<" L_y: ";
	if(A.L_y_ptr == nullptr)
		output<<"no list specified";
	else if(A.L_y_ptr->empty())
		output <<"is empty";
	else
	{
		for(const ParticleInfo &pi : *A.L_y_ptr)
			output << "  " << pi;
	}

	output<<" I: ";
	if(A.I.empty())
		output <<"is empty";
	else
	{
		for(const QuadTreeNodeNM *v : A.I)
			output << " [" << v->get_Sm_level() << " , "
				<< v->get_Sm_downleftcorner() << ","
				<< v->get_Sm_boxlength() << "]";
	}

	output<<" D1: ";
	if(A.D1.empty())
		output <<"is empty";
	else
	{
		for(const QuadTreeNodeNM *v : A.D1)
			output << " [" << v->get_Sm_level() << " , "
				<< v->get_Sm_downleftcorner() << ","
				<< v->get_Sm_boxlength() << "]";
	}

	output<<" D2: ";
	if(A.D2.empty())
		output <<"is empty";
	else
	{
		for(const QuadTreeNodeNM *v : A.D2)
			output << " [" << v->get_Sm_level() << " , "
				<< v->get_Sm_downleftcorner() << ","
				<< v->get_Sm_boxlength() << "]";
	}

	output<<" M: ";
	if(A.M.empty())
		output <<"is empty";
	else
	{
		for(const QuadTreeNodeNM *v : A.M)
			output << " [" << v->get_Sm_level() << " , "
				<< v->get_Sm_downleftcorner() << ","
				<< v->get_Sm_boxlength() << "]";
	}
	output<<" contained_nodes ";
	if(A.contained_nodes.empty())
		output <<"is empty";
	else
	{
		for(node v : A.contained_nodes)
			output << v->index() << " ";
	}
	return output;
}


std::istream &operator>> (std::istream & input,  QuadTreeNodeNM & A)
{
	input >> A.Sm_level;
	return input;
}


QuadTreeNodeNM::QuadTreeNodeNM()
{
	DPoint double_null(0,0);
	complex<double> comp_null(0,0);

	L_x_ptr = nullptr; ;L_y_ptr = nullptr;
	subtreeparticlenumber = 0;
	Sm_level = 0;
	Sm_downleftcorner = double_null;
	Sm_boxlength = 0;
	Sm_center = comp_null;
	ME = nullptr;
	LE = nullptr;
	contained_nodes.clear();
	I.clear();D1.clear();D2.clear();M.clear();
	father_ptr = nullptr;
	child_lt_ptr = child_rt_ptr = child_lb_ptr = child_rb_ptr = nullptr;
}


QuadTreeNodeNM::~QuadTreeNodeNM()
{
	delete L_x_ptr;
	L_x_ptr = nullptr;
	delete L_y_ptr;
	L_y_ptr = nullptr;
	contained_nodes.clear();
	I.clear();D1.clear();D2.clear();M.clear();
	delete[] ME;
	delete[] LE;
}

}
}
}
