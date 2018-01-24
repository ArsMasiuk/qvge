/** \file
 * \brief Implementation of GEXF format parsing utilities.
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

#include <ogdf/fileformats/GEXF.h>
#include <ogdf/fileformats/GexfParser.h>
#include <ogdf/fileformats/GraphML.h>
#include <ogdf/fileformats/GraphIO.h>


namespace ogdf {

namespace gexf {


Parser::Parser(std::istream &is) : m_is(is)
{
}


static inline bool readAttrDefs(
	std::unordered_map<std::string,
	std::string> &attrMap,
	const pugi::xml_node attrsTag)
{
	for (auto attrTag : attrsTag.children("attributes")) {
		pugi::xml_attribute idAttr = attrTag.attribute("id");
		pugi::xml_attribute idTitle = attrTag.attribute("title");

		if(!idAttr || !idTitle)
		{
			GraphIO::logger.lout() << "\"id\" or \"title\" attribute missing." << std::endl;
			return false;
		}

		attrMap[idAttr.value()] = idTitle.value();
	}

	return true;
}

bool Parser::init()
{
	pugi::xml_parse_result result = m_xml.load(m_is);

	if (!result) {
		GraphIO::logger.lout() << "XML parser error: " << result.description() << std::endl;
		return false;
	}

	m_nodeId.clear();
	m_clusterId.clear();
	m_nodeAttr.clear();
	m_edgeAttr.clear();

	pugi::xml_node rootNode = m_xml.child("gexf");

	if(!rootNode) {
		GraphIO::logger.lout() << "Root tag must be \"gexf\"." << std::endl;
		return false;
	}

	m_graphTag = rootNode.child("graph");
	if (!m_graphTag) {
		GraphIO::logger.lout() << "Expected \"graph\" tag." << std::endl;
		return false;
	}

	m_nodesTag = m_graphTag.child("nodes");
	if(!m_nodesTag) {
		GraphIO::logger.lout() << "No \"nodes\" tag found in graph." << std::endl;
		return false;
	}

	m_edgesTag = m_graphTag.child("edges");
	if(!m_edgesTag) {
		GraphIO::logger.lout() << "No \"edges\" tag found in graph." << std::endl;
		return false;
	}

	// Read attributes definitions. Could be lazily read later only
	// if GraphAttributes is given.
	for(pugi::xml_node attrsTag : m_graphTag.children("attributes")) {
		pugi::xml_attribute classAttr = attrsTag.attribute("class");

		if(!classAttr) {
			GraphIO::logger.lout() << "attributes tag is missing a class." << std::endl;
			return false;
		}

		std::unordered_map<std::string, std::string> *attrMap;

		if(string(classAttr.value()) == "node") {
			attrMap = &m_nodeAttr;
		} else if(string(classAttr.value()) == "edge") {
			attrMap = &m_edgeAttr;
		} else {
			GraphIO::logger.lout() << "unknown attributes tag class ('"
			           << classAttr.value() << "')." << std::endl;
			return false;
		}

		if(!readAttrDefs(*attrMap, attrsTag)) {
			return false;
		}
	}

	return true;
}


bool Parser::readNodes(Graph &G, GraphAttributes *GA)
{
	for(pugi::xml_node nodeTag : m_nodesTag.children("node")) {
		pugi::xml_attribute idAttr = nodeTag.attribute("id");

		if(!idAttr) {
			GraphIO::logger.lout() << "node is missing an id attribute." << std::endl;
			return false;
		}

		const node v = G.newNode();
		m_nodeId[idAttr.value()] = v;

		if(GA) {
			readAttributes(*GA, v, nodeTag);
		}
	}

	return true;
}


bool Parser::readCluster(
	Graph &G, ClusterGraph &C,
	ClusterGraphAttributes *CA,
	cluster rootCluster,
	const pugi::xml_node rootTag)
{
	for(pugi::xml_node nodeTag : rootTag.children("node")) {
		pugi::xml_attribute idAttr = nodeTag.attribute("id");

		if(!idAttr) {
			GraphIO::logger.lout() << "node is missing an id attribute." << std::endl;
			return false;
		}

		// Node is a cluster iff it contains other nodes.
		pugi::xml_node nodesTag = nodeTag.child("nodes");
		if(nodesTag) {
			// Node tag found, therefore it is a cluster.
			const cluster c = C.newCluster(rootCluster);
			m_clusterId[idAttr.value()] = c;

			if(!readCluster(G, C, CA, c, nodesTag)) {
				return false;
			}
		} else {
			// Node tag not found, therefore it is "normal" node.
			const node v = G.newNode();
			C.reassignNode(v, rootCluster);
			m_nodeId[idAttr.value()] = v;

			if(CA) {
				readAttributes(*CA, v, nodeTag);
			}
		}
	}

	return true;
}


/*
 * Just a helper method to avoid ugly code in Parser#readEdges method. It just
 * populates \p nodes list with either a given \p v node (if not \c nullptr) or all
 * nodes in certain cluster found by performing a lookup with given \p id in
 * \p clusterId association.
 */
