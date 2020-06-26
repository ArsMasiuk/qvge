/** \file
 * \brief Edge routing and node placement implementation.
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


//TODO: handle multiedges in a way that forbids assignment of different sides


#include <ogdf/orthogonal/EdgeRouter.h>


namespace ogdf {

#define SETMULTIMINDELTA //set multidelta,


const double machineeps = 1.0e-10;
const int m_init = -1234567; //just to check initialization


//edgerouter places original node boxes in preassigned cages, computes a number of
//bendfree edges minimizing placement and routes edges, thereby introducing bends, to
//achieve a correct layout

// routing channel and number of adjacent edges / generalization is supplied by previous
// compaction step in class routingchannel
// class NodeInfo holds the specific information for a single replaced node ( adjEntry != 0)


//constructor
EdgeRouter::EdgeRouter(
	PlanRep& pru,
	OrthoRep& H,
	GridLayoutMapped& L,
	CombinatorialEmbedding& E,
	RoutingChannel<int>& rou,
	MinimumEdgeDistances<int>& mid,
	NodeArray<int>& nodewidth,
	NodeArray<int>& nodeheight)
:
	m_prup(&pru),
	m_layoutp(&L),
	m_orp(&H),
	m_comb(&E),
	m_rc(&rou),
	m_med(&mid),
	m_nodewidth(&nodewidth),
	m_nodeheight(&nodeheight)
{
	init(pru, rou);
}

//initializes the members
void EdgeRouter::init(
	PlanRep& pru,
	RoutingChannel<int>& rou,
	bool align)
{
	//saves cage position left lower
	m_newx.init(pru, m_init);
	m_newy.init(pru, m_init);
	//saves glue and connection point positions
	m_agp_x.init(pru, m_init);
	m_agp_y.init(pru, m_init);
	m_acp_x.init(pru, m_init);
	m_acp_y.init(pru, m_init);
	m_abends.init(pru, BendType::BendFree);
	m_oppositeBendType.init(pru, BendType::BendFree);

	m_minDelta = false;
#ifdef SETMULTIMINDELTA
	m_minDelta = true;
#endif

	m_mergerSon.init(pru, false);
	m_mergeDir.init(pru, OrthoDir::North);
	m_align = align;

	m_fixed.init(pru, false);
	m_processStatus.init(pru, ProcessType::unprocessed);
	m_cage_point.init(pru);

	m_sep = rou.separation();
	m_overh = rou.overhang();
	Cconst = double(m_overh)/double(m_sep);
}

//call - function: placing nodes and routing edges
void EdgeRouter::call()
{
	//if no graph is given, you should stop or return 0,0
	OGDF_ASSERT(m_prup != nullptr);
	OGDF_ASSERT(m_layoutp != nullptr);
	OGDF_ASSERT(m_orp != nullptr);
	OGDF_ASSERT(m_comb != nullptr);
	OGDF_ASSERT(m_nodewidth != nullptr);
	call(*m_prup, *m_orp, *m_layoutp, *m_comb, *m_rc, *m_med, *m_nodewidth, *m_nodeheight);
}

void EdgeRouter::call(
	PlanRep& pru,
	OrthoRep& H,
	GridLayoutMapped& L,
	CombinatorialEmbedding& E,
	RoutingChannel<int>& rou,
	MinimumEdgeDistances<int>& mid,
	NodeArray<int>& nodewidth,
	NodeArray<int>& nodeheight,
	bool align)
{
	string msg;
	OGDF_ASSERT(H.check(msg));

	init(pru, rou, align);
	m_prup = &pru;
	m_layoutp = &L;
	m_orp = &H;
	m_comb = &E;
	m_rc = &rou;
	m_med = &mid;
	m_nodewidth = &nodewidth;
	m_nodeheight = &nodeheight;
	//just input some stuff
	infos.init(pru);

	int mysep = m_sep;
	//set specific delta values automatically for all nodes
	//preliminary: set to minimum value perimeter / degree of all nodes
	//seems to cause problems with compaction
	if (m_minDelta)
		for(node v : pru.nodes)
		{
			if ( (pru.expandAdj(v) != nullptr) &&
				(pru.typeOf(v) != Graph::NodeType::generalizationMerger))
			{
			int perimeter = 2*nodewidth[v] + 2*nodeheight[v];
			OrthoDir debod = OrthoDir::North;
			int vdeg = 0;
			do {
				OrthoRep::SideInfoUML sinfo = m_orp->cageInfo(v)->m_side[static_cast<int>(debod)];
				if (sinfo.m_adjGen)
					vdeg += (sinfo.m_nAttached[0] + 1 + sinfo.m_nAttached[1]);
				else
					vdeg += sinfo.m_nAttached[0];
				debod = OrthoRep::nextDir(debod);
				} while (debod != OrthoDir::North);
				if (vdeg != 0) Math::updateMin(mysep, int(floor((double)perimeter / vdeg)));
			}
		}

	for(node v : pru.nodes)
	{
		if ( (pru.expandAdj(v) != nullptr) &&
			 (pru.typeOf(v) != Graph::NodeType::generalizationMerger)) //==Expander) )
		//adjEntry != nil, this cage node is a copy(original node)
		{
			OGDF_ASSERT(pru.widthOrig(pru.original(v)) > 0.0);
			initialize_node_info(v, mysep); //delta, epsilon, cagesize, boxsize
		}
	}

	// The Rerouting

	//simple rerouting version: maximize the number of bend free edges in the
	//placement step, then try to minimize bends by changing attachment sides
	//in the rerouting step

	//work on all expanded nodes  / positioning info is kept in class NodeInfo
	//and in layout previous compaction step guarantees routing channel

	//for every hor. edge e, lowe(e) denotes the biggest yvalue possible for
	//the lower border of target(e)'s cage if e is routed bendfree (depending on pred edges)
	//uppe(e) denotes the minimum yvalue for the upper border of v's cage if ...
	lowe.init(*m_prup, m_init);
	uppe.init(*m_prup, m_init);
	alowe.init(*m_prup, m_init);
	auppe.init(*m_prup, m_init);
	//for each and every vert. edge e, lefte(e) denotes the biggest xvalue possible
	//for the left border of v's cage if e is routed bendfree (depending on pred edges)
	//righte(e) denotes the minimum xvalue for the right border of v's cage ...
	lefte.init(*m_prup, m_init);
	righte.init(*m_prup, m_init);
	alefte.init(*m_prup, m_init);
	arighte.init(*m_prup, m_init);

	//compute the lowe / uppe / lefte / righte values for every adjacent edge
	//if generalization exists the node position is already fixed


	//compute LOWER / UPPER / LEFTER / RIGHTER border values

	auto createComputeValuesFunc = [&](bool leftRight) {
		auto &low = leftRight ? lowe : lefte;
		auto &alow = leftRight ? alowe : alefte;
		auto &upp = leftRight ? uppe : righte;
		auto &aupp = leftRight ? auppe : arighte;
		auto &left = leftRight ? lefte : uppe;
		auto &aleft = leftRight ? alefte : auppe;
		auto &right = leftRight ? righte : lowe;
		auto &aright = leftRight ? arighte : alowe;
		const auto &pos = leftRight ? L.y() : L.x();
		const auto dirA = leftRight ? OrthoDir::West : OrthoDir::North;
		const auto dirB = leftRight ? OrthoDir::East : OrthoDir::South;

		return [&, dirA, dirB](const OrthoDir dir, NodeInfo& inf) {
			const List<edge> &side_in_edges = inf.inList(dir);
			int pos_e = 1;

			for (edge inedge : side_in_edges) {
				// incident edges lying above line
				int remaining_num = side_in_edges.size() - pos_e;

				// adjust multiples of delta
				int sepsA = inf.delta(dir, dirA) * (pos_e - 1);
				int sepsB = inf.delta(dir, dirB) * remaining_num;

				low[inedge] = pos[inedge->target()] - sepsA - inf.eps(dir, dirA);
				alow[outEntry(inf, dir, pos_e - 1)] = pos[inedge->target()] - sepsA - inf.eps(dir, dirA);
				upp[inedge] = pos[inedge->target()] + sepsB + inf.eps(dir, dirB);
				aupp[outEntry(inf, dir, pos_e - 1)] = pos[inedge->target()] + sepsB + inf.eps(dir, dirB);

				// unused for horizontal/vertical edges in twostep simple rerouting
				aright[outEntry(inf, dir, pos_e - 1)] = right[inedge] = 0;
				aleft[outEntry(inf, dir, pos_e - 1)] = left[inedge] = 0; // maybe initialize to -1
				pos_e++;
			}
		};
	};

	auto computeLeftRightValues = createComputeValuesFunc(true);
	auto computeTopBottomValues = createComputeValuesFunc(false);

	//forall expanded nodes
	for(node l_v : pru.nodes)
	{
		if ((pru.expandAdj(l_v) != nullptr) && (pru.typeOf(l_v) != Graph::NodeType::generalizationMerger) )//check if replaced
		{
			NodeInfo& inf = infos[l_v];

			// for all edges incident to cage, we compute lower, upper, lefter, righter values from the paper
			// edges to the left side, pointing towards cage
			computeLeftRightValues(OrthoDir::North, inf);
			// edges to the right side, pointing towards cage
			computeLeftRightValues(OrthoDir::South, inf);
			// edges at the top side, pointing towards cage
			computeTopBottomValues(OrthoDir::East, inf);
			// edges at the bottom side, pointing towards cage
			computeTopBottomValues(OrthoDir::West, inf);
		}
	}

	//now for all edges pointing towards cages representing nodes without generalization,
	//we defined lowe/uppe values for horizontal and lefte/righte values for vertical edges

	for(node v : pru.nodes)
	{
		if ( (pru.expandAdj(v) != nullptr) && (pru.typeOf(v) != Graph::NodeType::generalizationMerger) )//== Graph::highDegreeExpander) ) //expanded high degree
		{
			compute_place(v, infos[v]/*, m_sep, m_overh*/);

			//forall expanded nodes we computed a box placement and a preliminary routing
			//now we can reroute some edges to avoid unnecessary bends (won't be optimal)
			//simple approach: implement only local decision at corner between two
			//neighboured nodebox sides

			//classify_edges(v, infos[v]); //E sets from paper, reroutable
			compute_routing(v); //maybe store result in structure and apply later
		}
	}

#if 0
	//try to place deg1 nodes
	for(node v : pru.nodes)
	{
		//there is an omission of placement here leading to errors
		if (false) //preliminary until umlex4 error checked
		{
			if ( (pru.expandAdj(v) != 0) && (pru.typeOf(v) != Graph::generalizationMerger) )
			{
				//Re - Place degree one nodes if possible, to neighbour cage
				//mag sein, dass firstadj ein zeiger sein muss
				adjEntry fa = infos[v].firstAdj();
				//step over possibly inserted bends
				while (pru.typeOf(fa->twinNode()) == Graph::dummy)
					fa = fa->faceCycleSucc();
				if ((pru.typeOf(fa->twinNode()) != Graph::highDegreeExpander) &&
					(pru.typeOf(fa->twinNode()) != Graph::lowDegreeExpander) )
				{
					//place(v, m_sep, m_overh);
					continue;
				}
				//get neighbour node
				node v1 = fa->twinNode();
				//get attachment side
				OrthoDir od = OrthoRep::prevDir(H.direction(fa));

				node expandNode = pru.expandedNode(v1);
				//v is neighbour of expanded node and may fit in its cage
				//check if deg 1 can be placed near to node
				//problem: wenn nicht geflippt wird, ist meistens auch kein Platz auf der Seite
				if ( (infos[v].vDegree() == 1) && //can be placed freely
					//no edges to cross on neighbours side
					(infos[expandNode].num_edges(od) + infos[expandNode].flips(OrthoRep::prevDir(od), od) +
						infos[expandNode].flips(OrthoRep::nextDir(od), od) == 1) &&
					//enough space to host trabant
					(infos[expandNode].coordDistance(od) > infos[v].nodeSize(OrthoRep::prevDir(od)) + m_sep) &&
					(infos[expandNode].cageSize(od) > infos[v].nodeSize(od)) &&
					//dumm gelaufen: der Knoten muss auch kleiner als der Nachbar sein, damit er nicht in
					//abgeknickte Kanten der Seite laeuft, deshalb hier spaeter deren Position testen, Bed. weg
					//das ist etwas doppelt, damit man spaeter nur diese entfernen muss, die oben bleibt
					(infos[expandNode].nodeSize(od) >= infos[v].nodeSize(od))
				)
				{
					//find new place, v must be cage node with out edge attached
					int npos, spos, epos, wpos, vxpos, vypos;
					switch (od) {
					case OrthoDir::North:
						npos = infos[expandNode].coord(OrthoDir::North) - m_sep - infos[v].node_xsize();
						spos = infos[expandNode].coord(OrthoDir::North) - m_sep;
						wpos = infos[expandNode].cageCoord(OrthoDir::West)
						     + int(floor(infos[expandNode].cageSize(OrthoDir::North)/2.0)
						     - floor(infos[v].nodeSize(OrthoDir::North)/2.0));
						epos = wpos + infos[v].nodeSize(OrthoDir::North);
						vxpos = spos;
						vypos = wpos + int(floor(infos[v].nodeSize(OrthoDir::North)/2.0));
						break;
					case OrthoDir::South:
						npos = infos[expandNode].coord(OrthoDir::South) + m_sep;
						spos = infos[expandNode].coord(OrthoDir::South) + m_sep + infos[v].node_xsize();
						wpos = infos[expandNode].cageCoord(OrthoDir::West)
						     + int(floor(infos[expandNode].cageSize(OrthoDir::North)/2.0)
						     - floor(infos[v].nodeSize(OrthoDir::North)/2.0));
						epos = wpos + infos[v].nodeSize(OrthoDir::North);
						vxpos = npos;
						vypos = wpos + int(floor(infos[v].nodeSize(OrthoDir::North)/2.0));
						break;
					case OrthoDir::East:
						npos = infos[expandNode].cageCoord(OrthoDir::North)
						     + int(floor(infos[expandNode].cageSize(OrthoDir::East)/2.0)
						     - floor(infos[v].nodeSize(OrthoDir::East)/2.0));
						spos = npos + infos[v].cageSize(OrthoDir::East);
						wpos = infos[expandNode].coord(OrthoDir::East) + m_sep;
						epos = wpos + infos[v].nodeSize(OrthoDir::East);
						vypos = wpos;
						vxpos = npos + int(floor(infos[v].nodeSize(OrthoDir::East)/2.0));
						break;
					case OrthoDir::West:
						npos = infos[expandNode].cageCoord(OrthoDir::North)
						     + int(floor(infos[expandNode].cageSize(OrthoDir::East)/2.0)
						     - floor(infos[v].nodeSize(OrthoDir::East)/2.0));
						spos = npos + infos[v].cageSize(OrthoDir::East);
						epos = infos[expandNode].coord(OrthoDir::West) - m_sep;
						wpos = epos - infos[v].nodeSize(OrthoDir::North);
						vypos = epos;
						vxpos = npos + int(floor(infos[v].nodeSize(OrthoDir::East)/2.0));
						break;
					}
					infos[v].set_coord(OrthoDir::North, npos);
					infos[v].set_coord(OrthoDir::South, spos);
					infos[v].set_coord(OrthoDir::West, wpos);
					infos[v].set_coord(OrthoDir::East, epos);
					infos[v].setCageCoord(OrthoDir::North, npos);
					infos[v].setCageCoord(OrthoDir::South, spos);
					infos[v].setCageCoord(OrthoDir::West, wpos);
					infos[v].setCageCoord(OrthoDir::East, epos);
					//set v coordinates
					m_layoutp->x(v) = vxpos;
					m_layoutp->y(v) = vypos;
					m_layoutp->x(v1) = vxpos;
					m_layoutp->y(v1) = vypos;
					//set corner coordinates after placing
					set_corners(v);
					m_processStatus[v] = processed;
					m_processStatus[v1] = used;
					//hier muss man auch die Kantenendpunkte setzen, sonst gibt es einen Fehler
				}
			}
		}
	}
#endif

	for(node v : pru.nodes) {
		if (pru.expandAdj(v) != nullptr
		 && pru.typeOf(v) != Graph::NodeType::generalizationMerger
		 && m_processStatus[v] != ProcessType::processed) {
			place(v);
		}
	}

	setDistances();

#if 0
	PathFinder pf(pru, H, L, *m_comb);
	int routable = pf.analyse();
	if (routable > 0) pf.route(false);
#endif

	OGDF_ASSERT(H.check(msg));
}

//simple local placement decision based on incident edges attachment positions
//size of input original (replaced) node and original box

