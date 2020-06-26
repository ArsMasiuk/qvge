/** \file
 * \brief Implements UCINET DL write functionality of class GraphIO.
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

#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/fileformats/GraphIO.h>

namespace ogdf {


static void writeMatrix(
	std::ostream &os,
	const Graph &G, const GraphAttributes *GA,
	const NodeArray<int> &index)
{
	os << "DATA:\n";
	const long attrs = GA ? GA->attributes() : 0;
	const int n = G.numberOfNodes();
	std::vector<double> matrix(n * n, 0);

	for(edge e : G.edges) {
		const int vs = index[e->source()];
		const int vt = index[e->target()];

		if(attrs & GraphAttributes::edgeDoubleWeight) {
			matrix[vs * n + vt] = GA->doubleWeight(e);
		} else if(attrs & GraphAttributes::edgeIntWeight) {
			matrix[vs * n + vt] = GA->intWeight(e);
		} else {
			matrix[vs * n + vt] = 1;
		}
	}

	for(node v : G.nodes) {
		bool space = false;
		for(node u : G.nodes) {
			if(space) {
				os << " ";
			}
			space = true;

			const int vs = index[v], vt = index[u];
			os << matrix[vs * n + vt];
		}
		os << "\n";
	}
}


static void writeEdges(
	std::ostream &os,
	const Graph &G, const GraphAttributes *GA,
	const NodeArray<int> &index)
{
	os << "DATA:\n";
	const long attrs = GA ? GA->attributes() : 0;

	for(edge e : G.edges) {
		os << (index[e->source()] + 1) << " " << (index[e->target()] + 1);

		if(attrs & GraphAttributes::edgeDoubleWeight) {
			os << " " << GA->doubleWeight(e);
		} else if(attrs & GraphAttributes::edgeIntWeight) {
			os << " " << GA->intWeight(e);
		}

		os << "\n";
	}
}


static bool writeGraph(
	std::ostream &os,
	const Graph &G, const GraphAttributes *GA)
{
	std::ios_base::fmtflags currentFlags = os.flags();
	os.flags(currentFlags | std::ios::fixed);

	bool result = os.good();

	if(result) {
		const long long n = G.numberOfNodes(), m = G.numberOfEdges();

		os << "DL N = " << n << "\n";

		// First pick output format.
		// We cannot use matrix output if we have (directed) parallel edges.
		// If we have no parallel edges, we base our decision on the
		// number of bytes (length) used by the respective representation:
		//   * (2n + 1) n is the length of the matrix representation.
		//   * 6m is a rough estimate for the length of the edge list representation.
		enum class Format {
			Matrix, Edges
		} format = isParallelFree(G) && (2 * n + 1) * n < 6 * m ? Format::Matrix : Format::Edges;

		// Specify output format.
		os << "FORMAT = ";
		if (format == Format::Matrix) {
			os << "fullmatrix\n";
		} else if (format == Format::Edges) {
			os << "edgelist1\n";
		}

		NodeArray<int> indices{G};
		int index{0};
		for (node v : G.nodes) {
			indices[v] = index++;
		}

		const long attrs = GA ? GA->attributes() : 0;
		if(attrs & GraphAttributes::nodeLabel) {
			os << "LABELS:\n";
			bool comma = false;
			for(node v : G.nodes) {
				if(comma) {
					os << ",";
				}
				comma = true;
				os << GA->label(v);
			}
			os << "\n";
		}

		if (format == Format::Matrix) {
			writeMatrix(os, G, GA, indices);
		} else if (format == Format::Edges) {
			writeEdges(os, G, GA, indices);
		}
	}

	os.flags(currentFlags);

	return result;
}


bool GraphIO::writeDL(const Graph &G, std::ostream &os)
{
	return writeGraph(os, G, nullptr);
}


bool GraphIO::writeDL(const GraphAttributes &GA, std::ostream &os)
{
	return writeGraph(os, GA.constGraph(), &GA);
}

}
