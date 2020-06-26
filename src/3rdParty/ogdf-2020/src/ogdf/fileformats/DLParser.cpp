/** \file
 * \brief Implementation of UCINET DL format parser class.
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

#include <ogdf/fileformats/DLParser.h>
#include <ogdf/fileformats/GraphIO.h>


namespace ogdf {


DLParser::DLParser(std::istream &is) : m_istream(is)
{
	init();
}


void DLParser::init()
{
	m_initialized = false;
	m_nodeId.resize(1, nullptr);

	m_embedded = false;
	m_nodes = -1;
	m_format = Format::FullMatrix;
}


bool DLParser::initGraph(Graph &G)
{
	G.clear();

	if(m_nodes < 0) {
		GraphIO::logger.lout() << "Node count not specified or incorrect." << std::endl;
		return false;
	}

	for(int i = 0; i < m_nodes; i++) {
		m_nodeId.push_back(G.newNode());
	}
	m_initialized = true;

	return true;
}


// Common for both embedded and non-embedded mode.
static inline bool readMatrixRow(
	std::istream &is,
	Graph &G, GraphAttributes *GA, node v)
{
	const long attrs = GA ? GA->attributes() : 0;
	const bool iweight = (attrs & GraphAttributes::edgeIntWeight) != 0;
	const bool dweight = (attrs & GraphAttributes::edgeDoubleWeight) != 0;

	for(node u : G.nodes) {
		double weight;
		if(!(is >> weight)) {
			GraphIO::logger.lout() << "Expected matrix value." << std::endl;
			return false;
		}

		edge e = nullptr;
		if(weight != 0) {
			e = G.newEdge(v, u);
		}

		if(e && iweight) {
			GA->doubleWeight(e) = weight;
		} else if(e && dweight) {
			GA->intWeight(e) = static_cast<int>(weight);
		}
	}

	return true;
}


bool DLParser::readMatrix(Graph &G, GraphAttributes *GA)
{
	for(node v : G.nodes) {
		if(!readMatrixRow(m_istream, G, GA, v)) {
			return false;
		}
	}

	std::string extra;
	if(m_istream >> extra) {
		GraphIO::logger.lout() << "Expected EOF, but \"" << extra << "\" found." << std::endl;
		return false;
	}

	return true;
}


bool DLParser::readEmbeddedMatrix(Graph &G, GraphAttributes *GA)
{
	// First, top-label line.
	for(node v : G.nodes) {
		std::string label;
		if(!(m_istream >> label)) {
			GraphIO::logger.lout() << "Expected node embedded label." << std::endl;
			return false;
		}
		toLower(label);

		if(GA && GA->has(GraphAttributes::nodeLabel)) {
			GA->label(v) = label;
		}
		m_nodeLabel[label] = v;
	}

	// Now, each row have a label and then "normal" row.
	for(int i = 0; i < G.numberOfNodes(); i++) {
		std::string label;
		if(!(m_istream >> label)) {
			GraphIO::logger.lout() << "Expected node embedded label." << std::endl;
			return false;
		}
		toLower(label);

		node v = m_nodeLabel[label];
		if(!v) {
			GraphIO::logger.lout() << "Node with given label." << label << "\" not found." << std::endl;
			return false;
		}

		if(!readMatrixRow(m_istream, G, GA, v)) {
			return false;
		}
	}

	return true;
}


// Common for both embedded and non-emedded mode.
static inline bool readEdgeListRow(
	std::istringstream &is,
	Graph &G, GraphAttributes *GA, node v, node u)
{
	edge e = G.newEdge(v, u);
	double weight;
	is >> weight;
	if(GA && !is.bad()) {
		if(GA->has(GraphAttributes::edgeDoubleWeight)) {
			GA->doubleWeight(e) = weight;
		} else if(GA->has(GraphAttributes::edgeIntWeight)) {
			GA->intWeight(e) = static_cast<int>(weight);
		}
	}

	if (is.rdbuf()->in_avail()) {
		GraphIO::logger.lout() << "Could not parse entire row of edge list." << std::endl;
			return false;
	}

	return true;
}


inline node DLParser::requestLabel(
	GraphAttributes *GA, node &nextFree,
	const std::string &label)
{
	node v = m_nodeLabel[label];

	if(!v) {
		if(nextFree == nullptr) {
			GraphIO::logger.lout() << "Cannot assign label \"" << label << "\", "
			          << "node count in the graph is too low." << std::endl;
			return nullptr;
		}
		m_nodeLabel[label] = v = nextFree;
		if(GA && GA->has(GraphAttributes::nodeLabel)) {
			GA->label(v) = label;
		}
		nextFree = nextFree->succ();
	}

	return v;
}


bool DLParser::readEdgeList(Graph &G, GraphAttributes *GA)
{
	std::string buffer;
	for(size_t line = 1; std::getline(m_istream, buffer); line++) {
		buffer.erase(buffer.find_last_not_of(" \n\r\t") + 1);

		// Not necessary I guess, but does not do any harm.
		if(buffer.empty()) {
			continue;
		}

		std::istringstream is(buffer);
		int vid, uid;

		if(!(is >> vid >> uid) || !fineId(vid) || !fineId(uid)) {
			GraphIO::logger.lout() << "Node id incorrect (data line "
					  << line << "), maximum value is "
					  << m_nodeId.size() - 1 << "." << std::endl;
			return false;
		}

		if (!readEdgeListRow(is, G, GA, m_nodeId[vid], m_nodeId[uid])) {
			return false;
		}
	}

	return true;
}


bool DLParser::readEmbeddedEdgeList(Graph &G, GraphAttributes *GA)
{
	std::string buffer;

	node nextFree = G.firstNode();
	for(size_t line = 1; std::getline(m_istream, buffer); line++) {
		buffer.erase(buffer.find_last_not_of(" \n\r\t") + 1);

		if(buffer.empty()) {
			continue;
		}
		std::istringstream is(buffer);

		std::string vlabel, ulabel;
		if(!(is >> vlabel >> ulabel)) {
			GraphIO::logger.lout() << "Expected embedded node labels (data line "
					  << line << "), got \"" << is.str() << "\"." << std::endl;
			return false;
		}

		node v = requestLabel(GA, nextFree, vlabel);
		node u = requestLabel(GA, nextFree, ulabel);
		if(v == nullptr || u == nullptr) {
			return false;
		} else {
			if(!readEdgeListRow(is, G, GA, v, u)) {
				return false;
			}
		}
	}

	return true;
}


bool DLParser::readNodeList(Graph &G)
{
	std::string buffer;
	for(size_t line = 1; std::getline(m_istream, buffer); line++) {
		std::istringstream is(buffer);

		// As always, either ingore incorrect line or throw error.
		int vid;
		if(!(is >> vid)) {
			continue;
		}

		if(!fineId(vid)) {
			GraphIO::logger.lout() << "Node id incorrect (data line "
					  << line << "." << std::endl;
			return false;
		}
		node v = m_nodeId[vid];

		int uid;
		while(is >> uid) {
			if(!fineId(uid)) {
				GraphIO::logger.lout() << "Node id incorrect (data line "
						  << line << ")." << std::endl;
				return false;
			}

			G.newEdge(v, m_nodeId[uid]);
		}
	}

	return true;
}


bool DLParser::readEmbeddedNodeList(Graph &G, GraphAttributes *GA)
{
	std::string buffer;

	node nextFree = G.firstNode();
	for(size_t line = 1; std::getline(m_istream, buffer); line++) {
		std::istringstream is(buffer);

		std::string vlabel;
		if(!(is >> vlabel)) {
			continue;
		}

		node v = requestLabel(GA, nextFree, vlabel);
		if(v == nullptr) {
			return false;
		}

		std::string ulabel;
		while(is >> ulabel) {
			node u = requestLabel(GA, nextFree, ulabel);
			if(u == nullptr) {
				return false;
			}
			G.newEdge(v, u);
		}
	}

	return true;
}


bool DLParser::readData(Graph &G, GraphAttributes *GA)
{
	if(m_nodes < 0) {
		GraphIO::logger.lout() << "Number of nodes not specified or incorrect." << std::endl;
		return false;
	}

	if(!m_initialized) {
		initGraph(G);
	}

	// Now, depending on the method choosen we actually read the graph.
	switch(m_format) {
	case Format::FullMatrix:
		return m_embedded ? readEmbeddedMatrix(G, GA) : readMatrix(G, GA);
	case Format::EdgeList:
		return m_embedded ? readEmbeddedEdgeList(G, GA) : readEdgeList(G, GA);
	case Format::NodeList:
		return m_embedded ? readEmbeddedNodeList(G, GA) : readNodeList(G);
	}

	return false;
}


/*
 * This function is quite ugly (sphagetti code all the way). That is because
 * it is trying to mirror shitty DL format design. It is terrible, seriously.
 */