void EdgeRouter::compute_gen_glue_points_y(node v)
//compute preliminary glue point positions based on placement
//and generalizations for horizontal edges and set bend type accordingly
{
	OGDF_ASSERT(infos[v].has_gen(OrthoDir::North) || infos[v].has_gen(OrthoDir::South));
	int ybase = 0;
	int gen_y = infos[v].coord(OrthoDir::West) + int(floor((double)(infos[v].node_ysize())/2)); //in the middle
	//y coordinates
	//check for left/right generalization

	//NORTH SIDE
	ListIterator<edge> l_it = infos[v].inList(OrthoDir::North).begin();
	//NORTH GENERATOR
	if (infos[v].has_gen(OrthoDir::North))//gen at left side
	{
		int pos = infos[v].gen_pos(OrthoDir::North)-1; //compare edge position to generalization position
		if (pos > -1) l_it = infos[v].inList(OrthoDir::North).get(pos);//muss unter gen sein, -2???
		else {l_it = nullptr; pos = 0;}
		bool firstcheck = true;
		bool lastcheck = true;
		//classify edges

		//assign gp value for edges underneath generalization
		ybase = gen_y - infos[v].delta(OrthoDir::North, OrthoDir::West);
		//bendfree edges underneath
		while (l_it.valid() &&
			(pos*infos[v].delta(OrthoDir::North, OrthoDir::West) + infos[v].eps(OrthoDir::North, OrthoDir::West) <=
			(cp_y(outEntry(infos[v], OrthoDir::North, pos)) - infos[v].coord(OrthoDir::West)) )) //bendfree edges cageCoord??!!!
		{
			m_agp_y[outEntry(infos[v], OrthoDir::North, pos)] = cp_y(outEntry(infos[v], OrthoDir::North, pos));

			if (firstcheck) {firstcheck = false; infos[v].set_l_upper(cp_y(outEntry(infos[v], OrthoDir::North, pos)));}
			lastcheck = false;
			infos[v].set_l_lower(cp_y(outEntry(infos[v], OrthoDir::North, pos)));

			m_abends[outEntry(infos[v], OrthoDir::North, pos)] = BendType::BendFree;
			ybase = cp_y(outEntry(infos[v], OrthoDir::North, pos)) - infos[v].delta(OrthoDir::North, OrthoDir::West);
			--l_it;
			--pos;
			infos[v].num_bend_free_increment(OrthoDir::North);
		}

		//still some lower edges to bend
		while (l_it.valid())
		{
			NodeInfo &inf = infos[v];
			const adjEntry adj = outEntry(inf, OrthoDir::North, pos);

			m_agp_y[adj] = inf.coord(OrthoDir::West)
				+ inf.eps(OrthoDir::North, OrthoDir::West)
				+ pos*inf.delta(OrthoDir::North, OrthoDir::West);

			updateOneBend(cp_y(adj) >= inf.coord(OrthoDir::West) - m_sep, adj, v, OrthoDir::North, true, BendType::ProbB1L, BendType::ProbB2L);

			ybase = ybase - inf.delta(OrthoDir::North, OrthoDir::West);
			--pos;
			--l_it;
		}

		//assign gp value for generalization
		ybase = gen_y; //check == y(current edge)???
		l_it = infos[v].inList(OrthoDir::North).get(infos[v].gen_pos(OrthoDir::North));

		infos[v].num_bend_free_increment(OrthoDir::North);
		m_agp_y[outEntry(infos[v], OrthoDir::North, infos[v].gen_pos(OrthoDir::North))] = ybase;
		m_abends[outEntry(infos[v], OrthoDir::North, infos[v].gen_pos(OrthoDir::North))] = BendType::BendFree;

		if (lastcheck) infos[v].set_l_lower(ybase);
		infos[v].set_l_upper(ybase);

		//assign gp value for bendfree edges above generalization
		++l_it;
		pos = infos[v].gen_pos(OrthoDir::North) + 1;
		while (l_it.valid() &&
			((infos[v].inList(OrthoDir::North).size() - 1 - pos)*infos[v].delta(OrthoDir::North, OrthoDir::East)
			+ infos[v].eps(OrthoDir::North,OrthoDir::East) <=
			(infos[v].coord(OrthoDir::East) - cp_y(outEntry(infos[v], OrthoDir::North, pos))) ))
		{
			m_abends[outEntry(infos[v], OrthoDir::North, pos)] = BendType::BendFree;
			ybase = cp_y(outEntry(infos[v], OrthoDir::North, pos));//+= infos[v].delta(OrthoDir::North, OrthoDir::East);

			m_agp_y[outEntry(infos[v], OrthoDir::North, pos)] = ybase;

			infos[v].set_l_upper(ybase);
			++l_it; ++pos;
			infos[v].num_bend_free_increment(OrthoDir::North);
		}

		//assign gp value for bend edges on top of generalization
		int bendnum = infos[v].inList(OrthoDir::North).size() - pos;
		while (l_it.valid())
		{
			NodeInfo &inf = infos[v];
			const adjEntry adj = outEntry(inf, OrthoDir::North, pos);

			// check for single/2 bend
			// ybase += infos[v].delta(OrthoDir::North, OrthoDir::East); // there is a generalization
			ybase = inf.l_upper_unbend() +
				(pos  + 1 + bendnum - inf.inList(OrthoDir::North).size())*inf.delta(OrthoDir::North, OrthoDir::East);

			updateOneBend(m_acp_y[adj] < inf.coord(OrthoDir::East) + m_sep, adj, v, OrthoDir::North, false, BendType::ProbB1R, BendType::ProbB2R);

			m_agp_y[adj] = ybase;

			++l_it;
			++pos;
		}
	}

	//NO LEFT GENERATOR
	else
	{
		OGDF_ASSERT(infos[v].has_gen(OrthoDir::South));//obs
		//classify edges

		// edges bending downwards
		int pos = updateBends(v, l_it, false, OrthoDir::West, true, false);

		//bendfree edges
		bool check = true;
		while (l_it.valid() && ( infos[v].coord(OrthoDir::East)  >= (cp_y(outEntry(infos[v], OrthoDir::North, pos))
							 + (infos[v].inList(OrthoDir::North).size() - 1 - pos)*infos[v].delta(OrthoDir::North, OrthoDir::West)
							 + infos[v].eps(OrthoDir::North,OrthoDir::West)) ))
		{
			if (check) infos[v].set_l_lower(cp_y(outEntry(infos[v], OrthoDir::North, pos)));
			infos[v].set_l_upper(cp_y(outEntry(infos[v], OrthoDir::North, pos)));
			check = false;
			m_abends[outEntry(infos[v], OrthoDir::North, pos)] = BendType::BendFree;
			infos[v].num_bend_free_increment(OrthoDir::North);
			m_agp_y[outEntry(infos[v], OrthoDir::North, pos)] =  cp_y(outEntry(infos[v], OrthoDir::North, pos));//m_acp_y[outEntry(infos[v], OrthoDir::North, pos)];
			++l_it;
			++pos;
		}

		// edges bending upwards
		updateBends(v, l_it, false, OrthoDir::East, false, true, pos);
	}

	//RIGHT SIDE
	//RIGHT GENERATOR
	if (infos[v].has_gen(OrthoDir::South))
	{
		//left copy
		int pos = infos[v].gen_pos(OrthoDir::South)-1; //compare edge position to generalization position
		if (pos > -1)
			l_it = infos[v].inList(OrthoDir::South).get(pos);
		else l_it = nullptr;
		//classify edges

		//assign gp value for edges underneath generalization
		ybase = gen_y - infos[v].delta(OrthoDir::South, OrthoDir::West);

		//bendfree edges underneath
		bool check = false;
		bool lastcheck = true;
		while (l_it.valid() &&
			(pos*infos[v].delta(OrthoDir::South, OrthoDir::West) + infos[v].eps(OrthoDir::South,OrthoDir::West) <=
			(cp_y(outEntry(infos[v], OrthoDir::South, pos)) - infos[v].coord(OrthoDir::West)) ))
		{
			m_agp_y[outEntry(infos[v], OrthoDir::South, pos)] = cp_y(outEntry(infos[v], OrthoDir::South, pos));

			lastcheck = false;
			infos[v].set_r_lower(m_agp_y[outEntry(infos[v], OrthoDir::South, pos)]);
			if (!check) {infos[v].set_r_upper(m_agp_y[outEntry(infos[v], OrthoDir::South, pos)]); check = true;}
			m_abends[outEntry(infos[v], OrthoDir::South, pos)] = BendType::BendFree;
			ybase = cp_y(outEntry(infos[v], OrthoDir::South, pos)) - infos[v].delta(OrthoDir::South, OrthoDir::West);
			--l_it;
			--pos;
			infos[v].num_bend_free_increment(OrthoDir::South);
		}

		updateLowerEdgesBends(v, l_it, pos, ybase, false, OrthoDir::West, false);

		//assign gp value for generalization
		ybase = gen_y; //check == y(current edge)???
		l_it = infos[v].inList(OrthoDir::South).get(infos[v].gen_pos(OrthoDir::South));
		infos[v].num_bend_free_increment(OrthoDir::South);
		m_agp_y[outEntry(infos[v], OrthoDir::South, infos[v].gen_pos(OrthoDir::South))] = ybase;
		m_abends[outEntry(infos[v], OrthoDir::South, infos[v].gen_pos(OrthoDir::South))] = BendType::BendFree;
		if (lastcheck)
		{
			infos[v].set_r_lower(m_agp_y[outEntry(infos[v], OrthoDir::South, infos[v].gen_pos(OrthoDir::South))]);
		}
		infos[v].set_r_upper(ybase);

		//assign gp value for bendfree edges above generalization
		++l_it;
		pos = infos[v].gen_pos(OrthoDir::South) + 1;
		while (l_it.valid() &&
			((infos[v].inList(OrthoDir::South).size() - 1 - pos)*infos[v].delta(OrthoDir::South, OrthoDir::East) + infos[v].eps(OrthoDir::South,OrthoDir::East) <=
			(infos[v].coord(OrthoDir::East) - cp_y(outEntry(infos[v], OrthoDir::South, pos))) ))
		{
			m_abends[outEntry(infos[v], OrthoDir::South, pos)] = BendType::BendFree;
			ybase = cp_y(outEntry(infos[v], OrthoDir::South, pos));//+= infos[v].delta(OrthoDir::North, OrthoDir::East);
			m_agp_y[outEntry(infos[v], OrthoDir::South, pos)] = ybase;
			infos[v].set_r_upper(ybase);
			infos[v].num_bend_free_increment(OrthoDir::South);
			++l_it; ++pos;
		}

		//assign gp value for bend edges on top of generalization
		while (l_it.valid())
		{
			NodeInfo &inf = infos[v];
			const adjEntry adj = outEntry(inf, OrthoDir::South, pos);

			// check for single/2 bend
			updateOneBend(cp_y(adj) <= inf.coord(OrthoDir::East) + m_sep, adj, v, OrthoDir::South, true, BendType::ProbB1L, BendType::ProbB2L);

			ybase += inf.delta(OrthoDir::South, OrthoDir::East); // there is a generalization
			m_agp_y[adj] = ybase;
			++l_it;
			++pos;
		}
	} else {
		l_it = infos[v].inList(OrthoDir::South).begin();
		//classify edges

		// edges bending downwards
		int pos = updateBends(v, l_it, false, OrthoDir::West, false, false);

		//bendfree edges
		bool check = false;
		while (l_it.valid() && ( infos[v].coord(OrthoDir::East)  >=
			(cp_y(outEntry(infos[v], OrthoDir::South, pos))
			+ (infos[v].inList(OrthoDir::South).size() - 1 - pos)*infos[v].delta(OrthoDir::South, OrthoDir::West)
			+ infos[v].eps(OrthoDir::South,OrthoDir::West)) ))
		{
			if (!check) {
				infos[v].set_r_lower(cp_y(outEntry(infos[v], OrthoDir::South, pos)));
				check = true;
			}
			infos[v].set_r_upper(cp_y(outEntry(infos[v], OrthoDir::South, pos)));
			m_abends[outEntry(infos[v], OrthoDir::South,pos)] = BendType::BendFree;
			infos[v].num_bend_free_increment(OrthoDir::South);
			m_agp_y[outEntry(infos[v], OrthoDir::South, pos)] = cp_y(outEntry(infos[v], OrthoDir::South, pos));
			++l_it;
			++pos;
		}

		//edges bending upwards
		updateBends(v, l_it, false, OrthoDir::East, true, true, pos);
	}
	//end set m_gy

	//x coordinates, just on the cage boundary
	int l_pos = 0;
	l_it = infos[v].inList(OrthoDir::North).begin();
	while (l_it.valid())
	{
		m_agp_x[outEntry(infos[v], OrthoDir::North, l_pos)] = infos[v].coord(OrthoDir::North);
		++l_it;
		++l_pos;
	}
	l_it = infos[v].inList(OrthoDir::South).begin();
	l_pos = 0;
	while (l_it.valid())
	{
		m_agp_x[outEntry(infos[v], OrthoDir::South, l_pos)] = infos[v].coord(OrthoDir::South);
		++l_it;
		++l_pos;
	}
}

//todo: parameterize the different functions and delete the obsolete copies

//compute preliminary glue point positions based on placement
//and generalizations for horizontal edges
void EdgeRouter::compute_gen_glue_points_x(node v)
{
	OGDF_ASSERT(infos[v].has_gen(OrthoDir::East) || infos[v].has_gen(OrthoDir::West));
	int xbase = 0;

	//position generalization in the middle of the node
	int gen_x = infos[v].coord(OrthoDir::North) + infos[v].node_xsize()/2; //in the middle
	//x coordinates in m_gp_x, set bend types for all edges


	//TOP SIDE
	ListIterator<edge> l_it = infos[v].inList(OrthoDir::East).begin();

	//TOP GENERATOR
	if (infos[v].has_gen(OrthoDir::East))//gen at top side
	{
		int pos = infos[v].gen_pos(OrthoDir::East)-1; //compare edge position to generalization position
		if (pos > -1)
			l_it = infos[v].inList(OrthoDir::East).get(pos);
		else {
			l_it = nullptr; pos = 0;
		}
		//classify edges

		//assign gp value for edges underneath generalization
		xbase = gen_x - infos[v].delta(OrthoDir::East, OrthoDir::North);

		//bendfree edges underneath
		bool check = false;
		while (l_it.valid() &&
			(pos*infos[v].delta(OrthoDir::East, OrthoDir::North) + infos[v].eps(OrthoDir::East,OrthoDir::North) <=
			(cp_x(outEntry(infos[v], OrthoDir::East, pos)) - infos[v].coord(OrthoDir::North)) ))
		{
			m_agp_x[outEntry(infos[v], OrthoDir::East, pos)] = cp_x(outEntry(infos[v], OrthoDir::East, pos));
			infos[v].set_t_right(m_agp_x[outEntry(infos[v], OrthoDir::East, pos)]);
			if (!check) {
				check = true;
				infos[v].set_t_left(m_agp_x[outEntry(infos[v], OrthoDir::East, pos)]);
			}
			m_abends[outEntry(infos[v], OrthoDir::East, pos)] = BendType::BendFree;
			xbase = cp_x(outEntry(infos[v], OrthoDir::East, pos)) - infos[v].delta(OrthoDir::East, OrthoDir::North);
			--l_it;
			--pos;
			infos[v].num_bend_free_increment(OrthoDir::East);
		}

		updateLowerEdgesBends(v, l_it, pos, xbase, true, OrthoDir::North, true);

		//assign gp value for generalization
		xbase = gen_x; //check == x(current edge)???
		l_it = infos[v].inList(OrthoDir::East).get(infos[v].gen_pos(OrthoDir::East));
		m_agp_x[outEntry(infos[v], OrthoDir::East, infos[v].gen_pos(OrthoDir::East))] = xbase;
		m_abends[outEntry(infos[v], OrthoDir::East, infos[v].gen_pos(OrthoDir::East))] = BendType::BendFree;
		infos[v].num_bend_free_increment(OrthoDir::East);
		if (!check)
			infos[v].set_t_left(m_agp_x[outEntry(infos[v], OrthoDir::East, infos[v].gen_pos(OrthoDir::East))]);
		infos[v].set_t_right(m_agp_x[outEntry(infos[v], OrthoDir::East, infos[v].gen_pos(OrthoDir::East))]);

		//assign gp value for bendfree edges above generalization
		++l_it;
		pos = infos[v].gen_pos(OrthoDir::East) + 1;
		while (l_it.valid() &&
			((infos[v].inList(OrthoDir::East).size() - 1 - pos)*infos[v].delta(OrthoDir::East, OrthoDir::South) + infos[v].eps(OrthoDir::East,OrthoDir::South) <=
			(infos[v].coord(OrthoDir::South) - cp_x(outEntry(infos[v], OrthoDir::East, pos))) ))
		{
			m_abends[outEntry(infos[v], OrthoDir::East, pos)] = BendType::BendFree;
			xbase = cp_x(outEntry(infos[v], OrthoDir::East, pos));//+= infos[v].delta(OrthoDir::East, OrthoDir::South);
			m_agp_x[outEntry(infos[v], OrthoDir::East, pos)] = xbase;
			infos[v].set_t_right(m_agp_x[outEntry(infos[v], OrthoDir::East, pos)]);
			++l_it; ++pos;
			infos[v].num_bend_free_increment(OrthoDir::East);
		}

		//assign gp value for bend edges on top of generalization
		while (l_it.valid())
		{
			NodeInfo &inf = infos[v];
			const adjEntry adj = outEntry(inf, OrthoDir::East, pos);

			//check for single/2 bend
			updateOneBend(m_acp_x[adj] < inf.coord(OrthoDir::South) + m_sep, adj, v, OrthoDir::East, false, BendType::ProbB1R, BendType::ProbB2R);

			xbase += inf.delta(OrthoDir::East, OrthoDir::South); //there is a generalization
			m_agp_x[adj] = xbase;
			++l_it;
			++pos;
		}
	} else {
		OGDF_ASSERT(infos[v].has_gen(OrthoDir::West));//obs

		// edge bending downwards
		int pos = updateBends(v, l_it, true, OrthoDir::North, true, false);
		int numbends = pos;  //number of bend edges, used to correct position after assignment

		//bendfree edges
		bool check =  false;
		int lastunbend = m_init;
		int firstunbend = m_init;
		while (l_it.valid() && ( infos[v].coord(OrthoDir::South)  >=
			(cp_x(outEntry(infos[v], OrthoDir::East, pos))
			+ (infos[v].inList(OrthoDir::East).size() - 1 - pos)*infos[v].delta(OrthoDir::East, OrthoDir::North)
			+ infos[v].eps(OrthoDir::East,OrthoDir::North)) ))
		{
			m_abends[outEntry(infos[v], OrthoDir::East, pos)] = BendType::BendFree;
			infos[v].num_bend_free_increment(OrthoDir::East);
			lastunbend = m_agp_x[outEntry(infos[v], OrthoDir::East, pos)] = cp_x(outEntry(infos[v], OrthoDir::East, pos));
			if (firstunbend == m_init) firstunbend = lastunbend;
			if (!check) {
				check = true;
				infos[v].set_t_left(m_agp_x[outEntry(infos[v], OrthoDir::East, pos)] );
			}
			infos[v].set_t_right(m_agp_x[outEntry(infos[v], OrthoDir::East, pos)]);
			++l_it;
			++pos;
		}

		//now we set all bending edges (left) as close as possible to the unbend edges
		//to allow possible bend saving by edge flipping at the corner

		if (firstunbend != m_init)
		{
			ListIterator<edge> ll_it = infos[v].inList(OrthoDir::East).begin();
			int llpos = 0;
			while (ll_it.valid() && ( infos[v].coord(OrthoDir::North)  >
				(cp_x(outEntry(infos[v], OrthoDir::East, llpos))
				- llpos*infos[v].delta(OrthoDir::East, OrthoDir::North)
				- infos[v].eps(OrthoDir::East,OrthoDir::North)) ))
			{
				m_agp_x[outEntry(infos[v], OrthoDir::East, llpos)] = firstunbend -
					(numbends - llpos)*infos[v].delta(OrthoDir::East, OrthoDir::North);
				++ll_it;
				++llpos;
			}
		}

		//edges bending upwards
		updateBends(v, l_it, true, OrthoDir::South, false, true, pos);
	}

	//BOTTOM SIDE
	//BOTTOM GENERALIZATION
	if (infos[v].has_gen(OrthoDir::West))
	{
		//left copy
		int pos = infos[v].gen_pos(OrthoDir::West)-1; //compare edge position to generalization position
		if (pos > -1)
			l_it = infos[v].inList(OrthoDir::West).get(pos);
		else { l_it = nullptr; pos = 0; }
		//classify edges

		//assign gp value for edges underneath generalization
		xbase = gen_x - infos[v].delta(OrthoDir::West, OrthoDir::North);

		//bendfree edges underneath
		bool firstcheck = true;
		while (l_it.valid() &&
			(pos*infos[v].delta(OrthoDir::West, OrthoDir::North) + infos[v].eps(OrthoDir::West,OrthoDir::North) <=
			(cp_x(outEntry(infos[v], OrthoDir::West, pos)) - infos[v].coord(OrthoDir::North)) ))
		{
			m_agp_x[outEntry(infos[v], OrthoDir::West, pos)] = cp_x(outEntry(infos[v], OrthoDir::West, pos));
			m_abends[outEntry(infos[v], OrthoDir::West, pos)] = BendType::BendFree;
			xbase = cp_x(outEntry(infos[v], OrthoDir::West, pos)) - infos[v].delta(OrthoDir::West, OrthoDir::North);
			if (firstcheck) {
				firstcheck = false;
				infos[v].set_b_left(m_agp_x[outEntry(infos[v], OrthoDir::West, pos)]);
			}
			infos[v].set_b_right(m_agp_x[outEntry(infos[v], OrthoDir::West, pos)]);
			--l_it;
			--pos;
			infos[v].num_bend_free_increment(OrthoDir::West);
		}

		updateLowerEdgesBends(v, l_it, pos, xbase, true, OrthoDir::North, false);

		//assign gp value for generalization
		xbase = gen_x; //check == x(current edge)???std::cout
		l_it = infos[v].inList(OrthoDir::West).get(infos[v].gen_pos(OrthoDir::West));
		m_agp_x[outEntry(infos[v], OrthoDir::West, infos[v].gen_pos(OrthoDir::West))] = xbase;
		m_abends[outEntry(infos[v], OrthoDir::West, infos[v].gen_pos(OrthoDir::West))] = BendType::BendFree;
		infos[v].num_bend_free_increment(OrthoDir::West);
		if (firstcheck) {
			firstcheck = false;
			infos[v].set_b_right(m_agp_x[outEntry(infos[v], OrthoDir::West, infos[v].gen_pos(OrthoDir::West))]);
		}
		infos[v].set_b_left(m_agp_x[outEntry(infos[v], OrthoDir::West, infos[v].gen_pos(OrthoDir::West))]);

		//assign gp value for bendfree edges above generalization
		++l_it;
		pos = infos[v].gen_pos(OrthoDir::West) + 1;
		while (l_it.valid() &&
			((infos[v].inList(OrthoDir::West).size() - 1 - pos)*infos[v].delta(OrthoDir::West, OrthoDir::South) + infos[v].eps(OrthoDir::West,OrthoDir::South) <=
			(infos[v].coord(OrthoDir::South) - cp_x(outEntry(infos[v], OrthoDir::West, pos))) ))
		{
			m_abends[outEntry(infos[v], OrthoDir::West, pos)] = BendType::BendFree;
			xbase = cp_x(outEntry(infos[v], OrthoDir::West, pos));//+= infos[v].delta(OrthoDir::North, OrthoDir::East);
			m_agp_x[outEntry(infos[v], OrthoDir::West, pos)] = xbase;
			infos[v].num_bend_free_increment(OrthoDir::West);
			infos[v].set_b_left(m_agp_x[outEntry(infos[v], OrthoDir::West, pos)]);
			if (firstcheck)
			{
				infos[v].set_b_right(m_agp_x[outEntry(infos[v], OrthoDir::West, pos)]);
				firstcheck = false;
			}
			++l_it; ++pos;
		}

		//assign gp value for bend edges on top of generalization
		while (l_it.valid())
		{
			NodeInfo &inf = infos[v];
			const adjEntry adj = outEntry(inf, OrthoDir::West, pos);

			//check for single/2 bend
			updateOneBend(m_acp_x[adj] <= inf.coord(OrthoDir::South) + m_sep, adj, v, OrthoDir::West, true, BendType::ProbB1L, BendType::ProbB2L);

			xbase += inf.delta(OrthoDir::West, OrthoDir::South); //there is a generalization
			m_agp_x[adj] = xbase;
			++l_it;
			++pos;
		}
	} else {
		l_it = infos[v].inList(OrthoDir::West).begin();

		// edges bending downwards
		int pos = updateBends(v, l_it, true, OrthoDir::North, false, false);
		int rightbend = pos; // save number of actually bend edges to correct their position later

		//bendfree edges
		bool firstcheck = true;
		int lastunbend = m_init;
		int firstunbend = m_init;
		while (l_it.valid() && ( infos[v].coord(OrthoDir::South)  >=
			(cp_x(outEntry(infos[v], OrthoDir::West, pos))
			+ (infos[v].inList(OrthoDir::West).size() - 1 - pos)*infos[v].delta(OrthoDir::West, OrthoDir::North)
			+ infos[v].eps(OrthoDir::West,OrthoDir::North)) ))
		{
			m_abends[outEntry(infos[v], OrthoDir::West, pos)] = BendType::BendFree;
			infos[v].num_bend_free_increment(OrthoDir::West);
			lastunbend = m_agp_x[outEntry(infos[v], OrthoDir::West, pos)] = cp_x(outEntry(infos[v], OrthoDir::West, pos));

			if (firstunbend == m_init) firstunbend = lastunbend;

			if (firstcheck)
			{
				infos[v].set_b_right(lastunbend);
				firstcheck = false;
			}
			infos[v].set_b_left(lastunbend);
			++l_it;
			++pos;
		}

		//no assign bend edges as close as possible

		if (firstunbend != m_init)
		{
			ListIterator<edge> ll_it = infos[v].inList(OrthoDir::West).begin();
			int llpos = 0;
			while (ll_it.valid() && ( infos[v].coord(OrthoDir::North)  >
				(cp_x(outEntry(infos[v], OrthoDir::West, llpos)) - llpos*infos[v].delta(OrthoDir::West, OrthoDir::North) - infos[v].eps(OrthoDir::West,OrthoDir::North)) ))
			{
				m_agp_x[outEntry(infos[v], OrthoDir::West, llpos)] = firstunbend -
					(rightbend - llpos)*infos[v].delta(OrthoDir::West, OrthoDir::North);
				++ll_it;
				++llpos;
			}
		}

		//edges bending upwards
		updateBends(v, l_it, pos, lastunbend, true, OrthoDir::South, true, true, true);
	}
	//end set m_gx

	//y coordinates, just on the cage boundary
	l_it = infos[v].inList(OrthoDir::East).begin();
	int l_pos = 0;
	while (l_it.valid())
	{
		m_agp_y[outEntry(infos[v], OrthoDir::East, l_pos)] = infos[v].coord(OrthoDir::East);
		++l_it;
		l_pos++;
	}
	l_it = infos[v].inList(OrthoDir::West).begin();
	l_pos = 0;
	while (l_it.valid())
	{
		m_agp_y[outEntry(infos[v], OrthoDir::West, l_pos)] = infos[v].coord(OrthoDir::West);
		++l_it;
		l_pos++;
	}
}

