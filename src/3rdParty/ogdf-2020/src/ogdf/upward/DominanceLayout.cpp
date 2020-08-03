/** \file
 * \brief Implementation of dominance layout algorithm.
 *
 * \author Hoi-Ming Wong and Carsten Gutwenger
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

#include <ogdf/upward/DominanceLayout.h>
#include <ogdf/basic/simple_graph_alg.h>


namespace ogdf {

void DominanceLayout::call(GraphAttributes &GA)
{
	if (GA.constGraph().numberOfNodes() <= 1)
		return;

	OGDF_ASSERT(isSimpleUndirected(GA.constGraph()));
	//call upward planarizer
	UpwardPlanRep UPR;
	UPR.createEmpty(GA.constGraph());
	m_upPlanarizer->call(UPR);
	layout(GA, UPR);
}


void DominanceLayout::layout(GraphAttributes &GA, const UpwardPlanRep &UPROrig)
{

	UpwardPlanRep UPR = UPROrig;

	//clear some data
	for(edge e : GA.constGraph().edges) {
		GA.bends(e).clear();
	}

	//compute and splite transitiv edges
	List<edge> splitMe;
	findTransitiveEdges(UPR, splitMe);

	for(edge eSplit : splitMe) {
		UPR.getEmbedding().split(eSplit);
	}

	// set up first-/lastout, first-/lastin
	firstout.init(UPR, nullptr);
	lastout.init(UPR, nullptr);
	firstin.init(UPR, nullptr);
	lastin.init(UPR, nullptr);

	node s = UPR.getSuperSource();
	node t = UPR.getSuperSink();

	firstout[t] = lastout[t] = nullptr;
	firstin[s] = lastin[s] = nullptr;
	firstin[t] = lastin[t] =t->firstAdj()->theEdge();
	adjEntry adjRun = s->firstAdj();
	while (UPR.getEmbedding().rightFace(adjRun) != UPR.getEmbedding().externalFace()) {
		adjRun = adjRun->cyclicSucc();
	}
	lastout[s] = adjRun->theEdge();
	firstout[s] = adjRun->cyclicSucc()->theEdge();

	for(node v : UPR.nodes) {
		if (v == t || v == s) continue;

		adjEntry adj = UPR.leftInEdge(v);
		firstin[v] = adj->theEdge();
		firstout[v] = adj->cyclicSucc()->theEdge();

		adjEntry adjRightIn = adj;
		while (adjRightIn->cyclicPred()->theEdge()->source() != v)
			adjRightIn = adjRightIn->cyclicPred();

		lastin[v] = adjRightIn->theEdge();
		lastout[v] = adjRightIn->cyclicPred()->theEdge();
	}


	//compute m_L and m_R for min. area drawing
	m_L = 0;
	m_R = 0;
	for(edge e : UPR.edges) {
		node src = e->source();
		node tgt = e->target();
		if (lastin[tgt] == e && firstout[src] == e)
			m_L++;
		if (firstin[tgt] == e && lastout[src] == e)
			m_R++;
	}

	// compute preleminary coordinate
	xPreCoord.init(UPR);
	yPreCoord.init(UPR);
	int count = 0;
	labelX(UPR, s, count);
	count = 0;
	labelY(UPR, s, count);

	// compaction
	compact(UPR, GA);

	// map coordinate to GA
	for(node v : GA.constGraph().nodes) {
		node vUPR = UPR.copy(v);
		GA.x(v) = xCoord[vUPR];
		GA.y(v) = yCoord[vUPR];
	}
	// add bends to original edges
	for(edge e : GA.constGraph().edges) {
		const List<edge> &chain = UPR.chain(e);
		for(edge eChain : chain) {
			node tgtUPR = eChain->target();
			if (tgtUPR != chain.back()->target()) {
				DPoint p(xCoord[tgtUPR], yCoord[tgtUPR]);
				GA.bends(e).pushBack(p);
			}
		}
	}


	//rotate the drawing
	for(node v : GA.constGraph().nodes) {
		double r = sqrt(GA.x(v)*GA.x(v) + GA.y(v)*GA.y(v));
		if (r == 0)
			continue;
		double alpha = asin(GA.y(v)/r);
		double yNew = sin(alpha + m_angle)*r;
		double xNew = cos(alpha + m_angle)*r;
		GA.x(v) = xNew;
		GA.y(v) = yNew;
	}

	for(edge e : GA.constGraph().edges) {
		DPolyline &poly = GA.bends(e);
		DPoint pSrc(GA.x(e->source()), GA.y(e->source()));
		DPoint pTgt(GA.x(e->target()), GA.y(e->target()));
		poly.normalize(pSrc, pTgt);

		for(DPoint &p : poly) {
			double r = p.distance(DPoint(0,0));

			if (r == 0)
				continue;

			double alpha = asin( p.m_y/r);
			double yNew = sin(alpha + m_angle)*r;
			double xNew = cos(alpha + m_angle)*r;
			p.m_x = xNew;
			p.m_y = yNew;
		}

	}
}


void DominanceLayout::labelX(const UpwardPlanRep &UPR, node v, int &count)
{
	xNodes.pushBack(v);
	xPreCoord[v] = count;
	count++;
	if (v != UPR.getSuperSink()) {
		adjEntry adj = firstout[v]->adjSource();
		do {
			node w = adj->theEdge()->target();
			if (adj->theEdge() == lastin[w])
				labelX(UPR, w, count);
			adj = adj->cyclicSucc();
		} while (adj->cyclicPred()->theEdge() != lastout[v]);
	}
}



void DominanceLayout::labelY(const UpwardPlanRep &UPR, node v, int &count)
{
	yNodes.pushBack(v);
	yPreCoord[v] = count;
	count++;
	if (v != UPR.getSuperSink()) {
		adjEntry adj = lastout[v]->adjSource();
		do {
			node w = adj->theEdge()->target();
			if (adj->theEdge() == firstin[w])
				labelY(UPR, w, count);
			adj = adj->cyclicPred();
		} while (adj->cyclicSucc()->theEdge() != firstout[v]);
	}
}


void DominanceLayout::compact(const UpwardPlanRep &UPR, GraphAttributes &GA)
{
	double maxNodeSize = 0;
	for(node v : GA.constGraph().nodes) {
		if (GA.width(v) > maxNodeSize || GA.height(v) > maxNodeSize)
			maxNodeSize = max(GA.width(v), GA.height(v));
	}

	int gridDist = m_grid_dist;
	if (gridDist < maxNodeSize+1)
		gridDist = (int) maxNodeSize+1;

	xCoord.init(UPR);
	yCoord.init(UPR);

	//ASSIGN X COORDINATE

	OGDF_ASSERT(!xNodes.empty());

	node v = xNodes.popFrontRet();
	xCoord[v] = 0;
	while (!xNodes.empty()) {
		node u = xNodes.popFrontRet();
		if ( (yPreCoord[v] > yPreCoord[u]) || (firstout[v] == lastout[v] && firstin[u] == lastin[u] && m_L <= m_R)) {
			xCoord[u] = xCoord[v] + gridDist;
		}
		else
			xCoord[u] = xCoord[v];
		v = u;
	}

	//ASSIGN Y COORDINATE
	OGDF_ASSERT(!yNodes.empty());

	v = yNodes.popFrontRet();
	yCoord[v] = 0;
	while (!yNodes.empty()) {
		node u = yNodes.popFrontRet();
		if ( (xPreCoord[v] > xPreCoord[u]) || (firstout[v] == lastout[v] && firstin[u] == lastin[u] && m_L > m_R)) {
			yCoord[u] = yCoord[v] + gridDist;
		}
		else
			yCoord[u] = yCoord[v];
		v = u;
	}
}



void DominanceLayout::findTransitiveEdges(const UpwardPlanRep &UPR, List<edge> &edges)
{
	// for st-graphs:
	// e = (u,v) transitive <=> ex. face f: e in f and u source-switch and v = sink-switch
	for(face f : UPR.getEmbedding().faces) {
		if (f == UPR.getEmbedding().externalFace())
			continue;

		for(adjEntry adj : f->entries)
		{
			node src = adj->theEdge()->source();
			node tgt = adj->theEdge()->target();
			if ( (adj->faceCycleSucc()->theEdge()->source() == src && adj->faceCyclePred()->theEdge()->target() == tgt)
				|| (adj->faceCycleSucc()->theEdge()->target() == tgt && adj->faceCyclePred()->theEdge()->source() == src))
			{
					edges.pushBack(adj->theEdge());
					break;
			}
		}
	}
}

}
