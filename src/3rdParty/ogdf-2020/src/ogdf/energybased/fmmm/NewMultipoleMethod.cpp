/** \file
 * \brief Implementation of class NewMultipoleMethod (NMM)
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


#include <ogdf/energybased/fmmm/NewMultipoleMethod.h>
#include <ogdf/energybased/fmmm/common.h>

#define MIN_BOX_LENGTH   1e-300

namespace ogdf {
namespace energybased {
namespace fmmm {

using std::complex;

// Error-Handling for complex logarithm
static inline complex<double> log(complex<double> z)
{
	if (std::real(z) <= 0 && std::imag(z) == 0) { // no cont. compl. log fct exists
		return std::log(z + 0.0000001);
	}
	return std::log(z);
}

//! Returned state for traverse()
struct ParticleListState {
	//! Left particle list is empty
	bool leftEmpty;
	//! Right particle list is empty
	bool rightEmpty;
	//! Left particle list is larger
	bool leftLarger;
	//! Last left item
	ListIterator<ParticleInfo> lastLeft;
};

static ParticleListState traverse(List<ParticleInfo> &relevantList, double mid_coord)
{
	auto l_item = relevantList.begin();
	auto r_item = relevantList.rbegin();
	bool last_left_item_found = false;
	ParticleListState state{false, false, true, nullptr};

	// traverse *act_ptr->get_x(y)_List_ptr() from left and right
	while (!last_left_item_found) {
		double l_coord = (*l_item).get_x_y_coord();
		double r_coord = (*r_item).get_x_y_coord();
		if (l_coord >= mid_coord) {
			state.leftLarger = false;
			last_left_item_found = true;
			if (l_item != relevantList.begin()) {
				state.lastLeft = relevantList.cyclicPred(l_item);
			} else {
				state.leftEmpty = true;
			}
		} else if (r_coord < mid_coord) {
			last_left_item_found = true;
			if (r_item != relevantList.rbegin()) {
				state.lastLeft = r_item;
			} else {
				state.rightEmpty = true;
			}
		}
		if (!last_left_item_found) {
			l_item = relevantList.cyclicSucc(l_item);
			r_item = relevantList.cyclicSucc(r_item);
		}
	}

	return state;
}

NewMultipoleMethod::NewMultipoleMethod()
  : MIN_NODE_NUMBER(175)
  , using_NMM(true)
  , max_power_of_2_index(30)
{
	// setting predefined parameters
	precision(4); particles_in_leaves(25);
	tree_construction_way(FMMMOptions::ReducedTreeConstruction::SubtreeBySubtree);
	find_sm_cell(FMMMOptions::SmallestCellFinding::Iteratively);
}


void NewMultipoleMethod::calculate_repulsive_forces(
	const Graph &G,
	NodeArray <NodeAttributes>& A,
	NodeArray<DPoint>& F_rep)
{
	if (using_NMM) { // use NewMultipoleMethod
		calculate_repulsive_forces_by_NMM(G,A,F_rep);
	} else { // use the exact naive way
		calculate_repulsive_forces_by_exact_method(G,A,F_rep);
	}
}


void NewMultipoleMethod::calculate_repulsive_forces_by_NMM(
	const Graph &G,
	NodeArray<NodeAttributes>& A,
	NodeArray<DPoint>& F_rep)
{
	QuadTreeNM T;
	NodeArray<DPoint> F_direct(G);
	NodeArray<DPoint> F_local_exp(G);
	NodeArray<DPoint> F_multipole_exp(G);
	List<QuadTreeNodeNM*> quad_tree_leaves;

	// initializations
	for (node v : G.nodes) {
		F_direct[v] = F_local_exp[v] = F_multipole_exp[v] = DPoint(0, 0);
	}

	quad_tree_leaves.clear();
	switch (tree_construction_way()) {
	case FMMMOptions::ReducedTreeConstruction::PathByPath:
		build_up_red_quad_tree_path_by_path(G,A,T);
		break;
	case FMMMOptions::ReducedTreeConstruction::SubtreeBySubtree:
		build_up_red_quad_tree_subtree_by_subtree(G,A,T);
	}

	form_multipole_expansions(A,T,quad_tree_leaves);
	calculate_local_expansions_and_WSPRLS(A,T.get_root_ptr());
	transform_local_exp_to_forces(A,quad_tree_leaves,F_local_exp);
	transform_multipole_exp_to_forces(A,quad_tree_leaves,F_multipole_exp);
	calculate_neighbourcell_forces(A,quad_tree_leaves,F_direct);
	add_rep_forces(G,F_direct,F_multipole_exp,F_local_exp,F_rep);

	delete_red_quad_tree_and_count_treenodes(T);
}


inline void NewMultipoleMethod::calculate_repulsive_forces_by_exact_method(
	const Graph &G,
	NodeArray<NodeAttributes>& A,
	NodeArray<DPoint>& F_rep)
{
	ExactMethod.calculate_exact_repulsive_forces(G,A,F_rep);
}


void NewMultipoleMethod::make_initialisations(
	const Graph &G,
	double bl,
	DPoint d_l_c,
	int p_i_l,
	int p,
	FMMMOptions::ReducedTreeConstruction t_c_w,
	FMMMOptions::SmallestCellFinding f_s_c)
{
	if (G.numberOfNodes() >= MIN_NODE_NUMBER) { // using_NMM
		using_NMM = true; //indicate that NMM is used for force calculation

		particles_in_leaves(p_i_l);
		precision(p);
		tree_construction_way(t_c_w);
		find_sm_cell(f_s_c);
		down_left_corner = d_l_c; //Export this two values from FMMM
		boxlength = bl;
		init_binko(2* precision());
	} else { // use exact method
		using_NMM = false; //indicate that exact method is used for force calculation
		ExactMethod.make_initialisations(bl,d_l_c,0);
	}
}


void NewMultipoleMethod::deallocate_memory()
{
	if(using_NMM) {
		free_binko();
	}
}


void NewMultipoleMethod::update_boxlength_and_cornercoordinate(double b_l, DPoint d_l_c)
{
	if(using_NMM) {
		boxlength = b_l;
		down_left_corner = d_l_c;
	} else {
		ExactMethod.update_boxlength_and_cornercoordinate(b_l,d_l_c);
	}
}


inline int NewMultipoleMethod::power_of_two(int i)
{
	OGDF_ASSERT(i >= 0);
	OGDF_ASSERT(i <= max_power_of_2_index);
	return 1 << i;
}


inline int NewMultipoleMethod::maxboxindex (int level)
{
	if (level < 0) {
		std::cout <<"Failure NewMultipoleMethod::maxboxindex :wrong level "<<std::endl;
		std::cout <<"level" <<level<<std::endl;
		return -1;

	} else
		return power_of_two(level)-1;
}


void NewMultipoleMethod::build_up_red_quad_tree_path_by_path(
	const Graph& G,
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T)
{
	List<QuadTreeNodeNM*> act_leaf_List,new_leaf_List;
	List<QuadTreeNodeNM*> *act_leaf_List_ptr,*new_leaf_List_ptr,*help_ptr;
	List <ParticleInfo> act_x_List_copy,act_y_List_copy;
	QuadTreeNodeNM *act_node_ptr;

	build_up_root_node(G,A,T);

	act_leaf_List.clear();
	new_leaf_List.clear();
	act_leaf_List.pushFront(T.get_root_ptr());
	act_leaf_List_ptr = &act_leaf_List;
	new_leaf_List_ptr = &new_leaf_List;

	while(!act_leaf_List_ptr->empty())
	{
		while(!act_leaf_List_ptr->empty())
		{
			act_node_ptr = act_leaf_List_ptr->popFrontRet();
			make_copy_and_init_Lists(*(act_node_ptr->get_x_List_ptr()),act_x_List_copy,
						*(act_node_ptr->get_y_List_ptr()),act_y_List_copy);
			T.set_act_ptr(act_node_ptr);
			decompose_subtreenode(T,act_x_List_copy,act_y_List_copy,*new_leaf_List_ptr);
		}
		help_ptr = act_leaf_List_ptr;
		act_leaf_List_ptr = new_leaf_List_ptr;
		new_leaf_List_ptr = help_ptr;
	}
}


void NewMultipoleMethod::make_copy_and_init_Lists(
	List<ParticleInfo>& L_x_orig,
	List<ParticleInfo>& L_x_copy,
	List<ParticleInfo>& L_y_orig,
	List<ParticleInfo>& L_y_copy)
{
	ListIterator<ParticleInfo> origin_x_item,copy_x_item,origin_y_item,copy_y_item,
							new_cross_ref_item;
	ParticleInfo P_x_orig,P_y_orig,P_x_copy,P_y_copy;
	bool L_x_orig_traversed = false;
	bool L_y_orig_traversed = false;

	L_x_copy.clear();
	L_y_copy.clear();

	origin_x_item = L_x_orig.begin();
	while (!L_x_orig_traversed) {
		//reset values
		P_x_orig = *origin_x_item;
		P_x_orig.set_subList_ptr(nullptr); //clear subList_ptr
		P_x_orig.set_copy_item(nullptr);   //clear copy_item
		P_x_orig.unmark(); //unmark this element
		P_x_orig.set_tmp_cross_ref_item(nullptr);//clear tmp_cross_ref_item

		//update L_x_copy
		P_x_copy = P_x_orig;
		L_x_copy.pushBack(P_x_copy);

		//update L_x_orig
		P_x_orig.set_copy_item(L_x_copy.rbegin());
		*origin_x_item = P_x_orig;

		if(origin_x_item != L_x_orig.rbegin())
			origin_x_item = L_x_orig.cyclicSucc(origin_x_item);
		else
			L_x_orig_traversed = true;
	}

	origin_y_item = L_y_orig.begin();
	while (!L_y_orig_traversed) {
		//reset values
		P_y_orig = *origin_y_item;
		P_y_orig.set_subList_ptr(nullptr); //clear subList_ptr
		P_y_orig.set_copy_item(nullptr);   //clear copy_item
		P_y_orig.set_tmp_cross_ref_item(nullptr);//clear tmp_cross_ref_item
		P_y_orig.unmark(); //unmark this element

		//update L_x(y)_copy
		P_y_copy = P_y_orig;
		new_cross_ref_item = (*P_y_orig.get_cross_ref_item()).get_copy_item();
		P_y_copy.set_cross_ref_item(new_cross_ref_item);
		L_y_copy.pushBack(P_y_copy);
		P_x_copy = *new_cross_ref_item;
		P_x_copy.set_cross_ref_item(L_y_copy.rbegin());
		*new_cross_ref_item = P_x_copy;

		//update L_y_orig
		P_y_orig.set_copy_item(L_y_copy.rbegin());
		*origin_y_item = P_y_orig;

		if(origin_y_item != L_y_orig.rbegin())
			origin_y_item = L_y_orig.cyclicSucc(origin_y_item);
		else
			L_y_orig_traversed = true;
	}
}


void NewMultipoleMethod::build_up_root_node(
	const Graph& G,
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T)
{
	T.init_tree();
	T.get_root_ptr()->set_Sm_level(0);
	T.get_root_ptr()->set_Sm_downleftcorner(down_left_corner);
	T.get_root_ptr()->set_Sm_boxlength(boxlength);
	//allocate space for L_x and L_y List of the root node
	T.get_root_ptr()->set_x_List_ptr(new List<ParticleInfo>);
	T.get_root_ptr()->set_y_List_ptr(new List<ParticleInfo>);
	create_sorted_coordinate_Lists(G, A, *(T.get_root_ptr()->get_x_List_ptr()), *(T.get_root_ptr()->get_y_List_ptr()));
}


void NewMultipoleMethod::create_sorted_coordinate_Lists(
	const Graph& G,
	NodeArray <NodeAttributes>& A,
	List<ParticleInfo>& L_x,
	List<ParticleInfo>& L_y)
{
	ParticleInfo P_x,P_y;
	ListIterator<ParticleInfo> x_item,y_item;

	// build up L_x,L_y and link the Lists
	for (node v : G.nodes) {
		P_x.set_x_y_coord(A[v].get_x());
		P_y.set_x_y_coord(A[v].get_y());
		P_x.set_vertex(v);
		P_y.set_vertex(v);
		L_x.pushBack(P_x);
		L_y.pushBack(P_y);
		P_x.set_cross_ref_item(L_y.rbegin());
		P_y.set_cross_ref_item(L_x.rbegin());
		*L_x.rbegin() = P_x;
		*L_y.rbegin() = P_y;
	}

	//sort L_x and update the links of L_y
	ParticleInfoComparer comp;
	L_x.quicksort(comp);//Quicksort L_x

	for(x_item = L_x.begin(); x_item.valid();++x_item)
	{
		y_item = (*x_item).get_cross_ref_item();
		P_y = *y_item;
		P_y.set_cross_ref_item(x_item);
		*y_item = P_y;
	}

	//sort L_y and update the links of L_x
	L_y.quicksort(comp);//Quicksort L_x

	for(y_item = L_y.begin(); y_item.valid();++y_item)
	{
		x_item = (*y_item).get_cross_ref_item();
		P_x = *x_item;
		P_x.set_cross_ref_item(y_item);
		*x_item = P_x;
	}
}


void NewMultipoleMethod::decompose_subtreenode(
	QuadTreeNM& T,
	List<ParticleInfo>& act_x_List_copy,
	List<ParticleInfo>& act_y_List_copy,
	List<QuadTreeNodeNM*>& new_leaf_List)
{
	QuadTreeNodeNM* act_ptr = T.get_act_ptr();
	int act_particle_number = act_ptr->get_x_List_ptr()->size();
	List<ParticleInfo> *L_x_l_ptr,*L_x_r_ptr,*L_x_lb_ptr,*L_x_rb_ptr,*L_x_lt_ptr,
		*L_x_rt_ptr;
	List<ParticleInfo> *L_y_l_ptr,*L_y_r_ptr,*L_y_lb_ptr,*L_y_rb_ptr,*L_y_lt_ptr,
		*L_y_rt_ptr;

	L_x_l_ptr = L_x_r_ptr = L_x_lb_ptr = L_x_lt_ptr = L_x_rb_ptr = L_x_rt_ptr = nullptr;
	L_y_l_ptr = L_y_r_ptr = L_y_lb_ptr = L_y_lt_ptr = L_y_rb_ptr = L_y_rt_ptr = nullptr;

	DPoint min, max;
	calculate_boundaries_of_act_node(T.get_act_ptr(), min, max);
	find_small_cell(T.get_act_ptr(), DPoint(min.m_x, min.m_y), DPoint(max.m_x, max.m_y));

	if (act_particle_number > particles_in_leaves()
	 && (max.m_x - min.m_x >= MIN_BOX_LENGTH
	  || max.m_y - min.m_y >= MIN_BOX_LENGTH)) {
		// recursive calls for the half of the quad that contains the most particles

		split(act_ptr, L_x_l_ptr, L_y_l_ptr, L_x_r_ptr, L_y_r_ptr, true);
		if ((L_x_r_ptr == nullptr)
		 || (L_x_l_ptr != nullptr && L_x_l_ptr->size() > L_x_r_ptr->size())) { // left half contains more particles
			split(act_ptr, L_x_lb_ptr, L_y_lb_ptr, L_x_lt_ptr, L_y_lt_ptr, false);
			if ((L_x_lt_ptr == nullptr)
			 || (L_x_lb_ptr != nullptr && L_x_lb_ptr->size() > L_x_lt_ptr->size())) {
				T.create_new_lb_child(L_x_lb_ptr,L_y_lb_ptr);
				T.go_to_lb_child();
			} else {
				T.create_new_lt_child(L_x_lt_ptr,L_y_lt_ptr);
				T.go_to_lt_child();
			}
		} else { // right half contains more particles
			split(act_ptr, L_x_rb_ptr, L_y_rb_ptr, L_x_rt_ptr, L_y_rt_ptr, false);
			if ((L_x_rt_ptr == nullptr)
			 || (L_x_rb_ptr != nullptr && L_x_rb_ptr->size() > L_x_rt_ptr->size())) {
				T.create_new_rb_child(L_x_rb_ptr,L_y_rb_ptr);
				T.go_to_rb_child();
			} else {
				T.create_new_rt_child(L_x_rt_ptr,L_y_rt_ptr);
				T.go_to_rt_child();
			}
		}
		decompose_subtreenode(T, act_x_List_copy, act_y_List_copy, new_leaf_List);
		T.go_to_father();

		// build up the rest of the quad-subLists
		if (L_x_l_ptr != nullptr
		 && L_x_lb_ptr == nullptr
		 && L_x_lt_ptr == nullptr
		 && !act_ptr->child_lb_exists()
		 && !act_ptr->child_lt_exists()) {
			split_in_y_direction(act_ptr, L_x_l_ptr, L_x_lb_ptr, L_x_lt_ptr, L_y_l_ptr, L_y_lb_ptr, L_y_lt_ptr);
		} else
		if (L_x_r_ptr != nullptr
		 && L_x_rb_ptr == nullptr
		 && L_x_rt_ptr == nullptr
		 && !act_ptr->child_rb_exists()
		 && !act_ptr->child_rt_exists()) {
			split_in_y_direction(act_ptr, L_x_r_ptr, L_x_rb_ptr, L_x_rt_ptr, L_y_r_ptr, L_y_rb_ptr, L_y_rt_ptr);
		}

		// create rest of the childnodes
		auto add_leaf = [&] {
			new_leaf_List.pushBack(T.get_act_ptr());
			T.go_to_father();
		};
		if (!act_ptr->child_lb_exists() && L_x_lb_ptr != nullptr) {
			T.create_new_lb_child(L_x_lb_ptr, L_y_lb_ptr);
			T.go_to_lb_child();
			add_leaf();
		}
		if (!act_ptr->child_lt_exists() && L_x_lt_ptr != nullptr) {
			T.create_new_lt_child(L_x_lt_ptr, L_y_lt_ptr);
			T.go_to_lt_child();
			add_leaf();
		}
		if (!act_ptr->child_rb_exists() && L_x_rb_ptr != nullptr) {
			T.create_new_rb_child(L_x_rb_ptr, L_y_rb_ptr);
			T.go_to_rb_child();
			add_leaf();
		}
		if (!act_ptr->child_rt_exists() && L_x_rt_ptr != nullptr) {
			T.create_new_rt_child(L_x_rt_ptr, L_y_rt_ptr);
			T.go_to_rt_child();
			add_leaf();
		}

		// reset act_ptr->set_x(y)_List_ptr to avoid multiple deleting of dynamic memory;
		// (only if *act_ptr is a leaf of T the reserved space is freed (and this is
		// sufficient !!!))
		act_ptr->set_x_List_ptr(nullptr);
		act_ptr->set_y_List_ptr(nullptr);
	} else { // a leaf or machine precision is reached:
		// The List contained_nodes is set for *act_ptr and the information of
		// act_x_List_copy and act_y_List_copy is used to insert particles into the
		// shorter Lists of previous touched treenodes;additionaly the dynamical allocated
		// space for *act_ptr->get_x(y)_List_ptr() is freed.

		// set List contained nodes
		List<node> list;

		for (const ParticleInfo &pi : *act_ptr->get_x_List_ptr()) {
			list.pushBack(pi.get_vertex());
		}
		T.get_act_ptr()->set_contained_nodes(list);

		// insert particles into previous touched Lists
		build_up_sorted_subLists(act_x_List_copy,act_y_List_copy);

		// free allocated space for *act_ptr->get_x(y)_List_ptr()
		act_ptr->get_x_List_ptr()->clear(); // free used space for old L_x,L_y Lists
		act_ptr->get_y_List_ptr()->clear();
	}
}

inline void NewMultipoleMethod::calculate_boundaries_of_act_node(QuadTreeNodeNM* act_ptr, DPoint &min, DPoint &max)
{
	List<ParticleInfo>* L_x_ptr = act_ptr->get_x_List_ptr();
	List<ParticleInfo>* L_y_ptr = act_ptr->get_y_List_ptr();

	min = DPoint((*L_x_ptr->begin()).get_x_y_coord(), (*L_y_ptr->begin()).get_x_y_coord());
	max = DPoint((*L_x_ptr->rbegin()).get_x_y_coord(), (*L_y_ptr->rbegin()).get_x_y_coord());
}

bool NewMultipoleMethod::quadHelper(DPoint min, DPoint max, DPoint bottomleft, DPoint topright, QuadTreeNodeNM* act_ptr) {
	bottomleft += act_ptr->get_Sm_downleftcorner();
	topright += act_ptr->get_Sm_downleftcorner();
	return (bottomleft.m_x <= min.m_x && max.m_x < topright.m_x && bottomleft.m_y <= min.m_y && max.m_y < topright.m_y)
	    || (min == max && max == topright && topright == bottomleft);
}

bool NewMultipoleMethod::in_lt_quad(QuadTreeNodeNM* act_ptr, DPoint min, DPoint max) {
	double lo = act_ptr->get_Sm_boxlength() / 2;
	double hi = act_ptr->get_Sm_boxlength();
	return quadHelper(min, max, DPoint(0, lo), DPoint(lo, hi), act_ptr);
}

bool NewMultipoleMethod::in_rt_quad(QuadTreeNodeNM* act_ptr, DPoint min, DPoint max) {
	double lo = act_ptr->get_Sm_boxlength() / 2;
	double hi = act_ptr->get_Sm_boxlength();
	return quadHelper(min, max, DPoint(lo, lo), DPoint(hi, hi), act_ptr);
}

bool NewMultipoleMethod::in_lb_quad(QuadTreeNodeNM* act_ptr, DPoint min, DPoint max) {
	double lo = act_ptr->get_Sm_boxlength() / 2;
	return quadHelper(min, max, DPoint(0, 0), DPoint(lo, lo), act_ptr);
}

bool NewMultipoleMethod::in_rb_quad(QuadTreeNodeNM* act_ptr, DPoint min, DPoint max) {
	double lo = act_ptr->get_Sm_boxlength() / 2;
	double hi = act_ptr->get_Sm_boxlength();
	return quadHelper(min, max, DPoint(lo, 0), DPoint(hi, lo), act_ptr);
}

void NewMultipoleMethod::split(
	QuadTreeNodeNM* act_ptr,
	List<ParticleInfo>*& L_x_left_ptr,
	List<ParticleInfo>*& L_y_left_ptr,
	List<ParticleInfo>*& L_x_right_ptr,
	List<ParticleInfo>*& L_y_right_ptr,
	bool isHorizontal)
{
	double mid_coord;
	List<ParticleInfo>* this_ptr;
	if (isHorizontal) {
		this_ptr = act_ptr->get_x_List_ptr();
		mid_coord = act_ptr->get_Sm_downleftcorner().m_x;
	} else {
		this_ptr = act_ptr->get_y_List_ptr();
		mid_coord = act_ptr->get_Sm_downleftcorner().m_y;
	}
	mid_coord += act_ptr->get_Sm_boxlength() / 2;

	auto state = traverse(*this_ptr, mid_coord);

	//get the L_x(y) Lists of the bigger half (from *act_ptr->get_x(y)_List_ptr))
	//and make entries in L_x_copy,L_y_copy for the smaller halfs

	if (state.leftEmpty) {
		L_x_left_ptr = nullptr;
		L_y_left_ptr = nullptr;
		L_x_right_ptr = act_ptr->get_x_List_ptr();
		L_y_right_ptr = act_ptr->get_y_List_ptr();
	} else if (state.rightEmpty) {
		L_x_left_ptr = act_ptr->get_x_List_ptr();
		L_y_left_ptr = act_ptr->get_y_List_ptr();
		L_x_right_ptr = nullptr;
		L_y_right_ptr = nullptr;
	} else {
		delete_subLists(act_ptr, L_x_left_ptr, L_y_left_ptr, L_x_right_ptr, L_y_right_ptr, state.lastLeft, state.leftLarger, isHorizontal);
	}
}

void NewMultipoleMethod::delete_subLists(
	QuadTreeNodeNM* act_ptr,
	List<ParticleInfo>*& L_x_left_ptr,
	List<ParticleInfo>*& L_y_left_ptr,
	List<ParticleInfo>*& L_x_right_ptr,
	List<ParticleInfo>*& L_y_right_ptr,
	ListIterator<ParticleInfo> last_left_item,
	bool deleteRight,
	bool isHorizontal)
{
	ParticleInfo act_p_info, p_in_L_x_info, p_in_L_y_info, del_p_info;
	ListIterator<ParticleInfo> act_item, p_in_L_x_item, p_in_L_y_item, del_item;

	// figure out the right settings if we want to delete right/left
	// and if we go horizontal/vertical
	L_x_right_ptr = act_ptr->get_x_List_ptr();
	L_y_right_ptr = act_ptr->get_y_List_ptr();
	L_x_left_ptr = new List<ParticleInfo>;
	L_y_left_ptr = new List<ParticleInfo>;

	List<ParticleInfo>** x_ptr = &L_x_left_ptr;
	List<ParticleInfo>** y_ptr = &L_y_left_ptr;
	List<ParticleInfo>** x_opposite_ptr = &L_x_right_ptr;
	List<ParticleInfo>** y_opposite_ptr = &L_y_right_ptr;
	if (deleteRight) {
		std::swap(L_x_left_ptr, L_x_right_ptr);
		std::swap(L_y_left_ptr, L_y_right_ptr);
		std::swap(x_ptr, x_opposite_ptr);
		std::swap(y_ptr, y_opposite_ptr);
	}

	List<ParticleInfo>** this_dir_ptr = y_opposite_ptr;
	List<ParticleInfo>** that_dir_ptr = x_opposite_ptr;
	std::function<ListIterator<ParticleInfo>(const ParticleInfo &)>
	  xIter = [](const ParticleInfo &info) { return (*info.get_cross_ref_item()).get_copy_item(); },
	  yIter = [](const ParticleInfo &info) { return info.get_copy_item(); };
	if (isHorizontal) {
		std::swap(this_dir_ptr, that_dir_ptr);
		std::swap(xIter, yIter);
	}

	std::function<ListIterator<ParticleInfo>()> last_iter;
	if (deleteRight) {
		act_item = (*this_dir_ptr)->cyclicSucc(last_left_item);
		last_iter = [&] { return (*this_dir_ptr)->rbegin(); };
	} else {
		act_item = (*this_dir_ptr)->begin();
		last_iter = [&] { return last_left_item; };
	}

	// the actual loop
	bool last_item_reached = false;
	while (!last_item_reached) {
		act_p_info = *act_item;
		del_item = act_item;
		del_p_info = act_p_info;

		// save references for *L_x(y)_right(left)_ptr in L_x(y)_copy
		p_in_L_x_item = xIter(act_p_info);
		p_in_L_x_info = *p_in_L_x_item;
		p_in_L_x_info.set_subList_ptr(*x_ptr);
		*p_in_L_x_item = p_in_L_x_info;

		p_in_L_y_item = yIter(act_p_info);
		p_in_L_y_info = *p_in_L_y_item;
		p_in_L_y_info.set_subList_ptr(*y_ptr);
		*p_in_L_y_item = p_in_L_y_info;

		if (act_item != last_iter()) {
			act_item = (*this_dir_ptr)->cyclicSucc(act_item);
		} else {
			last_item_reached = true;
		}

		// create *L_x(y)_left(right)_ptr
		(*that_dir_ptr)->del(del_p_info.get_cross_ref_item());
		(*this_dir_ptr)->del(del_item);
	}
}

void NewMultipoleMethod::split_in_y_direction(
	QuadTreeNodeNM* act_ptr,
	List<ParticleInfo>*& L_x_ptr,
	List<ParticleInfo>*& L_x_b_ptr,
	List<ParticleInfo>*& L_x_t_ptr,
	List<ParticleInfo>*& L_y_ptr,
	List<ParticleInfo>*& L_y_b_ptr,
	List<ParticleInfo>*& L_y_t_ptr)
{
	//traverse *L_y_ptr from left and right
	auto state = traverse(*L_y_ptr, act_ptr->get_Sm_downleftcorner().m_y + act_ptr->get_Sm_boxlength() / 2);

	if (state.leftEmpty) {
		L_x_b_ptr = nullptr;
		L_y_b_ptr = nullptr;
		L_x_t_ptr = L_x_ptr;
		L_y_t_ptr = L_y_ptr;
	} else if (state.rightEmpty) {
		L_x_b_ptr = L_x_ptr;
		L_y_b_ptr = L_y_ptr;
		L_x_t_ptr = nullptr;
		L_y_t_ptr = nullptr;
	} else {
		move_subLists_vertical(L_x_ptr, L_x_b_ptr, L_x_t_ptr, L_y_ptr, L_y_b_ptr, L_y_t_ptr, state.lastLeft, state.leftLarger);
	}
}


void NewMultipoleMethod::move_subLists_vertical(
	List<ParticleInfo>*& L_x_ptr,
	List<ParticleInfo>*& L_x_l_ptr,
	List<ParticleInfo>*& L_x_r_ptr,
	List<ParticleInfo>*& L_y_ptr,
	List<ParticleInfo>*& L_y_l_ptr,
	List<ParticleInfo>*& L_y_r_ptr,
	ListIterator<ParticleInfo> last_left_item,
	bool moveRight)
{
	ParticleInfo p_in_L_x_info, p_in_L_y_info;
	ListIterator<ParticleInfo> p_in_L_x_item, p_in_L_y_item, del_item;
	bool last_item_reached = false;

	L_x_l_ptr = new List<ParticleInfo>;
	L_y_l_ptr = new List<ParticleInfo>;
	L_x_r_ptr = L_x_ptr;
	L_y_r_ptr = L_y_ptr;

	List<ParticleInfo>** this_ptr = &L_y_l_ptr;
	List<ParticleInfo>** that_ptr = &L_y_r_ptr;

	std::function<ListIterator<ParticleInfo>()> last_iter;
	if (moveRight) {
		std::swap(L_x_l_ptr, L_x_r_ptr);
		std::swap(L_y_l_ptr, L_y_r_ptr);
		std::swap(this_ptr, that_ptr);

		last_iter = [&] { return L_y_l_ptr->rbegin(); };
		p_in_L_y_item = L_y_l_ptr->cyclicSucc(last_left_item);
	} else {
		last_iter = [&] { return last_left_item; };
		p_in_L_y_item = L_y_r_ptr->begin();
	}

	// build up the L_y_Lists and update crossreferences in *L_x_l_ptr / *L_x_r_ptr
	while (!last_item_reached) {
		p_in_L_y_info = *p_in_L_y_item;
		del_item = p_in_L_y_item;

		// create *L_x(y)_l_ptr / *L_x(y)_r_ptr
		(*this_ptr)->pushBack(p_in_L_y_info);
		p_in_L_x_item = p_in_L_y_info.get_cross_ref_item();
		p_in_L_x_info = *p_in_L_x_item;
		p_in_L_x_info.set_cross_ref_item((*this_ptr)->rbegin());

		p_in_L_x_info.mark(); //mark this element of the List
		*p_in_L_x_item = p_in_L_x_info;

		if (p_in_L_y_item != last_iter()) {
			p_in_L_y_item = (*that_ptr)->cyclicSucc(p_in_L_y_item);
		} else {
			last_item_reached = true;
		}

		//create *L_y_l_ptr / *L_y_r_ptr
		(*that_ptr)->del(del_item);
	}

	// build up the L_x Lists and update crossreferences in *L_y_l_ptr / *L_y_r_ptr
	last_item_reached = false;
	this_ptr = &L_x_l_ptr;
	that_ptr = &L_x_r_ptr;
	if (moveRight) {
		std::swap(this_ptr, that_ptr);
		p_in_L_x_item = L_x_l_ptr->begin();
	} else {
		p_in_L_x_item = L_x_r_ptr->begin();
	}

	while (!last_item_reached) {
		del_item = p_in_L_x_item;

		if ((*del_item).is_marked()) {
			p_in_L_x_info = *p_in_L_x_item;
			p_in_L_x_info.unmark();
			(*this_ptr)->pushBack(p_in_L_x_info);
			p_in_L_y_item = p_in_L_x_info.get_cross_ref_item();
			p_in_L_y_info = *p_in_L_y_item;
			p_in_L_y_info.set_cross_ref_item((*this_ptr)->rbegin());
			*p_in_L_y_item = p_in_L_y_info;
		}

		if (p_in_L_x_item != (*that_ptr)->rbegin()) {
			p_in_L_x_item = (*that_ptr)->cyclicSucc(p_in_L_x_item);
		} else {
			last_item_reached = true;
		}

		// create *L_x_r_ptr
		if ((*del_item).is_marked()) {
			(*that_ptr)->del(del_item);
		}
	}
}

void NewMultipoleMethod::build_up_sorted_subLists(
	List<ParticleInfo>& L_x_copy,
	List<ParticleInfo>& L_y_copy)
{
	ParticleInfo P_x, P_y;
	List<ParticleInfo>  *L_x_ptr, *L_y_ptr;
	ListIterator<ParticleInfo> it, new_cross_ref_item;

	for (it = L_x_copy.begin(); it.valid(); ++it) {
		if ((*it).get_subList_ptr() != nullptr)
		{
			//reset values
			P_x = *it;
			L_x_ptr = P_x.get_subList_ptr();
			P_x.set_subList_ptr(nullptr); //clear subList_ptr
			P_x.set_copy_item(nullptr);   //clear copy_item
			P_x.unmark(); //unmark this element
			P_x.set_tmp_cross_ref_item(nullptr);//clear tmp_cross_ref_item

			//update *L_x_ptr
			L_x_ptr->pushBack(P_x);

			//update L_x_copy
			P_x.set_tmp_cross_ref_item(L_x_ptr->rbegin());
			*it = P_x;
		}
	}

	for (it = L_y_copy.begin(); it.valid(); ++it) {
		if ((*it).get_subList_ptr() != nullptr)
		{
			//reset values
			P_y = *it;
			L_y_ptr = P_y.get_subList_ptr();
			P_y.set_subList_ptr(nullptr); //clear subList_ptr
			P_y.set_copy_item(nullptr);   //clear copy_item
			P_y.unmark(); //unmark this element
			P_y.set_tmp_cross_ref_item(nullptr);//clear tmp_cross_ref_item

			//update *L_x(y)_ptr

			new_cross_ref_item = (*P_y.get_cross_ref_item()).get_tmp_cross_ref_item();
			P_y.set_cross_ref_item(new_cross_ref_item);
			L_y_ptr->pushBack(P_y);
			P_x = *new_cross_ref_item;
			P_x.set_cross_ref_item(L_y_ptr->rbegin());
			*new_cross_ref_item = P_x;
		}
	}
}


void NewMultipoleMethod::build_up_red_quad_tree_subtree_by_subtree(
	const Graph& G,
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T)
{
	List<QuadTreeNodeNM*> act_subtree_root_List,new_subtree_root_List;
	List<QuadTreeNodeNM*> *act_subtree_root_List_ptr,*new_subtree_root_List_ptr,*help_ptr;
	QuadTreeNodeNM *subtree_root_ptr;

	build_up_root_vertex(G,T);

	act_subtree_root_List.clear();
	new_subtree_root_List.clear();
	act_subtree_root_List.pushFront(T.get_root_ptr());
	act_subtree_root_List_ptr = &act_subtree_root_List;
	new_subtree_root_List_ptr = &new_subtree_root_List;

	while(!act_subtree_root_List_ptr->empty())
	{
		while(!act_subtree_root_List_ptr->empty())
		{
			subtree_root_ptr = act_subtree_root_List_ptr->popFrontRet();
			construct_subtree(A,T,subtree_root_ptr,*new_subtree_root_List_ptr);
		}
		help_ptr = act_subtree_root_List_ptr;
		act_subtree_root_List_ptr = new_subtree_root_List_ptr;
		new_subtree_root_List_ptr = help_ptr;
	}
}


void NewMultipoleMethod::build_up_root_vertex(const Graph&G, QuadTreeNM& T)
{
	T.init_tree();
	T.get_root_ptr()->set_Sm_level(0);
	T.get_root_ptr()->set_Sm_downleftcorner(down_left_corner);
	T.get_root_ptr()->set_Sm_boxlength(boxlength);
	T.get_root_ptr()->set_particlenumber_in_subtree(G.numberOfNodes());
	for(node v : G.nodes)
		T.get_root_ptr()->pushBack_contained_nodes(v);
}


void NewMultipoleMethod::construct_subtree(
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T,
	QuadTreeNodeNM *subtree_root_ptr,
	List<QuadTreeNodeNM*>& new_subtree_root_List)
{
	int n = subtree_root_ptr->get_particlenumber_in_subtree();
	int subtree_depth =  static_cast<int>(max(1.0,floor(Math::log4(n))-2.0));
	int maxindex=1;

	for(int i=1; i<=subtree_depth; i++)
		maxindex *= 2;
	double subtree_min_boxlength = subtree_root_ptr->get_Sm_boxlength()/maxindex;

	if (subtree_min_boxlength >=  MIN_BOX_LENGTH) {
		Array2D<QuadTreeNodeNM*> leaf_ptr(0,maxindex-1,0,maxindex-1);
		T.set_act_ptr(subtree_root_ptr);
		if (find_smallest_quad(A,T)) //not all nodes have the same position
		{
			construct_complete_subtree(T,subtree_depth,leaf_ptr,0,0,0);
			set_contained_nodes_for_leaves(A,subtree_root_ptr,leaf_ptr,maxindex);
			T.set_act_ptr(subtree_root_ptr);
			set_particlenumber_in_subtree_entries(T);
			T.set_act_ptr(subtree_root_ptr);
			construct_reduced_subtree(A,T,new_subtree_root_List);
		}
	}
}


void NewMultipoleMethod::construct_complete_subtree(
	QuadTreeNM& T,
	int subtree_depth,
	Array2D<QuadTreeNodeNM*>& leaf_ptr,
	int act_depth,
	int act_x_index,
	int act_y_index)
{
	if (act_depth < subtree_depth) {
		T.create_new_lt_child();
		T.create_new_rt_child();
		T.create_new_lb_child();
		T.create_new_rb_child();

		T.go_to_lt_child();
		construct_complete_subtree(T,subtree_depth,leaf_ptr,act_depth+1,2*act_x_index,
						2*act_y_index+1);
		T.go_to_father();

		T.go_to_rt_child();
		construct_complete_subtree(T,subtree_depth,leaf_ptr,act_depth+1,2*act_x_index+1,
						2*act_y_index+1);
		T.go_to_father();

		T.go_to_lb_child();
		construct_complete_subtree(T,subtree_depth,leaf_ptr,act_depth+1,2*act_x_index,
						2*act_y_index);
		T.go_to_father();

		T.go_to_rb_child();
		construct_complete_subtree(T,subtree_depth,leaf_ptr,act_depth+1,2*act_x_index+1,
						2*act_y_index);
		T.go_to_father();
	} else if (act_depth == subtree_depth) {
		leaf_ptr(act_x_index,act_y_index) = T.get_act_ptr();
	} else {
		std::cout<<"Error NewMultipoleMethod::construct_complete_subtree()"<<std::endl;
	}
}


void NewMultipoleMethod::set_contained_nodes_for_leaves(
	NodeArray<NodeAttributes> &A,
	QuadTreeNodeNM* subtree_root_ptr,
	Array2D<QuadTreeNodeNM*> &leaf_ptr,
	int maxindex)
{
	double minboxlength = subtree_root_ptr->get_Sm_boxlength()/maxindex;

	while(!subtree_root_ptr->contained_nodes_empty())
	{
		node v = subtree_root_ptr->pop_contained_nodes();
		double xcoord = A[v].get_x()-subtree_root_ptr->get_Sm_downleftcorner().m_x;
		double ycoord = A[v].get_y()-subtree_root_ptr->get_Sm_downleftcorner().m_y;
		int x_index = int(xcoord/minboxlength);
		int y_index = int(ycoord/minboxlength);
		QuadTreeNodeNM *act_ptr = leaf_ptr(x_index, y_index);
		act_ptr->pushBack_contained_nodes(v);
		act_ptr->set_particlenumber_in_subtree(act_ptr->get_particlenumber_in_subtree()+1);
	}
}


void NewMultipoleMethod::set_particlenumber_in_subtree_entries(QuadTreeNM& T)
{
	if (!T.get_act_ptr()->is_leaf()) {
		T.get_act_ptr()->set_particlenumber_in_subtree(0);

		if (T.get_act_ptr()->child_lt_exists())
		{
			T.go_to_lt_child();
			set_particlenumber_in_subtree_entries(T);
			T.go_to_father();
			int child_nr = T.get_act_ptr()->get_child_lt_ptr()->get_particlenumber_in_subtree();
			T.get_act_ptr()->set_particlenumber_in_subtree(child_nr + T.get_act_ptr()->
				get_particlenumber_in_subtree());
		}
		if (T.get_act_ptr()->child_rt_exists())
		{
			T.go_to_rt_child();
			set_particlenumber_in_subtree_entries(T);
			T.go_to_father();
			int child_nr = T.get_act_ptr()->get_child_rt_ptr()->get_particlenumber_in_subtree();
			T.get_act_ptr()->set_particlenumber_in_subtree(child_nr + T.get_act_ptr()->
				get_particlenumber_in_subtree());
		}
		if (T.get_act_ptr()->child_lb_exists())
		{
			T.go_to_lb_child();
			set_particlenumber_in_subtree_entries(T);
			T.go_to_father();
			int child_nr = T.get_act_ptr()->get_child_lb_ptr()->get_particlenumber_in_subtree();
			T.get_act_ptr()->set_particlenumber_in_subtree(child_nr + T.get_act_ptr()->
				get_particlenumber_in_subtree());
		}
		if (T.get_act_ptr()->child_rb_exists())
		{
			T.go_to_rb_child();
			set_particlenumber_in_subtree_entries(T);
			T.go_to_father();
			int child_nr = T.get_act_ptr()->get_child_rb_ptr()->get_particlenumber_in_subtree();
			T.get_act_ptr()->set_particlenumber_in_subtree(child_nr + T.get_act_ptr()->
				get_particlenumber_in_subtree());
		}
	}
}

void NewMultipoleMethod::construct_reduced_subtree(
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T,
	List<QuadTreeNodeNM*>& new_subtree_root_List)
{
	do
	{
		QuadTreeNodeNM* act_ptr = T.get_act_ptr();
		delete_empty_subtrees(T);
		T.set_act_ptr(act_ptr);
	}
	while(check_and_delete_degenerated_node(T)) ;

	if(!T.get_act_ptr()->is_leaf() && T.get_act_ptr()->get_particlenumber_in_subtree()
		<=  particles_in_leaves())
	{
		delete_sparse_subtree(T,T.get_act_ptr());
	}

	if(T.get_act_ptr()->is_leaf() && T.get_act_ptr()->get_particlenumber_in_subtree() > particles_in_leaves()) {
		// push leaves that contain many particles
		new_subtree_root_List.pushBack(T.get_act_ptr());
	} else if (T.get_act_ptr()->is_leaf() && T.get_act_ptr()->get_particlenumber_in_subtree() <= particles_in_leaves()) {
		// find smallest quad for leaves of T
		find_smallest_quad(A,T);
	} else if (!T.get_act_ptr()->is_leaf()) { // recursive calls
		if (T.get_act_ptr()->child_lt_exists()) {
			T.go_to_lt_child();
			construct_reduced_subtree(A,T,new_subtree_root_List);
			T.go_to_father();
		}
		if (T.get_act_ptr()->child_rt_exists()) {
			T.go_to_rt_child();
			construct_reduced_subtree(A,T,new_subtree_root_List);
			T.go_to_father();
		}
		if (T.get_act_ptr()->child_lb_exists()) {
			T.go_to_lb_child();
			construct_reduced_subtree(A,T,new_subtree_root_List);
			T.go_to_father();
		}
		if (T.get_act_ptr()->child_rb_exists()) {
			T.go_to_rb_child();
			construct_reduced_subtree(A,T,new_subtree_root_List);
			T.go_to_father();
		}
	}
}

void NewMultipoleMethod::delete_empty_subtrees(QuadTreeNM& T)
{
	int child_part_nr;
	QuadTreeNodeNM* act_ptr = T.get_act_ptr();

	if(act_ptr->child_lt_exists())
	{
		child_part_nr = act_ptr->get_child_lt_ptr()->get_particlenumber_in_subtree();
		if(child_part_nr == 0)
		{
			T.delete_tree(act_ptr->get_child_lt_ptr());
			act_ptr->set_child_lt_ptr(nullptr);
		}
	}

	if(act_ptr->child_rt_exists())
	{
		child_part_nr = act_ptr->get_child_rt_ptr()->get_particlenumber_in_subtree();
		if(child_part_nr == 0)
		{
			T.delete_tree(act_ptr->get_child_rt_ptr());
			act_ptr->set_child_rt_ptr(nullptr);
		}
	}

	if(act_ptr->child_lb_exists())
	{
		child_part_nr = act_ptr->get_child_lb_ptr()->get_particlenumber_in_subtree();
		if(child_part_nr == 0)
		{
			T.delete_tree(act_ptr->get_child_lb_ptr());
			act_ptr->set_child_lb_ptr(nullptr);
		}
	}

	if(act_ptr->child_rb_exists())
	{
		child_part_nr = act_ptr->get_child_rb_ptr()->get_particlenumber_in_subtree();
		if(child_part_nr == 0)
		{
			T.delete_tree(act_ptr->get_child_rb_ptr());
			act_ptr->set_child_rb_ptr(nullptr);
		}
	}
}


bool NewMultipoleMethod::check_and_delete_degenerated_node(QuadTreeNM& T)
{
	QuadTreeNodeNM* delete_ptr;
	QuadTreeNodeNM* father_ptr;
	QuadTreeNodeNM* child_ptr;

	bool lt_child = T.get_act_ptr()->child_lt_exists();
	bool rt_child = T.get_act_ptr()->child_rt_exists();
	bool lb_child = T.get_act_ptr()->child_lb_exists();
	bool rb_child = T.get_act_ptr()->child_rb_exists();
	bool is_degenerated = false;

	if (lt_child && !rt_child && !lb_child && !rb_child) {
		is_degenerated = true;
		delete_ptr = T.get_act_ptr();
		child_ptr = T.get_act_ptr()->get_child_lt_ptr();
		if (T.get_act_ptr() == T.get_root_ptr()) { // special case
			T.set_root_ptr(child_ptr);
			T.set_act_ptr(T.get_root_ptr());
		} else { // usual case
			father_ptr = T.get_act_ptr()->get_father_ptr();
			child_ptr->set_father_ptr(father_ptr);
			if (father_ptr->get_child_lt_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_lt_ptr(child_ptr);
			} else if (father_ptr->get_child_rt_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_rt_ptr(child_ptr);
			} else if (father_ptr->get_child_lb_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_lb_ptr(child_ptr);
			} else if (father_ptr->get_child_rb_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_rb_ptr(child_ptr);
			} else {
				std::cout<<"Error NewMultipoleMethod::delete_degenerated_node"<<std::endl;
			}
			T.set_act_ptr(child_ptr);
		}
		delete delete_ptr;
	} else if (!lt_child && rt_child && !lb_child && !rb_child) {
		is_degenerated = true;
		delete_ptr = T.get_act_ptr();
		child_ptr = T.get_act_ptr()->get_child_rt_ptr();
		if (T.get_act_ptr() == T.get_root_ptr()) { // special case
			T.set_root_ptr(child_ptr);
			T.set_act_ptr(T.get_root_ptr());
		} else { // usual case
			father_ptr = T.get_act_ptr()->get_father_ptr();
			child_ptr->set_father_ptr(father_ptr);
			if (father_ptr->get_child_lt_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_lt_ptr(child_ptr);
			} else if (father_ptr->get_child_rt_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_rt_ptr(child_ptr);
			} else if (father_ptr->get_child_lb_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_lb_ptr(child_ptr);
			} else if (father_ptr->get_child_rb_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_rb_ptr(child_ptr);
			} else {
				std::cout<<"Error NewMultipoleMethod::delete_degenerated_node"<<std::endl;
			}
			T.set_act_ptr(child_ptr);
		}
		delete delete_ptr;
	} else if (!lt_child && !rt_child && lb_child && !rb_child) {
		is_degenerated = true;
		delete_ptr = T.get_act_ptr();
		child_ptr = T.get_act_ptr()->get_child_lb_ptr();
		if (T.get_act_ptr() == T.get_root_ptr()) { // special case
			T.set_root_ptr(child_ptr);
			T.set_act_ptr(T.get_root_ptr());
		} else { // usual case
			father_ptr = T.get_act_ptr()->get_father_ptr();
			child_ptr->set_father_ptr(father_ptr);
			if (father_ptr->get_child_lt_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_lt_ptr(child_ptr);
			} else if (father_ptr->get_child_rt_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_rt_ptr(child_ptr);
			} else if (father_ptr->get_child_lb_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_lb_ptr(child_ptr);
			} else if (father_ptr->get_child_rb_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_rb_ptr(child_ptr);
			} else {
				std::cout<<"Error NewMultipoleMethod::delete_degenerated_node"<<std::endl;
			}
			T.set_act_ptr(child_ptr);
		}
		delete delete_ptr;
	} else if(!lt_child && !rt_child && !lb_child && rb_child) {
		is_degenerated = true;
		delete_ptr = T.get_act_ptr();
		child_ptr = T.get_act_ptr()->get_child_rb_ptr();
		if (T.get_act_ptr() == T.get_root_ptr()) { // special case
			T.set_root_ptr(child_ptr);
			T.set_act_ptr(T.get_root_ptr());
		} else { // usual case
			father_ptr = T.get_act_ptr()->get_father_ptr();
			child_ptr->set_father_ptr(father_ptr);
			if (father_ptr->get_child_lt_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_lt_ptr(child_ptr);
			} else if (father_ptr->get_child_rt_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_rt_ptr(child_ptr);
			} else if (father_ptr->get_child_lb_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_lb_ptr(child_ptr);
			} else if (father_ptr->get_child_rb_ptr() == T.get_act_ptr()) {
				father_ptr->set_child_rb_ptr(child_ptr);
			} else {
				std::cout<<"Error NewMultipoleMethod::delete_degenerated_node"<<std::endl;
			}
			T.set_act_ptr(child_ptr);
		}
		delete delete_ptr;
	}
	return is_degenerated;
}

void NewMultipoleMethod::delete_sparse_subtree(QuadTreeNM& T, QuadTreeNodeNM* new_leaf_ptr)
{
	collect_contained_nodes(T,new_leaf_ptr);

	if(new_leaf_ptr->child_lt_exists())
	{
		T.delete_tree(new_leaf_ptr->get_child_lt_ptr());
		new_leaf_ptr->set_child_lt_ptr(nullptr);
	}
	if(new_leaf_ptr->child_rt_exists())
	{
		T.delete_tree(new_leaf_ptr->get_child_rt_ptr());
		new_leaf_ptr->set_child_rt_ptr(nullptr);
	}
	if(new_leaf_ptr->child_lb_exists())
	{
		T.delete_tree(new_leaf_ptr->get_child_lb_ptr());
		new_leaf_ptr->set_child_lb_ptr(nullptr);
	}
	if(new_leaf_ptr->child_rb_exists())
	{
		T.delete_tree(new_leaf_ptr->get_child_rb_ptr());
		new_leaf_ptr->set_child_rb_ptr(nullptr);
	}
}


void NewMultipoleMethod::collect_contained_nodes(QuadTreeNM& T, QuadTreeNodeNM* new_leaf_ptr)
{
	if(T.get_act_ptr()->is_leaf())
		while(!T.get_act_ptr()->contained_nodes_empty())
			new_leaf_ptr->pushBack_contained_nodes(T.get_act_ptr()->pop_contained_nodes());
	else if(T.get_act_ptr()->child_lt_exists())
	{
		T.go_to_lt_child();
		collect_contained_nodes(T,new_leaf_ptr);
		T.go_to_father();
	}
	if(T.get_act_ptr()->child_rt_exists())
	{
		T.go_to_rt_child();
		collect_contained_nodes(T,new_leaf_ptr);
		T.go_to_father();
	}
	if(T.get_act_ptr()->child_lb_exists())
	{
		T.go_to_lb_child();
		collect_contained_nodes(T,new_leaf_ptr);
		T.go_to_father();
	}
	if(T.get_act_ptr()->child_rb_exists())
	{
		T.go_to_rb_child();
		collect_contained_nodes(T,new_leaf_ptr);
		T.go_to_father();
	}
}


bool NewMultipoleMethod::find_smallest_quad(NodeArray<NodeAttributes>& A, QuadTreeNM& T)
{
	OGDF_ASSERT(!T.get_act_ptr()->contained_nodes_empty());
#if 0
	if(T.get_act_ptr()->contained_nodes_empty())
		std::cout<<"Error NewMultipoleMethod :: find_smallest_quad()"<<std::endl;
#endif
	List<node> list;
	T.get_act_ptr()->get_contained_nodes(list);
	node v = list.popFrontRet();
	DPoint min(A[v].get_x(), A[v].get_y());
	DPoint max(min);

	while (!list.empty()) {
		v = list.popFrontRet();
		Math::updateMin(min.m_x, A[v].get_x());
		Math::updateMax(max.m_x, A[v].get_x());
		Math::updateMin(min.m_y, A[v].get_y());
		Math::updateMax(max.m_y, A[v].get_y());
	}
	if (min != max) {
		find_small_cell(T.get_act_ptr(), min, max);
		return true;
	}
	return false;
}


void NewMultipoleMethod::find_small_cell_iteratively(QuadTreeNodeNM* act_ptr, DPoint min, DPoint max)
{
	while (max.m_x - min.m_x >= MIN_BOX_LENGTH
	    || max.m_y - min.m_y >= MIN_BOX_LENGTH) {
		double new_boxlength = act_ptr->get_Sm_boxlength() / 2;
		DPoint new_dlc(act_ptr->get_Sm_downleftcorner());
		if (in_lt_quad(act_ptr, min, max)) {
			new_dlc.m_y += new_boxlength;
		} else if (in_rt_quad(act_ptr, min, max)) {
			new_dlc.m_x += new_boxlength;
			new_dlc.m_y += new_boxlength;
		} else if (in_lb_quad(act_ptr, min, max)) {
			// keep downleftcorner
		} else if (in_rb_quad(act_ptr, min, max)) {
			new_dlc.m_x += new_boxlength;
		} else {
			return; // Sm cell found
		}

		act_ptr->set_Sm_level(act_ptr->get_Sm_level() + 1);
		act_ptr->set_Sm_boxlength(new_boxlength);
		act_ptr->set_Sm_downleftcorner(new_dlc);
	}
}

void NewMultipoleMethod::find_small_cell_by_formula(QuadTreeNodeNM* act_ptr, DPoint min, DPoint max)
{
	int level_offset = act_ptr->get_Sm_level();
	double Sm_boxlength;
	int j_x = max_power_of_2_index+1;
	int j_y = max_power_of_2_index+1;
	bool rectangle_is_horizontal_line = false;
	bool rectangle_is_vertical_line = false;
	bool rectangle_is_point = false;

	// shift boundaries to the origin for easy calculations
	DPoint min_old(min);
	DPoint max_old(max);

	Sm_boxlength = act_ptr->get_Sm_boxlength();
	DPoint Sm_dlc(act_ptr->get_Sm_downleftcorner());

	min -= Sm_dlc;
	max -= Sm_dlc;

	// check if iterative way has to be used
	if (min == max) {
		rectangle_is_point = true;
	} else if (min.m_x == max.m_x && min.m_y != max.m_y) {
		rectangle_is_vertical_line = true;
	} else { // min.m_x != max.m_x
		j_x = static_cast<int>(ceil(std::log2(Sm_boxlength / (max.m_x - min.m_x))));
	}

	if (min.m_x != max.m_x && min.m_y == max.m_y) {
		rectangle_is_horizontal_line = true;
	} else { // min.m_y != max.m_y
		j_y = static_cast<int>(ceil(std::log2(Sm_boxlength / (max.m_y - min.m_y))));
	}

	if (!rectangle_is_point) {
		if (!numexcept::nearly_equal(min_old.m_x - max_old.m_x, min.m_x - max.m_x)
		 || !numexcept::nearly_equal(min_old.m_y - max_old.m_y, min.m_y - max.m_y)
		 || min.m_x / Sm_boxlength < MIN_BOX_LENGTH
		 || max.m_x / Sm_boxlength < MIN_BOX_LENGTH
		 || min.m_y / Sm_boxlength < MIN_BOX_LENGTH
		 || max.m_y / Sm_boxlength < MIN_BOX_LENGTH
		 || (j_x > max_power_of_2_index && j_y > max_power_of_2_index)
		 || (j_x > max_power_of_2_index && !rectangle_is_vertical_line)
		 || (j_y > max_power_of_2_index && !rectangle_is_horizontal_line)) {
			find_small_cell_iteratively(act_ptr, min_old, max_old);
		} else { // idea of Aluru et al.
			int k, a1, a2, A, j_minus_k;
			double h1;
			int Sm_x_level{}, Sm_y_level{};
			int Sm_x_position{}, Sm_y_position{};

			if (min.m_x != max.m_x) {
				// calculate Sm_x_level and Sm_x_position
				a1 = static_cast<int>(ceil((min.m_x / Sm_boxlength) * power_of_two(j_x)));
				a2 = static_cast<int>(floor((max.m_x / Sm_boxlength) * power_of_two(j_x)));
				h1 = (Sm_boxlength / power_of_two(j_x)) * a1;

				// special cases: two tangents or left tangent and righ cutline
				// or only one cutline
				if (h1 == min.m_x || a1 == a2) {
					A = a2;
				} else { // two cutlines or a right tangent and a left cutline (usual case)
					A = a1 % 2 != 0 ? a2 : a1;
				}

				j_minus_k = static_cast<int>(std::log2(1 + (A ^ (A - 1))) - 1);
				k = j_x - j_minus_k;
				Sm_x_level = k - 1;
				Sm_x_position = a1 / power_of_two(j_x - Sm_x_level);
			}

			if (min.m_y != max.m_y) {
				// calculate Sm_y_level and Sm_y_position
				a1 = static_cast<int>(ceil((min.m_y / Sm_boxlength) * power_of_two(j_y)));
				a2 = static_cast<int>(floor((max.m_y / Sm_boxlength) * power_of_two(j_y)));
				h1 = (Sm_boxlength / power_of_two(j_y)) * a1;

				// special cases: two tangents or bottom tangent and top cutline
				if (h1 == min.m_y) {
					A = a2;
				} else if (a1 == a2) { // only one cutline
					A = a1;
				} else { // two cutlines or a top tangent and a bottom cutline (usual case)
					A = a1 % 2 != 0 ? a2 : a1;
				}

				j_minus_k = static_cast<int>(std::log2(1 + (A ^ (A - 1))) - 1);
				k = j_y - j_minus_k;
				Sm_y_level = k - 1;
				Sm_y_position = a1 / power_of_two(j_y - Sm_y_level);
			}

			IPoint Sm_position(Sm_x_position, Sm_y_position);
			int Sm_level;
			if (min.m_x != max.m_x && min.m_y != max.m_y) { // a box with area > 0
				if (Sm_x_level == Sm_y_level) {
					Sm_level = Sm_x_level;
				} else if (Sm_x_level < Sm_y_level) {
					Sm_level = Sm_x_level;
					Sm_position.m_y /= power_of_two(Sm_y_level - Sm_x_level);
				} else { // Sm_x_level > Sm_y_level
					Sm_level = Sm_y_level;
					Sm_position.m_x /= power_of_two(Sm_x_level - Sm_y_level);
				}
			} else if (min.m_x == max.m_x) { // a vertical line
				OGDF_ASSERT(min.m_y != max.m_y); // otherwise Sm_y_{level,position} is undefined
				Sm_level = Sm_y_level;
				Sm_position.m_x = static_cast<int>(floor((min.m_x * power_of_two(Sm_level)) / Sm_boxlength));
			} else { // min.m_y == max.m_y (a horizontal line)
				OGDF_ASSERT(min.m_x != max.m_x); // otherwise Sm_x_{level,position} is undefined
				Sm_level = Sm_x_level;
				Sm_position.m_y = static_cast<int>(floor((min.m_y * power_of_two(Sm_level)) / Sm_boxlength));
			}

			Sm_boxlength = Sm_boxlength / power_of_two(Sm_level);
			act_ptr->set_Sm_level(Sm_level + level_offset);
			act_ptr->set_Sm_boxlength(Sm_boxlength);
			DPoint Sm_downleftcorner(Sm_dlc.m_x + Sm_boxlength * Sm_position.m_x,
			                         Sm_dlc.m_y + Sm_boxlength * Sm_position.m_y);
			act_ptr->set_Sm_downleftcorner(Sm_downleftcorner);
		}
	}
}

inline void NewMultipoleMethod::delete_red_quad_tree_and_count_treenodes(QuadTreeNM& T)
{
	T.delete_tree(T.get_root_ptr());
}


inline void NewMultipoleMethod::form_multipole_expansions(
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T,
	List<QuadTreeNodeNM*>& quad_tree_leaves)
{
	T.set_act_ptr(T.get_root_ptr());
	form_multipole_expansion_of_subtree(A,T,quad_tree_leaves);
}


void NewMultipoleMethod::form_multipole_expansion_of_subtree(
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T,
	List<QuadTreeNodeNM*>& quad_tree_leaves)
{
	init_expansion_Lists(T.get_act_ptr());
	set_center(T.get_act_ptr());

	if (T.get_act_ptr()->is_leaf()) { // form expansions for leaf nodes
		quad_tree_leaves.pushBack(T.get_act_ptr());
		form_multipole_expansion_of_leaf_node(A,T.get_act_ptr());
	} else { // recursive calls and add shifted expansions
		if (T.get_act_ptr()->child_lt_exists()) {
			T.go_to_lt_child();
			form_multipole_expansion_of_subtree(A,T,quad_tree_leaves);
			add_shifted_expansion_to_father_expansion(T.get_act_ptr());
			T.go_to_father();
		}
		if (T.get_act_ptr()->child_rt_exists()) {
			T.go_to_rt_child();
			form_multipole_expansion_of_subtree(A,T,quad_tree_leaves);
			add_shifted_expansion_to_father_expansion(T.get_act_ptr());
			T.go_to_father();
		}
		if (T.get_act_ptr()->child_lb_exists()) {
			T.go_to_lb_child();
			form_multipole_expansion_of_subtree(A,T,quad_tree_leaves);
			add_shifted_expansion_to_father_expansion(T.get_act_ptr());
			T.go_to_father();
		}
		if (T.get_act_ptr()->child_rb_exists()) {
			T.go_to_rb_child();
			form_multipole_expansion_of_subtree(A,T,quad_tree_leaves);
			add_shifted_expansion_to_father_expansion(T.get_act_ptr());
			T.go_to_father();
		}
	}
}


inline void NewMultipoleMethod::init_expansion_Lists(QuadTreeNodeNM* act_ptr)
{
	int i;
	Array<complex<double> > nulList (precision()+1);

	for (i = 0;i<=precision();i++)
		nulList[i] = 0;

	act_ptr->set_multipole_exp(nulList,precision());
	act_ptr->set_locale_exp(nulList,precision());
}


void NewMultipoleMethod::set_center(QuadTreeNodeNM* act_ptr)
{

	const int BILLION = 1000000000;
	DPoint Sm_downleftcorner = act_ptr->get_Sm_downleftcorner();
	double Sm_boxlength = act_ptr->get_Sm_boxlength();
	double boxcenter_x_coord,boxcenter_y_coord;
	DPoint Sm_dlc;
	double rand_y;

	boxcenter_x_coord = Sm_downleftcorner.m_x + Sm_boxlength * 0.5;
	boxcenter_y_coord = Sm_downleftcorner.m_y + Sm_boxlength * 0.5;

	//for use of complex logarithm: waggle the y-coordinates a little bit
	//such that the new center is really inside the actual box and near the exact center
	rand_y = double(randomNumber(1,BILLION)+1)/(BILLION+2);//rand number in (0,1)
	boxcenter_y_coord = boxcenter_y_coord + 0.001 * Sm_boxlength * rand_y;

	complex<double> boxcenter(boxcenter_x_coord,boxcenter_y_coord);
	act_ptr->set_Sm_center(boxcenter);
}


void NewMultipoleMethod::form_multipole_expansion_of_leaf_node(
	NodeArray<NodeAttributes>& A,
	QuadTreeNodeNM* act_ptr)
{
	complex<double> Q(0, 0);
	complex<double> z_0 = act_ptr->get_Sm_center();//center of actual box
	Array<complex<double> > coef(precision() + 1);
	complex<double> z_v_minus_z_0_over_k;
	List<node> nodes_in_box;

	act_ptr->get_contained_nodes(nodes_in_box);

	Q += nodes_in_box.size();
	coef[0] = Q;

	for (int i = 1; i <= precision(); i++) {
		coef[i] = complex<double>(0, 0);
	}

	for (node v : nodes_in_box)
	{
		complex<double> z_v(A[v].get_x(), A[v].get_y());
		z_v_minus_z_0_over_k = z_v - z_0;
		for (int k = 1; k <= precision(); k++)
		{
			coef[k] += ((double(-1))*z_v_minus_z_0_over_k) / double(k);
			z_v_minus_z_0_over_k *= z_v - z_0;
		}
	}
	act_ptr->replace_multipole_exp(coef, precision());
}


void NewMultipoleMethod::add_shifted_expansion_to_father_expansion(QuadTreeNodeNM* act_ptr)
{
	QuadTreeNodeNM* father_ptr = act_ptr->get_father_ptr();
	complex<double> sum;
	complex<double> z_0,z_1;
	Array<complex<double> > z_0_minus_z_1_over (precision()+1);

	z_1 = father_ptr->get_Sm_center();
	z_0 = act_ptr->get_Sm_center();
	father_ptr->get_multipole_exp()[0] += act_ptr->get_multipole_exp()[0];

	//init z_0_minus_z_1_over
	z_0_minus_z_1_over[0] = 1;
	for(int i = 1; i<= precision(); i++)
		z_0_minus_z_1_over[i] = z_0_minus_z_1_over[i-1] * (z_0 - z_1);

	for(int k=1; k<=precision(); k++)
	{
		sum = (act_ptr->get_multipole_exp()[0]*(double(-1))*z_0_minus_z_1_over[k])/
			double(k) ;
		for(int s=1; s<=k; s++)
			sum +=  act_ptr->get_multipole_exp()[s]*z_0_minus_z_1_over[k-s]* binko(k-1,s-1);
		father_ptr->get_multipole_exp()[k] += sum;
	}
}


void NewMultipoleMethod::calculate_local_expansions_and_WSPRLS(
	NodeArray<NodeAttributes>&A,
	QuadTreeNodeNM* act_node_ptr)
{
	List<QuadTreeNodeNM*> I,L,L2,E,D1,D2,M;
	QuadTreeNodeNM *selected_node_ptr;

	I.clear();L.clear();L2.clear();D1.clear();D2.clear();M.clear();

	//Step 1: calculate Lists I (min. ill sep. set), L (interaction List of well sep.
	//nodes , they are used to form the Local Expansions from the multipole expansions),
	//L2 (non bordering leaves that have a larger or equal Sm-cell and  are ill separated;
	//empty if the actual node is a leaf)
	//calculate List D1(bordering leaves that have a larger or equal Sm-cell and are
	//ill separated) and D2 (non bordering leaves that have a larger or equal Sm-cell and
	//are ill separated;empty if the actual node is an interior node)

	//special case: act_node is the root of T
	if (act_node_ptr->is_root()) {
		E.clear();
		if(act_node_ptr->child_lt_exists())
			E.pushBack(act_node_ptr->get_child_lt_ptr());
		if(act_node_ptr->child_rt_exists())
			E.pushBack(act_node_ptr->get_child_rt_ptr());
		if(act_node_ptr->child_lb_exists())
			E.pushBack(act_node_ptr->get_child_lb_ptr());
		if(act_node_ptr->child_rb_exists())
			E.pushBack(act_node_ptr->get_child_rb_ptr());
	} else { //usual case: act_node is an interior node of T
		const QuadTreeNodeNM *father_ptr = act_node_ptr->get_father_ptr();
		father_ptr->get_D1(E); //bordering leaves of father
		father_ptr->get_I(I);  //min ill sep. nodes of father

		for (QuadTreeNodeNM *ptr : I)
			E.pushBack(ptr);
		I.clear();
	}

	while (!E.empty()) {
		selected_node_ptr = E.popFrontRet();
		if (well_separated(act_node_ptr,selected_node_ptr)) {
			L.pushBack(selected_node_ptr);
		} else if (act_node_ptr->get_Sm_level() < selected_node_ptr->get_Sm_level()) {
			I.pushBack(selected_node_ptr);
		} else if (!selected_node_ptr->is_leaf()) {
			if (selected_node_ptr->child_lt_exists()) {
				E.pushBack(selected_node_ptr->get_child_lt_ptr());
			}
			if (selected_node_ptr->child_rt_exists()) {
				E.pushBack(selected_node_ptr->get_child_rt_ptr());
			}
			if (selected_node_ptr->child_lb_exists()) {
				E.pushBack(selected_node_ptr->get_child_lb_ptr());
			}
			if (selected_node_ptr->child_rb_exists()) {
				E.pushBack(selected_node_ptr->get_child_rb_ptr());
			}
		} else if (bordering(act_node_ptr, selected_node_ptr)) {
			D1.pushBack(selected_node_ptr);
		} else if (selected_node_ptr != act_node_ptr && act_node_ptr->is_leaf()) {
			D2.pushBack(selected_node_ptr); //direct calculation (no errors produced)
		} else if (selected_node_ptr != act_node_ptr && !act_node_ptr->is_leaf()) {
			L2.pushBack(selected_node_ptr);
		}
	}

	act_node_ptr->set_I(I);
	act_node_ptr->set_D1(D1);
	act_node_ptr->set_D2(D2);

	// Step 2: add local expansions from father(act_node_ptr) and calculate locale
	// expansions for all nodes in L
	if (!act_node_ptr->is_root()) {
		add_shifted_local_exp_of_parent(act_node_ptr);
	}

	for (QuadTreeNodeNM *ptr : L)
		add_local_expansion(ptr,act_node_ptr);

	// Step 3: calculate locale expansions for all nodes in D2 (simpler than in Step 2)

	for (QuadTreeNodeNM *ptr : L2)
		add_local_expansion_of_leaf(A,ptr,act_node_ptr);

	// Step 4: recursive calls if act_node is not a leaf
	if (!act_node_ptr->is_leaf()) {
		if(act_node_ptr->child_lt_exists()) {
			calculate_local_expansions_and_WSPRLS(A,act_node_ptr->get_child_lt_ptr());
		}
		if(act_node_ptr->child_rt_exists()) {
			calculate_local_expansions_and_WSPRLS(A,act_node_ptr->get_child_rt_ptr());
		}
		if(act_node_ptr->child_lb_exists()) {
			calculate_local_expansions_and_WSPRLS(A,act_node_ptr->get_child_lb_ptr());
		}
		if(act_node_ptr->child_rb_exists()) {
			calculate_local_expansions_and_WSPRLS(A,act_node_ptr->get_child_rb_ptr());
		}
	} else { // *act_node_ptr is a leaf
		// Step 5: WSPRLS(Well Separateness Preserving Refinement of leaf surroundings)
		// if act_node is a leaf then calculate the list D1,D2 and M from I and D1
		act_node_ptr->get_D1(D1);
		act_node_ptr->get_D2(D2);

		while (!I.empty()) {
			selected_node_ptr = I.popFrontRet();
			if (selected_node_ptr->is_leaf()) {
				//here D1 contains larger AND smaller bordering leaves!
				if (bordering(act_node_ptr, selected_node_ptr)) {
					D1.pushBack(selected_node_ptr);
				} else {
					D2.pushBack(selected_node_ptr);
				}
			} else if (bordering(act_node_ptr, selected_node_ptr)) {
				if (selected_node_ptr->child_lt_exists()) {
					I.pushBack(selected_node_ptr->get_child_lt_ptr());
				}
				if (selected_node_ptr->child_rt_exists()) {
					I.pushBack(selected_node_ptr->get_child_rt_ptr());
				}
				if (selected_node_ptr->child_lb_exists()) {
					I.pushBack(selected_node_ptr->get_child_lb_ptr());
				}
				if (selected_node_ptr->child_rb_exists()) {
					I.pushBack(selected_node_ptr->get_child_rb_ptr());
				}
			} else {
				M.pushBack(selected_node_ptr);
			}
		}
		act_node_ptr->set_D1(D1);
		act_node_ptr->set_D2(D2);
		act_node_ptr->set_M(M);
	}
}

bool NewMultipoleMethod::well_separated(QuadTreeNodeNM* node_1_ptr, QuadTreeNodeNM* node_2_ptr)
{
	double boxlength_1 = node_1_ptr->get_Sm_boxlength();
	double boxlength_2 = node_2_ptr->get_Sm_boxlength();
	double x1_min,x1_max,y1_min,y1_max,x2_min,x2_max,y2_min,y2_max;
	bool x_overlap,y_overlap;

	if (boxlength_1 <= boxlength_2) {
		x1_min = node_1_ptr->get_Sm_downleftcorner().m_x;
		x1_max = node_1_ptr->get_Sm_downleftcorner().m_x+boxlength_1;
		y1_min = node_1_ptr->get_Sm_downleftcorner().m_y;
		y1_max = node_1_ptr->get_Sm_downleftcorner().m_y+boxlength_1;

		//blow the box up
		x2_min = node_2_ptr->get_Sm_downleftcorner().m_x-boxlength_2;
		x2_max = node_2_ptr->get_Sm_downleftcorner().m_x+2*boxlength_2;
		y2_min = node_2_ptr->get_Sm_downleftcorner().m_y-boxlength_2;
		y2_max = node_2_ptr->get_Sm_downleftcorner().m_y+2*boxlength_2;
	} else {
		//blow the box up
		x1_min = node_1_ptr->get_Sm_downleftcorner().m_x-boxlength_1;
		x1_max = node_1_ptr->get_Sm_downleftcorner().m_x+2*boxlength_1;
		y1_min = node_1_ptr->get_Sm_downleftcorner().m_y-boxlength_1;
		y1_max = node_1_ptr->get_Sm_downleftcorner().m_y+2*boxlength_1;

		x2_min = node_2_ptr->get_Sm_downleftcorner().m_x;
		x2_max = node_2_ptr->get_Sm_downleftcorner().m_x+boxlength_2;
		y2_min = node_2_ptr->get_Sm_downleftcorner().m_y;
		y2_max = node_2_ptr->get_Sm_downleftcorner().m_y+boxlength_2;
	}

	//test if boxes overlap
	x_overlap = !(x1_max <= x2_min || numexcept::nearly_equal(x1_max, x2_min) || x2_max <= x1_min || numexcept::nearly_equal(x2_max,x1_min));
	y_overlap = !(y1_max <= y2_min || numexcept::nearly_equal(y1_max, y2_min) || y2_max <= y1_min || numexcept::nearly_equal(y2_max,y1_min));

	return !(x_overlap && y_overlap);
}


bool NewMultipoleMethod::bordering(QuadTreeNodeNM* node_1_ptr,QuadTreeNodeNM* node_2_ptr)
{
	double boxlength_1 = node_1_ptr->get_Sm_boxlength();
	double boxlength_2 = node_2_ptr->get_Sm_boxlength();
	double x1_min = node_1_ptr->get_Sm_downleftcorner().m_x;
	double x1_max = node_1_ptr->get_Sm_downleftcorner().m_x+boxlength_1;
	double y1_min = node_1_ptr->get_Sm_downleftcorner().m_y;
	double y1_max = node_1_ptr->get_Sm_downleftcorner().m_y+boxlength_1;
	double x2_min = node_2_ptr->get_Sm_downleftcorner().m_x;
	double x2_max = node_2_ptr->get_Sm_downleftcorner().m_x+boxlength_2;
	double y2_min = node_2_ptr->get_Sm_downleftcorner().m_y;
	double y2_max = node_2_ptr->get_Sm_downleftcorner().m_y+boxlength_2;

	auto boxIsContained = [&](double x1min, double x2min, double x1max, double x2max,
	                          double y1min, double y2min, double y1max, double y2max)
	{
		auto leq = [&](double a, double b) {
			return a <= b || numexcept::nearly_equal(a,b);
		};
		return (leq(x2min, x1min) && leq(x1max, x2max) && leq(y2min, y1min) && leq(y1max, y2max))
			|| (leq(x1min, x2min) && leq(x2max, x1max) && leq(y1min, y2min) && leq(y2max, y1max));
	};

	auto shiftBox = [&](double& x1min, double& x1max, double& x2min, double& x2max,
	                    double& y1min, double& y1max, double& y2min, double& y2max,
	                    double length)
	{
		if (x1min < x2min)      { x1min += length; x1max += length; }
		else if (x1max > x2max) { x1min -= length; x1max -= length; }
		if (y1min < y2min)      { y1min += length; y1max += length; }
		else if (y1max > y2max) { y1min -= length; y1max -= length; }
	};

	if (boxIsContained(x1_min, x2_min, x1_max, x2_max, y1_min, y2_min, y1_max, y2_max)) {
		// one box contains the other box (inclusive neighbours)
		return false;
	} else {
		if (boxlength_1 <= boxlength_2) {
			shiftBox(x1_min, x1_max, x2_min, x2_max, y1_min, y1_max, y2_min, y2_max, boxlength_1);
		} else {
			shiftBox(x2_min, x2_max, x1_min, x1_max, y2_min, y2_max, y1_min, y1_max, boxlength_2);
		}
		return boxIsContained(x1_min, x2_min, x1_max, x2_max, y1_min, y2_min, y1_max, y2_max);
	}
}


void NewMultipoleMethod::add_shifted_local_exp_of_parent(QuadTreeNodeNM* node_ptr)
{
	QuadTreeNodeNM* father_ptr = node_ptr->get_father_ptr();

	complex<double> z_0 = father_ptr->get_Sm_center();
	complex<double> z_1 = node_ptr->get_Sm_center();
	Array<complex<double> > z_1_minus_z_0_over (precision()+1);

	//init z_1_minus_z_0_over
	z_1_minus_z_0_over[0] = 1;
	for(int i = 1; i<= precision(); i++)
		z_1_minus_z_0_over[i] = z_1_minus_z_0_over[i-1] * (z_1 - z_0);


	for(int k = 0; k <= precision(); k++)
	{
		complex<double> sum (0,0);
		for(int n = k; n <= precision(); n++)
			sum += binko(n, k) * father_ptr->get_local_exp()[n] * z_1_minus_z_0_over[n-k];
		node_ptr->get_local_exp()[k] += sum;
	}
}


void NewMultipoleMethod::add_local_expansion(QuadTreeNodeNM* ptr_0, QuadTreeNodeNM* ptr_1)
{
	complex<double> z_0 = ptr_0->get_Sm_center();
	complex<double> z_1 = ptr_1->get_Sm_center();
	complex<double> sum;
	complex<double> factor;
	complex<double> z_1_minus_z_0_over_k;
	complex<double> z_1_minus_z_0_over_s;
	complex<double> pow_minus_1_s_plus_1;
	complex<double> pow_minus_1_s;

	sum = ptr_0->get_multipole_exp()[0] * log(z_1 - z_0);

	z_1_minus_z_0_over_k = z_1 - z_0;
	for(int k = 1; k<=precision(); k++)
	{
		sum += ptr_0->get_multipole_exp()[k]/z_1_minus_z_0_over_k;
		z_1_minus_z_0_over_k *= z_1-z_0;
	}
	ptr_1->get_local_exp()[0] += sum;

	z_1_minus_z_0_over_s = z_1 - z_0;
	for (int s = 1; s <= precision(); s++)
	{
		pow_minus_1_s_plus_1 = (((s+1)% 2 == 0) ? 1 : -1);
		pow_minus_1_s = ((pow_minus_1_s_plus_1 == double(1))? -1 : 1);
		sum = pow_minus_1_s_plus_1*ptr_0->get_multipole_exp()[0]/(z_1_minus_z_0_over_s *
			double(s));
		factor = pow_minus_1_s/z_1_minus_z_0_over_s;
		z_1_minus_z_0_over_s *= z_1-z_0;
		complex<double> sum_2 (0,0);

		z_1_minus_z_0_over_k = z_1 - z_0;
		for(int k=1; k<=precision(); k++)
		{
			sum_2 += binko(s+k-1,k-1)*ptr_0->get_multipole_exp()[k]/z_1_minus_z_0_over_k;
			z_1_minus_z_0_over_k *= z_1-z_0;
		}
		ptr_1->get_local_exp()[s] += sum + factor* sum_2;
	}
}


void NewMultipoleMethod::add_local_expansion_of_leaf(
	NodeArray<NodeAttributes>&A,
	QuadTreeNodeNM* ptr_0,
	QuadTreeNodeNM* ptr_1)
{
	List<node> contained_nodes;
	double multipole_0_of_v = 1;//only the first coefficient is not zero
	complex<double> z_1 = ptr_1->get_Sm_center();
	complex<double> z_1_minus_z_0_over_s;
	complex<double> pow_minus_1_s_plus_1;

	ptr_0->get_contained_nodes(contained_nodes);

	for (node v : contained_nodes) {
		//set position of v as center ( (1,0,....,0) are the multipole coefficients at v)
		complex<double> z_0  (A[v].get_x(),A[v].get_y());

		//now transform multipole_0_of_v to the locale expansion around z_1
		ptr_1->get_local_exp()[0] += multipole_0_of_v * log(z_1 - z_0);
		z_1_minus_z_0_over_s = z_1 - z_0;
		for (int s = 1;s <= precision();s++)
		{
			pow_minus_1_s_plus_1 = (((s+1)% 2 == 0) ? 1 : -1);
			ptr_1->get_local_exp()[s] += pow_minus_1_s_plus_1*multipole_0_of_v/
				(z_1_minus_z_0_over_s * double(s));
			z_1_minus_z_0_over_s *= z_1-z_0;
		}
	}
}

void NewMultipoleMethod::transform_local_exp_to_forces(
	NodeArray <NodeAttributes>&A,
	List<QuadTreeNodeNM*>& quad_tree_leaves,
	NodeArray<DPoint>& F_local_exp)
{
	complex<double> sum;
	complex<double> complex_null (0,0);
	complex<double> z_0;
	complex<double> z_v_minus_z_0_over_k_minus_1;
	DPoint force_vector;

	//calculate derivative of the potential polynom (= local expansion at leaf nodes)
	//and evaluate it for each node in contained_nodes()
	//and transform the complex number back to the real-world, to obtain the force

	for(const QuadTreeNodeNM *leaf_ptr : quad_tree_leaves)
	{
		List<node> contained_nodes;
		leaf_ptr->get_contained_nodes(contained_nodes);
		z_0 = leaf_ptr->get_Sm_center();

		for(node v : contained_nodes)
		{
			complex<double> z_v (A[v].get_x(),A[v].get_y());
			sum = complex_null;
			z_v_minus_z_0_over_k_minus_1 = 1;
			for(int k=1; k<=precision(); k++)
			{
				sum += double(k) * leaf_ptr->get_local_exp()[k] *
					z_v_minus_z_0_over_k_minus_1;
				z_v_minus_z_0_over_k_minus_1 *= z_v - z_0;
			}
			force_vector.m_x = sum.real();
			force_vector.m_y = (-1) * sum.imag();
			F_local_exp[v] = force_vector;
		}
	}
}


void NewMultipoleMethod::transform_multipole_exp_to_forces(
	NodeArray<NodeAttributes>& A,
	List<QuadTreeNodeNM*>& quad_tree_leaves,
	NodeArray<DPoint>& F_multipole_exp)
{
	complex<double> sum;
	complex<double> z_0;
	complex<double> z_v_minus_z_0_over_minus_k_minus_1;
	DPoint force_vector;

	//for each leaf u in the M-List of an actual leaf v do:
	//calculate derivative of the multipole expansion function at u
	//and evaluate it for each node in v.get_contained_nodes()
	//and transform the complex number back to the real-world, to obtain the force

	for(QuadTreeNodeNM *act_leaf_ptr : quad_tree_leaves)
	{
		List<node> act_contained_nodes;
		act_leaf_ptr->get_contained_nodes(act_contained_nodes);

		List<QuadTreeNodeNM*> M;
		act_leaf_ptr->get_M(M);

		for(const QuadTreeNodeNM *M_node_ptr : M)
		{
			z_0 = M_node_ptr->get_Sm_center();
			for(node v : act_contained_nodes)
			{
				complex<double> z_v (A[v].get_x(),A[v].get_y());
				z_v_minus_z_0_over_minus_k_minus_1 = 1.0/(z_v-z_0);
				sum = M_node_ptr->get_multipole_exp()[0]*
					z_v_minus_z_0_over_minus_k_minus_1;

				for(int k=1; k<=precision(); k++)
				{
					z_v_minus_z_0_over_minus_k_minus_1 /= z_v - z_0;
					sum -= double(k) * M_node_ptr->get_multipole_exp()[k] *
						z_v_minus_z_0_over_minus_k_minus_1;
				}
				force_vector.m_x = sum.real();
				force_vector.m_y = (-1) * sum.imag();
				F_multipole_exp[v] =  F_multipole_exp[v] + force_vector;

			}
		}
	}
}


void NewMultipoleMethod::calculate_neighbourcell_forces(
	NodeArray<NodeAttributes>& A,
	List <QuadTreeNodeNM*>& quad_tree_leaves,
	NodeArray<DPoint>& F_direct)
{
	List<node> act_contained_nodes,neighbour_contained_nodes,non_neighbour_contained_nodes;
	List<QuadTreeNodeNM*> neighboured_leaves;
	List<QuadTreeNodeNM*> non_neighboured_leaves;
	double act_leaf_boxlength,neighbour_leaf_boxlength;
	DPoint act_leaf_dlc,neighbour_leaf_dlc;

	for (QuadTreeNodeNM *act_leaf : quad_tree_leaves) {
		act_leaf->get_contained_nodes(act_contained_nodes);

		if (act_contained_nodes.size() <= particles_in_leaves()) { // usual case
			// Step 1: calculate forces inside act_contained_nodes
			calculate_forces_inside_contained_nodes(F_direct, A, act_contained_nodes);

			//Step 2: calculated forces to nodes in act_contained_nodes() of
			//leaf_ptr->get_D1()

			act_leaf->get_D1(neighboured_leaves);
			act_leaf_boxlength = act_leaf->get_Sm_boxlength();
			act_leaf_dlc = act_leaf->get_Sm_downleftcorner();

			for (const QuadTreeNodeNM *neighbour_leaf : neighboured_leaves) {
				//forget boxes that have already been looked at

				neighbour_leaf_boxlength = neighbour_leaf->get_Sm_boxlength();
				neighbour_leaf_dlc = neighbour_leaf->get_Sm_downleftcorner();

				if ((act_leaf_boxlength > neighbour_leaf_boxlength)
				 || (act_leaf_boxlength == neighbour_leaf_boxlength && act_leaf_dlc < neighbour_leaf_dlc)) {
					neighbour_leaf->get_contained_nodes(neighbour_contained_nodes);

					for(node v : act_contained_nodes)
					{
						for(node u : neighbour_contained_nodes)
						{
							DPoint f_rep_u_on_v = numexcept::f_rep_u_on_v(A[u].get_position(), A[v].get_position());
							F_direct[v] += f_rep_u_on_v;
							F_direct[u] -= f_rep_u_on_v;
						}
					}
				}
			}

			//Step 3: calculated forces to nodes in act_contained_nodes() of
			//leaf_ptr->get_D2()

			act_leaf->get_D2(non_neighboured_leaves);
			for (const QuadTreeNodeNM *non_neighbour_leaf : non_neighboured_leaves) {
				non_neighbour_leaf->get_contained_nodes(non_neighbour_contained_nodes);
				for (node v : act_contained_nodes) {
					for (node u : non_neighbour_contained_nodes) {
						F_direct[v] += numexcept::f_rep_u_on_v(A[u].get_position(), A[v].get_position());
					}
				}
			}
		} else { // special case (more than particles_in_leaves() particles in this leaf)
			for (node v : act_contained_nodes) {
				F_direct[v] += numexcept::f_rep_u_on_v(A[v].get_position(), A[v].get_position());
			}
		}
	}
}


inline void NewMultipoleMethod::add_rep_forces(
	const Graph& G,
	NodeArray<DPoint>& F_direct,
	NodeArray<DPoint>& F_multipole_exp,
	NodeArray<DPoint>& F_local_exp,
	NodeArray<DPoint>& F_rep)
{
	for(node v : G.nodes)
	{
		F_rep[v] = F_direct[v]+F_local_exp[v]+F_multipole_exp[v];
	}
}

void NewMultipoleMethod::init_binko(int t)
{
	using double_ptr = double*;

	BK = new double_ptr[t+1];

	for (int i = 0; i<= t ; i++) {
		BK[i] = new double[i+1];
	}

	//Pascal's triangle

	for (int i = 0; i <= t; i++)
		BK[i][0] = BK[i][i] = 1;

	for (int i = 2; i <= t; i ++)
		for (int j = 1; j < i; j++)
		{
			BK[i][j] = BK[i-1][j-1]+BK[i-1][j];
		}
}


inline void NewMultipoleMethod::free_binko()
{
	for(int i = 0;i<= 2*precision();i++)
		delete[] BK[i];
	delete[] BK;
}


inline double NewMultipoleMethod::binko(int n, int k)
{
	return BK[n][k];
}

}
}
}
