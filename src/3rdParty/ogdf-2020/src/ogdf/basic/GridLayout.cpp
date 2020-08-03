/** \file
 * \brief Implements classes GridLayout and GridLayoutMapped
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


#include <ogdf/basic/GridLayoutMapped.h>
#include <ogdf/basic/HashArray.h>


namespace ogdf {

const int GridLayoutMapped::cGridScale = 2;

IPolyline GridLayout::polyline(edge e) const
{
	IPolyline ipl = m_bends[e];
	IPoint ipStart = IPoint(m_x[e->source()],m_y[e->source()]);
	IPoint ipEnd   = IPoint(m_x[e->target()],m_y[e->target()]);

	if(ipl.empty() || ipStart != ipl.front())
		ipl.pushFront(ipStart);

	if(ipEnd != ipl.back() || ipl.size() < 2)
		ipl.pushBack(ipEnd);

	return ipl;
}

struct OGDF_EXPORT GridPointInfo
{
	GridPointInfo() = default;
	explicit GridPointInfo(node v) : m_v(v) { }
	explicit GridPointInfo(edge e) : m_e(e) { }

	bool operator==(const GridPointInfo &i) const {
		return m_v == i.m_v && m_e == i.m_e;
	}

	bool operator!=(const GridPointInfo &i) const {
		return !operator==(i);
	}

	node m_v = nullptr;
	edge m_e = nullptr;
};

std::ostream &operator<<(std::ostream &os, const GridPointInfo &i)
{
	if(i.m_v == nullptr && i.m_e == nullptr)
		os << "{}";
	else if(i.m_v != nullptr)
		os << "{node " << i.m_v << "}";
	else
		os << "{edge " << i.m_e << "}";

	return os;
}


class IPointHashFunc {
public:
	int hash(const IPoint &ip) {
		return 7*ip.m_x + 23*ip.m_y;
	}
};


bool GridLayout::checkLayout()
{
	const Graph &G = *m_x.graphOf();
	HashArray<IPoint,GridPointInfo> H;

	for(node v : G.nodes)
	{
		IPoint ip = IPoint(m_x[v],m_y[v]);
		GridPointInfo i = H[ip];
		if(i != GridPointInfo()) {
			std::cout << "conflict of " << v << " with " << H[ip] << std::endl;
			return false;
		}

		H[ip] = GridPointInfo(v);
	}

	for(edge e : G.edges)
	{
		const IPolyline &bends = m_bends[e];

		for(const IPoint &ip : bends) {
			GridPointInfo i = H[ip];
			if(i != GridPointInfo()) {
				std::cout << "conflict of bend point " << ip << " of edge " << e << " with " << H[ip] << std::endl;
				return false;
			}

			H[ip] = GridPointInfo(e);
		}

	}

	return true;
}

bool GridLayout::isRedundant(IPoint &p1, IPoint &p2, IPoint &p3)
{
	int dzy1 = p3.m_x - p2.m_x;
	int dzy2 = p3.m_y - p2.m_y;
	int dyx1 = p2.m_x - p1.m_x;

	if (dzy1 == 0) return dyx1 == 0 || dzy2 == 0;

	int f = dyx1 * dzy2;

	return f % dzy1 == 0 && p2.m_y - p1.m_y == f / dzy1;
}

void GridLayout::compact(IPolyline &ip)
{
	if (ip.size() < 3) return;

	ListIterator<IPoint> it = ip.begin();
	IPoint p = *it; ++it;
	for (it = it.succ(); it.valid(); ++it) {
		ListIterator<IPoint> itPred = it.pred();
		if(p == *itPred || isRedundant(p,*itPred,*it)) {
			ip.del(itPred);
		} else {
			p = *itPred;
		}
	}
}

IPolyline GridLayout::getCompactBends(edge e) const
{
	IPolyline ipl = m_bends[e];

	if (ipl.size() == 0) return ipl;

#if 0
	IPoint ip_first = ipl.front();
	IPoint ip_last  = ipl.back();
#endif

	IPoint ip_src(m_x[e->source()],m_y[e->source()]);
	IPoint ip_tgt(m_x[e->target()],m_y[e->target()]);

	ipl.pushFront(ip_src);
	ipl.pushBack (ip_tgt);

	compact(ipl);

	ipl.popFront();
	ipl.popBack();

#if 0
	if (ip_first != ip_src && (ipl.empty() || ip_first != ipl.front()))
		ipl.pushFront(ip_first);
	if (ip_last != ip_tgt && (ipl.empty() || ip_last != ipl.back()))
		ipl.pushBack(ip_last);
#endif

	return ipl;
}


void GridLayout::compactAllBends()
{
	const Graph &G = *m_x.graphOf();

	for(edge e : G.edges)
		m_bends[e] = getCompactBends(e);
}

void GridLayout::remap(Layout &drawing)
{
	const Graph &G = *m_x.graphOf();

	for(node v : G.nodes) {
		drawing.x(v) = m_x[v];
		drawing.y(v) = m_y[v];
	}

}


void GridLayout::computeBoundingBox(int &xmin, int &xmax, int &ymin, int &ymax)
{
	const Graph *pG = m_x.graphOf();

	if(pG == nullptr || pG->empty()) {
		xmin = xmax = ymin = ymax = 0;
		return;
	}

	xmin = ymin = std::numeric_limits<int>::max();
	xmax = ymax = std::numeric_limits<int>::min();

	for(node v : pG->nodes) {
		int x = m_x[v];
		if(x < xmin) xmin = x;
		if(x > xmax) xmax = x;

		int y = m_y[v];
		if(y < ymin) ymin = y;
		if(y > ymax) ymax = y;
	}

	for(edge e : pG->edges) {
		for(const IPoint &ip : m_bends[e]) {
			int x = ip.m_x;
			if(x < xmin) xmin = x;
			if(x > xmax) xmax = x;

			int y = ip.m_y;
			if(y < ymin) ymin = y;
			if(y > ymax) ymax = y;
		}
	}
}


int GridLayout::manhattanDistance(const IPoint &ip1, const IPoint &ip2)
{
	return abs(ip2.m_x-ip1.m_x) + abs(ip2.m_y-ip1.m_y);
}

double GridLayout::euclideanDistance(const IPoint &ip1, const IPoint &ip2)
{
	double dx = ip2.m_x-ip1.m_x;
	double dy = ip2.m_y-ip1.m_y;

	return sqrt(dx*dx + dy*dy);
}


int GridLayout::totalManhattanEdgeLength() const
{
	const Graph *pG = m_x.graphOf();
	int length = 0;

	for(edge e : pG->edges)
		length += manhattanEdgeLength(e);

	return length;
}


int GridLayout::maxManhattanEdgeLength() const
{
	const Graph *pG = m_x.graphOf();
	int length = 0;

	for(edge e : pG->edges)
		Math::updateMax(length, manhattanEdgeLength(e));

	return length;
}


int GridLayout::manhattanEdgeLength(edge e) const
{
	int length = 0;

	IPoint ip1 = IPoint(m_x[e->source()],m_y[e->source()]);
	for(const IPoint &ip : m_bends[e]) {
		length += manhattanDistance(ip1,ip);
		ip1 = ip;
	}
	length += manhattanDistance(ip1,IPoint(m_x[e->target()],m_y[e->target()]));

	return length;
}


double GridLayout::totalEdgeLength() const
{
	const Graph *pG = m_x.graphOf();
	double length = 0;

	for(edge e : pG->edges) {
		IPoint ip1 = IPoint(m_x[e->source()],m_y[e->source()]);
		for(const IPoint &ip : m_bends[e]) {
			length += euclideanDistance(ip1,ip);
			ip1 = ip;
		}
		length += euclideanDistance(ip1,IPoint(m_x[e->target()],m_y[e->target()]));
	}

	return length;
}


int GridLayout::numberOfBends() const
{
	const Graph *pG = m_x.graphOf();
	int num = 0;

	for(edge e : pG->edges)
		num += m_bends[e].size();

	return num;
}


GridLayoutMapped::GridLayoutMapped(
	const PlanRep &PG,
	const OrthoRep &OR,
	double separation,
	double cOverhang,
	int fineness) :
		GridLayout(PG), m_gridWidth(PG,0), m_gridHeight(PG,0), m_pPG(&PG)
{
	// determine grid mapping factor
	double minDelta = separation;

	for(node v : PG.nodes)
	{
		node vOrig = PG.original(v);
		if(vOrig == nullptr) continue;

		const OrthoRep::VertexInfoUML *pInfo = OR.cageInfo(v);

		for(int s = 0; s <= 3; ++s) {
			const OrthoRep::SideInfoUML &si = pInfo->m_side[s];
			double size = (s & 1) ? PG.widthOrig(vOrig) : PG.heightOrig(vOrig);
			if (size == 0) continue;

			if(si.m_adjGen) {
				int k = max(si.m_nAttached[0],si.m_nAttached[1]);
				if (k == 0)
					Math::updateMin(minDelta, size/2);
				else
					Math::updateMin(minDelta, size / (2*(k + cOverhang)));

			} else {
				if (si.m_nAttached[0] == 0)
					Math::updateMin(minDelta, size);

				else if ( !((si.m_nAttached[0] == 1) && (cOverhang == 0.0)) ) //hier cov= 0 abfangen
					Math::updateMin(minDelta, size / (si.m_nAttached[0] - 1 + 2*cOverhang));
				else
					Math::updateMin(minDelta, size/2);
			}

		}

	}

	if(0 < cOverhang && cOverhang < 1) {
#if 0
		double tryMinDelta = max(cOverhang * minDelta, separation/10000.0);
		double tryMinDelta = max(cOverhang * minDelta, separation/10000.0);
		if(tryMinDelta < minDelta)
			minDelta = tryMinDelta;
#endif
		minDelta *= cOverhang;
	}

	m_fMapping = fineness / minDelta;


	// initialize grid sizes of vertices
	for(node v : PG.nodes)
	{
		node vOrig = PG.original(v);

		if(vOrig) {
			m_gridWidth [v] = toGrid(PG.widthOrig (vOrig));
			m_gridHeight[v] = toGrid(PG.heightOrig(vOrig));
		}
	}
}


void GridLayoutMapped::remap(Layout &drawing)
{
	for(node v : m_pPG->nodes) {
		//should use toDouble here
		drawing.x(v) = (m_x[v]/cGridScale) / m_fMapping;
		drawing.y(v) = (m_y[v]/cGridScale) / m_fMapping;
	}
}

}
