/** \file
 * \brief TLP format parser utility implementation.
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

#include <ogdf/fileformats/TlpParser.h>
#include <ogdf/fileformats/Utils.h>
#include <ogdf/fileformats/GraphIO.h>


namespace ogdf {

namespace tlp {


inline void Parser::tokenError(const char *str, bool got)
{
#ifdef OGDF_DEBUG
	if(m_begin == m_end) {
		GraphIO::logger.lout() << str << "." << std::endl;
	} else {
		if (got) {
			GraphIO::logger.lout() << str << " at " << m_begin->line << ", " << m_begin->column << " (got " << *m_begin << ")." << std::endl;
		} else {
			GraphIO::logger.lout() << str << " at " << m_begin->line << ", " << m_begin->column << "." << std::endl;
		}
	}
#endif
}


inline void Parser::tokenError(const std::string &str, bool got)
{
	tokenError(str.c_str(), got);
}


Parser::Parser(std::istream &is) : m_istream(is)
{
}


bool Parser::readEdge(Graph &G)
{
	std::stringstream ss;
	if(m_begin == m_end || !m_begin->identifier()) {
		tokenError("expected edge id");
		return false;
	}
	ss << *(m_begin->value) << " ";
	++m_begin;

	if(m_begin == m_end || !m_begin->identifier()) {
		tokenError("expected source id of edge");
		return false;
	}
	ss << *(m_begin->value) << " ";
	++m_begin;

	if(m_begin == m_end || !m_begin->identifier()) {
		tokenError("expected target id of edge");
		return false;
	}
	ss << *(m_begin->value);
	++m_begin;

	int id, sid, tid;
	if(!(ss >> id >> sid >> tid)) {
		tokenError(
			"incorrect edge statement format "
			"(got \"" + ss.str() + "\", expected (\"int int int\")", false);
		return false;
	}

	node source = m_idNode[sid], target = m_idNode[tid];
	if(!source || !target) {
		GraphIO::logger.lout() << "Node with id " << sid << " or " << tid << " is not declared." << std::endl;
		return false;
	}

	if (m_idEdge[id] != nullptr) {
		GraphIO::logger.lout() << "Encountered duplicate edge id: " + to_string(id) << std::endl;
		return false;
	}

	m_idEdge[id] = G.newEdge(source, target);

	if(m_begin == m_end || !m_begin->rightParen()) {
		tokenError("expected \")\" for edge statement");
		return false;
	}
	++m_begin;

	return true;
}


inline bool Parser::applyNodes(
	Graph &G, ClusterGraph *C, cluster c,
	const std::string &str)
{
	/*
	 * Node statement is either a single node id or a range with format a..b.
	 * To handle this we simply care about ranges, setting a = b = id if
	 * non-range is given. To convert string to a integer we use this trivial
	 * loop as only non-negative integers can be valid literals.
	 */
	int a = 0, b = 0;

	std::string::const_iterator it = str.begin();
	for(; it != str.end() && isdigit(*it); ++it) {
		a = 10 * a + (*it) - '0';
	}

	if(it == str.end()) {
		b = a;
	} else if((it + 1) != str.end() && *it == '.' && *(it + 1) == '.') {
		it += 2;
		for(; it != str.end() && isdigit(*it); ++it) {
			b = 10 * b + (*it) - '0';
		}

		if(it != str.end()) {
			tokenError("incorrect range literal");
			return false;
		}
	} else {
		tokenError("expected id literal or range");
		return false;
	}

	for(int nid = a; nid <= b; nid++) {
		node v = m_idNode[nid];
		if(!v) {
			m_idNode[nid] = v = G.newNode();
		}

		if(C) {
			/*
			 * Okay, so we move here a node to another cluster if and only if
			 * current cluster is deeper then old one (root by default). TLP
			 * format allows node to be in two separate subgraphs, but OGDF's
			 * clusters concept is more tree-alike. Therefore, I can either
			 * fail with an error or make this undefined behaviour (declaration
			 * ordering matters). I chose the latter one.
			 */
			cluster prev = C->clusterOf(v);
			if(c->depth() > prev->depth()) {
				C->reassignNode(v, c);
			}
		}
	}

	return true;
}


bool Parser::readNodes(Graph &G, ClusterGraph *C, cluster c)
{
	while(m_begin != m_end && !m_begin->rightParen()) {
		if(!m_begin->identifier()) {
			tokenError("expected node id for \"nodes\" statement");
			return false;
		}

		if(!applyNodes(G, C, c, *(m_begin->value))) {
			return false;
		}
		++m_begin;
	}

	if(m_begin == m_end || !m_begin->rightParen()) {
		tokenError("expected \")\" for \"nodes\" statement");
		return false;
	}
	++m_begin;

	return true;
}


