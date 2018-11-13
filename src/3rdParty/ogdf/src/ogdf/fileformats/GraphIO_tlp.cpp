/** \file
 * \brief Implements TLP format write functionality of class GraphIO.
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

#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/fileformats/Tlp.h>


namespace ogdf {

namespace tlp {


static inline void writeRange(
	std::ostream &os,
	int a, int b)
{
	if(a == b) {
		os << " " << a;
	} else if(a + 1 == b) {
		os << " " << a << " " << b;
	} else {
		os << " " << a << ".." << b;
	}
}


static void writeNodes(
	std::ostream &os,
	const Graph &G)
{
	os << "\n";
	GraphIO::indent(os, 1) << "(nodes";

	for(node v = G.firstNode(); v;) {
		int a = v->index(), b = a;
		for(v = v->succ(); v && v->index() == b + 1; v = v->succ()) {
			b++;
		}

		writeRange(os, a, b);
	}

	os << ")";
}


static void writeEdges(
	std::ostream &os,
	const Graph &G)
{
	for(edge e : G.edges) {
		os << "\n";
		GraphIO::indent(os, 1) << "(edge " << e->index() << " "
		                       << e->source() << " " << e->target()
		                       << ")";
	}
}


static inline void writePropertyHeader(
	std::ostream &os,
	const Attribute &attr, const std::string &type)
{
	GraphIO::indent(os, 1) << "(property "
	                       << "0 " // No clue what was is idea behind this id.
	                       << type << " "
	                       << "\"" << toString(attr) << "\"";
}


static inline void writeColor(
	std::ostream &os,
	const Color &c)
{
	const int r = c.red(), g = c.green(), b = c.blue(), a = c.alpha();
	os << "\"(" << r << "," << g << "," << b << "," << a << ")\"";
}


static void writeProperties(
	std::ostream &os,
	const Graph &G, const GraphAttributes &GA)
{
	const long attrs = GA.attributes();

	if(attrs & (GraphAttributes::nodeLabel |
	            GraphAttributes::edgeLabel))
	{
		os << "\n";
		writePropertyHeader(os, Attribute::label, "string");
		os << "\n";
		GraphIO::indent(os, 2) << "(default \"\" \"\")";

		if(attrs & GraphAttributes::nodeLabel) {
			for(node v : G.nodes) {
				if(GA.label(v).empty()) {
					continue;
				}

				os << "\n";
				GraphIO::indent(os, 2) << "(node " << v->index() << " "
				                       << "\"" << GA.label(v) << "\")";
			}
		}

		if(attrs & GraphAttributes::edgeLabel) {
			for(edge e : G.edges) {
				if(GA.label(e).empty()) {
					continue;
				}

				os << "\n";
				GraphIO::indent(os, 2) << "(edge " << e->index() << " "
				                       << "\"" << GA.label(e) << "\")";
			}
		}

		os << ")";
	}

	if(attrs & (GraphAttributes::nodeStyle |
	            GraphAttributes::edgeStyle))
	{
		os << "\n";
		writePropertyHeader(os, Attribute::color, "color");

		const Color defaultColor = Color(); // Defaults to (0, 0, 0, 255).

		if(attrs & GraphAttributes::nodeStyle) {
			for(node v : G.nodes) {
				const Color &color = GA.fillColor(v);

				if(color == defaultColor) {
					continue;
				}

				os << "\n";
				GraphIO::indent(os, 2) << "(node " << v->index() << " ";
				writeColor(os, color);
				os << ")";
			}
		}

		if(attrs & GraphAttributes::edgeStyle) {
			for(edge e : G.edges) {
				const Color &color = GA.strokeColor(e);

				if(color == defaultColor) {
					continue;
				}

				os << "\n";
				GraphIO::indent(os, 2) << "(edge " << e->index() << " ";
				writeColor(os, color);
				os << ")";
			}
		}

		os << ")";
	}

	if(attrs & GraphAttributes::nodeGraphics) {
		os << "\n";
		writePropertyHeader(os, Attribute::position, "layout");
		for(node v : G.nodes) {
			const double z =
				(attrs & GraphAttributes::threeD) ? GA.z(v) : 0;

			os << "\n";
			GraphIO::indent(os, 2) << "(node " << v->index() << " \"("
			                       << GA.x(v) << ","
			                       << GA.y(v) << ","
			                       << z << ")\")";
		}
		os << ")";

		os << "\n";
		writePropertyHeader(os, Attribute::size, "size");
		for(node v : G.nodes) {
			os << "\n";
			GraphIO::indent(os, 2) << "(node " << v->index() << " \"("
			                       << GA.width(v) << ","
			                       << GA.height(v) << ")\")";
		}
		os << ")";
	}
}


/*
 * Helper functions populating \p nodes vector with nodes of given cluster and
 * all its subclusters. Why would I need something with such bad complexity?
 * See below, in #writeGraph function.
 */

