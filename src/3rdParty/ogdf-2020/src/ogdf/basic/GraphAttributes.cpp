/** \file
 * \brief Implementation of class GraphAttributes.
 *
 * Class GraphAttributes extends a graph by graphical attributes like
 * node position, color, etc.
 *
 * \author Carsten Gutwenger, Karsten Klein
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


#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/GraphCopy.h>


namespace ogdf {

const long GraphAttributes::nodeGraphics      = 1 << 0;
const long GraphAttributes::edgeGraphics      = 1 << 1;
const long GraphAttributes::edgeIntWeight     = 1 << 2;
const long GraphAttributes::edgeDoubleWeight  = 1 << 3;
const long GraphAttributes::edgeLabel         = 1 << 4;
const long GraphAttributes::nodeLabel         = 1 << 5;
const long GraphAttributes::edgeType          = 1 << 6;
const long GraphAttributes::nodeType          = 1 << 7;
const long GraphAttributes::nodeId            = 1 << 8;
const long GraphAttributes::edgeArrow         = 1 << 9;
const long GraphAttributes::edgeStyle         = 1 << 10;
const long GraphAttributes::nodeStyle         = 1 << 11;
const long GraphAttributes::nodeTemplate      = 1 << 12;
const long GraphAttributes::edgeSubGraphs     = 1 << 13;
const long GraphAttributes::nodeWeight        = 1 << 14;
const long GraphAttributes::threeD            = 1 << 15;
const long GraphAttributes::nodeLabelPosition = 1 << 16;
// Make sure this covers all other flags!
// Also if you ever add any attributes that are not to be encompassed by
// GraphAttributes::all, make sure to update ClusterGraphattributes.cpp
// as well so the attributes do not overlap.
const long GraphAttributes::all               = (1 << 17) - 1;

GraphAttributes::GraphAttributes() : m_pGraph(nullptr), m_directed(true), m_attributes(0) { }

GraphAttributes::GraphAttributes(const Graph &G, long attr) : GraphAttributes()
{
	m_pGraph = &G;
	addAttributes(attr);
}


void GraphAttributes::addAttributes(long attr)
{
	m_attributes |= attr;

	// assert implications of attributes
	OGDF_ASSERT(!(m_attributes & nodeStyle) || (m_attributes & nodeGraphics));
	OGDF_ASSERT(!(m_attributes & threeD) || (m_attributes & nodeGraphics));
	OGDF_ASSERT(!(m_attributes & edgeStyle) || (m_attributes & edgeGraphics));
	OGDF_ASSERT(!(m_attributes & nodeLabelPosition) || (m_attributes & nodeLabel));

	if (attr & nodeGraphics) {
		m_x        .init( *m_pGraph, 0.0 );
		m_y        .init( *m_pGraph, 0.0 );
		m_width    .init( *m_pGraph, LayoutStandards::defaultNodeWidth () );
		m_height   .init( *m_pGraph, LayoutStandards::defaultNodeHeight() );
		m_nodeShape.init( *m_pGraph, LayoutStandards::defaultNodeShape () );
	}
	if (attr & threeD) {
		m_z.init(*m_pGraph, 0.0);
		if ((attr | m_attributes) & nodeLabelPosition) {
			m_nodeLabelPosZ.init(*m_pGraph, 0.0);
		}
	}
	if (attr & nodeStyle) {
		m_nodeStroke.init( *m_pGraph, LayoutStandards::defaultNodeStroke() );
		m_nodeFill  .init( *m_pGraph, LayoutStandards::defaultNodeFill  () );
	}
	if (attr & edgeGraphics) {
		m_bends.init( *m_pGraph, DPolyline() );
	}
	if (attr & edgeStyle) {
		m_edgeStroke.init( *m_pGraph, LayoutStandards::defaultEdgeStroke() );
	}
	if (attr & nodeWeight) {
		m_nodeIntWeight.init( *m_pGraph, 0 );
	}
	if (attr & edgeIntWeight) {
		m_intWeight.init( *m_pGraph, 1 );
	}
	if (attr & edgeDoubleWeight) {
		m_doubleWeight.init( *m_pGraph, 1.0 );
	}
	if (attr & nodeLabel) {
		m_nodeLabel.init(*m_pGraph);
	}
	if (attr & nodeLabelPosition) {
		m_nodeLabelPosX.init(*m_pGraph, 0.0);
		m_nodeLabelPosY.init(*m_pGraph, 0.0);
		if ((attr | m_attributes) & threeD) {
			m_nodeLabelPosZ.init(*m_pGraph, 0.0);
		}
	}
	if (attr & edgeLabel) {
		m_edgeLabel.init(*m_pGraph);
	}
	if (attr & edgeType) {
		m_eType.init( *m_pGraph, Graph::EdgeType::association ); //should be Graph::standard and explicitly set
	}
	if (attr & nodeType) {
		m_vType.init( *m_pGraph, Graph::NodeType::vertex );
	}
	if (attr & nodeId) {
		m_nodeId.init( *m_pGraph, -1 );
	}
	if (attr & edgeArrow) {
		m_edgeArrow.init( *m_pGraph, LayoutStandards::defaultEdgeArrow() );
	}
	if (attr & nodeTemplate) {
		m_nodeTemplate.init(*m_pGraph);
	}
	if (attr & edgeSubGraphs) {
		m_subGraph.init( *m_pGraph, 0 );
	}
}


void GraphAttributes::destroyAttributes(long attr)
{
	m_attributes &= ~attr;

	if (attr & nodeGraphics) {
		m_x     .init();
		m_y     .init();
		m_width .init();
		m_height.init();
		m_nodeShape.init();
		if (attr & nodeStyle) {
			m_nodeStroke.init();
			m_nodeFill  .init();
		}
	}
	if (attr & threeD) {
		m_z.init();
		m_nodeLabelPosZ.init();
	}
	if (attr & edgeGraphics) {
		m_bends.init();
	}
	if (attr & edgeStyle) {
		m_edgeStroke.init();
	}
	if (attr & nodeWeight) {
		m_nodeIntWeight.init();
	}
	if (attr & edgeIntWeight) {
		m_intWeight.init();
	}
	if (attr & edgeDoubleWeight) {
		m_doubleWeight.init();
	}
	if (attr & nodeLabel) {
		m_nodeLabel.init();
	}
	if (attr & nodeLabelPosition) {
		m_nodeLabelPosX.init();
		m_nodeLabelPosY.init();
		m_nodeLabelPosZ.init();
	}
	if (attr & edgeLabel) {
		m_edgeLabel.init();
	}
	if (attr & nodeId) {
		m_nodeId.init();
	}
	if (attr & edgeArrow) {
		m_edgeArrow.init();
	}
	if (attr & nodeTemplate) {
		m_nodeTemplate.init();
	}
	if (attr & edgeSubGraphs) {
		m_subGraph.init();
	}
}

void GraphAttributes::init(long attr)
{
	destroyAttributes(m_attributes);
	addAttributes(attr);
}

void GraphAttributes::init(const Graph &G, long attr)
{
	m_pGraph = &G;
	init(attr);
}

void GraphAttributes::setAllWidth(double w)
{
	for(node v : m_pGraph->nodes)
		m_width[v] = w;
}


void GraphAttributes::setAllHeight(double h)
{
	for (node v : m_pGraph->nodes)
		m_height[v] = h;
}


void GraphAttributes::clearAllBends()
{
	for(edge e : m_pGraph->edges)
		m_bends[e].clear();
}


//
// calculates the bounding box of the graph
DRect GraphAttributes::boundingBox() const
{
	const Graph &G = constGraph();
	double minx = 0;
	double maxx = 0;
	double miny = 0;
	double maxy = 0;

	if(has(nodeGraphics) && !G.empty()) {
		minx = maxx = x(G.firstNode());
		miny = maxy = y(G.firstNode());

		for(node v : G.nodes) {
			double lw = has(GraphAttributes::nodeStyle) ? 0.5*strokeWidth(v) : 0;

			Math::updateMin(minx, x(v) - width(v) / 2 - lw);
			Math::updateMax(maxx, x(v) + width(v) / 2 + lw);
			Math::updateMin(miny, y(v) - height(v) / 2 - lw);
			Math::updateMax(maxy, y(v) + height(v) / 2 + lw);
		}
	}

	if(has(edgeGraphics)) {
		for(edge e : G.edges) {
			const DPolyline &dpl = bends(e);
			double lw = has(GraphAttributes::edgeStyle) ? 0.5*strokeWidth(e) : 0;

			for (const DPoint &p : dpl) {
				Math::updateMin(minx, p.m_x - lw);
				Math::updateMax(maxx, p.m_x + lw);
				Math::updateMin(miny, p.m_y - lw);
				Math::updateMax(maxy, p.m_y + lw);
			}
		}
	}

	return DRect(minx, miny, maxx, maxy);
}


//
// returns a list of all hierachies in the graph (a hierachy consists of a set of nodes)
// at least one list is returned, which is the list of all nodes not belonging to any hierachy
// this is always the first list
// the return-value of this function is the number of hierachies
int GraphAttributes::hierarchyList(List<List<node>* > &list) const
{
	// list must be empty during startup
	OGDF_ASSERT(list.empty());

	const Graph &G = constGraph();
	Array<bool> processed(0, G.maxNodeIndex(), false);

	// initialize the first list of all single nodes
	List<node> *firstList = new List<node>;
	list.pushBack(firstList);

	for(node v : G.nodes) { // scan all nodes

		// skip, if already processed
		if (processed[v->index()])
			continue;

		List<node> nodeSet;                    // set of nodes in this hierachy,
		// whose neighbours have to be processed
		List<node> *hierachy = new List<node>; // holds all nodes in this hierachy

		nodeSet.pushBack(v);           // push the unprocessed node to the list
		processed[v->index()] = true;  // and mark it as processed

		do { // scan all neighbours of nodes in 'nodeSet'
			node front = nodeSet.popFrontRet();
			hierachy->pushBack(front); // push front to the list of nodes in this hierachy

			// process all the neighbours of front, e.g. push them into 'nodeSet'
			for(adjEntry adj : front->adjEntries) {
				edge e = adj->theEdge();

				if (type(e) == Graph::EdgeType::generalization) {
					node w = e->source() == front ? e->target() : e->source();
					if (!processed[w->index()]) {
						nodeSet.pushBack(w);
						processed[w->index()] = true;
					}
				}
			}
		} while (!nodeSet.empty());

		// skip adding 'hierachy', if it contains only one node
		if (hierachy->size() == 1) {
			firstList->conc(*hierachy);
			delete hierachy;
		}
		else
			list.pushBack(hierachy);
	}

	return list.size() - 1 + (*list.begin())->size();
}


//
// returns a list of all hierarchies in the graph (in this case, a hierarchy consists of a set of edges)
// list may be empty, if no generalizations are used
// the return-value of this function is the number of hierarchies with generalizations
int GraphAttributes::hierarchyList(List<List<edge>* > &list) const
{
	// list must be empty during startup
	OGDF_ASSERT(list.empty());

	const Graph &G = constGraph();
	Array<bool> processed(0, G.maxNodeIndex(), false);

	for(node v : G.nodes) { // scan all nodes

		// skip, if already processed
		if (processed[v->index()])
			continue;

		List<node> nodeSet;                    // set of nodes in this hierarchy,
		// whose neighbours have to be processed
		List<edge> *hierarchy = new List<edge>; // holds all edges in this hierarchy

		nodeSet.pushBack(v);           // push the unprocessed node to the list
		processed[v->index()] = true;  // and mark it as processed

		do { // scan all neighbours of nodes in 'nodeSet'
			node front = nodeSet.popFrontRet();

			// process all the neighbours of front, e.g. push them into 'nodeSet'
			for(adjEntry adj : front->adjEntries) {
				edge e = adj->theEdge();

				if (type(e) == Graph::EdgeType::generalization) {
					node w = e->source() == front ? e->target() : e->source();
					if (!processed[w->index()]) {
						nodeSet.pushBack(w);
						processed[w->index()] = true;
						hierarchy->pushBack(e); // push e to the list of edges in this hierarchy
					}
				}
			}
		} while (!nodeSet.empty());

		// skip adding 'hierarchy', if it contains only one node
		if (hierarchy->empty())
			delete hierarchy;
		else
			list.pushBack(hierarchy);
	}

	return list.size();
}



void GraphAttributes::removeUnnecessaryBendsHV()
{
	for(edge e: m_pGraph->edges)
	{
		DPolyline &dpl = m_bends[e];

		if(dpl.size() < 3)
			continue;

		ListIterator<DPoint> it1, it2, it3;

		it1 = dpl.begin();
		it2 = it1.succ();
		it3 = it2.succ();

		do {
			if(((*it1).m_x == (*it2).m_x && (*it2).m_x == (*it3).m_x) ||
				((*it1).m_y == (*it2).m_y && (*it2).m_y == (*it3).m_y))
			{
				dpl.del(it2);
				it2 = it3;
			} else {
				it1 = it2;
				it2 = it3;
			}

			it3 = it2.succ();
		} while(it3.valid());
	}
}


void GraphAttributes::addNodeCenter2Bends(int mode)
{
	OGDF_ASSERT(mode >= 0);
	OGDF_ASSERT(mode <= 2);
	for (edge e : m_pGraph->edges) {
		node v = e->source();
		node w = e->target();
		DPolyline &bendpoints = bends(e);
		if (mode <= 1) {
			// push center to the bends
			bendpoints.pushFront(point(v));
			bendpoints.pushBack (point(w));
		}
		if (mode >= 1) {
			// determine intersection between node and last bend-segment
			DPoint sp1(x(v) - width(v)/2, y(v) - height(v)/2);
			DPoint sp2(x(v) - width(v)/2, y(v) + height(v)/2);
			DPoint sp3(x(v) + width(v)/2, y(v) + height(v)/2);
			DPoint sp4(x(v) + width(v)/2, y(v) - height(v)/2);
			DSegment sourceRect[4] = {
				DSegment(sp1, sp2),
				DSegment(sp2, sp3),
				DSegment(sp3, sp4),
				DSegment(sp4, sp1)
			};

			DPoint tp1(x(w) - width(w)/2, y(w) - height(w)/2);
			DPoint tp2(x(w) - width(w)/2, y(w) + height(w)/2);
			DPoint tp3(x(w) + width(w)/2, y(w) + height(w)/2);
			DPoint tp4(x(w) + width(w)/2, y(w) - height(w)/2);
			DSegment targetRect[4] = {
				DSegment(tp1, tp2),
				DSegment(tp2, tp3),
				DSegment(tp3, tp4),
				DSegment(tp4, tp1)
			};

			DRect source(sp1, sp3);
			DRect target(tp1, tp3);

			DPoint c1 = bendpoints.popFrontRet();
			DPoint c2 = bendpoints.popBackRet();

			while (!bendpoints.empty() && source.contains(bendpoints.front()))
				c1 = bendpoints.popFrontRet();
			while (!bendpoints.empty() && target.contains(bendpoints.back()))
				c2 = bendpoints.popBackRet();

			DPoint a1, a2;
			int i;
			// TODO: What to do when IntersectionType::Overlapping is returned?
			if (bendpoints.size() == 0) {
				DSegment cross(c1, c2);
				for (i = 0; i < 4; i++)
					if (cross.intersection(sourceRect[i], a1) == IntersectionType::SinglePoint) break;
				for (i = 0; i < 4; i++)
					if (cross.intersection(targetRect[i], a2) == IntersectionType::SinglePoint) break;
			}
			else {
				DSegment cross1(c1, bendpoints.front());
				for (i = 0; i < 4; i++)
					if (cross1.intersection(sourceRect[i], a1) == IntersectionType::SinglePoint) break;
				DSegment cross2(bendpoints.back(), c2);
				for (i = 0; i < 4; i++)
					if (cross2.intersection(targetRect[i], a2) == IntersectionType::SinglePoint) break;
			}
			bendpoints.pushFront(a1);
			bendpoints.pushBack(a2);
		}
		bendpoints.normalize();
	}
}


void GraphAttributes::scale(double sx, double sy, bool scaleNodes)
{
	if (m_attributes & nodeGraphics) {
		for (node v : m_pGraph->nodes) {
			m_x[v] *= sx;
			m_y[v] *= sy;
		}

		if (scaleNodes) {
			double asx = fabs(sx), asy = fabs(sy);
			for (node v : m_pGraph->nodes) {
				m_width [v] *= asx;
				m_height[v] *= asy;
			}
		}
	}

	if (m_attributes & edgeGraphics) {
		for (edge e : m_pGraph->edges) {
			for (DPoint &p : m_bends[e]) {
				p.m_x *= sx;
				p.m_y *= sy;
			}
		}
	}
}


void GraphAttributes::translate(double dx, double dy)
{
	if (m_attributes & nodeGraphics) {
		for (node v : m_pGraph->nodes) {
			m_x[v] += dx;
			m_y[v] += dy;
		}
	}

	if (m_attributes & edgeGraphics) {
		for (edge e : m_pGraph->edges) {
			for (DPoint &p : m_bends[e]) {
				p.m_x += dx;
				p.m_y += dy;
			}
		}
	}
}


void GraphAttributes::translateToNonNeg()
{
	if ((m_attributes & nodeGraphics) == 0)
		return;

	DRect bb = boundingBox();

	double dx = -bb.p1().m_x;
	double dy = -bb.p1().m_y;

	if (dx != 0 || dy != 0)
		translate(dx, dy);
}


void GraphAttributes::flipVertical(const DRect &box)
{
	if ((m_attributes & nodeGraphics) == 0)
		return;

	double dy = box.p1().m_y + box.p2().m_y;

	for (node v : m_pGraph->nodes) {
		m_y[v] = dy - m_y[v];
	}

	if (m_attributes & edgeGraphics) {
		for (edge e : m_pGraph->edges) {
			for (DPoint &p : m_bends[e]) {
				p.m_y = dy - p.m_y;
			}
		}
	}
}


void GraphAttributes::flipHorizontal(const DRect &box)
{
	if ((m_attributes & nodeGraphics) == 0)
		return;

	double dx = box.p1().m_x + box.p2().m_x;

	for (node v : m_pGraph->nodes) {
		m_x[v] = dx - m_x[v];
	}

	if (m_attributes & edgeGraphics) {
		for (edge e : m_pGraph->edges) {
			for (DPoint &p : m_bends[e]) {
				p.m_x = dx - p.m_x;
			}
		}
	}
}


void GraphAttributes::scaleAndTranslate(double sx, double sy, double dx, double dy, bool scaleNodes)
{
	if (m_attributes & nodeGraphics) {
		for (node v : m_pGraph->nodes) {
			m_x[v] = m_x[v] * sx + dx;
			m_y[v] = m_y[v] * sy + dy;
		}

		if (scaleNodes) {
			for (node v : m_pGraph->nodes) {
				double asx = fabs(sx), asy = fabs(sy);
				m_width[v]  *= asx;
				m_height[v] *= asy;
			}
		}
	}

	if (m_attributes & edgeGraphics) {
		for (edge e : m_pGraph->edges) {
			for (DPoint &p : m_bends[e]) {
				p.m_x = p.m_x * sx + dx;
				p.m_y = p.m_y * sy + dy;
			}
		}
	}
}


void GraphAttributes::rotateRight90()
{
	if (m_attributes & nodeGraphics) {
		for (node v : m_pGraph->nodes) {
			double x = m_x[v], y = m_y[v];
			m_x[v] = -y;
			m_y[v] = x;

			std::swap(m_width[v], m_height[v]);
		}
	}

	if (m_attributes & edgeGraphics) {
		for (edge e : m_pGraph->edges) {
			for (DPoint &p : m_bends[e]) {
				double x = p.m_x, y = p.m_y;
				p.m_x = -y;
				p.m_y = x;
			}
		}
	}
}


void GraphAttributes::rotateLeft90()
{
	if (m_attributes & nodeGraphics) {
		for (node v : m_pGraph->nodes) {
			double x = m_x[v], y = m_y[v];
			m_x[v] = y;
			m_y[v] = -x;

			std::swap(m_width[v], m_height[v]);
		}
	}

	if (m_attributes & edgeGraphics) {
		for (edge e : m_pGraph->edges) {
			for (DPoint &p : m_bends[e]) {
				double x = p.m_x, y = p.m_y;
				p.m_x = y;
				p.m_y = -x;
			}
		}
	}
}

void GraphAttributes::copyNodeAttributes(GraphAttributes &toAttr, node vFrom, node vTo, long attrs) const {
	if (vTo != nullptr && vFrom != nullptr) {
		if (attrs & nodeGraphics) {
			toAttr.x(vTo) = x(vFrom);
			toAttr.y(vTo) = y(vFrom);
			toAttr.width(vTo) = width(vFrom);
			toAttr.height(vTo) = height(vFrom);
			toAttr.shape(vTo) = shape(vFrom);
		}
		if (attrs & threeD) {
			toAttr.z(vTo) = z(vFrom);
		}
		if (attrs & nodeStyle) {
			toAttr.strokeColor(vTo) = strokeColor(vFrom);
			toAttr.strokeType(vTo) = strokeType(vFrom);
			toAttr.strokeWidth(vTo) = strokeWidth(vFrom);
			toAttr.fillBgColor(vTo) = fillBgColor(vFrom);
			toAttr.fillColor(vTo) = fillColor(vFrom);
			toAttr.fillPattern(vTo) = fillPattern(vFrom);
		}
		if (attrs & nodeWeight) {
			toAttr.weight(vTo) = weight(vFrom);
		}
		if (attrs & nodeLabel) {
			toAttr.label(vTo) = label(vFrom);
		}
		if (attrs & nodeLabelPosition) {
			toAttr.xLabel(vTo) = xLabel(vFrom);
			toAttr.yLabel(vTo) = yLabel(vFrom);
			if (attrs & threeD) {
				toAttr.zLabel(vTo) = zLabel(vFrom);
			}
		}
		if (attrs & nodeType) {
			toAttr.type(vTo) = type(vFrom);
		}
		if (attrs & nodeId) {
			toAttr.idNode(vTo) = idNode(vFrom);
		}
		if (attrs & nodeTemplate) {
			toAttr.templateNode(vTo) = templateNode(vFrom);
		}
	}
}

void GraphAttributes::copyEdgeAttributes(GraphAttributes &toAttr, edge eFrom, edge eTo, long attrs) const {
	if (eTo != nullptr && eFrom != nullptr) {
		if (attrs & edgeStyle) {
			toAttr.strokeColor(eTo) = strokeColor(eFrom);
			toAttr.strokeType(eTo) = strokeType(eFrom);
			toAttr.strokeWidth(eTo) = strokeWidth(eFrom);
		}
		if (attrs & edgeIntWeight) {
			toAttr.intWeight(eTo) = intWeight(eFrom);
		}
		if (attrs & edgeDoubleWeight) {
			toAttr.doubleWeight(eTo) = doubleWeight(eFrom);
		}
		if (attrs & edgeLabel) {
			toAttr.label(eTo) = label(eFrom);
		}
		if (attrs & edgeType) {
			toAttr.type(eTo) = type(eFrom);
		}
		if (attrs & edgeArrow) {
			toAttr.arrowType(eTo) = arrowType(eFrom);
		}
		if (attrs & edgeSubGraphs) {
			toAttr.subGraphBits(eTo) = subGraphBits(eFrom);
		}
	}
}

void GraphAttributes::transferToOriginal(GraphAttributes &origAttr) const {
	// GC is the GraphCopy belonging to this GraphAttributes.
	const GraphCopy &GC = dynamic_cast<const GraphCopy&>(constGraph());
	const Graph &G = origAttr.constGraph();
	long bothAttrs = attributes() & origAttr.attributes();

	origAttr.directed() = directed();

	// Transfer node attributes if they are enabled in both.
	for (node vOrig : G.nodes) {
		copyNodeAttributes(origAttr, GC.copy(vOrig), vOrig, bothAttrs);
	}

	auto pushBends = [&](DPolyline &origBends, edge eInChain, bool isReversed) {
		if (isReversed) {
			for (auto bendPoint : reverse(bends(eInChain))) {
				origBends.pushBack(bendPoint);
			}
		} else {
			for (auto bendPoint : bends(eInChain)) {
				origBends.pushBack(bendPoint);
			}
		}
	};

	// Transfer edge attributes if they are enabled in both.
	for (edge eOrig : G.edges) {
		// Always transfer attributes from the first edge in chain(eOrig).
		edge eCopy = GC.copy(eOrig);

		if (eCopy != nullptr && (bothAttrs & edgeGraphics)) {
			// Push bends of the first edge in the chain to the original.
			DPolyline &origBends = origAttr.bends(eOrig);
			origBends.clear();
			pushBends(origBends, eCopy, GC.isReversed(eOrig));

			// Add both dummy nodes and bends of other chain edges as bends to the original.
			const List<edge> &chain = GC.chain(eOrig);
			auto dummyIter = chain.begin();
			auto nextDummyIter = chain.begin();
			for (nextDummyIter++; nextDummyIter != chain.end(); dummyIter++, nextDummyIter++) {
				node dummy = (*dummyIter)->commonNode(*nextDummyIter);
				origBends.pushBack(point(dummy));
				pushBends(origBends, *nextDummyIter, dummy == (*nextDummyIter)->source());
			}

			origBends.normalize();
		}
		copyEdgeAttributes(origAttr, eCopy, eOrig, bothAttrs);
	}
}

void GraphAttributes::transferToCopy(GraphAttributes &copyAttr) const {
	// GC is the GraphCopy belonging to copyAttr.
	const GraphCopy &GC = dynamic_cast<const GraphCopy&>(copyAttr.constGraph());
	const Graph &G = constGraph();
	long bothAttrs = attributes() & copyAttr.attributes();

	copyAttr.directed() = directed();

	// Transfer node attributes if they are enabled in both.
	for (node vOrig : G.nodes) {
		copyNodeAttributes(copyAttr, vOrig, GC.copy(vOrig), bothAttrs);
	}

	// Transfer edge attributes if they are enabled in both.
	for (edge eOrig : G.edges) {
		// Transfer attributes to all copy edges in the chain.
		for (edge eCopy : GC.chain(eOrig)) {
			if (bothAttrs & edgeGraphics) {
				copyAttr.bends(eCopy).clear();
			}
			copyEdgeAttributes(copyAttr, eOrig, eCopy, bothAttrs);
		}

		// Transfer bends to the first copy edge in the chain.
		edge eCopy = GC.copy(eOrig);
		if (eCopy != nullptr && (bothAttrs & edgeGraphics)) {
			DPolyline &copyBends = copyAttr.bends(eCopy);
			if (GC.isReversed(eOrig)) {
				for (auto bendPoint : reverse(bends(eOrig))) {
					copyBends.pushBack(bendPoint);
				}
			} else {
				for (auto bendPoint : bends(eOrig)) {
					copyBends.pushBack(bendPoint);
				}
			}
			copyBends.normalize();
		}

	}
}

}