bool Parser::readCluster(Graph &G, ClusterGraph *C, cluster root)
{
	if(m_begin == m_end || !m_begin->identifier()) {
		tokenError("expected cluster id");
		return false;
	}
	const std::string &cid = *(m_begin->value);
	++m_begin;

	while(m_begin != m_end && m_begin->leftParen()) {
		++m_begin;
		if(!readClusterStatement(G, C, root)) {
			if (!G.empty()) {
				GraphIO::logger.lout() << "Encountered duplicate node section" << std::endl;
				return false;
			}

			return false;
		}
	}

	if(m_begin == m_end || !m_begin->rightParen()) {
		tokenError("expected \")\" for cluster " + cid + ".\n");
		return false;
	}
	++m_begin;

	return true;
}


static inline bool setAttribute(
	GraphAttributes &GA, node v,
	const Attribute &attr, const std::string &value)
{
	const long attrs = GA.attributes();

	switch(attr) {
	case Attribute::label:
		if(attrs & GraphAttributes::nodeLabel) {
			GA.label(v) = value;
		}
		break;
	case Attribute::strokeColor:
		if(attrs & GraphAttributes::nodeStyle) {
			std::istringstream is(value);
			int r, g, b, a;
			is >> TokenIgnorer('(') >> r >> TokenIgnorer(',') >> g >> TokenIgnorer(',') >> b >> TokenIgnorer(',') >> a >> TokenIgnorer(')');
			GA.strokeColor(v) = Color(r, g, b, a);
		}
		break;
	case Attribute::color:
		if(attrs & GraphAttributes::nodeStyle) {
			std::istringstream is(value);
			int r, g, b, a;
			is >> TokenIgnorer('(') >> r >> TokenIgnorer(',') >> g >> TokenIgnorer(',') >> b >> TokenIgnorer(',') >> a >> TokenIgnorer(')');
			GA.fillColor(v) = Color(r, g, b, a);
		}
		break;
	case Attribute::position:
		if(attrs & GraphAttributes::nodeGraphics) {
			std::istringstream is(value);
			double x, y, z;
			is >> TokenIgnorer('(') >> x >> TokenIgnorer(',') >> y >> TokenIgnorer(',') >> z >> TokenIgnorer(')');
			GA.x(v) = x;
			GA.y(v) = y;
			if(attrs & GraphAttributes::threeD) {
				GA.z(v) = z;
			}
		}
		break;
	case Attribute::size:
		if(attrs & GraphAttributes::nodeGraphics) {
			std::istringstream is(value);
			double width, height;
			is >> TokenIgnorer('(') >> width >> TokenIgnorer(',') >> height >> TokenIgnorer(')');
			GA.width(v) = width;
			GA.height(v) = height;
		}
		break;
	case Attribute::strokeWidth:
		if(attrs & GraphAttributes::nodeStyle) {
			std::istringstream is(value);
			is >> GA.strokeWidth(v);
		}
		break;
	case Attribute::strokeType:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.strokeType(v) = fromString<StrokeType>(value);
		}
		break;
	case Attribute::fillPattern:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.fillPattern(v) = fromString<FillPattern>(value);
		}
		break;
	case Attribute::fillBackground:
		if(attrs & GraphAttributes::nodeStyle) {
			std::istringstream is(value);
			int r, g, b, a;
			is >> TokenIgnorer('(') >> r >> TokenIgnorer(',') >> g >> TokenIgnorer(',') >> b >> TokenIgnorer(',') >> a >> TokenIgnorer(')');
			GA.fillBgColor(v) = Color(r, g, b, a);
		}
		break;
	case Attribute::shape:
		if(attrs & GraphAttributes::nodeStyle) {
			GA.shape(v) = fromString<Shape>(value);
		}
		break;
	default:
		break;
	}

	return true;
}


static inline bool setAttribute(
	GraphAttributes &GA, edge e,
	const Attribute &attr, const std::string &value)
{
	const long attrs = GA.attributes();

	switch(attr) {
	case Attribute::label:
		if(attrs & GraphAttributes::edgeLabel) {
			GA.label(e) = value;
		}
		break;
	case Attribute::color:
		if(attrs & GraphAttributes::edgeStyle) {
			std::istringstream is(value);
			int r, g, b, a;
			is >> TokenIgnorer('(') >> r >> TokenIgnorer(',') >> g >> TokenIgnorer(',') >> b >> TokenIgnorer(',') >> a >> TokenIgnorer(')');
			GA.strokeColor(e) = Color(r, g, b, a);
		}
		break;
	default:
		break;
	}

	return true;
}


