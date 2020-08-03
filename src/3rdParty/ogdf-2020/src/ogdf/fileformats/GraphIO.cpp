/** \file
 * \brief Implements class GraphIO which provides access to all
 *        graph read and write functionality.
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

#include <cctype>
#include <unordered_map>
#include <ogdf/basic/Logger.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/fileformats/GmlParser.h>
#include <ogdf/fileformats/GraphMLParser.h>
#include <ogdf/fileformats/DotParser.h>
#include <ogdf/fileformats/GexfParser.h>
#include <ogdf/fileformats/GdfParser.h>
#include <ogdf/fileformats/TlpParser.h>
#include <ogdf/fileformats/DLParser.h>
#include <ogdf/fileformats/SvgPrinter.h>

// we use these data structures from the stdlib
using std::map;
using std::ifstream;
using std::ofstream;
using std::istringstream;

namespace ogdf {

char GraphIO::s_indentChar  = '\t';
int  GraphIO::s_indentWidth = 1;
Logger GraphIO::logger;

std::ostream &GraphIO::indent(std::ostream &os, int depth)
{
	int n = s_indentWidth * depth;
	for( ; n > 0; --n)
		os.put(s_indentChar);

	return os;
}

bool GraphIO::read(Graph &G, std::istream &is)
{
	//! Supported formats for automated detection
	static const std::vector<GraphIO::ReaderFunc> readers = {
		GraphIO::readDOT,
		GraphIO::readGML,
		GraphIO::readTLP,
		GraphIO::readLEDA,
		GraphIO::readChaco,
		GraphIO::readDL,
		GraphIO::readGDF,
		GraphIO::readGraphML,
		GraphIO::readGEXF,
		GraphIO::readSTP,
		GraphIO::readGraph6WithForcedHeader,
		GraphIO::readDigraph6WithForcedHeader,
		GraphIO::readSparse6WithForcedHeader,
		GraphIO::readDMF,
		GraphIO::readPMDissGraph,
		GraphIO::readRudy,
#if 0
		// The following graph formats let the generic reader tests fail:
		GraphIO::readRome,
		GraphIO::readYGraph,
		GraphIO::readMatrixMarket,

		// The following graph formats have no corresponding ReaderFunc:
		GraphIO::readBENCH,
		GraphIO::readPLA,
		GraphIO::readChallengeGraph,
#endif
	};

	for(auto &reader : readers) {
		if(reader(G, is)) {
			return true;
		} else {
			G.clear();
			is.clear();
			is.seekg(0, std::ios::beg);
		}
	}

	return false;
}

bool GraphIO::read(GraphAttributes &GA, Graph &G, std::istream &is)
{
	static const std::vector<GraphIO::AttrReaderFunc> attrReaders = {
		GraphIO::readDOT,
		GraphIO::readGML,
		GraphIO::readTLP,
		GraphIO::readDL,
		GraphIO::readGDF,
		GraphIO::readGraphML,
		GraphIO::readGEXF,
		GraphIO::readSTP,
		GraphIO::readDMF,
		GraphIO::readRudy
	};

	for (auto &reader : attrReaders) {
		if (reader(GA, G, is)) {
			return true;
		} else {
			G.clear();
			is.clear();
			is.seekg(0, std::ios::beg);
		}
	}

	return false;
}

bool GraphIO::write(const Graph &G, const string &filename)
{
	static const std::unordered_map<string, GraphIO::WriterFunc> writerByExtension =
		{ { "gml", GraphIO::writeGML }
		, { "rome", GraphIO::writeRome } // unofficial
		, { "leda", GraphIO::writeLEDA }
		, { "gw", GraphIO::writeLEDA } // GraphWin, extension of LEDA
		, { "chaco", GraphIO::writeChaco }
		, { "pm", GraphIO::writePMDissGraph } // unofficial
		, { "pmd", GraphIO::writePMDissGraph } // unofficial
		, { "g6", GraphIO::writeGraph6 }
		, { "d6", GraphIO::writeDigraph6 }
		, { "s6", GraphIO::writeSparse6 }
		, { "graphml", GraphIO::writeGraphML }
		, { "dot", GraphIO::writeDOT }
		, { "gv", GraphIO::writeDOT } // GraphViz
		, { "gefx", GraphIO::writeGEXF }
		, { "gdf", GraphIO::writeGDF }
		, { "tlp", GraphIO::writeTLP }
		, { "dl", GraphIO::writeDL }
	};

	GraphIO::WriterFunc writer;
	string ext = filename.substr(filename.find_last_of(".") + 1);

	auto search = writerByExtension.find(ext);
	if (search != writerByExtension.end()) {
		writer = search->second;
	} else {
		// Graphs in the old Rome format have the form "grafoX.Y" where both X
		// and Y are positive integers (Y being the number of nodes).
		const string romeStart = "grafo";
		if (!ext.empty()
		 && std::all_of(ext.begin(), ext.end(), [](unsigned char c){ return std::isdigit(c); })
		 && filename.compare(0, romeStart.length(), romeStart) == 0) {
			writer = GraphIO::writeRome;
		} else {
			return false;
		}
	}

	return write(G, filename, writer);
}

bool GraphIO::readGML(Graph &G, std::istream &is)
{
	if(!is.good()) return false;
	gml::Parser parser(is);
	return parser.read(G);
}

bool GraphIO::readRome(Graph &G, std::istream &is)
{
	if(!is.good()) return false;

	G.clear();  // start with empty graph

	bool readNodes = true;
	map<int,node> indexToNode;

	string buffer;
	istringstream iss;
	while(std::getline(is, buffer))
	{
		if(buffer.size() == 0)
			continue;

		iss.str(buffer);
		iss.clear();

		if(readNodes) {
			if(buffer[0] == '#') {
				readNodes = false;
				continue;
			}

			int index = -1;
			iss >> index;
			if(index < 1 || indexToNode.find(index) != indexToNode.end()) {
				Logger::slout() << "GraphIO::readRome: Illegal node index!\n";
				return false;
			}

			indexToNode[index] = G.newNode();

		} else {

			int index, dummy, srcIndex = -1, tgtIndex = -1;
			iss >> index >> dummy >> srcIndex >> tgtIndex;

			map<int,node>::const_iterator itSrc = indexToNode.find(srcIndex);
			map<int,node>::const_iterator itTgt = indexToNode.find(tgtIndex);

			if(itSrc == indexToNode.end() || itTgt == indexToNode.end()) {
				Logger::slout() << "GraphIO::readRome: Illegal node index in edge specification.\n";
				return false;
			}

			G.newEdge(itSrc->second, itTgt->second);
		}
	}
	return true;
}

bool GraphIO::writeRome(const Graph &G, std::ostream &os)
{
	if(!os.good()) return false;

	// assign indices 1, 2, 3, ... to nodes
	NodeArray<int> index(G);

	int i = 0;
	for(node v : G.nodes) {
		index[v] = ++i;
		// write node v
		os << i << " " << "0\n";
	}

	os << "#\n"; // write node-edge separator

	i = 0;
	for(edge e : G.edges) {
		// write edge e
		os << ++i << " 0 " << index[e->source()] << " " << index[e->target()] << "\n";
	}

	return true;
}

bool GraphIO::readChaco(Graph &G, std::istream &is)
{
	if (!is.good()) {
		return false;
	}

	G.clear();
	string buffer;
	istringstream iss;
	auto log = [&](std::string&& s) {
		Logger::slout() << "GraphIO::readChaco: " << s << "\n";
	};

	int numN = -1;
	int numE = -1;
	int numWeightInfo = -1;
	bool nodesNumbered = false;
	bool nodesWeighted = false;
	bool edgesWeighted = false;

	while (std::getline(is, buffer) && (buffer[0] == '%' || buffer[0] == '#')) {
		// Ignore comments: leading lines starting with '%' or '#'.
	}

	// Get number of nodes and edges in the first uncommented line.
	iss.str(buffer);
	if (!(iss >> numN) || numN < 0) {
		log("Number of nodes is not a non-negative integer.");
		return false;
	}
	if (!(iss >> numE) || numE < 0) {
		log("Number of edges is not a non-negative integer.");
		return false;
	}

	// If optional weight info (three digit number) is set, read it.
	if (!iss.eof()) {
		if (!(iss >> numWeightInfo) || numWeightInfo < 0 || numWeightInfo > 999) {
			log("Weight info number is not an integer in {0,...,999}.");
			return false;
		}
		nodesNumbered = numWeightInfo > 99;
		nodesWeighted = numWeightInfo % 100 > 9;
		edgesWeighted = numWeightInfo % 10 > 0;
	}

	// If n = 0: Return true only if m == 0 and there are no more lines.
	if (numN == 0) {
		if (numE > 0) {
			log("No nodes but a positive amount of edges specified.");
		}
		if (std::getline(is, buffer)) {
			log("Number of nodes is 0 but adjacency lists found.");
		}
		return true;
	}

	Array<node> indexToNode(1,numN,nullptr);
	for (int i = 1; i <= numN; i++) {
		indexToNode[i] = G.newNode();
	}

	int vid = 0;
	while (std::getline(is, buffer)) {
		if (vid >= numN) {
			log("More lines with adjacency lists than expected.");
			return false;
		}

		iss.str(buffer);
		iss.clear();

		// Get correct node using its id.
		node v;
		if (nodesNumbered) {
			int nodeId;
			if (!(iss >> nodeId)) {
				log("Invalid node index.");
				return false;
			}
			if (nodeId == vid) {
				v = indexToNode[vid];
			} else if (nodeId == vid+1) {
				v = indexToNode[++vid];
			} else {
				log("Invalid order of adjacency lists.");
				if (nodeId >= 0 && nodeId <= numN) {
					vid = nodeId;
					v = indexToNode[vid];
				} else {
					return false;
				}
			}
		} else {
			v = indexToNode[++vid];
		}

		// Get node weight.
		if (nodesWeighted) {
			int nodeWeight;
			if (!(iss >> nodeWeight)) {
				log("Illegal node weight in adjacency list.");
			}
		}

		// Blank lines represent nodes without adjacency lists.
		if (buffer.empty()) {
			continue;
		}

		// Read adjacent nodes (alternating with edge weights if specified).
		bool readId = false;
		while (!iss.eof()) {
			readId = edgesWeighted ? !readId : true;
			if (readId) {
				int neighbourId;
				if (!(iss >> neighbourId) || neighbourId < 1 || neighbourId > numN) {
					log("Illegal node index in adjacency list.");
					return false;
				}
				if (neighbourId >= vid) {
					G.newEdge(v, indexToNode[neighbourId]);
				}
			} else {
				double edgeWeight;
				if (!(iss >> edgeWeight)) {
					log("Illegal edge weight in adjacency list.");
				}
			}
		}

		// Iff edge weights are given, the line should end with one.
		if (edgesWeighted == readId) {
			log("Invalid number of entries in adjacency list.");
		}
	}

	if (vid != numN) {
		log("Invalid number of lines with adjacency lists: " +
			to_string(vid) + " but expected " + to_string(numN));
	}
	if (G.numberOfEdges() != numE) {
		log("Invalid number of edges: " + to_string(G.numberOfEdges()) +
			" but expected " + to_string(numE));
	}

	return true;
}

bool GraphIO::writeChaco(const Graph &G, std::ostream &os)
{
	if(!os.good()) return false;

	os << G.numberOfNodes() << " " << G.numberOfEdges() << "\n";

	NodeArray<int> index(G);

	int count = 0;
	for(node v : G.nodes)
		index[v] = ++count;

	for(node v : G.nodes) {
		for(adjEntry adj : v->adjEntries) {
			// Write each self-loop only once.
			if (!adj->theEdge()->isSelfLoop() || adj->isSource()) {
				os << " " << index[adj->twinNode()];
			}
		}
		os << "\n";
	}

	return true;
}

bool GraphIO::readYGraph(Graph &G, std::istream &is)
{
	if(!is.good()) return false;

	const char *errorLineTooShort = "GraphIO::readYGraph: line too short!\n";

	G.clear();

	if(!is) {
		Logger::slout() << errorLineTooShort;
		return false;
	}

	int n = is.get();
	if(!is.good() || n == '\n' || n < 0) {
		Logger::slout() << errorLineTooShort;
		return false;
	}
	n &= 0x3F;

	Array<node> indexToNode(n);
	for(int i = n; i-- > 0; )
		indexToNode[i] = G.newNode();

	int s = 0, c;
	for(int i = 1; i < n; ++i)
	{
		for(int j = 0; j < i; ++j) {
			if(!s) {
				c = is.get();
				if(!is.good() || c == '\n') {
					Logger::slout() << errorLineTooShort;
					return false;
				}
				c &= 0x3F;

				s = 5;
			} else --s;
			if(c & (1 << s))
				G.newEdge(indexToNode[i], indexToNode[j]);
		}
	}

	c = is.get();
	if(!is.eof() && c != '\n') {
		Logger::slout(Logger::Level::Minor) << "GraphIO::readYGraph: Warning: line too long! ignoring...";
	}

	return true;
}

bool GraphIO::readPMDissGraph(Graph &G, std::istream &is)
{
	if(!is.good()) return false;

	const char *errorInFileHeader = "GraphIO::readPMDissGraph: Error in file header.\n";

	G.clear();

	string buffer;
	istringstream iss;

	int numN = -1, numE = -1;

	// first two lines look as follows (example with 20 nodes, 30 edges):
	// *BEGIN unknown_comp.20.30
	// *GRAPH 20 30 UNDIRECTED UNWEIGHTED

	if (std::getline(is, buffer))
	{
		iss.str(buffer); iss.clear();

		string str;
		iss >> str;
		if(str != "*BEGIN") {
			Logger::slout() << "GraphIO::readPMDissGraph: Error in file header, could not find \"*BEGIN\".\n";
			return false;
		}

		if (std::getline(is, buffer)) {
			iss.str(buffer); iss.clear();

			iss >> str >> numN >> numE;

			if(str != "*GRAPH" || numN < 0 || numE < 0) {
				Logger::slout() << errorInFileHeader;
				return false;
			}
		}
		else {
			Logger::slout() << errorInFileHeader;
			return false;
		}
	}
	else {
		Logger::slout() << errorInFileHeader;
		return false;
	}

	if (numN == 0)
		return true;

	Array<node> indexToNode(1,numN,nullptr);
	for (int i = 1; i <= numN; i++)
	{
		indexToNode[i] = G.newNode();
	}

	while(std::getline(is, buffer))
	{
		if(buffer.empty())
			continue;

		if(buffer[0] == '*')
			continue;

		iss.str(buffer); iss.clear();

		int srcIndex = -1, tgtIndex = -1;
		iss >> srcIndex >> tgtIndex;

		if(srcIndex < 1 || srcIndex > numN || tgtIndex < 1 || tgtIndex > numN) {
			Logger::slout() << "GraphIO::readPMDissGraph: Illegal node index in edge specification.\n";
			return false;
		}

		G.newEdge(indexToNode[srcIndex], indexToNode[tgtIndex]);
	}
	return true;
}

bool GraphIO::writePMDissGraph(const Graph &G, std::ostream &os)
{
	if(!os.good()) return false;

	os << "*BEGIN unknown_name." << G.numberOfNodes() << "." << G.numberOfEdges() << "\n";
	os << "*GRAPH " << G.numberOfNodes() << " " << G.numberOfEdges() << " UNDIRECTED UNWEIGHTED\n";

	NodeArray<int> index(G);
	int nextIndex = 1;
	for(node v : G.nodes)
		index[v] = nextIndex++;

	for(edge e : G.edges)
		os << index[e->source()] << " " << index[e->target()] << "\n";

	os << "*CHECKSUM -1\n";
	os << "*END unknown_name." << G.numberOfNodes() << "." << G.numberOfEdges() << "\n";

	return true;
}

bool GraphIO::readGML(ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) return false;
	gml::Parser gml(is);
	return gml.read(G) && gml.readCluster(G, C);
}

bool GraphIO::readGML(GraphAttributes &A, Graph &G, std::istream &is)
{
	if (!is.good()) return false;
	gml::Parser parser(is);
	return parser.read(G, A);
}

bool GraphIO::readRudy(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) return false;

	G.clear();

	int n = -1;
	int m = -1;

	if (!(is >> n) || n < 0) {
		Logger::slout() << "GraphIO::readRudy: Number of nodes is not a non-negative integer.";
		return false;
	}
	if (!(is >> m) || m < 0) {
		Logger::slout() << "GraphIO::readRudy: Number of edges is not a non-negative integer.";
		return false;
	}

	Array<node> mapToNode(0, n-1, nullptr);
	for(int i = 0; i < n; ++i) {
		mapToNode[i] = G.newNode();
	}

	bool haveDoubleWeight = A.has(GraphAttributes::edgeDoubleWeight);

	for(int i = 0; i < m; i++) {
		int src = 0, tgt = 0;
		double weight = 1.0;

		is >> src >> tgt >> weight;
		if(src < 1 || src > n || tgt < 1 || tgt > n) {
			Logger::slout() << "GraphIO::readRudy: Illegal node index!\n";
			return false;
		}

		src--; tgt--;

		edge e = G.newEdge(mapToNode[src],mapToNode[tgt]);
		if (haveDoubleWeight) {
			A.doubleWeight(e) = weight;
		}
	}

	return true;
}

bool GraphIO::writeRudy(const GraphAttributes &A, std::ostream &os)
{
	if(!os.good()) return false;

	const Graph &G = A.constGraph();
	os << G.numberOfNodes() << " " << G.numberOfEdges() << std::endl;

	// assign indices 1, 2, 3, ... to nodes
	NodeArray<int> index(G);

	int i = 0;
	for(node v : G.nodes)
		index[v] = ++i;

	bool haveDoubleWeight = A.has(GraphAttributes::edgeDoubleWeight) != 0;

	for(edge e : G.edges) {
		double w = (haveDoubleWeight) ? A.doubleWeight(e) : 1.0;
		os << index[e->source()] << " " << index[e->target()] << " " << w << "\n";
	}

	return true;
}

bool GraphIO::readGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) return false;
	gml::Parser gml(is);
	return gml.read(G, A) && gml.readCluster(G, C, &A);
}

bool GraphIO::readMatrixMarket(Graph& G, std::istream &inStream)
{
	// check if the stream is good
	if (!inStream.good())
		return false;

	// clear the graph
	G.clear();

	// flag for reading the first triplet because it is special
	bool isFirstEntry = true;

	// simple map to map from indices to nodes
	std::map<int, node> idToNodeMap;

	// while there is somethng to read
	while (!inStream.eof())
	{
		// the current line
		std::string line;

		// read one line at a time
		getline(inStream, line);

		// skip empty lines
		if (line.empty())
			continue;

		// skip comments
		if (line.at(0) == '%')
			continue;

		// stringstream for the single line
		std::stringstream fin(line);

		// if this is the real line (triplet
		if (isFirstEntry)
		{
			// read the number of rows, columns and non zero entries
			int numRows, numCols, numNonZero;

			// read triplet
			fin >> numRows;
			fin >> numCols;
			fin >> numNonZero;

			// set flag that we parsed that line
			isFirstEntry = false;
		} else
		{
			// the usual triplet ( row and column indices and the corresponding weight
			int rowIndex, colIndex;
			double weight;

			// read the triplet
			fin >> rowIndex;
			fin >> colIndex;
			fin >> weight;

			// node corresponding to the row
			node s = nullptr;

			// and to the column
			node t = nullptr;

			// look up the nodes in the map
			std::map<int, node>::const_iterator s_it = idToNodeMap.find(rowIndex);
			std::map<int, node>::const_iterator t_it = idToNodeMap.find(colIndex);

			// check if we already created the node
			if (s_it == idToNodeMap.end())
			{
				// no we did not, create it
				s = G.newNode();
				// and put it in the map
				idToNodeMap[rowIndex] = s;
			} else {
				// we did, get it
				s = (*s_it).second;
			}

			// check if we already created the node
			if (t_it == idToNodeMap.end())
			{
				// no we did not, create it
				t = G.newNode();
				// and put it in the map
				idToNodeMap[colIndex] = t;
			} else {
				 // we did, get it
				t = (*t_it).second;
			}

			// both nodes are there now, create the edge
			G.newEdge(s, t);
		}
	}

	makeParallelFree(G);
	return true;
}

bool GraphIO::readChallengeGraph(Graph &G, GridLayout &gl, std::istream &is)
{
	if(!is.good()) return false;

	G.clear();

	string buffer;
	istringstream iss;

	int n = -1;
	do {
		if(is.eof()) return false;
		std::getline(is, buffer);
		if(!buffer.empty() && buffer[0] != '#') {
			iss.str(buffer); iss.clear();
			iss >> n;
			if(n < 0) return false;
		}
	} while(n < 0);

	Array<node> indexToNode(n);
	for(int i = 0; i < n; ) {
		if(is.eof()) return false;
		std::getline(is, buffer);

		if(!buffer.empty() && buffer[0] != '#') {
			node v = G.newNode();
			iss.str(buffer); iss.clear();
			iss >> gl.x(v) >> gl.y(v);
			indexToNode[i++] = v;
		}
	}

	while(!is.eof()) {
		std::getline(is, buffer);

		if(!buffer.empty() && buffer[0] != '#') {
			iss.str(buffer); iss.clear();
			int srcIndex, tgtIndex;

			if(iss.eof()) return false;
			iss >> srcIndex;
			if(srcIndex < 0 || srcIndex >= n) return false;

			if(iss.eof()) return false;
			iss >> tgtIndex;
			if(tgtIndex < 0 || tgtIndex >= n) return false;

			node src = indexToNode[srcIndex];
			node tgt = indexToNode[tgtIndex];
			edge e = G.newEdge(src,tgt);

			string symbol;
			if(iss.eof()) return false;
			iss >> symbol;
			if(symbol != "[") return false;

			IPolyline &ipl = gl.bends(e);
			for(;;) {
				if(iss.eof()) return false;
				iss >> symbol;
				if(symbol == "]") break;

				IPoint ip;
				ip.m_x = atoi(symbol.c_str());
				if(iss.eof()) return false;
				iss >> ip.m_y;
				ipl.pushBack(ip);
			}
		}
	}

	return true;
}

bool GraphIO::writeChallengeGraph(const Graph &G, const GridLayout &gl, std::ostream &os)
{
	if(!os.good()) return false;

	os << "# Number of Nodes\n";
	os << G.numberOfNodes() << "\n";

	os << "# Nodes\n";
	NodeArray<int> index(G);
	int i = 0;
	for(node v : G.nodes) {
		os << gl.x(v) << " " << gl.y(v) << "\n";
		index[v] = i++;
	}

	os << "# Edges\n";
	for(edge e : G.edges) {
		os << index[e->source()] << " " << index[e->target()] << " [";
		const IPolyline &ipl = gl.bends(e);
		for(const IPoint &ip : ipl)
			os << " " << ip.m_x << " " << ip.m_y;
		os << " ]\n";
	}

	return true;
}

bool GraphIO::readSTP(GraphAttributes &attr, Graph &graph,
		List<node> &terminals,
		NodeArray<bool> &isTerminal,
		std::istream &is)
{
	attr.addAttributes(GraphAttributes::nodeGraphics);
	attr.directed() = false;
	const long attrs = attr.attributes();
	const bool iWeight = (attrs & GraphAttributes::edgeIntWeight) != 0;
	const bool dWeight = (attrs & GraphAttributes::edgeDoubleWeight) != 0;

	graph.clear();
	terminals.clear();
	isTerminal.init(graph, false);
	int expectedNumberOfTerminals = -1;
	int expectedNumberOfEdges = -1;
	int expectedCoordinateDimension = -1;

	int encounteredNumberOfCoordinates = 0;

	string buffer;

	enum class Section {
		None,
		Comment,
		Graph,
		Terminals,
		Coordinates,
		Ignore, // mostly: not implemented
	} section = Section::None;

	string key, value;

	Array<node> indexToNode;
	node root = nullptr; // root terminal (directed case)

	double version;

	// 1. line = identifier; ignore that it must be all on the first line
	std::vector<string> firstline{"33D32945", "STP", "File,", "STP", "Format", "Version"};

	is >> buffer;
	if (is.good() && !equalIgnoreCase(buffer, firstline[0])) {
		logger.lout() << "No STP Header found, assuming simplified STP format" << std::endl;
		is.clear();
		is.seekg(0, std::ios::beg);
	} else {
		for (int i = 1; i < 6; ++i) {
			is >> buffer;
			if (!is.good() || !equalIgnoreCase(buffer, firstline[i])) {
				logger.lout() << "Faulty header" << std::endl;
				return false;
			}
		}
		is >> version;
		if (!is.good() || version != 1.0) {
			logger.lout() << "Encountered unknown STP format version." << std::endl;
			return false;
		}
	}

	while (std::getline(is, buffer)) {
		removeTrailingWhitespace(buffer);

		if (buffer.empty() || buffer[0] == '#') {
			continue;
		}

		std::istringstream iss(buffer);
		iss >> key;
		if (section != Section::None && equalIgnoreCase(key, "END")) {
			section = Section::None;
			continue;
		}
		switch (section) {
		case Section::None:
			if (equalIgnoreCase(key, "SECTION")) {
				string what;
				iss >> what;
				if (equalIgnoreCase(what, "Comment")) {
					section = Section::Comment;
				} else if (equalIgnoreCase(what, "Graph")) {
					if (graph.numberOfNodes() != 0) {
						logger.lout(Logger::Level::Minor) << "Encountered duplicate graph section.";
						section = Section::Ignore;
					} else {
						section = Section::Graph;
					}
				} else if (equalIgnoreCase(what, "Terminals")) {
					if (!terminals.empty()) {
						logger.lout(Logger::Level::Minor) << "Encountered duplicate terminal section.";
						section = Section::Ignore;
					} else {
						section = Section::Terminals;
					}
				} else if (equalIgnoreCase(what, "Coordinates")) {
					if (encounteredNumberOfCoordinates != 0) {
						logger.lout(Logger::Level::Minor) << "Encountered duplicate coordinate section.";
						section = Section::Ignore;
					} else {
						section = Section::Coordinates;
					}
				} else {
					logger.lout(Logger::Level::Minor) << "Invalid Section encountered: " << what << std::endl;
					section = Section::Ignore;
				}

				if (!iss.eof()) {
					iss >> what;
					if (equalIgnoreCase(what, "FROM")) {
						logger.lout(Logger::Level::Minor) << "External loading not implemented" << std::endl;
						section = Section::None;
					}
				}
			} else if (equalIgnoreCase(buffer, "EOF")) {
				if(expectedNumberOfTerminals != -1 && expectedNumberOfTerminals != terminals.size()) {
					logger.lout()
						<< "Invalid number of terminals. Was " << terminals.size()
						<< " but expected " << expectedNumberOfTerminals << "." << std::endl;
				}

				if(expectedNumberOfEdges != -1 && expectedNumberOfEdges != graph.numberOfEdges()) {
					logger.lout()
						<<  "Invalid number of edges. Was " << graph.numberOfEdges()
						<< " but expected " << expectedNumberOfEdges << "." << std::endl;
				}

				if (encounteredNumberOfCoordinates != 0 && graph.numberOfNodes() != encounteredNumberOfCoordinates) {
					logger.lout()
						<< "Invalid number of coordinates. Was " << encounteredNumberOfCoordinates
						<< " but expected 0 or " << graph.numberOfNodes() << "." << std::endl;
				}

				if (root == nullptr && attr.directed() && !terminals.empty()) {
					logger.lout()
						<< "Expected root specification in directed STP instance,"
						<< " but got none." << std::endl;
				}
				return true;
			}
			break;

		case Section::Ignore:
		case Section::Comment:
			// allow anything
			break;

		case Section::Graph:
			if (equalIgnoreCase(key, "Nodes")) {
				int n = -1;
				iss >> n;
				if (n < 0) {
					logger.lout() << "Invalid number of nodes specified: " << n << std::endl;
					return false;
				}

				indexToNode = Array<node>(1, n, nullptr);
				for (int i = 1; i <= n; ++i) {
					indexToNode[i] = graph.newNode();
					attr.shape(indexToNode[i]) = Shape::Ellipse;
				}
			} else if(equalIgnoreCase(key, "Edges") || equalIgnoreCase(key, "Arcs")) {
				if (expectedNumberOfEdges == -1) {
					iss >> expectedNumberOfEdges;
					if (expectedNumberOfEdges < 0) {
						logger.lout() << "Invalid number of edges specified: " << expectedNumberOfEdges << std::endl;
						return false;
					}
					attr.directed() = equalIgnoreCase(key, "Arcs");
				} else {
					logger.lout() << "Repeated or mixed specification of edge/arc count" << std::endl;
				}
			} else if (equalIgnoreCase(key, "E") || equalIgnoreCase(key, "A")) {
				if ((equalIgnoreCase(key, "E") && attr.directed())
					|| (equalIgnoreCase(key, "A") && !attr.directed())) {
					logger.lout() << "Edge key " + key + " does not match the edge type specified beforehand" << std::endl;
				}
				int source = -1, target = -1;
				double weight = -1.0;
				iss >> source >> target >> weight;
				if (source <= 0 || source > graph.numberOfNodes()
					|| target <= 0 || target > graph.numberOfNodes()
					|| weight < 0) {
					logger.lout() << "Invalid edge given: " << source << "->" << target << "(weight: " << weight << ")" << std::endl;
					return false;
				}
				edge newE = graph.newEdge(indexToNode[source], indexToNode[target]);
				if (iWeight) {
					attr.intWeight(newE) = static_cast<int>(weight);
				}
				if (dWeight) {
					attr.doubleWeight(newE) = weight;
				}
			} else {
				logger.lout(Logger::Level::Minor) << "Invalid edge key encountered: " << key << std::endl;
			}
			break;

		case Section::Terminals:
			if (equalIgnoreCase(key, "Terminals")) {
				iss >> expectedNumberOfTerminals;
			} else if (equalIgnoreCase(key, "T")) {
				int v = -1;
				iss >> v;
				if (v <= 0 || v > graph.numberOfNodes()) {
					logger.lout() << "Invalid terminal encountered: " << v << std::endl;
					return false;
				}
				terminals.pushBack(indexToNode[v]);
				isTerminal[indexToNode[v]] = true;
				attr.shape(indexToNode[v]) = Shape::Rect;
			} else if (equalIgnoreCase(key, "Root") && root == nullptr) {
				int v = -1;
				iss >> v;
				if (v <= 0 || v > graph.numberOfNodes()) {
					logger.lout() << "Invalid root terminal encountered: " << v << std::endl;
					return false;
				}
				root = indexToNode[v];
				terminals.pushFront(root);
				isTerminal[root] = true;
				attr.shape(root) = Shape::Triangle;
			} else {
				logger.lout(Logger::Level::Minor) << "Invalid Terminal key encountered: " << key << std::endl;
			}
			break;

		case Section::Coordinates:
			if (equalIgnoreCase(key, "D") || equalIgnoreCase(key, "DD") || equalIgnoreCase(key, "DDD")) {
				if (expectedCoordinateDimension == -1) {
					expectedCoordinateDimension = key.length();
					if (expectedCoordinateDimension == 3) {
						attr.addAttributes(GraphAttributes::threeD);
					}
				} else if (static_cast<size_t>(expectedCoordinateDimension) != key.length()) {
					logger.lout(Logger::Level::Minor) << "Non-uniform coordinate dimensions encountered" << std::endl;
					return false;
				}
				long v = -1;
				iss >> v;
				if (v <= 0 || v > graph.numberOfNodes()) {
					logger.lout() << "Invalid node (coordinate) encountered: " << v << std::endl;
					return false;
				}
				if (expectedCoordinateDimension >= 1) {
					double x = 0;
					iss >> x;
					attr.x(indexToNode[v]) = x;
				}
				if (expectedCoordinateDimension >= 2) {
					double y = 0;
					iss >> y;
					attr.y(indexToNode[v]) = y;
				}
				if (expectedCoordinateDimension == 3) {
					double z = 0;
					iss >> z;
					attr.z(indexToNode[v]) = z;
				}
				encounteredNumberOfCoordinates++;
			} else {
				logger.lout(Logger::Level::Minor) << "Invalid Coordinate key encountered: " << key << std::endl;
			}
			break;
		}
	}
	logger.lout() << "Unexpected end of file." << std::endl;
	return false;
}

bool GraphIO::writeSTP(const GraphAttributes &attr, const List<node> &terminals, std::ostream &os, const string &comments)
{
	if(!os.good()) return false;
	const long attrs = attr.attributes();
	const bool iWeight = (attrs & GraphAttributes::edgeIntWeight) != 0;
	const bool dWeight = (attrs & GraphAttributes::edgeDoubleWeight) != 0;
	const bool xyCoord = (attrs & GraphAttributes::nodeGraphics) != 0;
	const bool zCoord = (attrs & GraphAttributes::threeD) != 0;
	const Graph &graph = attr.constGraph();

	string edgeName, edgeKey;
	node root = nullptr;
	if (attr.directed()) {
		edgeName = "Arcs";
		edgeKey = "A";
		if (!terminals.empty()) {
			root = terminals.front();
		}
	} else {
		edgeName ="Edges";
		edgeKey = "E";
	}

	os << "33d32945 STP File, STP Format Version  1.00" << std::endl;

	os << std::endl << "Section Comment" << std::endl;
	if (comments.length() != 0) {
		os << comments << std::endl;
	}
	os << "End" << std::endl;

	os << std::endl << "Section Graph" << std::endl;
	os << "Nodes " << graph.numberOfNodes() << std::endl;
	os << edgeName << " " << graph.numberOfEdges() << std::endl;

	NodeArray<int> nodeToIndex(graph);
	int i = 1;
	for (node v : graph.nodes) {
		nodeToIndex[v] = i++;
	}
	for (edge e : graph.edges) {
		os << edgeKey << " " << nodeToIndex[e->source()]
			<< " " << nodeToIndex[e->target()];
		if (dWeight) {
			os << " " << attr.doubleWeight(e) << std::endl;
		} else if (iWeight) {
			os << " " << attr.intWeight(e) << std::endl;
		}
	}
	os << "End" << std::endl
		<< std::endl
		<< "Section Terminals" << std::endl
		<< "Terminals " << terminals.size() << std::endl;
	for (node v : terminals) {
		if (root != nullptr && v == root) {
			os << "Root " << nodeToIndex[root] << std::endl;
		} else {
			os << "T " << nodeToIndex[v] << std::endl;
		}
	}
	os << "End" << std::endl
		<< std::endl;
	if (xyCoord) {
		os << "Section Coordinates" << std::endl;
		for (node v : graph.nodes) {
			if (zCoord) {
				os << "DDD " << nodeToIndex[v] << " "
					<< attr.x(v) << " "
					<< attr.y(v) << " "
					<< attr.z(v) << " "
					<< std::endl;
			} else {
				os << "DD " <<  nodeToIndex[v] << " "
					<< attr.x(v) << " "
					<< attr.y(v) << " "
					<< std::endl;
			}
		}
		os << "End" << std::endl
			<< std::endl;
	}
	os << "EOF" << std::endl;

	return true;
}

bool GraphIO::readDMF(GraphAttributes &attr, Graph &graph, node &source, node &sink, std::istream &is)
{
	const long attrs = attr.attributes();
	const bool iWeight = (attrs & GraphAttributes::edgeIntWeight) != 0;
	const bool dWeight = (attrs & GraphAttributes::edgeDoubleWeight) != 0;

	int expectedNumberOfEdges = -1;
	List<node> nodes;
	graph.clear();
	source = nullptr;
	sink = nullptr;

	string buffer;

	while (std::getline(is, buffer)) {
		removeTrailingWhitespace(buffer);
		std::istringstream iss(buffer);
		string tmp;
		iss >> tmp;

		if (!buffer.empty() && buffer[0] != 'c') {
			if(buffer[0] == 'p') {
				// problem definition section
				if(!graph.empty()) {
					logger.lout() << "Ambiguous problem definition encountered." << std::endl;
					return false;
				}

				string problemType = "";
				iss >> problemType;
				if(problemType.compare("max")) {
					logger.lout() << "Invalid problem type encountered: " << problemType << std::endl;
					return false;
				}

				int numberOfNodes = -1;
				iss >> numberOfNodes >> expectedNumberOfEdges;

				if(numberOfNodes < 2) {
					logger.lout() << "The given number of nodes is invalid (at least two)." << std::endl;
					return false;
				}

				if(expectedNumberOfEdges < 0) {
					logger.lout() << "The given number of edges is invalid." << std::endl;
					return false;
				}

				for(int i = 0; i < numberOfNodes; i++) {
					graph.newNode();
				}
				graph.allNodes(nodes);
			} else if(buffer[0] == 'n') {
				// target or source definition
				int nodeIndex = -1;
				string nodeTypeString = "";
				iss >> nodeIndex >> nodeTypeString;

				if (nodeIndex < 1 || nodeIndex > nodes.size()) {
					logger.lout() << "Invalid node index supplied: " << nodeIndex << std::endl;
					return false;
				}

				node w = *nodes.get(nodeIndex - 1);
				if (!nodeTypeString.compare("t")) {
					if(sink != nullptr) {
						logger.lout() << "Duplicate sink encountered: " << nodeTypeString << std::endl;
						return false;
					}
					sink = w;
				} else if (!nodeTypeString.compare("s")) {
					if(source != nullptr) {
						logger.lout() << "Duplicate source encountered: " << nodeTypeString << std::endl;
						return false;
					}
					source = w;
				} else {
					logger.lout() << "Malformed node type encountered: " << nodeTypeString << std::endl;
					return false;
				}

			} else if (buffer[0] == 'a') {
				// edge definition
				int sourceIndex = -1;
				int targetIndex = -1;
				double cap = -1.0;

				iss >> sourceIndex >> targetIndex >> cap;

				if (sourceIndex < 1 || sourceIndex > nodes.size()) {
					logger.lout() << "Invalid node index supplied: " << sourceIndex << std::endl;
					return false;
				}
				node newSource = *nodes.get(sourceIndex - 1);

				if (targetIndex < 1 || targetIndex > nodes.size()) {
					logger.lout() << "Invalid node index supplied: " << targetIndex << std::endl;
					return false;
				}
				node newTarget = *nodes.get(targetIndex - 1);

				if(cap < 0) {
					logger.lout() << "Negative capacity supplied: " << targetIndex << std::endl;
					return false;
				}

				edge e = graph.newEdge(newSource, newTarget);
				if (iWeight) {
					attr.intWeight(e) = static_cast<int>(cap);
				}
				if (dWeight) {
					attr.doubleWeight(e) = cap;
				}
			} else {
				logger.lout() << "Encountered invalid line: " << buffer << std::endl;
				return false;
			}
		}
	}

	if (graph.empty()) {
		logger.lout() << "Missing problem definition." << std::endl;
		return false;
	}

	if (source == nullptr) {
		logger.lout() << "Missing source node." << std::endl;
		return false;
	}

	if(sink == nullptr) {
		logger.lout() << "Missing sink node." << std::endl;
		return false;
	}

	if(sink == source) {
		logger.lout() << "Source must be different from sink." << std::endl;
		return false;
	}

	if(expectedNumberOfEdges != graph.numberOfEdges()) {
		logger.lout() << "Invalid number of edges: expected " << expectedNumberOfEdges << " but was " << graph.numberOfEdges() << std::endl;
		return false;
	}

	return true;
}

bool GraphIO::writeDMF(const GraphAttributes &attr, const node source, const node sink, std::ostream &os)
{
	if(!os.good()) return false;
	const long attrs = attr.attributes();
	const bool iWeight = (attrs & GraphAttributes::edgeIntWeight) != 0;
	const bool dWeight = (attrs & GraphAttributes::edgeDoubleWeight) != 0;

	const Graph &graph = attr.constGraph();
	NodeArray<int> nodeIndices(graph);

	int counter = 0;
	for(node v : graph.nodes) {
		nodeIndices[v] = ++counter;
	}

	os << "p max " << graph.numberOfNodes() << " "  << graph.numberOfEdges() << std::endl;
	os << "n " << nodeIndices[source] << " s" << std::endl;
	os << "n " << nodeIndices[sink] << " t" << std::endl;

	for(edge e : graph.edges) {
		os << "a " << nodeIndices[e->source()] << " " << nodeIndices[e->target()] << " ";
		if (dWeight) {
			os << attr.doubleWeight(e);
		} else if (iWeight) {
			os << attr.intWeight(e);
		}
		os << std::endl;
	}

	return true;
}

bool GraphIO::readEdgeListSubgraph(Graph &G, List<edge> &delEdges, std::istream &is)
{
	if(!is.good()) return false;

	G.clear();
	delEdges.clear();

	string buffer;

	if(is.eof()) return false;
	std::getline(is, buffer);
	istringstream iss(buffer);

	int n = 0, m = 0, m_del = 0;
	iss >> n >> m >> m_del;

	if(n < 0 || m < 0 || m_del < 0)
		return false;

	Array<node> indexToNode(n);
	for(int i = 0; i < n; ++i)
		indexToNode[i] = G.newNode();

	int m_all = m + m_del;
	for(int i = 0; i < m_all; ++i) {
		if(is.eof()) return false;

		std::getline(is, buffer);
		iss.str(buffer);
		iss.clear();

		int src = -1, tgt = -1;
		iss >> src >> tgt;
		if(src < 0 || src >= n || tgt < 0 || tgt >= n)
			return false;

		edge e = G.newEdge(indexToNode[src], indexToNode[tgt]);

		if(i >= m)
			delEdges.pushBack(e);
	}

	return true;
}

bool GraphIO::writeEdgeListSubgraph(const Graph &G, const List<edge> &delEdges, std::ostream &os)
{
	if(!os.good()) return false;

	const int m_del = delEdges.size();
	const int n = G.numberOfNodes();
	const int m = G.numberOfEdges() - m_del;

	os << n << " " << m << " " << m_del << "\n";

	EdgeArray<bool> markSub(G,true);
	for(edge e : delEdges)
		markSub[e] = false;

	NodeArray<int> index(G);
	int i = 0;
	for(node v : G.nodes)
		index[v] = i++;

	for(edge e : G.edges)
		if(markSub[e])
			os << index[e->source()] << " " << index[e->target()] << "\n";

	for(edge e : delEdges)
		os << index[e->source()] << " " << index[e->target()] << "\n";

	return true;
}

bool GraphIO::drawSVG(const GraphAttributes &attr, std::ostream &os, const SVGSettings &settings)
{
	SvgPrinter printer(attr, settings);
	return printer.draw(os);
}

bool GraphIO::drawSVG(const ClusterGraphAttributes &attr, std::ostream &os, const SVGSettings &settings)
{
	SvgPrinter printer(attr, settings);
	return printer.draw(os);
}

bool GraphIO::readGraphML(Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	GraphMLParser parser(is);
	return parser.read(G);
}

bool GraphIO::readGraphML(ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	GraphMLParser parser(is);
	return parser.read(G, C);
}

bool GraphIO::readGraphML(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	GraphMLParser parser(is);
	return parser.read(G, A);
}

bool GraphIO::readGraphML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
{
	GraphMLParser parser(is);
	return parser.read(G, C, A);
}

bool GraphIO::readDOT(Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	dot::Parser parser(is);
	return parser.read(G);
}

bool GraphIO::readDOT(ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	dot::Parser parser(is);
	return parser.read(G, C);
}

bool GraphIO::readDOT(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	dot::Parser parser(is);
	return parser.read(G, A);
}

bool GraphIO::readDOT(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	dot::Parser parser(is);
	return parser.read(G, C, A);
}

bool GraphIO::readGEXF(Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	gexf::Parser parser(is);
	return parser.read(G);
}

bool GraphIO::readGEXF(ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	gexf::Parser parser(is);
	return parser.read(G, C);
}

bool GraphIO::readGEXF(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	gexf::Parser parser(is);
	return parser.read(G, A);
}

bool GraphIO::readGEXF(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	gexf::Parser parser(is);
	return parser.read(G, C, A);
}

bool GraphIO::readGDF(Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	gdf::Parser parser(is);
	return parser.read(G);
}

bool GraphIO::readGDF(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	gdf::Parser parser(is);
	return parser.read(G, A);
}

bool GraphIO::readTLP(Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	tlp::Parser parser(is);
	return parser.read(G);
}

bool GraphIO::readTLP(ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	tlp::Parser parser(is);
	return parser.read(G, C);
}

bool GraphIO::readTLP(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	tlp::Parser parser(is);
	return parser.read(G, A);
}

bool GraphIO::readTLP(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	tlp::Parser parser(is);
	return parser.read(G, C, A);
}

bool GraphIO::readDL(Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	DLParser parser(is);
	return parser.read(G);
}

bool GraphIO::readDL(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	DLParser parser(is);
	return parser.read(G, A);
}

}
