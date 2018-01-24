/** \file
 * \brief Declaration of class NodeInfo.
 *
 * The class NodeInfo holds the information that is necessary for
 * the rerouting of the edges after the constructive compaction step
 * the rerouting works on a PlanRep and derives the info in member
 * get_data.
 *
 * \author Karsten Klein
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

#include <ogdf/orthogonal/internal/RoutingChannel.h>
#include <ogdf/orthogonal/MinimumEdgeDistances.h>
#include <ogdf/basic/AdjEntryArray.h>
#include <ogdf/orthogonal/OrthoRep.h>
#include <ogdf/planarity/PlanRep.h>
#include <ogdf/basic/GridLayout.h>
#include <array>

namespace ogdf {
namespace edge_router {

class OGDF_EXPORT NodeInfo
{
public:
	//standard constr.
	NodeInfo() { init(); }

	void init()
	{
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				m_nbe[i][j] = 0;
				m_delta[i][j] = 0;
				m_eps[i][j] = 0;
				m_routable[i][j] = 0;
				m_flips[i][j] = 0;
			}
			num_s_edges[i] = 0;
			m_gen_pos[i] = -1;
			m_nbf[i] = 0;
			m_coord[i] = 0;
			m_ccoord[i] = 0;
		}
		lu = ll = ru = rl = tl = tr = bl = br = 0;
	}

	//Constructor, adj holds entry for inner face edge
	NodeInfo(
		OrthoRep& H,
		GridLayout& L,
		node v,
		adjEntry adj,
		RoutingChannel<int>& rc,
		NodeArray<int>& nw,
		NodeArray<int>& nh) : m_adj(adj)
	{
		init();
		get_data(H, L, v, rc, nw, nh);
	}

	// The following two definitions are necessary because the existence
	// of the move constructor deletes the implicitly defined copy constructor.
	NodeInfo(const NodeInfo &) = default;
	NodeInfo &operator=(const NodeInfo &) = default;

	NodeInfo(NodeInfo && other)
		: m_rc(other.m_rc), m_coord(other.m_coord), m_ccoord(other.m_ccoord),
		m_gen_pos(other.m_gen_pos), num_s_edges(other.num_s_edges), m_nbf(other.m_nbf)
	{
		cage_x_size = other.cage_x_size;
		cage_y_size = other.cage_y_size;
		box_x_size  = other.box_x_size;
		box_y_size  = other.box_y_size;

		lu = other.lu;
		ll = other.ll;
		ru = other.ru;
		rl = other.rl;
		tl = other.tl;
		tr = other.tr;
		bl = other.bl;
		br = other.br;

		m_firstAdj = other.m_firstAdj;
		m_adj = other.m_adj;
		m_vdegree = other.m_vdegree;

		for (int i = 0; i < 4; ++i) {
			// these should work with move cstrs once VC++ implements rvalue references v3
			in_edges[i] = std::move(other.in_edges[i]);
			point_in[i] = std::move(other.point_in[i]);

			for (int j = 0; j < 4; ++j) {
				m_delta   [i][j] = other.m_delta   [i][j];
				m_eps     [i][j] = other.m_eps     [i][j];
				m_routable[i][j] = other.m_routable[i][j];
				m_flips   [i][j] = other.m_flips   [i][j];
				m_nbe     [i][j] = other.m_nbe     [i][j];
			}
		}
	}

	virtual ~NodeInfo() { }

	//! Returns nodeboxside coordinates (real size)
	int coord(OrthoDir bs) const { return m_coord[static_cast<int>(bs)]; }
	//! Returns nodecageside coordinates (expanded size)
	int cageCoord(OrthoDir bs) const { return m_ccoord[static_cast<int>(bs)]; }

	//return distance between Node and  Cage coord
	int coordDistance(OrthoDir bs)
	{
		int result;
		int bsi = static_cast<int>(bs);
		switch (bs)
		{
		case OrthoDir::South:
		case OrthoDir::East:
			result = m_ccoord[bsi] - m_coord[bsi];
			break;
		case OrthoDir::North:
		case OrthoDir::West:
			result =  m_coord[bsi] - m_ccoord[bsi];
			break;
		default: std::cout<<"unknown direction in coordDistance"<<std::flush;
			OGDF_THROW(AlgorithmFailureException);
		}
		OGDF_ASSERT(result >= 0);
		return result;
	}

	// original box sizes, fake
	int node_xsize() const { return box_x_size; }
	int node_ysize() const { return box_y_size; }

	int nodeSize(OrthoDir od) const { return (static_cast<int>(od) % 2 == 0) ? box_y_size : box_x_size; }
	int cageSize(OrthoDir od) const { return (static_cast<int>(od) % 2 == 0) ? cage_y_size : cage_x_size; }
	//! Returns routing channel size
	int rc(OrthoDir od) const { return m_rc[static_cast<int>(od)]; }

	List<edge>& inList(OrthoDir bs) { return in_edges[static_cast<int>(bs)]; }
	List<bool>& inPoint(OrthoDir bs) { return point_in[static_cast<int>(bs)]; }

	//these values are computed dependant on the nodes placement
	// position of first and last unbend edge on every side
	int l_upper_unbend() { return lu; }
	int l_lower_unbend() { return ll; }
	int r_upper_unbend() { return ru; }
	int r_lower_unbend() { return rl; }
	int t_left_unbend()  { return tl; }
	int t_right_unbend() { return tr; }
	int b_left_unbend()  { return bl; }
	int b_right_unbend() { return br; }

	//object separation distances
	//if (no) generalization enters..., side/gener. dependant paper delta values
	//distance at side mainside, left/right from existing generalization to side neighbour
	int delta(OrthoDir mainside, OrthoDir neighbour) const { return m_delta[static_cast<int>(mainside)][static_cast<int>(neighbour)]; }

	//paper epsilon
	int eps(OrthoDir mainside, OrthoDir neighbour) const { return m_eps[static_cast<int>(mainside)][static_cast<int>(neighbour)]; }

	//cardinality of the set of edges that will bend, bside side to the side bneighbour
	int num_bend_edges(OrthoDir s1, OrthoDir sneighbour) { return m_nbe[static_cast<int>(s1)][static_cast<int>(sneighbour)]; }
	//number of edges flipped from s1 to s2 to save one bend
	int& flips(OrthoDir s1, OrthoDir s2) { return m_flips[static_cast<int>(s1)][static_cast<int>(s2)]; }

	// number of edges routed bendfree
	int num_bend_free(OrthoDir s) const { return m_nbf[static_cast<int>(s)]; }
	void num_bend_free_increment(OrthoDir s) { ++m_nbf[static_cast<int>(s)]; }

	int num_edges(OrthoDir od) const {
		return num_s_edges[static_cast<int>(od)]; //return number of edges at side od
	}

	//position of gen. edges in edge lists for every side, starting with 1
	int gen_pos(OrthoDir od) const { return m_gen_pos[static_cast<int>(od)]; }
	bool has_gen(OrthoDir od) { return m_gen_pos[static_cast<int>(od)] > -1; }

	bool is_in_edge(OrthoDir od, int pos) {
		ListConstIterator<bool> b_it = point_in[static_cast<int>(od)].get(pos);
		return *b_it;
	}

	void set_coord(OrthoDir bs, int co) { m_coord[static_cast<int>(bs)] = co; }
	void setCageCoord(OrthoDir bs, int co) { m_ccoord[static_cast<int>(bs)] = co; }

	//delta values, due to placement problems, cut to box_size / 2
	void set_delta(OrthoDir bside, OrthoDir bneighbour, int dval) {
		int bsidei = static_cast<int>(bside);
		int bneighbouri = static_cast<int>(bneighbour);
		switch (bside)
		{
		case OrthoDir::North:
		case OrthoDir::South:
			if (dval > box_y_size) {
				dval = int(floor(((double)box_y_size / 2))) - m_eps[bsidei][bneighbouri];
			} break;
		case OrthoDir::East:
		case OrthoDir::West:
			if (dval > box_x_size) {
				dval = int(floor(((double)box_x_size / 2))) - m_eps[bsidei][bneighbouri];
			} break;
		default:
			OGDF_ASSERT(false);
		}
		m_delta[bsidei][bneighbouri] = dval;
	}

	void set_eps(OrthoDir mainside, OrthoDir neighbour, int dval) { m_eps[static_cast<int>(mainside)][static_cast<int>(neighbour)] = dval; }

#if 0
	//! number of bending edges on one side at corner to second side
	void set_num_bend_edges(box_side bs1, box_side bs2, int num) {nbe[bs1][bs2] = num;}
#endif

	//! set position of generalization on each side
	void set_gen_pos(OrthoDir od, int pos) {
		m_gen_pos[static_cast<int>(od)] = pos; //odir: N 0, E 1
	}
	void set_num_edges(OrthoDir od, int num) {
		num_s_edges[static_cast<int>(od)] = num; //odir: N 0, E 1, check correct od parameter?
	}


	//! compute the size of the cage face and the node box
	void compute_cage_size() {
		cage_x_size = m_ccoord[static_cast<int>(OrthoDir::South)]-m_ccoord[static_cast<int>(OrthoDir::North)];
		cage_y_size = m_ccoord[static_cast<int>(OrthoDir::East)] - m_ccoord[static_cast<int>(OrthoDir::West)];
	}

	//set the unbend edges after (in) placement step
	void set_l_upper(int d) { lu = d; }
	void set_l_lower(int d) { ll = d; }
	void set_r_upper(int d) { ru = d; }
	void set_r_lower(int d) { rl = d; }
	void set_t_left(int d)  { tl = d; }
	void set_t_right(int d) { tr = d; }
	void set_b_left(int d)  { bl = d; }
	void set_b_right(int d) { br = d; }

	//paper set E_s1_s2
	void inc_E_hook(OrthoDir s_from, OrthoDir s_to, int num = 1) {
		int s_from_i = static_cast<int>(s_from);
		int s_to_i = static_cast<int>(s_to);
		m_routable[s_from_i][s_to_i] += num; m_nbe[s_from_i][s_to_i] += num;
	}
	void inc_E(OrthoDir s_from, OrthoDir s_to, int num = 1) {
		m_nbe[static_cast<int>(s_from)][static_cast<int>(s_to)] += num;
	}

	//read the information for node v from attributed graph/planrep
	//(needs positions ...)
	void get_data(
		OrthoRep& O,
		GridLayout& L,
		node v,
		RoutingChannel<int>& rc,
		NodeArray<int>& nw,
		NodeArray<int>& nh); //check input parameter
	// card. of paper E^_s1,s2
	int num_routable(OrthoDir s_from, OrthoDir s_to) const { return m_routable[static_cast<int>(s_from)][static_cast<int>(s_to)]; }
	int vDegree() { return m_vdegree; }
	adjEntry& firstAdj() { return m_firstAdj; }

	friend std::ostream& operator<<(std::ostream& O, const NodeInfo& inf);

private:
	std::array<int,4> m_rc;
	std::array<int,4> m_coord; //coordinates of box segments, x for ls_left/right, y for s_top/bottom
	std::array<int,4> m_ccoord; //coordinates of expanded cage segments, -"-
	int cage_x_size, cage_y_size, //cage size
		box_x_size, box_y_size; //box size
	int lu,ll,ru,rl,tl,tr,bl,br; //first/last unbend edge on all sides
	//most of the following are only [4][2] but use 44 for users conv
	int m_delta[4][4]; //sepa. distance (paper delta)
	int m_eps[4][4]; //corner separation distance (paper epsilon)
	std::array<int,4> m_gen_pos; //pos num of generaliz. edge in adj lists
	std::array<int,4> num_s_edges; //number of edges at sides 0..3=N..W
	int m_routable[4][4]; //number of reroutable edges, paper E^_s1,s2, got to be initialized after box placement
	int m_flips[4][4]; //real number of flipped edges
	int m_nbe[4][4]; //paper E_s1,s2
	std::array<int,4> m_nbf; //number of bendfree edges per side
	adjEntry m_firstAdj; //adjEntry of first encountered outgoing edge, note: this is a copy

	std::array<List<edge>,4> in_edges; //inedges on each side will be replaced by dynamic ops
	//preliminary bugfix of in/out dilemma
	std::array<List<bool>,4> point_in; //save in/out info
	adjEntry m_adj; //entry of inner cage face
	//degree of expanded vertex
	int m_vdegree;
};

std::ostream& operator<<(std::ostream& O, const NodeInfo& inf);

}
}
