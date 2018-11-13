/** \file
 * \brief Implements GEXF write functionality of class GraphIO.
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

#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/fileformats/GEXF.h>
#include <ogdf/fileformats/GraphML.h>
#include <ogdf/lib/pugixml/pugixml.h>


namespace ogdf {

namespace gexf {


static inline pugi::xml_node writeHeader(pugi::xml_document &doc, bool viz)
{
	pugi::xml_node rootNode = doc.append_child("gexf");
	rootNode.append_attribute("version") = "1.2";
	rootNode.append_attribute("xmlns") = "http://www.gexf.net/1.2draft";

	if(viz) {
		rootNode.append_attribute("xmlns:viz") = "http://www.gexf.net/1.2draft/viz";
	}

	// TODO: creator, description and date information.

	return rootNode;
}


template <typename T>
static inline void writeAttValue(
	pugi::xml_node xmlNode,
	const graphml::Attribute &attr,
	const T &value)
{
	pugi::xml_node child = xmlNode.append_child("attvalue");
	child.append_attribute("for") = graphml::toString(attr).c_str();
	child.append_attribute("value") = value;
}


static inline void defineAttribute(
	pugi::xml_node xmlNode,
	const std::string &name,
	const std::string &type)
{
	pugi::xml_node child = xmlNode.append_child("attribute");
	child.append_attribute("id") = name.c_str();
	child.append_attribute("title") = name.c_str();
	child.append_attribute("type") = type.c_str();
}


static inline void defineAttributes(
	pugi::xml_node xmlNode,
	const GraphAttributes &GA)
{
	const long attrs = GA.attributes();

	// Declare node attributes.
	pugi::xml_node child = xmlNode.append_child("attributes");
	child.append_attribute("class") = "node";

	if(attrs & GraphAttributes::nodeType) {
		defineAttribute(child, graphml::toString(graphml::Attribute::NodeType), "string");
	}

	if(attrs & GraphAttributes::nodeTemplate) {
		defineAttribute(child, graphml::toString(graphml::Attribute::Template), "string");
	}

	if(attrs & GraphAttributes::nodeWeight) {
		defineAttribute(child, graphml::toString(graphml::Attribute::NodeWeight), "float");
	}

	// Declare edge attributes.
	child = xmlNode.append_child("attributes");
	child.append_attribute("class") = "edge";

	if(attrs & GraphAttributes::edgeType) {
		defineAttribute(child, graphml::toString(graphml::Attribute::EdgeType), "string");
	}

	if(attrs & GraphAttributes::edgeArrow) {
		defineAttribute(child, graphml::toString(graphml::Attribute::EdgeArrow), "string");
	}
}


static inline void writeColor(pugi::xml_node xmlNode, const Color color)
{
	pugi::xml_node child = xmlNode.append_child("viz:color");
	child.attribute("red") = color.red();
	child.attribute("green") = color.green();
	child.attribute("blue") = color.blue();
	child.attribute("alpha") = color.alpha();
}


static inline void writeAttributes(
	pugi::xml_node xmlNode,
	const GraphAttributes &GA,
	node v)
{
	const long attrs = GA.attributes();

	if(attrs & GraphAttributes::nodeGraphics) {
		const double z = (attrs & GraphAttributes::threeD) ? GA.z(v) : 0.0;
		pugi::xml_node child = xmlNode.append_child("viz:position");
		child.attribute("x") = GA.x(v);
		child.attribute("y") = GA.y(v);
		child.attribute("z") = z;

		// TODO: size is a scale here, so we have to know average size first.
#if 0
		const double size = std::max(GA.width(v), GA.height(v));
		GraphIO::indent(out, depth) << "<viz:size "
		                            << "value=\"" << size << "\" "
		                            << "/>\n";
#endif

		const Shape shape = GA.shape(v);
		xmlNode.append_child("viz:shape").append_attribute("value") = toString(shape).c_str();
	}

	if(attrs & GraphAttributes::nodeStyle) {
		writeColor(xmlNode, GA.fillColor(v));
	}

	/*
	 * Node type, template and weight are not supported by VIZ module. So, they
	 * need to be written using <attvalues> tag (for estetic reasons, we write
	 * them only if either of them is present). For convenience reasons, we use
	 * the same names and values as in GraphML format.
	 */
	if(!(attrs & (GraphAttributes::nodeType |
	              GraphAttributes::nodeTemplate |
	              GraphAttributes::nodeWeight))) {
		return;
	}

	pugi::xml_node attValues = xmlNode.append_child("attvalues");

	if(attrs & GraphAttributes::nodeType) {
		writeAttValue(attValues, graphml::Attribute::NodeType, graphml::toString(GA.type(v)).c_str());
	}

	if(attrs & GraphAttributes::nodeTemplate) {
		writeAttValue(attValues, graphml::Attribute::Template, GA.templateNode(v).c_str());
	}

	if(attrs & GraphAttributes::nodeWeight) {
		writeAttValue(attValues, graphml::Attribute::NodeWeight, GA.weight(v));
	}
}


