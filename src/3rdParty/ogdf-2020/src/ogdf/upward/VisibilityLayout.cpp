/** \file
 * \brief Implementation of visibility layout algorithm.
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

#include <ogdf/upward/VisibilityLayout.h>
#include <ogdf/basic/simple_graph_alg.h>

namespace ogdf {


void VisibilityLayout::call(GraphAttributes &GA)
{
	if (GA.constGraph().numberOfNodes() <= 1)
		return;

	//call upward planarizer
	UpwardPlanRep UPR;
	UPR.createEmpty(GA.constGraph());
	m_upPlanarizer->call(UPR);
	layout(GA, UPR);
}


void VisibilityLayout::layout(GraphAttributes &GA, const UpwardPlanRep &UPROrig)
{
	UpwardPlanRep UPR = UPROrig;

	//clear some data
	for(edge e : GA.constGraph().edges) {
		GA.bends(e).clear();
	}

	int minGridDist = 1;
	for(node v : GA.constGraph().nodes) {
		if (minGridDist < max(GA.height(v), GA.width(v)))
			minGridDist = (int) max(GA.height(v), GA.width(v));
	}
	minGridDist = max(minGridDist*2+1, m_grid_dist);

	CombinatorialEmbedding &gamma = UPR.getEmbedding();
	//add edge (s,t)
	adjEntry adjSrc = nullptr;
	for(adjEntry adj : UPR.getSuperSource()->adjEntries) {
		if (gamma.rightFace(adj) == gamma.externalFace()) {
			adjSrc = adj;
			break;
		}
	}

	OGDF_ASSERT(adjSrc != nullptr);

	edge e_st = UPR.newEdge(adjSrc, UPR.getSuperSink()); // on the right
	gamma.computeFaces();
	gamma.setExternalFace(gamma.rightFace(e_st->adjSource()));

	constructVisibilityRepresentation(UPR);

	// the preliminary postion
	NodeArray<int> xPos(UPR);
	NodeArray<int> yPos(UPR);

	// node Position
	for(node v : UPR.nodes) {
		NodeSegment vVis = nodeToVis[v];
		int x = (int) (vVis.x_l + vVis.x_r)/2 ; // median positioning
		xPos[v] = x;
		yPos[v] = vVis.y;

		if (UPR.original(v) != nullptr) {
			node vOrig = UPR.original(v);
			//final position
			GA.x(vOrig) = x * minGridDist;
			GA.y(vOrig)	= vVis.y * minGridDist;
		}
	}

	//compute bendpoints
	for(edge e : GA.constGraph().edges) {
		const List<edge> &chain = UPR.chain(e);
		for(edge eUPR : chain) {
			EdgeSegment eVis = edgeToVis[eUPR];
			if (chain.size() == 1) {
				if ((yPos[eUPR->target()] - yPos[eUPR->source()]) > 1) {
					DPoint p1(eVis.x*minGridDist, (yPos[eUPR->source()]+1)*minGridDist);
					DPoint p2(eVis.x*minGridDist, (yPos[eUPR->target()]-1)*minGridDist);
					GA.bends(e).pushBack(p1);
					if (yPos[eUPR->source()]+1 != yPos[eUPR->target()]-1)
						GA.bends(e).pushBack(p2);
				}
			}
			else {
				//short edge
				if ((yPos[eUPR->target()] - yPos[eUPR->source()]) == 1) {
					if (UPR.original(eUPR->target()) == nullptr) {
						node tgtUPR = eUPR->target();
						DPoint p(xPos[tgtUPR]*minGridDist, yPos[tgtUPR]*minGridDist);
						GA.bends(e).pushBack(p);
					}
				}
				//long edge
				else {
					DPoint p1(eVis.x*minGridDist, (yPos[eUPR->source()]+1)*minGridDist);
					DPoint p2(eVis.x*minGridDist, (yPos[eUPR->target()]-1)*minGridDist);
					GA.bends(e).pushBack(p1);
					if (yPos[eUPR->source()]+1 != yPos[eUPR->target()]-1)
						GA.bends(e).pushBack(p2);
					if (UPR.original(eUPR->target()) == nullptr) {
						node tgtUPR = eUPR->target();
						DPoint p(xPos[tgtUPR]*minGridDist, yPos[tgtUPR]*minGridDist);
						GA.bends(e).pushBack(p);
					}
				}
			}
		}

		DPolyline &poly = GA.bends(e);
		if(GA.y(e->source()) > GA.y(e->target())) {
			poly.reverse();
		}
		DPoint pSrc(GA.x(e->source()), GA.y(e->source()));
		DPoint pTgt(GA.x(e->target()), GA.y(e->target()));
		poly.normalize(pSrc, pTgt);
	}
}


void VisibilityLayout::constructDualGraph(
		const UpwardPlanRep& UPR,
		Graph& D,
		node& s_D,
		node& t_D,
		FaceArray<node>& faceToNode,
		NodeArray<face>& leftFace_node,
		NodeArray<face>& rightFace_node,
		EdgeArray<face>& leftFace_edge,
		EdgeArray<face>& rightFace_edge) {
	const CombinatorialEmbedding& gamma = UPR.getEmbedding();

	faceToNode.init(gamma, nullptr);
	leftFace_node.init(UPR, nullptr);
	rightFace_node.init(UPR, nullptr);
	leftFace_edge.init(UPR, nullptr);
	rightFace_edge.init(UPR, nullptr);

	//construct a node for each face f
	for(face f : gamma.faces) {
		faceToNode[f] = D.newNode();

		if (f == gamma.externalFace())
			s_D = faceToNode[f] ;

		//compute face switches
		node s = nullptr, t = nullptr;
		for(adjEntry adj : f->entries) {
			adjEntry adjNext = adj->faceCycleSucc();
			if (adjNext->theEdge()->source() == adj->theEdge()->source())
				s = adjNext->theEdge()->source();
			if (adjNext->theEdge()->target() == adj->theEdge()->target())
				t = adjNext->theEdge()->target();
		}
		OGDF_ASSERT(s);
		OGDF_ASSERT(t);

		//compute left and right face
		bool passSource = false;
		adjEntry adj;
		if (f == gamma.externalFace()) {
			adj = UPR.getSuperSink()->firstAdj();
			if (gamma.rightFace(adj) != gamma.externalFace())
				adj = adj->cyclicSucc();
		}
		else
			adj = UPR.getAdjEntry(gamma, t, f);

		adjEntry adjBegin = adj;
		do {
			node v = adj->theEdge()->source();
			if (!passSource) {
				if (v != s)
					leftFace_node[v] = f;
				leftFace_edge[adj->theEdge()] = f;
			}
			else {
				if (v != s)
					rightFace_node[v] = f;
				rightFace_edge[adj->theEdge()] = f;
			}
			if (adj->theEdge()->source() == s)
				passSource = true;
			adj = adj->faceCycleSucc();
		} while(adj != adjBegin);
	}
	t_D = D.newNode(); // the second (right) node associated with the external face

	//construct dual edges
	for(edge e : UPR.edges) {
		face f_r = rightFace_edge[e];
		face f_l = leftFace_edge[e];
		node u = faceToNode[f_l];
		node v = faceToNode[f_r];
		if (f_r == gamma.externalFace() || f_r == f_l)
			D.newEdge(u, t_D);
		else
			D.newEdge(u,v);
	}

	OGDF_ASSERT(isConnected(D));
}


void VisibilityLayout::constructVisibilityRepresentation(const UpwardPlanRep& UPR)
{
	FaceArray<node> faceToNode;
	NodeArray<face> leftFace_node;
	NodeArray<face> rightFace_node;
	EdgeArray<face> leftFace_edge;
	EdgeArray<face> rightFace_edge;

	Graph D; // the dual graph of the UPR
	node s_D; // super source of D
	node t_D; // super sink f D

	constructDualGraph(UPR, D, s_D, t_D, faceToNode, leftFace_node, rightFace_node, leftFace_edge, rightFace_edge);
#if 0
	makeSimple(D);
	if (t_D->degree() <= 1)
		D.newEdge(s_D, t_D); // make biconnected

	OGDF_ASSERT(isSimple(UPR));
	OGDF_ASSERT(isBiconnected(UPR));
	OGDF_ASSERT(isSimple(D));
	OGDF_ASSERT(isBiconnected(D));
#endif

	//compute top. numbering
	NodeArray<int> topNumberUPR(UPR);
	NodeArray<int> topNumberD(D);

	topologicalNumbering(UPR, topNumberUPR);
	topologicalNumbering(D, topNumberD);

	nodeToVis.init(UPR);
	edgeToVis.init(UPR);

	for(node v : UPR.nodes) {
		NodeSegment vVis;

		//std::cout << "node : " << v << " stNum: " << topNumberUPR[v] << std::endl;

		if (v == UPR.getSuperSource() || v == UPR.getSuperSink()) {
			vVis.y = topNumberUPR[v];
			vVis.x_l = topNumberD[s_D];
			vVis.x_r = topNumberD[t_D]-1;
			nodeToVis[v] =vVis;
			continue;
		}

		vVis.y = topNumberUPR[v];
		face f_v = leftFace_node[v];
		node vD = faceToNode[f_v];
		vVis.x_l = topNumberD[vD];
		f_v = rightFace_node[v];
		vD = faceToNode[f_v];
		vVis.x_r = topNumberD[vD]-1;
		nodeToVis[v] =vVis;
	}

	for(edge e : UPR.edges) {
		EdgeSegment eVis;
		face f_v = leftFace_edge[e];
		node vD = faceToNode[f_v];
		eVis.x = topNumberD[vD];
		eVis.y_b = topNumberUPR[e->source()];
		eVis.y_t = topNumberUPR[e->target()];
		edgeToVis[e] = eVis;
	}
}

}
