/** \file
 * \brief Implements grid mapping mechanism of class GridLayoutModule
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

#include <ogdf/planarlayout/GridLayoutModule.h>

namespace ogdf {

void GridLayoutModule::call(GraphAttributes &AG)
{
	const Graph &G = AG.constGraph();

	// compute grid layout
	GridLayout gridLayout(G);
	doCall(G,gridLayout,m_gridBoundingBox);

	// transform grid layout to real layout
	mapGridLayout(G,gridLayout,AG);
}


void GridLayoutModule::callGrid(const Graph &G, GridLayout &gridLayout)
{
	gridLayout.init(G);
	doCall(G,gridLayout,m_gridBoundingBox);
}


void GridLayoutModule::mapGridLayout(const Graph &G,
	GridLayout &gridLayout,
	GraphAttributes &AG)
{
	// maximum width of columns and rows
	double maxWidth = 0;
	double yMax = 0;

	for(node v : G.nodes) {
		Math::updateMax<double>(maxWidth, AG.width(v));
		Math::updateMax<double>(maxWidth, AG.height(v));
		Math::updateMax<double>(yMax, gridLayout.y(v));
	}

	maxWidth += m_separation;

	// set position of nodes
	for(node v : G.nodes) {
		AG.x(v) = gridLayout.x(v) * maxWidth;
		AG.y(v) = (yMax - gridLayout.y(v)) * maxWidth;
	}

	// transform bend points of edges
	for(edge e : G.edges) {
		IPolyline ipl = gridLayout.polyline(e);

		// Remove superfluous bendpoints
		node v = e->source();
		while(!ipl.empty() && ipl.front() == IPoint(gridLayout.x(v), gridLayout.y(v))) {
			ipl.popFront();
		}
		v = e->target();
		while(!ipl.empty() && ipl.back() == IPoint(gridLayout.x(v), gridLayout.y(v))) {
			ipl.popBack();
		}

		DPolyline &dpl = AG.bends(e);
		dpl.clear();

		for (const IPoint &ip : ipl) {
			dpl.pushBack(DPoint(ip.m_x*maxWidth, (yMax-ip.m_y)*maxWidth));
		}

		dpl.normalize();
	}
}

bool PlanarGridLayoutModule::handleTrivial(const Graph &G, GridLayout &gridLayout, IPoint &boundingBox)
{
	// handle special case of graphs with less than 3 nodes
	node v1, v2;
	switch (G.numberOfNodes()) {
	case 0:
		boundingBox = IPoint(0, 0);
		return true;
	case 1:
		v1 = G.firstNode();
		gridLayout.x(v1) = gridLayout.y(v1) = 0;
		boundingBox = IPoint(0, 0);
		return true;
	case 2:
		v1 = G.firstNode();
		v2 = G.lastNode();
		gridLayout.x(v1) = gridLayout.y(v1) = gridLayout.y(v2) = 0;
		gridLayout.x(v2) = 1;
		boundingBox = IPoint(1, 0);
		return true;
	}
	return false;
}

void PlanarGridLayoutModule::callFixEmbed(GraphAttributes &AG, adjEntry adjExternal)
{
	const Graph &G = AG.constGraph();

	// compute grid layout
	GridLayout gridLayout(G);
	if (!handleTrivial(G, gridLayout, m_gridBoundingBox)) {
		doCall(G, adjExternal, gridLayout, m_gridBoundingBox, true);
	}

	// transform grid layout to real layout
	mapGridLayout(G,gridLayout,AG);
}


void PlanarGridLayoutModule::callGridFixEmbed(
	const Graph &G,
	GridLayout &gridLayout,
	adjEntry adjExternal)
{
	gridLayout.init(G);
	if (!handleTrivial(G, gridLayout, m_gridBoundingBox)) {
		doCall(G, adjExternal, gridLayout, m_gridBoundingBox, true);
	}
}


void GridLayoutPlanRepModule::callGrid(PlanRep &PG, GridLayout &gridLayout)
{
	gridLayout.init(PG);
	if (!handleTrivial(PG, gridLayout, m_gridBoundingBox)) {
		doCall(PG, nullptr, gridLayout, m_gridBoundingBox, false);
	}
}

void GridLayoutPlanRepModule::callGridFixEmbed(
	PlanRep &PG,
	GridLayout &gridLayout,
	adjEntry adjExternal)
{
	gridLayout.init(PG);
	if (!handleTrivial(PG, gridLayout, m_gridBoundingBox)) {
		doCall(PG, adjExternal, gridLayout, m_gridBoundingBox, true);
	}
}

void GridLayoutPlanRepModule::doCall(
	const Graph &G,
	adjEntry adjExternal,
	GridLayout &gridLayout,
	IPoint &boundingBox,
	bool fixEmbedding)
{
	if (G.numberOfNodes() < 2) {
		return;
	}

	// create temporary graph copy and grid layout
	PlanRep PG(G);
	PG.initCC(0); // currently only for a single component!
	GridLayout glPG(PG);

	// determine adjacency entry on external face of PG (if required)
	if(adjExternal != nullptr) {
		edge eG  = adjExternal->theEdge();
		edge ePG = PG.copy(eG);
		adjExternal = (adjExternal == eG->adjSource()) ? ePG->adjSource() : ePG->adjTarget();
	}

	// call algorithm for copy
	doCall(PG,adjExternal,glPG,boundingBox,fixEmbedding);

	// extract layout for original graph
	for(node v : G.nodes) {
		node vPG = PG.copy(v);
		gridLayout.x(v) = glPG.x(vPG);
		gridLayout.y(v) = glPG.y(vPG);
	}

	for(edge e : G.edges) {
		IPolyline &ipl = gridLayout.bends(e);
		ipl.clear();

		for(edge ec : PG.chain(e))
			ipl.conc(glPG.bends(ec));
	}
}

}
