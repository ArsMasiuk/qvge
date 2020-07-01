/** \file
 * \brief Implementation of the pivot MDS.
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

#include <ogdf/energybased/PivotMDS.h>
#include <ogdf/basic/GraphCopy.h>


namespace ogdf {

const double PivotMDS::EPSILON = 1 - 1e-10;
const double PivotMDS::FACTOR = -0.5;


void PivotMDS::call(GraphAttributes& GA)
{
	OGDF_ASSERT(isConnected(GA.constGraph()));
	OGDF_ASSERT(!m_hasEdgeCostsAttribute || GA.has(GraphAttributes::edgeDoubleWeight));
	pivotMDSLayout(GA);
}


void PivotMDS::centerPivotmatrix(Array<Array<double> >& pivotMatrix)
{
	int numberOfPivots = pivotMatrix.size();
	// this is ensured since the graph size is at least 2!
	int nodeCount = pivotMatrix[0].size();

	double normalizationFactor = 0;
	double rowColNormalizer;
	Array<double> colNormalization(numberOfPivots);

	for (int i = 0; i < numberOfPivots; i++) {
		rowColNormalizer = 0;
		for (int j = 0; j < nodeCount; j++) {
			rowColNormalizer += pivotMatrix[i][j] * pivotMatrix[i][j];
		}
		normalizationFactor += rowColNormalizer;
		colNormalization[i] = rowColNormalizer / nodeCount;
	}
	normalizationFactor = normalizationFactor / (nodeCount * numberOfPivots);
	for (int i = 0; i < nodeCount; i++) {
		rowColNormalizer = 0;
		for (int j = 0; j < numberOfPivots; j++) {
			double square = pivotMatrix[j][i] * pivotMatrix[j][i];
			pivotMatrix[j][i] = square + normalizationFactor
					- colNormalization[j];
			rowColNormalizer += square;
		}
		rowColNormalizer /= numberOfPivots;
		for (int j = 0; j < numberOfPivots; j++) {
			pivotMatrix[j][i] = FACTOR * (pivotMatrix[j][i] - rowColNormalizer);
		}
	}
}


void PivotMDS::pivotMDSLayout(GraphAttributes& GA)
{
	const Graph& G = GA.constGraph();
	bool use3D = GA.has(GraphAttributes::threeD) && DIMENSION_COUNT > 2;

	const int n = G.numberOfNodes();

	// trivial cases
	if (n == 0)
		return;

	if (n == 1) {
		node v1 = G.firstNode();
		GA.x(v1) = 0.0;
		GA.y(v1) = 0.0;
		if (use3D)
			GA.z(v1) = 0.0;
		return;
	}

	// check whether the graph is a path or not
	const node head = getRootedPath(G);
	if (head != nullptr) {
		doPathLayout(GA, head);
	}
	else {
		Array<Array<double> > pivDistMatrix;
		// compute the pivot matrix
		getPivotDistanceMatrix(GA, pivDistMatrix);
		// center the pivot matrix
		centerPivotmatrix(pivDistMatrix);
		// init the coordinate matrix
		Array<Array<double> > coord(DIMENSION_COUNT);
		for (auto &elem : coord) {
			elem.init(n);
		}
		// init the eigen values array
		Array<double> eVals(DIMENSION_COUNT);
		singularValueDecomposition(pivDistMatrix, coord, eVals);
		// compute the correct aspect ratio
		for (int i = 0; i < coord.size(); i++) {
			eVals[i] = sqrt(eVals[i]);
			for (int j = 0; j < n; j++) {
				coord[i][j] *= eVals[i];
			}
		}
		// set the new positions to the graph
		int i = 0;
		for (node v : G.nodes)
		{
			GA.x(v) = coord[0][i];
			GA.y(v) = coord[1][i];
			if (use3D){
				GA.z(v) = coord[2][i];//std::cout << coord[2][i] << "\n";
			}
			++i;
		}
	}
}


void PivotMDS::doPathLayout(GraphAttributes& GA, const node& v)
{
	double xPos = 0;
	node prev = nullptr;
	node oldCur = nullptr;
	node cur = v;
	// since the given node is the beginning of the path just
	// use bfs and increment the x coordinate by the average
	// edge costs.
	do {
		oldCur = cur;
		GA.x(cur) = xPos;
		GA.y(cur) = 0;
		for(adjEntry adj : cur->adjEntries) {
			node w = adj->twinNode();
			// Ignore multi-edges and self-loops.
			if (w != prev && w != cur) {
				prev = cur;
				cur = w;
				if(m_hasEdgeCostsAttribute) {
					xPos+=GA.doubleWeight(adj->theEdge());
				} else {
					xPos += m_edgeCosts;
				}
				break;
			}
		}
	} while (cur != oldCur);
}


void PivotMDS::eigenValueDecomposition(
	Array<Array<double> >& K,
	Array<Array<double> >& eVecs,
	Array<double>& eValues)
{
	randomize(eVecs);
	const int p = K.size();
	double r = 0;
	for (int i = 0; i < DIMENSION_COUNT; i++) {
		eValues[i] = normalize(eVecs[i]);
	}
	while (r < EPSILON) {
		if (std::isnan(r) || isinf(r)) {
			// Throw arithmetic exception (Shouldn't occur
			// for DIMEMSION_COUNT = 2
			OGDF_THROW(AlgorithmFailureException);
			return;
		}
		// remember prev values
		Array<Array<double> > tmpOld(DIMENSION_COUNT);
		for (int i = 0; i < DIMENSION_COUNT; i++) {
			tmpOld[i].init(p);
			for (int j = 0; j < p; j++) {
				tmpOld[i][j] = eVecs[i][j];
				eVecs[i][j] = 0;
			}
		}
		// multiply matrices
		for (int i = 0; i < DIMENSION_COUNT; i++) {
			for (int j = 0; j < p; j++) {
				for (int k = 0; k < p; k++) {
					eVecs[i][k] += K[j][k] * tmpOld[i][j];
				}
			}
		}
		// orthogonalize
		for (int i = 0; i < DIMENSION_COUNT; i++) {
			for (int j = 0; j < i; j++) {
				double fac = prod(eVecs[j], eVecs[i])
						/ prod(eVecs[j], eVecs[j]);
				for (int k = 0; k < p; k++) {
					eVecs[i][k] -= fac * eVecs[j][k];
				}
			}
		}
		// normalize
		for (int i = 0; i < DIMENSION_COUNT; i++) {
			eValues[i] = normalize(eVecs[i]);
		}
		r = 1;
		for (int i = 0; i < DIMENSION_COUNT; i++) {
			// get absolute value (abs only defined for int)
			double tmp = prod(eVecs[i], tmpOld[i]);
			if (tmp < 0) {
				tmp *= -1;
			}
			Math::updateMin(r, tmp);
		}
	}
}


void PivotMDS::getPivotDistanceMatrix(
	const GraphAttributes& GA,
	Array<Array<double> >& pivDistMatrix)
{
	const Graph& G = GA.constGraph();
	const int n = G.numberOfNodes();

	// lower the number of pivots if necessary
	int numberOfPivots = min(n, m_numberOfPivots);
	// number of pivots times n matrix used to store the graph distances
	pivDistMatrix.init(numberOfPivots);
	for (int i = 0; i < numberOfPivots; i++) {
		pivDistMatrix[i].init(n);
	}
	// edges costs array
	EdgeArray<double> edgeCosts;
	bool hasEdgeCosts = false;
	// already checked whether this attribute exists or not (see call method)
	if (m_hasEdgeCostsAttribute) {
		edgeCosts.init(G);
		for(edge e : G.edges)
		{
			edgeCosts[e] = GA.doubleWeight(e);
		}
		hasEdgeCosts = true;
	}
	// used for min-max strategy
	NodeArray<double> minDistances(G, std::numeric_limits<double>::infinity());
	NodeArray<double> shortestPathSingleSource(G);
	// the current pivot node
	node pivNode = G.firstNode();
	for (int i = 0; i < numberOfPivots; i++) {
		// get the shortest path from the currently processed pivot node to
		// all other nodes in the graph
		shortestPathSingleSource.fill(std::numeric_limits<double>::infinity());
		if (hasEdgeCosts) {
			dijkstra_SPSS(pivNode, G, shortestPathSingleSource, edgeCosts);
		} else {
			bfs_SPSS(pivNode, G, shortestPathSingleSource, m_edgeCosts);
		}
		copySPSS(pivDistMatrix[i], shortestPathSingleSource);
		// update the pivot and the minDistances array ... to ensure the
		// correctness set minDistance of the pivot node to zero
		minDistances[pivNode] = 0;
		for(node v : G.nodes)
		{
			Math::updateMin(minDistances[v], shortestPathSingleSource[v]);
			if (minDistances[v] > minDistances[pivNode]) {
				pivNode = v;
			}
		}
	}
}


void PivotMDS::copySPSS(Array<double>& copyTo, NodeArray<double>& copyFrom)
{
	const Graph &G = *copyFrom.graphOf();

	int i = 0;
	for (node v : G.nodes) {
		copyTo[i++] = copyFrom[v];
	}
}


node PivotMDS::getRootedPath(const Graph& G)
{
	GraphCopy GC(G);
	makeSimpleUndirected(GC);
	node head = nullptr;
	int numDegree1 = 0;
	int numDegree2 = 0;

	for (node v : GC.nodes) {
		if (v->degree() == 2) {
			numDegree2++;
		} else if (v->degree() == 1) {
			head = v;
			numDegree1++;
		} else {
			return nullptr;
		}
	}

	// Given n >= 2 (as guaranteed by pivotMDSLayout()),
	// a path has two nodes with degree 1 and n-2 nodes with degree 2.
	return numDegree1 == 2 && numDegree2 == GC.numberOfNodes() - 2 ?
		   GC.original(head) : nullptr;
}


double PivotMDS::normalize(Array<double>& x)
{
	double norm = sqrt(prod(x, x));
	if (norm != 0) {
		for (auto &elem : x) {
			elem /= norm;
		}
	}
	return norm;
}


double PivotMDS::prod(const Array<double>& x, const Array<double>& y)
{
	double result = 0;
	for (int i = 0; i < x.size(); i++) {
		result += x[i] * y[i];
	}
	return result;
}


void PivotMDS::randomize(Array<Array<double> >& matrix)
{
	srand(SEED);
	for (auto &elem : matrix) {
		for (int j = 0; j < elem.size(); j++) {
			elem[j] = ((double) rand()) / RAND_MAX;
		}
	}
}


void PivotMDS::selfProduct(const Array<Array<double> >& d, Array<Array<double> >& result)
{
	double sum;
	for (int i = 0; i < d.size(); i++) {
		for (int j = 0; j <= i; j++) {
			sum = 0;
			for (int k = 0; k < d[0].size(); k++) {
				sum += d[i][k] * d[j][k];
			}
			result[i][j] = sum;
			result[j][i] = sum;
		}
	}
}


void PivotMDS::singularValueDecomposition(
	Array<Array<double> >& pivDistMatrix,
	Array<Array<double> >& eVecs,
	Array<double>& eVals)
{
	const int size = pivDistMatrix.size();
	const int n = pivDistMatrix[0].size();
	Array<Array<double> > K(size);
	for (int i = 0; i < size; i++) {
		K[i].init(size);
	}
	// calc C^TC
	selfProduct(pivDistMatrix, K);

	Array<Array<double> > tmp(DIMENSION_COUNT);
	for (int i = 0; i < DIMENSION_COUNT; i++) {
		tmp[i].init(size);
	}

	eigenValueDecomposition(K, tmp, eVals);

	// C^Tx
	for (int i = 0; i < DIMENSION_COUNT; i++) {
		eVals[i] = sqrt(eVals[i]);
		for (int j = 0; j < n; j++) { // node j
			eVecs[i][j] = 0;
			for (int k = 0; k < size; k++) { // pivot k
				eVecs[i][j] += pivDistMatrix[k][j] * tmp[i][k];
			}
		}
	}
	for (int i = 0; i < DIMENSION_COUNT; i++) {
		normalize(eVecs[i]);
	}
}

}
