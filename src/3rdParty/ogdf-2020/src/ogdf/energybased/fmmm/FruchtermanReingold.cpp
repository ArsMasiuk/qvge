/** \file
 * \brief Implementation of class FruchtermanReingold (computation of forces).
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

#include <ogdf/energybased/fmmm/FruchtermanReingold.h>
#include <ogdf/energybased/fmmm/common.h>
#include <ogdf/basic/Array2D.h>

namespace ogdf {
namespace energybased {
namespace fmmm {

FruchtermanReingold::FruchtermanReingold()
{
	grid_quotient(2);
}


void FruchtermanReingold::calculate_exact_repulsive_forces(
	const Graph &G,
	NodeArray<NodeAttributes> &A,
	NodeArray<DPoint>& F_rep)
{
	//naive algorithm by Fruchterman & Reingold
	DPoint nullpoint(0, 0);
	int node_number = G.numberOfNodes();
	Array<node> array_of_the_nodes(node_number + 1);

	for (node v : G.nodes)
		F_rep[v] = nullpoint;

	int counter = 1;
	for (node v : G.nodes)
	{
		array_of_the_nodes[counter] = v;
		counter++;
	}

	for (int i = 1; i < node_number; i++) {
		for (int j = i + 1; j <= node_number; j++)
		{
			node u = array_of_the_nodes[i];
			node v = array_of_the_nodes[j];
			DPoint f_rep_u_on_v = numexcept::f_rep_u_on_v(A[u].get_position(), A[v].get_position());
			F_rep[v] += f_rep_u_on_v;
			F_rep[u] -= f_rep_u_on_v;
		}
	}
}


void FruchtermanReingold::calculate_approx_repulsive_forces(
	const Graph &G,
	NodeArray<NodeAttributes> &A,
	NodeArray<DPoint>& F_rep)
{
	//GRID algorithm by Fruchterman & Reingold
	List<IPoint> neighbour_boxes;
	List<node> neighbour_box;
	IPoint neighbour;
	DPoint nullpoint(0, 0);

	double gridboxlength;//length of a box in the GRID

	//init F_rep
	for (node v : G.nodes)
		F_rep[v] = nullpoint;

	//init max_gridindex and set contained_nodes

	max_gridindex = static_cast<int> (sqrt(double(G.numberOfNodes())) / grid_quotient()) - 1;
	max_gridindex = ((max_gridindex > 0) ? max_gridindex : 0);
	Array2D<List<node> >  contained_nodes(0, max_gridindex, 0, max_gridindex);

	for (int i = 0; i <= max_gridindex; i++) {
		for (int j = 0; j <= max_gridindex; j++)
		{
			contained_nodes(i, j).clear();
		}
	}

	gridboxlength = boxlength / (max_gridindex + 1);
	for (node v : G.nodes)
	{
		double x = A[v].get_x() - down_left_corner.m_x;//shift comput. box to nullpoint
		double y = A[v].get_y() - down_left_corner.m_y;
		int x_index = static_cast<int>(x / gridboxlength);
		int y_index = static_cast<int>(y / gridboxlength);
		contained_nodes(x_index, y_index).pushBack(v);
	}

	//force calculation

	for (int i = 0; i <= max_gridindex; i++) {
		for (int j = 0; j <= max_gridindex; j++) {
			// Step1: calculate forces inside contained_nodes(i,j)
			calculate_forces_inside_contained_nodes(F_rep, A, contained_nodes(i, j));

			// Step 2: calculated forces to nodes in neighbour boxes

			//find_neighbour_boxes

			neighbour_boxes.clear();
			for (int x = i - 1; x <= i + 1; x++) {
				for (int y = j - 1; y <= j + 1; y++) {
					if ((x >= 0) && (y >= 0) && (x <= max_gridindex) && (y <= max_gridindex))
					{
						neighbour.m_x = x;
						neighbour.m_y = y;
						if ((x != i) || (y != j))
							neighbour_boxes.pushBack(neighbour);
					}
				}
			}

			//forget neighbour_boxes that already had access to this box
			for (const IPoint &act_neighbour_box : neighbour_boxes)
			{
				int act_i = act_neighbour_box.m_x;
				int act_j = act_neighbour_box.m_y;
				if ((act_j == j + 1) || ((act_j == j) && (act_i == i + 1)))
				{
					for (node v : contained_nodes(i, j)) {
						for (node u : contained_nodes(act_i, act_j))
						{
							DPoint f_rep_u_on_v = numexcept::f_rep_u_on_v(A[u].get_position(), A[v].get_position());
							F_rep[v] += f_rep_u_on_v;
							F_rep[u] -= f_rep_u_on_v;
						}
					}
				}
			}
		}
	}
}


void FruchtermanReingold::make_initialisations(double bl, DPoint d_l_c, int grid_quot)
{
	grid_quotient(grid_quot);
	down_left_corner = d_l_c; //export this two values from FMMM
	boxlength = bl;
}

}
}
}
