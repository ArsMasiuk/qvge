/** \file
 * \brief Declarations for GraphML Parser
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

#include <ogdf/fileformats/GraphIO.h>

#include <ogdf/basic/HashArray.h>
#include <ogdf/lib/pugixml/pugixml.h>

#include <sstream>
#include <unordered_map>

namespace ogdf {

class GraphMLParser {
private:
	pugi::xml_document m_xml;
	pugi::xml_node m_graphTag; // "Almost root" tag.

	 // Maps GraphML node id to Graph node.
	std::unordered_map<string, node> m_nodeId;

	// Maps attribute id to its name.
	std::unordered_map<string, string> m_attrName;

	bool readData(
		GraphAttributes &GA,
		const node &v, const pugi::xml_node nodeData);
	bool readData(
		GraphAttributes &GA,
		const edge &e, const pugi::xml_node edgeData);
	bool readData(
		ClusterGraphAttributes &CA,
		const cluster &c, const pugi::xml_node clusterData);

	// Finds all data-keys for given element and calls appropiate "readData".
	template <typename A, typename T>
	bool readAttributes(A &GA, const T &elem, const pugi::xml_node xmlElem) {
		for (pugi::xml_node dataTag : xmlElem.children("data")) {
			const bool result = readData(GA, elem, dataTag);
			if(!result) {
				return false;
			}
		}

		return true;
	}

	bool readNodes(
		Graph &G, GraphAttributes *GA,
		const pugi::xml_node rootTag);
	bool readEdges(
		Graph &G, GraphAttributes *GA,
		const pugi::xml_node rootTag);
	bool readClusters(
		Graph &G, ClusterGraph &C, ClusterGraphAttributes *CA,
		const cluster &rootCluster, const pugi::xml_node clusterRoot);

	bool m_error;

public:
	explicit GraphMLParser(std::istream &in);
	~GraphMLParser();

	bool read(Graph &G);
	bool read(Graph &G, GraphAttributes &GA);
	bool read(Graph &G, ClusterGraph &C);
	bool read(Graph &G, ClusterGraph &C, ClusterGraphAttributes &CA);
};

}
