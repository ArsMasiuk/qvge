/** \file
 * \brief Declaration of class Set.
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

#include <ogdf/basic/List.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/energybased/fmmm/NodeAttributes.h>

namespace ogdf {
namespace energybased {
namespace fmmm {

//! Helping data structure that holds set S_node of nodes in the range [0,
//! G.number_of_nodes()-1] (needed for class Multilevel) for randomly choosing nodes
//! (with uniform or weighted probability!)
class Set
{
public:
	Set();     //!< constructor
	~Set();    //!< destructor

	void set_seed(int rand_seed); //!< the the random seed to rand_seed


	//! \name for set of nodes @{

	//! Inits S_node[0,...,G.number_of_nodes()-1] and stores the i-th node of P
	//! at position S_node[i] and in position_in_node_set for each node its index in
	//! S_node.
	void init_node_set(Graph& G);

	//! Returns whether S_node is empty or not.
	bool empty_node_set() {
		return last_selectable_index_of_S_node < 0;
	}

	//! Returns true if and only if v is deleted from S_node.
	bool is_deleted(node v) {
		return position_in_node_set[v] > last_selectable_index_of_S_node;
	}

	//! Deletes the node v from S_node.
	void delete_node(node v);

	//! @}
	//! \name for set of nodes with uniform probability @{

	//! Selects a random element from S_node with uniform probability and updates S_node
	//! and position_in_node_set.
	node get_random_node();

	//! @}
	//! \name for set of nodes with weighted  probability @{

	//! Same as init_node_set(G), but additionally the array mass_of_star is caculated.
	void init_node_set(Graph& G,NodeArray<NodeAttributes>& A);

	//! @}
	//! \name for set of nodes with ``lower mass'' probability @{

	//! Gets rand_tries random elements from S_node and selects the one with the lowest
	//! mass_of_star and updates S_node and position_in_node_set.
	node get_random_node_with_lowest_star_mass(int rand_tries);

	//! @}
	//! \name for set of nodes with ``higher mass'' probability @{

	//! Gets rand_tries random elements from S_node and selects the one with the highest
	//! mass_of_star and updates S_node and position_in_node_set.
	node get_random_node_with_highest_star_mass(int rand_tries);

	//! @}

protected:
	//! Common updates for each get_random_node method
	node get_random_node_common(int, int &);

	//! Helper function for get_random_node methods with lowest or highest star mass
	template<typename Comp>
	node get_random_node_with_some_star_mass(int rand_tries, Comp comp = Comp());

private:
	node* S_node;       //!< representation of the node set S_node[0,G.number_of_nodes()-1]
	int last_selectable_index_of_S_node;//!< index of the last randomly choosable element
	//! in S_node (this value is decreased after each
	//! select operation)
	NodeArray<int> position_in_node_set;//!< holds for each node of G the index of its
	//! position in S_node
	NodeArray<int> mass_of_star; //!< the sum of the masses of a node and its neighbours
};

}
}
}