//compute preliminary glue point positions based on placement
//maybe: use earlier classification of edges: bendfree?
void EdgeRouter::compute_glue_points_y(node v)
{
	//forall edges in horizontal lists, we set the glue point y coordinate
	ListIterator<edge> l_it = infos[v].inList(OrthoDir::North).begin();
	//left edges
	//classify edges

	// edges bending downwards
	int pos = updateBends(v, l_it, false, OrthoDir::West, true, false);
	int bendDownCounter = pos;

	//bendfree edges
	int lastunbend = m_init;
	int firstunbend = m_init;
	bool firstcheck = true;
	while (l_it.valid() && ( infos[v].coord(OrthoDir::East)  >=
		(cp_y(outEntry(infos[v], OrthoDir::North, pos))
		+ (infos[v].inList(OrthoDir::North).size() - 1 - pos)*infos[v].delta(OrthoDir::North, OrthoDir::West)
		+ infos[v].eps(OrthoDir::North,OrthoDir::West)) ))
	{
		m_abends[outEntry(infos[v], OrthoDir::North, pos)] = BendType::BendFree;
		infos[v].num_bend_free_increment(OrthoDir::North);
		lastunbend = m_agp_y[outEntry(infos[v], OrthoDir::North, pos)]  = cp_y(outEntry(infos[v], OrthoDir::North, pos));
		if (firstcheck)
		{
			infos[v].set_l_lower(m_agp_y[outEntry(infos[v], OrthoDir::North, pos)]);
			firstunbend = lastunbend;
			firstcheck = false;
		}
		infos[v].set_l_upper(m_agp_y[outEntry(infos[v], OrthoDir::North, pos)]);
		++l_it;
		++pos;
	}

	//correct left edges
	if (firstunbend != m_init)
	{
		ListIterator<edge> ll_it = infos[v].inList(OrthoDir::North).begin();
		int llpos = 0;
		while (ll_it.valid() && ( infos[v].coord(OrthoDir::West)  >
			(cp_y(outEntry(infos[v], OrthoDir::North, llpos)) - llpos*infos[v].delta(OrthoDir::North, OrthoDir::West) - infos[v].eps(OrthoDir::North, OrthoDir::West)) ))
		{
			m_agp_y[outEntry(infos[v], OrthoDir::North, llpos)] = firstunbend -
				(bendDownCounter - llpos)*infos[v].delta(OrthoDir::North, OrthoDir::West);
			++ll_it;
			++llpos;
		}
	}

	//edges bending upwards
	updateBends(v, l_it, pos, lastunbend, false, OrthoDir::East, false, true, true);

	//South edges

	l_it = infos[v].inList(OrthoDir::South).begin();
	//classify edges
	pos = updateBends(v, l_it, false, OrthoDir::West, false, false);
	bendDownCounter = pos;

	//bendfree edges
	firstcheck = true;
	lastunbend = m_init;
	firstunbend = m_init;
	while (l_it.valid() && ( infos[v].coord(OrthoDir::East)  >=
		(cp_y(outEntry(infos[v], OrthoDir::South, pos))
		+ (infos[v].inList(OrthoDir::South).size() - 1 - pos)*infos[v].delta(OrthoDir::South, OrthoDir::West)
		+ infos[v].eps(OrthoDir::South,OrthoDir::West)) ))
	{
		m_abends[outEntry(infos[v], OrthoDir::South, pos)] = BendType::BendFree;
		infos[v].num_bend_free_increment(OrthoDir::South);
		lastunbend = m_agp_y[outEntry(infos[v], OrthoDir::South, pos)] = cp_y(outEntry(infos[v], OrthoDir::South, pos));
		if (firstcheck)
		{
			firstcheck = false;
			infos[v].set_r_lower(m_agp_y[outEntry(infos[v], OrthoDir::South, pos)]);
			firstunbend = lastunbend;
		}
		infos[v].set_r_upper(m_agp_y[outEntry(infos[v], OrthoDir::South, pos)]);
		++l_it;
		++pos;
	}

	//correct right bending edges
	if (firstunbend != m_init)
	{
		ListIterator<edge> ll_it = infos[v].inList(OrthoDir::South).begin();
		int llpos = 0;
		while (ll_it.valid() && ( infos[v].coord(OrthoDir::West)  >
			(cp_y(outEntry(infos[v], OrthoDir::South, llpos)) - llpos*infos[v].delta(OrthoDir::South, OrthoDir::West)
			- infos[v].eps(OrthoDir::South, OrthoDir::West)) ))
		{
			m_agp_y[outEntry(infos[v], OrthoDir::South, llpos)] = firstunbend -
				(bendDownCounter - llpos)*infos[v].delta(OrthoDir::South, OrthoDir::West);
			++ll_it;
			++llpos;
		}
	}

	//edges bending upwards
	updateBends(v, l_it, pos, lastunbend, false, OrthoDir::East, true, true, true);

	//x coordinates, just on the cage boundary
	l_it = infos[v].inList(OrthoDir::North).begin();
	int l_pos = 0;
	while (l_it.valid())
	{
		m_agp_x[outEntry(infos[v], OrthoDir::North, l_pos)] = infos[v].coord(OrthoDir::North);
		++l_it;
		l_pos++;
	}
	l_it = infos[v].inList(OrthoDir::South).begin();
	l_pos = 0;
	while (l_it.valid())
	{
		m_agp_x[outEntry(infos[v], OrthoDir::South, l_pos)] = infos[v].coord(OrthoDir::South);
		++l_it;
		l_pos++;
	}
}


void EdgeRouter::compute_glue_points_x(node& v)
//compute preliminary glue point positions based on placement
//maybe: use earlier classification of edges: bendfree?
{
	//forall edges in vertical lists, we set the glue point x coordinate
	//TOP SIDE
	ListIterator<edge> l_it = infos[v].inList(OrthoDir::East).begin();
	//classify edges
	int pos = updateBends(v, l_it, true, OrthoDir::North, true, false);
	int numbends = pos;
	int lastunbend = m_init;
	int firstunbend = m_init;

	//bendfree edges
	bool firstcheck = true;
	while (l_it.valid() && ( infos[v].coord(OrthoDir::South)  >=
		(cp_x(outEntry(infos[v], OrthoDir::East, pos))
		+ (infos[v].inList(OrthoDir::East).size() - 1 - pos)*infos[v].delta(OrthoDir::East, OrthoDir::North)
		+ infos[v].eps(OrthoDir::East,OrthoDir::North)) ))
	{
		m_abends[outEntry(infos[v], OrthoDir::East, pos)] = BendType::BendFree;
		infos[v].num_bend_free_increment(OrthoDir::East);
		lastunbend = m_agp_x[outEntry(infos[v], OrthoDir::East, pos)] = cp_x(outEntry(infos[v], OrthoDir::East, pos));
		if (firstunbend == m_init) firstunbend = lastunbend;
		if (firstcheck)
		{
			firstcheck = false;
			infos[v].set_t_left(lastunbend);
		}
		infos[v].set_t_right(lastunbend);
		++l_it;
		++pos;
	}

	//temporary sol, correct left edges

	if (firstunbend != m_init)
	{
		ListIterator<edge> ll_it = infos[v].inList(OrthoDir::East).begin();
		int llpos = 0;
		while (ll_it.valid() && ( infos[v].coord(OrthoDir::North)  >
			(cp_x(outEntry(infos[v], OrthoDir::East, llpos))
			- llpos*infos[v].delta(OrthoDir::East, OrthoDir::North)
			- infos[v].eps(OrthoDir::East,OrthoDir::North)) ))
		{
			m_agp_x[outEntry(infos[v], OrthoDir::East, llpos)] = firstunbend -
				(numbends - llpos)*infos[v].delta(OrthoDir::East, OrthoDir::North);
			++ll_it;
			++llpos;
		}
	}

	//edges bending to the right side
	while (l_it.valid()) //&&???!!!
	{
		if (cp_x(outEntry(infos[v], OrthoDir::East, pos)) <= infos[v].coord(OrthoDir::South) + m_sep)
		{
			if (cp_x(outEntry(infos[v], OrthoDir::East, pos)) > infos[v].coord(OrthoDir::South) - infos[v].eps(OrthoDir::East, OrthoDir::South))
			{
				m_abends[outEntry(infos[v], OrthoDir::East, pos)] = BendType::ProbB2R;
				infos[v].inc_E(OrthoDir::East, OrthoDir::South);
			} else {
				m_abends[outEntry(infos[v], OrthoDir::East, pos)] = BendType::ProbB2R;//use ProbBf;
				infos[v].inc_E(OrthoDir::East, OrthoDir::South);
			}
		} else {
			m_abends[outEntry(infos[v], OrthoDir::East, pos)] = BendType::ProbB1R;
			infos[v].inc_E_hook(OrthoDir::East, OrthoDir::South);
		}
		if (lastunbend != m_init)
		{
			m_agp_x[outEntry(infos[v], OrthoDir::East, pos)] = lastunbend + infos[v].delta(OrthoDir::East, OrthoDir::South);
			lastunbend = lastunbend + infos[v].delta(OrthoDir::East, OrthoDir::South);
		} else {
			m_agp_x[outEntry(infos[v], OrthoDir::East, pos)] =
			    infos[v].coord(OrthoDir::South)
			    - infos[v].eps(OrthoDir::East, OrthoDir::South)
			    - (infos[v].inList(OrthoDir::East).size() - 1 - pos)*infos[v].delta(OrthoDir::East, OrthoDir::South);
		}
		++l_it;
		++pos;
	}

	//bottom
	l_it = infos[v].inList(OrthoDir::West).begin();
	//classify edges
	pos = updateBends(v, l_it, true, OrthoDir::North, false, false);
	int rightbend = pos; //save number of actually bend edges to correct their positions later

	//bendfree edges
	firstcheck = true;
	lastunbend = m_init;
	firstunbend = m_init;
	while (l_it.valid() && ( infos[v].coord(OrthoDir::South)  >=
		(cp_x(outEntry(infos[v], OrthoDir::West, pos))
		+ (infos[v].inList(OrthoDir::West).size() - 1 - pos)*infos[v].delta(OrthoDir::West, OrthoDir::North)
		+ infos[v].eps(OrthoDir::West,OrthoDir::North)) ))
	{
		m_abends[outEntry(infos[v], OrthoDir::West, pos)] = BendType::BendFree;
		infos[v].num_bend_free_increment(OrthoDir::West);
		lastunbend = m_agp_x[outEntry(infos[v], OrthoDir::West, pos)] = cp_x(outEntry(infos[v], OrthoDir::West, pos));
		if (firstunbend == m_init) firstunbend = lastunbend;
		if (firstcheck)
		{
			firstcheck = false;
			infos[v].set_b_right(lastunbend);
		}
		infos[v].set_b_left(lastunbend);
		++l_it;
		++pos;
	}

	//temporary sol, correct left edges

	if (firstunbend != m_init)
	{
		ListIterator<edge> ll_it = infos[v].inList(OrthoDir::West).begin();
		int llpos = 0;
		while (ll_it.valid() && ( infos[v].coord(OrthoDir::North)  >
			(cp_x(outEntry(infos[v], OrthoDir::West, llpos)) - llpos*infos[v].delta(OrthoDir::West, OrthoDir::North) - infos[v].eps(OrthoDir::West,OrthoDir::North)) ))
		{
			//ab unterem Rand, oder ab gen, teile Abstand Anzahl Kanten?
			m_agp_x[outEntry(infos[v], OrthoDir::West, llpos)] = firstunbend -
				(rightbend - llpos)*infos[v].delta(OrthoDir::West, OrthoDir::North);
			++ll_it;
			++llpos;
		}
		OGDF_ASSERT(rightbend == llpos);
	}

	//edges bending upwards
	updateBends(v, l_it, pos, lastunbend, true, OrthoDir::South, true, true, true);
	//y values
	//y coordinates, just on the cage boundary
	l_it = infos[v].inList(OrthoDir::East).begin();
	int l_pos = 0;
	while (l_it.valid())
	{
		m_agp_y[outEntry(infos[v], OrthoDir::East, l_pos)] = infos[v].coord(OrthoDir::East);
		++l_it;
		l_pos++;
	}
	l_it = infos[v].inList(OrthoDir::West).begin();
	l_pos = 0;
	while (l_it.valid())
	{
		m_agp_y[outEntry(infos[v], OrthoDir::West, l_pos)] = infos[v].coord(OrthoDir::West);
		++l_it;
		l_pos++;
	}
}

