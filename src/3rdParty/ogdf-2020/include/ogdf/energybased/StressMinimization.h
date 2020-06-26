/** \file
 * \brief Declration of stress minimized layout based on majorization.
 * It can be applied to connected as well as unconnected graphs. If
 * the graph is disconnected either the infinite distances will be
 * replaced by the average edge costs times sqrt(n) or the components
 * will be processed separately.
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

#pragma once

#include <ogdf/basic/LayoutModule.h>
#include <ogdf/energybased/PivotMDS.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/graphalg/ShortestPathAlgorithms.h>
#include <ogdf/packing/ComponentSplitterLayout.h>

namespace ogdf {

//! Energy-based layout using stress minimization.
/**
 * @ingroup gd-energy
 */
class OGDF_EXPORT StressMinimization: public LayoutModule {

public:
	enum class TerminationCriterion {
		None, PositionDifference, Stress
	};

	//! Constructor: Constructs instance of stress majorization.
	StressMinimization() :
			m_hasEdgeCostsAttribute(false), m_hasInitialLayout(false), m_numberOfIterations(
					200), m_edgeCosts(100), m_avgEdgeCosts(-1), m_componentLayout(
					false), m_terminationCriterion(TerminationCriterion::None), m_fixXCoords(false), m_fixYCoords(
					false), m_fixZCoords(false) {
	}

	//! Destructor.
	~StressMinimization() {
	}

	//! Calls the layout algorithm with uniform edge costs.
	virtual void call(GraphAttributes& GA) override;

	//! Tells whether the current layout should be used or the initial layout
	//! needs to be computed.
	inline void hasInitialLayout(bool hasInitialLayout);

	//! Tells whether the x coordinates are allowed to be modified or not.
	inline void fixXCoordinates(bool fix);

	//! Tells whether the y coordinates are allowed to be modified or not.
	inline void fixYCoordinates(bool fix);

	//! Tells whether the z coordinates are allowed to be modified or not.
	inline void fixZCoordinates(bool fix);

	//! Sets whether the graph's components should be layouted separately or a dummy
	//! distance should be used for nodes within different components.
	inline void layoutComponentsSeparately(bool separate);

	//! Sets the desired distance between adjacent nodes. If the new value is smaller or equal
	//! 0 the default value (100) is used.
	inline void setEdgeCosts(double edgeCosts);

	//! Sets a fixed number of iterations for stress majorization. If the new value is smaller or equal
	//! 0 the default value (200) is used.
	inline void setIterations(int numberOfIterations);

	//! Tells which TerminationCriterion should be used
	inline void convergenceCriterion(TerminationCriterion criterion);

	//! Tells whether the edge costs are uniform or defined by some edge costs attribute.
	inline void useEdgeCostsAttribute(bool useEdgeCostsAttribute);

private:

	//! Convergence constant.
	const static double EPSILON;

	//! Default number of pivots used for the initial Pivot-MDS layout
	const static int DEFAULT_NUMBER_OF_PIVOTS;

	//! Tells whether the stress minimization is based on uniform edge costs or a
	//! edge costs attribute
	bool m_hasEdgeCostsAttribute;

	//! Tells whether an initial layout has to be computed or not.
	bool m_hasInitialLayout;

	//! Number of iterations performed by the stress minimization.
	int m_numberOfIterations;

	//! The weight of an edge.
	double m_edgeCosts;

	//! The average edge costs. Needed to define distances of nodes belonging to
	//! different graph components.
	double m_avgEdgeCosts;

	//! Indicates whether the components should be treated separately.
	bool m_componentLayout;

	//! Indicates whether epsilon convergence is used or not.
	TerminationCriterion m_terminationCriterion;

	//! Indicates whether the x coordinates will be modified or not.
	bool m_fixXCoords;

	//! Indicates whether the y coordinates will be modified or not.
	bool m_fixYCoords;

	//! Indicates whether the z coordinates will be modified or not.
	bool m_fixZCoords;

	//! Calculates the stress for the given layout
	double calcStress(const GraphAttributes& GA,
			NodeArray<NodeArray<double> >& shortestPathMatrix,
			NodeArray<NodeArray<double> >& weightMatrix);

	//! Runs the stress for a given Graph, shortest path and weight matrix.
	void call(GraphAttributes& GA,
			NodeArray<NodeArray<double> >& shortestPathMatrix,
			NodeArray<NodeArray<double> >& weightMatrix);

	//! Calculates the weight matrix of the shortest path matrix. This is done by w_ij = s_ij^{-2}
	void calcWeights(const Graph& G,
			NodeArray<NodeArray<double> >& shortestPathMatrix,
			NodeArray<NodeArray<double> >& weightMatrix);

	//! Calculates the intial layout of the graph if necessary.
	void computeInitialLayout(GraphAttributes& GA);

	//! Convenience method copying the layout of the graph in case of epsilon convergence.
	void copyLayout(const GraphAttributes& GA, NodeArray<double>& newX,
			NodeArray<double>& newY);

	//! Convenience method copying the layout of the graph in case of epsilon convergence for 3D.
	void copyLayout(const GraphAttributes& GA, NodeArray<double>& newX,
			NodeArray<double>& newY, NodeArray<double>& newZ);

	//! Checks for epsilon convergence and whether the performed number of iterations
	//! exceed the predefined maximum number of iterations.
	bool finished(GraphAttributes& GA, int numberOfPerformedIterations,
			NodeArray<double>& prevXCoords, NodeArray<double>& prevYCoords,
			const double prevStress, const double curStress);

	//! Convenience method to initialize the matrices.
	void initMatrices(const Graph& G,
			NodeArray<NodeArray<double> >& shortestPathMatrix,
			NodeArray<NodeArray<double> >& weightMatrix);

	//! Minimizes the stress for each component separately given
	//! the shortest path matrix and the weight matrix.
	void minimizeStress(GraphAttributes& GA,
			NodeArray<NodeArray<double> >& shortestPathMatrix,
			NodeArray<NodeArray<double> >& weightMatrix);

	//! Runs the next iteration of the stress minimization process. Note that serial update
	//! is used.
	void nextIteration(GraphAttributes& GA,
			NodeArray<NodeArray<double> >& shortestPathMatrix,
			NodeArray<NodeArray<double> >& weightMatrix);

	//! Replaces infinite distances to the given value
	void replaceInfinityDistances(
			NodeArray<NodeArray<double> >& shortestPathMatrix, double newVal);

}
;

void StressMinimization::fixXCoordinates(bool fix) {
	m_fixXCoords = fix;
}

void StressMinimization::fixYCoordinates(bool fix) {
	m_fixYCoords = fix;
}

void StressMinimization::hasInitialLayout(bool hasInitialLayout) {
	m_hasInitialLayout = hasInitialLayout;
}

void StressMinimization::layoutComponentsSeparately(bool separate) {
	m_componentLayout = separate;
}

void StressMinimization::setEdgeCosts(double edgeCosts) {
	m_edgeCosts = (edgeCosts > 0) ? edgeCosts : 100;
}

void StressMinimization::setIterations(int numberOfIterations) {
	m_numberOfIterations = (numberOfIterations > 0) ? numberOfIterations : 100;
}

void StressMinimization::convergenceCriterion(TerminationCriterion criterion) {
	m_terminationCriterion = criterion;
}

void StressMinimization::useEdgeCostsAttribute(bool useEdgeCostsAttribute) {
	m_hasEdgeCostsAttribute = useEdgeCostsAttribute;
}

}
