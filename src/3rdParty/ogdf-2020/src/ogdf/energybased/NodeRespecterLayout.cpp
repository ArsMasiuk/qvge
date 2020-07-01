/** \file
 * \brief Implementation of class NodeRespecterLayout.
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

#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/energybased/NodeRespecterLayout.h>
#include <ogdf/packing/TileToRowsCCPacker.h>

//#define OGDF_NODERESPECTERLAYOUT_DEBUG

namespace ogdf {

NodeRespecterLayout::NodeRespecterLayout()
	: m_randomInitialPlacement(true)
	, m_postProcessing(PostProcessingMode::Complete)
	, m_bendNormalizationAngle(Math::pi)
	, m_numberOfIterations(30000)
	, m_minimalTemperature(1.0)
	, m_initialTemperature(10.0)
	, m_temperatureDecreaseOffset(0.0)
	, m_gravitation(1.0/16.0)
	, m_oscillationAngle(Math::pi_2)
	, m_desiredMinEdgeLength(LayoutStandards::defaultNodeSeparation())
	, m_initDummiesPerEdge(1)
	, m_maxDummiesPerEdge(3)
	, m_dummyInsertionThreshold(5)
	, m_maxDisturbance(0)
	, m_repulsionDistance(2*m_desiredMinEdgeLength)
	, m_minDistCC(LayoutStandards::defaultCCSeparation())
	, m_pageRatio(1.0)
    { }

void NodeRespecterLayout::setRandomInitialPlacement(bool randomInitialPlacement)
{
	m_randomInitialPlacement = randomInitialPlacement;
}

void NodeRespecterLayout::setPostProcessing(PostProcessingMode postProcessing)
{
	m_postProcessing = postProcessing;
}

void NodeRespecterLayout::setBendNormalizationAngle(double bendNormalizationAngle)
{
	OGDF_ASSERT(OGDF_GEOM_ET.geq(bendNormalizationAngle, 0.0));
	OGDF_ASSERT(OGDF_GEOM_ET.leq(bendNormalizationAngle, Math::pi));
	m_bendNormalizationAngle = bendNormalizationAngle;
}

void NodeRespecterLayout::setNumberOfIterations(int numberOfIterations)
{
	OGDF_ASSERT(numberOfIterations >= 0);
	m_numberOfIterations = numberOfIterations;
}

void NodeRespecterLayout::setMinimalTemperature(double minimalTemperature)
{
	OGDF_ASSERT(OGDF_GEOM_ET.geq(minimalTemperature, 0.0));
	m_minimalTemperature = minimalTemperature;
}

void NodeRespecterLayout::setInitialTemperature(double initialTemperature)
{
	OGDF_ASSERT(OGDF_GEOM_ET.greater(initialTemperature, m_minimalTemperature));
	m_initialTemperature = initialTemperature;
}

void NodeRespecterLayout::setTemperatureDecreaseOffset(double temperatureDecreaseOffset)
{
	OGDF_ASSERT(OGDF_GEOM_ET.geq(temperatureDecreaseOffset, 0.0));
	OGDF_ASSERT(OGDF_GEOM_ET.leq(temperatureDecreaseOffset, 1.0));
	m_temperatureDecreaseOffset = temperatureDecreaseOffset;
}

void NodeRespecterLayout::setGravitation(double gravitation)
{
	OGDF_ASSERT(OGDF_GEOM_ET.geq(gravitation, 0.0));
	m_gravitation = gravitation;
}

void NodeRespecterLayout::setOscillationAngle(double oscillationAngle)
{
	OGDF_ASSERT(OGDF_GEOM_ET.geq(oscillationAngle, 0.0));
	OGDF_ASSERT(OGDF_GEOM_ET.leq(oscillationAngle, Math::pi));
	m_oscillationAngle = oscillationAngle;
}

void NodeRespecterLayout::setDesiredMinEdgeLength(double desiredMinEdgeLength)
{
	OGDF_ASSERT(OGDF_GEOM_ET.geq(desiredMinEdgeLength, 0.0));
	m_desiredMinEdgeLength = desiredMinEdgeLength;
}

void NodeRespecterLayout::setInitDummiesPerEdge(int initDummiesPerEdge)
{
	OGDF_ASSERT(initDummiesPerEdge >= 0);
	m_initDummiesPerEdge = initDummiesPerEdge;
}

void NodeRespecterLayout::setMaxDummiesPerEdge(int maxDummiesPerEdge)
{
	OGDF_ASSERT(maxDummiesPerEdge >= m_initDummiesPerEdge);
	m_maxDummiesPerEdge = maxDummiesPerEdge;
}

void NodeRespecterLayout::setDummyInsertionThreshold(double dummyInsertionThreshold)
{
	OGDF_ASSERT(OGDF_GEOM_ET.geq(dummyInsertionThreshold, 1.0));
	m_dummyInsertionThreshold = dummyInsertionThreshold;
}

void NodeRespecterLayout::setMaxDisturbance(double maxDisturbance)
{
	OGDF_ASSERT(OGDF_GEOM_ET.geq(maxDisturbance, 0.0));
	m_maxDisturbance = maxDisturbance;
}

void NodeRespecterLayout::setRepulsionDistance(double repulsionDistance)
{
	OGDF_ASSERT(OGDF_GEOM_ET.geq(repulsionDistance, 0.0));
	m_repulsionDistance = repulsionDistance;
}

void NodeRespecterLayout::setMinDistCC(double minDistCC)
{
	OGDF_ASSERT(OGDF_GEOM_ET.geq(minDistCC, 0.0));
	m_minDistCC = minDistCC;
}

void NodeRespecterLayout::setPageRatio(double pageRatio)
{
	OGDF_ASSERT(OGDF_GEOM_ET.greater(pageRatio, 0.0));
	m_pageRatio = pageRatio;
}

void NodeRespecterLayout::initData()
{
	m_impulseX.init(m_copy, 0);
	m_impulseY.init(m_copy, 0);
	m_localTemperature.init(m_copy, m_initialTemperature);
	m_nodeRadius.init(m_copy, 0.0);
	m_desiredDistance.init(m_copy);
	m_degreeSum = m_copy.numberOfEdges() == 0 ? 1 : m_copy.numberOfEdges() * 2;
	m_barycenterX = 0;
	m_barycenterY = 0;
	m_iterCounter = m_numberOfIterations;
	m_globalTemperature = m_initialTemperature;
	m_factor = m_temperatureDecreaseOffset <= 0.0 ? 0.0 :
		(m_initialTemperature - m_minimalTemperature) /
		(m_numberOfIterations * m_temperatureDecreaseOffset);
	m_cos = cos(m_oscillationAngle / 2.0);
}

void NodeRespecterLayout::freeData()
{
	m_impulseX.init();
	m_impulseY.init();
	m_localTemperature.init();
	m_nodeRadius.init();
	m_desiredDistance.init();
}


void NodeRespecterLayout::createBends(const ArrayBuffer<edge> &origEdges, GraphAttributes &attr)
{
	DPoint inter;

	// Get bounding rectangles of all nodes.
	NodeArray<DRect> nodeRects(m_copy);
	if (m_postProcessing != PostProcessingMode::None) {
		m_copyAttr.nodeBoundingBoxes(nodeRects);
	}

	auto toSegment = [&](node v, node w) {
		return DSegment(m_copyAttr.x(v), m_copyAttr.y(v),
		                m_copyAttr.x(w), m_copyAttr.y(w));
	};

	// For all dummy nodes (in the correct order for each edge):
	for (edge eOrig : origEdges) {
		DPolyline &bendLine = attr.bends(eOrig);
		List<edge> chain = m_copy.chain(eOrig);
		node last = chain.popFrontRet()->source();

		for (edge e : chain) {
			// Get last --> v --> next where v is a dummy node.
			node v    = e->source();
			node next = e->target();
#ifdef OGDF_DEBUG
			OGDF_ASSERT(m_copy.isDummy(v));
#endif
			DSegment segmentLastNext = toSegment(last, next);
			DSegment segmentLastV    = toSegment(last, v);
			DSegment segmentVNext    = toSegment(v, next);

			if (m_postProcessing == PostProcessingMode::Complete ||
			    (m_postProcessing == PostProcessingMode::KeepMultiEdgeBends &&
			     !m_hasParEdges[eOrig] && !eOrig->isSelfLoop())) {
				int nIntersectionsDummy = 0;
				int nIntersectionsNoDummy = 0;

				// Count how often the edge segments between v and its neighbors
				// intersect a non-dummy w (!= v or its neighbors).
				for (node w : m_copy.nodes) {
					if (!m_copy.isDummy(w) && w != last && w != next) {
						if (nodeRects[w].intersection(segmentLastNext)) {
							nIntersectionsNoDummy++;
						}
						if (nodeRects[w].intersection(segmentLastV)) {
							nIntersectionsDummy++;
						}
						if (nodeRects[w].intersection(segmentVNext)) {
							nIntersectionsDummy++;
						}
					}
				}

				// If the inclusion of v does not reduce the number of
				// intersections, delete v from the graph copy.
				if (nIntersectionsNoDummy <= nIntersectionsDummy) {
					edge e1 = v->firstAdj()->theEdge();
					edge e2 = v->lastAdj()->theEdge();
					if (e1->target() == v) {
						m_copy.unsplit(e1, e2);
					} else {
						m_copy.unsplit(e2, e1);
					}
				} else {
					// Else use v as a bend point.
					bendLine.pushBack(m_copyAttr.point(v));
					last = v;
				}
			} else {
				// If post processing is not activated, add all dummies as
				// bends points.
				bendLine.pushBack(m_copyAttr.point(v));
			}
		}

		// Normalize the DPolyline of bend points.
		if (m_postProcessing != PostProcessingMode::Complete ||
			!OGDF_GEOM_ET.equal(m_bendNormalizationAngle, Math::pi)) {
			node src = eOrig->source();
			node tgt = eOrig->target();
			bendLine.normalize(attr.point(src), attr.point(tgt),
			                   m_bendNormalizationAngle);
		}
	}
}


void NodeRespecterLayout::call(GraphAttributes &attr)
{
	const Graph &G = attr.constGraph();

	if (G.empty()) {
		return;
	}

	// Start with a straight-line layout.
	attr.clearAllBends();

	// If the user wants bends of multi-edges to be kept during post-processing,
	// find out which edges have parallel edges.
	if (m_postProcessing == PostProcessingMode::KeepMultiEdgeBends) {
		m_hasParEdges.init(G, false);
		EdgeArray<List<edge>> parallelEdges(G);
		getParallelFreeUndirected(G, parallelEdges);
		for (edge e : G.edges) {
			for (edge parEdge : parallelEdges[e]) {
				m_hasParEdges[e] = m_hasParEdges[parEdge] = true;
			}
		}
	}

	// Create empty graph copy associated with G.
	m_copy.createEmpty(G);

	// Intialize arrays with a list of nodes/edges for each connected component.
	NodeArray<int> component(G);
	int numCC = connectedComponents(G, component);

	Array<List<node>> nodesInCC(numCC);
	for (node v : G.nodes) {
		nodesInCC[component[v]].pushBack(v);
	}

	Array<ArrayBuffer<edge>> edgesInCC(numCC);
	for (edge e : G.edges) {
		edgesInCC[component[e->source()]].push(e);
	}

	Array<DPoint> boundingBox(numCC);

	// For every connected component:
	for (int i = 0; i < numCC; ++i) {
		// Initialize graph copy and its data.
		EdgeArray<edge> copyEdges(G);
		m_copy.initByNodes(nodesInCC[i], copyEdges);
		initData();

		// Initially place nodes randomly.
		if (m_randomInitialPlacement) {
			int nCC = nodesInCC[i].size();
			for (node vOrig : nodesInCC[i]) {
				attr.x(vOrig) = randomDouble(0.0, nCC);
				attr.y(vOrig) = randomDouble(0.0, nCC);
			}
		}

		// Get graph copy attributes with coordinates of original.
		m_copyAttr = GraphAttributes(m_copy);
		for (node vCopy : m_copy.nodes) {
			node vOrig = m_copy.original(vCopy);
			m_copyAttr.x(vCopy) = attr.x(vOrig);
			m_copyAttr.y(vCopy) = attr.y(vOrig);
			m_copyAttr.width(vCopy) = attr.width(vOrig);
			m_copyAttr.height(vCopy) = attr.height(vOrig);

			// Calculate the radius for each (non-dummy) node.
			m_nodeRadius[vCopy] = radius(attr, vOrig);

			// Calculate the barycenter of all (non-dummy) nodes.
			m_barycenterX += weight(vCopy) * m_copyAttr.x(vCopy);
			m_barycenterY += weight(vCopy) * m_copyAttr.y(vCopy);
		}


		if (m_initDummiesPerEdge > 0) {
			// Create dummy nodes splitting each edge in edges of equal length.
			for (edge eOrig : edgesInCC[i]) {
				edge e = copyEdges[eOrig];
				node v = e->source();
				node w = e->target();
				edge edgeToSplit = e;

				// Get distance between centers of v and w.
				double vBorderPointX = m_copyAttr.x(v);
				double vBorderPointY = m_copyAttr.y(v);
				double deltaX = m_copyAttr.x(v) - m_copyAttr.x(w);
				double deltaY = m_copyAttr.y(v) - m_copyAttr.y(w);
				double delta = std::hypot(deltaX, deltaY);
				double borderDelta = delta - m_nodeRadius[v] - m_nodeRadius[w];

				// If v- and w-circles do not overlap.
				if  (borderDelta > 0.0) {
					// Get cos/sin of angle in center of v between center of w
					// and horizontal line.
					double cosPhi = deltaX / delta;
					double sinPhi = deltaY / delta;

					// Get point on border of v-circle in direction of w.
					vBorderPointX -= cosPhi * m_nodeRadius[v];
					vBorderPointY -= sinPhi * m_nodeRadius[v];

					// Get distance between border points of v- and w-circles.
					deltaX = cosPhi * borderDelta;
					deltaY = sinPhi * borderDelta;
				}

				for (int j = 0; j < m_initDummiesPerEdge; j++) {
					edgeToSplit = m_copy.split(edgeToSplit);
					node dummy = edgeToSplit->source();
					double distRatio = (static_cast<double>(j+1) /
							static_cast<double>(m_initDummiesPerEdge + 1));
					m_copyAttr.x(dummy) = vBorderPointX - deltaX * distRatio;
					m_copyAttr.y(dummy) = vBorderPointY - deltaY * distRatio;
				}
			}
		}

		// Get desired distance between each pair of nodes (respect node radii
		// of non-dummies).
		double halfDesiredEdgeLength = 0.5 * m_desiredMinEdgeLength;
		for (node v : m_copy.nodes) {
			m_desiredDistance[v].init(m_copy);
			for (node w : m_copy.nodes) {
				m_desiredDistance[v][w] = m_copy.isDummy(v) || m_copy.isDummy(w) ?
				    halfDesiredEdgeLength : m_desiredMinEdgeLength;
				m_desiredDistance[v][w] += m_nodeRadius[v] + m_nodeRadius[w];
			}
		}

		if (m_initDummiesPerEdge > 0) {
			int desiredDummyEdgeLength = m_desiredMinEdgeLength /
			                             (m_initDummiesPerEdge + 1);
			for (edge e : m_copy.edges) {
				node v = e->source();
				node w = e->target();
				m_desiredDistance[v][w] = m_desiredDistance[w][v] =
					desiredDummyEdgeLength + m_nodeRadius[v] + m_nodeRadius[w];
			}
		}

		// Main loop: Compute forces and update node positions.
		SListPure<node> nodes;
		m_copy.allNodes(nodes);
		updateNodeLoop(nodes);

		// Get bounding box of connected component,
		// respect minimal distance between connected components.
		node vFirst = m_copy.firstNode();
		double minX = m_copyAttr.x(vFirst),
			   maxX = m_copyAttr.x(vFirst),
			   minY = m_copyAttr.y(vFirst),
			   maxY = m_copyAttr.y(vFirst);

		for (node vCopy : m_copy.nodes) {
			Math::updateMin(minX, m_copyAttr.x(vCopy) - m_copyAttr.width(vCopy)/2);
			Math::updateMin(minY, m_copyAttr.y(vCopy) - m_copyAttr.height(vCopy)/2);
			Math::updateMax(maxX, m_copyAttr.x(vCopy) + m_copyAttr.width(vCopy)/2);
			Math::updateMax(maxY, m_copyAttr.y(vCopy) + m_copyAttr.height(vCopy)/2);
		}

		minX -= m_minDistCC;
		minY -= m_minDistCC;
		boundingBox[i] = DPoint(maxX - minX, maxY - minY);

		// Move all nodes to the borders of their bounding box.
		for (node vCopy : m_copy.nodes) {
			m_copyAttr.x(vCopy) -= minX;
			m_copyAttr.y(vCopy) -= minY;

			// Get node positions for original graph from graph copy.
			if (!m_copy.isDummy(vCopy)) {
				node v = m_copy.original(vCopy);
				attr.x(v) = m_copyAttr.x(vCopy);
				attr.y(v) = m_copyAttr.y(vCopy);
			}
		}

		createBends(edgesInCC[i], attr);
	}

	// Pack connected components into the bounding box.
	Array<DPoint> offset(numCC);
	TileToRowsCCPacker packer;
	packer.call(boundingBox, offset, m_pageRatio);

	// Move each node and bendpoint by the offset of its connected component.
	for (int i = 0; i < numCC; ++i) {
		const double dx = offset[i].m_x;
		const double dy = offset[i].m_y;

		for (node v : nodesInCC[i]) {
			attr.x(v) += dx;
			attr.y(v) += dy;
		}

		for (edge e : edgesInCC[i]) {
			for (DPoint &bendPoint : attr.bends(e)) {
				bendPoint.m_x += dx;
				bendPoint.m_y += dy;
			}
		}
	}

	freeData();
}


void NodeRespecterLayout::updateNodeLoop(SListPure<node> &nodes)
{
	SListIterator<node> iter = SListIterator<node>();
	while (OGDF_GEOM_ET.greater(m_globalTemperature, m_minimalTemperature) &&
	       m_iterCounter-- > 0) {
		// Choose nodes by permuting them randomly.
		// Move one node per iteration.
		if (!iter.valid()) {
			nodes.permute();
			iter = nodes.begin();
		}
		node v = *nodes.cyclicSucc(iter++);

		// Compute the impulse of node v and update it.
		updateNode(v, computeImpulse(v));

		// Add dummies to incident edges if necessary.
		addDummies(v, nodes);

#ifdef OGDF_NODERESPECTERLAYOUT_DEBUG
		if (m_iterCounter % 2000 == 0) {
			std::cout << "Counter: " << m_iterCounter << ", ";
			std::cout << "Global Temperature: " << m_globalTemperature << std::endl;
		}
#endif
	}
}


std::pair<double,double> NodeRespecterLayout::computeImpulse(node v)
{
	double deltaX, deltaY, delta;

	// Disturb randomly.
	double newImpulseX = randomDouble(-m_maxDisturbance, m_maxDisturbance);
	double newImpulseY = randomDouble(-m_maxDisturbance, m_maxDisturbance);

	// Compute attraction to barycenter.
	if (!m_copy.isDummy(v)) {
		double n = m_copy.numberOfNodes();
		newImpulseX += (m_barycenterX / n - m_copyAttr.x(v)) * m_gravitation;
		newImpulseY += (m_barycenterY / n - m_copyAttr.y(v)) * m_gravitation;
	}

	// Compute repulsive forces.
	for (node w : m_copy.nodes) {
		if (v != w && !haveSameOriginalEdge(v, w)) {
			// Calculate distance between centers of v and w.
			deltaX = m_copyAttr.x(v) - m_copyAttr.x(w);
			deltaY = m_copyAttr.y(v) - m_copyAttr.y(w);
			delta = std::hypot(deltaX, deltaY);

			// If v & w are in the same place, push v in a random direction and
			// set delta to new distance between them.
			if (OGDF_GEOM_ET.equal(delta, 0.0)) {
				// Note: shiftDist can be any constant [0.1 ... 2*(r1+r2)], it
				// barely affects the results.
				const double shiftDist = 0.5;
				double angle = randomDouble(0.0, 2.0*Math::pi);
				newImpulseX += cos(angle) * shiftDist;
				newImpulseY += sin(angle) * shiftDist;
				delta = shiftDist;
			}

			// Repulsion formula (if v or w is a dummy node, only use the
			// formula if the distance between them is small enough):
			if (delta < m_repulsionDistance ||
			    (!m_copy.isDummy(v) && !m_copy.isDummy(w))) {
				double deltaSqu = delta * delta;
				double desired = m_desiredDistance[v][w];
				double desiredSqu = desired * desired;
				newImpulseX += deltaX * desiredSqu / deltaSqu;
				newImpulseY += deltaY * desiredSqu / deltaSqu;
			}
		}
	}

	// Compute attractive forces.
	for (adjEntry adj : v->adjEntries) {
		node w = adj->twinNode();

		// Calculate distance between centers of v and w.
		deltaX = m_copyAttr.x(v) - m_copyAttr.x(w);
		deltaY = m_copyAttr.y(v) - m_copyAttr.y(w);
		delta = std::hypot(deltaX, deltaY);

		// If the nodes do not overlap, use the attraction formula.
		if (delta - m_nodeRadius[v] - m_nodeRadius[w] > 0.0) {
			double divisor = m_desiredDistance[v][w];
			newImpulseX -= deltaX * delta / divisor;
			newImpulseY -= deltaY * delta / divisor;
		}
	}

	// Scale impulse by node temperature.
	double impulseLength = std::hypot(newImpulseX, newImpulseY);
	if (OGDF_GEOM_ET.greater(impulseLength, 0.0)) {
		newImpulseX *= m_localTemperature[v] / impulseLength;
		newImpulseY *= m_localTemperature[v] / impulseLength;
	}

	return std::pair<double,double>(newImpulseX, newImpulseY);
}


void NodeRespecterLayout::updateNode(node v, std::pair<double,double> newImpulse)
{
	int n = m_copy.numberOfNodes();
	double newImpulseX = std::get<0>(newImpulse);
	double newImpulseY = std::get<1>(newImpulse);
	double impulseLength = std::hypot(newImpulseX, newImpulseY);

	// Remove old local temperature from global temperature.
	m_globalTemperature -= m_localTemperature[v] / n;

	if (OGDF_GEOM_ET.greater(impulseLength, 0.0)) {
		// Move node.
		m_copyAttr.x(v) += newImpulseX;
		m_copyAttr.y(v) += newImpulseY;

		// Adjust barycenter.
		if (!m_copy.isDummy(v)) {
			double nodeWeight = weight(v);
			m_barycenterX += nodeWeight * newImpulseX;
			m_barycenterY += nodeWeight * newImpulseY;
		}

		// Get impulse length * previous impulse length.
		impulseLength *= std::hypot(m_impulseX[v], m_impulseY[v]);

		if (OGDF_GEOM_ET.greater(impulseLength, 0.0)) {
			// Check for oscillation (angle between impulse and previous impulse
			// close to 180°), update local temperature.
			double cosBeta = (newImpulseX * m_impulseX[v] +
	                          newImpulseY * m_impulseY[v]) / impulseLength;
			if (OGDF_GEOM_ET.greater(std::abs(cosBeta), m_cos)){
				m_localTemperature[v] *= (1.0 + cosBeta * 0.3);
			}

			double currentMaxTemp =
				m_iterCounter <= m_numberOfIterations * m_temperatureDecreaseOffset ?
				m_iterCounter * m_factor + m_minimalTemperature :
				m_initialTemperature;

			if (OGDF_GEOM_ET.geq(m_localTemperature[v], m_initialTemperature)) {
				m_localTemperature[v] = m_initialTemperature;
			}

			m_localTemperature[v] = (currentMaxTemp * m_localTemperature[v]) / m_initialTemperature;
		}

		// Save impulse.
		m_impulseX[v] = newImpulseX;
		m_impulseY[v] = newImpulseY;
	} else {
		// If the node does not move, reduce its temperature drastically.
		m_localTemperature[v] -= 1.0;
	}

	// Add new local temperature to global temperature.
	m_globalTemperature += m_localTemperature[v] / n;
}


void NodeRespecterLayout::addDummies(node v, SListPure<node> &nodes)
{
	double halfDesiredEdgeLength = 0.5 * m_desiredMinEdgeLength;

	// For each incident edge, add dummy if there is too much space between
	// v and its neighbor.
	for (adjEntry adj : v->adjEntries) {
		edge eOrig = m_copy.original(adj->theEdge());
		int nDummiesEOrig = m_copy.chain(eOrig).size() - 1;

		// If the maximum number of dummies is not reached yet for this edge:
		if (nDummiesEOrig < m_maxDummiesPerEdge) {
			node w = adj->twinNode();
			// Get distance between borders of v and w.
			double deltaX = m_copyAttr.x(v) - m_copyAttr.x(w);
			double deltaY = m_copyAttr.y(v) - m_copyAttr.y(w);
			double delta = std::hypot(deltaX, deltaY);

			// If the distance between v's and w's borders is large enough.
			if (delta > m_dummyInsertionThreshold * m_desiredDistance[v][w]) {
				node dummy = m_copy.split(adj->theEdge())->source();
				nodes.pushBack(dummy);

				// Place dummy directly between the borders of v and w.
				double cosPhi = deltaX / delta;
				double sinPhi = deltaY / delta;
				double halfBorderDelta = m_nodeRadius[v] +
					(delta - m_nodeRadius[v] - m_nodeRadius[w]) * 0.5;
				m_copyAttr.x(dummy) = m_copyAttr.x(v) - cosPhi * halfBorderDelta;
				m_copyAttr.y(dummy) = m_copyAttr.y(v) - sinPhi * halfBorderDelta;

				// Get desired distance between new dummy and other nodes.
				m_desiredDistance[dummy].init(m_copy);
				for (node u : m_copy.nodes) {
					m_desiredDistance[u][dummy] = m_desiredDistance[dummy][u] =
						halfDesiredEdgeLength + m_nodeRadius[u] + m_nodeRadius[dummy];
				}

				// Update desired distance for all nodes on the same original edge.
				int desiredDummyEdgeLength = m_desiredMinEdgeLength / (nDummiesEOrig + 2);
				for (edge eCopy : m_copy.chain(eOrig)) {
					node srcCopy = eCopy->source();
					node tgtCopy = eCopy->target();
					m_desiredDistance[srcCopy][tgtCopy] = m_desiredDistance[tgtCopy][srcCopy] =
						desiredDummyEdgeLength + m_nodeRadius[srcCopy] + m_nodeRadius[tgtCopy];
				}
			}
		}
	}
}


}