bool DLParser::readWithLabels(Graph &G, GraphAttributes *GA)
{

	std::string buffer;

	initGraph(G);
	for(node v = G.firstNode(); v;) {
		if(!(m_istream >> buffer)) {
			GraphIO::logger.lout() << "Expected node labels." << std::endl;
			return false;
		}
		toLower(buffer); // Labels should be lowercase.

		// We check whether we need to end reading labels.
		if(buffer == "data:") {
			return readData(G, GA);
		} else if(buffer == "labels") {
			// Or we have "labels embedded" information.
			m_istream >> buffer;
			toLower(buffer);
			if(buffer != "embedded:" && buffer != "embedded") {
				GraphIO::logger.lout() << "Expected embedded keyword, got \""
						  << buffer << "\"." << std::endl;
				return false;
			}

			m_embedded = true;
			break;
		}

		// We split input via comma and read labels for succesive nodes.
		std::istringstream is(buffer);
		while(std::getline(is, buffer, ',')) {
			// There is no need parsing labels if GA is not given.
			if(GA && GA->has(GraphAttributes::nodeLabel)) {
				GA->label(v) = buffer;
			}
			m_nodeLabel[buffer] = v;
			v = v->succ();
		}
	}

	m_istream >> buffer;
	toUpper(buffer);

	if(buffer == "LABELS") {
		m_istream >> buffer;
		toUpper(buffer);
		if(buffer != "EMBEDDED:" && buffer != "EMBEDDED") {
			GraphIO::logger.lout() << "Expected \"EMBEDDED\" keyword, got \""
					  << buffer << "\"." << std::endl;
			return false;
		}

		m_embedded = true;
		m_istream >> buffer;
		toUpper(buffer);
	}

	if(buffer != "DATA:") {
		GraphIO::logger.lout() << "Expected \"DATA:\" statement, got \""
				  << buffer << "\"." << std::endl;
		return false;
	}

	return readData(G, GA);
}


