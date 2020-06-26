/** \file
 * \brief Declarations for GDF Parser
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

#pragma once

#include <ogdf/basic/HashArray.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/fileformats/GDF.h>

#include <istream>
#include <vector>
#include <string>
#include <sstream>


namespace ogdf {

namespace gdf {


class Parser {
private:
	std::istream &m_istream;
	HashArray<std::string, node> m_nodeId;
	std::vector<NodeAttribute> m_nodeAttrs;
	std::vector<EdgeAttribute> m_edgeAttrs;

	bool readAttributes(
		GraphAttributes &GA, node v,
		const std::vector<std::string> &values);
	bool readAttributes(
		GraphAttributes &GA, edge e,
		const std::vector<std::string> &values);

	bool readNodeDef(const std::string &str);
	bool readEdgeDef(const std::string &str);

	bool readNodeStmt(
		Graph &G, GraphAttributes *GA,
		const std::string &str, size_t line);
	bool readEdgeStmt(
		Graph &G, GraphAttributes *GA,
		const std::string &str, size_t line);

	bool readGraph(Graph &G, GraphAttributes *GA);

public:
	explicit Parser(std::istream &is);

	bool read(Graph &G) {
		return readGraph(G, nullptr);
	}

	bool read(Graph &G, GraphAttributes &GA) {
		return readGraph(G, &GA);
	}
};

}
}
