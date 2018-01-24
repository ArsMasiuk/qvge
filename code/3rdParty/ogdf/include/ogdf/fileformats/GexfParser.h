/** \file
 * \brief Declaration of GEXF format reading utilities.
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

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>
#include <ogdf/lib/pugixml/pugixml.h>

#include <unordered_map>
#include <memory>
#include <sstream>


namespace ogdf {

namespace gexf {


class Parser {
private:
	std::istream &m_is;

	pugi::xml_document m_xml;
	pugi::xml_node m_graphTag, m_nodesTag, m_edgesTag;

	std::unordered_map<std::string, node> m_nodeId;
	std::unordered_map<std::string, cluster> m_clusterId;

	std::unordered_map<std::string, std::string> m_nodeAttr, m_edgeAttr;

	bool init();
	bool readNodes(Graph &G, GraphAttributes *GA);
	bool readEdges(Graph &G, ClusterGraph *C, GraphAttributes *GA);
	bool readCluster(
		Graph &G, ClusterGraph &C, ClusterGraphAttributes *CA,
		cluster rootCluster,
		const pugi::xml_node rootTag);
	bool readAttributes(
		GraphAttributes &GA, node v,
		const pugi::xml_node nodeTag);
	bool readAttributes(
		GraphAttributes &GA, edge e,
		const pugi::xml_node edgeTag);

	static void error(const pugi::xml_node tag, const std::string &msg);

public:
	explicit Parser(std::istream &is);

	bool read(Graph &G);
	bool read(Graph &G, GraphAttributes &GA);
	bool read(Graph &G, ClusterGraph &C);
	bool read(Graph &G, ClusterGraph &C, ClusterGraphAttributes &CA);
};

}
}
