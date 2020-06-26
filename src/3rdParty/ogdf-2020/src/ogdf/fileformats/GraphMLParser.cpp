/** \file
 * \brief Implementation of GraphML parser.
 *
 * \author Łukasz Hanuszczak, Tilo Wiedera
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

#include <ogdf/fileformats/GraphMLParser.h>
#include <ogdf/fileformats/GraphML.h>

namespace ogdf {


GraphMLParser::GraphMLParser(std::istream &in) : m_error(false)
{
	pugi::xml_parse_result result = m_xml.load(in);

	if (!result) {
		GraphIO::logger.lout() << "XML parser error: " << result.description() << std::endl;
		m_error = true;
		return;
	}

	pugi::xml_node root = m_xml.child("graphml");

	if(!root) {
		GraphIO::logger.lout() << "File root tag is not a <graphml>." << std::endl;
		m_error = true;
		return;
	}

	m_graphTag = root.child("graph");

	if (!m_graphTag) {
		GraphIO::logger.lout() << "<graph> tag not found." << std::endl;
		m_error = true;
		return;
	}

	for (const pugi::xml_node &keyTag : root.children("key")) {
		pugi::xml_attribute idAttr = keyTag.attribute("id");
		pugi::xml_attribute nameAttr = keyTag.attribute("attr.name");

		if (!idAttr) {
			GraphIO::logger.lout() << "Key does not have an id attribute." << std::endl;
			m_error = true;
			return;
		}
		if (!nameAttr) {
			GraphIO::logger.lout() << "Key does not have an attr.name attribute." << std::endl;
			m_error = true;
			return;
		}

		m_attrName[idAttr.value()] = nameAttr.value();
	}
}


GraphMLParser::~GraphMLParser()
{
}


bool GraphMLParser::readData(
	GraphAttributes &GA,
	const node &v,
	const pugi::xml_node nodeData)
{
	pugi::xml_attribute keyId = nodeData.attribute("key");

	if (!keyId) {
		GraphIO::logger.lout() << "Node data does not have a key." << std::endl;
		return false;
	}

	const long attrs = GA.attributes();

	pugi::xml_text text = nodeData.text();

	switch (graphml::toAttribute(m_attrName[keyId.value()])) {
	case graphml::Attribute::NodeId:
		if(attrs & GraphAttributes::nodeId) {
			GA.idNode(v) = text.as_int();
		}
		break;
	case graphml::Attribute::NodeLabel:
		if(attrs & GraphAttributes::nodeLabel) {
			GA.label(v) = text.get();
		}
		break;
	case graphml::Attribute::X:
		if(attrs & GraphAttributes::nodeGraphics) {
			GA.x(v) = text.as_double();
		}
		break;
	case graphml::Attribute::Y:
		if(attrs & GraphAttributes::nodeGraphics) {
			GA.y(v) = text.as_double();;
		}
		break;
	case graphml::Attribute::Width:
		if(attrs & GraphAttributes::nodeGraphics) {
			GA.width(v) = text.as_double();
		}
		break;
	case graphml::Attribute::Height:
		if(attrs & GraphAttributes::nodeGraphics) {
			GA.height(v) = text.as_double();
		}
		break;
	case graphml::Attribute::Size:
		if(attrs & GraphAttributes::nodeGraphics) {
			double size = text.as_double();

			// We want to set a new size only if width and height was not set.
			if (GA.height(v) == GA.width(v)) {
				GA.height(v) = GA.width(v) = size;
			}
		}
		break;
	case graphml::Attribute::Shape:
		if(attrs & GraphAttributes::nodeGraphics) {
			GA.shape(v) = graphml::toShape(text.get());
		}
		break;
	case graphml::Attribute::Z:
		if(attrs & GraphAttributes::threeD) {
			GA.z(v) = text.as_double();
		}
		break;
	case graphml::Attribute::NodeLabelX:
		if (attrs & GraphAttributes::nodeLabelPosition) {
			GA.xLabel(v) = text.as_double();
		}
		break;
	case graphml::Attribute::NodeLabelY:
		if (attrs & GraphAttributes::nodeLabelPosition) {
			GA.yLabel(v) = text.as_double();
		}
		break;
	case graphml::Attribute::NodeLabelZ:
		if (attrs & GraphAttributes::nodeLabelPosition && attrs & GraphAttributes::threeD) {
			GA.zLabel(v) = text.as_double();
		}
		break;
	case graphml::Attribute::R:
		if (attrs & GraphAttributes::nodeStyle
		 && !GraphIO::setColorValue(text.as_int(), [&](uint8_t val) { GA.fillColor(v).red(val); })) {
			return false;
		}
		break;
	case graphml::Attribute::G:
		if(attrs & GraphAttributes::nodeStyle
		 && !GraphIO::setColorValue(text.as_int(), [&](uint8_t val) { GA.fillColor(v).green(val); })) {
			return false;
		}
		break;
	case graphml::Attribute::B:
		if(attrs & GraphAttributes::nodeStyle
		 && !GraphIO::setColorValue(text.as_int(), [&](uint8_t val) { GA.fillColor(v).blue(val); })) {
			return false;
		}
		break;
	case graphml::Attribute::NodeFillPattern:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.fillPattern(v) = FillPattern(text.as_int());
		}
		break;
	case graphml::Attribute::NodeFillBackground:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.fillBgColor(v) = text.get();
		}
		break;
	case graphml::Attribute::NodeStrokeColor:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.strokeColor(v) = text.get();
		}
		break;
	case graphml::Attribute::NodeStrokeType:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.strokeType(v) = StrokeType(text.as_int());
		}
		break;
	case graphml::Attribute::NodeStrokeWidth:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.strokeWidth(v) = text.as_float();
		}
		break;
	case graphml::Attribute::NodeType:
		if(attrs & GraphAttributes::nodeType) {
			GA.type(v) = Graph::NodeType(text.as_int());
		}
		break;
	case graphml::Attribute::Template:
		if(attrs & GraphAttributes::nodeTemplate) {
			GA.templateNode(v) = text.get();
		}
		break;
	case graphml::Attribute::NodeWeight:
		if(attrs & GraphAttributes::nodeWeight) {
			GA.weight(v) = text.as_int();
		}
		break;
	default:
		GraphIO::logger.lout(Logger::Level::Minor) << "Unknown node attribute: \"" << keyId.value() << "\"." << std::endl;
	}

	return true;
}


bool GraphMLParser::readData(
	GraphAttributes &GA,
	const edge &e,
	const pugi::xml_node edgeData)
{
	pugi::xml_attribute keyId = edgeData.attribute("key");
	if (!keyId) {
		GraphIO::logger.lout() << "Edge data does not have a key." << std::endl;
		return false;
	}

	const long attrs = GA.attributes();
	pugi::xml_text text = edgeData.text();

	switch(graphml::toAttribute(m_attrName[keyId.value()])) {
	case graphml::Attribute::EdgeLabel:
		if(attrs & GraphAttributes::edgeLabel) {
			GA.label(e) = text.get();
		}
		break;
	case graphml::Attribute::EdgeWeight:
		if(attrs & GraphAttributes::edgeDoubleWeight) {
			GA.doubleWeight(e) = text.as_double();
		} else if(attrs & GraphAttributes::edgeIntWeight) {
			GA.intWeight(e) = text.as_int();
		}
		break;
	case graphml::Attribute::EdgeType:
		if(attrs & GraphAttributes::edgeType) {
			GA.type(e) = graphml::toEdgeType(text.get());
		}
		break;
	case graphml::Attribute::EdgeArrow:
		if(attrs & GraphAttributes::edgeArrow) {
			GA.arrowType(e) = graphml::toArrow(text.get());
		}
		break;
	case graphml::Attribute::EdgeStrokeColor:
		if(attrs & GraphAttributes::edgeStyle) {
			GA.strokeColor(e) = text.get();
		}
		break;
	case graphml::Attribute::EdgeStrokeType:
		if(attrs & GraphAttributes::edgeStyle) {
			GA.strokeType(e) = StrokeType(text.as_int());
		}
		break;
	case graphml::Attribute::EdgeStrokeWidth:
		if(attrs & GraphAttributes::edgeStyle) {
			GA.strokeWidth(e) = text.as_float();
		}
		break;
	case graphml::Attribute::EdgeBends:
		if(attrs & GraphAttributes::edgeGraphics) {
			std::stringstream is(text.get());
			double x, y;
			DPolyline& polyline = GA.bends(e);
			polyline.clear();
			while(is >> x && is >> y) {
				polyline.pushBack(DPoint(x, y));
			}
		}
		break;
	case graphml::Attribute::EdgeSubGraph:
		if(attrs & GraphAttributes::edgeSubGraphs) {
			std::stringstream sstream(text.get());
			int sg;
			while(sstream >> sg) {
				GA.addSubGraph(e, sg);
			}
		}
		break;
	default:
		GraphIO::logger.lout(Logger::Level::Minor) << "Unknown edge attribute with \""
		             << keyId.value()
		             << "\"." << std::endl;
	}

	return true;
}


bool GraphMLParser::readData(
	ClusterGraphAttributes &CA,
	const cluster &c,
	const pugi::xml_node clusterData)
{
	auto keyId = clusterData.attribute("key");
	if (!keyId) {
		GraphIO::logger.lout() << "Cluster data does not have a key." << std::endl;
		return false;
	}

	pugi::xml_text text = clusterData.text();

	using namespace graphml;
	switch (toAttribute(m_attrName[keyId.value()])) {
	case Attribute::NodeLabel:
		CA.label(c) = text.get();
		break;
	case Attribute::X:
		CA.x(c) = text.as_double();
		break;
	case Attribute::Y:
		CA.y(c) = text.as_double();
		break;
	case Attribute::Width:
		CA.width(c) = text.as_double();
		break;
	case Attribute::Height:
		CA.height(c) = text.as_double();
		break;
	case Attribute::Size:
		// We want to set a new size only if width and height was not set.
		if (CA.width(c) == CA.height(c)) {
			CA.width(c) = CA.height(c) = text.as_double();
		}
		break;
	case Attribute::R:
		if (!GraphIO::setColorValue(text.as_int(), [&](uint8_t val) { CA.fillColor(c).red(val); })) {
			return false;
		}
		break;
	case Attribute::G:
		if (!GraphIO::setColorValue(text.as_int(), [&](uint8_t val) { CA.fillColor(c).green(val); })) {
			return false;
		}
		break;
	case Attribute::B:
		if (!GraphIO::setColorValue(text.as_int(), [&](uint8_t val) { CA.fillColor(c).blue(val); })) {
			return false;
		}
		break;
	case Attribute::ClusterStroke:
		CA.strokeColor(c) = text.get();
		break;
	default:
		GraphIO::logger.lout(Logger::Level::Minor) << "Unknown cluster attribute with \""
		             << keyId.value()
		             << "--enum: " << m_attrName[keyId.value()] << "--"
		             << "\"." << std::endl;
	}

	return true;
}


bool GraphMLParser::readNodes(
	Graph &G,
	GraphAttributes *GA,
	const pugi::xml_node rootTag)
{
	for(pugi::xml_node nodeTag : rootTag.children("node")) {
		pugi::xml_attribute idAttr = nodeTag.attribute("id");
		if(!idAttr) {
			GraphIO::logger.lout() << "Node is missing id attribute." << std::endl;
			return false;
		}

		const node v = G.newNode();
		m_nodeId[idAttr.value()] = v;

		// Search for data-key attributes if GA given.
		if(GA && !readAttributes(*GA, v, nodeTag)) {
			return false;
		}

		pugi::xml_node clusterTag = nodeTag.child("graph");
		if (clusterTag) {
			GraphIO::logger.lout(Logger::Level::Minor) << "Nested graphs are not fully supported." << std::endl;
			return readNodes(G, GA, clusterTag);
		}
	}

	return readEdges(G, GA, rootTag);
}


bool GraphMLParser::readEdges(
	Graph &G,
	GraphAttributes *GA,
	const pugi::xml_node rootTag)
{
	for (pugi::xml_node edgeTag : rootTag.children("edge")) {
		pugi::xml_attribute sourceId = edgeTag.attribute("source");
		pugi::xml_attribute targetId = edgeTag.attribute("target");

		if (!sourceId) {
			GraphIO::logger.lout() << "Edge is missing source node." << std::endl;
			return false;
		}
		if (!targetId) {
			GraphIO::logger.lout() << "Edge is missing target node." << std::endl;
			return false;
		}

		auto sourceIt = m_nodeId.find(sourceId.value());
		if (sourceIt == std::end(m_nodeId)) {
			GraphIO::logger.lout() << "Edge source node \""
			           << sourceId.value()
			           << "\" is incorrect.\n" << std::endl;
			return false;
		}

		auto targetIt = m_nodeId.find(targetId.value());
		if (targetIt == std::end(m_nodeId)) {
			GraphIO::logger.lout() << "Edge source node \""
			           << targetId.value()
			           << "\" is incorrect.\n" << std::endl;
			return false;
		}

		const edge e = G.newEdge(sourceIt->second, targetIt->second);

		// Search for data-key attributes if GA given, return false on error.
		if(GA && !readAttributes(*GA, e, edgeTag)) {
			return false;
		}
	}

	return true;
}


bool GraphMLParser::readClusters(
	Graph &G,
	ClusterGraph &C,
	ClusterGraphAttributes *CA,
	const cluster &rootCluster,
	const pugi::xml_node rootTag)
{
	for(pugi::xml_node nodeTag : rootTag.children("node")) {
		pugi::xml_attribute idAttr = nodeTag.attribute("id");
		pugi::xml_node clusterTag = nodeTag.child("graph");

		if (clusterTag == nullptr) {
			// Got normal node then, add it to the graph - id is required.
			if (!idAttr) {
				GraphIO::logger.lout() << "Node is missing id attribute." << std::endl;
				return false;
			}

			const node v = G.newNode();
			m_nodeId[idAttr.value()] = v;
			C.reassignNode(v, rootCluster);

			// Read attributes when CA given and return false if error.
			if(CA && !readAttributes(*CA, v, nodeTag)) {
				return false;
			}
		} else {
			// Got a cluster node - read it recursively.
			const cluster c = C.newCluster(rootCluster);
			if (!readClusters(G, C, CA, c, clusterTag)) {
				return false;
			}

			// Read attributes when CA given and return false if error.
			if(CA && !readAttributes(*CA, c, nodeTag)) {
				return false;
			}
		}
	}

	return readEdges(G, CA, rootTag);
}


bool GraphMLParser::read(Graph &G)
{
	if(m_error) {
		return false;
	}

	G.clear();
	m_nodeId.clear();

	return readNodes(G, nullptr, m_graphTag);
}


bool GraphMLParser::read(Graph &G, GraphAttributes &GA)
{
	// Check whether graph is directed or not (directed by default).
	pugi::xml_attribute edgeDefaultAttr = m_graphTag.attribute("edgedefault");
	GA.directed() = (!edgeDefaultAttr || string(edgeDefaultAttr.value()) == "directed");

	if(m_error) {
		return false;
	}

	G.clear();
	m_nodeId.clear();

	return readNodes(G, &GA, m_graphTag);
}


bool GraphMLParser::read(Graph &G, ClusterGraph &C)
{
	if(m_error) {
		return false;
	}

	G.clear();
	m_nodeId.clear();

	return readClusters(G, C, nullptr, C.rootCluster(), m_graphTag);
}


bool GraphMLParser::read(Graph &G, ClusterGraph &C, ClusterGraphAttributes &CA)
{
	if(m_error) {
		return false;
	}

	G.clear();
	m_nodeId.clear();

	return readClusters(G, C, &CA, C.rootCluster(), m_graphTag);
}

}
