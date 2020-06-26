/** \file
 * \brief Implement class ClusterGraphAttributes
 *
 * \author Karsten Klein, Carsten Gutwenger, Max Ilsen
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

// Start enumerating our attributes as one larger than the regular GraphAttributes
const long firstAttribute = GraphAttributes::all + 1;
const long ClusterGraphAttributes::clusterGraphics = firstAttribute << 0;
const long ClusterGraphAttributes::clusterStyle    = firstAttribute << 1;
const long ClusterGraphAttributes::clusterLabel    = firstAttribute << 2;
const long ClusterGraphAttributes::clusterTemplate = firstAttribute << 3;
const long ClusterGraphAttributes::all             = (firstAttribute << 4) - 1; // bitmask that covers all other attributes

ClusterGraphAttributes::ClusterGraphAttributes(const ClusterGraph& cg, long initAttributes)
	: GraphAttributes(cg.constGraph(), initAttributes | edgeType | nodeType | nodeGraphics | edgeGraphics)
	, m_pClusterGraph(&cg)
{
	addAttributes(initAttributes);
}

void ClusterGraphAttributes::addClusterAttributes(long attr) {
	if (attr & clusterGraphics) {
		m_x.init(*m_pClusterGraph, 0.0);
		m_y.init(*m_pClusterGraph, 0.0);
		m_width.init(*m_pClusterGraph, 0.0);
		m_height.init(*m_pClusterGraph, 0.0);
	}
	if (attr & clusterStyle) {
		m_stroke.init(*m_pClusterGraph, LayoutStandards::defaultClusterStroke());
		m_fill.init(*m_pClusterGraph, LayoutStandards::defaultClusterFill());
	}
	if (attr & clusterLabel) {
		m_label.init(*m_pClusterGraph);
	}
	if (attr & clusterTemplate) {
		m_clusterTemplate.init(*m_pClusterGraph);
	}
}

void ClusterGraphAttributes::destroyClusterAttributes(long attr) {
	if (attr & clusterGraphics) {
		m_x.init();
		m_y.init();
		m_width.init();
		m_height.init();
	}
	if (attr & clusterStyle) {
		m_stroke.init();
		m_fill.init();
	}
	if (attr & clusterLabel) {
		m_label.init();
	}
	if (attr & clusterTemplate) {
		m_clusterTemplate.init();
	}
}

void ClusterGraphAttributes::init(long attr)
{
	GraphAttributes::init(attr);
	destroyClusterAttributes(m_attributes);
	addClusterAttributes(attr);
}

void ClusterGraphAttributes::init(ClusterGraph &cg, long attr)
{
	GraphAttributes::init(cg.constGraph(), attr);
	m_pClusterGraph = &cg;
	destroyClusterAttributes(m_attributes);
	addClusterAttributes(attr);
}

void ClusterGraphAttributes::addAttributes(long attr)
{
	OGDF_ASSERT(!(m_attributes & clusterStyle) || (m_attributes & clusterGraphics));
	GraphAttributes::addAttributes(attr);
	addClusterAttributes(attr);
}

void ClusterGraphAttributes::destroyAttributes(long attr)
{
	GraphAttributes::destroyAttributes(attr);
	destroyClusterAttributes(attr);
}

// calculates the bounding box of the graph including clusters
DRect ClusterGraphAttributes::boundingBox() const
{
	DRect bb = GraphAttributes::boundingBox();
	double minx = bb.p1().m_x;
	double miny = bb.p1().m_y;
	double maxx = bb.p2().m_x;
	double maxy = bb.p2().m_y;

	if (has(ClusterGraphAttributes::clusterGraphics)) {
		bool hasClusterStyle = has(ClusterGraphAttributes::clusterStyle);

		for (cluster c : m_pClusterGraph->clusters) {
			if (c != m_pClusterGraph->rootCluster()) {
				double lw = hasClusterStyle ? 0.5*strokeWidth(c) : 0.0;

				Math::updateMin(minx, x(c) - lw);
				Math::updateMax(maxx, x(c) + width(c) + lw);
				Math::updateMin(miny, y(c) - lw);
				Math::updateMax(maxy, y(c) + height(c) + lw);
			}
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
			x(c) = x(*nit) - width(*nit)/2;
			y(c) = y(*nit) - height(*nit)/2;
			width(c) = x(*nit) + width(*nit)/2;
			height(c) = y(*nit) + height(*nit)/2;
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
			if (x(c) > x(*nit) - width(*nit)/2)
				x(c) = x(*nit) - width(*nit)/2;
			if (y(c) > y(*nit) - height(*nit)/2)
				y(c) = y(*nit) - height(*nit)/2;
			if (width(c) < x(*nit) + width(*nit)/2)
				width(c) = x(*nit) + width(*nit)/2;
			if (height(c) < y(*nit) + height(*nit)/2)
				height(c) = y(*nit) + height(*nit)/2;
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
		m_x[c] *= sx;
		m_y[c] *= sy;
		m_width[c] *= asx;
		m_height[c] *= asy;
	}
}


void ClusterGraphAttributes::translate(double dx, double dy)
{
	GraphAttributes::translate(dx, dy);

	for (cluster c : m_pClusterGraph->clusters) {
		x(c) += dx;
		y(c) += dy;
	}
}


void ClusterGraphAttributes::flipVertical(const DRect &box)
{
	GraphAttributes::flipVertical(box);

	double dy = box.p1().m_y + box.p2().m_y;
	for (cluster c : m_pClusterGraph->clusters) {
		y(c) = dy - y(c);
	}
}


void ClusterGraphAttributes::flipHorizontal(const DRect &box)
{
	GraphAttributes::flipHorizontal(box);

	double dx = box.p1().m_x + box.p2().m_x;
	for (cluster c : m_pClusterGraph->clusters) {
		x(c) = dx - x(c);
	}
}

}
