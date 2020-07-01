/** \file
 * \brief Implementation of class Multilevel (used by FMMMLayout).
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


#include <ogdf/energybased/fmmm/Multilevel.h>
#include <ogdf/energybased/fmmm/Set.h>
#include <ogdf/basic/simple_graph_alg.h>

namespace ogdf {
namespace energybased {
namespace fmmm {

void Multilevel::create_multilevel_representations(
	Graph& G,
	NodeArray<NodeAttributes>& A,
	EdgeArray<EdgeAttributes>& E,
	int rand_seed,
	FMMMOptions::GalaxyChoice galaxy_choice,
	int min_Graph_size,
	int random_tries,
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray <NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int & max_level)
{
	setSeed(rand_seed);
	G_mult_ptr[0] = &G; //init graph at level 0 to the original undirected simple
	A_mult_ptr[0] = &A; //and loopfree connected graph G/A/E
	E_mult_ptr[0] = &E;

	int bad_edgenr_counter = 0;
	int act_level = 0;
	Graph* act_Graph_ptr = G_mult_ptr[0];

	while( (act_Graph_ptr->numberOfNodes() > min_Graph_size) &&
		edgenumbersum_of_all_levels_is_linear(G_mult_ptr,act_level,bad_edgenr_counter) )
	{
		Graph* G_new = new (Graph);
		NodeArray<NodeAttributes>* A_new = new NodeArray<NodeAttributes>;
		EdgeArray<EdgeAttributes>* E_new = new EdgeArray<EdgeAttributes>;
		G_mult_ptr[act_level+1] = G_new;
		A_mult_ptr[act_level+1] = A_new;
		E_mult_ptr[act_level+1] = E_new;

		init_multilevel_values(*G_mult_ptr[act_level], *A_mult_ptr[act_level], *E_mult_ptr[act_level]);
		partition_galaxy_into_solar_systems(G_mult_ptr,A_mult_ptr,E_mult_ptr,rand_seed,
			galaxy_choice,random_tries,act_level);
		collaps_solar_systems(G_mult_ptr,A_mult_ptr,E_mult_ptr,act_level);

		act_level++;
		act_Graph_ptr = G_mult_ptr[act_level];
	}
	max_level = act_level;
}


bool Multilevel::edgenumbersum_of_all_levels_is_linear(
	Array<Graph*> &G_mult_ptr,
	int act_level,
	int& bad_edgenr_counter)
{
	if(act_level == 0)
		return true;
	else
	{
		if(G_mult_ptr[act_level]->numberOfEdges()<=
			0.8 * double (G_mult_ptr[act_level-1]->numberOfEdges()))
			return true;
		else if(bad_edgenr_counter < 5)
		{
			bad_edgenr_counter++;
			return true;
		}
		return false;
	}
}


void Multilevel::init_multilevel_values(const Graph &G_mult,
                                        NodeArray<NodeAttributes> &A_mult,
                                        EdgeArray<EdgeAttributes> &E_mult) {
	for(node v : G_mult.nodes) {
		A_mult[v].init_mult_values();
	}

	for(edge e : G_mult.edges) {
		E_mult[e].init_mult_values();
	}
}


inline void Multilevel::partition_galaxy_into_solar_systems(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int rand_seed,
	FMMMOptions::GalaxyChoice galaxy_choice,
	int random_tries,
	int act_level)
{
	create_suns_and_planets(G_mult_ptr, A_mult_ptr, E_mult_ptr, rand_seed, galaxy_choice,
		random_tries, act_level);
	create_moon_nodes_and_pm_nodes(*G_mult_ptr[act_level], *A_mult_ptr[act_level], *E_mult_ptr[act_level]);
}


void Multilevel::create_suns_and_planets(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int rand_seed,
	FMMMOptions::GalaxyChoice galaxy_choice,
	int random_tries,
	int act_level)
{
	Set Node_Set;
	List<node> planet_nodes;
	List<node> sun_nodes;

	//make initialisations
	sun_nodes.clear();
	Node_Set.set_seed(rand_seed); //set seed for random number generator

	for (node v : G_mult_ptr[act_level]->nodes)
	if (act_level == 0)
		(*A_mult_ptr[act_level])[v].set_mass(1);
	switch (galaxy_choice) {
	case FMMMOptions::GalaxyChoice::UniformProb:
		Node_Set.init_node_set(*G_mult_ptr[act_level]);
		break;
	case FMMMOptions::GalaxyChoice::NonUniformProbLowerMass:
	case FMMMOptions::GalaxyChoice::NonUniformProbHigherMass:
		Node_Set.init_node_set(*G_mult_ptr[act_level], *A_mult_ptr[act_level]);
	}

	while (!Node_Set.empty_node_set()) {
		//randomly select a sun node
		planet_nodes.clear();
		node sun_node = nullptr;
		switch (galaxy_choice) {
		case FMMMOptions::GalaxyChoice::UniformProb:
			sun_node = Node_Set.get_random_node();
			break;
		case FMMMOptions::GalaxyChoice::NonUniformProbLowerMass:
			sun_node = Node_Set.get_random_node_with_lowest_star_mass(random_tries);
			break;
		case FMMMOptions::GalaxyChoice::NonUniformProbHigherMass:
			sun_node = Node_Set.get_random_node_with_highest_star_mass(random_tries);
			break;
		}
		sun_nodes.pushBack(sun_node);

		//create new node at higher level that represents the collapsed solar_system
		node newNode = G_mult_ptr[act_level + 1]->newNode();

		//update information for sun_node
		(*A_mult_ptr[act_level])[sun_node].set_higher_level_node(newNode);
		(*A_mult_ptr[act_level])[sun_node].set_type(1);
		(*A_mult_ptr[act_level])[sun_node].set_dedicated_sun_node(sun_node);
		(*A_mult_ptr[act_level])[sun_node].set_dedicated_sun_distance(0);

		//update information for planet_nodes
		for(adjEntry adj : sun_node->adjEntries) {
			edge sun_edge = adj->theEdge();
			double dist_to_sun = (*E_mult_ptr[act_level])[sun_edge].get_length();
			node planet_node;
			if (sun_edge->source() != sun_node)
				planet_node = sun_edge->source();
			else
				planet_node = sun_edge->target();
			(*A_mult_ptr[act_level])[planet_node].set_type(2);
			(*A_mult_ptr[act_level])[planet_node].set_dedicated_sun_node(sun_node);
			(*A_mult_ptr[act_level])[planet_node].set_dedicated_sun_distance(dist_to_sun);
			planet_nodes.pushBack(planet_node);
		}

		//delete all planet_nodes and possible_moon_nodes from Node_Set

		for (node planet_node : planet_nodes)
		if (!Node_Set.is_deleted(planet_node))
			Node_Set.delete_node(planet_node);

		for (node planet_node : planet_nodes)
		{
			for(adjEntry adj : planet_node->adjEntries) {
				edge e = adj->theEdge();

				node pos_moon_node = (e->source() == planet_node) ? e->target() : e->source();
				if (!Node_Set.is_deleted(pos_moon_node))
					Node_Set.delete_node(pos_moon_node);
			}
		}
	}

	//init *A_mult_ptr[act_level+1] and set NodeAttributes information for new nodes
	A_mult_ptr[act_level + 1]->init(*G_mult_ptr[act_level + 1]);
	for (node sun_node : sun_nodes)
	{
		node newNode = (*A_mult_ptr[act_level])[sun_node].get_higher_level_node();
		(*A_mult_ptr[act_level + 1])[newNode].set_NodeAttributes(
			(*A_mult_ptr[act_level])[sun_node].get_width(),
			(*A_mult_ptr[act_level])[sun_node].get_height(),
			(*A_mult_ptr[act_level])[sun_node].get_position(),
			sun_node, nullptr);
		(*A_mult_ptr[act_level + 1])[newNode].set_mass(0);
	}
}


void Multilevel::create_moon_nodes_and_pm_nodes(const Graph &G_mult,
                                                NodeArray<NodeAttributes> &A_mult,
                                                EdgeArray<EdgeAttributes> &E_mult)
{
	for (node v : G_mult.nodes) {
		if (A_mult[v].get_type() == 0) { // a moon node
			node nearest_neighbour_node = nullptr;
			double dist_to_nearest_neighbour(0);
			edge moon_edge = nullptr;

			// find nearest neighbour node
			for (adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();
				node neighbour_node = (v == e->source()) ? e->target() : e->source();
				int neighbour_type = A_mult[neighbour_node].get_type();
				const double dist = E_mult[e].get_length();
				if ((neighbour_type == 2
				  || neighbour_type == 3)
				 && (nearest_neighbour_node == nullptr
				  || dist_to_nearest_neighbour > dist)) {
					moon_edge = e;
					dist_to_nearest_neighbour = dist;
					nearest_neighbour_node = neighbour_node;
				}
			}
			// find dedic. solar system for v and update information
			// in A_mult and E_mult

			OGDF_ASSERT(nearest_neighbour_node != nullptr);
			E_mult[moon_edge].make_moon_edge(); //mark this edge
			node dedicated_sun_node = A_mult[nearest_neighbour_node].get_dedicated_sun_node();
			double dedicated_sun_distance = dist_to_nearest_neighbour
			  + A_mult[nearest_neighbour_node].get_dedicated_sun_distance();
			A_mult[v].set_type(4);
			A_mult[v].set_dedicated_sun_node(dedicated_sun_node);
			A_mult[v].set_dedicated_sun_distance(dedicated_sun_distance);
			A_mult[v].set_dedicated_pm_node(nearest_neighbour_node);

			// identify nearest_neighbour_node as a pm_node and update its information
			A_mult[nearest_neighbour_node].set_type(3);
			A_mult[nearest_neighbour_node].get_dedicated_moon_node_List_ptr()->pushBack(v);
		}
	}
}


inline void Multilevel::collaps_solar_systems(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int act_level)
{
	EdgeArray<double> new_edgelength;
	calculate_mass_of_collapsed_nodes(G_mult_ptr, A_mult_ptr, act_level);
	create_edges_edgedistances_and_lambda_Lists(G_mult_ptr, A_mult_ptr, E_mult_ptr,
		new_edgelength, act_level);
	delete_parallel_edges_and_update_edgelength(G_mult_ptr, E_mult_ptr, new_edgelength,
		act_level);
}


void Multilevel::calculate_mass_of_collapsed_nodes(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray <NodeAttributes>*> &A_mult_ptr,
	int act_level)
{
	for (node v : G_mult_ptr[act_level]->nodes)
	{
		node dedicated_sun = (*A_mult_ptr[act_level])[v].get_dedicated_sun_node();
		node high_level_node = (*A_mult_ptr[act_level])[dedicated_sun].get_higher_level_node();
		(*A_mult_ptr[act_level + 1])[high_level_node].set_mass((*A_mult_ptr[act_level + 1])
			[high_level_node].get_mass() + 1);
	}
}


void Multilevel::create_edges_edgedistances_and_lambda_Lists(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	EdgeArray<double>& new_edgelength, int
	act_level)
{
	edge e_new;
	List<edge> inter_solar_system_edges;

	//create new edges at act_level+1 and create for each inter solar system edge  at
	//act_level a link to its corresponding edge

	for (edge e : G_mult_ptr[act_level]->edges) {
		node s_node = e->source();
		node t_node = e->target();
		node s_sun_node = (*A_mult_ptr[act_level])[s_node].get_dedicated_sun_node();
		node t_sun_node = (*A_mult_ptr[act_level])[t_node].get_dedicated_sun_node();
		if (s_sun_node != t_sun_node) //a inter solar system edge
		{
			node high_level_sun_s = (*A_mult_ptr[act_level])[s_sun_node].get_higher_level_node();
			node high_level_sun_t = (*A_mult_ptr[act_level])[t_sun_node].get_higher_level_node();

			//create new edge in *G_mult_ptr[act_level+1]
			e_new = G_mult_ptr[act_level + 1]->newEdge(high_level_sun_s, high_level_sun_t);
			(*E_mult_ptr[act_level])[e].set_higher_level_edge(e_new);
			inter_solar_system_edges.pushBack(e);
		}
	}

	//init new_edgelength calculate the values of new_edgelength and the lambda Lists

	new_edgelength.init(*G_mult_ptr[act_level + 1]);
	for (edge e : inter_solar_system_edges) {
		node s_node = e->source();
		node t_node = e->target();
		node s_sun_node = (*A_mult_ptr[act_level])[s_node].get_dedicated_sun_node();
		node t_sun_node = (*A_mult_ptr[act_level])[t_node].get_dedicated_sun_node();
		double length_e = (*E_mult_ptr[act_level])[e].get_length();
		double length_s_edge = (*A_mult_ptr[act_level])[s_node].get_dedicated_sun_distance();
		double length_t_edge = (*A_mult_ptr[act_level])[t_node].get_dedicated_sun_distance();
		double newlength = length_s_edge + length_e + length_t_edge;

		//set new edge_length in *G_mult_ptr[act_level+1]
		e_new = (*E_mult_ptr[act_level])[e].get_higher_level_edge();
		new_edgelength[e_new] = newlength;

		//create entries in lambda Lists
		double lambda_s = length_s_edge / newlength;
		double lambda_t = length_t_edge / newlength;
		(*A_mult_ptr[act_level])[s_node].get_lambda_List_ptr()->pushBack(lambda_s);
		(*A_mult_ptr[act_level])[t_node].get_lambda_List_ptr()->pushBack(lambda_t);
		(*A_mult_ptr[act_level])[s_node].get_neighbour_sun_node_List_ptr()->pushBack(t_sun_node);
		(*A_mult_ptr[act_level])[t_node].get_neighbour_sun_node_List_ptr()->pushBack(s_sun_node);
	}
}

void Multilevel::delete_parallel_edges_and_update_edgelength(
	Array<Graph*> &G_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	EdgeArray<double>& new_edgelength, int
	act_level)
{
	EdgeMaxBucketFunc get_max_index;
	EdgeMinBucketFunc get_min_index;
	edge e_save;
	Edge f_act;
	List<Edge> sorted_edges;
	Graph* Graph_ptr = G_mult_ptr[act_level + 1];
	int save_s_index, save_t_index;
	int counter = 1;

	//make *G_mult_ptr[act_level+1] undirected
	makeSimpleUndirected(*G_mult_ptr[act_level + 1]);

	//sort the List sorted_edges
	for (edge e_act : Graph_ptr->edges)
	{
		f_act.set_Edge(e_act, Graph_ptr);
		sorted_edges.pushBack(f_act);
	}

	sorted_edges.bucketSort(0, Graph_ptr->numberOfNodes() - 1, get_max_index);
	sorted_edges.bucketSort(0, Graph_ptr->numberOfNodes() - 1, get_min_index);

	//now parallel edges are consecutive in sorted_edges
	bool firstEdge = true;
	for (const Edge &ei : sorted_edges) {
		edge e_act = ei.get_edge();
		int act_s_index = e_act->source()->index();
		int act_t_index = e_act->target()->index();

		if (!firstEdge)
		{
			if ((act_s_index == save_s_index && act_t_index == save_t_index) ||
				(act_s_index == save_t_index && act_t_index == save_s_index))
			{
				new_edgelength[e_save] += new_edgelength[e_act];
				Graph_ptr->delEdge(e_act);
				counter++;
			} else {
				if (counter > 1)
				{
					new_edgelength[e_save] /= counter;
					counter = 1;
				}
				save_s_index = act_s_index;
				save_t_index = act_t_index;
				e_save = e_act;
			}
		} else { //first edge
			firstEdge = false;
			save_s_index = act_s_index;
			save_t_index = act_t_index;
			e_save = e_act;
		}
	}

	//treat special case (last edges were multiple edges)
	if (counter > 1)
		new_edgelength[e_save] /= counter;

	//init *E_mult_ptr[act_level+1] and import EdgeAttributes
	E_mult_ptr[act_level + 1]->init(*G_mult_ptr[act_level + 1]);
	for (edge e_act : Graph_ptr->edges)
		(*E_mult_ptr[act_level + 1])[e_act].set_length(new_edgelength[e_act]);
}


void Multilevel::find_initial_placement_for_level(
	int level,
	FMMMOptions::InitialPlacementMult init_placement_way,
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr)
{
	List<node> pm_nodes;
	set_initial_positions_of_sun_nodes(level, G_mult_ptr, A_mult_ptr);
	set_initial_positions_of_planet_and_moon_nodes(level, init_placement_way, G_mult_ptr,
		A_mult_ptr, E_mult_ptr, pm_nodes);
	set_initial_positions_of_pm_nodes(level, init_placement_way, A_mult_ptr,
		E_mult_ptr, pm_nodes);
}


void Multilevel::set_initial_positions_of_sun_nodes(
	int level,
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray <NodeAttributes>*> &A_mult_ptr)
{
	for (node v_high : G_mult_ptr[level + 1]->nodes)
	{
		node v_act = (*A_mult_ptr[level + 1])[v_high].get_lower_level_node();
		DPoint new_pos = (*A_mult_ptr[level + 1])[v_high].get_position();
		(*A_mult_ptr[level])[v_act].set_position(new_pos);
		(*A_mult_ptr[level])[v_act].place();
	}
}


void Multilevel::set_initial_positions_of_planet_and_moon_nodes(
	int level,
	FMMMOptions::InitialPlacementMult init_placement_way,
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	List<node>& pm_nodes)
{
	DPoint new_pos;
	List<DPoint> L;

	create_all_placement_sectors(G_mult_ptr, A_mult_ptr, E_mult_ptr, level);
	for (node v : G_mult_ptr[level]->nodes)
	{
		int node_type = (*A_mult_ptr[level])[v].get_type();
		if (node_type == 3)
			pm_nodes.pushBack(v);
		else if (node_type == 2 || node_type == 4) //a planet_node or moon_node
		{
			L.clear();
			node dedicated_sun = (*A_mult_ptr[level])[v].get_dedicated_sun_node();
			DPoint dedicated_sun_pos = (*A_mult_ptr[level])[dedicated_sun].get_position();
			double dedicated_sun_distance = (*A_mult_ptr[level])[v].get_dedicated_sun_distance();

			switch (init_placement_way) {
			case FMMMOptions::InitialPlacementMult::Simple:
				break;
			case FMMMOptions::InitialPlacementMult::Advanced:
				for(adjEntry adj : v->adjEntries) {
					edge e = adj->theEdge();
					node v_adj = (e->source() != v) ? e->source() : e->target();
					if (((*A_mult_ptr[level])[v].get_dedicated_sun_node() ==
						(*A_mult_ptr[level])[v_adj].get_dedicated_sun_node()) &&
						((*A_mult_ptr[level])[v_adj].get_type() != 1) &&
						((*A_mult_ptr[level])[v_adj].is_placed()))
					{
						new_pos = calculate_position(dedicated_sun_pos, (*A_mult_ptr[level])
							[v_adj].get_position(), dedicated_sun_distance,
							(*E_mult_ptr[level])[e].get_length());
						L.pushBack(new_pos);
					}
				}
			}
			if ((*A_mult_ptr[level])[v].get_lambda_List_ptr()->empty())
			{//special case
				if (L.empty())
				{
					new_pos = create_random_pos(dedicated_sun_pos, (*A_mult_ptr[level])
						[v].get_dedicated_sun_distance(),
						(*A_mult_ptr[level])[v].get_angle_1(),
						(*A_mult_ptr[level])[v].get_angle_2());
					L.pushBack(new_pos);
				}
			} else { // usual case
				ListIterator<double> lambdaIterator = (*A_mult_ptr[level])[v].get_lambda_List_ptr()->begin();

				for (node adj_sun : *(*A_mult_ptr[level])[v].get_neighbour_sun_node_List_ptr())
				{
					double lambda = *lambdaIterator;
					DPoint adj_sun_pos = (*A_mult_ptr[level])[adj_sun].get_position();
					new_pos = get_waggled_inbetween_position(dedicated_sun_pos, adj_sun_pos, lambda);
					L.pushBack(new_pos);
					if (lambdaIterator != (*A_mult_ptr[level])[v].get_lambda_List_ptr()->rbegin())
						lambdaIterator = (*A_mult_ptr[level])[v].get_lambda_List_ptr()->cyclicSucc(lambdaIterator);
				}
			}

			(*A_mult_ptr[level])[v].set_position(get_barycenter_position(L));
			(*A_mult_ptr[level])[v].place();
		}
	}
}


void Multilevel::create_all_placement_sectors(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int level)
{
	for(node v_high : G_mult_ptr[level+1]->nodes) {
		double angle_1(0), angle_2(0);
		//find pos of adjacent nodes
		List<DPoint> adj_pos;
		DPoint v_high_pos((*A_mult_ptr[level+1])[v_high].get_x(), (*A_mult_ptr[level+1])[v_high].get_y());

		for(adjEntry adj : v_high->adjEntries) {
			edge e_high = adj->theEdge();

			if (!(*E_mult_ptr[level+1])[e_high].is_extra_edge()) {
				node w_high;
				if (v_high == e_high->source())
					w_high = e_high->target();
				else
					w_high = e_high->source();

				DPoint w_high_pos ((*A_mult_ptr[level+1])[w_high].get_x(),
					(*A_mult_ptr[level+1])[w_high].get_y());
				adj_pos.pushBack(w_high_pos);
			}
		}
		const DPoint x_parallel_pos(v_high_pos.m_x + 1, v_high_pos.m_y);
		if (adj_pos.empty()) { //easy case
			angle_2 = 2.0 * Math::pi;
		} else if (adj_pos.size() == 1) { //special case
			angle_1 = v_high_pos.angle(x_parallel_pos, *adj_pos.begin());
			angle_2 = angle_1 + Math::pi;
		} else { //usual case
			const int MAX = 10; //the biggest of at most MAX random selected sectors is choosen
			int steps = 1;
			ListIterator<DPoint> it = adj_pos.begin();
			do {
				double act_angle_1 = v_high_pos.angle(x_parallel_pos, *it);
				double min_next_angle = std::numeric_limits<double>::max();
				for (const auto &next_pos : adj_pos) {
					if (*it != next_pos) {
						Math::updateMin(min_next_angle, v_high_pos.angle(*it, next_pos));
					}
				}
				OGDF_ASSERT(min_next_angle < std::numeric_limits<double>::max());

				if ((it == adj_pos.begin())
				 || (min_next_angle > angle_2 - angle_1)) {
					angle_1 = act_angle_1;
					angle_2 = act_angle_1 + min_next_angle;
				}
				if (it.valid() && it.succ().valid())
					it = adj_pos.cyclicSucc(it);
				steps++;
			} while (steps <= MAX && it.valid() && it.succ().valid());

			if (angle_1 == angle_2)
				angle_2 = angle_1 + Math::pi;
		}

		//import angle_1 and angle_2 to the dedicated suns at level level
		node sun_node = (*A_mult_ptr[level+1])[v_high].get_lower_level_node();
		(*A_mult_ptr[level])[sun_node].set_angle_1(angle_1);
		(*A_mult_ptr[level])[sun_node].set_angle_2(angle_2);
	}

	//import the angle values from the values of the dedicated sun nodes
	for(node v : G_mult_ptr[level]->nodes)
	{
		node ded_sun = (*A_mult_ptr[level])[v].get_dedicated_sun_node();
		(*A_mult_ptr[level])[v].set_angle_1((*A_mult_ptr[level])[ded_sun].get_angle_1());
		(*A_mult_ptr[level])[v].set_angle_2((*A_mult_ptr[level])[ded_sun].get_angle_2());
	}
}


void Multilevel::set_initial_positions_of_pm_nodes(
	int level,
	FMMMOptions::InitialPlacementMult init_placement_way,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	List<node>& pm_nodes)
{
	double moon_dist, lambda;
	node v_adj, sun_node;
	DPoint sun_pos, moon_pos, new_pos, adj_sun_pos;
	List<DPoint> L;
	ListIterator<double> lambdaIterator;

	for (node v : pm_nodes) {
		L.clear();
		sun_node = (*A_mult_ptr[level])[v].get_dedicated_sun_node();
		sun_pos =  (*A_mult_ptr[level])[sun_node].get_position();
		double sun_dist = (*A_mult_ptr[level])[v].get_dedicated_sun_distance();

		switch (init_placement_way) {
		case FMMMOptions::InitialPlacementMult::Simple:
			break;
		case FMMMOptions::InitialPlacementMult::Advanced:
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();

				if(e->source() != v)
					v_adj = e->source();
				else
					v_adj = e->target();
				if( (!(*E_mult_ptr[level])[e].is_moon_edge()) &&
					( (*A_mult_ptr[level])[v].get_dedicated_sun_node() ==
					(*A_mult_ptr[level])[v_adj].get_dedicated_sun_node() ) &&
					( (*A_mult_ptr[level])[v_adj].get_type() != 1 ) &&
					( (*A_mult_ptr[level])[v_adj].is_placed() ) )
				{
					new_pos = calculate_position(sun_pos,(*A_mult_ptr[level])[v_adj].
						get_position(),sun_dist,(*E_mult_ptr[level])
						[e].get_length());
					L.pushBack(new_pos);
				}
			}
		}
		for(node moon_node : *(*A_mult_ptr[level])[v].get_dedicated_moon_node_List_ptr())
		{
			moon_pos = (*A_mult_ptr[level])[moon_node].get_position();
			moon_dist =  (*A_mult_ptr[level])[moon_node].get_dedicated_sun_distance();
			lambda = sun_dist/moon_dist;
			new_pos = get_waggled_inbetween_position(sun_pos,moon_pos,lambda);
			L.pushBack(new_pos);
		}

		if (!(*A_mult_ptr[level])[v].get_lambda_List_ptr()->empty())
		{
			lambdaIterator = (*A_mult_ptr[level])[v].get_lambda_List_ptr()->begin();

			for(node adj_sun : *(*A_mult_ptr[level])[v].get_neighbour_sun_node_List_ptr())
			{
				lambda = *lambdaIterator;
				adj_sun_pos = (*A_mult_ptr[level])[adj_sun].get_position();
				new_pos = get_waggled_inbetween_position(sun_pos,adj_sun_pos,lambda);
				L.pushBack(new_pos);
				if(lambdaIterator != (*A_mult_ptr[level])[v].get_lambda_List_ptr()->rbegin())
					lambdaIterator = (*A_mult_ptr[level])[v].get_lambda_List_ptr()->cyclicSucc(lambdaIterator);
			}
		}

		(*A_mult_ptr[level])[v].set_position(get_barycenter_position(L));
		(*A_mult_ptr[level])[v].place();
	}
}

inline DPoint Multilevel::create_random_pos(DPoint center,double radius,double angle_1,
	double angle_2)
{
	const int BILLION = 1000000000;
	DPoint new_point;
	double rnd = double(randomNumber(1,BILLION)+1)/(BILLION+2);//rand number in (0,1)
	double rnd_angle = angle_1 +(angle_2-angle_1)*rnd;
	double dx = cos(rnd_angle) * radius;
	double dy = sin(rnd_angle) * radius;
	new_point.m_x = center.m_x + dx ;
	new_point.m_y = center.m_y + dy;
	return new_point;
}


inline DPoint Multilevel::get_waggled_inbetween_position(DPoint s, DPoint t, double lambda)
{
	const double WAGGLEFACTOR = 0.05;
	const int BILLION = 1000000000;
	DPoint inbetween_point;
	inbetween_point.m_x = s.m_x + lambda*(t.m_x - s.m_x);
	inbetween_point.m_y = s.m_y + lambda*(t.m_y - s.m_y);
	double radius = WAGGLEFACTOR * (t-s).norm();
	double rnd = double(randomNumber(1,BILLION)+1)/(BILLION+2);//rand number in (0,1)
	double rand_radius =  radius * rnd;
	return create_random_pos(inbetween_point,rand_radius,0,2.0*Math::pi);
}


inline DPoint Multilevel::get_barycenter_position(List<DPoint>& L)
{
	DPoint sum (0,0);
	DPoint barycenter;

	for(const DPoint &act_point : L)
		sum = sum + act_point;
	barycenter.m_x = sum.m_x/L.size();
	barycenter.m_y = sum.m_y/L.size();
	return barycenter;
}


inline DPoint Multilevel::calculate_position(DPoint P, DPoint Q, double dist_P, double dist_Q)
{
	double dist_PQ = (P-Q).norm();
	double lambda = (dist_P + (dist_PQ - dist_P - dist_Q)/2)/dist_PQ;
	return get_waggled_inbetween_position(P,Q,lambda);
}


void Multilevel::delete_multilevel_representations(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int max_level)
{
	for(int i=1; i<= max_level; i++)
	{
		delete G_mult_ptr[i];
		delete A_mult_ptr[i];
		delete E_mult_ptr[i];
	}
}

}
}
}
