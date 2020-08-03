/** \file
 * \brief Declaration of class NodeRespecterLayout.
 *
 * Force-directed layout algorithm respecting the height and width of nodes,
 * aiming to minimize node overlaps as well as edges crossing through
 * non-incident nodes.
 *
 * \author Max Ilsen
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
#include <ogdf/basic/GraphCopy.h>

namespace ogdf {

//! The NodeRespecterLayout layout algorithm.
/**
 * @ingroup gd-energy
 *
 * This is a force-directed layout algorithm respecting the shapes and sizes of
 * nodes. It aims to minimize the number of node overlaps as well as the number
 * of edges crossing through non-incident nodes. In order to achieve this, the
 * algorithm adapts its forces to the node sizes and bends edges around close-by
 * nodes. The edge bends are created by introducing dummy nodes into the graph,
 * positioning all nodes according to forces acting upon them, filtering out
 * unnecessary dummy nodes, and then replacing the remaining dummy nodes by edge
 * bends.
 *
 * The algorithm is documented in and was developed for the bachelor thesis:
 * Max Ilsen: <i>Energy-Based %Layout Algorithms for Graphs with Large Nodes</i>.
 * University of Osnabrueck, 2017
 *
 * The following parameters can be set:
 * <table>
 *   <tr>
 *     <th><i>Parameter</i><th><i>Default Value</i>
 *   </tr>
 *     <tr><td> #m_randomInitialPlacement </td><td> true </td></tr>
 *     <tr><td> #m_postProcessing </td><td> PostProcessingMode::Complete </td></tr>
 *     <tr><td> #m_bendNormalizationAngle </td><td> Math::pi </td></tr>
 *     <tr><td> #m_numberOfIterations </td><td> 30000 </td></tr>
 *     <tr><td> #m_minimalTemperature </td><td> 1.0 </td></tr>
 *     <tr><td> #m_initialTemperature </td><td> 10.0 </td></tr>
 *     <tr><td> #m_temperatureDecreaseOffset </td><td> 0.0 </td></tr>
 *     <tr><td> #m_gravitation </td><td> 1.0/16.0 </td></tr>
 *     <tr><td> #m_oscillationAngle </td><td> Math::pi_2 </td></tr>
 *     <tr><td> #m_desiredMinEdgeLength </td><td> LayoutStandards::defaultNodeSeparation() </td></tr>
 *     <tr><td> #m_initDummiesPerEdge </td><td> 1 </td></tr>
 *     <tr><td> #m_maxDummiesPerEdge </td><td> 3 </td></tr>
 *     <tr><td> #m_dummyInsertionThreshold </td><td> 5 </td></tr>
 *     <tr><td> #m_maxDisturbance </td><td> 0 </td></tr>
 *     <tr><td> #m_repulsionDistance </td><td> 2*#m_desiredMinEdgeLength </td></tr>
 *     <tr><td> #m_minDistCC </td><td> LayoutStandards::defaultCCSeparation() </td></tr>
 *     <tr><td> #m_pageRatio </td><td> 1.0 </td></tr>
 *   </tr>
 * </table>
 */
class OGDF_EXPORT NodeRespecterLayout : public LayoutModule
{

public:
	//! Creates an instance of the NodeRespecterLayout.
	NodeRespecterLayout();

	//! Destroys an instance of the NodeRespecterLayout.
	~NodeRespecterLayout(){}

	//! Calls the layout algorithm for the GraphAttributes \p attr.
	virtual void call(GraphAttributes &attr) override;

	//! Sets whether unnecessary edge bends should be filtered out in a
	//! post-processing step.
	enum class PostProcessingMode {
		None,               //!< Keep all bends.
		KeepMultiEdgeBends, //!< Activate post-processing but keep all bends on
		                    //!< multi-edges and self-loops (such that the
		                    //!< corresponding edges are visible).
		Complete            //!< Activate post-processing: Remove all bends that
		                    //!< do not prevent edge-node intersections.
	};

	//! \name Setters for Algorithm Parameters
	//! @{

	//! Sets #m_randomInitialPlacement to \p randomInitialPlacement.
	void setRandomInitialPlacement(bool randomInitialPlacement);

	//! Sets #m_postProcessing to \p postProcessing.
	void setPostProcessing(PostProcessingMode postProcessing);

	//! Sets #m_bendNormalizationAngle to \p bendNormalizationAngle in [0...Pi].
	void setBendNormalizationAngle(double bendNormalizationAngle);

	//! Sets #m_numberOfIterations to \p numberOfIterations >= 0.
	void setNumberOfIterations(int numberOfIterations);

	//! Sets #m_minimalTemperature to \p minimalTemperature >= 0.
	void setMinimalTemperature(double minimalTemperature);

	//! Sets #m_initialTemperature to \p initialTemperature >
	//! #m_minimalTemperature.
	void setInitialTemperature(double initialTemperature);