static inline bool edgeNodes(
	node v,
	const std::string &id,
	const std::unordered_map<std::string, cluster> &clusterId,
	List<node> &nodes)
{
	if(v) {
		nodes.clear();
		nodes.pushBack(v);
	} else {
		auto c = clusterId.find(id);
		if(c == std::end(clusterId)) {
			return false;
		}

		(c->second)->getClusterNodes(nodes);
	}

	return true;
}


bool Parser::readEdges(Graph &G, ClusterGraph *C, GraphAttributes *GA)
{
	List<node> sourceNodes, targetNodes;

	for(pugi::xml_node edgeTag : m_edgesTag.children("edge")) {
		pugi::xml_attribute sourceAttr = edgeTag.attribute("source");
		if(!sourceAttr) {
			GraphIO::logger.lout() << "edge is missing a source attribute." << std::endl;
			return false;
		}

		pugi::xml_attribute targetAttr = edgeTag.attribute("target");
		if(!targetAttr) {
			GraphIO::logger.lout() << "edge is missing a target attribute." << std::endl;
			return false;
		}

		const node source = m_nodeId[sourceAttr.value()];
		const node target = m_nodeId[targetAttr.value()];

		if(source && target) {
			const edge e = G.newEdge(source, target);
			if(GA) {
				readAttributes(*GA, e, edgeTag);
			}
		} else if(C && edgeNodes(source, sourceAttr.value(), m_clusterId, sourceNodes)
		            && edgeNodes(target, targetAttr.value(), m_clusterId, targetNodes)) {
			// So, we perform cartesian product on two sets with Graph#newEdge.
			for(node s : sourceNodes) {
				for(node t : targetNodes) {
					const edge e = G.newEdge(s, t);
					if(GA) {
						readAttributes(*GA, e, edgeTag);
					}
				}
			}
		} else {
			GraphIO::logger.lout() << "source or target node doesn't exist." << std::endl;
			return false;
		}
	}

	return true;
}


static inline bool readColor(Color &color, const pugi::xml_node tag)
{
	pugi::xml_attribute redAttr = tag.attribute("red");
	pugi::xml_attribute greenAttr = tag.attribute("green");
	pugi::xml_attribute blueAttr = tag.attribute("blue");
	pugi::xml_attribute alphaAttr = tag.attribute("alpha");

	if(!redAttr || !greenAttr || !blueAttr)
	{
		GraphIO::logger.lout() << "Missing compound attribute on color tag." << std::endl;
		return false;
	}

	bool success = true;
	success &= GraphIO::setColorValue(redAttr.as_int(), [&](uint8_t val) { color.red(val); });
	success &= GraphIO::setColorValue(greenAttr.as_int(), [&](uint8_t val) { color.green(val); });
	success &= GraphIO::setColorValue(blueAttr.as_int(), [&](uint8_t val) { color.blue(val); });
	success &= !alphaAttr || GraphIO::setColorValue(alphaAttr.as_int(), [&](uint8_t val) { color.alpha(val); });
	return success;
}