void EdgeRouter::set_corners(node v)
{
	//set the layout position
	//set the expandedNode entries for the corners, should be where they are inserted
	edge e;
	node w;
	const OrthoRep::VertexInfoUML* vinfo = m_orp->cageInfo(v);
	adjEntry ae = vinfo->m_corner[0];
	e = *ae;
	w = e->source();//pointing towards north, on left side
	m_prup->setExpandedNode(w, v);

	m_layoutp->x(w) = infos[v].coord(OrthoDir::North);
	m_layoutp->y(w) = infos[v].coord(OrthoDir::West);
	ae = vinfo->m_corner[1];
	e = *ae;
	w = e->source();
	m_prup->setExpandedNode(w, v);

	m_layoutp->x(w) = infos[v].coord(OrthoDir::North);
	m_layoutp->y(w) = infos[v].coord(OrthoDir::East);
	ae = vinfo->m_corner[2];
	e = *ae;
	w = e->source();
	m_prup->setExpandedNode(w, v);

	m_layoutp->x(w) = infos[v].coord(OrthoDir::South);
	m_layoutp->y(w) = infos[v].coord(OrthoDir::East);
	ae = vinfo->m_corner[3];
	e = *ae;
	w = e->source();
	m_prup->setExpandedNode(w, v);

	m_layoutp->x(w) = infos[v].coord(OrthoDir::South);
	m_layoutp->y(w) = infos[v].coord(OrthoDir::West);
}

//locally decide where to place the node in the computed cage area
//allow individual separation and overhang distance, input original node v
//classify edges with preliminary bend_types prob_xxx
//choose bendfree edges, bend edges may be rerouted to save bends
void EdgeRouter::compute_place(node v, NodeInfo& inf/*, int l_sep, int l_overh*/)
{
	const int l_directvalue = 10; //value of edges connecting two cages bendfree, with bends: value = 1

	//gen at left or right cage side
	bool horizontal_merger = (inf.has_gen(OrthoDir::North) || inf.has_gen(OrthoDir::South));
	//gen at top or bottom side
	bool vertical_merger = (inf.has_gen(OrthoDir::West) || inf.has_gen(OrthoDir::East));

	List<edge> l_horz; //contains horizontal incoming edges at v's cage,  sorted increasing uppe, paper L
	List<edge> l_horzl; //by increasing lowe

	List<int> edgevalue; //saves value for direct / bend edges in lhorz

	//for every element of list l_horzl, we store its iterator in l_horz, ist Wahnsinn, spaeter global
	EdgeArray< ListIterator<edge> > horz_entry(*m_prup);
	EdgeArray< ListIterator<edge> > vert_entry(*m_prup);
	EdgeArray< ListIterator<int> > value_entry(*m_prup);
	EdgeArray<bool> valueCounted(*m_prup, false); //did we consider edge in numunbend sum?
	List<edge> l_vert; //         vertical
	List<edge> l_vertl; //by increasing lefte
	//attachment side, maybe check the direction instead
	EdgeArray<bool> at_left(*m_prup, false);
	EdgeArray<bool> at_top(*m_prup, false);

	//Fill edge lists
	int lhorz_size = inf.inList(OrthoDir::North).size() + inf.inList(OrthoDir::South).size();
	if (lhorz_size && (!horizontal_merger))
	{
		edge e;
		//fill l_horz sorting by uppe, remember entry for edges sorted by lowe
		//all inf.inList[North] and [South], sorted by lower

		// check: each side is already sorted by lowe/uppe definition!
		ListIterator<edge> li_l = inf.inList(OrthoDir::North).begin();
		ListIterator<edge> li_r = inf.inList(OrthoDir::South).begin();
		//iterators for increasing lower value
		ListIterator<edge> li_ll = inf.inList(OrthoDir::North).begin();
		ListIterator<edge> li_lr = inf.inList(OrthoDir::South).begin();

		int uppe_l, uppe_r, lowe_l, lowe_r;

		uppe_l = li_l.valid() ? auppe[outEntry(inf, OrthoDir::North, 0)] : std::numeric_limits<int>::max(); // only both 10000.0 if both empty!!!
		uppe_r = li_r.valid() ? auppe[outEntry(inf, OrthoDir::South, 0)] : std::numeric_limits<int>::max(); // never in for - loop
		lowe_l = li_ll.valid() ? alowe[outEntry(inf, OrthoDir::North, 0)] : std::numeric_limits<int>::max(); // only both 10000.0 if both empty!!!
		lowe_r = li_lr.valid() ? alowe[outEntry(inf, OrthoDir::South, 0)] : std::numeric_limits<int>::max(); // never in for - loop

		int lcount, rcount, llcount, rlcount;//zaehle fuer outEntry in Listen
		lcount = rcount = llcount = rlcount = 0;
		//forall horizontal edges, sort
		for (int k = 0; k < lhorz_size; k++)
		{
			node nextNeighbour;
			//run in parallel over opposite side lists
			//upper value
			if (uppe_l <= uppe_r) //favour left edges, maybe we should prefer them
			{
				e = *li_l;
				at_left[e] = true;
				nextNeighbour = inf.is_in_edge(OrthoDir::North, lcount) ? e->source() : e->target();
				++li_l;
				++lcount;
				uppe_l = lcount < inf.inList(OrthoDir::North).size() ? auppe[outEntry(inf, OrthoDir::North, lcount)] : std::numeric_limits<int>::max();
			}
			else
			{
				e = *li_r;
				nextNeighbour = inf.is_in_edge(OrthoDir::South, rcount) ? e->source() : e->target();
				++li_r;
				++rcount;
				uppe_r = rcount < inf.inList(OrthoDir::South).size() ? auppe[outEntry(inf, OrthoDir::South, rcount)] : std::numeric_limits<int>::max();
			}
			horz_entry[e] = l_horz.pushBack(e);//fill edge in L

			value_entry[e] = edgevalue.pushBack(m_prup->expandedNode(nextNeighbour) ? l_directvalue : 1);

			//lower value
			if (lowe_l <= lowe_r) //favour left edges, maybe we should prefer them
			{
				e = *li_ll;
				// at_left[e] = true;
				++li_ll;
				++llcount;
				lowe_l = llcount < inf.inList(OrthoDir::North).size() ? alowe[outEntry(inf, OrthoDir::North, llcount)] : std::numeric_limits<int>::max();
			}
			else
			{
				e = *li_lr;
				++li_lr;
				++rlcount;
				// lowe_r = li_lr.valid() ? lowe[*li_lr] : 10000.0;
				lowe_r = rlcount < inf.inList(OrthoDir::South).size() ? alowe[outEntry(inf, OrthoDir::South, rlcount)] : std::numeric_limits<int>::max();
			}
			l_horzl.pushBack(e); //fill edge in e_0,...e_k
		}
	}
	//vertical edges
	int lvert_size = inf.inList(OrthoDir::East).size() + inf.inList(OrthoDir::West).size();
	if (lvert_size && !vertical_merger)
	{
		//fill l_vert
		//fill l_vert sorting by righte, remember entry for edges sorted by lefte
		//all inf.inList[East] and [West], sorted by lefte

		// check: each side is already sorted by lefte/righte definition!
		ListIterator<edge> li_t = inf.inList(OrthoDir::East).begin();
		ListIterator<edge> li_b = inf.inList(OrthoDir::West).begin();
		//iterators for increasing lefte value
		ListIterator<edge> li_lt = inf.inList(OrthoDir::East).begin();
		ListIterator<edge> li_lb = inf.inList(OrthoDir::West).begin();

		int righte_t, righte_b, lefte_t, lefte_b;
		righte_t = li_t.valid() ? arighte[outEntry(inf, OrthoDir::East, 0)] : std::numeric_limits<int>::max(); // only both 10000.0 if both empty!!!
		righte_b = li_b.valid() ? arighte[outEntry(inf, OrthoDir::West, 0)] : std::numeric_limits<int>::max(); // never in for - loop
		lefte_t = li_lt.valid() ? alefte[outEntry(inf, OrthoDir::East, 0)] : std::numeric_limits<int>::max(); // only both 10000.0 if both empty!!!
		lefte_b = li_lb.valid() ? alefte[outEntry(inf, OrthoDir::West, 0)] : std::numeric_limits<int>::max(); // never in for - loop

		//forall horizontal edges, sort
		int tcount, bcount, tlcount, blcount;//zaehle fuer outEntry in Listen
		tcount = bcount = tlcount = blcount = 0;
		for (int k = 0; k < lvert_size; k++)
		{
			edge e = nullptr;
			if (li_t.valid() || li_b.valid())
			{
				//righter value
				if (righte_t <= righte_b) //favour top edges, maybe we should prefer them
				{
					if (li_t.valid())
					{
						e = *li_t;
						at_top[e] = true;
						++li_t;
						++tcount;
					}
					righte_t = tcount < inf.inList(OrthoDir::East).size() ? arighte[outEntry(inf, OrthoDir::East, tcount)] : std::numeric_limits<int>::max();
				}
				else
				{
					if (li_b.valid())
					{
						e = *li_b;
						++li_b;
						++bcount;
					}
					righte_b = bcount < inf.inList(OrthoDir::West).size()  ? arighte[outEntry(inf, OrthoDir::West, bcount)] : std::numeric_limits<int>::max();
				}
				vert_entry[e] = l_vert.pushBack(e);//fill edge in L
			}
			//lefter value
			if (lefte_t <= lefte_b) //favour top edges, maybe we should prefer them
			{
				e = *li_lt;
				// at_left[e] = true;
				++li_lt;
				++tlcount;
				// lefte_t = li_lt.valid() ? lefte[*li_lt] : 100000.0;
				lefte_t = tlcount < inf.inList(OrthoDir::East).size() ? alefte[outEntry(inf, OrthoDir::East, tlcount)] : std::numeric_limits<int>::max();
			}
			else
			{
				e = *li_lb;
				++li_lb;
				++blcount;
				// lefte_b = li_lb.valid() ? lefte[*li_lb] : 100000.0;
				lefte_b = blcount < inf.inList(OrthoDir::West).size() ? alefte[outEntry(inf, OrthoDir::West, blcount)] : std::numeric_limits<int>::max();
			}
			OGDF_ASSERT(e);
			l_vertl.pushBack(e);
		}
	}

	//we inserted the edges incident to v int the list l_horz (horizontal edges sorted
	//by upper) / l_horzl (horizontal edges sorted by lower) and l_vert (vertical edges
	//sorted by righter / l_vertl (vertical edges sorted by lefter)
	int boxx = inf.node_xsize();
	int boxy = inf.node_ysize();

	//now two iterations are executed to compute the best starting index
	//in the list of horizontal/vertical edges to maximise unbend edges

	//some variables for the placement iteration
	int num_unbend = 0; //current number of unbend edges(paper: size)
	int best_unbend = -1;//best number of unbend edges so far (paper: best)
	int i;

	//vertical position paper ALGORITHM1
	//check e_i sorted by lowe, l_horz sorted by uppe !
	//we need to have an listentry for all edges sorted by lowe to
	//delete them in l_horz

	if (!l_horz.empty())
	{
		//starting from the lowest horizontal edge, we move the virtual node box up and count the number of potentially
		//unbend edges to find the best starting unbend edge additionally, we set the values for the connection/glue point position
		int stop = l_horz.size();
		int bestvalue = m_init;

		if (l_horz.size() == 1)
		{
			best_unbend = 1;
			if (at_left[*(l_horz.begin())])
				bestvalue = alowe[outEntry(inf, OrthoDir::North, 0)];
			else bestvalue = alowe[outEntry(inf, OrthoDir::South, 0)];
		}
		else
		{
			ListIterator<edge> p = l_horz.begin();
			ListIterator<int> valp = edgevalue.begin();// run over edgevalue in parallel to p

			int leftcount = 0, rightcount = 0;
			for (i = 1; i < stop+1; i++) //in der Regel laenger als noetig??
			{
				if (!valueCounted[l_horzl.front()])
				{
					num_unbend += *value_entry[l_horzl.front()];
					valueCounted[l_horzl.front()] = true;
				}
				while (p.valid())
				{
					//the edge indicated by p fits in the box started with l_horzl.front lower value
					if (uppe[*p]<= (lowe[l_horzl.front()]+boxy)) //+machineeps)) //||(p == horz_entry[l_horzl.front()]))
					//assert first edge int horzl is edge i?
					//p's entry needs no bend => increase num_unbend (edges)
					{
						//num_unbend++;
						num_unbend += *valp;
						valueCounted[*p] = true;
						//only to be set if new best_unbend
						++p;
						++valp;
					} else {
						break;
					}
				}

				if (num_unbend > best_unbend)
				{
					best_unbend =  num_unbend;
					bestvalue = at_left[l_horzl.front()] ? alowe[outEntry(inf,OrthoDir::North, leftcount)] : alowe[outEntry(inf, OrthoDir::South, rightcount)];
				}

				if (at_left[l_horzl.front()]) leftcount++;
				else rightcount++;

				if ( p == horz_entry[l_horzl.front()]) ++p;
				if ( valp == value_entry[l_horzl.front()]) ++valp;

				l_horz.del(horz_entry[l_horzl.front()]);

				if (num_unbend) num_unbend -= *value_entry[l_horzl.front()];

				OGDF_ASSERT(num_unbend >= 0);

				edgevalue.del(value_entry[l_horzl.front()]);

				valueCounted[l_horzl.front()] = false;

				l_horzl.popFront();
			}
		}

		m_newy[v] = min((inf.cageCoord(OrthoDir::East) - inf.node_ysize() - inf.rc(OrthoDir::East)), bestvalue);
		inf.set_coord(OrthoDir::West, m_newy[v]);
		inf.set_coord(OrthoDir::East, m_newy[v] + inf.node_ysize());
	} else {
		if (horizontal_merger)
		{
			//position is fixed //North was OrthoDir::North
			edge e;
			//note that get starts indexing with zero, whereas gen_pos starts with one
			if (inf.has_gen(OrthoDir::North)) e = *(inf.inList(OrthoDir::North).get(inf.gen_pos(OrthoDir::North)));
			else e = *(inf.inList(OrthoDir::South).get(inf.gen_pos(OrthoDir::South))); // XXX: check e
			int gen_y = m_layoutp->y( e->target()); // XXX: koennte man auch in inf schreiben!
			m_newy[v] =  gen_y - int(floor((double)(inf.node_ysize())/2));
			inf.set_coord(OrthoDir::West, m_newy[v]);
			inf.set_coord(OrthoDir::East, m_newy[v] + inf.node_ysize());
		} else {
			//new heuristics: look for vertical generalization and shift towards it
			if (vertical_merger)
			{
				//find out direction of generalization
				bool wg = inf.has_gen(OrthoDir::West);
				bool eg = inf.has_gen(OrthoDir::East);
				int mynewy = 0;
				if (wg)
				{
					if (eg)
					{
						if (inf.is_in_edge(OrthoDir::West, inf.gen_pos(OrthoDir::West))) //position to OrthoDir::East
						{
							mynewy = inf.cageCoord(OrthoDir::East) - inf.rc(OrthoDir::East) - inf.node_ysize();
						} else {
							mynewy = inf.cageCoord(OrthoDir::West) + inf.rc(OrthoDir::West);
						}
					} else {
						mynewy = inf.cageCoord(OrthoDir::West) + inf.rc(OrthoDir::West);
					}
				} else {
					mynewy = inf.cageCoord(OrthoDir::East) - inf.rc(OrthoDir::East) - inf.node_ysize();
				}
				m_newy[v] = mynewy;
				inf.set_coord(OrthoDir::West, m_newy[v]);
				inf.set_coord(OrthoDir::East, m_newy[v] + inf.node_ysize());
			} else {
				//we place the node at the position cage_lower_border(v)+routing_channel_bottom(v)
				m_newy[v] = inf.cageCoord(OrthoDir::East) - inf.rc(OrthoDir::East) - inf.node_ysize();
				inf.set_coord(OrthoDir::West, m_newy[v]);
				inf.set_coord(OrthoDir::East, m_newy[v] + inf.node_ysize());
				//forall horizontal edges set their glue point y coordinate
			}
		}
	}

	//forall horizontal edges we computed the y-coordinate of their glue point in m_gp_y
	//and we computed the y-coordinate of the lower box segment in m_newy
	//horizontal position
	if (!l_vert.empty())
	{
		//starting from the leftmost vertical edge, we move the virtual
		//node box rightwards and count the number of potentially unbend edges
		//to find the best starting unbend edge
		num_unbend = 0;
		best_unbend = -1;
#if 0
		edge bestedge;
#endif
		int bestvalue = m_init;
		int stop = l_vert.size();
		//bugfix
		if (l_vert.size() == 1)
		{
			best_unbend = 1;
#if 0
			bestedge = l_vert.front();
#endif
			if (at_top[*(l_vert.begin())])
				bestvalue = alefte[outEntry(inf, OrthoDir::East, 0)];
			else bestvalue = alefte[outEntry(inf, OrthoDir::West, 0)];
		}
		else
		{
			//ALGORITHM 1
			int topcount = 0, lowcount = 0;
			ListIterator<edge> p = l_vert.begin(); //pointer on paper list L
			for (i = 1; i < stop+1; i++)
			{
				while (p.valid())
				{
#if 1
					if (righte[*p] <= lefte[l_vertl.front()] + boxx + machineeps) //assert first edge is edge i?
#else
					if (p == vert_entry[l_vertl.front()]))
#endif
					//p's entry needs no bend => increase num_unbend (edges)
					{
						num_unbend++;
						++p;
					} else {
						break;
					}
				}
				if (num_unbend > best_unbend)
				{
					best_unbend =  num_unbend;
					//bestedge = l_vertl.front();
					bestvalue = at_top[l_vertl.front()] ? alefte[outEntry(inf,OrthoDir::East, topcount)] : alefte[outEntry(inf, OrthoDir::West, lowcount)];
				}
				if (at_top[l_vertl.front()]) topcount++;
				else lowcount++;

				if (p == vert_entry[l_vertl.front()]) ++p; //may be -- if valid

				OGDF_ASSERT(p != vert_entry[l_vertl.front()]);

				l_vert.del(vert_entry[l_vertl.front()]);
				l_vertl.popFront();

				if (num_unbend) num_unbend--;  //the next index will bend current start edge
			}
		}

		//assign computed value
		m_newx[v] = min( (inf.cageCoord(OrthoDir::South) - inf.node_xsize() - inf.rc(OrthoDir::South)),
			(bestvalue));
		// (lefte[bestedge]));
		inf.set_coord(OrthoDir::North, m_newx[v]);
		inf.set_coord(OrthoDir::South, m_newx[v] + inf.node_xsize());
		// XXX: do we need this here?
		//- (at_left[bestedge] ?
		// inf.eps(OrthoDir::North, OrthoDir::West) : inf.eps(OrthoDir::South, OrthoDir::West))));//maybe top / down epsilon
	} else {
		if (vertical_merger)
		{
			//position is fixed //North was OrthoDir::North
			edge e;
			//note that get starts indexing with zero, whereas gen_pos starts with one
			if (inf.has_gen(OrthoDir::East)) e = *(inf.inList(OrthoDir::East).get(inf.gen_pos(OrthoDir::East)));
			else e = *(inf.inList(OrthoDir::West).get(inf.gen_pos(OrthoDir::West))); // XXX: check e
			int gen_x = m_layoutp->x( e->target());
			m_newx[v] =  gen_x - int(inf.node_xsize()/2.0);// XXX: abziehen => aufrunden!
			inf.set_coord(OrthoDir::North, m_newx[v]);
			inf.set_coord(OrthoDir::South, m_newx[v] + inf.node_xsize());
		} else {
			if (horizontal_merger)
			{
				//find out direction of generalization
				bool sg = inf.has_gen(OrthoDir::South);
				bool ng = inf.has_gen(OrthoDir::North);

				int mynewx = 0;
				if (sg)
				{
					if (ng)
					{
						if (inf.is_in_edge(OrthoDir::South, inf.gen_pos(OrthoDir::South))) //position to OrthoDir::North
						{
							mynewx = inf.cageCoord(OrthoDir::North) + inf.rc(OrthoDir::North);
						} else {
							mynewx = inf.cageCoord(OrthoDir::South) - inf.rc(OrthoDir::South) - inf.node_xsize();
						}
					} else {
						mynewx = inf.cageCoord(OrthoDir::South) - inf.rc(OrthoDir::South) - inf.node_xsize();
					}
				} else {
					mynewx = inf.cageCoord(OrthoDir::North) + inf.rc(OrthoDir::North);
				}
				m_newx[v] = mynewx;
				inf.set_coord(OrthoDir::North, mynewx);
				inf.set_coord(OrthoDir::South, mynewx + inf.node_xsize());
			} else {
				//we place the node at the position cage_left_border(v)+routing_channel_left(v)
				//should be handled by value assign
				m_newx[v] = inf.cageCoord(OrthoDir::South) - inf.rc(OrthoDir::South) - inf.node_xsize();
				inf.set_coord(OrthoDir::North, m_newx[v]);
				inf.set_coord(OrthoDir::South, m_newx[v] + inf.node_xsize());
			}
		}
	}

	if (m_mergerSon[v])
	{
		if (vertical_merger) //change vertical position
		{
			//test incons.
			OGDF_ASSERT(!horizontal_merger);
			//we have to know the exact direction of the edge to the merger
			OGDF_ASSERT( (m_mergeDir[v] == OrthoDir::North) || (m_mergeDir[v] == OrthoDir::South));
			if (m_mergeDir[v] == OrthoDir::North)
				m_newy[v] = (inf.cageCoord(OrthoDir::East) - inf.node_ysize() - inf.rc(OrthoDir::East));
			else
				m_newy[v] = (inf.cageCoord(OrthoDir::West) + inf.rc(OrthoDir::West));
			inf.set_coord(OrthoDir::West, m_newy[v]);
			inf.set_coord(OrthoDir::East, m_newy[v] + inf.node_ysize());
		}
		if (horizontal_merger)
		{
			//test inconsistency
			OGDF_ASSERT(!vertical_merger);
			//we have to know the exact direction of the edge to the merger
			OGDF_ASSERT( (m_mergeDir[v] == OrthoDir::East) || (m_mergeDir[v] == OrthoDir::West));
			if (m_mergeDir[v] == OrthoDir::West)
				m_newx[v] = inf.cageCoord(OrthoDir::North) + inf.rc(OrthoDir::North);
			else
				m_newx[v] = inf.cageCoord(OrthoDir::South) - inf.rc(OrthoDir::South) - inf.node_xsize();
			inf.set_coord(OrthoDir::North, m_newx[v]);
			inf.set_coord(OrthoDir::South, m_newx[v] + inf.node_xsize());
		}
	}

	//now we have vertical as well as horizontal position and can assign both values
	if (horizontal_merger)
		compute_gen_glue_points_y(v);
	else compute_glue_points_y(v);
	if (vertical_merger)
		compute_gen_glue_points_x(v);
	else compute_glue_points_x(v);
		set_corners(v);
	//we computed the new placement position for original node box v and stored it in m_new as well as in inf (debug)
	//assert some assigment
