/** \file
 * \brief Generator for visualizing graphs using the XML-based SVG format.
 *
 * \author Tilo Wiedera
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

#include <algorithm>
#include <cmath>

#include <ogdf/fileformats/SvgPrinter.h>
#include <ogdf/basic/Queue.h>

using namespace ogdf;

GraphIO::SVGSettings GraphIO::svgSettings;

GraphIO::SVGSettings::SVGSettings()
{
	m_margin = 1;
	m_curviness = 0;
	m_bezierInterpolation = false;
	m_fontSize = 10;
	m_fontColor = "#000000";
	m_fontFamily = "Arial";
	m_width = "";
	m_height = "";
}

bool SvgPrinter::draw(std::ostream &os)
{
	pugi::xml_document doc;
	pugi::xml_node rootNode = writeHeader(doc);

	if(m_clsAttr) {
		drawClusters(rootNode);
	}

	drawEdges(rootNode);
	drawNodes(rootNode);

	doc.save(os);

	return true;
}

pugi::xml_node SvgPrinter::writeHeader(pugi::xml_document &doc)
{
	pugi::xml_node rootNode = doc.append_child("svg");
	rootNode.append_attribute("xmlns") = "http://www.w3.org/2000/svg";
	rootNode.append_attribute("xmlns:xlink") = "http://www.w3.org/1999/xlink";
	rootNode.append_attribute("xmlns:ev") = "http://www.w3.org/2001/xml-events";
	rootNode.append_attribute("version") = "1.1";
	rootNode.append_attribute("baseProfile") = "full";

	if(!m_settings.width().empty()) {
		rootNode.append_attribute("width") = m_settings.width().c_str();
	}

	if(!m_settings.height().empty()) {
		rootNode.append_attribute("height") = m_settings.height().c_str();
	}

	DRect box = m_clsAttr ? m_clsAttr->boundingBox() : m_attr.boundingBox();

	double margin = m_settings.margin();
	std::stringstream is;
	is << (box.p1().m_x - margin);
	is << " " << (box.p1().m_y - margin);
	is << " " << (box.width() + 2*margin);
	is << " " << (box.height() + 2*margin);
	rootNode.append_attribute("viewBox") = is.str().c_str();

	return rootNode;
}

void SvgPrinter::writeDashArray(pugi::xml_node xmlNode, StrokeType lineStyle, double lineWidth)
{
	if(lineStyle != StrokeType::None && lineStyle != StrokeType::Solid) {
		std::stringstream is;

		switch(lineStyle) {
		case StrokeType::Dash:
			is << 4*lineWidth << "," << 2*lineWidth;
			break;
		case StrokeType::Dot:
			is << 1*lineWidth << "," << 2*lineWidth;
			break;
		case StrokeType::Dashdot:
			is << 4*lineWidth << "," << 2*lineWidth << "," << 1*lineWidth << "," << 2*lineWidth;
			break;
		case StrokeType::Dashdotdot:
			is << 4*lineWidth << "," << 2*lineWidth << "," << 1*lineWidth << "," << 2*lineWidth << "," << 1*lineWidth << "," << 2*lineWidth;
			break;
		default:
			// will never happen
			break;
		}

		xmlNode.append_attribute("stroke-dasharray") = is.str().c_str();
	}
}

void SvgPrinter::drawNode(pugi::xml_node xmlNode, node v)
{
#if 1
	const double
	  hexagonHalfHeight = 0.43301270189222 * m_attr.height(v),
	  pentagonHalfWidth = 0.475528258147577 * m_attr.width(v),
	  pentagonSmallHeight = 0.154508497187474 * m_attr.height(v),
	  pentagonSmallWidth = 0.293892626146236 * m_attr.width(v),
	  pentagonHalfHeight = 0.404508497187474 * m_attr.height(v),
	  octagonHalfWidth = 0.461939766255643 * m_attr.width(v),
	  octagonSmallWidth = 0.191341716182545 * m_attr.width(v),
	  octagonHalfHeight  = 0.461939766255643 * m_attr.height(v),
	  octagonSmallHeight = 0.191341716182545 * m_attr.height(v);
#endif
	pugi::xml_node shape;
	std::stringstream is;
	double x = m_attr.x(v);
	double y = m_attr.y(v);
	xmlNode = xmlNode.append_child("g");

	// values are precomputed to save expensive sin/cos calls
	switch (m_attr.shape(v)) {
	case Shape::Ellipse:
		shape = xmlNode.append_child("ellipse");
		shape.append_attribute("cx") = x;
		shape.append_attribute("cy") = y;
		shape.append_attribute("rx") = m_attr.width(v) / 2;
		shape.append_attribute("ry") = m_attr.height(v) / 2;
		break;
	case Shape::Triangle:
		shape = drawPolygon(xmlNode, {
					x, y - m_attr.height(v)/2,
					x - m_attr.width(v)/2, y + m_attr.height(v)/2,
					x + m_attr.width(v)/2, y + m_attr.height(v)/2
				});
		break;
	case Shape::InvTriangle:
		shape = drawPolygon(xmlNode, {x, y + m_attr.height(v)/2,
					x - m_attr.width(v)/2, y - m_attr.height(v)/2,
					x + m_attr.width(v)/2,y - m_attr.height(v)/2
				});
		break;
	case Shape::Pentagon:
		shape = drawPolygon(xmlNode, {
					x, y - m_attr.height(v)/2,
					x + pentagonHalfWidth, y - pentagonSmallHeight,
					x + pentagonSmallWidth, y + pentagonHalfHeight,
					x - pentagonSmallWidth, y + pentagonHalfHeight,
					x - pentagonHalfWidth, y - pentagonSmallHeight
				});
		break;
	case Shape::Hexagon:
		shape = drawPolygon(xmlNode, {
					x + m_attr.width(v)/4, y + hexagonHalfHeight,
					x - m_attr.width(v)/4, y + hexagonHalfHeight,
					x - m_attr.width(v)/2, y,
					x - m_attr.width(v)/4, y - hexagonHalfHeight,
					x + m_attr.width(v)/4, y - hexagonHalfHeight,
					x + m_attr.width(v)/2, y
				});
		break;
	case Shape::Octagon:
		shape = drawPolygon(xmlNode, {
					x + octagonHalfWidth, y + octagonSmallHeight,
					x + octagonSmallWidth, y + octagonHalfHeight,
					x - octagonSmallWidth, y + octagonHalfHeight,
					x - octagonHalfWidth, y + octagonSmallHeight,
					x - octagonHalfWidth, y - octagonSmallHeight,
					x - octagonSmallWidth, y - octagonHalfHeight,
					x + octagonSmallWidth, y - octagonHalfHeight,
					x + octagonHalfWidth, y - octagonSmallHeight
				});
		break;
	case Shape::Rhomb:
		shape = drawPolygon(xmlNode, {
					x + m_attr.width(v)/2, y,
					x, y + m_attr.height(v)/2,
					x - m_attr.width(v)/2, y,
					x, y - m_attr.height(v)/2
				});
		break;
	case Shape::Trapeze:
		shape = drawPolygon(xmlNode, {
					x - m_attr.width(v)/2, y + m_attr.height(v)/2,
					x + m_attr.width(v)/2, y + m_attr.height(v)/2,
					x + m_attr.width(v)/4, y - m_attr.height(v)/2,
					x - m_attr.width(v)/4, y - m_attr.height(v)/2
				});
		break;
	case Shape::InvTrapeze:
		shape = drawPolygon(xmlNode, {
					x - m_attr.width(v)/2, y - m_attr.height(v)/2,
					x + m_attr.width(v)/2, y - m_attr.height(v)/2,
					x + m_attr.width(v)/4, y + m_attr.height(v)/2,
					x - m_attr.width(v)/4, y + m_attr.height(v)/2
				});
		break;
	case Shape::Parallelogram:
		shape = drawPolygon(xmlNode, {
					x - m_attr.width(v)/2, y + m_attr.height(v)/2,
					x + m_attr.width(v)/4, y + m_attr.height(v)/2,
					x + m_attr.width(v)/2, y - m_attr.height(v)/2,
					x - m_attr.width(v)/4, y - m_attr.height(v)/2
				});
		break;
	case Shape::InvParallelogram:
		shape = drawPolygon(xmlNode, {
					x - m_attr.width(v)/2, y - m_attr.height(v)/2,
					x + m_attr.width(v)/4, y - m_attr.height(v)/2,
					x + m_attr.width(v)/2, y + m_attr.height(v)/2,
					x - m_attr.width(v)/4, y + m_attr.height(v)/2
				});
		break;
	// unsupported shapes are rendered as rectangle
	default:
		shape = xmlNode.append_child("rect");
		shape.append_attribute("x") = x - m_attr.width(v)/2;
		shape.append_attribute("y") = y - m_attr.height(v)/2;
		shape.append_attribute("width") = m_attr.width(v);
		shape.append_attribute("height") = m_attr.height(v);

		if (m_attr.shape(v) == Shape::RoundedRect) {
			shape.append_attribute("rx") = m_attr.width(v) / 10;
			shape.append_attribute("ry") = m_attr.height(v) / 10;
		}
	}

	if (m_attr.has(GraphAttributes::nodeStyle)) {
		shape.append_attribute("fill") = m_attr.fillColor(v).toString().c_str();
		shape.append_attribute("stroke-width") = (to_string(m_attr.strokeWidth(v)) + "px").c_str();

		StrokeType lineStyle = m_attr.has(GraphAttributes::nodeStyle) ? m_attr.strokeType(v) : StrokeType::Solid;

		if(lineStyle == StrokeType::None) {
			shape.append_attribute("stroke") = "none";
		} else {
			shape.append_attribute("stroke") = m_attr.strokeColor(v).toString().c_str();
			writeDashArray(shape, lineStyle, m_attr.strokeWidth(v));
		}
	}

	if (m_attr.has(GraphAttributes::nodeLabel)) {
		pugi::xml_node label = xmlNode.append_child("text");
		label.append_attribute("x") = m_attr.x(v);
		label.append_attribute("y") = m_attr.y(v);
		label.append_attribute("text-anchor") = "middle";
		label.append_attribute("dominant-baseline") = "middle";
		label.append_attribute("font-family") = m_settings.fontFamily().c_str();
		label.append_attribute("font-size") = m_settings.fontSize();
		label.append_attribute("fill") = m_settings.fontColor().c_str();
		label.text() = m_attr.label(v).c_str();

		if(m_attr.has(GraphAttributes::nodeLabelPosition)) {
			label.attribute("x") = m_attr.x(v) + m_attr.xLabel(v);
			label.attribute("y") = m_attr.y(v) + m_attr.yLabel(v);
		}
	}
}

void SvgPrinter::drawCluster(pugi::xml_node xmlNode, cluster c)
{
	OGDF_ASSERT(m_clsAttr);

	pugi::xml_node cluster;

	if (c == m_clsAttr->constClusterGraph().rootCluster()) {
		cluster = xmlNode;
	} else {
		pugi::xml_node clusterXmlNode = xmlNode.append_child("rect");
		if (m_clsAttr->has(ClusterGraphAttributes::clusterGraphics)) {
			clusterXmlNode.append_attribute("x") = m_clsAttr->x(c);
			clusterXmlNode.append_attribute("y") = m_clsAttr->y(c);
			clusterXmlNode.append_attribute("width") = m_clsAttr->width(c);
			clusterXmlNode.append_attribute("height") = m_clsAttr->height(c);
		}
		if (m_clsAttr->has(ClusterGraphAttributes::clusterStyle)) {
			clusterXmlNode.append_attribute("fill") = m_clsAttr->fillPattern(c) == FillPattern::None ? "none" : m_clsAttr->fillColor(c).toString().c_str();
			clusterXmlNode.append_attribute("stroke") = m_clsAttr->strokeType(c) == StrokeType::None ? "none" : m_clsAttr->strokeColor(c).toString().c_str();
			clusterXmlNode.append_attribute("stroke-width") = (to_string(m_clsAttr->strokeWidth(c)) + "px").c_str();
		}
	}
}

void SvgPrinter::drawNodes(pugi::xml_node xmlNode)
{
	List<node> nodes;
	m_attr.constGraph().allNodes(nodes);

	if (m_attr.has(GraphAttributes::nodeGraphics | GraphAttributes::threeD)) {
		nodes.quicksort(GenericComparer<node, double>([&](node v) { return m_attr.z(v); }));
	}

	for(node v : nodes) {
		drawNode(xmlNode, v);
	}
}

void SvgPrinter::drawClusters(pugi::xml_node xmlNode)
{
	OGDF_ASSERT(m_clsAttr);

	Queue<cluster> queue;
	queue.append(m_clsAttr->constClusterGraph().rootCluster());

	while(!queue.empty()) {
		cluster c = queue.pop();
		drawCluster(xmlNode.append_child("g"), c);

		for(cluster child : c->children) {
			queue.append(child);
		}
	}
}

void SvgPrinter::drawEdges(pugi::xml_node xmlNode)
{
	if (m_attr.has(GraphAttributes::edgeGraphics)) {
		xmlNode = xmlNode.append_child("g");

		for(edge e : m_attr.constGraph().edges) {
			drawEdge(xmlNode, e);
		}
	}
}

void SvgPrinter::appendLineStyle(pugi::xml_node line, edge e) {

	StrokeType lineStyle = m_attr.has(GraphAttributes::edgeStyle) ? m_attr.strokeType(e) : StrokeType::Solid;

	if(lineStyle != StrokeType::None) {
		if (m_attr.has(GraphAttributes::edgeStyle)) {
			line.append_attribute("stroke") = m_attr.strokeColor(e).toString().c_str();
			line.append_attribute("stroke-width") = (to_string(m_attr.strokeWidth(e)) + "px").c_str();
			writeDashArray(line, lineStyle, m_attr.strokeWidth(e));
		} else {
			line.append_attribute("stroke") = "#000000";
		}
	}
}

pugi::xml_node SvgPrinter::drawPolygon(pugi::xml_node xmlNode, const std::list<double> points) {
	pugi::xml_node result = xmlNode.append_child("polygon");
	OGDF_ASSERT(points.size() % 2 == 0);

	std::stringstream is;
	bool writeSpace = false;

	for(double p : points) {
		is << p << (writeSpace ? " " : ",");
	}

	result.append_attribute("points") = is.str().c_str();

	return result;
}

bool SvgPrinter::isArrowEnabled(adjEntry adj) {
	bool result = false;

	if(m_attr.has(GraphAttributes::edgeArrow)) {
		switch(m_attr.arrowType(*adj)) {
			case EdgeArrow::Undefined:
				result = !adj->isSource() && m_attr.directed();
				break;
			case EdgeArrow::First:
				result = adj->isSource();
				break;
			case EdgeArrow::Last:
				result = !adj->isSource();
				break;
			case EdgeArrow::Both:
				result = true;
				break;
			case EdgeArrow::None:
				;
		}
	} else {
		result = !adj->isSource() && m_attr.directed();
	}

	return result;
}

double SvgPrinter::getArrowSize(adjEntry adj) {
	double result = 0;

	if(isArrowEnabled(adj)) {
		const double minSize = (m_attr.has(GraphAttributes::edgeStyle) ? m_attr.strokeWidth(adj->theEdge()) : 1) * 3;
		node v = adj->theNode();
		node w = adj->twinNode();
		result = std::max(minSize, (m_attr.width(v) + m_attr.height(v) + m_attr.width(w) + m_attr.height(w)) / 16.0);
	}

	return result;
}

bool SvgPrinter::isCoveredBy(const DPoint &point, adjEntry adj) {
	double arrowSize = getArrowSize(adj);
	node v = adj->theNode();

	return point.m_x >= m_attr.x(v) - m_attr.width(v)/2 - arrowSize
	    && point.m_x <= m_attr.x(v) + m_attr.width(v)/2 + arrowSize
	    && point.m_y >= m_attr.y(v) - m_attr.height(v)/2 - arrowSize
	    && point.m_y <= m_attr.y(v) + m_attr.height(v)/2 + arrowSize;
}

void SvgPrinter::drawEdge(pugi::xml_node xmlNode, edge e) {
	bool drawSourceArrow = isArrowEnabled(e->adjSource());
	bool drawTargetArrow = isArrowEnabled(e->adjTarget());

	xmlNode = xmlNode.append_child("g");
	bool drawLabel = m_attr.has(GraphAttributes::edgeLabel) && !m_attr.label(e).empty();
	pugi::xml_node label;

	if(drawLabel) {
		label = xmlNode.append_child("text");
		label.append_attribute("text-anchor") = "middle";
		label.append_attribute("dominant-baseline") = "middle";
		label.append_attribute("font-family") = m_settings.fontFamily().c_str();
		label.append_attribute("font-size") = m_settings.fontSize();
		label.append_attribute("fill") = m_settings.fontColor().c_str();
		label.text() = m_attr.label(e).c_str();
	}

	DPolyline path = m_attr.bends(e);
	node s = e->source();
	node t = e->target();
	path.pushFront(m_attr.point(s));
	path.pushBack(m_attr.point(t));

	bool drawSegment = false;
	bool finished = false;

	List<DPoint> points;

	for(ListConstIterator<DPoint> it = path.begin(); it.succ().valid() && !finished; it++) {
		DPoint p1 = *it;
		DPoint p2 = *(it.succ());

		// leaving segment at source node ?
		if(isCoveredBy(p1, e->adjSource()) && !isCoveredBy(p2, e->adjSource())) {
			if(!drawSegment && drawSourceArrow) {
				drawArrowHead(xmlNode, p2, p1, e->adjSource());
			}

			drawSegment = true;
		}

		// entering segment at target node ?
		if(!isCoveredBy(p1, e->adjTarget()) && isCoveredBy(p2, e->adjTarget())) {
			finished = true;

			if(drawTargetArrow) {
				drawArrowHead(xmlNode, p1, p2, e->adjTarget());
			}
		}

		if(drawSegment && drawLabel) {
			label.append_attribute("x") = (p1.m_x + p2.m_x) / 2;
			label.append_attribute("y") = (p1.m_y + p2.m_y) / 2;

			drawLabel = false;
		}

		if(drawSegment) {
			points.pushBack(p1);
		}

		if(finished) {
			points.pushBack(p2);
		}
	}

	if(points.size() < 2) {
		GraphIO::logger.lout() << "Could not draw edge since nodes are overlapping: " << e << std::endl;
	} else {
		drawCurve(xmlNode, e, points);
	}
}

void SvgPrinter::drawLine(std::stringstream &ss, const DPoint &p1, const DPoint &p2) {
	ss << " M" << p1.m_x << "," << p1.m_y << " L" << p2.m_x << "," << p2.m_y;
}


void SvgPrinter::drawBezier(std::stringstream &ss, const DPoint &p1, const DPoint &p2, const DPoint &c1, const DPoint &c2) {
	ss << " M" << p1.m_x << "," << p1.m_y << " C" << c1.m_x << "," << c1.m_y << "  " << c2.m_x << "," << c2.m_y << " " << p2.m_x << "," << p2.m_y;
}

void SvgPrinter::drawBezierPath(std::stringstream &ss, List<DPoint> &points) {
	const double c = m_settings.curviness();
	DPoint cLast = 0.5 * (points.front() + *points.get(1));

	while(points.size() >= 3) {
		const DPoint p1 = points.popFrontRet();
		const DPoint p2 = points.front();
		const DPoint p3 = *points.get(1);

		const DPoint delta = p2 - 0.5 * (p1+p3);
		const DPoint c1 = p1 + c * delta + (1-c) * (p2-p1);
		const DPoint c2 = p3 + c * delta + (1-c) * (p2-p3);

		drawBezier(ss, p1, p2, cLast, c1);

		cLast = c2;
	}

	const DPoint p1 = points.popFrontRet();
	const DPoint p2 = points.popFrontRet();
	const DPoint c1 = 0.5 * (p2 + p1);

	drawBezier(ss, p1, p2, cLast, c1);
}

void SvgPrinter::drawRoundPath(std::stringstream &ss, List<DPoint> &points) {
	const double c = m_settings.curviness();

	DPoint p1 = points.front();
	DPoint p2 = *points.get(1);

	drawLine(ss, p1, .5 * ((p1+p2) + (1-c) * (p2-p1)));

	while(points.size() >= 3) {
		p1 = points.popFrontRet();
		p2 = points.front();
		DPoint p3 = *points.get(1);

		DPoint v1 = (p1 - p2);
		DPoint v2 = (p3 - p2);

		double length = std::min(v1.norm(), v2.norm()) * c / 2;

		v1 *= length / v1.norm();
		v2 *= length / v2.norm();

		DPoint pA = p2 + v1;
		DPoint pB = p2 + v2;

		drawLine(ss, 0.5 * (p1+p2), pA);
		drawLine(ss, 0.5 * (p3+p2), pB);

		DPoint vA = p2 - p1;
		DPoint vB = p3 - p1;
		bool doSweep = vA.m_x*vB.m_y - vA.m_y*vB.m_x > 0;

		ss << " M" << pA.m_x << "," << pA.m_y << " A" << length << "," << length << " 0 0 " << (doSweep ? 1 : 0) << " " << pB.m_x << "," << pB.m_y << "";
	}

	p1 = points.popFrontRet();
	p2 = points.popFrontRet();

	drawLine(ss, p2, .5 * ((p1 + p2) + (1-c) * (p1-p2)));
}

void SvgPrinter::drawLines(std::stringstream &ss, List<DPoint> &points) {
	while(points.size() > 1) {
		DPoint p = points.popFrontRet();
		drawLine(ss, p, points.front());
	}
}

pugi::xml_node SvgPrinter::drawCurve(pugi::xml_node xmlNode, edge e, List<DPoint> &points) {
	OGDF_ASSERT(points.size() >= 2);

	pugi::xml_node line = xmlNode.append_child("path");
	std::stringstream ss;

	if(points.size() == 2) {
		const DPoint p1 = points.popFrontRet();
		const DPoint p2 = points.popFrontRet();

		drawLine(ss, p1, p2);
	} else {
		if(m_settings.curviness() == 0) {
			drawLines(ss, points);
		} else if(m_settings.bezierInterpolation()) {
			drawBezierPath(ss, points);
		} else {
			drawRoundPath(ss, points);
		}
	}

	line.append_attribute("fill") = "none";
	line.append_attribute("d") = ss.str().c_str();
	appendLineStyle(line, e);

	return line;
}

void SvgPrinter::drawArrowHead(pugi::xml_node xmlNode, const DPoint &start, DPoint &end, adjEntry adj)
{
	const double dx = end.m_x - start.m_x;
	const double dy = end.m_y - start.m_y;
	const double size = getArrowSize(adj);
	node v = adj->theNode();

	pugi::xml_node arrowHead;

	if(dx == 0) {
		int sign = dy > 0 ? 1 : -1;
		double y = m_attr.y(v) - m_attr.height(v)/2 * sign;
		end.m_y = y - sign * size;

		arrowHead = drawPolygon(xmlNode, {
				end.m_x, y,
				end.m_x - size/4, y - size*sign,
				end.m_x + size/4, y - size*sign
		});
	} else {
		// identify the position of the tip

		double slope = dy / dx;
		int sign = dx > 0 ? 1 : -1;

		double x = m_attr.x(v) - m_attr.width(v)/2 * sign;
		double delta = x - start.m_x;
		double y = start.m_y + delta*slope;

		if(!isCoveredBy(DPoint(x,y), adj)) {
			sign = dy > 0 ? 1 : -1;
			y = m_attr.y(v) - m_attr.height(v)/2 * sign;
			delta = y - start.m_y;
			x = start.m_x + delta/slope;
		}

		end.m_x = x;
		end.m_y = y;

		// draw the actual arrow head

		double dx2 = end.m_x - start.m_x;
		double dy2 = end.m_y - start.m_y;
		double length = std::sqrt(dx2*dx2 + dy2*dy2);
		dx2 /= length;
		dy2 /= length;

		double mx = end.m_x - size * dx2;
		double my = end.m_y - size * dy2;

		double x2 = mx - size/4 * dy2;
		double y2 = my + size/4 * dx2;

		double x3 = mx + size/4 * dy2;
		double y3 = my - size/4 * dx2;

		arrowHead = drawPolygon(xmlNode, {end.m_x, end.m_y, x2, y2, x3, y3});
	}

	appendLineStyle(arrowHead, *adj);
}
