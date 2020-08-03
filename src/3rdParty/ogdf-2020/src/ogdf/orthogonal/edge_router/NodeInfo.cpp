/** \file
 * \brief Implementation of class NodeInfo.
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

#include <ogdf/orthogonal/edge_router/NodeInfo.h>

namespace ogdf {
namespace edge_router {

void NodeInfo::get_data(
	OrthoRep& O,
	GridLayout& L,
	node v,
	RoutingChannel<int>& rc,
	NodeArray<int>& nw,
	NodeArray<int>& nh)
//initializes basic node data
//nodeboxsize, numsedges, mgenpos
//ACHTUNG: North ist 0, soll aber links sein
{
	edge e;
	//first, initialize the node and cage size
	box_x_size = nw[v]; //nw[P.original(v)];//P.widthOrig(P.original(v));
	box_y_size = nh[v]; //nh[P.original(v)];//P.heightOrig(P.original(v));
	//{ fright, fleft, ftop, fbottom}
	m_vdegree = 0;
	// get the generalization edge position on all four sides if existent
	OrthoDir od = OrthoDir::North;
	int odi = static_cast<int>(od);
	do
	{
		OrthoRep::SideInfoUML sinfo = O.cageInfo(v)->m_side[odi];
		if (sinfo.m_adjGen)
		{
			if ((od == OrthoDir::North) || (od == OrthoDir::East)) set_gen_pos(od, sinfo.m_nAttached[0]);
			else set_gen_pos(od, sinfo.m_nAttached[1]);
			set_num_edges(od, sinfo.m_nAttached[0] + 1 + sinfo.m_nAttached[1]);
			m_vdegree += num_s_edges[odi];
		}
		else
		{
			set_gen_pos(od, -1);
			set_num_edges(od, sinfo.m_nAttached[0]);
			m_vdegree += num_s_edges[odi];
		}
		m_rc[odi] = rc(v, od);//sinfo.m_routingChannel;

		od = OrthoRep::nextDir(od);
		odi = static_cast<int>(od);
	} while (od != OrthoDir::North);

	//compute cage coordinates, use cage corners vertexinfoUML::m_corner
	const OrthoRep::VertexInfoUML* vinfo = O.cageInfo(v);
	adjEntry ae = vinfo->m_corner[0]; e = *ae; //pointing towards north, on left side
	m_ccoord[0] =  L.x(e->source()); //already odDir
	ae = vinfo->m_corner[1]; e = *ae;
	m_ccoord[1] = L.y(e->source()); //already odDir
	ae = vinfo->m_corner[2]; e = *ae;
	m_ccoord[2] = L.x(e->source()); //already odDir
	ae = vinfo->m_corner[3]; e = *ae;
	m_ccoord[3] = L.y(e->source()); //already odDir
	compute_cage_size();
	//fill the in_edges lists for all box_sides
}


std::ostream& operator<<(std::ostream& O, const NodeInfo& inf)
{
	O.precision(5);
#if 0
	O.setf(ios::fixed);
#endif
	O
		<< "box left/top/right/bottom: " << inf.coord(OrthoDir(0)) << "/" << inf.coord(OrthoDir(1)) << "/"
		<< inf.coord(OrthoDir(2)) << "/" << inf.coord(OrthoDir(3)) << "\n"
		<< "boxsize:                   " << inf.box_x_size << ":" << inf.box_y_size << "\n"
		<< "cage l/t/r/b:              " << inf.cageCoord(OrthoDir(0)) << "/" << inf.cageCoord(OrthoDir(1)) << "/"
		<< inf.cageCoord(OrthoDir(2)) << "/" << inf.cageCoord(OrthoDir(3)) << "\n"
		<< "gen. pos.:                 " << inf.gen_pos(OrthoDir(0)) << "/"
		<< inf.gen_pos(OrthoDir(1)) << "/"
		<< inf.gen_pos(OrthoDir(2)) << "/" << inf.gen_pos(OrthoDir(3)) << "\n"
		<< "delta l/t/r/b (left/right):" << inf.delta(OrthoDir::North, OrthoDir::West) << ":" << inf.delta(OrthoDir::North, OrthoDir::East) << " / \n"
		<< "                          " << inf.delta(OrthoDir::East, OrthoDir::North) << ":" << inf.delta(OrthoDir::East, OrthoDir::South) << " / \n"
		<< "                          " << inf.delta(OrthoDir::South, OrthoDir::East) << ":" << inf.delta(OrthoDir::South, OrthoDir::West) << " / "
		<< inf.delta(OrthoDir::West, OrthoDir::South) << ":" << inf.delta(OrthoDir::West, OrthoDir::North) << "\n"
		<< "eps l/t/r/b (left/right):  " << inf.eps(OrthoDir::North, OrthoDir::West) << ":" << inf.eps(OrthoDir::North, OrthoDir::East) << " / \n"
		<< "                          " << inf.eps(OrthoDir::East, OrthoDir::North) << ":" << inf.eps(OrthoDir::East, OrthoDir::South) << " / \n"
		<< "                          " << inf.eps(OrthoDir::South, OrthoDir::East) << ":" << inf.eps(OrthoDir::South, OrthoDir::West) << " / "
		<< inf.eps(OrthoDir::West, OrthoDir::South) << ":" << inf.eps(OrthoDir::West, OrthoDir::North) << "\n"
		<< "rc:                         " << inf.rc(OrthoDir(0)) << "/" << inf.rc(OrthoDir(1)) << "/" << inf.rc(OrthoDir(2)) << "/" << inf.rc(OrthoDir(3)) << "\n"
		<< "num edges:                  " << inf.num_edges(OrthoDir(0)) << "/" << inf.num_edges(OrthoDir(1)) << "/" << inf.num_edges(OrthoDir(2))
		<< "/" << inf.num_edges(OrthoDir(3)) << "\n"
		<< "num bendfree edges:         " << inf.num_bend_free(OrthoDir(0)) << "/" << inf.num_bend_free(OrthoDir(1)) << "/" << inf.num_bend_free(OrthoDir(2))
		<< "/" << inf.num_bend_free(OrthoDir(3)) << std::endl;

	return O;
}

}
}
