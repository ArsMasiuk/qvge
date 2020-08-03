/** \file
 * \brief implementation of class PlanarizationLayout.
 *
 * applies planarization approach for drawing graphs
 * by calling a planar layouter for every planarized connected
 * component
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

#include <ogdf/planarity/PlanarizationLayout.h>
#include <ogdf/planarity/SubgraphPlanarizer.h>
#include <ogdf/orthogonal/OrthoLayout.h>
#include <ogdf/planarity/SimpleEmbedder.h>
#include <ogdf/packing/TileToRowsCCPacker.h>
#include <ogdf/clique/CliqueFinderSPQR.h>
#include <ogdf/clique/CliqueFinderHeuristic.h>

namespace ogdf {

PlanarizationLayout::PlanarizationLayout()
{
	//modules
	m_crossMin      .reset(new SubgraphPlanarizer);
	m_planarLayouter.reset(new OrthoLayout);
	m_packer        .reset(new TileToRowsCCPacker);
	m_embedder      .reset(new SimpleEmbedder);

	//parameters
	m_pageRatio = 1.0;
	m_cliqueSize = 10;
}


void PlanarizationLayout::call(GraphAttributes &ga)
{
	m_nCrossings = 0;

	PlanRep pr(ga);
	const int numCC = pr.numberOfCCs();

	Array<DPoint> boundingBox(numCC);

	for(int cc = 0; cc < numCC; ++cc)
	{
		// 1. crossing minimization
		int cr;
		m_crossMin->call(pr, cc, cr);
		m_nCrossings += cr;
		OGDF_ASSERT(isPlanar(pr));

		// 2. embedding
		adjEntry adjExternal;
		m_embedder->call(pr, adjExternal);

		// 3. (planar) layout

		Layout drawing(pr);
		m_planarLayouter->call(pr, adjExternal, drawing);

		for(int i = pr.startNode(); i < pr.stopNode(); ++i) {
			node vG = pr.v(i);

			ga.x(vG) = drawing.x(pr.copy(vG));
			ga.y(vG) = drawing.y(pr.copy(vG));

			for(adjEntry adj : vG->adjEntries) {
				if ((adj->index() & 1) == 0)
					continue;
				edge eG = adj->theEdge();
				drawing.computePolylineClear(pr, eG, ga.bends(eG));
			}
		}

		boundingBox[cc] = m_planarLayouter->getBoundingBox();
	}

	// 4. arrange CCs
	arrangeCCs(pr, ga, boundingBox);

	ga.removeUnnecessaryBendsHV();
}


// special call with clique processing (changes graph g temporarily)
// this has been derived from the old implementation
// drawback is that the graph g is changed, it would be better to work on a copy
// where the cliques are replaced
void PlanarizationLayout::call(GraphAttributes &ga, Graph &g)
{
	OGDF_ASSERT(&ga.constGraph() == &g);

	ga.clearAllBends();
	CliqueReplacer cliqueReplacer(ga,g);
	preprocessCliques(g, cliqueReplacer);

	m_nCrossings = 0;

	PlanRep pr(ga);
	const int numCC = pr.numberOfCCs();

	EdgeArray<bool> forbiddenOrig(g,false);
	EdgeArray<int>  costOrig(g,1);

	// In the case of clique processing we have to prevent the
	// clique replacement (star) edges from being crossed.
	// We do not allow that clique replacement is done in the case
	// of UML diagrams, therefore we can temporarily set the type of
	// all deleted and replacement edges to generalization to
	// avoid crossings

	for(edge eOrig : g.edges)
	{
		if (cliqueReplacer.isReplacement(eOrig)) {
			costOrig[eOrig] = 10;
			forbiddenOrig[eOrig] = true;
		}
	}

	Array<DPoint> boundingBox(numCC);

	for(int cc = 0; cc < numCC; ++cc)
	{
		// 1. crossing minimization
		int cr;
		m_crossMin->call(pr, cc, cr, &costOrig, &forbiddenOrig);
		m_nCrossings += cr;
		OGDF_ASSERT(isPlanar(pr));

		// 2. embedding
		adjEntry adjExternal;
		m_embedder->call(pr, adjExternal);

		// 3. (planar) layout

		// insert boundaries around clique representation node
		// and compute a representation layout for the cliques
		// (is needed to guarantee the size of the replacement boundary)
		// conserve external face information
		const SListPure<node> &centerNodes = cliqueReplacer.centerNodes();
		for(node centerNode : centerNodes)
		{
			pr.insertBoundary(centerNode, adjExternal);
		}

		Layout drawing(pr);
		m_planarLayouter->call(pr, adjExternal, drawing);

		// we now have to reposition clique nodes

		// first, we derive the current size of the boundary around
		// the center nodes, then we position the clique nodes
		// in a circular fashion in the boundary

		// the node array is only used for the simple anchor move strategy at the
		// end of this if and can later be removed
		NodeArray<bool> isClique(pr, false);

		for(node centerNode : centerNodes)
		{
			adjEntry adjBoundary = pr.boundaryAdj(centerNode);
			// derive the boundary size
			// if the boundary does not exist (connected component is clique), we
			// only run over the nodes adjacent to centerNode
			double minx, maxx, miny, maxy;
			minx = miny = std::numeric_limits<double>::max();
			maxx = maxy = std::numeric_limits<double>::lowest();
			if (adjBoundary)
			{
				adjEntry adjRunner = adjBoundary;
				// explore the dimension and position of the boundary rectangle
				// TODO: guarantee (edge types?) that we run around a boundary
				do {
					double vx = drawing.x(adjRunner->theNode());
					double vy = drawing.y(adjRunner->theNode());
					if (vx < minx) minx = vx;
					if (vx > maxx) maxx = vx;
					if (vy < miny) miny = vy;
					if (vy > maxy) maxy = vy;

					// are we at a bend or a crossing?
					OGDF_ASSERT(adjRunner->twinNode()->degree() == 2
						 || adjRunner->twinNode()->degree() == 4);
					// bend
					if (adjRunner->twinNode()->degree() < 4)
						adjRunner = adjRunner->faceCycleSucc();
					else adjRunner = adjRunner->faceCycleSucc()->cyclicPred();
				} while (adjRunner != adjBoundary);
			} else {
				for(adjEntry adjCenterNode : centerNode->adjEntries)
				{
					node w = adjCenterNode->twinNode();
					double vx = drawing.x(pr.copy(w));
					double vy = drawing.y(pr.copy(w));
					if (vx < minx) minx = vx;
					if (vx > maxx) maxx = vx;
					if (vy < miny) miny = vy;
					if (vy > maxy) maxy = vy;
				}
			}

			// we now have to arrange the nodes on a circle with positions
			// that respect the position within the given rectangle, i.e.
			// the node with highest position should be on top etc. . This
			// helps in avoiding unnecessary crossings of outgoing edges
			// with the clique circle and unnecessary long edges to the anchors
			// Note that the ordering around centerNode in umlGraph is different
			// to the ordering in the drawing (defined on PG)
			// recompute size of clique and the node positions
			// test

			// derive the ordering of the nodes around centerNode in the
			// planarized copy
			List<node> adjNodes;
			fillAdjNodes(adjNodes, pr, centerNode, isClique, drawing);

			// compute clique node positions
			cliqueReplacer.computeCliquePosition(adjNodes, centerNode, min(maxx-minx,maxy-miny));
			//testend

			double centralX = (maxx-minx)/2.0+minx;
			double centralY = (maxy-miny)/2.0+miny;
			double circleX  = cliqueReplacer.cliqueRect(centerNode).width()/2.0;
			double circleY  = cliqueReplacer.cliqueRect(centerNode).height()/2.0;

			// now we have the position and size of the rectangle around
			// the clique

			// assign shifted coordinates to drawing
			for(adjEntry adjCenterNode : centerNode->adjEntries)
			{
				node w = adjCenterNode->twinNode();
				drawing.x(pr.copy(w)) = centralX-circleX+cliqueReplacer.cliquePos(w).m_x;
				drawing.y(pr.copy(w)) = centralY-circleY+cliqueReplacer.cliquePos(w).m_y;
			}
		}

		// simple strategy to move anchor positions too (they are not needed:
		// move to same position)
		for(node w : pr.nodes)
		{
			// forall clique nodes shift the anchor points
			if (isClique[w])
			{
				adjEntry adRun = w->firstAdj();
				do
				{
					node wOpp = adRun->twinNode();
					drawing.x(wOpp) = drawing.x(w);
					drawing.y(wOpp) = drawing.y(w);
					adRun = adRun->cyclicSucc();
				} while (adRun != w->firstAdj());

			}
		}

		for(int i = pr.startNode(); i < pr.stopNode(); ++i) {
			node vG = pr.v(i);

			ga.x(vG) = drawing.x(pr.copy(vG));
			ga.y(vG) = drawing.y(pr.copy(vG));

			for(adjEntry adj : vG->adjEntries) {
				if ((adj->index() & 1) == 0)
					continue;
				edge eG = adj->theEdge();
				drawing.computePolylineClear(pr, eG, ga.bends(eG));
			}
		}

		boundingBox[cc] = m_planarLayouter->getBoundingBox();
	}

	// 4. arrange CCs
	arrangeCCs(pr, ga, boundingBox);

	ga.removeUnnecessaryBendsHV();
	cliqueReplacer.undoStars();
}


void PlanarizationLayout::preprocessCliques(Graph &G, CliqueReplacer &cliqueReplacer)
{
	cliqueReplacer.setDefaultCliqueCenterSize(m_planarLayouter->separation());

	CliqueFinderHeuristic heurCf;
	CliqueFinderSPQR cf(heurCf);
	cf.setMinSize(m_cliqueSize);
	List<List<node>*> cliques;
	cf.call(G, cliques);

	// now replace all found cliques by stars
	cliqueReplacer.replaceByStar(cliques);
}


// collects and stores nodes adjacent to centerNode in adjNodes
void PlanarizationLayout::fillAdjNodes(List<node>& adjNodes,
	PlanRep& PG,
	node centerNode,
	NodeArray<bool>& isClique,
	Layout& drawing)
{
	// at this point, cages are collapsed, i.e. we have a center node
	// in the planrep representing the copy of the original node
	node cCopy = PG.copy(centerNode);
	OGDF_ASSERT(cCopy != nullptr);
	OGDF_ASSERT(cCopy->degree() == centerNode->degree());
	OGDF_ASSERT(cCopy->degree() > 1);
	// store the node with mostright position TODO: consider cage
	node rightNode = nullptr;

	// due to the implementation in PlanRepUML::collapseVertices, the
	// ordering of the nodes in the copy does not correspond to the
	// ordering in the used embedding, but to the original graph
	// we therefore need to run around the cage to search for the
	// attached edges

	adjEntry adjRun = cCopy->firstAdj();
	do
	{
		// we search for the edge outside the node cage
		// anchor node
		OGDF_ASSERT(adjRun->twinNode()->degree() == 4);
		// should be cs->cs, but doesnt work, comb. embedding?
		// adjEntry outerEdgeAdj = adjRun->twin()->cyclicSucc()->cyclicSucc();
		// TODO: braucht man glaube ich gar nicht, da bereits erste Kante Original hat
		adjEntry outerEdgeAdj = adjRun->twin()->cyclicSucc();
		// this may fail in case of bends if there are no orig edges, but there
		// always is one!?
		while (!PG.original(outerEdgeAdj->theEdge()))
			outerEdgeAdj = outerEdgeAdj->cyclicSucc();
		OGDF_ASSERT(outerEdgeAdj != adjRun);

		// hier besser: if... und alle anderen ignorieren
		edge umlEdge = PG.original(outerEdgeAdj->theEdge());
		OGDF_ASSERT(umlEdge != nullptr);
		node u = umlEdge->opposite(centerNode);
		adjNodes.pushBack(u);
		isClique[PG.copy(u)] = true;

		// part to delete all bends that lie within the clique rectangle
		// first we identify the copy node of the clique node we currently
		// look at
		node uCopy = PG.copy(u);
		OGDF_ASSERT(uCopy);
		adjEntry adjURun = uCopy->firstAdj();
		do
		{
			// we search for the edge outside the node cage
			OGDF_ASSERT(adjURun->twinNode()->degree() == 4);
			adjEntry outerEdgeUAdj = adjURun->twin()->cyclicSucc();
			while (!PG.original(outerEdgeUAdj->theEdge()))
				outerEdgeUAdj = outerEdgeUAdj->cyclicSucc();
			OGDF_ASSERT(outerEdgeUAdj != adjRun);
			// outerEdgeUAdj points outwards, edge does too per implementation,
			// but we don't want to rely on that fact
			bool outwards;
			edge potKill = outerEdgeUAdj->theEdge();
			node splitter;
			if (potKill->source() == outerEdgeUAdj->theNode()) //Could use opposite
			{
				splitter = potKill->target();
				outwards = true;

			} else {
				splitter = potKill->source();
				outwards = false;
			}
			// we erase bends and should check the node type here, but the only
			// case that can happen is bend
			while (splitter->degree() == 2)
			{
				if (outwards)
				{
					PG.unsplit(potKill, potKill->adjTarget()->cyclicSucc()->theEdge());
					splitter = potKill->target();

				} else {
					edge ek = potKill->adjSource()->cyclicSucc()->theEdge();
					PG.unsplit(ek, potKill);
					potKill = ek;
					splitter = potKill->source();
				}
			}

			adjURun = adjURun->cyclicPred(); //counterclockwise, Succ clockwise
		} while (adjURun != uCopy->firstAdj());

		// check if node is better suited to lie at the right position
		if (rightNode != nullptr) {
			if (drawing.x(PG.copy(u)) > drawing.x(PG.copy(rightNode)))
				rightNode = u;

		} else {
			rightNode = u;
		}

		adjRun = adjRun->cyclicPred(); //counterclockwise, Succ clockwise
	} while (adjRun != cCopy->firstAdj());

	// adjust ordering to start with rightNode
	while (adjNodes.front() != rightNode)
	{
		node tempV = adjNodes.popFrontRet();
		adjNodes.pushBack(tempV);
	}
}


void PlanarizationLayout::callSimDraw(GraphAttributes &ga)
{
	const Graph &g = ga.constGraph();
	m_nCrossings = 0;

	EdgeArray<int> costOrig(g,1);
	EdgeArray<uint32_t> esgOrig(g,0);

	for(edge e : g.edges)
		esgOrig[e] = ga.subGraphBits(e);

	PlanRep pr(ga);
	const int numCC = pr.numberOfCCs();

	Array<DPoint> boundingBox(numCC);

	for(int cc = 0; cc < numCC; ++cc)
	{
		// 1. crossing minimization
		int cr;
		m_crossMin->call(pr, cc, cr, &costOrig, nullptr, &esgOrig);
		m_nCrossings += cr;
		OGDF_ASSERT(isPlanar(pr));

		// 2. embedding
		adjEntry adjExternal;
		m_embedder->call(pr, adjExternal);

		// 3. (planar) layout

		Layout drawing(pr);
		m_planarLayouter->call(pr, adjExternal, drawing);

		for(int i = pr.startNode(); i < pr.stopNode(); ++i) {
			node vG = pr.v(i);

			ga.x(vG) = drawing.x(pr.copy(vG));
			ga.y(vG) = drawing.y(pr.copy(vG));

			for(adjEntry adj : vG->adjEntries) {
				if ((adj->index() & 1) == 0)
					continue;
				edge eG = adj->theEdge();
				drawing.computePolylineClear(pr, eG, ga.bends(eG));
			}
		}

		boundingBox[cc] = m_planarLayouter->getBoundingBox();
	}

	// 4. arrange CCs
	arrangeCCs(pr, ga, boundingBox);

	ga.removeUnnecessaryBendsHV();
}


void PlanarizationLayout::arrangeCCs(PlanRep &pr, GraphAttributes &ga, Array<DPoint> &boundingBox) const
{
	const int numCC = pr.numberOfCCs();
	Array<DPoint> offset(numCC);
	m_packer->call(boundingBox, offset, m_pageRatio);

	for(int cc = 0; cc < numCC; ++cc) {

		const double dx = offset[cc].m_x;
		const double dy = offset[cc].m_y;

		// iterate over all nodes in ith CC
		for(int i = pr.startNode(cc); i < pr.stopNode(cc); ++i)
		{
			node v = pr.v(i);

			ga.x(v) += dx;
			ga.y(v) += dy;

			for(adjEntry adj : v->adjEntries) {
				if ((adj->index() & 1) == 0) continue;
				edge e = adj->theEdge();

				DPolyline &dpl = ga.bends(e);
				for(DPoint &dp : dpl) {
					dp.m_x += dx;
					dp.m_y += dy;
				}
			}
		}
	}
}

}
