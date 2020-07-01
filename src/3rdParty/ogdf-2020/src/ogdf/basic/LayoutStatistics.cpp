/** \file
 * \brief Implements class LayoutStatistics which provides various
 *        functions for computing statistical measures of a layout.
 *
 * \author Carsten Gutwenger
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

#include <ogdf/basic/LayoutStatistics.h>

namespace ogdf {

ArrayBuffer<double> LayoutStatistics::edgeLengths(
	const GraphAttributes &ga,
	bool considerSelfLoops)
{
	ArrayBuffer<double> values;
	for (edge e : ga.constGraph().edges) {
		if (!considerSelfLoops && e->isSelfLoop()) {
			continue;
		}

		const DPolyline &dpl = ga.bends(e);
		DPoint pv = ga.point(e->source());
		DPoint pw = ga.point(e->target());

		double len = 0;
		if (!dpl.empty()) {
			len = dpl.length();
			len += pv.distance(dpl.front());
			len += pw.distance(dpl.back());
		} else {
			len = pv.distance(pw);
		}

		values.push(len);
	}

	return values;
}


ArrayBuffer<int> LayoutStatistics::numberOfBends(
	const GraphAttributes &ga,
	bool considerSelfLoops)
{
	ArrayBuffer<int> values;
	for (edge e : ga.constGraph().edges) {
		if (considerSelfLoops || !e->isSelfLoop()) {
			values.push(ga.bends(e).size());
		}
	}

	return values;
}


ArrayBuffer<double> LayoutStatistics::angles(
	const GraphAttributes &ga,
	bool considerBends)
{
	ArrayBuffer<double> values;
	const Graph &G = ga.constGraph();

	for (node v : G.nodes) {
		double vx = ga.x(v);
		double vy = ga.y(v);

		// Get angles for edge segments incident to v.
		List<double> angles;
		for (adjEntry adj : v->adjEntries) {
			const DPolyline &dpl = ga.bends(adj->theEdge());
			double ex, ey;
			if (dpl.empty()) {
				ex = ga.x(adj->twinNode());
				ey = ga.y(adj->twinNode());
			} else {
				ex = dpl.front().m_x;
				ey = dpl.front().m_y;
			}

			angles.pushBack(atan2(ex-vx, ey-vy));
		}

		if (angles.size() < 2) {
			continue;
		}

		angles.quicksort();
		double lastAngle = angles.back();
		for (double psi : angles) {
			double alpha = psi - lastAngle;

			// happens in the first iteration only
			if (alpha < 0) {
				OGDF_ASSERT(psi == angles.front());
				alpha += 2*Math::pi;
			}

			values.push(alpha);
			lastAngle = psi;
		}
	}

	if (considerBends) {
		for (edge e : G.edges) {
			DPolyline dpl = ga.bends(e);

			dpl.pushFront(ga.point(e->source()));
			dpl.pushBack (ga.point(e->target()));
			dpl.normalize();

			if (dpl.size() < 3) {
				continue;
			}

			for (ListConstIterator<DPoint> it = dpl.begin().succ(); it.succ().valid(); ++it) {
				double bx = (*it).m_x, by = (*it).m_y;

				const DPoint &p1 = *it.pred();
				double psi1 = atan2(p1.m_x-bx, p1.m_y-by);

				const DPoint &p2 = *it.succ();
				double psi2 = atan2(p2.m_x - bx, p2.m_y - by);

				double alpha = fabs(psi1 - psi2);
				if (alpha > Math::pi) {
					alpha -= Math::pi;
				}

				values.push(alpha);
				values.push(alpha*Math::pi);
			}
		}
	}

	return values;
}


ArrayBuffer<int> LayoutStatistics::numberOfCrossings(const GraphAttributes &ga)
{
	ArrayBuffer<int> values;
	const Graph &G = ga.constGraph();
	EdgeArray<int> crossings(G,0);

	Graph H;
	NodeArray<DPoint> points;
	NodeArray<node> origNode;
	EdgeArray<edge> origEdge;
	intersectionGraph(ga, H, points, origNode, origEdge);

	for(node v : H.nodes) {
		node vOrig = origNode[v];
		int d = (vOrig != nullptr) ? vOrig->degree() : 0;
		int k = (v->degree() - d) / 2;

		// If there are two or more intersecting edges:
		if (k >= 2) {
			// For every original edge involved in the crossing:
			for (adjEntry adj : v->adjEntries) {
				if (adj->isSource()) {
					edge e = adj->theEdge();
					edge eOrig = origEdge[e];

					// Ignore original edges incident to vOrig.
					if (eOrig->source() != e->source() ||
						eOrig->target() != e->target()) {
						crossings[eOrig] += (k - 1);
					}
				}
			}
		}
	}

	for (edge e : G.edges) {
		values.push(crossings[e]);
	}

	return values;
}


ArrayBuffer<int> LayoutStatistics::numberOfNodeCrossings(const GraphAttributes &ga)
{
	ArrayBuffer<int> values;
	const Graph &G = ga.constGraph();
	DPoint inter;

	// Get bounding rectangle of every node.
	NodeArray<DRect> nodeRects(G);
	ga.nodeBoundingBoxes<DRect>(nodeRects);

	// For all edges, get the target point of each of their edge segments.
	for (edge e : G.edges) {
		int nCrossingsE = 0;
		node src = e->source();
		node tgt = e->target();
		DPoint vPoint = ga.point(src);

		DPolyline edgeSegmentTargets = ga.bends(e);
		edgeSegmentTargets.pushBack(ga.point(tgt));

		int i = 0;
		int last = edgeSegmentTargets.size()-1;

		// For all edge segments from vPoint to wPoint:
		for (DPoint wPoint : edgeSegmentTargets) {
			DSegment segment = DSegment(vPoint, wPoint);

			// Count crossing of segment with nodes u, but do not count
			// "crossing" of source/target node with first/last edge segment.
			for (node u : G.nodes) {
				if ((u != src || i != 0) && (u != tgt || i != last) &&
				    nodeRects[u].intersection(segment)) {
					nCrossingsE++;
				}
			}
			vPoint = wPoint;
			i++;
		}
		values.push(nCrossingsE);
	}

	return values;
}


ArrayBuffer<int> LayoutStatistics::numberOfNodeOverlaps(const GraphAttributes &ga)
{
	ArrayBuffer<int> values;
	const Graph &G = ga.constGraph();

	// Get bounding rectangle of every node.
	NodeArray<DIntersectableRect> nodeRects(G);
	ga.nodeBoundingBoxes<DIntersectableRect>(nodeRects);

	// For all pairs of nodes, test whether they overlap.
	for (node v : G.nodes) {
		int nOverlapsV = 0;
		for (node w : G.nodes) {
			if (v != w && nodeRects[v].intersects(nodeRects[w])) {
				nOverlapsV++;
			}
		}
		values.push(nOverlapsV);
	}

	return values;
}

}