#if 0
	{
		if ( (m_prup->expandAdj(v) != 0) && (m_prup->typeOf(v) != Graph::generalizationMerger) )
		{
			OGDF_ASSERT(m_newx[v] >= inf.cageCoord(OrthoDir::North));
			OGDF_ASSERT(m_newy[v] >= inf.cageCoord(OrthoDir::West));
			OGDF_ASSERT(m_newx[v] + inf.node_xsize() <= inf.cageCoord(OrthoDir::South));
			OGDF_ASSERT(m_newy[v] + inf.node_ysize() <= inf.cageCoord(OrthoDir::East));
		}
	}
#endif

	//now assign boolean values for reroutability: is edge to box distance large enough to allow rerouting
	//{} is done in call function in classify_edges()

	//we computed the new placement position for original node box v
	//and stored it in m_newxy as well as in inf (debug)
}

//REAL PLACEMENT Change graph based on placement/rerouting
void EdgeRouter::place(node l_v)
{
	//two steps: first, introduce the bends on the incoming edges,
	// normalise, then adjust the layout information for the cage nodes and bend nodes
	string m;
	string msg;
	OrthoRep::VertexInfoUML* vinfo = m_orp->cageInfo(l_v);
	edge e;
	bool inedge;
	bool corn, acorn; //test on last and first corner transition after rerouting
	//forall four sides check the edges

	//NORTH SIDE
	//integrate offset for double bend edges without bendfree edges because of rerouting
	int leftofs = infos[l_v].num_bend_free(OrthoDir::North) ? 0 :
		infos[l_v].delta(OrthoDir::North, OrthoDir::West)*infos[l_v].flips(OrthoDir::West, OrthoDir::North);
	int rightofs = infos[l_v].num_bend_free(OrthoDir::North) ? 0 :
		infos[l_v].delta(OrthoDir::North, OrthoDir::East)*infos[l_v].flips(OrthoDir::East, OrthoDir::North);

	List<edge>& inlist = infos[l_v].inList(OrthoDir::North); //left side
	ListIterator<edge> it = inlist.begin();
	int ipos = 0;
	corn = false; //check if end corner changed after necessary rerouting
	acorn = false; //check if start corner changed after necessary re

	while (it.valid())
	{
		e = *it;
		node v;
		adjEntry ae; //"outgoing" from cage
		inedge = infos[l_v].is_in_edge(OrthoDir::North, ipos);
		if (inedge)
		{
			ae = e->adjTarget();
		}
		else
		{
			ae = e->adjSource();
		}
		v = m_cage_point[ae];

		if (m_processStatus[v] == ProcessType::used)
		{
			++it;//should be enough to break
			continue;
		}
		adjEntry saveadj = ae;
		//correct possible shift by bends from other cage
		if (!((inedge && (v == e->target()))
			|| ((v == e->source()) && !inedge) ))
		{
			edge run = e;
			if (inedge)
			{
				adjEntry runadj = run->adjSource();
				while (v != runadj->theEdge()->target()) {
					runadj = runadj->faceCycleSucc();
				}
				e = runadj->theEdge();
				OGDF_ASSERT(v == runadj->twin()->cyclicSucc()->theNode());
				saveadj = runadj->twin();
			}
			OGDF_ASSERT(((v == e->target()) && (inedge)) || (v == e->source()));
		}
		//position
		if ((m_agp_x[ae] != m_init) &&  (m_agp_y[ae] != m_init))
			set_position(v, m_agp_x[ae], m_agp_y[ae]);
		OGDF_ASSERT(m_agp_y[ae] != m_init);
		OGDF_ASSERT(m_agp_x[ae] != m_init);

		//bends
		if (abendType(ae) != BendType::BendFree)
		{
			edge newe;
			node newbend, newglue;
			int xtacy;
			switch (abendType(ae))
			{
				//case ProbB1L:
				case BendType::Bend1Left: //rerouted single bend
					//delete old corner
					if (ipos == 0)
					{
						OGDF_ASSERT(infos[l_v].flips(OrthoDir::North, OrthoDir::West) > 0);
						adjEntry ae2 = saveadj->cyclicPred();//ae->cyclicPred(); //aussen an cage
						adjEntry ae3 = ae2->faceCycleSucc();
						//as long as unsplit does not work, transition
						//node oldcorner;
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							//oldcorner = ae2->theEdge()->target();
							unsplit(ae2->theEdge(), ae3->theEdge());
						} else {
							unsplit(ae3->theEdge(), ae2->theEdge());
							//oldcorner = ae2->theEdge()->source();
						}
					}
					//delete corner after last rerouting
					if ((!acorn) && (ipos == infos[l_v].flips(OrthoDir::North, OrthoDir::West) - 1))
					{
						acorn = true;
						edge newe2;
						adjEntry ae2 = saveadj->cyclicSucc(); //next to the right in cage
						edge savedge = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[0] = newe2->adjSource();
						}
						else {
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[0] = savedge->adjTarget();
						}
						fix_position(newe2->source(), infos[l_v].coord(OrthoDir::North), infos[l_v].coord(OrthoDir::West));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}

					xtacy = infos[l_v].coord(OrthoDir::North)
						+ infos[l_v].delta(OrthoDir::West, OrthoDir::North)
						*(infos[l_v].flips(OrthoDir::North, OrthoDir::West) - 1 - ipos)
						+ infos[l_v].eps(OrthoDir::West, OrthoDir::North);
					if (inedge) newe = addLeftBend(e);
					else newe = addRightBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();
					fix_position(newglue, xtacy, infos[l_v].coord(OrthoDir::West));
					fix_position(newbend, xtacy, cp_y(ae));
					break;
				case BendType::ProbB1L:
				case BendType::ProbB2L:
				case BendType::Bend2Left:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae], m_agp_y[ae]+leftofs);
					xtacy = infos[l_v].cageCoord(OrthoDir::North)
						+ (infos[l_v].num_bend_edges(OrthoDir::North, OrthoDir::West) -ipos)*m_sep;
					newe = addLeftBend(e);
					fix_position(newe->source(), xtacy, inedge ? cp_y(ae) : gp_y(ae) + leftofs);
					newe = addRightBend(newe);
					fix_position(newe->source(), xtacy, inedge ? gp_y(ae) + leftofs : cp_y(ae));
					break;//int bend downwards
				case BendType::Bend1Right:
					if (!corn)
					{
						corn = true;
						edge newe2;
						adjEntry ae2 = saveadj->cyclicPred();//ae->cyclicPred();
						edge savedge = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[1] = savedge->adjTarget();
						}
						else {
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[1] = newe2->adjSource();
						}
						fix_position(newe2->source(), infos[l_v].coord(OrthoDir::North), infos[l_v].coord(OrthoDir::East));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}
					//delete corner after last rerouting
					if (corn && (ipos == infos[l_v].inList(OrthoDir::North).size()-1))
					{
						adjEntry ae2 = saveadj->cyclicSucc();//ae->cyclicSucc();
						adjEntry ae3 = ae2->faceCycleSucc();
						if (ae2 == (ae2->theEdge()->adjSource()))
							unsplit(ae2->theEdge(), ae3->theEdge());
						else
							unsplit(ae3->theEdge(), ae2->theEdge());
					}
					xtacy = infos[l_v].coord(OrthoDir::North)
						+ (ipos + infos[l_v].flips(OrthoDir::North, OrthoDir::East) - infos[l_v].inList(OrthoDir::North).size())
						* infos[l_v].delta(OrthoDir::East, OrthoDir::North) + infos[l_v].eps(OrthoDir::East, OrthoDir::North);
					if (inedge) newe = addRightBend(e);
					else newe = addLeftBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();
					fix_position(newbend, xtacy, cp_y(ae));
					fix_position(newglue, xtacy, infos[l_v].coord(OrthoDir::East));
					break;//rerouted single bend
				case BendType::ProbB1R:
				case BendType::ProbB2R:
				case BendType::Bend2Right:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae], m_agp_y[ae]-rightofs);
					xtacy = infos[l_v].cageCoord(OrthoDir::North)
						+ (1 + ipos + infos[l_v].num_bend_edges(OrthoDir::North, OrthoDir::East) - infos[l_v].inList(OrthoDir::North).size())
						* m_sep;
					//* infos[l_v].delta(OrthoDir::North, OrthoDir::East);
					newe = addRightBend(e);
					fix_position(newe->source(), xtacy, inedge ? cp_y(ae) : gp_y(ae) - rightofs);
					newe = addLeftBend(newe);
					fix_position(newe->source(), xtacy, inedge ? gp_y(ae) - rightofs : cp_y(ae));
					break; //double bend upwards
				default: break;
			}
			m_orp->normalize();
		}
		++ipos;
		++it;
	}

	//EAST SIDE (bottom)
	leftofs = infos[l_v].num_bend_free(OrthoDir::East) ? 0 :
		infos[l_v].delta(OrthoDir::East, OrthoDir::North)*infos[l_v].flips(OrthoDir::North, OrthoDir::East);
		//temp
		//leftofs = infos[l_v].delta(OrthoDir::East, OrthoDir::North)*infos[l_v].flips(OrthoDir::North, OrthoDir::East);
		//temp end
	rightofs = infos[l_v].num_bend_free(OrthoDir::East) ? 0 :
		infos[l_v].delta(OrthoDir::East, OrthoDir::South)*infos[l_v].flips(OrthoDir::South, OrthoDir::East);
		//temp
		//rightofs = infos[l_v].delta(OrthoDir::East, OrthoDir::South)*infos[l_v].flips(OrthoDir::South, OrthoDir::East);
		//tempend

	it = infos[l_v].inList(OrthoDir::East).begin();
	ipos = 0;
	corn = false; //check if end corner changed after necessary rerouting
	acorn = false; //check if start corner changed after necessary re
	while (it.valid())
	{
		e = *it;
		node v;
		adjEntry ae; //"outgoing" from cage
		inedge = infos[l_v].is_in_edge(OrthoDir::East, ipos);
		if (inedge)
		{
			ae = e->adjTarget();
		}
		else
		{
			ae = e->adjSource();
		}
		v = m_cage_point[ae];
		if (m_processStatus[v] == ProcessType::used)
		{
			++it;//should be enough to break
			continue;
		}
		adjEntry adjsave = ae;
		//correct possible shift by bends from other cage
		if (!((inedge && (v == e->target()))
			|| ((v == e->source()) && !inedge) ))
		{
			edge run = e;
			if (inedge)
			{
				adjEntry runadj = run->adjSource();
				while (v != runadj->theEdge()->target()) {runadj = runadj->faceCycleSucc(); }
				e = runadj->theEdge();
				OGDF_ASSERT(v == runadj->twin()->cyclicSucc()->theNode());
				adjsave = runadj->twin();
			}
			OGDF_ASSERT((v == e->target()) ||(v == e->source()));
		}
		OGDF_ASSERT((v == e->target()) ||(v == e->source()));
		//position
		if ((m_agp_x[ae] != m_init) && (m_agp_y[ae] != m_init)) set_position(v, m_agp_x[ae], m_agp_y[ae]);
		OGDF_ASSERT(m_agp_y[ae] != m_init);
		OGDF_ASSERT(m_agp_x[ae] != m_init);
		//bends
		if (abendType(ae) != BendType::BendFree)
		{
			switch (abendType(ae))
			{
				edge newe;
				node newbend, newglue;
				int ypsiqueen;
				//case ProbB1L:
				case BendType::Bend1Left:
					//set new corner before first rerouting
					//delete corner before starting
					if (ipos == 0)
					{
						adjEntry ae2 = adjsave->cyclicPred();  //ae->cyclicPred(); //aussen an cage
						adjEntry ae3 = ae2->faceCycleSucc();

						if (ae2 == (ae2->theEdge()->adjSource()))
							unsplit(ae2->theEdge(), ae3->theEdge());
						else
							unsplit(ae3->theEdge(), ae2->theEdge());
						}
						if ((!acorn) && (ipos == infos[l_v].flips(OrthoDir::East, OrthoDir::North) - 1))
						{

						acorn = true;
						edge newe2;
						adjEntry ae2 = adjsave->cyclicSucc();//next to the right in cage
						edge savedge = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[1] = newe2->adjSource();
						}
						else {
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[1] = savedge->adjTarget();
						}
						fix_position(newe2->source(), infos[l_v].coord(OrthoDir::North), infos[l_v].coord(OrthoDir::East));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}
					if (inedge) newe = addLeftBend(e); //abhaengig von inedge, ??
					else newe = addRightBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();
					ypsiqueen = infos[l_v].coord(OrthoDir::East) - (infos[l_v].flips(OrthoDir::East, OrthoDir::North) - ipos - 1)
						* infos[l_v].delta(OrthoDir::North, OrthoDir::East) - infos[l_v].eps(OrthoDir::North, OrthoDir::East);
					fix_position(newbend, cp_x(ae), ypsiqueen);
					fix_position(newglue, infos[l_v].coord(OrthoDir::North), ypsiqueen);
					//target oder source von newe coord
					break; //rerouted single bend
				case BendType::ProbB1L:
				case BendType::ProbB2L:
				case BendType::Bend2Left:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae]+leftofs, m_agp_y[ae]);
					ypsiqueen = infos[l_v].cageCoord(OrthoDir::East)
							 - (infos[l_v].num_bend_edges(OrthoDir::East, OrthoDir::North) - ipos)*m_sep;
					newe = addLeftBend(e);
					fix_position(newe->source(), inedge ? cp_x(ae) : (m_agp_x[ae] + leftofs), ypsiqueen);
					newe = addRightBend(newe);
					fix_position(newe->source(), inedge ? (m_agp_x[ae] + leftofs) : cp_x(ae), ypsiqueen);
					break;//double bend downwards
				//case ProbB1R:
				case BendType::Bend1Right:
					//set new corner before first rerouting
					if (!corn)
					{
						corn = true;
						edge newe2;
						adjEntry ae2 = adjsave->cyclicPred();
						edge le = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[2] = le->adjTarget();
						}
						else {
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[2] = newe2->adjSource();
						}
						fix_position(newe2->source(),infos[l_v].coord(OrthoDir::South), infos[l_v].coord(OrthoDir::East));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}
					ypsiqueen = infos[l_v].coord(OrthoDir::East) - (ipos + infos[l_v].flips(OrthoDir::East, OrthoDir::South) - infos[l_v].inList(OrthoDir::East).size())
												* infos[l_v].delta(OrthoDir::South, OrthoDir::East) - infos[l_v].eps(OrthoDir::South, OrthoDir::East);
					//delete corner after last rerouting
					if (corn && (ipos == infos[l_v].inList(OrthoDir::East).size() - 1))
					{
						adjEntry ae2 = adjsave->cyclicSucc();//ae->cyclicSucc();
						adjEntry ae3 = ae2->faceCycleSucc();
						//as long as unsplit does not work, transition
						//node oldcorner;
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							//oldcorner = ae2->theEdge()->target();
							unsplit(ae2->theEdge(), ae3->theEdge());
						}
						else
						{
							unsplit(ae3->theEdge(), ae2->theEdge());
							//oldcorner = ae2->theEdge()->source();
						}
					}
					if (inedge) newe = addRightBend(e);
					else newe = addLeftBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();

					fix_position(newbend, cp_x(ae), ypsiqueen);
					fix_position(newglue, infos[l_v].coord(OrthoDir::South), ypsiqueen);
					break;//rerouted single bend
				case BendType::ProbB1R:
				case BendType::ProbB2R:
				case BendType::Bend2Right:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae]-rightofs, m_agp_y[ae]);
					ypsiqueen = infos[l_v].cageCoord(OrthoDir::East) -
							 (ipos - infos[l_v].inList(OrthoDir::East).size() + infos[l_v].num_bend_edges(OrthoDir::East, OrthoDir::South))*m_sep;
							 //(infos[l_v].inList(OrthoDir::East).size() + infos[l_v].flips(OrthoDir::East, OrthoDir::South) - ipos)*m_sep;
					newe = addRightBend(e);
					fix_position(newe->source(), inedge ? cp_x(ae) : (m_agp_x[ae] - rightofs), ypsiqueen);
					newe = addLeftBend(newe);
					fix_position(newe->source(), inedge ? (m_agp_x[ae] - rightofs) : cp_x(ae), ypsiqueen);
					break; //double bend upwards
				default: break;
			}
			m_orp->normalize();
		}
		++ipos;
		++it;
	}

	//SOUTH SIDE
	leftofs = infos[l_v].num_bend_free(OrthoDir::South) ? 0 :
		infos[l_v].delta(OrthoDir::South, OrthoDir::East)*infos[l_v].flips(OrthoDir::East, OrthoDir::South);
	rightofs = infos[l_v].num_bend_free(OrthoDir::South) ? 0 :
		infos[l_v].delta(OrthoDir::South, OrthoDir::West)*infos[l_v].flips(OrthoDir::West, OrthoDir::South);
	it = infos[l_v].inList(OrthoDir::South).begin();
	ipos = 0;
	corn = false; //check if end corner changed after necessary rerouting
	acorn = false; //check if start corner changed after necessary re
	while (it.valid() && (infos[l_v].inList(OrthoDir::South).size() > 0))
	{
		e = *it;
		node v;//test
		adjEntry ae; //"outgoing" from cage
		inedge = infos[l_v].is_in_edge(OrthoDir::South, ipos);
		if (inedge)
		{
			ae = e->adjTarget();
		}
		else
		{
			ae = e->adjSource();
		}
		v = m_cage_point[ae];
		if (m_processStatus[v] == ProcessType::used)
		{
			++it;//should be enough to break
			continue;
		}
		adjEntry saveadj = ae;
		//correct possible shift by bends from other cage
		if (!((inedge && (v == e->target()))
			|| ((v == e->source()) && !inedge) )) {
			edge run = e;
			if (inedge) {
				adjEntry runadj = run->adjSource();
				while (v != runadj->theEdge()->target()) {runadj = runadj->faceCycleSucc();}
				e = runadj->theEdge();
				OGDF_ASSERT(v == runadj->twin()->cyclicSucc()->theNode());
				saveadj = runadj->twin();
			}
			OGDF_ASSERT((v == e->target()) ||(v == e->source()));
		}
		//position
		if ((m_agp_x[ae] != m_init) && (m_agp_y[ae] != m_init)) set_position(v, m_agp_x[ae], m_agp_y[ae]);
		OGDF_ASSERT(m_agp_x[ae] != m_init);
		OGDF_ASSERT(m_agp_y[ae] != m_init);
		//bends
		if (abendType(ae) != BendType::BendFree)
		{
			edge newe;
			node newbend, newglue;
			int xtacy;
			switch (abendType(ae))
			{
				case BendType::Bend1Left:
					if (!corn)
					{
						corn = true;
						edge newe2;
						adjEntry ae2 = saveadj->cyclicSucc();//next to the right in cage
						edge savedge = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[2] = newe2->adjSource();
						}
						else {
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[2] = savedge->adjTarget();
						}
						fix_position(newe2->source(), infos[l_v].coord(OrthoDir::South), infos[l_v].coord(OrthoDir::East));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}
					//delete corner after last rerouting
					if (ipos == infos[l_v].inList(OrthoDir::South).size()-1)
					{
						adjEntry ae2 = saveadj->cyclicPred();//ae->cyclicPred();
						adjEntry ae3 = ae2->faceCycleSucc();
						if (ae2 == (ae2->theEdge()->adjSource()))
							unsplit(ae2->theEdge(), ae3->theEdge());
						else
							unsplit(ae3->theEdge(), ae2->theEdge());
					}
					xtacy = infos[l_v].coord(OrthoDir::South)
						- infos[l_v].delta(OrthoDir::East, OrthoDir::South)
						*(infos[l_v].flips(OrthoDir::South, OrthoDir::East) + ipos - infos[l_v].inList(OrthoDir::South).size())
						-infos[l_v].eps(OrthoDir::East, OrthoDir::South);
					if (inedge) newe = addLeftBend(e);
					else newe = addRightBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();
					fix_position(newglue, xtacy, infos[l_v].coord(OrthoDir::East));
					fix_position(newbend, xtacy, cp_y(ae));
					break; //rerouted single bend
				case BendType::ProbB1L:
				case BendType::ProbB2L:
				case BendType::Bend2Left:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae], m_agp_y[ae]-leftofs);
					xtacy = infos[l_v].cageCoord(OrthoDir::South)
						- (ipos + 1 + infos[l_v].num_bend_edges(OrthoDir::South, OrthoDir::East)
						- infos[l_v].inList(OrthoDir::South).size())*m_sep;
					newe = addLeftBend(e);