static inline bool readVizAttribute(
	GraphAttributes &GA,
	node v,
	const pugi::xml_node tag)
{
	const long attrs = GA.attributes();

	if(string(tag.name()) == "viz:position") {
		if(attrs & GraphAttributes::nodeGraphics) {
			pugi::xml_attribute xAttr = tag.attribute("x");
			pugi::xml_attribute yAttr = tag.attribute("y");
			pugi::xml_attribute zAttr = tag.attribute("z");

			if(!xAttr || !yAttr) {
				GraphIO::logger.lout() << "Missing \"x\" or \"y\" in position tag." << std::endl;
				return false;
			}

			GA.x(v) = xAttr.as_int();
			GA.y(v) = yAttr.as_int();

			// z attribute is optional and avaliable only in \a threeD mode
			GA.y(v) = yAttr.as_int();
			if (zAttr && (attrs & GraphAttributes::threeD)) {
				GA.z(v) = zAttr.as_int();
			}
		}
	} else if(string(tag.name()) == "viz:size") {
		pugi::xml_attribute valueAttr = tag.attribute("value");
		if(!valueAttr) {
			GraphIO::logger.lout() << "\"size\" attribute is missing a value." << std::endl;
			return false;
		}

		/*
		 * Because size is just a scale here, I assume that all nodes have
		 * some default width and height value. Then, I just rescale them
		 * using given size. Things can go wrong if viz:size is declared twice
		 * for the same node but this is not our problem (just fix this file!).
		 */
		double size = valueAttr.as_double();
		GA.width(v) *= size;
		GA.height(v) *= size;
	} else if(string(tag.name()) == "viz:shape") {
		if(attrs & GraphAttributes::nodeGraphics) {
			pugi::xml_attribute valueAttr = tag.attribute("value");
			if(!valueAttr) {
				GraphIO::logger.lout() << "\"shape\" attribute is missing a value." << std::endl;
				return false;
			}

			GA.shape(v) = toShape(valueAttr.value());
		}
	} else if(string(tag.name()) == "viz:color") {
		if(attrs & GraphAttributes::nodeStyle) {
			return readColor(GA.fillColor(v), tag);
		}
	} else {
		GraphIO::logger.lout() << "Incorrect tag: \"" << tag.name() << "\"." << std::endl;
		return false;
	}

	return true;
}


static inline bool readVizAttribute(
	GraphAttributes &GA,
	edge e,
	const pugi::xml_node tag)
{
	const long attrs = GA.attributes();

	if(string(tag.name()) == "viz:color") {
		if(attrs & GraphAttributes::edgeStyle) {
			return readColor(GA.strokeColor(e), tag);
		}
	} else if(string(tag.name()) == "viz:thickness") {
		auto thickAttr = tag.attribute("value");
		if(!thickAttr) {
			GraphIO::logger.lout() << "Missing \"value\" on thickness tag." << std::endl;
			return false;
		}

		if(attrs & GraphAttributes::edgeDoubleWeight) {
			GA.doubleWeight(e) = thickAttr.as_double();
		} else if(attrs & GraphAttributes::edgeIntWeight) {
			GA.intWeight(e) = thickAttr.as_int();
		}
	} else if(string(tag.name()) == "viz:shape") {
		// Values: solid, dotted, dashed, double. Not supported in OGDF.
	} else {
		GraphIO::logger.lout() << "Incorrect tag \"" << tag.name() << "\"." << std::endl;
		return false;
	}

	return true;
}


