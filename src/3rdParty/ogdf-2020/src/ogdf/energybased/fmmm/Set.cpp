/** \file
 * \brief Implementation of class Set.
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

#include <functional>
#include <ogdf/energybased/fmmm/Set.h>

namespace ogdf {
namespace energybased {
namespace fmmm {

Set::Set()
{
	last_selectable_index_of_S_node = -1;
	S_node = nullptr;
}


Set::~Set()
{
	delete[] S_node;
}


void Set::set_seed(int rand_seed)
{
	setSeed(rand_seed);
}


void Set::init_node_set(Graph& G)
{
	S_node = new node[G.numberOfNodes()];
	position_in_node_set.init(G);

	for (node v : G.nodes)
	{
		S_node[v->index()] = v;
		position_in_node_set[v] = v->index();
	}
	last_selectable_index_of_S_node = G.numberOfNodes() - 1;
}

void Set::delete_node(node del_node)
{
	int del_node_index = position_in_node_set[del_node];
	node last_selectable_node = S_node[last_selectable_index_of_S_node];

	S_node[last_selectable_index_of_S_node] = del_node;
	S_node[del_node_index] = last_selectable_node;
	position_in_node_set[del_node] = last_selectable_index_of_S_node;
	position_in_node_set[last_selectable_node] = del_node_index;
	last_selectable_index_of_S_node -=1;
}


// for set of nodes with uniform probability

node Set::get_random_node()
{
	return get_random_node_common(randomNumber(0,last_selectable_index_of_S_node), last_selectable_index_of_S_node);
}

node Set::get_random_node_common(int rand_index, int &last_trie_index)
{
	node random_node = S_node[rand_index];
	node last_trie_node = S_node[last_trie_index];

	S_node[last_trie_index] = random_node;
	S_node[rand_index] = last_trie_node;
	position_in_node_set[random_node] = last_trie_index;
	position_in_node_set[last_trie_node] = rand_index;
	--last_trie_index;
	return random_node;
}


// for set of nodes with weighted  probability

void Set::init_node_set(Graph& G, NodeArray<NodeAttributes>& A)
{
	init_node_set(G);
	mass_of_star.init(G);
	for (node v : G.nodes)
	{
		mass_of_star[v] = A[v].get_mass();
		for(adjEntry adj : v->adjEntries) {
			edge e_adj = adj->theEdge();
			node v_adj;
			if (e_adj->source() != v)
				v_adj = e_adj->source();
			else
				v_adj = e_adj->target();
			mass_of_star[v] += A[v_adj].get_mass();
		}
	}
}

// for set of nodes with "lower mass" or "higher mass" probability

template<typename Comp>
node Set::get_random_node_with_some_star_mass(int rand_tries, Comp comp)
{
	int rand_index = -1;
	int cmp_mass(0);

	// randomly select rand_tries distinct!!! nodes from S_node and select one
	// by the lowest (std::less) or the highest (std::greater) star mass

	int last_trie_index = last_selectable_index_of_S_node;
	OGDF_ASSERT(last_trie_index >= 0);

	for (int i = 1; i <= rand_tries && last_trie_index >= 0; ++i) {
		int new_rand_index = randomNumber(0, last_trie_index);
		int mass = mass_of_star[S_node[new_rand_index]];
		get_random_node_common(new_rand_index, last_trie_index);
		if ((i == 1) || (comp(mass, cmp_mass))) {
			rand_index = last_trie_index + 1;
			cmp_mass = mass;
		}
	}
	OGDF_ASSERT(rand_index != -1);

	return get_random_node_common(rand_index, last_selectable_index_of_S_node);
}

node Set::get_random_node_with_lowest_star_mass(int rand_tries)
{
	return get_random_node_with_some_star_mass<std::less<int>>(rand_tries);
}

node Set::get_random_node_with_highest_star_mass(int rand_tries)
{
	return get_random_node_with_some_star_mass<std::greater<int>>(rand_tries);
}

}
}
}
