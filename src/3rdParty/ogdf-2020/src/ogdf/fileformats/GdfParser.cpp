/** \file
 * \brief Implementation of GDF format parsing utilities
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

#include <ogdf/fileformats/GdfParser.h>
#include <ogdf/fileformats/Utils.h>
#include <ogdf/fileformats/GraphIO.h>

namespace ogdf {

namespace gdf {

Parser::Parser(std::istream &is) : m_istream(is), m_nodeId(nullptr)
{
}


/*
 * A method to avoid code duplication: reads either a "nodedef" or
 * "edgedef" GDF file header.
 */
template <typename Attr>
static inline bool readDef(
	const std::string &str,
	Attr toAttribute(const std::string &str), Attr a_unknown,
	std::vector<Attr> &attrs)
{
	std::istringstream is(str);
	std::string attr;

	/*
	 * Okay, so there is a attribute name first and then optional type.
	 * Therefore we chop this stream by commas, and then we read first
	 * non-whitespace string out of it.
	 */
	while(std::getline(is, attr, ',')) {
		std::istringstream attrss(attr);
		std::string name;
		attrss >> name;

		Attr attrib = toAttribute(name);
		if(attrib == a_unknown) {
			GraphIO::logger.lout(Logger::Level::Minor) << "attribute \"" << name << "\"" << " not supported. Ignoring." << std::endl;
		}
		attrs.push_back(attrib);
	}

	return true;

}


bool Parser::readNodeDef(const std::string &str)
{
	return readDef(str, toNodeAttribute, NodeAttribute::Unknown, m_nodeAttrs);
}


bool Parser::readEdgeDef(const std::string &str)
{
	return readDef(str, toEdgeAttribute, EdgeAttribute::Unknown, m_edgeAttrs);
}


static size_t scanQuoted(
	const std::string &str, size_t pos,
	std::string &buff)
{
	for(size_t j = 1; pos + j < str.length(); j++) {
		if(str[pos] == str[pos + j] && str[pos + j - 1] != '\\') {
			return j; // was j before, but j is always >= 1
		}
		buff += str[pos + j];
	}

	return 0;
}


static bool split(
	const std::string &str,
	std::vector<std::string> &result)
{
	result.clear();
	std::string buff = "";

	const size_t len = str.length();
	for(size_t i = 0; i < len; i++) {
		if(str[i] == '\"' || str[i] == '\'') {
			size_t quoted = scanQuoted(str, i, buff);
			if(quoted) {
				i += quoted;
			} else {
				GraphIO::logger.lout() << "Unescaped quote." << std::endl;
				return false;
			}
		} else if(str[i] == ',') {
			result.push_back(buff);
			buff = "";
		} else {
			buff += str[i];
		}
	}

	// Last buffer is not inserted during the loop.
	result.push_back(buff);

	return true;
}


bool Parser::readNodeStmt(
	Graph &G, GraphAttributes *GA,
	const std::string &str, size_t line)
{
	std::vector<std::string> values;
	split(str, values);

	if(values.size() != m_nodeAttrs.size()) {
		GraphIO::logger.lout() << "node definition does not match the header "
		          << "(line " << line << ")." << std::endl;
		return false;
	}

	node v = G.newNode();
	for(size_t i = 0; i < values.size(); i++) {
		if(m_nodeAttrs[i] == NodeAttribute::Name) {
			m_nodeId[values[i]] = v;
		}
	}

	return GA == nullptr || readAttributes(*GA, v, values);
}


