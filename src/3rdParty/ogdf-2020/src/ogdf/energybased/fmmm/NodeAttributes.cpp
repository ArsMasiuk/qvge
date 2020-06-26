/** \file
 * \brief Implementation of class NodeAttributes.
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

#include <ogdf/energybased/fmmm/NodeAttributes.h>

namespace ogdf {
namespace energybased {
namespace fmmm {

std::ostream &operator<< (std::ostream & output, const NodeAttributes & A)
{
	output <<"width: "<< A.width<<" height: "<<A.height<<" position: "<<A.position ;
	output<<" index of lower level node ";
	if (A.v_lower_level == nullptr)
		output <<"nullptr";
	else output<<A.v_lower_level->index();
	output<<" index of higher level node ";
	if (A.v_higher_level == nullptr)
		output <<"nullptr";
	else output<<A.v_higher_level->index();
	output<<" mass "<<A.mass<<" type "<<A.type;
	if(A.type == 3)
	{
		output<<" dedic_moon_nodes ";
		if(A.moon_List.empty())
			output<<" is empty";
		else {
			for(node v : A.moon_List) {
				output<<v->index()<<" ";
			}
		}
	}
	if(A.type == 4)
		output<<" dedic_pm_node "<<A.dedicated_pm_node;
	output<<" index of dedicated sun_node ";
	if (A.get_dedicated_sun_node() == nullptr)
		output<<"nullptr";
	else
		output<<A.dedicated_sun_node->index();
	output<<" distance to dedicated sun "<<A.dedicated_sun_distance;
	output<<" lambda_List ";
	if(A.lambda.empty())
		output<<" is empty";
	else {
		for(double lambda : A.lambda) {
			output << lambda << " ";
		}
	}
	output<<" neighbour_sun_node_List ";
	if(A.neighbour_s_node.empty())
		output<<" is empty";
	else
		for(node v : A.neighbour_s_node)
			output<<v->index()<<" ";
	if(A.placed)
		output<<" is placed";
	else
		output<<" is not placed";
	std::cout<<" angle_1 "<<A.angle_1<<" angle_2 "<<A.angle_2<<std::endl;
	return output;
}


std::istream &operator>> (std::istream & input,  NodeAttributes & /* A */)
{
	//input >> A.l;
	return input;
}


void NodeAttributes::init_mult_values()
{
	type = 0;
	dedicated_sun_node = nullptr;
	dedicated_sun_distance = 0;
	dedicated_pm_node = nullptr;
	lambda.clear();
	neighbour_s_node.clear();
	lambda_List_ptr = &lambda;
	neighbour_s_node_List_ptr = &neighbour_s_node;
	moon_List.clear();
	moon_List_ptr = &moon_List;
	placed = false;
	angle_1 = 0;
	angle_2 = 2.0 * Math::pi;
}


NodeAttributes::NodeAttributes()
{
	position.m_x = 0;
	position.m_y = 0;
	width = 0;
	height = 0;
	v_lower_level = nullptr;
	v_higher_level = nullptr;

	//for multilevel step
	mass = 0;
	init_mult_values();
}

}
}
}