bool Parser::readProperty(Graph &G, GraphAttributes *GA)
{
	if(m_begin == m_end && !m_begin->identifier()) {
		tokenError("expected cluster id for property");
		return false;
	}
	++m_begin; // I don't really know what that id is for so move along...

	if(m_begin == m_end && !m_begin->identifier()) {
		tokenError("expected property type");
		return false;
	}
	++m_begin;// ... and the type value is completely useless for us as well.

	if(m_begin == m_end && !m_begin->string()) {
		tokenError("expected property name string");
		return false;
	}
	const std::string &pname = *(m_begin->value);
	++m_begin;

	const Attribute attr = toAttribute(pname);

	std::string nodeDefault, edgeDefault;
	NodeArray<bool> doneNode(G, false);
	EdgeArray<bool> doneEdge(G, false);

	// So now we read and set all the properties.
	while(m_begin != m_end && m_begin->leftParen()) {
		++m_begin;
		if(!readPropertyStatement(
			GA, attr,
			doneNode, nodeDefault,
			doneEdge, edgeDefault))
		{
			return false;
		}
	}

	if(m_begin == m_end || !m_begin->rightParen()) {
		tokenError("expected \")\" for \"" + pname + "\" property definition");
		return false;
	}
	++m_begin;

	/*
	 * And now we can apply defaults to those which were not set before if
	 * GraphAttributes is given, attribute is known and these defaults are
	 * set. Otherwise our job is done here.
	 */
	if(!GA || attr == Attribute::unknown) {
		return true;
	}

	if(!nodeDefault.empty()) {
		for(node v : G.nodes) {
			if(!doneNode[v] && !setAttribute(*GA, v, attr, nodeDefault)) {
				return false;
			}
		}
	}

	if(!edgeDefault.empty()) {
		for(edge e : G.edges) {
			if(!doneEdge[e] && !setAttribute(*GA, e, attr, edgeDefault)) {
				return false;
			}
		}
	}

	return true;
}


bool Parser::readPropertyStatement(
	GraphAttributes *GA, const Attribute &attr,
	NodeArray<bool> &nodeDone, std::string &nodeDefault,
	EdgeArray<bool> &edgeDone, std::string &edgeDefault)
{
	if(m_begin == m_end || !m_begin->identifier()) {
		tokenError("expected property statement");
		return false;
	}

	const std::string &head = *(m_begin->value);
	++m_begin;

	if(head == "node") {
		if(m_begin == m_end || !m_begin->identifier()) {
			tokenError("expected node id");
			return false;
		}
		std::istringstream is(*(m_begin->value));
		++m_begin;

		int nid;
		node v;
		if(!(is >> nid) || !(v = m_idNode[nid])) {
			tokenError("incorrect node id");
			return false;
		}

		if(m_begin == m_end || !m_begin->string()) {
			tokenError("expected node property value");
			return false;
		}

		const std::string &value = *(m_begin->value);
		++m_begin;

		if(GA && !setAttribute(*GA, v, attr, value)) {
			return false;
		}
		nodeDone[v] = true;
	} else if(head == "edge") {
		if(m_begin == m_end || !m_begin->identifier()) {
			tokenError("expected edge id");
			return false;
		}
		std::istringstream is(*(m_begin->value));
		++m_begin;

		int eid;
		edge e;
		if(!(is >> eid) || !(e = m_idEdge[eid])) {
			tokenError("incorrect edge id");
			return false;
		}

		if(m_begin == m_end || !m_begin->string()) {
			tokenError("expected edge property value");
		}

		const std::string &value = *(m_begin->value);
		++m_begin;

		if(GA && !setAttribute(*GA, e, attr, value)) {
			return false;
		}
		edgeDone[e] = true;
	} else if(head == "default") {
		if(m_begin == m_end || !m_begin->string()) {
			tokenError("expected node default property value");
			return false;
		}
		nodeDefault = *(m_begin->value);
		++m_begin;

		if(m_begin == m_end || !m_begin->string()) {
			tokenError("expected edge default property value");
			return false;
		}

		edgeDefault = *(m_begin->value);
		++m_begin;
	} else {
		tokenError("unknown property statement \"" + head + "\"", false);
		return false;
	}

	if(m_begin == m_end || !m_begin->rightParen()) {
		tokenError("expected \")\" for \"" + head + "\" property statement");
		return false;
	}
	++m_begin;

	return true;
}


