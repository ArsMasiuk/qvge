/** \file
 * \brief Implements DOT write functionality of class GraphIO.
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
#include <ogdf/fileformats/DOT.h>

namespace ogdf {

namespace dot {


template <typename T>
static inline void writeAttribute(
	std::ostream &out, bool &separator,
	const std::string &name, const T &value)
{
	if(separator) {
		out << ", ";
	}

	out << name << "=\"" << value << "\"";
	separator = true;
}


static inline void writeAttributes(
	std::ostream &out,
	const GraphAttributes &GA, const node &v)
{
	const long flags = GA.attributes();

	out << "[";

	bool separator = false; // Wheter to put separator before attribute.

	if(flags & GraphAttributes::nodeId) {
		writeAttribute(out, separator, "id", GA.idNode(v));
	}

	if(flags & GraphAttributes::nodeLabel) {
		writeAttribute(out, separator, "label", GA.label(v));
	}

	if(flags & GraphAttributes::nodeTemplate) {
		writeAttribute(out, separator, "comment", GA.templateNode(v));
	}

	if(flags & GraphAttributes::nodeGraphics) {
		writeAttribute(out, separator, "width", GA.width(v));
		writeAttribute(out, separator, "height", GA.height(v));
		writeAttribute(out, separator, "shape", dot::toString(GA.shape(v)));

		out << ", pos=\"" << GA.x(v) << "," << GA.y(v);
		if(flags & GraphAttributes::threeD) {
			out << "," << GA.z(v);
		}
		out << "\"";
	}

	if(flags & GraphAttributes::nodeLabelPosition) {
		// No need to check separator, as `nodeLabel` must be set as well
		// and is handled earlier.
		out << ", labelpos=\"" << GA.xLabel(v) << "," << GA.yLabel(v);
		if(flags & GraphAttributes::threeD) {
			out << "," << GA.zLabel(v);
		}
		out << "\"";
	}

	if(flags & GraphAttributes::nodeStyle) {
		writeAttribute(out, separator, "color", GA.strokeColor(v));
		writeAttribute(out, separator, "fillcolor", GA.fillColor(v));
		writeAttribute(out, separator, "stroketype", toString(GA.strokeType(v)));
		writeAttribute(out, separator, "strokewidth", GA.strokeWidth(v));
		writeAttribute(out, separator, "fillpattern", toString(GA.fillPattern(v)));
		writeAttribute(out, separator, "fillbgcolor", GA.fillBgColor(v));
	}

	if(flags & GraphAttributes::nodeType) {
		writeAttribute(out, separator, "type", int(GA.type(v)));
	}

	if(flags & GraphAttributes::nodeWeight) {
		writeAttribute(out, separator, "weight", GA.weight(v));
	}

	out << "]";
}


static inline void writeAttributes(
	std::ostream &out,
	const GraphAttributes &GA, const edge &e)
{
	const long flags = GA.attributes();

	out << "[";

	bool comma = false; // Whether to put comma before attribute.

	if(flags & GraphAttributes::edgeLabel) {
		writeAttribute(out, comma, "label", GA.label(e));
	}

	if(flags & GraphAttributes::edgeDoubleWeight) {
		writeAttribute(out, comma, "weight", GA.doubleWeight(e));
	} else if(flags & GraphAttributes::edgeIntWeight) {
		writeAttribute(out, comma, "weight", GA.intWeight(e));
	}

	if(flags & GraphAttributes::edgeGraphics) {
		// This should be legal cubic B-Spline in the future.
		std::stringstream sstream;
		std::ios_base::fmtflags currentFlags = sstream.flags();
		sstream.flags(currentFlags | std::ios::fixed);
		for(const DPoint &p : GA.bends(e)) {
			sstream << p.m_x << "," << p.m_y << " ";
		}
		sstream.flags(currentFlags);

		writeAttribute(out, comma, "pos", sstream.str());
	}

	if(flags & GraphAttributes::edgeArrow) {
		writeAttribute(out, comma, "dir", dot::toString(GA.arrowType(e)));
	}

	if(flags & GraphAttributes::edgeStyle) {
		writeAttribute(out, comma, "color", GA.strokeColor(e));
		writeAttribute(out, comma, "stroketype", GA.strokeType(e));
		writeAttribute(out, comma, "strokewidth", GA.strokeWidth(e));
	}

	if(flags & GraphAttributes::edgeType) {
		writeAttribute(out, comma, "type", dot::toString(GA.type(e)));
	}

	if(flags & GraphAttributes::edgeSubGraphs) {
		const uint32_t mask = GA.subGraphBits(e);

		// Iterate over all subgraphs and print the ones the edge is part of.
		std::stringstream sstream;
		for (size_t sg = 0; sg < sizeof(mask) * 8; ++sg) {
			if((1 << sg) & mask) {
				sstream << (sg == 0 ? "" : " ") << sg;
			}
		}
		writeAttribute(out, comma, "available_for", sstream.str());
	}

	out << "]";
}


static inline bool writeAttributes(
	std::ostream &out,
	const ClusterGraphAttributes &CA, const cluster &c)
{
	const long flags = CA.attributes();
	bool separator = false;

	if(flags & ClusterGraphAttributes::clusterGraphics) {
		writeAttribute(out, separator, "width", CA.width(c));
		writeAttribute(out, separator, "height", CA.height(c));
		out << ", pos=\"" << CA.x(c) << "," << CA.y(c) << "\"";
	}
	if(flags & ClusterGraphAttributes::clusterStyle) {
		writeAttribute(out, separator, "color", CA.strokeColor(c));
		writeAttribute(out, separator, "stroketype", CA.strokeType(c));
		writeAttribute(out, separator, "strokewidth", CA.strokeWidth(c));
		writeAttribute(out, separator, "fillpattern", CA.fillPattern(c));
		writeAttribute(out, separator, "fillcolor", CA.fillColor(c));
		writeAttribute(out, separator, "fillbgcolor", CA.fillBgColor(c));
	}
	if(flags & ClusterGraphAttributes::clusterLabel) {
		writeAttribute(out, separator, "label", CA.label(c));
	}
	if(flags & ClusterGraphAttributes::clusterTemplate) {
		writeAttribute(out, separator, "comment", CA.templateCluster(c));
	}
	return separator;
}


static inline bool writeHeader(
	std::ostream &out, const int &depth,
	const GraphAttributes *GA, bool writeAttributes = true)
{
	if(GA) {
		GraphIO::indent(out, depth) << (GA->directed() ? "digraph" : "graph")
		                            << " G {\n";
	} else {
		GraphIO::indent(out, depth) << "digraph G {\n";
		return false;
	}

	if (!writeAttributes) return false;

	bool whitespace = false;

	if(GA->has(GraphAttributes::threeD)) {
		GraphIO::indent(out, depth + 1) << "graph [";
		writeAttribute(out, whitespace, "dim", 3);
		out << "]\n";
	}
	return whitespace;
}

static inline bool writeHeader(
	std::ostream &out, const int &depth,
	const ClusterGraphAttributes *CA, cluster rootCluster,
	cluster c, int clusterId)
{
	if (rootCluster == c) {
		writeHeader(out, depth, CA, false);
	}
	else {
		GraphIO::indent(out, depth) << "subgraph cluster" << clusterId << " {\n";
	}

	if (!CA) return false;

	std::ostringstream attr;
	attr.setf(std::ios::fixed);

	bool separator = false;
	separator = writeAttributes(attr, *CA, c);
	if(CA->has(GraphAttributes::threeD)) {
		writeAttribute(attr, separator, "dim", 3);
	}
	string attributes = attr.str();
	if (!attributes.empty()) {
		GraphIO::indent(out, depth + 1) << "graph ["
			<< attributes << "]\n";
	}

	return separator;
}


static inline bool writeEdge(
	std::ostream &out, const int &depth,
	const GraphAttributes *GA, const edge &e)
{
	GraphIO::indent(out, depth) << e->source()
	                            << (GA && !GA->directed() ? " -- " : " -> ")
	                            << e->target();

	if(GA) {
		out << " ";
		writeAttributes(out, *GA, e);
	}

	out << "\n";
	return true;
}


static inline bool writeNode(
	std::ostream &out, const int &depth,
	const GraphAttributes *GA, const node &v
)
{
	// Write a node iff it has some attributes or has no edges.
	if(!GA && v->degree() > 0) {
		return false;
	}

	GraphIO::indent(out, depth) << v;

	if(GA) {
		out << " ";
		writeAttributes(out, *GA, v);
	}

	out << "\n";
	return true;
}


static bool writeCluster(
	std::ostream &out, int depth,
	const ClusterArray < std::vector<edge> > &edgeMap,
	const ClusterGraph &C, const ClusterGraphAttributes *CA, const cluster &c,
	int &clusterId)
{
	std::ios_base::fmtflags currentFlags = out.flags();
	out.flags(currentFlags | std::ios::fixed);
	bool result = out.good();

	if(result) {
		bool whitespace; // True if a whitespace should printed (readability).

		whitespace = writeHeader(out, depth++, CA, C.rootCluster(), c, clusterId);
		clusterId++;

		if (whitespace) {
			out << "\n";
		}

		// Recursively export all subclusters.
		whitespace = false;
		for (cluster child : c->children) {
			writeCluster(out, depth, edgeMap, C, CA, child, clusterId);
			whitespace = true;
		}

		if (whitespace) {
			out << "\n";
		}

		// Then, print all nodes whithout an adjacent edge.
		whitespace = false;
		for (node v : c->nodes) {
			whitespace |= writeNode(out, depth, CA, v);
		}

		if (whitespace) {
			out << "\n";
		}

		// Finally, we print all edges for this cluster (ugly version for now).
		const std::vector<edge> &edges = edgeMap[c];
		whitespace = false;
		for (auto &e : edges) {
			whitespace |= writeEdge(out, depth, CA, e);
		}

		GraphIO::indent(out, --depth) << "}\n";
	}

	out.flags(currentFlags);

	return result;
}


static bool writeGraph(
	std::ostream &out,
	const Graph &G, const GraphAttributes *GA)
{
	std::ios_base::fmtflags currentFlags = out.flags();
	out.flags(currentFlags | std::ios::fixed);

	bool result = out.good();

	if(result) {
		bool whitespace = false;

		whitespace |= writeHeader(out, 0, GA);

		if (whitespace) {
			out << "\n";
		}

		// We need to print all the nodes that do not have any adjacent edge.
		whitespace = false;
		for (node v : G.nodes) {
			whitespace |= dot::writeNode(out, 1, GA, v);
		}

		if (whitespace) {
			out << "\n";
		}

		// In this dummy version we just output list of all edges. It works, sure,
		// but is ugly as hell. A nicer approach has to be developed in future.
		whitespace = false;
		for (edge e : G.edges) {
			whitespace |= dot::writeEdge(out, 1, GA, e);
		}

		out << "}\n";
	}

	out.flags(currentFlags);

	return result;
}

}


bool GraphIO::writeDOT(const Graph &G, std::ostream &out)
{
	return dot::writeGraph(out, G, nullptr);
}


bool GraphIO::writeDOT(const GraphAttributes &GA, std::ostream &out)
{
	return dot::writeGraph(out, GA.constGraph(), &GA);
}


bool GraphIO::writeDOT(const ClusterGraph &C, std::ostream &out)
{
	const Graph &G = C.constGraph();
	int id = 1;

	// Assign a list of edges for each cluster. Perhaps usage of std::vector
	// here needs reconsideration - vector is fast but usage of STL iterators
	// is ugly without C++11 for-each loop.
	ClusterArray< std::vector<edge> > edgeMap(C);
	for(edge e : G.edges) {
		const node s = e->source(), t = e->target();
		edgeMap[C.commonCluster(s, t)].push_back(e);
	}

	return dot::writeCluster(out, 0, edgeMap, C, nullptr, C.rootCluster(), id);
}


bool GraphIO::writeDOT(const ClusterGraphAttributes &CA, std::ostream &out)
{
	const Graph &G = CA.constGraph();
	const ClusterGraph &C = CA.constClusterGraph();
	int id = 1;

	// Assign a list of edges for each cluster. Perhaps usage of std::vector
	// here needs reconsideration - vector is fast but usage of STL iterators
	// is ugly without C++11 for-each loop.
	ClusterArray< std::vector<edge> > edgeMap(C);
	for(edge e : G.edges) {
		const node s = e->source(), t = e->target();
		edgeMap[C.commonCluster(s, t)].push_back(e);
	}

	return dot::writeCluster(out, 0, edgeMap, C, &CA, C.rootCluster(), id);
}

}
