/** \file
 * \brief Definition of ogdf::TutteLayout.
 *
 * \author David Alberts \and Andrea Wagner
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

#include <ogdf/energybased/TutteLayout.h>


#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>


namespace ogdf {


// solves a system of linear equations with a linear solver for optimization problems.
// I'm sorry but there is no Gauss-Algorithm (or some numerical stuff) in OGDF...
bool TutteLayout::solveLP(
	int cols,
	const CoinPackedMatrix &Matrix,
	const Array<double> &rightHandSide,
	Array<double> &x)
{
	int i;

	OsiSolverInterface *osi = CoinManager::createCorrectOsiSolverInterface();

	// constructs a dummy optimization problem.
	// The given system of equations is used as constraint.
	osi->setObjSense(-1);                        // maximize...
	Array<double> obj(0,cols-1,1);               // ...the sum of variables
	Array<double> lowerBound(0,cols-1,-1*(osi->getInfinity()));
	Array<double> upperBound(0,cols-1,osi->getInfinity());

	// loads the problem to Coin-Osi
	osi->loadProblem(Matrix, &lowerBound[0], &upperBound[0], &obj[0], &rightHandSide[0], &rightHandSide[0]);

	// solves the linear program
	osi->initialSolve();

	// gets the solution and returns true if it is optimal
	const double *sol = osi->getColSolution();
	for(i=0; i<cols; i++) x[i] = sol[i];

	bool returnValue = osi->isProvenOptimal();
	delete osi;
	return returnValue;
}


TutteLayout::TutteLayout() : m_bbox(0.0, 0.0, 250.0, 250.0)
{
}



// sets the positions of the nodes in a largest face of G in the form
// of a regular k-gon. The corresponding nodes and their positions are
// stored in nodes and pos, respectively.
void TutteLayout::setFixedNodes(
	const Graph &G,
	List<node>& nodes,
	List<DPoint>& pos,
	double radius)
{
	// compute faces of a copy of G
	GraphCopy GC(G);

	// compute a planar embedding if G is planar
	OGDF_ASSERT(isPlanar(G));
	planarEmbedPlanarGraph(GC);

	CombinatorialEmbedding E(GC);
	E.computeFaces();

	// search for largest face
	face maxFace = E.maximalFace();

	// delete possible old entries in nodes and pos
	nodes.clear();
	pos.clear();

	// set nodes and pos
	NodeArray<bool> addMe(GC,true);

	List<node> maxNodes;
	for(adjEntry adj : maxFace->entries) {
		maxNodes.pushBack(adj->theNode());
	}

	for(node w : maxNodes) {
		if(addMe[w]) {
			nodes.pushBack(GC.original(w));
			addMe[w] = false;
		}
	}

	double step  = 2.0 * Math::pi / (double)(nodes.size());
	double alpha = 0.0;
	for(int i = 0; i < nodes.size(); ++i) {
		pos.pushBack(DPoint(radius * cos(alpha), radius * sin(alpha)));
		alpha += step;
	}
}

// Overload setFixedNodes for given nodes
void TutteLayout::setFixedNodes(
	const Graph &G,
	List<node>& nodes,
	const List<node>& givenNodes,
	List<DPoint>& pos,
	double radius)
{
	GraphCopy GC(G);

	// delete possible old entries
	pos.clear();

	// set nodes and pos
	nodes = givenNodes;

	double step  = 2.0 * Math::pi / (double)(nodes.size());
	double alpha = 0.0;
	for(int i = 0; i < nodes.size(); ++i) {
		pos.pushBack(DPoint(radius * cos(alpha), radius * sin(alpha)));
		alpha += step;
	}
}

void TutteLayout::call(GraphAttributes &AG, const List<node> &givenNodes)
{
	const Graph &G = AG.constGraph();

	List<node> fixedNodes;
	List<DPoint> positions;
	DRect oldBBox = m_bbox;

	double diam =
	sqrt((m_bbox.width()) * (m_bbox.width())
		 + (m_bbox.height()) * (m_bbox.height()));

	// handle graphs with less than two nodes
	switch (G.numberOfNodes()) {
		case 0:
			return;
		case 1:
			node v = G.firstNode();

			DPoint center(0.5 * m_bbox.width(),0.5 * m_bbox.height());
			center = center + m_bbox.p1();

			AG.x(v) = center.m_x;
			AG.y(v) = center.m_y;

			return;
	}

	// increase radius to have no overlap on the outer circle
	node v = G.firstNode();

	double r        = diam/2.8284271;
	// Prevent division by zero that occurs for n=2 in later if-condition.
	int n           = G.numberOfNodes() == 2 ? 3 : G.numberOfNodes();
	double nodeDiam = 2.0*sqrt((AG.width(v)) * (AG.width(v))
			 + (AG.height(v)) * (AG.height(v)));

	if(r<nodeDiam/(2*sin(2*Math::pi/n))) {
		r=nodeDiam/(2*sin(2*Math::pi/n));
		m_bbox = DRect (0.0, 0.0, 2*r, 2*r);
	}

	setFixedNodes(G,fixedNodes,givenNodes,positions,r);

	doCall(AG,fixedNodes,positions);

	m_bbox = oldBBox;
}

void TutteLayout::call(GraphAttributes &AG)
{
	const Graph &G = AG.constGraph();

	List<node> fixedNodes;
	List<DPoint> positions;
	DRect oldBBox = m_bbox;

	double diam =
	sqrt((m_bbox.width()) * (m_bbox.width())
		 + (m_bbox.height()) * (m_bbox.height()));

	// handle graphs with less than two nodes
	switch (G.numberOfNodes()) {
		case 0:
			return;
		case 1:
			node v = G.firstNode();

			DPoint center(0.5 * m_bbox.width(),0.5 * m_bbox.height());
			center = center + m_bbox.p1();

			AG.x(v) = center.m_x;
			AG.y(v) = center.m_y;

			return;
	}

	// increase radius to have no overlap on the outer circle
	node v = G.firstNode();

	double r        = diam/2.8284271;
	// Prevent division by zero that occurs for n=2 in later if-condition.
	int n           = G.numberOfNodes() == 2 ? 3 : G.numberOfNodes();
	double nodeDiam = 2.0*sqrt((AG.width(v)) * (AG.width(v))
			 + (AG.height(v)) * (AG.height(v)));

	if(r<nodeDiam/(2*sin(2*Math::pi/n))) {
		r=nodeDiam/(2*sin(2*Math::pi/n));
		m_bbox = DRect (0.0, 0.0, 2*r, 2*r);
	}

	setFixedNodes(G,fixedNodes,positions,r);

	doCall(AG,fixedNodes,positions);

	m_bbox = oldBBox;
}



// does the actual computation. fixedNodes and fixedPositions
// contain the nodes with fixed positions.
bool TutteLayout::doCall(
	GraphAttributes &AG,
	const List<node> &fixedNodes,
	List<DPoint> &fixedPositions)
{
	const Graph &G = AG.constGraph();

	OGDF_ASSERT(isTriconnected(G));

	GraphCopy GC(G);
	GraphAttributes AGC(GC);

	// mark fixed nodes and set their positions in a
	NodeArray<bool> fixed(GC, false);
	for (node w : fixedNodes) {
		node v = GC.copy(w);
		fixed[v] = true;

		DPoint p = fixedPositions.popFrontRet();   // slightly dirty...
		fixedPositions.pushBack(p);          // ...

		AGC.x(v) = p.m_x;
		AGC.y(v) = p.m_y;
	}

	if (fixedNodes.size() == G.numberOfNodes()) {
		for (node v : GC.nodes) {
			AG.x(GC.original(v)) = AGC.x(v);
			AG.y(GC.original(v)) = AGC.y(v);
		}
		return true;
	}
	// all nodes have fixed positions - nothing left to do

	// collect other nodes
	List<node> otherNodes;
	for (node v : GC.nodes)
	if (!fixed[v]) otherNodes.pushBack(v);

	NodeArray<int> ind(GC);       // position of v in otherNodes and A

	int i = 0;
	for (node v : otherNodes)
		ind[v] = i++;

	int n = otherNodes.size();           // #other nodes
	Array<double> coord(n);              // coordinates (first x then y)
	Array<double> rhs(n);                // right hand side
	double oneOverD = 0.0;

	CoinPackedMatrix A(false, 0, 0);       // equations
	A.setDimensions(n, n);

	// initialize non-zero entries in matrix A
	for (node v : otherNodes) {
		oneOverD = (double) (1.0 / (v->degree()));

		for(adjEntry adj : v->adjEntries) {
			node w = adj->twinNode();
			if (!fixed[w]) {
				A.modifyCoefficient(ind[v], ind[w], oneOverD);
			}
		}
		A.modifyCoefficient(ind[v], ind[v], -1);
	}

	// compute right hand side for x coordinates
	for (node v : otherNodes) {
		rhs[ind[v]] = 0;
		oneOverD = (double) (1.0 / (v->degree()));

		for(adjEntry adj : v->adjEntries) {
			node w = adj->twinNode();
			if (fixed[w]) rhs[ind[v]] -= (oneOverD*AGC.x(w));
		}
	}

	// compute x coordinates
	if (!(solveLP(n, A, rhs, coord))) return false;
	for (node v : otherNodes)
		AGC.x(v) = coord[ind[v]];

	// compute right hand side for y coordinates
	for (node v : otherNodes) {
		rhs[ind[v]] = 0;
		oneOverD = (double) (1.0 / (v->degree()));

		for(adjEntry adj : v->adjEntries) {
			node w = adj->twinNode();
			if (fixed[w]) rhs[ind[v]] -= (oneOverD*AGC.y(w));
		}
	}

	// compute y coordinates
	if (!(solveLP(n, A, rhs, coord))) return false;
	for (node v : otherNodes)
		AGC.y(v) = coord[ind[v]];

	// translate coordinates, such that the center lies in
	// the center of the bounding box
	DPoint center(0.5 * m_bbox.width(), 0.5 * m_bbox.height());

	for (node v : GC.nodes) {
		AGC.x(v) += center.m_x;
		AGC.y(v) += center.m_y;
	}

	for (node v : GC.nodes) {
		AG.x(GC.original(v)) = AGC.x(v);
		AG.y(GC.original(v)) = AGC.y(v);
	}

	return true;
}

}
