/** \file
 * \brief Implementation of class QuadTreeNM.
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


#include <ogdf/energybased/fmmm/new_multipole_method/QuadTreeNM.h>

namespace ogdf {
namespace energybased {
namespace fmmm {

using std::complex;

QuadTreeNM::QuadTreeNM()
{
	root_ptr = act_ptr =nullptr;
}


void QuadTreeNM::create_new_lt_child(
	List<ParticleInfo>* L_x_ptr,
	List<ParticleInfo>* L_y_ptr)
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y+act_ptr->get_Sm_boxlength()/2;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_x_List_ptr(L_x_ptr);
	new_ptr->set_y_List_ptr(L_y_ptr);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_lt_ptr(new_ptr);
}


void QuadTreeNM::create_new_lt_child()
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y+act_ptr->get_Sm_boxlength()/2;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_lt_ptr(new_ptr);
}


void QuadTreeNM::create_new_rt_child(
	List<ParticleInfo>* L_x_ptr,
	List<ParticleInfo>* L_y_ptr)
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x+act_ptr->get_Sm_boxlength()/2;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y+act_ptr->get_Sm_boxlength()/2;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_x_List_ptr(L_x_ptr);
	new_ptr->set_y_List_ptr(L_y_ptr);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_rt_ptr(new_ptr);
}


void QuadTreeNM::create_new_rt_child()
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x+act_ptr->get_Sm_boxlength()/2;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y+act_ptr->get_Sm_boxlength()/2;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_rt_ptr(new_ptr);
}


void QuadTreeNM::create_new_lb_child(
	List<ParticleInfo>* L_x_ptr,
	List<ParticleInfo>* L_y_ptr)
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_x_List_ptr(L_x_ptr);
	new_ptr->set_y_List_ptr(L_y_ptr);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_lb_ptr(new_ptr);
}


void QuadTreeNM::create_new_lb_child()
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_lb_ptr(new_ptr);
}


void QuadTreeNM::create_new_rb_child(
	List<ParticleInfo>* L_x_ptr,
	List<ParticleInfo>* L_y_ptr)
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x+act_ptr->get_Sm_boxlength()/2;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_x_List_ptr(L_x_ptr);
	new_ptr->set_y_List_ptr(L_y_ptr);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_rb_ptr(new_ptr);
}


void QuadTreeNM::create_new_rb_child()
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x+act_ptr->get_Sm_boxlength()/2;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_rb_ptr(new_ptr);
}


void QuadTreeNM::delete_tree(QuadTreeNodeNM* node_ptr)
{
	if(node_ptr != nullptr)
	{
		if(node_ptr->get_child_lt_ptr() != nullptr)
			delete_tree(node_ptr->get_child_lt_ptr());
		if(node_ptr->get_child_rt_ptr() != nullptr)
			delete_tree(node_ptr->get_child_rt_ptr());
		if(node_ptr->get_child_lb_ptr() != nullptr)
			delete_tree(node_ptr->get_child_lb_ptr());
		if(node_ptr->get_child_rb_ptr() != nullptr)
			delete_tree(node_ptr->get_child_rb_ptr());
		delete node_ptr;
		if (node_ptr == root_ptr)
			root_ptr = nullptr;
	}
}


void QuadTreeNM::delete_tree_and_count_nodes(QuadTreeNodeNM* node_ptr, int& nodecounter)
{
	if(node_ptr != nullptr)
	{
		nodecounter++;
		if(node_ptr->get_child_lt_ptr() != nullptr)
			delete_tree_and_count_nodes(node_ptr->get_child_lt_ptr(),nodecounter);
		if(node_ptr->get_child_rt_ptr() != nullptr)
			delete_tree_and_count_nodes(node_ptr->get_child_rt_ptr(),nodecounter);
		if(node_ptr->get_child_lb_ptr() != nullptr)
			delete_tree_and_count_nodes(node_ptr->get_child_lb_ptr(),nodecounter);
		if(node_ptr->get_child_rb_ptr() != nullptr)
			delete_tree_and_count_nodes(node_ptr->get_child_rb_ptr(),nodecounter);
		delete node_ptr;
		if (node_ptr == root_ptr)
			root_ptr = nullptr;
	}
}


void QuadTreeNM::cout_preorder(QuadTreeNodeNM* node_ptr)
{
	if(node_ptr != nullptr)
	{
		std::cout<< *node_ptr <<std::endl;
		if(node_ptr->get_child_lt_ptr() != nullptr)
			cout_preorder(node_ptr->get_child_lt_ptr());
		if(node_ptr->get_child_rt_ptr() != nullptr)
			cout_preorder(node_ptr->get_child_rt_ptr());
		if(node_ptr->get_child_lb_ptr() != nullptr)
			cout_preorder(node_ptr->get_child_lb_ptr());
		if(node_ptr->get_child_rb_ptr() != nullptr)
			cout_preorder(node_ptr->get_child_rb_ptr());
	}
}


void QuadTreeNM::cout_preorder(QuadTreeNodeNM* node_ptr, int precision)
{
	if (node_ptr != nullptr)
	{
		complex<double>* L = node_ptr->get_local_exp();
		complex<double>* M = node_ptr->get_multipole_exp();
		std::cout << *node_ptr << std::endl;
		std::cout << " ME: ";
		for (int i = 0; i <= precision; i++)
			std::cout << M[i] << " ";
		std::cout << std::endl;
		std::cout << " LE: ";
		for (int i = 0; i <= precision; i++)
			std::cout << L[i] << " ";
		std::cout << std::endl << std::endl;

		if (node_ptr->get_child_lt_ptr() != nullptr)
			cout_preorder(node_ptr->get_child_lt_ptr(), precision);
		if (node_ptr->get_child_rt_ptr() != nullptr)
			cout_preorder(node_ptr->get_child_rt_ptr(), precision);
		if (node_ptr->get_child_lb_ptr() != nullptr)
			cout_preorder(node_ptr->get_child_lb_ptr(), precision);
		if (node_ptr->get_child_rb_ptr() != nullptr)
			cout_preorder(node_ptr->get_child_rb_ptr(), precision);
	}
}

}
}
}