	//! Sets #m_temperatureDecreaseOffset to \p temperatureDecreaseOffset in
	//! [0...1].
	void setTemperatureDecreaseOffset(double temperatureDecreaseOffset);

	//! Sets #m_gravitation to \p gravitation >= 0.
	void setGravitation(double gravitation);

	//! Sets #m_oscillationAngle to \p oscillationAngle in [0...Pi].
	void setOscillationAngle(double oscillationAngle);

	//! Sets #m_desiredMinEdgeLength to \p desiredMinEdgeLength > 0.
	void setDesiredMinEdgeLength(double desiredMinEdgeLength);

	//! Sets #m_initDummiesPerEdge to \p initDummiesPerEdge >= 0.
	void setInitDummiesPerEdge(int initDummiesPerEdge);

	//! Sets #m_maxDummiesPerEdge to \p maxDummiesPerEdge >
	//! #m_initDummiesPerEdge.
	void setMaxDummiesPerEdge(int maxDummiesPerEdge);

	//! Sets #m_dummyInsertionThreshold to \p dummyInsertionThreshold >= 1.
	void setDummyInsertionThreshold(double dummyInsertionThreshold);

	//! Sets #m_maxDisturbance to \p maxDisturbance >= 0.
	void setMaxDisturbance(double maxDisturbance);

	//! Sets #m_repulsionDistance to \p repulsionDistance >= 0.
	void setRepulsionDistance(double repulsionDistance);

	//! Sets #m_minDistCC to \p minDistCC >= 0.
	void setMinDistCC(double minDistCC);

	//! Sets #m_pageRatio to \p pageRatio > 0.
	void setPageRatio(double pageRatio);

	//! @}
	//! \name Getters for Algorithm Parameters
	//! @{

	//! Returns #m_randomInitialPlacement.
	bool getRandomInitialPlacement() const { return m_randomInitialPlacement; }

	//! Returns #m_postProcessing.
	PostProcessingMode getPostProcessing() const { return m_postProcessing; }

	//! Returns #m_bendNormalizationAngle.
	double getBendNormalizationAngle() const { return m_bendNormalizationAngle; }

	//! Returns #m_numberOfIterations.
	int getNumberOfIterations() const { return m_numberOfIterations; }

	//! Returns #m_minimalTemperature.
	double getMinimalTemperature() const { return m_minimalTemperature; }

	//! Returns #m_initialTemperature.
	double getInitialTemperature() const { return m_initialTemperature; }

	//! Returns #m_temperatureDecreaseOffset.
	double getTemperatureDecreaseOffset() const { return m_temperatureDecreaseOffset; }

	//! Returns #m_gravitation.
	double getGravitation() const { return m_gravitation; }

	//! Returns #m_oscillationAngle.
	double getOscillationAngle() const { return m_oscillationAngle; }

	//! Returns #m_desiredMinEdgeLength.
	double getDesiredMinEdgeLength() const { return m_desiredMinEdgeLength; }

	//! Returns #m_initDummiesPerEdge.
	int getInitDummiesPerEdge() const { return m_initDummiesPerEdge; }

	//! Returns #m_maxDummiesPerEdge.
	int getMaxDummiesPerEdge() const { return m_maxDummiesPerEdge; }

	//! Returns #m_dummyInsertionThreshold.
	double getDummyInsertionThreshold() const { return m_dummyInsertionThreshold; }

	//! Returns #m_maxDisturbance.
	double getMaxDisturbance() const { return m_maxDisturbance; }

	//! Returns #m_repulsionDistance.
	double getRepulsionDistance() const { return m_repulsionDistance; }

	//! Returns #m_minDistCC.
	double getMinDistCC() const { return m_minDistCC; }

	//! Returns #m_pageRatio.
	double getPageRatio() const { return m_pageRatio; }

	//! @}

private:
	//! \name Algorithm Parameters
	//! @{

	//! Whether nodes should be initialized in random positions.
	bool m_randomInitialPlacement;

	//! Whether unnecessary bends should be filtered out in a post processing step.
	PostProcessingMode m_postProcessing;

	//! Lower bound for the minimum angle between two line segments such that
	//! the bend point between them is still removed.
	double m_bendNormalizationAngle;

	//! Number of times a single node is moved for each connected component.
	int m_numberOfIterations;

	//! Minimal temperature, lower bound for the global temperature.
	double m_minimalTemperature;

	//! Initial temperature of every node.
	double m_initialTemperature;

	//! Factor for which holds: If only #m_numberOfIterations *
	//! #m_temperatureDecreaseOffset iterations are left, the global temperature
	//! starts to be decreased linearly.
	double m_temperatureDecreaseOffset;

	//! Gravitational constant scaling attractive forces towards the barycenter.
	double m_gravitation;

	//! Maximum angle between new and previous impulse such that the node
	//! movement is counted as an oscillation.
	double m_oscillationAngle;

	//! Desired minimal node separation/edge length.
	double m_desiredMinEdgeLength;

