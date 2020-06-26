/** \file
 * \brief Implements GraphML write functionality of class GraphIO.
 *
 * \author Łukasz Hanuszczak
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

#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/fileformats/GraphML.h>
#include <ogdf/lib/pugixml/pugixml.h>


namespace ogdf {


static inline pugi::xml_node writeGraphMLHeader(pugi::xml_document &doc)
{
	const std::string xmlns = "http://graphml.graphdrawing.org/xmlns";

	pugi::xml_node rootNode = doc.append_child("graphml");
	rootNode.append_attribute("xmlns") = xmlns.c_str();
	rootNode.append_attribute("xmlns:xsi") = "http://www.w3.org/2001/XMLSchema-instance";
	rootNode.append_attribute("xsi:schemaLocation") = (xmlns + "\n" + xmlns + "/1.0/graphml.xsd\">\n").c_str();

	return rootNode;
}

static inline pugi::xml_node writeGraphTag(pugi::xml_node xmlNode, std::string edgeDefault)
{
	pugi::xml_node graphNode = xmlNode.append_child("graph");
	graphNode.append_attribute("id") = "G";
	graphNode.append_attribute("edgedefault") = edgeDefault.c_str();

	return graphNode;

}


static inline void defineGraphMLAttribute(
	pugi::xml_node xmlNode,
	const std::string &kind,
	const std::string &name,
	const std::string &type)
{
	pugi::xml_node key = xmlNode.append_child("key");
	key.append_attribute("for") = kind.c_str();
	key.append_attribute("attr.name") = name.c_str();
	key.append_attribute("attr.type") = type.c_str();
	key.append_attribute("id") = name.c_str();
}


static inline void defineGraphMLAttributes(pugi::xml_node xmlNode, long attributes)
{
	using namespace graphml;

	// Gephi-compatible attributes
	if(attributes & GraphAttributes::nodeLabel) {
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::NodeLabel), "string");
	}

	if(attributes & GraphAttributes::nodeLabelPosition) {
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::NodeLabelX), "float");
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::NodeLabelY), "float");
		if (attributes & GraphAttributes::threeD) {
			defineGraphMLAttribute(xmlNode, "node", toString(Attribute::NodeLabelZ), "float");
		}
	}

	if(attributes & GraphAttributes::nodeGraphics) {
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::X), "double");
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::Y), "double");
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::Size), "double");
	}

	if(attributes & GraphAttributes::nodeStyle) {
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::R), "int");
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::G), "int");
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::B), "int");
	}

	if(attributes & GraphAttributes::edgeLabel) {
		defineGraphMLAttribute(xmlNode, "edge", toString(Attribute::EdgeLabel), "string");
	}

	if(attributes & GraphAttributes::edgeDoubleWeight) {
		defineGraphMLAttribute(xmlNode, "edge", toString(Attribute::EdgeWeight), "double");
	} else if(attributes & GraphAttributes::edgeIntWeight) {
		defineGraphMLAttribute(xmlNode, "edge", toString(Attribute::EdgeWeight), "int");
	}

	// OGDF-specific attributes.
	if (attributes & GraphAttributes::nodeGraphics) {
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::Width), "double");
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::Height), "double");
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::Shape), "string");
	}

	if(attributes & GraphAttributes::nodeStyle) {
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::NodeStrokeColor), "string");
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::NodeStrokeType), "int");
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::NodeStrokeWidth), "double");
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::NodeFillPattern), "int");
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::NodeFillBackground), "string");
	}

	if(attributes & GraphAttributes::nodeWeight) {
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::NodeWeight), "int");
	}

	if(attributes & GraphAttributes::nodeType) {
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::NodeType), "int");
	}

	if(attributes & GraphAttributes::nodeId) {
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::NodeId), "int");
	}

	if(attributes & GraphAttributes::nodeTemplate) {
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::Template), "string");
	}

	if(attributes & GraphAttributes::threeD) {
		defineGraphMLAttribute(xmlNode, "node", toString(Attribute::Z), "double");
	}

	if(attributes & GraphAttributes::edgeGraphics) {
		// Currently bending points are printed as list. More XML-ish way has
		// to be adopted in future (it will probably involve writing custom
		// XML schema...).
		defineGraphMLAttribute(xmlNode, "edge", toString(Attribute::EdgeBends), "string");
	}

	if(attributes & GraphAttributes::edgeType) {
		defineGraphMLAttribute(xmlNode, "edge", toString(Attribute::EdgeType), "string");
	}

	if(attributes & GraphAttributes::edgeArrow) {
		defineGraphMLAttribute(xmlNode, "edge", toString(Attribute::EdgeArrow), "string");
	}

	if(attributes & GraphAttributes::edgeStyle) {
		defineGraphMLAttribute(xmlNode, "edge", toString(Attribute::EdgeStrokeColor), "string");
		defineGraphMLAttribute(xmlNode, "edge", toString(Attribute::EdgeStrokeType), "int");
		defineGraphMLAttribute(xmlNode, "edge", toString(Attribute::EdgeStrokeWidth), "double");
	}

	if(attributes & GraphAttributes::edgeSubGraphs) {
		// Similarly to bend points, print as list.
		defineGraphMLAttribute(xmlNode, "edge", toString(Attribute::EdgeSubGraph), "string");
	}
}


template <typename T>
static inline void writeGraphMLAttribute(
	pugi::xml_node xmlNode,
	const std::string &name,
	const T &value)
{
	pugi::xml_node data = xmlNode.append_child("data");
	data.append_attribute("key") = name.c_str();
	data.text() = value;
}


static inline void writeGraphMLNode(pugi::xml_node xmlNode, const node &v)
{
	xmlNode.append_child("node").append_attribute("id") = v->index();
}


static inline pugi::xml_node writeGraphMLEdge(pugi::xml_node xmlNode, const edge &e)
{
	pugi::xml_node edgeXmlNode = xmlNode.append_child("edge");
	edgeXmlNode.append_attribute("id") = e->index();
	edgeXmlNode.append_attribute("source") = e->source()->index();
	edgeXmlNode.append_attribute("target") = e->target()->index();

	return edgeXmlNode;
}


static inline void writeGraphMLNode(
	pugi::xml_node xmlNode,
	const GraphAttributes &GA,
	const node &v)
{
	using namespace graphml;

	pugi::xml_node nodeTag = xmlNode.append_child("node");

	nodeTag.append_attribute("id") = v->index();

	if(GA.has(GraphAttributes::nodeId)) {
		writeGraphMLAttribute(nodeTag, toString(Attribute::NodeId), GA.idNode(v));
	}

	if(GA.has(GraphAttributes::nodeLabel) && GA.label(v) != "") {
		writeGraphMLAttribute(nodeTag, toString(Attribute::NodeLabel), GA.label(v).c_str());
	}

	if(GA.has(GraphAttributes::nodeGraphics)) {
		writeGraphMLAttribute(nodeTag, toString(Attribute::X), GA.x(v));
		writeGraphMLAttribute(nodeTag, toString(Attribute::Y), GA.y(v));
		writeGraphMLAttribute(nodeTag, toString(Attribute::Width), GA.width(v));
		writeGraphMLAttribute(nodeTag, toString(Attribute::Height), GA.height(v));
		writeGraphMLAttribute(nodeTag, toString(Attribute::Size), std::max(GA.width(v), GA.height(v)));
		writeGraphMLAttribute(nodeTag, toString(Attribute::Shape), toString(GA.shape(v)).c_str());
	}

	if(GA.has(GraphAttributes::threeD)) {
		writeGraphMLAttribute(nodeTag, toString(Attribute::Z), GA.z(v));
	}

	if(GA.has(GraphAttributes::nodeLabelPosition)) {
		writeGraphMLAttribute(nodeTag, toString(Attribute::NodeLabelX), GA.xLabel(v));
		writeGraphMLAttribute(nodeTag, toString(Attribute::NodeLabelY), GA.yLabel(v));
		if(GA.has(GraphAttributes::threeD)) {
			writeGraphMLAttribute(nodeTag, toString(Attribute::NodeLabelZ), GA.zLabel(v));
		}
	}

	if(GA.has(GraphAttributes::nodeStyle)) {
		const Color &col = GA.fillColor(v);
		writeGraphMLAttribute(nodeTag, toString(Attribute::R), col.red());
		writeGraphMLAttribute(nodeTag, toString(Attribute::G), col.green());
		writeGraphMLAttribute(nodeTag, toString(Attribute::B), col.blue());
		writeGraphMLAttribute(nodeTag, toString(Attribute::NodeFillPattern), int(GA.fillPattern(v)));
		writeGraphMLAttribute(nodeTag, toString(Attribute::NodeFillBackground), GA.fillBgColor(v).toString().c_str());

		writeGraphMLAttribute(nodeTag, toString(Attribute::NodeStrokeColor), GA.strokeColor(v).toString().c_str());
		writeGraphMLAttribute(nodeTag, toString(Attribute::NodeStrokeType), int(GA.strokeType(v)));
		writeGraphMLAttribute(nodeTag, toString(Attribute::NodeStrokeWidth), GA.strokeWidth(v));
	}

	if(GA.has(GraphAttributes::nodeType)) {
		writeGraphMLAttribute(nodeTag, toString(Attribute::NodeType), int(GA.type(v)));
	}

	if(GA.has(GraphAttributes::nodeTemplate) &&
	   GA.templateNode(v).length() > 0) {
		writeGraphMLAttribute(nodeTag, toString(Attribute::Template), GA.templateNode(v).c_str());
	}

	if(GA.has(GraphAttributes::nodeWeight)) {
		writeGraphMLAttribute(nodeTag, toString(Attribute::NodeWeight), GA.weight(v));
	}
}


static inline void writeGraphMLEdge(
	pugi::xml_node xmlNode,
	const GraphAttributes &GA,
	const edge &e)
{
	using namespace graphml;

	pugi::xml_node edgeTag = writeGraphMLEdge(xmlNode, e);

	if(GA.has(GraphAttributes::edgeLabel) && GA.label(e) != "") {
		writeGraphMLAttribute(edgeTag, toString(Attribute::EdgeLabel), GA.label(e).c_str());
	}

	if(GA.has(GraphAttributes::edgeDoubleWeight)) {
		writeGraphMLAttribute(edgeTag, toString(Attribute::EdgeWeight), GA.doubleWeight(e));
	} else if(GA.has(GraphAttributes::edgeIntWeight)) {
		writeGraphMLAttribute(edgeTag, toString(Attribute::EdgeWeight), GA.intWeight(e));
	}

	if(GA.has(GraphAttributes::edgeGraphics) && !GA.bends(e).empty()) {
		std::stringstream sstream;
		sstream.setf(std::ios::fixed);

		for(const DPoint &p : GA.bends(e)) {
			sstream << p.m_x << " " << p.m_y << " ";
		}

		writeGraphMLAttribute(edgeTag, toString(Attribute::EdgeBends), sstream.str().c_str());
	}

	if(GA.has(GraphAttributes::edgeType)) {
		writeGraphMLAttribute(edgeTag, toString(Attribute::EdgeType), toString(GA.type(e)).c_str());
	}

	if(GA.has(GraphAttributes::edgeArrow)) {
		const EdgeArrow &arrow = GA.arrowType(e);
		if (arrow != EdgeArrow::Undefined) {
			writeGraphMLAttribute(edgeTag, toString(Attribute::EdgeArrow), toString(arrow).c_str());
		}
	}

	if(GA.has(GraphAttributes::edgeStyle)) {
		writeGraphMLAttribute(edgeTag, toString(Attribute::EdgeStrokeColor), GA.strokeColor(e).toString().c_str());
		writeGraphMLAttribute(edgeTag, toString(Attribute::EdgeStrokeType), int(GA.strokeType(e)));
		writeGraphMLAttribute(edgeTag, toString(Attribute::EdgeStrokeWidth), GA.strokeWidth(e));
	}

	if(GA.has(GraphAttributes::edgeSubGraphs)) {
		const uint32_t mask = GA.subGraphBits(e);

		// Iterate over all subgraphs and print the ones the edge is part of.
		std::stringstream sstream;
		for (size_t sg = 0; sg < sizeof(mask) * 8; ++sg) {
			if((1 << sg) & mask) {
				sstream << (sg == 0 ? "" : " ") << sg;
			}
		}
		writeGraphMLAttribute(edgeTag, toString(Attribute::EdgeSubGraph), sstream.str().c_str());
	}
}


static void writeGraphMLCluster(
	pugi::xml_node xmlNode,
	const ClusterGraph &C,
	const cluster &c,
	int clusterId)
{
	pugi::xml_node graph;

	if(C.rootCluster() != c) {
		pugi::xml_node clusterXmlNode = xmlNode.append_child("node");
		const char* idValue = ("cluster" + to_string(clusterId)).c_str();
		clusterXmlNode.append_attribute("id") = idValue;

		graph = clusterXmlNode.append_child("graph");
		graph.append_attribute("id") = idValue;
		graph.append_attribute("edgedefault") = "directed";
	}

	clusterId++;

	for(cluster child : c->children) {
		writeGraphMLCluster(graph, C, child, clusterId);
	}

	for(node v : c->nodes) {
		writeGraphMLNode(graph, v);
	}
}


static void writeGraphMLCluster(
	pugi::xml_node xmlNode,
	const ClusterGraphAttributes &CA,
	const cluster &c,
	int clusterId)
{
	pugi::xml_node graph;
	pugi::xml_node clusterTag;
	const bool isRoot = CA.constClusterGraph().rootCluster() == c;

	if(isRoot) {
		graph = xmlNode;
	} else {
		clusterTag = xmlNode.append_child("node");
		const char* idValue = ("cluster" + to_string(clusterId)).c_str();
		clusterTag.append_attribute("id") = idValue;

		pugi::xml_node graphXmlNode = clusterTag.append_child("graph");
		graphXmlNode.append_attribute("id") = idValue;
		graphXmlNode.append_attribute("edgedefault") = CA.directed() ? "directed" : "undirected";
	}

	clusterId++;

	for(cluster child : c->children) {
		writeGraphMLCluster(graph, CA, child, clusterId);
	}

	for(node v : c->nodes) {
		writeGraphMLNode(graph, CA, v);
	}

	// There should be no attributes for root cluster.
	if(isRoot) {
		return;
	}

	using namespace graphml;

	// Writing cluster attributes (defined as cluster-node attributes).
	if(CA.label(c).length() > 0) {
		writeGraphMLAttribute(clusterTag, toString(Attribute::NodeLabel), CA.label(c).c_str());
	}
	writeGraphMLAttribute(clusterTag, toString(Attribute::X), CA.x(c));
	writeGraphMLAttribute(clusterTag, toString(Attribute::Y), CA.y(c));

	const Color &col = CA.fillColor(c);
	writeGraphMLAttribute(clusterTag, toString(Attribute::R), col.red());
	writeGraphMLAttribute(clusterTag, toString(Attribute::G), col.green());
	writeGraphMLAttribute(clusterTag, toString(Attribute::B), col.blue());
	writeGraphMLAttribute(clusterTag, toString(Attribute::ClusterStroke), CA.strokeColor(c).toString().c_str());

	if(CA.templateCluster(c).length() > 0) {
		writeGraphMLAttribute(clusterTag, toString(Attribute::Template), CA.templateCluster(c).c_str());
	}

	// TODO: not important cluster attributes like fill patterns, background
	// color, stroke width, etc.
}


bool GraphIO::writeGraphML(const Graph &G, std::ostream &out)
{
	bool result = out.good();

	if(result) {
		pugi::xml_document doc;

		pugi::xml_node rootNode = writeGraphMLHeader(doc);
		pugi::xml_node graphNode = writeGraphTag(rootNode, "directed");

		for (node v : G.nodes) {
			writeGraphMLNode(graphNode, v);
		}

		for (edge e : G.edges) {
			writeGraphMLEdge(graphNode, e);
		}

		doc.save(out);
	}

	return result;
}


bool GraphIO::writeGraphML(const ClusterGraph &C, std::ostream &out)
{
	bool result = out.good();

	if(result) {
		const Graph &G = C.constGraph();
		pugi::xml_document doc;

		pugi::xml_node rootNode = writeGraphMLHeader(doc);
		pugi::xml_node graphNode = writeGraphTag(rootNode, "directed");

		writeGraphMLCluster(graphNode, G, C.rootCluster(), 0);

		for (edge e : G.edges) {
			writeGraphMLEdge(graphNode, e);
		}

		doc.save(out);
	}

	return result;
}


bool GraphIO::writeGraphML(const GraphAttributes &GA, std::ostream &out)
{
	bool result = out.good();

	if(result) {
		const Graph &G = GA.constGraph();
		const std::string edgeDefault = GA.directed() ? "directed" : "undirected";
		pugi::xml_document doc;

		pugi::xml_node rootNode = writeGraphMLHeader(doc);
		defineGraphMLAttributes(rootNode, GA.attributes());
		pugi::xml_node graphNode = writeGraphTag(rootNode, edgeDefault);

		for (node v : G.nodes) {
			writeGraphMLNode(graphNode, GA, v);
		}

		for (edge e : G.edges) {
			writeGraphMLEdge(graphNode, GA, e);
		}

		doc.save(out);
	}

	return result;
}


bool GraphIO::writeGraphML(const ClusterGraphAttributes &CA, std::ostream &out)
{
	bool result = out.good();

	if(result) {
		const Graph &G = CA.constGraph();
		const ClusterGraph &C = CA.constClusterGraph();
		pugi::xml_document doc;

		pugi::xml_node rootNode = writeGraphMLHeader(doc);
		defineGraphMLAttributes(rootNode, CA.attributes());
		defineGraphMLAttribute(rootNode, "node", toString(graphml::Attribute::ClusterStroke), "string");
		pugi::xml_node graphNode = writeGraphTag(rootNode, "directed");
		writeGraphMLCluster(graphNode, CA, C.rootCluster(), 0);

		for (edge e : G.edges) {
			writeGraphMLEdge(graphNode, CA, e);
		}

		doc.save(out);
	}

	return result;
}


}