#if 0
					gy = gp_y(ae) - infos[l_v].flips(OrthoDir::East, OrthoDir::South)*infos[l_v].delta(OrthoDir::South, OrthoDir::East);
					if (inedge) fix_position(e->target(), m_agp_x[ae], gy);
					else fix_position(e->source(), m_agp_x[ae], gy);
#endif
					fix_position(newe->source(), xtacy, inedge ? cp_y(ae) : (gp_y(ae) - leftofs));
					newe = addRightBend(newe);
					fix_position(newe->source(), xtacy, inedge ? (gp_y(ae) - leftofs) : cp_y(ae));
					break;//double bend downwards
				//case ProbB1R:
				case BendType::Bend1Right:
					//delete corner before rerouting
					if (ipos == 0)
					{
						adjEntry ae2 = saveadj->cyclicSucc();
						adjEntry ae3 = ae2->faceCycleSucc();

						//node oldcorner;
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							//oldcorner = ae2->theEdge()->target();
							unsplit(ae2->theEdge(), ae3->theEdge());
						} else {
							unsplit(ae3->theEdge(), ae2->theEdge());
							//oldcorner = ae2->theEdge()->source();
						}
					}
					//last flipped edge, insert sw - corner node
					if ((!acorn) && (ipos == infos[l_v].flips(OrthoDir::South, OrthoDir::West) - 1))
					{
						acorn = true;
						edge newe2;
						adjEntry ae2 = saveadj->cyclicPred();
						edge le = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[3] = le->adjTarget();
						}
						else {
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[3] = newe2->adjSource();
						}
						fix_position(newe2->source(), infos[l_v].coord(OrthoDir::South), infos[l_v].coord(OrthoDir::West));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}

					if (inedge) newe = addRightBend(e);
					else newe = addLeftBend(e);
					newbend = newe->source();
					//source ist immer neuer Knick, aber target haengt von der Richtung ab...
					if (inedge) newglue = newe->target();
					else newglue = e->source();
					xtacy = infos[l_v].coord(OrthoDir::South) - (infos[l_v].flips(OrthoDir::South, OrthoDir::West) - ipos - 1)
						* infos[l_v].delta(OrthoDir::West, OrthoDir::South) - infos[l_v].eps(OrthoDir::West, OrthoDir::South);
					fix_position(newbend, xtacy, cp_y(ae));
					fix_position(newglue, xtacy, infos[l_v].coord(OrthoDir::West));
					break;//rerouted single bend
				case BendType::ProbB1R:
				case BendType::ProbB2R:
				case BendType::Bend2Right:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae], m_agp_y[ae]+rightofs);
					xtacy = infos[l_v].cageCoord(OrthoDir::South) -
						(infos[l_v].num_bend_edges(OrthoDir::South, OrthoDir::West) - ipos) * m_sep;
					newe = addRightBend(e);
					fix_position(newe->source(), xtacy, inedge ? cp_y(ae) : m_agp_y[ae] + rightofs);
					newe = addLeftBend(newe);
					fix_position(newe->source(), xtacy, inedge ? m_agp_y[ae] + rightofs : cp_y(ae));
					break; //double bend downwards
				default: break;
			}
			m_orp->normalize();
		}
		++ipos;
		++it;
	}

	//WEST SIDE
	leftofs = infos[l_v].num_bend_free(OrthoDir::West) ? 0 :
		infos[l_v].delta(OrthoDir::West, OrthoDir::South)*infos[l_v].flips(OrthoDir::South, OrthoDir::West);
	rightofs = infos[l_v].num_bend_free(OrthoDir::West) ? 0 :
		infos[l_v].delta(OrthoDir::West, OrthoDir::North)*infos[l_v].flips(OrthoDir::North, OrthoDir::West);
	it = infos[l_v].inList(OrthoDir::West).begin();
	ipos = 0;
	corn = acorn = false;
	while (it.valid())
	{
		e = *it;
		node v;
		adjEntry ae; //"outgoing" from cage
		inedge = infos[l_v].is_in_edge(OrthoDir::West, ipos);
		if (inedge)
		{
			ae = e->adjTarget();
		}
		else
		{
			ae = e->adjSource();
		}
		v = m_cage_point[ae];
		if (m_processStatus[v] == ProcessType::used)
		{
			++it;//should be enough to break
			continue;
		}
		//position
		adjEntry saveadj = ae;
		//correct possible shift by bends from other cage
		if (!((inedge && (v == e->target()))
		|| ((v == e->source()) && !inedge) )) {
			edge run = e;
			if (inedge) {
				adjEntry runadj = run->adjSource();
				while (v != runadj->theEdge()->target()) {runadj = runadj->faceCycleSucc(); }
				e = runadj->theEdge();
				OGDF_ASSERT(v == runadj->twin()->cyclicSucc()->theNode());
				saveadj = runadj->twin();
			}
			OGDF_ASSERT((v == e->target()) ||(v == e->source()));
		}
		//position
		if ((m_agp_x[ae] != m_init) && (m_agp_y[ae] != m_init)) set_position(v, m_agp_x[ae], m_agp_y[ae]);
		OGDF_ASSERT(m_agp_x[ae] != m_init);
		OGDF_ASSERT(m_agp_y[ae] != m_init);
		//bends
		if (abendType(ae) != BendType::BendFree)
		{
			edge newe;
			node newbend, newglue;
			int ypsiqueen;
			switch (abendType(ae))
			{
				case BendType::Bend1Left:
					//node right side
					//set new corner before first rerouting
					if (!acorn)
					{
						acorn = true;
						edge newe2;
						adjEntry ae2 = saveadj->cyclicSucc(); //next to the right in cage
						edge savedge = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[3] = newe2->adjSource();
						}
						else {
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[3] = savedge->adjTarget();
						}
						fix_position(newe2->source(), infos[l_v].coord(OrthoDir::South), infos[l_v].coord(OrthoDir::West));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}
					//delete corner after last rerouting
					if (acorn && (ipos == infos[l_v].inList(OrthoDir::West).size()-1))
					{
						adjEntry ae2 = saveadj->cyclicPred(); //ae->cyclicPred(); //aussen an cage
						adjEntry ae3 = ae2->faceCycleSucc();
						if (ae2 == (ae2->theEdge()->adjSource()))
							unsplit(ae2->theEdge(), ae3->theEdge());
						else
							unsplit(ae3->theEdge(), ae2->theEdge());
					}
					//
					if (inedge) newe = addLeftBend(e);
					else newe = addRightBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();
					ypsiqueen = infos[l_v].coord(OrthoDir::West)
										 + (infos[l_v].flips(OrthoDir::West, OrthoDir::South) + ipos - infos[l_v].inList(OrthoDir::West).size())
										 *infos[l_v].delta(OrthoDir::South, OrthoDir::West)
										 +infos[l_v].eps(OrthoDir::South, OrthoDir::West);
					fix_position(newbend, cp_x(ae), ypsiqueen);
					fix_position(newglue, infos[l_v].coord(OrthoDir::South), ypsiqueen);
					break; //rerouted single bend
				case BendType::ProbB1L:
				case BendType::ProbB2L:
				case BendType::Bend2Left:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae]-leftofs, m_agp_y[ae]);
					ypsiqueen = infos[l_v].cageCoord(OrthoDir::West)
						+ (ipos + 1 + infos[l_v].num_bend_edges(OrthoDir::West, OrthoDir::South)
						- infos[l_v].inList(OrthoDir::West).size())*m_sep;
					//coord - (-infos[l_v].flips(OrthoDir::West, OrthoDir::South) - ipos  + infos[l_v].inList(OrthoDir::West).size())*m_sep;
					newe = addLeftBend(e);
					fix_position(newe->source(), inedge ? cp_x(ae) : m_agp_x[ae] - leftofs, ypsiqueen);
					newe = addRightBend(newe);
					fix_position(newe->source(), inedge ? m_agp_x[ae] - leftofs : cp_x(ae), ypsiqueen);
					break;//double bend downwards
				//case ProbB1R:
				case BendType::Bend1Right:
					//set new corner before first rerouting

					//delete corner after last rerouting
					if (ipos == 0)//(corn && (ipos == infos[l_v].inList(OrthoDir::West).size()-1))
					{
						adjEntry ae2 = saveadj->cyclicSucc();
						adjEntry ae3 = ae2->faceCycleSucc();
						//as long as unsplit does not work, transition
						//node oldcorner;
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							//oldcorner = ae2->theEdge()->target();
							unsplit(ae2->theEdge(), ae3->theEdge());
						}
						else  {
							unsplit(ae3->theEdge(), ae2->theEdge());
							//oldcorner = ae2->theEdge()->source();
						}
					}
					if ((!corn) && (ipos == infos[l_v].flips(OrthoDir::West, OrthoDir::North) - 1))
					{
						corn = true;
						edge newe2;
						adjEntry ae2 = saveadj->cyclicPred();//ae->cyclicPred();
						edge savedge = ae2->theEdge();
						if (ae2 == (ae2->theEdge()->adjSource()))
						{
							newe2 = addLeftBend(ae2->theEdge());
							vinfo->m_corner[0] = savedge->adjTarget();
						}
						else {
							newe2 = addRightBend(ae2->theEdge());
							vinfo->m_corner[0] = newe2->adjSource();
						}
						fix_position(newe2->source(), infos[l_v].coord(OrthoDir::North), infos[l_v].coord(OrthoDir::West));
						m_prup->setExpandedNode(newe2->source(), l_v);
					}
					if (inedge) newe = addRightBend(e);
					else newe = addLeftBend(e);
					newbend = newe->source();

					if (inedge) newglue = newe->target();
					else newglue = e->source();
					ypsiqueen = infos[l_v].coord(OrthoDir::West) + (infos[l_v].flips(OrthoDir::West, OrthoDir::North) - ipos - 1)
						* infos[l_v].delta(OrthoDir::North, OrthoDir::West) + infos[l_v].eps(OrthoDir::North, OrthoDir::West);
					fix_position(newbend, cp_x(ae), ypsiqueen);
					fix_position(newglue, infos[l_v].coord(OrthoDir::North), ypsiqueen);
					break;//rerouted single bend
				case BendType::ProbB1R:
				case BendType::ProbB2R:
				case BendType::Bend2Right:
					//adjust glue point to rerouted edges
					fix_position(v, m_agp_x[ae]+rightofs, m_agp_y[ae]);
					ypsiqueen = infos[l_v].cageCoord(OrthoDir::West)
						 + (infos[l_v].num_bend_edges(OrthoDir::West, OrthoDir::North) - ipos)*m_sep;
					//infos[l_v].coord(OrthoDir::West) - ((ipos + 1) - infos[l_v].flips(OrthoDir::North, OrthoDir::West)) * m_sep;
					newe = addRightBend(e);
					fix_position(newe->source(), inedge ? cp_x(ae) : m_agp_x[ae] + rightofs, ypsiqueen);
					newe = addLeftBend(newe);

					fix_position(newe->source(), inedge ? m_agp_x[ae] + rightofs : cp_x(ae), ypsiqueen);
					break; //double bend upwards
				default: break;
			}
			m_orp->normalize();
		}

		++ipos;
		++it;
	}

	//OGDF_ASSERT(m_orp->check(msg));
}