	//! How many dummy nodes should initially be created for one edge.
	int m_initDummiesPerEdge;

	//! How many dummy nodes should maximally be created for one edge.
	int m_maxDummiesPerEdge;

	//! How many times larger than the desired edge length an edge has to be in
	//! order for a new dummy node to be created by splitting said edge.
	double m_dummyInsertionThreshold;

	//! Maximal disturbance, i.e. maximal random node movement.
	double m_maxDisturbance;

	//! Maximum distance between a dummy and another node such that the former
	//! is repulsed by the latter.
	double m_repulsionDistance;

	//! Minimal distance between connected components.
	double m_minDistCC;

	//! Page ratio used for the layout of connected components.
	double m_pageRatio;

	//! @}
	//! \name Graph Data Used by the Algorithm
	//! @{

	//! Copy of the given graph which may contain dummy nodes.
	GraphCopy m_copy;

	//! GraphAttributes for #m_copy.
	GraphAttributes m_copyAttr;

	//! X-coordinate of the last impulse of the node.
	NodeArray<double> m_impulseX;

	//! Y-coordinate of the last impulse of the node.
	NodeArray<double> m_impulseY;

	//! Local temperature of the node.
	NodeArray<double> m_localTemperature;

	//! Radius of the smallest circle encompassing the node.
	NodeArray<double> m_nodeRadius;

	//! Whether the edge has parallel edges.
	EdgeArray<bool> m_hasParEdges;

	//! Desired distance between each pair of nodes.
	NodeArray<NodeArray<double>> m_desiredDistance;

	//! @}
	//! \name Other Data Used by the Algorithm
	//! @{

	//! Twice the number of all edges in the original graph.
	int m_degreeSum;

	//! Weighted sum of x-coordinates of all nodes.
	double m_barycenterX;

	//! Weighted sum of y-coordinates of all nodes.
	double m_barycenterY;

	//! Number of iterations for which the algorithm still has to run.
	int m_iterCounter;

	//! Average of all local node temperatures.
	double m_globalTemperature;

	//! Precomputed constant used to get the max. temperature for each iteration.
	double m_factor;

	//! Precomputed cosinus of (#m_oscillationAngle / 2).
	double m_cos;

	//! @}

	//! Initializes all graph data used by the algorithm.
	void initData();

	//! Frees all graph data used by the algorithm.
	void freeData();

	//! Creates bends in \p attr at the coordinates of dummy nodes in \p copy.
	void createBends(const ArrayBuffer<edge> &origEdges, GraphAttributes &attr);

	//! Computes new impulses and updates positions for random permutations
	//! of \p nodes until #m_numberOfIterations iterations are over or
	//! #m_globalTemperature drops below #m_minimalTemperature.
	void updateNodeLoop(SListPure<node> &nodes);

	//! Returns the new impulse for node \p v.
	std::pair<double,double> computeImpulse(node v);

	//! Updates the node data for node \p v using \p newImpulse as the x- and
	//! y-coordinate of the new impulse onto \p v.
	void updateNode(node v, std::pair<double,double> newImpulse);

	//! Adds dummy nodes to incident edges of \p v if they are too long.
	void addDummies(node v, SListPure<node> &nodes);

	//! Returns the radius of the smallest circle surrounding the shape of \p v
	//! (while still having its center at the position of \p v).
	inline double radius(const GraphAttributes &attr, node v) const {
		switch (attr.shape(v)) {
			case Shape::Pentagon:
			case Shape::Octagon:
			case Shape::Hexagon:
			case Shape::Rhomb:
			case Shape::Ellipse:
				return std::max(attr.height(v), attr.width(v)) / 2.0;

			default:
				// For Rect, RoundedRect, Triangle, Trapeze, Parallelogram, InvTriangle,
				// InvTrapeze, InvParallelogram, Image and unknown shapes.
				return std::hypot(attr.height(v), attr.width(v)) / 2.0;
		}
	}

	//! Returns whether \p v and \p w belong to the same original edge. If only
	//! one of the nodes is a dummy node, returns whether its original edge is
	//! incident to the other node. If none of the nodes is a dummy node,
	//! returns false.
	inline bool haveSameOriginalEdge(node v, node w) const {
		if (m_copy.isDummy(v) && m_copy.isDummy(w)) {
			return m_copy.original(v->firstAdj()->theEdge()) ==
			       m_copy.original(w->firstAdj()->theEdge());
		} else if (m_copy.isDummy(v)) {
			return m_copy.original(v->firstAdj()->theEdge())->isIncident(w);
		} else if (m_copy.isDummy(w)) {
			return m_copy.original(w->firstAdj()->theEdge())->isIncident(v);
		} else {
			return false;
		}
	}

	//! Returns the weight of node \p v according to its degree.
	inline double weight(node v) const {
		return v->degree() / m_degreeSum;
	}

	OGDF_NEW_DELETE
};

}