bool Parser::readEdgeStmt(
	Graph &G, GraphAttributes *GA,
	const std::string &str, size_t line)
{
	std::vector<std::string> values;
	split(str, values);

	if(values.size() != m_edgeAttrs.size()) {
		GraphIO::logger.lout() << "edge definition does not match the header "
		          << "(line " << line << ")." << std::endl;
		return false;
	}

	// First, we scan a list for source, target and edge direction.
	bool directed = false;
	node source = nullptr, target = nullptr;
	for(size_t i = 0; i < values.size(); i++) {
		switch(m_edgeAttrs[i]) {
		case EdgeAttribute::Directed:
			if(values[i] == "true") {
				directed = true;
			} else if(values[i] == "false") {
				directed = false;
			} else {
				GraphIO::logger.lout() << "edge direction must be a boolean "
				          << "(line " << line << ")." << std::endl;
			}
			break;
		case EdgeAttribute::Source:
			source = m_nodeId[values[i]];
			break;
		case EdgeAttribute::Target:
			target = m_nodeId[values[i]];
			break;
		default:
			break;
		}
	}

	// Then, we can create edge(s) and read attributes (if needed).
	if(!source || !target) {
		GraphIO::logger.lout() << "source or target for edge not found "
		          << "(line " << line << ")." << std::endl;
		return false;
	}

	edge st = G.newEdge(source, target);
	OGDF_ASSERT(st);

	if (GA) {
		GA->directed() = directed;
		if (!readAttributes(*GA, st, values)) {
			return false;
		}
	}

	return true;
}


static inline Color toColor(const std::string &str)
{
	std::istringstream is(str);
	int r, g, b;
	is >> r >> TokenIgnorer(',') >> g >> TokenIgnorer(',') >> b;

	return Color(r, g, b);
}


static bool inline readAttribute(
	GraphAttributes &GA, node v,
	const NodeAttribute &attr, const std::string &value)
{
	const long attrs = GA.attributes();
	switch(attr) {
	case NodeAttribute::Name:
		// Not really an attribute, handled elsewhere.
		break;
	case NodeAttribute::Label:
		if(attrs & GraphAttributes::nodeLabel) {
			GA.label(v) = value;
		}
		break;
	case NodeAttribute::X:
		if(attrs & GraphAttributes::nodeGraphics) {
			std::istringstream is(value);
			is >> GA.x(v);
		}
		break;
	case NodeAttribute::Y:
		if(attrs & GraphAttributes::nodeGraphics) {
			std::istringstream is(value);
			is >> GA.y(v);
		}
		break;
	case NodeAttribute::Z:
		if(attrs & GraphAttributes::threeD) {
			std::istringstream is(value);
			is >> GA.z(v);
		}
		break;
	case NodeAttribute::FillPattern:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.fillPattern(v) = fromString<FillPattern>(value);
		}
		break;
	case NodeAttribute::FillColor:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.fillColor(v) = toColor(value);
		}
		break;
	case NodeAttribute::FillBgColor:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.fillBgColor(v) = toColor(value);
		}
		break;
	case NodeAttribute::StrokeWidth:
		if(attrs & GraphAttributes::nodeStyle) {
			std::istringstream is(value);
			is >> GA.strokeWidth(v);
		}
		break;
	case NodeAttribute::StrokeType:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.strokeType(v) = fromString<StrokeType>(value);
		}
		break;
	case NodeAttribute::StrokeColor:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.strokeColor(v) = toColor(value);
		}
		break;
	case NodeAttribute::Shape:
		if(attrs & GraphAttributes::nodeGraphics) {
			GA.shape(v) = toShape(value);
		}
		break;
	case NodeAttribute::Width:
		if(attrs & GraphAttributes::nodeGraphics) {
			std::istringstream is(value);
			is >> GA.width(v);
		}
		break;
	case NodeAttribute::Height:
		if(attrs & GraphAttributes::nodeGraphics) {
			std::istringstream is(value);
			is >> GA.height(v);
		}
		break;
	case NodeAttribute::Template:
		if(attrs & GraphAttributes::nodeTemplate) {
			GA.templateNode(v) = value;
		}
		break;
	case NodeAttribute::Weight:
		if(attrs & GraphAttributes::nodeWeight) {
			std::istringstream is(value);
			is >> GA.weight(v);
		}
		break;
	default:
		break;
	}
	return true;
}