bool DLParser::readAssignment(
	Graph &G,
	const std::string &lhs, const std::string &rhs)
{

	if(lhs == "N") {
		std::istringstream is(rhs);
		if(!(is >> m_nodes)) {
			GraphIO::logger.lout() << "Incorrect number of nodes." << std::endl;
			return false;
		}
	} else if(lhs == "FORMAT") {
		if(rhs == "FULLMATRIX" || rhs == "FM") {
			m_format = Format::FullMatrix;
		} else if(rhs == "EDGELIST1" || rhs == "EL1") {
			m_format = Format::EdgeList;
		} else if(rhs == "NODELIST1" || rhs == "NL1") {
			m_format = Format::NodeList;
		} else {
			GraphIO::logger.lout() << "Unknown data format \"" << rhs << "\"."
			                       << "Supported formats are: FM, EL1 and NL1" << std::endl;
			return false;
		}
	} else {
		GraphIO::logger.lout() << "Unkown assignment statement: "
				  << "\"" << lhs << "\"." << std::endl;
		return false;
	}

	return true;
}


bool DLParser::readStatements(Graph &G, GraphAttributes *GA)
{
	std::string buffer;

	if(!(m_istream >> buffer)) {
		GraphIO::logger.lout() << "Expected statement." << std::endl;
		return false;
	}
	toUpper(buffer);

	if(buffer == "DATA:") {
		return readData(G, GA);
	}

	if(buffer == "LABELS:") {
		return readWithLabels(G, GA);
	}

	if(buffer == "LABELS") {
		m_istream >> buffer;
		toUpper(buffer);
		if(buffer != "EMBEDDED" && buffer != "EMBEDDED:") {
			GraphIO::logger.lout() << "Unknown statement "
					  << "\"LABELS " << buffer << "\". "
					  << "Did you mean \"LABELS:\" or \"LABELS EMBEDDED\"?" << std::endl;
			return false;
		}

		m_embedded = true;

		// ... and here we go again.
		return readStatements(G, GA);
	}

	// If none of the above, try interpreting this as assignment statement.
	size_t eq = buffer.find('=');
	std::string lhs, rhs;
	if(eq == std::string::npos) {
		// '=' not found inside, therefore buffer has to be left side.
		lhs = buffer;
		char c;
		if(!(m_istream >> c) || c != '=') {
			GraphIO::logger.lout() << "Expected definition or assignment "
					  << "statement, got: \"" << lhs << "\"." << std::endl;
			return false;
		}

		if(!(m_istream >> rhs)) {
			GraphIO::logger.lout() << "Expected assignment right side." << std::endl;
			return false;
		}
	} else if(eq == buffer.size() - 1) {
		// 'lhs= rhs' case.
		if(!(m_istream >> rhs)) {
			GraphIO::logger.lout() << "Expected assignment right side." << std::endl;
			return false;
		}
		lhs = buffer.substr(0, eq);
	} else {
		// 'lhs=rhs' case.
		lhs = buffer.substr(0, eq);
		rhs = buffer.substr(eq + 1);
	}
	toUpper(lhs);
	toUpper(rhs);

	return readAssignment(G, lhs, rhs) && readStatements(G, GA);

}


bool DLParser::readGraph(Graph &G, GraphAttributes *GA)
{
	init();
	std::string buffer;

	m_istream >> buffer;
	toUpper(buffer);

	if(buffer != "DL") {
		GraphIO::logger.lout() << "Expected the \"DL\" header, got: \""
				  << buffer << "\"." << std::endl;
	}

	return readStatements(G, GA);
}


}
