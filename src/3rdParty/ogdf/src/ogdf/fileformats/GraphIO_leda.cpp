/** \file
 * \brief Implements read and write functionality for LEDA format.
 *
 * \author Carsten Gutwenger, Markus Chimani, Karsten Klein
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

#include <ogdf/basic/Logger.h>
#include <ogdf/fileformats/GraphIO.h>

using std::istringstream;


namespace ogdf {

static bool read_next_line(std::istream &is, string &buffer)
{
	while(std::getline(is,buffer)) {
		if(!buffer.empty() && buffer[0] != '#')
			return true;
	}
	return false;
}


static bool buffer_equal(const string &buffer, const char *str)
{
	string::size_type n = buffer.length();
	string::size_type left = 0, ns = n;
	while(left < n && isspace(buffer[left])) ++left;
	while(ns > 0 && isspace(buffer[ns-1])) --ns;
	return buffer.compare(left, ns, str) == 0;
}


bool GraphIO::readLEDA(Graph &G, std::istream &is)
{
	G.clear();

	// header

	try {
		string buffer;
		if(!read_next_line(is,buffer))
			return false;
		if(!buffer_equal(buffer, "LEDA.GRAPH")) // check type
			return false;
		if(!read_next_line(is,buffer)) // skip node type (ignored)
			return false;
		if(!read_next_line(is,buffer)) // skip edge type (ignored)
			return false;

		// nodes

		// check if next line specifies direction (-1 = directed, -2 = undirected) or nodes
		if(!read_next_line(is,buffer))
			return false;

		int n = std::stoi(buffer);
		if(n < 0) {
			if(!read_next_line(is,buffer))
				return false;
			n = std::stoi(buffer);
		}
		if(n < 0) return false; // makes no sense

		Array<node> nodes(1,n);
		for(int i = 1; i <= n; ++i) {
			if (!read_next_line(is, buffer))
				return false;
			nodes[i] = G.newNode();
		}

		// edges

		if(!read_next_line(is,buffer))
			return false;

		int m = std::stoi(buffer);
		if(m < 0) return false; // makes no sense

		for(int i = 1; i <= m; ++i) {
			if (!read_next_line(is, buffer))
				return false;
			istringstream iss(buffer);

			// read index of source and target node
			int src = -1, tgt = -1;
			iss >> src >> tgt;

			// indices valid?
			if (src < 1 || n < src || tgt < 1 || n < tgt)
				return false;

			G.newEdge(nodes[src],nodes[tgt]);
		}

	} catch(...) {
		return false;
	}

	return true;
}


bool GraphIO::writeLEDA(const Graph &G, std::ostream &os)
{
	bool result = os.good();

	if(result) {
		// write header
		os << "LEDA.GRAPH\n";    // format specification
		os << "void\n";            // no node type
		os << "void\n";            // no edge type
		os << "-1\n";            // directed graph

		// write nodes and assign indices 1, 2, ..., n
		os << G.numberOfNodes() << "\n";

		NodeArray<int> index(G);
		int nextIndex = 1;
		for (node v : G.nodes) {
			os << "|{}|\n";
			index[v] = nextIndex++;
		}

		// write edges
		os << G.numberOfEdges() << "\n";

		for (edge e : G.edges) {
			os << index[e->source()] << " " << index[e->target()] << " 0 |{}|\n";
		}

	}

	return result;
}

}