static inline void readAttValue(
	GraphAttributes &GA,
	node v,
	const std::string &name,
	const std::string &value)
{
	const long attrs = GA.attributes();

	// For not "viz" attributes, we use GraphML ones.
	switch(graphml::toAttribute(name)) {
	case graphml::Attribute::NodeType:
		if(attrs & GraphAttributes::nodeType) {
			GA.type(v) = graphml::toNodeType(value);
		}
		break;
	case graphml::Attribute::Template:
		if(attrs & GraphAttributes::nodeTemplate) {
			GA.templateNode(v) = value;
		}
		break;
	case graphml::Attribute::NodeWeight:
		if(attrs & GraphAttributes::nodeWeight) {
			std::istringstream ss(value);
			ss >> GA.weight(v);
		}
		break;
	default:
		// Not supported attribute, just ignore.
		break;
	}
}


static inline void readAttValue(
	GraphAttributes &GA,
	edge e,
	const std::string &name,
	const std::string &value)
{
	const long attrs = GA.attributes();

	// For not "viz" attributes, we use GraphML ones.
	switch(graphml::toAttribute(name)) {
	case graphml::Attribute::EdgeType:
		if(attrs & GraphAttributes::edgeType) {
			GA.type(e) = graphml::toEdgeType(value);
		}
		break;
	case graphml::Attribute::EdgeArrow:
		if(attrs & GraphAttributes::edgeArrow) {
			GA.arrowType(e) = graphml::toArrow(value);
		}
		break;
	default:
		// Not supported attribute, just ignore.
		break;
	}
}


template <typename T>
static inline bool readAttValues(
	GraphAttributes &GA,
	T element,
	const pugi::xml_node tag,
	std::unordered_map<std::string, std::string> &attrMap)
{
	for(pugi::xml_node attVal : tag.children("attvalue")) {
		pugi::xml_attribute forAttr = attVal.attribute("for");
		pugi::xml_attribute valueAttr = attVal.attribute("value");

		if(!forAttr || !valueAttr)
		{
			GraphIO::logger.lout() << "\"for\" or \"value\" not found for attvalue tag." << std::endl;
			return false;
		}

		const std::string &attrName = attrMap[forAttr.value()];
		readAttValue(GA, element, attrName, valueAttr.value());
	}

	return true;
}


bool Parser::readAttributes(
	GraphAttributes &GA, node v,
	const pugi::xml_node nodeTag)
{
	for(const pugi::xml_node tag : nodeTag.children()) {
		if(string(tag.name()) == "nodes") {
			continue;
		} else if(string(tag.name()) == "attvalues") {
			return readAttValues(GA, v, tag, m_nodeAttr);
		} else if(!readVizAttribute(GA, v, tag)) {
			return false;
		}
	}

	return true;
}


bool Parser::readAttributes(
	GraphAttributes &GA, edge e,
	const pugi::xml_node edgeTag)
{
	for(const pugi::xml_node tag : edgeTag.children()) {
		if(string(tag.name()) == "attvalues") {
			return readAttValues(GA, e, tag, m_edgeAttr);
		} else if(!readVizAttribute(GA, e, tag)) {
			return false;
		}
	}

	return true;
}


bool Parser::read(Graph &G)
{
	if(!init()) {
		return false;
	}
	OGDF_ASSERT(m_graphTag);

	G.clear();

	return readNodes(G, nullptr) && readEdges(G, nullptr, nullptr);
}


bool Parser::read(Graph &G, GraphAttributes &GA)
{
	if(!init()) {
		return false;
	}
	OGDF_ASSERT(m_graphTag);

	G.clear();

	return readNodes(G, &GA) && readEdges(G, nullptr, &GA);
}


bool Parser::read(Graph &G, ClusterGraph &C)
{
	if(!init()) {
		return false;
	}
	OGDF_ASSERT(m_graphTag);

	G.clear();

	return readCluster(G, C, nullptr, C.rootCluster(), m_nodesTag) &&
	       readEdges(G, &C, nullptr);
}


bool Parser::read(Graph &G, ClusterGraph &C, ClusterGraphAttributes &CA)
{
	if(!init()) {
		return false;
	}
	OGDF_ASSERT(m_graphTag);

	G.clear();

	return readCluster(G, C, &CA, C.rootCluster(), m_nodesTag) &&
	       readEdges(G, &C, &CA);
}

}
}