static void getClusterChildren(cluster c, std::vector<node> &nodes)
{
	for(node v : c->nodes) {
		nodes.push_back(v);
	}

	for(cluster child : c->children) {
		getClusterChildren(child, nodes);
	}
}


bool clusterCompare(node a, node b)
{
	return a->index() < b->index();
}


static void writeCluster(
	std::ostream &os, int depth,
	const Graph &G, const ClusterGraph &C,
	cluster c)
{
	// Top-level cluster is handled as normal graph.
	if(c == C.rootCluster()) {
		return;
	}

	os << "\n";
	GraphIO::indent(os, depth) << "(cluster " << c->index();
	/*
	 * Tulip's subgraphs aren't 1-1 to graphs clusters. In fact, to declare
	 * a node in a subgraph we first need to declare it in the parent graph.
	 * Hence, when exporting given cluster and recursively its subclusters
	 * I need to know all the children in current cluster. And yes,
	 * the complexity of it is pretty bad (like O(c * n) I guess). Moreover,
	 * to get some file simplification I sort nodes for each layer which
	 * results in complexity O(c * n lg n).
	 */
	std::vector<node> clusterNodes;
	getClusterChildren(c, clusterNodes);
	std::sort(clusterNodes.begin(), clusterNodes.end(), clusterCompare);

	os << "\n";
	GraphIO::indent(os, depth + 1) << "(nodes";
	for(std::vector<node>::const_iterator it = clusterNodes.begin();
	    it != clusterNodes.end();)
	{
		// We want to keep file small, se we write whole ranges.
		int a = (*it)->index(), b = a;
		for(it++; it != clusterNodes.end() && (*it)->index() == b + 1; ++it) {
			b++;
		}

		writeRange(os, a, b);
	}
	os << ")";

	for(cluster child : c->children) {
		writeCluster(os, depth + 1, G, C, child);
	}

	os << ")";
}


static void writeGraph(
	std::ostream &os,
	const Graph &G, const ClusterGraph *C, const GraphAttributes *GA)
{
	os << "(tlp \"2.3\"\n";
	GraphIO::indent(os, 1) << "(nb_nodes " << G.numberOfNodes() << ")\n";
	GraphIO::indent(os, 1) << "(nb_edges " << G.numberOfEdges() << ")\n";

	writeNodes(os, G);
	os << "\n";
	writeEdges(os, G);

	if(C) {
		// We print additional newline to separate cluster definition from edges.
		if(G.numberOfEdges() > 0) {
			os << "\n";
		}

		const cluster c = C->rootCluster();
		for(cluster child : c->children) {
			writeCluster(os, 1, G, *C, child);
		}
	}

	if(GA) {
		// Once again, additional newline if required.
		if((C && C->numberOfClusters() > 1) || (!C && G.numberOfEdges() > 0)) {
			os << "\n";
		}

		writeProperties(os, G, *GA);
	}

	os << ")\n";
}

}


bool GraphIO::writeTLP(const Graph &G, std::ostream &os)
{
	bool result = os.good();

	if(result) {
		tlp::writeGraph(os, G, nullptr, nullptr);
	}

	return result;
}


bool GraphIO::writeTLP(const GraphAttributes &GA, std::ostream &os)
{
	bool result = os.good();

	if(result) {
		tlp::writeGraph(os, GA.constGraph(), nullptr, &GA);
	}

	return result;
}


bool GraphIO::writeTLP(const ClusterGraph &C, std::ostream &os)
{
	bool result = os.good();

	if(result) {
		tlp::writeGraph(os, C.constGraph(), &C, nullptr);
	}

	return result;
}


bool GraphIO::writeTLP(const ClusterGraphAttributes &CA, std::ostream &os)
{
	bool result = os.good();

	if(result) {
		tlp::writeGraph(os, CA.constGraph(), &CA.constClusterGraph(), &CA);
	}

	return result;
}

}