static bool inline readAttribute(
	GraphAttributes &GA, edge e,
	const EdgeAttribute &attr, const std::string &value)
{
	const long attrs = GA.attributes();

	switch(attr) {
	case EdgeAttribute::Label:
		if(attrs & GraphAttributes::edgeLabel) {
			GA.label(e) = value;
		}
		break;
	case EdgeAttribute::Source:
		// Handled elsewhere.
		break;
	case EdgeAttribute::Target:
		// Handled elsewhere.
		break;
	case EdgeAttribute::Directed:
		// Handled elsewhere.
		break;
	case EdgeAttribute::Weight:
		if(attrs & GraphAttributes::edgeDoubleWeight) {
			std::istringstream is(value);
			is >> GA.doubleWeight(e);
		} else if (attrs & GraphAttributes::edgeIntWeight) {
			std::istringstream is(value);
			is >> GA.intWeight(e);
		}
		break;
	case EdgeAttribute::Color:
		if(attrs & GraphAttributes::edgeStyle) {
			GA.strokeColor(e) = toColor(value);
		}
		break;
	case EdgeAttribute::Bends:
		if(attrs & GraphAttributes::edgeGraphics) {
			std::istringstream is(value);
			std::string x, y;

			DPolyline &line = GA.bends(e);
			line.clear();
			while(std::getline(is, x, ',') && std::getline(is, y, ',')) {
				std::istringstream conv;
				double dx, dy;

				conv.clear();
				conv.str(x);
				conv >> dx;

				conv.clear();
				conv.str(y);
				conv >> dy;

				line.pushBack(DPoint(dx, dy));
			}
		}
		break;
	default:
		break;
	}

	return true;
}


/*
 * Once again, generic \i readAttributes method to avoid code duplication.
 */
template <typename T, typename A>
static inline bool readAttrs(
	GraphAttributes &GA, T elem,
	const std::vector<A> &attrs,
	const std::vector<std::string> &values)
{
	for(size_t i = 0; i < values.size(); i++) {
		if(!readAttribute(GA, elem, attrs[i], values[i])) {
			return false;
		}
	}

	return true;
}


bool Parser::readAttributes(
	GraphAttributes &GA, node v,
	const std::vector<std::string> &values)
{
	return readAttrs(GA, v, m_nodeAttrs, values);
}


bool Parser::readAttributes(
	GraphAttributes &GA, edge e,
	const std::vector<std::string> &values)
{
	return readAttrs(GA, e, m_edgeAttrs, values);
}


/*
 * Just checks wheter beginning of the string is equal to the pattern.
 * Returns number of matched letters (length of the pattern).
 */
size_t match(const std::string &text, const std::string &pattern) {
	const size_t len = pattern.length();
	if(len > text.length()) {
		return 0;
	}

	for(size_t i = 0; i < len; i++) {
		if(pattern[i] != text[i]) {
			return 0;
		}
	}

	return len;
}


bool gdf::Parser::readGraph(
	Graph &G, GraphAttributes *GA)
{
	G.clear();

	enum class Mode { None, Node, Edge } mode = Mode::None;

	size_t line = 0;
	std::string str;
	while(std::getline(m_istream, str)) {
		line += 1;

		/*
		 * We skip empty lines (it is not stated in documentation whether they
		 * are allowed or not, but I like empty lines so since it causes no
		 * charm I think they are fine).
		 */
		if(str.empty()) {
			continue;
		}

		size_t matched = 0;
		if((matched = match(str, "nodedef>"))) {
			if(!readNodeDef(str.substr(matched))) {
				return false;
			}
			mode = Mode::Node;
		} else if((matched = match(str, "edgedef>"))) {
			if(!readEdgeDef(str.substr(matched))) {
				return false;
			}
			mode = Mode::Edge;
		} else if(mode == Mode::Node) {
			if(!readNodeStmt(G, GA, str, line)) {
				return false;
			}
		} else if(mode == Mode::Edge) {
			if(!readEdgeStmt(G, GA, str, line)) {
				return false;
			}
		} else {
			GraphIO::logger.lout() << "Expected node or edge definition header "
			          << "(line " << line << ")." << std::endl;
			return false;
		}
	}

	return true;
}

}
}
