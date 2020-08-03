/** \file
 * \brief Implementation of class ShorthestPathWithBFM
 *
 * \author G. W. Klau
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

// Purpose:
//   implementation of shortest path computation
//	 via Bellman-Ford-Moore
//
//   implementation follows Cormen/Leiserson/Rivest

#include <ogdf/graphalg/ShortestPathWithBFM.h>

namespace ogdf {

bool ShortestPathWithBFM::call
	(
	const Graph &G,						// directed graph
	const node s,						// source node
	const EdgeArray<int> &length,		// length of an edge
	NodeArray<int> &d,					// contains shortest path distances after call
	NodeArray<edge> &pi					// predecessors
	)
{
	//Initialize-Single-Source(G, s):
	for (node v : G.nodes) {
		d[v] = std::numeric_limits<int>::max();
		pi[v] = nullptr;
	}
	d[s] = 0;
	for (int i = 1; i < G.numberOfNodes(); ++i) {
		for (edge e : G.edges)	{
			//relax(u, v, w): // e == (u, v), length == w
			if (d[e->target()] > d[e->source()] + length[e]) {
				d[e->target()] = d[e->source()] + length[e];
				pi[e->target()] = e;
			}
		}
	}

	//check for negative cycle:
	for (edge e : G.edges) {
		if (d[e->target()] > d[e->source()] + length[e]) return false;
	}

	return true;
}

}
