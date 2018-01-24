/** \file
 * \brief Implement class ClusterGraphAttributes
 *
 * \author Karsten Klein, Carsten Gutwenger
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


#include <ogdf/cluster/ClusterGraphAttributes.h>


namespace ogdf {


ClusterGraphAttributes::ClusterGraphAttributes(ClusterGraph& cg, long initAttributes)
	: GraphAttributes(cg.constGraph(), initAttributes | edgeType | nodeType | nodeGraphics | edgeGraphics),
	m_pClusterGraph(&cg), m_clusterInfo(cg), m_clusterTemplate(cg)
{
	//should we always fill the cluster infos here?
}

//reinitialize graph
void ClusterGraphAttributes::init(ClusterGraph &cg, long initAttributes)
{
	GraphAttributes::init(cg.constGraph(), initAttributes);

	m_pClusterGraph = &cg;
	m_clusterInfo.init(cg);
	m_clusterTemplate.init(cg);
}


//
// calculates the bounding box of the graph including clusters
DRect ClusterGraphAttributes::boundingBox() const
{
	DRect bb = GraphAttributes::boundingBox();
	double minx = bb.p1().m_x;
	double miny = bb.p1().m_y;
	double maxx = bb.p2().m_x;
	double maxy = bb.p2().m_y;

	for (cluster c : m_pClusterGraph->clusters)
	{
		if (c != m_pClusterGraph->rootCluster()) {
			double lw = 0.5*strokeWidth(c);

			Math::updateMin(minx, x(c) - lw);
			Math::updateMax(maxx, x(c) + width(c) + lw);
			Math::updateMin(miny, y(c) - lw);
			Math::updateMax(maxy, y(c) + height(c) + lw);
		}
	}

	return DRect(minx, miny, maxx, maxy);
}


void ClusterGraphAttributes::updateClusterPositions(double boundaryDist)
{
	cluster c;
	//run through children and nodes and update size accordingly
	//we use width, height temporarily to store max values
	forall_postOrderClusters(c,*m_pClusterGraph)
	{
		ListConstIterator<node> nit = c->nBegin();
		ListConstIterator<ClusterElement*> cit = c->cBegin();
		//Initialize with first element
		if (nit.valid())
		{
			x(c) = m_x[*nit] - m_width[*nit]/2;
			y(c) = m_y[*nit] - m_height[*nit]/2;
			width(c) = m_x[*nit] + m_width[*nit]/2;
			height(c) = m_y[*nit] + m_height[*nit]/2;
			++nit;
		}
		else
		{
			if (cit.valid())
			{
				x(c) = x(*cit);
				y(c) = y(*cit);
				width(c) = x(*cit) + width(*cit);
				height(c) = y(*cit) + height(*cit);
				++cit;
			}
			else
			{
				x(c) = 0.0;
				y(c) = 0.0;
				width(c) = 1.0;
				height(c) = 1.0;
			}
		}
		//run through elements and update
		while (nit.valid())
		{
			if (x(c) > m_x[*nit] - m_width[*nit]/2)
				x(c) = m_x[*nit] - m_width[*nit]/2;
			if (y(c) > m_y[*nit] - m_height[*nit]/2)
				y(c) = m_y[*nit] - m_height[*nit]/2;
			if (width(c) < m_x[*nit] + m_width[*nit]/2)
				width(c) = m_x[*nit] + m_width[*nit]/2;
			if (height(c) < m_y[*nit] + m_height[*nit]/2)
				height(c) = m_y[*nit] + m_height[*nit]/2;
			++nit;
		}
		while (cit.valid())
		{
			if (x(c) > x(*cit))
				x(c) = x(*cit);
			if (y(c) > y(*cit))
				y(c) = y(*cit);
			if (width(c) < x(*cit) + width(*cit))
				width(c) = x(*cit) + width(*cit);
			if (height(c) < y(*cit) + height(*cit))
				height(c) = y(*cit) + height(*cit);
			++cit;
		}
		x(c) -= boundaryDist;
		y(c) -= boundaryDist;
		width(c) = width(c) - x(c) + boundaryDist;
		height(c) = height(c) - y(c) + boundaryDist;
	}
}


void ClusterGraphAttributes::scale(double sx, double sy, bool scaleNodes)
{
	GraphAttributes::scale(sx, sy, scaleNodes);

	double asx = fabs(sx), asy = fabs(sy);
	for (cluster c : m_pClusterGraph->clusters) {
		ClusterInfo &info = m_clusterInfo[c];

		info.m_x *= sx;
		info.m_y *= sy;

		info.m_w *= asx;
		info.m_h *= asy;
	}
}


void ClusterGraphAttributes::translate(double dx, double dy)
{
	GraphAttributes::translate(dx, dy);

	for (cluster c : m_pClusterGraph->clusters) {
		ClusterInfo &info = m_clusterInfo[c];

		info.m_x += dx;
		info.m_y += dy;
	}
}


void ClusterGraphAttributes::flipVertical(const DRect &box)
{
	GraphAttributes::flipVertical(box);

	double dy = box.p1().m_y + box.p2().m_y;
	for (cluster c : m_pClusterGraph->clusters) {
		ClusterInfo &info = m_clusterInfo[c];

		info.m_y = dy - info.m_y;
	}
}


void ClusterGraphAttributes::flipHorizontal(const DRect &box)
{
	GraphAttributes::flipHorizontal(box);

	double dx = box.p1().m_x + box.p2().m_x;
	for (cluster c : m_pClusterGraph->clusters) {
		ClusterInfo &info = m_clusterInfo[c];

		info.m_x = dx - info.m_x;
	}
}

}