//given a replacement cage (defining routing channels??)
//and a box placement, compute a bend-minimising routing
//box placement is in m_newx[v], m_newy[v] for left lower corner
//now it is in NodeInfo, m_new may be obs
// XXX: check rounding!
void EdgeRouter::compute_routing(node v) //reroute(face f)
{
	//compute for each box corner the possible reroutings

	//first, define some values
	//find the uppermost/lowest unbend edge on the left and right side
	//and the rightmost/leftmost unbend edge on the top and bottom side
	//use values stored in inf

	//alpha used in move - functions, variable al_12 means:
	//max. number of edges to be moved from side 2 to side 1
	//eight cases, two for ev corner
	//1space left for edges moved from top to left side
	int al_lt = alpha_move(OrthoDir::North, OrthoDir::East, v);
	//2space left for edges moved from left to top side
	int al_tl = alpha_move(OrthoDir::East, OrthoDir::North, v);
	//3space left for edges moved from left to bottom side
	//int al_bl = alpha_move(OrthoDir::West, OrthoDir::North, v);
	//4Space left for edges moved from bottom side to left side
	//int al_lb = alpha_move(OrthoDir::North, OrthoDir::West, v);
	//5space left for edges moved from right to top side
	//int al_tr = alpha_move(OrthoDir::East, OrthoDir::South, v);
	//6space left for edges moved from top to right side
	int al_rt = alpha_move(OrthoDir::South, OrthoDir::East, v);
	//7space left for edges moved from right to bottom side
	int al_br = alpha_move(OrthoDir::West, OrthoDir::South, v);
	//8space left for edges moved from bottom to right side
	//int al_rb = alpha_move(OrthoDir::South, OrthoDir::West, v);

	//beta used in move functions
	//be_12k defines the number of edges with preliminary bends
	//but connection point in the glue point area, so that
	//they could be routed bendfree if k (enough) edges are moved from
	//side 1 to side 2
	//k is computed as the minimum of al_21 and card(E^12) (rerouteable edges)
	//beta is only used internally to compute bend save
	//save_12 holds the number of saved bends by moving edges from 1 to 2

	//flip_12 contains the number of edges flipped from
	//side 1 to 2 and flip_21 ~
	//from side 2 to 1 (only one case is possible)

	//algorithm 3 in the paper

	//This means that for two sides with equal value, our algorithm always
	//prefers one (the same) above the other

	//top-left
	int flip_tl, flip_lt;
	if (compute_move(OrthoDir::East, OrthoDir::North, flip_tl, v) < compute_move(OrthoDir::North, OrthoDir::East, flip_lt, v))
		flip_tl = 0;
	else flip_lt = 0;
	//left-bottom
	int flip_lb, flip_bl;
	if (compute_move(OrthoDir::North, OrthoDir::West, flip_lb, v) < compute_move(OrthoDir::West, OrthoDir::North, flip_bl, v))
		flip_lb = 0;
	else flip_bl = 0;
	//top-right
	int flip_tr, flip_rt;
	if (compute_move(OrthoDir::East, OrthoDir::South, flip_tr, v) < compute_move(OrthoDir::South, OrthoDir::East, flip_rt, v))
		flip_tr = 0;
	else flip_rt = 0;
	//bottom-right
	int flip_br, flip_rb;
	if (compute_move(OrthoDir::West, OrthoDir::South, flip_br, v) < compute_move(OrthoDir::South, OrthoDir::West, flip_rb, v))
		flip_br = 0;
	else flip_rb = 0;
	//std::cout << "resulting flip numbers: " << flip_tl << "/" << flip_tr << "/" << flip_bl << "/" << flip_br << "/"
	//          << flip_rt << "/" << flip_rb << "/" << flip_lt << "/" << flip_lb << "\n";
	//if there are no bendfree edges on side s, we have
	//to assure that the edges moved in from both sides
	//dont take up too much space, simple approach: decrease
	{
		int surplus; //flippable edges without enough space to place
		if (infos[v].num_bend_free(OrthoDir::East) == 0)//top, contributing neigbours are left/right
		{
			surplus = flip_lt + flip_rt - al_tl;
			if (surplus > 0)
			{
				flip_lt -= int(floor(surplus/2.0));
				flip_rt -= int(ceil(surplus/2.0));
			}
		}
		if (infos[v].num_bend_free(OrthoDir::West) == 0)//bottom
		{
			surplus = flip_rb + flip_lb - al_br;
			if (surplus > 0)
			{
				flip_lb -= int(floor(surplus/2.0));
				flip_rb -= int(ceil(surplus/2.0));
			}
		}
		if (infos[v].num_bend_free(OrthoDir::South) == 0)
		{
			surplus = flip_br + flip_tr - al_rt;
			if (surplus > 0)
			{
				flip_br -= int(floor(surplus/2.0));
				flip_tr -= int(ceil(surplus/2.0));
			}
		}
		if (infos[v].num_bend_free(OrthoDir::North) == 0)
		{
			surplus = flip_tl + flip_bl - al_lt;
			if (surplus > 0)
			{
				flip_tl -= int(floor(surplus/2.0));
				flip_bl -= int(ceil(surplus/2.0));
			}
		}
	}
	//std::cout << "resulting flip numbers: " << flip_tl << "/" << flip_tr << "/" << flip_bl << "/" << flip_br << "/"
	//          << flip_rt << "/" << flip_rb << "/" << flip_lt << "/" << flip_lb << "\n";
	//now we have the exact number of edges to be rerouted at every corner
	//we change the bendtype for rerouted edges and change their glue point
	//correction: we set the glue point at bend introduction

	int flipedges;

	OrthoDir od = OrthoDir::North;
#if 0
	// TO BE REMOVED IF OBSOLETE
	do
	{
		l_it = infos[v].inList(od).begin();
		while (l_it.valid())
		{
			++l_it; //TODO: what happens here?07.2004
		}
		od =OrthoRep::nextDir(od);
	} while (od != OrthoDir::North);
#endif
	//start flipping
	for (flipedges = 0; flipedges < flip_lt; flipedges++)
	{
		m_abends[outEntry(infos[v], OrthoDir::North, infos[v].inList(OrthoDir::North).size() - 1 - flipedges)] = BendType::Bend1Right;
		infos[v].flips(OrthoDir::North, OrthoDir::East)++;
	}

	int newbendfree;
	int newbf;
	if (flip_lt) //we flipped and may save bends, if may be not necessary cause fliplt parameter in betamove
	{
		newbendfree = beta_move(OrthoDir::North, OrthoDir::East, flip_lt, v);
		for (newbf = 0; newbf < newbendfree; newbf++)
		{
			m_abends[outEntry(infos[v], OrthoDir::North, infos[v].inList(OrthoDir::North).size() - 1 - flip_lt - newbf)] = BendType::BendFree;
			m_agp_y[outEntry(infos[v], OrthoDir::North, infos[v].inList(OrthoDir::North).size() - 1 - flip_lt - newbf)] =
				cp_y(outEntry(infos[v], OrthoDir::North, infos[v].inList(OrthoDir::North).size() - 1 - flip_lt - newbf));
		}
	}

	for (flipedges = 0; flipedges < flip_lb; flipedges++)
	{
		infos[v].flips(OrthoDir::North, OrthoDir::West)++;
		m_abends[outEntry(infos[v], OrthoDir::North, flipedges)] = BendType::Bend1Left;
	}

	newbendfree = beta_move(OrthoDir::North, OrthoDir::West, flip_lb, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], OrthoDir::North, flip_lb + newbf)] = BendType::BendFree;
		m_agp_y[outEntry(infos[v], OrthoDir::North, flip_lb + newbf)] =
			cp_y(outEntry(infos[v], OrthoDir::North, flip_lb + newbf));
	}

	for (flipedges = 0; flipedges < flip_rt; flipedges++)
	{
		infos[v].flips(OrthoDir::South, OrthoDir::East)++;
		m_abends[outEntry(infos[v], OrthoDir::South, infos[v].inList(OrthoDir::South).size() - 1 - flipedges)] = BendType::Bend1Left;
	}

	newbendfree = beta_move(OrthoDir::South, OrthoDir::East, flip_rt, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], OrthoDir::South, infos[v].inList(OrthoDir::South).size() - 1 - flip_rt - newbf)] = BendType::BendFree;
		m_agp_y[outEntry(infos[v], OrthoDir::South, infos[v].inList(OrthoDir::South).size() - 1 - flip_rt - newbf)] =
			cp_y(outEntry(infos[v], OrthoDir::South, infos[v].inList(OrthoDir::South).size() - 1 - flip_rt - newbf));
	}

	for (flipedges = 0; flipedges < flip_rb; flipedges++)
	{
		m_abends[outEntry(infos[v], OrthoDir::South, flipedges)] = BendType::Bend1Right;
		infos[v].flips(OrthoDir::South, OrthoDir::West)++;
	}

	newbendfree = beta_move(OrthoDir::South, OrthoDir::West, flip_rb, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], OrthoDir::South, flip_rb + newbf)] = BendType::BendFree;
		m_agp_y[outEntry(infos[v], OrthoDir::South, flip_rb + newbf)] =
			cp_y(outEntry(infos[v], OrthoDir::South, flip_rb + newbf));
	}

	//only one of the quadruples will we executed
	for (flipedges = 0; flipedges < flip_tl; flipedges++)
	{
		m_abends[outEntry(infos[v], OrthoDir::East, flipedges)] = BendType::Bend1Left;
		infos[v].flips(OrthoDir::East, OrthoDir::North)++;
	}

	newbendfree = beta_move(OrthoDir::East, OrthoDir::North, flip_tl, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], OrthoDir::East, flip_tl + newbf)] = BendType::BendFree;
		m_agp_x[outEntry(infos[v], OrthoDir::East, flip_tl + newbf)] =
			cp_x(outEntry(infos[v], OrthoDir::East, flip_tl + newbf));
	}

	for (flipedges = 0; flipedges < flip_bl; flipedges++)
	{
		m_abends[outEntry(infos[v], OrthoDir::West, flipedges)] = BendType::Bend1Right;
		infos[v].flips(OrthoDir::West, OrthoDir::North)++;
	}

	newbendfree = beta_move(OrthoDir::West, OrthoDir::North, flip_bl, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], OrthoDir::West, flip_bl + newbf)] = BendType::BendFree;
		m_agp_x[outEntry(infos[v], OrthoDir::West, flip_bl + newbf)] =
		cp_x(outEntry(infos[v], OrthoDir::West, flip_bl + newbf));
	}

	ListReverseIterator<edge> l_it = infos[v].inList(OrthoDir::East).rbegin();
	for (flipedges = 0; flipedges < flip_tr; flipedges++)
	{
		if (l_it.valid()) //temporary check
		{
			m_abends[outEntry(infos[v], OrthoDir::East, infos[v].inList(OrthoDir::East).size() - 1 - flipedges)] = BendType::Bend1Right;
			infos[v].flips(OrthoDir::East, OrthoDir::South)++;
			++l_it;
		}
	}

	newbendfree = beta_move(OrthoDir::East, OrthoDir::South, flip_tr, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], OrthoDir::East, infos[v].inList(OrthoDir::East).size() - 1 - flip_tr - newbf)] = BendType::BendFree;
		m_agp_x[outEntry(infos[v], OrthoDir::East, infos[v].inList(OrthoDir::East).size() - 1 - flip_tr - newbf)] =
			cp_x(outEntry(infos[v], OrthoDir::East, infos[v].inList(OrthoDir::East).size() - 1 - flip_tr - newbf));
	}

	for (flipedges = 0; flipedges < flip_br; flipedges++)
	{
		m_abends[outEntry(infos[v], OrthoDir::West, infos[v].inList(OrthoDir::West).size() - 1 - flipedges)] = BendType::Bend1Left;
		infos[v].flips(OrthoDir::West, OrthoDir::South)++;
	}

	newbendfree = beta_move(OrthoDir::West, OrthoDir::South, flip_br, v);
	for (newbf = 0; newbf < newbendfree; newbf++)
	{
		m_abends[outEntry(infos[v], OrthoDir::West, infos[v].inList(OrthoDir::West).size() - 1 - flip_br - newbf)] = BendType::BendFree;
		m_agp_x[outEntry(infos[v], OrthoDir::West, infos[v].inList(OrthoDir::West).size() - 1 - flip_br - newbf)] =
			cp_x(outEntry(infos[v], OrthoDir::West, infos[v].inList(OrthoDir::West).size() - 1 - flip_br - newbf));
	}

	//rest loeschen ?????
	od = OrthoDir::North;
	do
	{
		od =OrthoRep::nextDir(od);
	} while (od != OrthoDir::North);
}

void EdgeRouter::initialize_node_info(node v, int sep)
{
	//derive simple data
	const OrthoRep::VertexInfoUML* vinfo = m_orp->cageInfo(v);
#if 0
	inf.get_data(*m_prup, *m_orp, v);
#endif
	//zuerst die kantenlisten und generalizations finden und pos festlegen
	{//construct the edge lists for the incoming edges on ev side
		// side information (OrthoDir::North, OrthoDir::East, OrthoDir::South, OrthoDir::West corresponds to left, top, right, bottom)
#if 0
		SideInfoUML m_side[4];
#endif
		// m_corner[dir] is adjacency entry in direction dir starting at
		// a corner
#if 0
		adjEntry m_corner[4];
#endif
		infos[v].firstAdj() = nullptr;

		adjEntry adj = m_prup->expandAdj(v);

		if (adj != nullptr)
		{
			//preliminary: reset PlanRep expandedNode values if necessary
			adjEntry adjRun = adj;

			do
			{
				if (!m_prup->expandedNode(adjRun->theNode()))
					m_prup->setExpandedNode(adjRun->theNode(), v);
				adjRun = adjRun->faceCycleSucc();
			} while (adjRun != adj);

			OGDF_ASSERT(m_prup->typeOf(v) != Graph::NodeType::generalizationMerger);
			OrthoDir od = OrthoDir::North; //start with edges to the left
			do {
				adjEntry sadj = vinfo->m_corner[static_cast<int>(od)];
				adjEntry adjSucc = sadj->faceCycleSucc();

				List<edge>& inedges = infos[v].inList(od);
				List<bool>& inpoint = infos[v].inPoint(od);

				//parse the side and insert incoming edges
				while (m_orp->direction(sadj) == m_orp->direction(adjSucc)) //edges may never be attached at corners
				{
					adjEntry in_edge_adj = adjSucc->cyclicPred();
					edge in_edge = in_edge_adj->theEdge();
					//clockwise cyclic search  ERROR: in/out edges: I always use target later for cage nodes!!
					bool is_in = (in_edge->adjTarget() == in_edge_adj);
					if (infos[v].firstAdj() == nullptr) infos[v].firstAdj() = in_edge_adj;

					OGDF_ASSERT(m_orp->direction(in_edge_adj) == OrthoRep::nextDir(od) ||
						m_orp->direction(in_edge_adj) == OrthoRep::prevDir(od));
					if ((od == OrthoDir::North) || (od == OrthoDir::East)) //if left or top
					{
						inedges.pushBack(in_edge);
						inpoint.pushBack(is_in);
					}
					else
					{
						inedges.pushFront(in_edge);
						inpoint.pushFront(is_in);
					}
					//setting connection point coordinates
					if (is_in)
					{
						m_acp_x[in_edge_adj] = m_layoutp->x(in_edge->target());
						m_acp_y[in_edge_adj] = m_layoutp->y(in_edge->target());
						m_cage_point[in_edge_adj] = in_edge->target();
						//align test
						if (m_prup->typeOf(in_edge->source()) == Graph::NodeType::generalizationExpander)
						{
							if (m_align) m_mergerSon[v] = true;
							m_mergeDir[v] = OrthoRep::oppDir(m_orp->direction(in_edge->adjSource()));
						}
					} else {
						m_acp_x[in_edge_adj] = m_layoutp->x(in_edge->source());
						m_acp_y[in_edge_adj] = m_layoutp->y(in_edge->source());
						m_cage_point[in_edge_adj] = in_edge->source();
						//align test
						if (m_prup->typeOf(in_edge->target()) == Graph::NodeType::generalizationExpander)
						{
							if (m_align) m_mergerSon[v] = true;
							m_mergeDir[v] = m_orp->direction(in_edge->adjSource());
						}
					}
					sadj = adjSucc;
					adjSucc = sadj->faceCycleSucc();
				}
				od =  OrthoRep::nextDir(od);
			} while (od != OrthoDir::North);

			infos[v].get_data(*m_orp, *m_layoutp, v, *m_rc, *m_nodewidth, *m_nodeheight);
		}
	}

	//derive the maximum separation between edges on the node sides
	//left side
	int dval, dsep;

	if (infos[v].has_gen(OrthoDir::North))
	{
		int le = vinfo->m_side[0].m_nAttached[0]; //to bottom side
		int re = vinfo->m_side[0].m_nAttached[1]; //to top
		dsep = (le + Cconst == 0) ? sep : int(floor(infos[v].node_ysize() / (2*(le + Cconst))));
		dval = min(dsep,sep);
		OGDF_ASSERT( dval > 0);
		infos[v].set_delta(OrthoDir(0),OrthoDir(3),dval); infos[v].set_eps(OrthoDir(0),OrthoDir(3),int(floor(Cconst*dval)));
		//top side
		dsep = (re + Cconst == 0) ? sep : int(floor(infos[v].node_ysize() / (2*(re + Cconst))));
		dval = min(dsep, sep);
		OGDF_ASSERT( dval > 0);
		infos[v].set_delta(OrthoDir(0),OrthoDir(1),dval); infos[v].set_eps(OrthoDir(0),OrthoDir(1),int(floor(Cconst*dval)));
	} else {
		int ae = vinfo->m_side[0].m_nAttached[0];
		if (ae > 0)
			dsep = (ae+Cconst == 1) ? sep : int(floor(infos[v].node_ysize() / (ae - 1 + 2*Cconst))); // may be < 0
		else dsep = sep;
		dval = min(dsep, sep);
		OGDF_ASSERT( dval > 0);
		if (dval >= infos[v].node_ysize()) dval = int(floor((double)(infos[v].node_ysize()) / 2)); //allow 1 flip
		OGDF_ASSERT( dval < infos[v].node_ysize() );
		infos[v].set_delta(OrthoDir(0),OrthoDir(1),dval); infos[v].set_eps(OrthoDir(0),OrthoDir(1),int(floor(Cconst*dval)));
		infos[v].set_delta(OrthoDir(0),OrthoDir(3),dval); infos[v].set_eps(OrthoDir(0),OrthoDir(3),int(floor(Cconst*dval)));
	}
	if (infos[v].has_gen(OrthoDir::East))
	{
		int le = vinfo->m_side[1].m_nAttached[0]; //to left side
		int re = vinfo->m_side[1].m_nAttached[1]; //to right
		dsep = (le + Cconst == 0) ? sep : int(floor(infos[v].node_xsize() / (2*(le + Cconst))));
		dval = min(dsep,sep);
		OGDF_ASSERT(dval > 0);
		infos[v].set_delta(OrthoDir::East,OrthoDir::North,dval); infos[v].set_eps(OrthoDir::East,OrthoDir::North,int(floor(Cconst*dval)));
		//top side
		dsep = (re + Cconst == 0) ? sep : int(floor((double)(infos[v].node_xsize()) / (2*(re + Cconst))));
		dval = min(dsep, sep);
		OGDF_ASSERT( dval > 0);
		infos[v].set_delta(OrthoDir::East,OrthoDir::South,dval); infos[v].set_eps(OrthoDir::East,OrthoDir::South,int(floor(Cconst*dval)));
	} else {
		int ae = vinfo->m_side[1].m_nAttached[0];
		if (ae > 0)
			dsep = (ae+Cconst == 1) ? sep : int(floor((double)(infos[v].node_xsize()) / (ae - 1 + 2*Cconst))); // may be <= 0
		else dsep = sep < int(floor((double)(infos[v].node_xsize())/2)) ? sep : int(floor((double)(infos[v].node_xsize())/2));
		dval = min(dsep, sep);
		OGDF_ASSERT( dval > 0);

		infos[v].set_delta(OrthoDir::East,OrthoDir::North,dval); infos[v].set_eps(OrthoDir::East,OrthoDir::North,int(floor(Cconst*dval)));
		infos[v].set_delta(OrthoDir::East,OrthoDir::South,dval); infos[v].set_eps(OrthoDir::East,OrthoDir::South,int(floor(Cconst*dval)));
	}
	if (infos[v].has_gen(OrthoDir::South))
	{
		int le = vinfo->m_side[2].m_nAttached[0]; //to top
		int re = vinfo->m_side[2].m_nAttached[1]; //to bottom
		dsep = (le + Cconst == 0) ? sep : int(floor((double)(infos[v].node_ysize()) / (2*(le + Cconst))));
		dval = min(dsep,sep);
		OGDF_ASSERT( dval > 0);
		infos[v].set_delta(OrthoDir::South,OrthoDir::East,dval); infos[v].set_eps(OrthoDir::South,OrthoDir::East,int(floor(Cconst*dval)));
		//top side
		dsep = (re + Cconst == 0) ? sep : int(floor((double)(infos[v].node_ysize()) / (2*(re + Cconst))));
		dval = min(dsep, sep);
		infos[v].set_delta(OrthoDir::South,OrthoDir::West,dval); infos[v].set_eps(OrthoDir::South,OrthoDir::West,int(floor(Cconst*dval)));
	} else {
		int ae = vinfo->m_side[2].m_nAttached[0];
		if (ae > 0)
			dsep = (ae + Cconst == 1) ? sep : int(floor(infos[v].node_ysize() / (ae - 1 + 2*Cconst))); // may be <= 0
		else dsep = sep;
		dval = min(dsep, sep);
		infos[v].set_delta(OrthoDir::South,OrthoDir::East,dval); infos[v].set_eps(OrthoDir::South,OrthoDir::East,int(floor(Cconst*dval)));
		infos[v].set_delta(OrthoDir::South,OrthoDir::West,dval); infos[v].set_eps(OrthoDir::South,OrthoDir::West,int(floor(Cconst*dval)));
	}
	if (infos[v].has_gen(OrthoDir::West))
	{
		int le = vinfo->m_side[3].m_nAttached[0]; //to left side
		int re = vinfo->m_side[3].m_nAttached[1]; //to right
		dsep = (le + Cconst == 0) ? sep : int(floor((double)(infos[v].node_xsize()) / (2*(le + Cconst))));
		dval = min(dsep,sep);
		OGDF_ASSERT( dval > 0);
		infos[v].set_delta(OrthoDir::West,OrthoDir::South,dval); infos[v].set_eps(OrthoDir::West,OrthoDir::South,int(floor(Cconst*dval)));
		//top side
		dsep = (re + Cconst == 0) ? sep : int(floor((double)(infos[v].node_xsize()) / (2*(re + Cconst))));
		dval = min(dsep, sep);
		infos[v].set_delta(OrthoDir::West,OrthoDir::North,dval); infos[v].set_eps(OrthoDir::West,OrthoDir::North,int(floor(Cconst*dval)));
	} else {
		int ae = vinfo->m_side[3].m_nAttached[0];
		if (ae > 0)
			dsep = (ae + Cconst == 1) ? sep : int(floor((double)(infos[v].node_xsize()) / (ae - 1 + 2*Cconst))); // may be <= 0
		else dsep = sep;
		dval = min(dsep, sep);
		OGDF_ASSERT( dval > 0);
		infos[v].set_delta(OrthoDir::West,OrthoDir::South,dval); infos[v].set_eps(OrthoDir::West,OrthoDir::South,int(floor(Cconst*dval)));
		infos[v].set_delta(OrthoDir::West,OrthoDir::North,dval); infos[v].set_eps(OrthoDir::West,OrthoDir::North,int(floor(Cconst*dval)));
	}
	//{cagesize, boxsize, delta, epsilon, gen_pos  ...}
}

