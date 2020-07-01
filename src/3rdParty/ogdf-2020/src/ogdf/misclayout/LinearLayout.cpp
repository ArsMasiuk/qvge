/** \file
 * \brief This class is a simple layout that places nodes next to each other
 * and draws edges as bows above the nodes.
 * The user may decide whether to use a custom permutation or use the ordering
 * given by the nodes indices.
 *
 * \author Sebastian Semper
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

#include <ogdf/misclayout/LinearLayout.h>

namespace ogdf {

LinearLayout::LinearLayout(double w, ListPure<node> o) {
	m_outWidth = w;
	m_nodeOrder = o;
	m_customOrder = 0;
}

LinearLayout::LinearLayout() {
	m_outWidth = 100;
	m_customOrder = 0;
}

LinearLayout::~LinearLayout() {
}

void LinearLayout::call(GraphAttributes& GA) {
	const Graph& G = GA.constGraph();
	ListPure<node> nodes;
	if (m_customOrder) {
		nodes = m_nodeOrder;
	} else {
		G.allNodes(nodes);
	}

	double x(0);
	double step(m_outWidth / double(nodes.size()-1));

	//first position all nodes next to each other
	for (auto &n : nodes) {
		GA.x(n) = x;
		x += step;
	}

	//then bend all the edges
	ListPure<edge> edges;
	G.allEdges(edges);
	for (auto &e : edges) {
		node n1 = e->source();
		node n2 = e->target();
		if (!(++(nodes.search(n1)) == nodes.search(n2)) && !(++(nodes.search(n2)) == nodes.search(n1))) {
			DPolyline &pL = GA.bends(e);
			const double m(0.5 * (GA.x(n1) + GA.x(n2)));
			const double r(std::abs(GA.x(n1) - m));
			const double sgn(GA.x(n1) > GA.x(n2) ? 1.0 : -1.0);
			int segments = int(sqrt(r*Math::pi / 0.2));
			for (int i = segments; i--;) {
				const double di = double(i);
				const double newX = m - sgn*r*cos(di / segments*Math::pi);
				const double newY = 0.5*GA.height(n1) - r*sin(di / segments*Math::pi);
				pL.pushBack(DPoint(newX, newY));
			}
		}
	}
}

void LinearLayout::setCustomOrder(bool o) {
	m_customOrder = o;
}

}
