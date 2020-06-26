/** \file
 * \brief Implementation of the stress minimization via majorization
 * algorithm.
 *
 * \author Mark Ortmann, University of Konstanz
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

#include <ogdf/energybased/StressMinimization.h>


namespace ogdf {

const double StressMinimization::EPSILON = 10e-4;

const int StressMinimization::DEFAULT_NUMBER_OF_PIVOTS = 50;


void StressMinimization::call(GraphAttributes& GA)
{
	const Graph& G = GA.constGraph();
	// if the graph has at most one node nothing to do
	if (G.numberOfNodes() <= 1) {
		// make it exception save
		for(node v : G.nodes)
		{
			GA.x(v) = 0;
			GA.y(v) = 0;
		}
		return;
	}
	// Separate component layout cant be applied to a non-connected graph
	OGDF_ASSERT(!m_componentLayout || isConnected(G));
	NodeArray<NodeArray<double> > shortestPathMatrix(G);
	NodeArray<NodeArray<double> > weightMatrix(G);
	initMatrices(G, shortestPathMatrix, weightMatrix);
	// if the edge costs are defined by the attribute copy it to an array and
	// construct the proper shortest path matrix
	if (m_hasEdgeCostsAttribute) {
		OGDF_ASSERT(GA.has(GraphAttributes::edgeDoubleWeight));
		m_avgEdgeCosts = dijkstra_SPAP(GA, shortestPathMatrix);
		// compute shortest path all pairs
	} else {
		m_avgEdgeCosts = m_edgeCosts;
		bfs_SPAP(G, shortestPathMatrix, m_edgeCosts);
	}
	call(GA, shortestPathMatrix, weightMatrix);
}


void StressMinimization::call(
	GraphAttributes& GA,
	NodeArray<NodeArray<double> >& shortestPathMatrix,
	NodeArray<NodeArray<double> >& weightMatrix)
{
	// compute the initial layout if necessary
	if (!m_hasInitialLayout) {
		computeInitialLayout(GA);
	}
	const Graph& G = GA.constGraph();
	// replace infinity distances by sqrt(n) and compute weights.
	// Note isConnected is only true during calls triggered by the
	// ComponentSplitterLayout.
	if (!m_componentLayout && !isConnected(G)) {
		replaceInfinityDistances(shortestPathMatrix,
				m_avgEdgeCosts * sqrt((double)(G.numberOfNodes())));
	}
	// calculate the weights
	calcWeights(G, shortestPathMatrix, weightMatrix);
	// minimize the stress
	minimizeStress(GA, shortestPathMatrix, weightMatrix);
}


void StressMinimization::computeInitialLayout(GraphAttributes& GA)
{
	PivotMDS* pivMDS = new PivotMDS();
	pivMDS->setNumberOfPivots(DEFAULT_NUMBER_OF_PIVOTS);
	pivMDS->useEdgeCostsAttribute(m_hasEdgeCostsAttribute);
	pivMDS->setEdgeCosts(m_edgeCosts);
	if (!m_componentLayout) {
		// the graph might be disconnected therefore we need
		// the component layouter
		//design decision: should that parameter be passed to CSL?
		ComponentSplitterLayout compLayouter;//(m_hasEdgeCostsAttribute);
		compLayouter.setLayoutModule(pivMDS);
		compLayouter.call(GA);
	} else {
		pivMDS->call(GA);
		delete pivMDS;
	}
}


void StressMinimization::replaceInfinityDistances(
	NodeArray<NodeArray<double> >& shortestPathMatrix,
	double newVal)
{
	const Graph &G = *shortestPathMatrix.graphOf();

	for(node v : G.nodes) {
		for(node w : G.nodes) {
			if (v != w && isinf(shortestPathMatrix[v][w])) {
				shortestPathMatrix[v][w] = newVal;
			}
		}
	}
}


void StressMinimization::calcWeights(
	const Graph& G,
	NodeArray<NodeArray<double> >& shortestPathMatrix,
	NodeArray<NodeArray<double> >& weightMatrix)
{
	for (node v : G.nodes) {
		for (node w : G.nodes) {
			if (v != w) {
				// w_ij = d_ij^-2
				weightMatrix[v][w] = 1
					/ (shortestPathMatrix[v][w] * shortestPathMatrix[v][w]);
			}
		}
	}
}


double StressMinimization::calcStress(
	const GraphAttributes& GA,
	NodeArray<NodeArray<double> >& shortestPathMatrix,
	NodeArray<NodeArray<double> >& weightMatrix)
{
	double stress = 0;
	for (node v = GA.constGraph().firstNode(); v != nullptr; v = v->succ()) {
		for (node w = v->succ(); w != nullptr; w = w->succ()) {
			double xDiff = GA.x(v) - GA.x(w);
			double yDiff = GA.y(v) - GA.y(w);
			double zDiff = 0.0;
			if (GA.has(GraphAttributes::threeD))
			{
				zDiff = GA.z(v) - GA.z(w);
			}
			double dist = sqrt(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff);
			if (dist != 0) {
				stress += weightMatrix[v][w] * (shortestPathMatrix[v][w] - dist)
					* (shortestPathMatrix[v][w] - dist);//
			}
		}
	}
	return stress;
}


void StressMinimization::copyLayout(
	const GraphAttributes& GA,
	NodeArray<double>& newX,
	NodeArray<double>& newY)
{
	// copy the layout
	for(node v : GA.constGraph().nodes)
	{
		newX[v] = GA.x(v);
		newY[v] = GA.y(v);
	}
}


void StressMinimization::copyLayout(
	const GraphAttributes& GA,
	NodeArray<double>& newX,
	NodeArray<double>& newY,
	NodeArray<double>& newZ)
{
	// copy the layout
	for(node v : GA.constGraph().nodes)
	{
		newX[v] = GA.x(v);
		newY[v] = GA.y(v);
		newZ[v] = GA.z(v);
	}
}


void StressMinimization::minimizeStress(
	GraphAttributes& GA,
	NodeArray<NodeArray<double> >& shortestPathMatrix,
	NodeArray<NodeArray<double> >& weightMatrix)
{
	const Graph& G = GA.constGraph();
	int numberOfPerformedIterations = 0;

	double prevStress = std::numeric_limits<double>::max();
	double curStress = std::numeric_limits<double>::max();

	if (m_terminationCriterion == TerminationCriterion::Stress) {
		curStress = calcStress(GA, shortestPathMatrix, weightMatrix);
	}

	NodeArray<double> newX;
	NodeArray<double> newY;
	NodeArray<double> newZ;

	if (m_terminationCriterion == TerminationCriterion::PositionDifference) {
		newX.init(G);
		newY.init(G);
		if (GA.has(GraphAttributes::threeD))
			newZ.init(G);
	}
	do {
		if (m_terminationCriterion == TerminationCriterion::PositionDifference) {
			if (GA.has(GraphAttributes::threeD))
				copyLayout(GA, newX, newY, newZ);
			else copyLayout(GA, newX, newY);
		}
		nextIteration(GA, shortestPathMatrix, weightMatrix);
		if (m_terminationCriterion == TerminationCriterion::Stress) {
			prevStress = curStress;
			curStress = calcStress(GA, shortestPathMatrix, weightMatrix);
		}
	} while (!finished(GA, ++numberOfPerformedIterations, newX, newY, prevStress, curStress));

	Logger::slout() << "Iteration count:\t" << numberOfPerformedIterations
		<< "\tStress:\t" << calcStress(GA, shortestPathMatrix, weightMatrix) << std::endl;
}


void StressMinimization::nextIteration(
	GraphAttributes& GA,
	NodeArray<NodeArray<double> >& shortestPathMatrix,
	NodeArray<NodeArray<double> >& weights)
{
	const Graph& G = GA.constGraph();

	for (node v : G.nodes)
	{
		double newXCoord = 0.0;
		double newYCoord = 0.0;
		double newZCoord = 0.0;
		double& currXCoord = GA.x(v);
		double& currYCoord = GA.y(v);
		double totalWeight = 0;
		for (node w : G.nodes)
		{
			if (v == w) {
				continue;
			}
			// calculate euclidean distance between both points
			double xDiff = currXCoord - GA.x(w);
			double yDiff = currYCoord - GA.y(w);
			double zDiff = (GA.has(GraphAttributes::threeD)) ? GA.z(v) - GA.z(w) : 0.0;
			double euclideanDist = sqrt(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff);
			// get the weight
			double weight = weights[v][w];
			// get the desired distance
			double desDistance = shortestPathMatrix[v][w];
			// reset the voted x coordinate
			// if x is not fixed
			if (!m_fixXCoords) {
				double voteX = GA.x(w);
				if (euclideanDist != 0) {
					// calc the vote
					voteX += desDistance * (currXCoord - voteX) / euclideanDist;
				}
				// add the vote
				newXCoord += weight * voteX;
			}
			// reset the voted y coordinate
			// y is not fixed
			if (!m_fixYCoords) {
				double voteY = GA.y(w);
				if (euclideanDist != 0) {
					// calc the vote
					voteY += desDistance * (currYCoord - voteY) / euclideanDist;
				}
				newYCoord += weight * voteY;
			}
			if (GA.has(GraphAttributes::threeD) && !m_fixZCoords) {
				// reset the voted z coordinate
				double voteZ = GA.z(w);
				if (euclideanDist != 0) {
					// calc the vote
					voteZ += desDistance * (GA.z(v) - voteZ) / euclideanDist;
				}
				newZCoord += weight * voteZ;
			}
			// sum up the weights
			totalWeight += weight;
		}
		// update the positions
		if (totalWeight != 0) {
			if (!m_fixXCoords) {
				currXCoord = newXCoord / totalWeight;
			}
			if (!m_fixYCoords) {
				currYCoord = newYCoord / totalWeight;
			}
			if (GA.has(GraphAttributes::threeD) && !m_fixZCoords) {
				GA.z(v) = newZCoord / totalWeight;
			}
		}
	}
}


bool StressMinimization::finished(
	GraphAttributes& GA,
	int numberOfPerformedIterations,
	NodeArray<double>& prevXCoords,
	NodeArray<double>& prevYCoords,
	const double prevStress,
	const double curStress)
{
	if (numberOfPerformedIterations == m_numberOfIterations) {
		return true;
	}

	switch (m_terminationCriterion)
	{
	case TerminationCriterion::PositionDifference:
	{
		double eucNorm = 0;
		double dividend = 0;
		// compute the translation of all nodes between
		// the consecutive layouts
		for (node v : GA.constGraph().nodes)
		{
			double diffX = prevXCoords[v] - GA.x(v);
			double diffY = prevYCoords[v] - GA.y(v);
			dividend += diffX * diffX + diffY * diffY;
			eucNorm += prevXCoords[v] * prevXCoords[v] + prevYCoords[v] * prevYCoords[v];
		}
		return sqrt(dividend) / sqrt(eucNorm) < EPSILON;
	}
	case TerminationCriterion::Stress:
		return curStress == 0 || prevStress - curStress < prevStress * EPSILON;

	default:
		return false;
	}
}


void StressMinimization::initMatrices(const Graph& G,
	NodeArray<NodeArray<double> >& shortestPathMatrix,
	NodeArray<NodeArray<double> >& weightMatrix)
{
	// init shortest path matrix by infinity distances
	for (node v : G.nodes)
	{
		shortestPathMatrix[v].init(G, std::numeric_limits<double>::infinity());
		shortestPathMatrix[v][v] = 0;
		weightMatrix[v].init(G, 0);
	}
}

}