bool Parser::readClusterStatement(Graph &G, ClusterGraph *C, cluster c)
{
	if(m_begin == m_end || !m_begin->identifier()) {
		tokenError("expected cluster statement head");
		return false;
	}
	const std::string &head = *(m_begin->value);
	++m_begin;

	if(head == "edge") {
		return readEdge(G);
	}

	if(head == "nodes") {
		return readNodes(G, C, c);
	}

	if(head == "cluster") {
		return readCluster(G, C, C ? C->newCluster(c) : nullptr);
	}

	tokenError("unknown cluster statement \"" + head + "\"", false);
	return false;
}


bool Parser::readStatement(Graph &G, GraphAttributes *GA, ClusterGraph *C)
{
	if(m_begin == m_end || !m_begin->identifier()) {
		tokenError("expected statement identifier");
		return false;
	}

	const std::string &head = *(m_begin->value);
	++m_begin;

	if(head == "edge") {
		return readEdge(G);
	}

	if(head == "nodes") {
		if (!G.empty()) {
			GraphIO::logger.lout() << "Encountered duplicate node section" << std::endl;
			return false;
		}

		return readNodes(G, C, C ? C->rootCluster() : nullptr);
	}

	if(head == "cluster") {
		return readCluster(G, C, C ? C->rootCluster() : nullptr);
	}

	if(head == "property") {
		return readProperty(G, GA);
	}

	// We don't care much about these currently...
	if(head == "date") {
		if(m_begin == m_end || !m_begin->string()) {
			tokenError("expected date string");
			return false;
		}
		++m_begin;
	} else if(head == "author" ) {
		if(m_begin == m_end || !m_begin->string()) {
			tokenError("expected author string");
			return false;
		}
		++m_begin;
	} else if(head == "comments") {
		if(m_begin == m_end || !m_begin->string()) {
			tokenError("expected comments string");
			return false;
		}
		++m_begin;
	} else if(head == "nb_nodes") {
		if(m_begin == m_end || !m_begin->identifier()) {
			tokenError("expected node count");
			return false;
		}
		++m_begin;
	} else if(head == "nb_edges") {
		if(m_begin == m_end || !m_begin->identifier()) {
			tokenError("expected edge count");
			return false;
		}
		++m_begin;
	} else {
		GraphIO::logger.lout(Logger::Level::Minor) << "Unknown statement \"" << head << "\", ignoring.\n" << std::endl;
		// We got unknown statement, so we ignore until ending paren.
		int opened = 1;
		while(m_begin != m_end && opened != 0) {
			if(m_begin->leftParen()) {
				opened++;
			} else if(m_begin->rightParen()) {
				opened--;
			}
			++m_begin;
		}

		if(opened != 0) {
			tokenError("expected paren closing \"" + head +"\"");
			return false;
		}

		// We handled right paren above, so we quit here.
		return true;
	}

	// ... but ending paren has to be read.
	if(m_begin == m_end || !m_begin->rightParen()) {
		tokenError("expected \")\" for \"" + head + "\" statement");
		return false;
	}
	++m_begin;

	return true;
}


bool Parser::readGraph(Graph &G, GraphAttributes *GA, ClusterGraph *C)
{
	G.clear();

	Lexer lexer(m_istream);

	if(!lexer.tokenize()) {
		GraphIO::logger.lout() << "Lexical analysis failed." << std::endl;
		return false;
	}
	m_begin = lexer.tokens().begin();
	m_end = lexer.tokens().end();

	if(m_begin == m_end || !m_begin->leftParen()) {
		GraphIO::logger.lout() << "Expected \"(\"." << std::endl;
		return false;
	}
	++m_begin;

	if(m_begin == m_end || !m_begin->identifier("tlp")) {
		tokenError("expected \"tlp\" statement");
		return false;
	}
	++m_begin;

	if(m_begin == m_end || !m_begin->string()) {
		tokenError("expected version string");
		return false;
	}
	++m_begin;

	m_idEdge.clear();

	while(m_begin != m_end && m_begin->leftParen()) {
		++m_begin;
		if(!readStatement(G, GA, C)) {
			return false;
		}
	}

	if(m_begin == m_end || !m_begin->rightParen()) {
		tokenError("expected \")\" for \"tlp\" statement");
		return false;
	}
	++m_begin;

	if(m_begin != m_end) {
		tokenError("expected end of file");
		return false;
	}

	return true;
}

}
}
