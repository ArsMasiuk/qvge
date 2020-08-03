/** \file
 * \brief Implements planar orthogonal drawing algorithm
 *
 * \author Carsten Gutwenger, Sebastian Leipert
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


#include <ogdf/orthogonal/OrthoLayout.h>

#include <ogdf/orthogonal/OrthoShaper.h>
#include <ogdf/orthogonal/FlowCompaction.h>
#include <ogdf/orthogonal/EdgeRouter.h>


namespace ogdf {


OrthoLayout::OrthoLayout()
{
	// options
	m_separation = LayoutStandards::defaultNodeSeparation();
	m_margin     = LayoutStandards::defaultNodeSeparation();
	m_cOverhang  = 0.2;

	m_progressive = true;
	m_bendBound = 2;

	m_useScalingCompaction = false;
	m_scalingSteps = 0;
}


void OrthoLayout::call(PlanRep &PG,
	adjEntry adjExternal,
	Layout &drawing)
{
	// special cases
	if(PG.empty())
		return;

	if(PG.numberOfNodes() == 1) {
		node v1 = PG.firstNode();
		node vOrig = PG.original(v1);
		double w = PG.widthOrig(vOrig);
		double h = PG.heightOrig(vOrig);

		drawing.x(v1) = m_margin + w/2;
		drawing.y(v1) = m_margin + h/2;
		m_boundingBox = DPoint(w + 2*m_margin, h + 2*m_margin);
		return;
	}

	// compaction with scaling: help node cages to pass by each other
	double separation = m_separation;
	if (m_useScalingCompaction)
	{
		m_scalingSteps = 6;
		double scaleFactor = double(int(1 << m_scalingSteps));
		separation = scaleFactor*m_separation; //reduce this step by step in compaction
	}

	// PHASE 1: determine orthogonal shape

	// expand high-degree vertices
	PG.expand();

	// create combinatorial embedding
	CombinatorialEmbedding E(PG);
	E.setExternalFace(E.rightFace(adjExternal));

	// determine orthogonal shape
	OrthoRep OR;
	OrthoShaper OFG;

	OFG.traditional(!m_progressive);
	OFG.setBendBound(m_bendBound);

	OFG.call(PG,E,OR);


	// PHASE 2: construction of a feasible drawing of the expanded graph

	// expand low degree vertices
	PG.expandLowDegreeVertices(OR);

	OGDF_ASSERT(PG.representsCombEmbedding());

	// restore embedding
	E.computeFaces();
	E.setExternalFace(E.rightFace(adjExternal));

	// apply constructive compaction heuristics
	OR.normalize();
	OR.dissect2(&PG);
	OR.orientate(PG,OrthoDir::North);

	// compute cage information and routing channels
	OR.computeCageInfoUML(PG);

	// adjust value of cOverhang
	if(m_cOverhang < 0.05)
		m_cOverhang = 0.0;
	if(m_cOverhang > 0.5)
		m_cOverhang = 0.5;

	// temporary grid layout
	GridLayoutMapped gridDrawing(PG, OR, separation, m_cOverhang, 2);

	RoutingChannel<int> rcGrid(PG, gridDrawing.toGrid(separation), m_cOverhang);
	rcGrid.computeRoutingChannels(OR);

	const OrthoRep::VertexInfoUML *pInfoExp;
	for(node v : PG.nodes) {
		pInfoExp = OR.cageInfo(v);
		if (pInfoExp) break;
	}
	OGDF_ASSERT(pInfoExp);

	FlowCompaction fca;
	fca.constructiveHeuristics(PG,OR,rcGrid,gridDrawing);

	OR.undissect();

	// call flow compaction on grid
	FlowCompaction fc;
	fc.scalingSteps(m_scalingSteps);
	fc.improvementHeuristics(PG, OR, rcGrid, gridDrawing);


	// PHASE 3: routing of edges

	MinimumEdgeDistances<int> minDistGrid(PG, gridDrawing.toGrid(separation));
	{
		EdgeRouter router;
		router.call(PG, OR, gridDrawing, E, rcGrid, minDistGrid, gridDrawing.width(), gridDrawing.height());
	}

	OR.orientate(pInfoExp->m_corner[static_cast<int>(OrthoDir::North)],OrthoDir::North);


	// PHASE 4: apply improvement compaction heuristics

	try {
		// call flow compaction on grid
		fc.improvementHeuristics(PG, OR, minDistGrid, gridDrawing, int(gridDrawing.toGrid(m_separation)));
	} catch(AlgorithmFailureException) {
		// too bad, that did not work out..
	}


	// re-map result
	gridDrawing.remap(drawing);

	// collapse all expanded vertices by introducing a new node in the center
	// of each cage representing the original vertex
	PG.collapseVertices(OR,drawing);

	// finally set the bounding box
	computeBoundingBox(PG,drawing);
}



// compute bounding box and move final drawing such that it is 0 aligned
// respecting margins
void OrthoLayout::computeBoundingBox(
	const PlanRep &PG,
	Layout &drawing)
{
	double minX, maxX, minY, maxY;

	minX = maxX = drawing.x(PG.firstNode());
	minY = maxY = drawing.y(PG.firstNode());

	for(node v : PG.nodes)
	{
		double x = drawing.x(v);
		if (x < minX) minX = x;
		if (x > maxX) maxX = x;

		double y = drawing.y(v);
		if (y < minY) minY = y;
		if (y > maxY) maxY = y;
	}

	double deltaX = m_margin - minX;
	double deltaY = m_margin - minY;

	for(node v : PG.nodes)
	{
		drawing.x(v) += deltaX;
		drawing.y(v) += deltaY;
	}

	m_boundingBox = DPoint(maxX+deltaX+m_margin, maxY+deltaY+m_margin);
}

}