static inline void writeAttributes(
	pugi::xml_node xmlNode,
	const GraphAttributes &GA,
	edge e)
{
	const long attrs = GA.attributes();

	if(attrs & GraphAttributes::edgeStyle) {
		writeColor(xmlNode, GA.strokeColor(e));
	}

	if(attrs & GraphAttributes::edgeDoubleWeight) {
		xmlNode.append_child("viz:thickness").append_attribute("value") = GA.doubleWeight(e);
	} else if(attrs & GraphAttributes::edgeIntWeight) {
		xmlNode.append_child("viz:thickness").append_attribute("value") = GA.intWeight(e);
	}

	/*
	 * Edge type and arrow are not supported by VIZ module. Therefore, they
	 * need to be written using <attvalues> tag (for estetic reasons, we write
	 * them only if either of them is present). For convenience reasons, we use
	 * the same names and values as in GraphML format.
	 */
	if(!(attrs & (GraphAttributes::edgeType | GraphAttributes::edgeArrow))) {
		return;
	}

	pugi::xml_node attValues = xmlNode.append_child("attvalues");

	if(attrs & GraphAttributes::edgeType) {
		writeAttValue(attValues, graphml::Attribute::EdgeType, graphml::toString(GA.type(e)).c_str());
	}
	if(attrs & GraphAttributes::edgeArrow) {
		writeAttValue(attValues, graphml::Attribute::EdgeArrow, graphml::toString(GA.arrowType(e)).c_str());
	}
}


static inline void writeNode(
	pugi::xml_node xmlNode,
	const GraphAttributes *GA,
	node v)
{
	pugi::xml_node nodeTag = xmlNode.append_child("node");
	nodeTag.append_attribute("id") = v->index();

	if(GA) {
		if(GA->has(GraphAttributes::nodeLabel)) {
			nodeTag.append_attribute("label") = GA->label(v).c_str();
		}

		writeAttributes(nodeTag, *GA, v);
	}
}


static inline void writeEdge(
	pugi::xml_node xmlNode,
	const GraphAttributes *GA,
	edge e)
{
	pugi::xml_node edge = xmlNode.append_child("edge");
	edge.append_attribute("id") = e->index();

	if(GA) {
		if(GA->has(GraphAttributes::edgeLabel)) {
			edge.append_attribute("label") = GA->label(e).c_str();
		}

		writeAttributes(edge, *GA, e);
	} else {
		edge.append_attribute("source") = e->source()->index();
		edge.append_attribute("target") = e->target()->index();
	}
}


static inline void writeEdges(
	pugi::xml_node xmlNode,
	const Graph &G,
	const GraphAttributes *GA)
{
	pugi::xml_node edges = xmlNode.append_child("edges");

	for(edge e : G.edges) {
		writeEdge(edges, GA, e);
	}
}


static void writeCluster(
	pugi::xml_node rootNode,
	const ClusterGraph &C,
	const ClusterGraphAttributes *CA,
	cluster c)
{
	pugi::xml_node graph;

	if(C.rootCluster() != c) {
		graph = rootNode.append_child("node");
		graph.append_attribute("id") = ("cluster" + to_string(c->index())).c_str();
	} else {
		graph = rootNode.append_child("graph");
		graph.append_attribute("mode") = "static";
		graph.append_attribute("defaultedgetype") = CA && !CA->directed() ? "undirected" : "directed";

		if(CA) {
			defineAttributes(graph, *CA);
		}
	}

	pugi::xml_node nodes = graph.append_child("nodes");

	for(cluster child : c->children) {
		writeCluster(nodes, C, CA, child);
	}

	for(node v : c->nodes) {
		writeNode(nodes, CA, v);
	}

	if(C.rootCluster() == c) {
		writeEdges(graph, C.constGraph(), CA);
	}
}


static void writeGraph(
	pugi::xml_node rootNode,
	const Graph &G,
	const GraphAttributes *GA)
{
	pugi::xml_node graph = rootNode.append_child("graph");
	graph.append_attribute("mode") = "static";
	graph.append_attribute("defaultedgetype") = GA && !GA->directed() ? "undirected" : "directed";

	if(GA) {
		defineAttributes(graph, *GA);
	}

	pugi::xml_node nodes = graph.append_child("nodes");

	for(node v : G.nodes) {
		writeNode(nodes, GA, v);
	}

	gexf::writeEdges(graph, G, GA);
}

}

bool GraphIO::writeGEXF(const Graph &G, std::ostream &out)
{
	bool result = out.good();

	if(result) {
		pugi::xml_document doc;
		pugi::xml_node rootNode = gexf::writeHeader(doc, false);
		gexf::writeGraph(rootNode, G, nullptr);
		doc.save(out);
	}

	return result;
}


bool GraphIO::writeGEXF(const ClusterGraph &C, std::ostream &out)
{
	bool result = out.good();

	if(result) {
		pugi::xml_document doc;
		pugi::xml_node rootNode = gexf::writeHeader(doc, false);
		gexf::writeCluster(rootNode, C, nullptr, C.rootCluster());
		doc.save(out);

		return true;
	}

	return result;
}


bool GraphIO::writeGEXF(const GraphAttributes &GA, std::ostream &out)
{
	bool result = out.good();

	if(result) {
		pugi::xml_document doc;
		pugi::xml_node rootNode = gexf::writeHeader(doc, true);
		gexf::writeGraph(rootNode, GA.constGraph(), &GA);
		doc.save(out);

		return true;
	}

return result;
}


bool GraphIO::writeGEXF(const ClusterGraphAttributes &CA, std::ostream &out)
{
	bool result = out.good();

	if(result) {
		const ClusterGraph &C = CA.constClusterGraph();

		pugi::xml_document doc;
		pugi::xml_node rootNode = gexf::writeHeader(doc, true);
		gexf::writeCluster(rootNode, C, &CA, C.rootCluster());
		doc.save(out);

		return true;
	}

return result;
}

}