//COMPUTING THE CONSTANTS
//maybe faster: alpha as parameter, recompute if -1, use otherwise
//paper algorithm 2
int EdgeRouter::compute_move(OrthoDir s_from, OrthoDir s_to, int& kflip, node v)
//compute the maximal number of moveable edges from s_from to s_to
//(and thereby the moveable edges, counted from the box corner)
//and return the number of saved bends
{
	//debug: first, we compute (min(al_21, E12)
	kflip = min( alpha_move(s_to, s_from, v), infos[v].num_routable(s_from, s_to) );
	OGDF_ASSERT(kflip > -1);

	return kflip + 2*beta_move(s_from, s_to, kflip, v);
}

//helper functions computing the intermediate values
//number of edges that can additionally be routed bendfree at s_from if move_num edges are
//moved from s_from to s_to
int EdgeRouter::beta_move(OrthoDir s_from, OrthoDir s_to, int move_num, node v)
{
	//check the edges in E if their connection point
	//could be routed bendfree to an glue point if move_num edges are flipped to s_to
	if (move_num < 1) return 0;
	int ic = 0;
	bool down = (s_to == OrthoDir::North) || (s_to == OrthoDir::West);

	//try to find out which bend direction is opposite
	//these edges can not be routed bendfree
	BendType bt1, bt2, bt3, bt4;
	if ((s_from == OrthoDir::East && s_to == OrthoDir::South)
	 || (s_from == OrthoDir::North && s_to == OrthoDir::East)
	 || (s_from == OrthoDir::West && s_to == OrthoDir::North)
	 || (s_from == OrthoDir::South && s_to == OrthoDir::West)) {
		bt1 = BendType::ProbB1L;
		bt2 = BendType::ProbB2L;
		bt3 = BendType::Bend1Left;
		bt4 = BendType::Bend2Left;
	} else {
		OGDF_ASSERT((s_from == OrthoDir::East && s_to == OrthoDir::North)
		         || (s_from == OrthoDir::North && s_to == OrthoDir::West)
		         || (s_from == OrthoDir::West && s_to == OrthoDir::South)
		         || (s_from == OrthoDir::South && s_to == OrthoDir::East));
		bt1 = BendType::ProbB1R;
		bt2 = BendType::ProbB2R;
		bt3 = BendType::Bend1Right;
		bt4 = BendType::Bend2Right;
	}


	{//debug top side to
		//first list all bend edges at corner s_from->s_to by increasing distance to corner in list E
		ListIterator<edge> ep;
		adjEntry ae; //used to find real position
		int adjcount;

		if (down)
		{
			ep = infos[v].inList(s_from).begin(); //first entry iterator
			if (ep.valid()) ae = outEntry(infos[v], s_from, 0);
			adjcount = 0;
		}
		else
		{
			adjcount = infos[v].inList(s_from).size()-1;
			ep = infos[v].inList(s_from).rbegin(); //last entry iterator
			if (ep.valid()) ae = outEntry(infos[v], s_from, adjcount);
		}
		ic = 0;
		while (ep.valid() && (ic < move_num))
		{
			++ic;
			if (down) { ++ep; ++adjcount; }
			else { --ep; --adjcount; }
		}

		if (ep.valid()) ae = outEntry(infos[v], s_from, adjcount);

		//now ep should point to first usable edge, if list was not empty
		if (!ep.valid()) return 0;

		//if this edge is already unbend, there is nothing to save
		if ((m_abends[ae] == BendType::BendFree) ||
			(m_abends[ae] == bt1) ||
			(m_abends[ae] == bt2) ||
			(m_abends[ae] == bt3) ||
			(m_abends[ae] == bt4) )
			return 0;

		bool bend_saveable;
		bool in_E_sfrom_sto; //models set E_s_from_s_to from paper

		ic = 0; //will hold the number of new unbend edges

		//four cases:
		switch (s_to)
		{
		case OrthoDir::East: //from left and right
			bend_saveable = (cp_y(ae) <= (infos[v].coord(s_to)
				- infos[v].delta(s_from, s_to)*ic
				- infos[v].eps(s_from, s_to)));
			in_E_sfrom_sto =  (cp_y(ae) > gp_y(ae));
			break;
		case OrthoDir::North:  //from top and bottom
			bend_saveable = (cp_x(ae) >= (infos[v].coord(s_to)
				+ infos[v].delta(s_from, s_to)*ic
				+ infos[v].eps(s_from, s_to)));
			in_E_sfrom_sto =  (cp_x(ae) < gp_x(ae));
			break;
		case OrthoDir::South:
			bend_saveable = (cp_x(ae) <= (infos[v].coord(s_to)
				- infos[v].delta(s_from, s_to)*ic
				- infos[v].eps(s_from, s_to)));
			in_E_sfrom_sto =  (cp_x(ae) > gp_x(ae));
			break;
		default:
			OGDF_ASSERT(s_to == OrthoDir::West);
			bend_saveable = (cp_y(ae) >= (infos[v].coord(s_to)
				+ infos[v].delta(s_from, s_to)*ic
				+ infos[v].eps(s_from, s_to)));
			in_E_sfrom_sto =  (cp_y(ae) < gp_y(ae));
		}

		// compare edges connection point with available space
		// while valid, connection point ok and edge was preliminarily bend (far enough from corner)...
		while (ep.valid()
		    && bend_saveable
		    && in_E_sfrom_sto // models E_from_to set in paper
		    && (down ? adjcount < infos[v].inList(s_from).size() - 1
		             : adjcount > 0)) {
			if (down)
			{
				++ep;
				ae = outEntry(infos[v], s_from, ++adjcount);
			}
			else {
				--ep;
				ae = outEntry(infos[v], s_from, --adjcount);
			}
			++ic;

			//four cases:
			if (ep.valid())
			{

				if ((m_abends[ae] == BendType::BendFree) ||
					(m_abends[ae] == bt1) ||
					(m_abends[ae] == bt2) ||
					(m_abends[ae] == bt3) ||
					(m_abends[ae] == bt4) )
				break; //no further saving possible
				//hier noch: falls Knick in andere Richtung: auch nicht bendfree

				switch (s_to)
				{
				case OrthoDir::East: //from left and right , ersetzte cp(*ep) durch cp(ae)
					bend_saveable = (cp_y(ae) <= (infos[v].coord(OrthoDir::East)
						- infos[v].delta(s_from, s_to)*ic
						- infos[v].eps(s_from, s_to)));
					in_E_sfrom_sto =  (cp_y(ae) > gp_y(ae));
					break;
				case OrthoDir::North:  //from top and bottom
					bend_saveable = (cp_x(ae) >= (infos[v].coord(OrthoDir::North)
						+ infos[v].delta(s_from, s_to)*ic
						+ infos[v].eps(s_from, s_to)));
					in_E_sfrom_sto =  (cp_x(ae) < gp_x(ae));
					break;
				case OrthoDir::South:
					bend_saveable = (cp_x(ae) <= (infos[v].coord(OrthoDir::South)
						- infos[v].delta(s_from, s_to)*ic
						- infos[v].eps(s_from, s_to)));
					in_E_sfrom_sto =  (cp_x(ae) > gp_x(ae));
					break;
				case OrthoDir::West:
					bend_saveable = (cp_y(ae) >= (infos[v].coord(OrthoDir::West)
						+ infos[v].delta(s_from, s_to)*ic
						+ infos[v].eps(s_from, s_to)));
					in_E_sfrom_sto =  (cp_y(ae) < gp_y(ae));
					break;
				default:
					OGDF_ASSERT(false);
				}
			}
		}
	}
	return ic;
}

//compute the maximum number of edges to be moved from s_from to s_to
//attention: order of sides reversed: to - from
//optimisation: check for the minimum distance (separaration on "to" side,
//separation on from side) to allow flipping improvement even if the separation
//cannot be guaranted, but the "to" separation will be improved (but assure
//that the postprocessing knows the changed values
//maybe reassign all glue points at the two sides
int EdgeRouter::alpha_move(OrthoDir s_to, OrthoDir s_from, node v)
{
	//Test fuer alignment: Falls der Knoten aligned wird, Kanten nicht an Seiten verlegen
	//zunaechst: garnicht
	if (m_align && m_mergerSon[m_prup->expandedNode(v)]) {
		return 0;
	}

	double result = -1.;

	if (s_from == s_to || s_from == OrthoRep::oppDir(s_to)) {
		OGDF_THROW(AlgorithmFailureException);
	}

	if (infos[v].num_bend_free(s_to) != 0) {
		if (s_to == OrthoDir::North) {
			if (s_from == OrthoDir::East) {
				result = infos[v].coord(s_from) - infos[v].l_upper_unbend();
			} else {
				result = infos[v].l_lower_unbend() - infos[v].coord(s_from);
			}
		} else
		if (s_to == OrthoDir::South) {
			if (s_from == OrthoDir::East) {
				result = infos[v].coord(s_from) - infos[v].r_upper_unbend();
			} else {
				result = infos[v].r_lower_unbend() - infos[v].coord(s_from);
			}
		} else
		if (s_to == OrthoDir::East) {
			if (s_from == OrthoDir::North) {
				result = infos[v].t_left_unbend() - infos[v].coord(s_from);
			} else {
				result = infos[v].coord(s_from) - infos[v].t_right_unbend();
			}
		} else {
			if (s_from == OrthoDir::North) {
				result = infos[v].b_right_unbend() - infos[v].coord(s_from);
			} else {
				result = infos[v].coord(s_from) - infos[v].b_left_unbend();
			}
		}
		result -= infos[v].delta(s_to, s_from) * infos[v].num_bend_edges(s_to, s_from);
		result -= infos[v].eps(s_to, s_from);
		result /= infos[v].delta(s_to, s_from);
	} else {
		OrthoDir from = s_from;
		if (s_from == OrthoDir::East || s_from == OrthoDir::West) {
			result = infos[v].node_ysize();
		} else {
			result = infos[v].node_xsize();
		}
		if ((s_from == OrthoDir::West && s_to == OrthoDir::North)
		 || (s_from == OrthoDir::West && s_to == OrthoDir::South)
		 || (s_from == OrthoDir::South && s_to == OrthoDir::East)
		 || (s_from == OrthoDir::South && s_to == OrthoDir::West)) {
			from = OrthoRep::oppDir(from);
		}
		result -= infos[v].delta(s_to, from) * (infos[v].num_bend_edges(s_to, from) + infos[v].num_bend_edges(s_to, OrthoRep::oppDir(from)) - 1);
		result -= 2 * infos[v].eps(s_to, from);
		result /= infos[v].delta(s_to, from);
	}

	if (result < 0) {
		return 0;
	}

	return int(std::floor(result));
}


void EdgeRouter::addbends(BendString& bs, const char* s2)
{
	const char* s1 = bs.toString();
	size_t len = strlen(s1) + strlen(s2) + 1;
	char* resi = new char[len];
	bs.set(resi);
	delete[] resi;
}

//add a left bend to edge e
edge EdgeRouter::addLeftBend(edge e)
{
	int a1 = m_orp->angle(e->adjSource());
	int a2 = m_orp->angle(e->adjTarget());

	edge ePrime = m_comb->split(e);
	m_orp->angle(ePrime->adjSource()) = 3;
	m_orp->angle(ePrime->adjTarget()) = a2;
	m_orp->angle(e->adjSource()) = a1;
	m_orp->angle(e->adjTarget()) = 1;

	return ePrime;
}

//add a right bend to edge e
edge EdgeRouter::addRightBend(edge e)
{
	string msg;

	int a1 = m_orp->angle(e->adjSource());
	int a2 = m_orp->angle(e->adjTarget());

	edge ePrime = m_comb->split(e);

	m_orp->angle(ePrime->adjSource()) = 1;
	m_orp->angle(ePrime->adjTarget()) = a2;
	m_orp->angle(e->adjSource()) = a1;
	m_orp->angle(e->adjTarget()) = 3;

	return ePrime;
}


//set the computed values in the m_med structure
void EdgeRouter::setDistances()
{
	for(node v : m_prup->nodes)
	{
		if ((m_prup->expandAdj(v) != nullptr) && (m_prup->typeOf(v) != Graph::NodeType::generalizationMerger))
		{
			OrthoDir od = OrthoDir::North;
			do
			{
				m_med->delta(v, od, 0) = infos[v].delta(od, OrthoRep::prevDir(od));
				m_med->delta(v, od, 1) = infos[v].delta(od, OrthoRep::nextDir(od));
				m_med->epsilon(v, od, 0) = infos[v].eps(od, OrthoRep::prevDir(od));
				m_med->epsilon(v, od, 1) = infos[v].eps(od, OrthoRep::nextDir(od));

				od = OrthoRep::nextDir(od);
			} while (od != OrthoDir::North);
		}
	}
}

void EdgeRouter::unsplit(edge e1, edge e2)
{
	//precondition: ae1 is adjsource/sits on original edge
	int a1 = m_orp->angle(e1->adjSource()); //angle at source
	int a2 = m_orp->angle(e2->adjTarget()); //angle at target
	m_comb->unsplit(e1, e2);
	m_orp->angle(e1->adjSource()) = a1;
	m_orp->angle(e1->adjTarget()) = a2;
}

void EdgeRouter::set_position(node v, int x, int y)
{
	if (!m_fixed[v])
	{
		m_layoutp->x(v) = x;
		m_layoutp->y(v) = y;
	}
}

void EdgeRouter::fix_position(node v, int x, int y)
{
	m_layoutp->x(v) = x;
	m_layoutp->y(v) = y;
	m_fixed[v] = true;
}

void EdgeRouter::updateBends(
		const node v,
		ListIterator<edge> &it,
		int &pos,
		int &lastunbend,
		const bool updateX,
		const OrthoDir dir,
		const bool bendLeft,
		const bool bendUp,
		const bool subtract) {
	const OrthoDir dirB = bendLeft ? OrthoRep::nextDir(dir) : OrthoRep::prevDir(dir);
	const auto &acp = updateX ? m_acp_x : m_acp_y;
	auto &agp = updateX ? m_agp_x : m_agp_y;
	const int sep = bendUp ? m_sep : - m_sep;
	const BendType btSingle = bendLeft ? BendType::ProbB1L : BendType::ProbB1R;
	const BendType btDouble = bendLeft ? BendType::Bend2Left : BendType::Bend2Right;
	auto &inf = infos[v];
	const int sign = subtract ? -1 : 1;
	const int delta = inf.delta(dirB, dir);
	const int eps = inf.eps(dirB, dir);
	const int coord = inf.coord(dir);

	// TODO: Do we need additional conditions if bendUp?
	while (it.valid() && (bendUp || coord > acp[outEntry(inf, dirB, pos)] - pos*delta - eps)) {
		const adjEntry adj = outEntry(inf, dirB, pos);

		updateOneBend((acp[adj] > coord + sep) != bendUp, adj, v, dir, bendLeft, btSingle, btDouble);

		// leave space to re-route
		if(lastunbend != m_init) {
			lastunbend += delta;
			agp[adj] = lastunbend;
		} else {
			int factor = bendUp ? inf.inList(dirB).size() - 1 - pos : pos;
			agp[adj] = coord + sign*(eps + factor*delta);
		}

		++it;
		++pos;
	}
}

int EdgeRouter::updateBends(
		const node v,
		ListIterator<edge> &it,
		const bool updateX,
		const OrthoDir dir,
		const bool bendLeft,
		const bool bendUp,
		int pos) {
	int lastunbend = m_init;
	updateBends(v, it, pos, lastunbend, updateX, dir, bendLeft, bendUp, false);
	return pos;
}

void EdgeRouter::updateLowerEdgesBends(
		const node v,
		ListIterator<edge> &it,
		int &pos,
		int &base,
		const bool updateX,
		const OrthoDir dir,
		const bool bendLeft) {
	const OrthoDir dirB = bendLeft ? OrthoRep::nextDir(dir) : OrthoRep::prevDir(dir);
	auto &inf = infos[v];
	const auto &acp = updateX ? m_acp_x : m_acp_y;

	auto &agp = updateX ? m_agp_x : m_agp_y;
	const BendType btSingle = bendLeft ? BendType::ProbB1L : BendType::ProbB1R;
	const BendType btDouble = bendLeft ? BendType::ProbB2L : BendType::ProbB2R;

	while (it.valid()) // still some lower edges to bend
	{
		const adjEntry adj = outEntry(inf, dirB, pos);
		agp[adj] = base;

		// first parameter: paper E^
		updateOneBend(acp[adj] >= infos[v].coord(dir) - m_sep, adj, v, dir, bendLeft, btSingle, btDouble);

		base -= inf.delta(dirB, dir);
		--it;
		--pos;
	}
}

}
