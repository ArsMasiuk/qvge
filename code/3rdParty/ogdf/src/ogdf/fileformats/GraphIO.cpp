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

#include <ogdf/basic/Logger.h>
#include <ogdf/basic/AdjacencyOracle.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/fileformats/GmlParser.h>
#include <ogdf/fileformats/OgmlParser.h>
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

char GraphIO::s_indentChar  = ' ';
int  GraphIO::s_indentWidth = 2;
Logger GraphIO::logger;

//! Supported formats for automated detection
const std::vector<GraphIO::ReaderFunc> readers = {
	GraphIO::readDOT,
	GraphIO::readGML,
	GraphIO::readTLP,
	GraphIO::readLEDA,
	GraphIO::readChaco,
	GraphIO::readDL,
	GraphIO::readGDF,
	GraphIO::readGraphML,
	GraphIO::readGEXF,
	GraphIO::readOGML,
	GraphIO::readSTP,
	GraphIO::readGraph6WithForcedHeader
};

std::ostream &GraphIO::indent(std::ostream &os, int depth)
{
	int n = s_indentWidth * depth;
	for( ; n > 0; --n)
		os.put(s_indentChar);

	return os;
}

bool GraphIO::read(Graph &G, std::istream &is)
{
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

bool GraphIO::readGML(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGML(G, is);
}

bool GraphIO::readGML(Graph &G, std::istream &is)
{
	if(!is.good()) return false;
	GmlParser parser(is);
	return !parser.error() && parser.read(G);
}

bool GraphIO::writeGML(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writeGML(G, os);
}

bool GraphIO::readOGML(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readOGML(G, is);
}

bool GraphIO::readOGML(Graph &G, std::istream &is)
{
	if(!is.good()) return false;
	OgmlParser parser;
	return parser.read(is, G);
}

bool GraphIO::writeOGML(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writeOGML(G, os);
}

bool GraphIO::readRome(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readRome(G, is);
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

bool GraphIO::writeRome(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writeRome(G, os);
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

bool GraphIO::readLEDA(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readLEDA(G, is);
}

bool GraphIO::writeLEDA(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writeLEDA(G, os);
}

bool GraphIO::readChaco(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readChaco(G, is);
}

bool GraphIO::readChaco(Graph &G, std::istream &is)
{
	if(!is.good()) return false;

	G.clear();

	string buffer;
	istringstream iss;

	int numN = -1, numE = -1;
	if (std::getline(is, buffer)) {
		iss.str(buffer);
		iss >> numN >> numE;
		if(numN < 0 || numE < 0)
			return false;
	} else
		return false;

	if (numN == 0) return true;

	Array<node> indexToNode(1,numN,nullptr);
	for (int i = 1; i <= numN; i++)
		indexToNode[i] = G.newNode();

	int vid = 0;
	while(std::getline(is, buffer))
	{
		if(buffer.empty())
			continue;

		if(vid >= numN) {
			Logger::slout() << "GraphIO::readChaco: More lines with adjacency lists than expected.\n";
			return false;
		}

		iss.str(buffer); iss.clear();
		node v = indexToNode[++vid];

		int wid;
		while(iss >> wid) {
			if(wid < 1 || wid > numN) {
				Logger::slout() << "GraphIO::readChaco: Illegal node index in adjacency list.\n";
				return false;
			}
			if(wid >= vid)
				G.newEdge(v, indexToNode[wid]);
		}
	}

	if(G.numberOfEdges() != numE) {
		Logger::slout() << "GraphIO::readChaco: Invalid number of edges: " << G.numberOfEdges() << " but expected " << numE << "\n";
		return false;
	}

	return true;
}

bool GraphIO::writeChaco(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writeChaco(G, os);
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
		for(adjEntry adj : v->adjEntries)
			os << " " << index[adj->twinNode()];
		os << "\n";
	}

	return true;
}

bool GraphIO::readYGraph(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readYGraph(G, is);
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

bool GraphIO::readPMDissGraph(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readPMDissGraph(G, is);
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

bool GraphIO::writePMDissGraph(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writePMDissGraph(G, os);
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

bool
GraphIO::readGraph6(Graph &G, std::istream &is, bool forceHeader)
{
	if (!is.good()) {
		return false;
	}

	G.clear();

	const int asciishift = 63;
	Array<node> index;
	int sourceIdx = 0;
	int targetIdx = 1;
	int numberOfNodes = 0;
	auto addEdge = [&](int add) {
		if (add) {
			G.newEdge(index[sourceIdx], index[targetIdx]);
		}
		++sourceIdx;
		if (sourceIdx == targetIdx) {
			sourceIdx = 0;
			++targetIdx;
		}
	};
	enum class State {
		Start,
		EighteenBit,
		RemainingBits,
		Triangle,
	} state = State::Start;
	auto addNodes = [&]() {
		index.init(numberOfNodes);
		for (int i = 0; i < numberOfNodes; ++i) {
			index[i] = G.newNode();
		}
		state = State::Triangle;
	};
	int remainingBits;
	if (forceHeader) {
		string header;
		header.resize(10);
		is.read(&header[0], 10);
		if (header != ">>graph6<<") {
			return false;
		}
	}
	for (unsigned char readbyte; is >> readbyte;) {
		int byte = readbyte;
		switch (state) {
		case State::Triangle:
			if (byte >= '?' && byte <= '~') {
				OGDF_ASSERT(numberOfNodes == G.numberOfNodes());
				OGDF_ASSERT(sourceIdx < targetIdx);
				if (targetIdx >= numberOfNodes) {
					return false;
				}
				byte -= asciishift;
				addEdge(byte & 040);
				addEdge(byte & 020);
				addEdge(byte & 010);
				addEdge(byte & 04);
				addEdge(byte & 02);
				addEdge(byte & 01);
			}
			break;
		case State::EighteenBit:
			if (byte == '~') {
				state = State::RemainingBits;
				remainingBits = 6;
			} else
			if (byte >= '?' && byte < '~') {
				numberOfNodes |= ((byte - asciishift) << 12);
				state = State::RemainingBits;
				remainingBits = 2;
			}
			break;
		case State::RemainingBits:
			if (byte >= '?' && byte <= '~') {
				--remainingBits;
				numberOfNodes |= ((byte - asciishift) << (6*remainingBits));
				if (remainingBits == 0) {
					addNodes();
				}
			}
			break;
		case State::Start:
			if (byte == '>') {
				string header;
				header.resize(9);
				is.read(&header[0], 9);
				if (header != ">graph6<<") {
					return false;
				}
			} else
			if (byte == '~') {
				state = State::EighteenBit;
			} else
			if (byte >= '?' && byte < '~') {
				numberOfNodes = byte - asciishift;
				addNodes();
			}
			// ignore others
		}
	}
	return numberOfNodes == G.numberOfNodes();
}

bool GraphIO::readGraph6WithForcedHeader(Graph &G, std::istream &is)
{
	return readGraph6(G, is, true);
}

bool GraphIO::readGraph6(Graph &G, const string &filename, bool forceHeader)
{
	ifstream is(filename);
	return readGraph6(G, is, forceHeader);
}

bool GraphIO::writeGraph6(const Graph &G, std::ostream &os)
{
	if (!os.good()) {
		return false;
	}
	const int asciishift = 63;

	os << ">>graph6<<";
	int n = G.numberOfNodes();
	auto sixtetChar = [&](int sixtet) {
		return static_cast<unsigned char>(((n >> (6*sixtet)) & asciishift) + asciishift);
	};
	if (n < 64) {
		os << sixtetChar(0);
	}
	else
	if (n < 258048) {
		os
		  << '~'
		  << sixtetChar(2)
		  << sixtetChar(1)
		  << sixtetChar(0);
	}
	else { // XXX: < 68719476736
		os
		  << "~~"
		  << sixtetChar(5)
		  << sixtetChar(4)
		  << sixtetChar(3)
		  << sixtetChar(2)
		  << sixtetChar(1)
		  << sixtetChar(0);
	}

	AdjacencyOracle oracle(G);
	int shift = 6;
	int sixtet = 0;
	for (node v : G.nodes) {
		for (node w : G.nodes) {
			if (v == w) {
				break;
			}
			int bit = oracle.adjacent(v, w);
			--shift;
			sixtet |= bit << shift;
			if (shift == 0) {
				os << static_cast<unsigned char>(sixtet + asciishift);
				shift = 6;
				sixtet = 0;
			}
		}
	}
	if (shift != 6) {
		os << static_cast<unsigned char>(sixtet + asciishift);
	}
	os << "\n";

	return true;
}

bool GraphIO::writeGraph6(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writeGraph6(G, os);
}

bool GraphIO::readGML(ClusterGraph &C, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGML(C, G, is);
}

bool GraphIO::readGML(ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) return false;

	GmlParser gml(is);
	if (gml.error())
		return false;

	return gml.read(G) && gml.readCluster(G, C);
}

bool GraphIO::writeGML(const ClusterGraph &C, const string &filename)
{
	ofstream os(filename);
	return writeGML(C, os);
}

bool GraphIO::readOGML(ClusterGraph &C, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readOGML(C, G, is);
}

bool GraphIO::readOGML(ClusterGraph &C, Graph &G, std::istream &is)
{
	OgmlParser parser;
	return parser.read(is, G, C);
}

bool GraphIO::writeOGML(const ClusterGraph &C, const string &filename)
{
	ofstream os(filename);
	return writeOGML(C, os);
}

bool GraphIO::readGML(GraphAttributes &A, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGML(A, G, is);
}

bool GraphIO::readGML(GraphAttributes &A, Graph &G, std::istream &is)
{
	if (!is.good()) return false;
	GmlParser parser(is);
	if (parser.error()) return false;
	return parser.read(G, A);
}

bool GraphIO::writeGML(const GraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeGML(A, os);
}

bool GraphIO::readOGML(GraphAttributes &A, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readOGML(A, G, is);
}

bool GraphIO::readOGML(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) return false;
	OgmlParser parser;
	return parser.read(is, G, A);
}

bool GraphIO::writeOGML(const GraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeOGML(A, os);
}

bool GraphIO::readRudy(GraphAttributes &A, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readRudy(A, G, is);
}

bool GraphIO::readRudy(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) return false;

	G.clear();

	int n, m;
	is >> n >> m;

	if(n < 0 || m < 0) {
		Logger::slout() << "GraphIO::readRudy: Illegal number of nodes or edges!\n";
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

bool GraphIO::writeRudy(const GraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeRudy(A, os);
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

bool GraphIO::readGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGML(A, C, G, is);
}

bool GraphIO::readGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) return false;

	GmlParser gml(is);
	if (gml.error())
		return false;

	if(!gml.read(G, A))
		return false;

	return gml.readCluster(G, C, &A);
}

bool GraphIO::writeGML(const ClusterGraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeGML(A, os);
}

bool GraphIO::readOGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readOGML(A, C, G, is);
}

bool GraphIO::readOGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) return false;
	OgmlParser parser;
	return parser.read(is, G, C, A);
}

bool GraphIO::writeOGML(const ClusterGraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeOGML(A, os);
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

bool GraphIO::readMatrixMarket(Graph& G, const string& filename)
{
	ifstream inStream(filename);
	return readMatrixMarket(G, inStream);
}

bool GraphIO::readBENCH(Graph &G, List<node>& hypernodes, List<edge>* shell, const string &filename)
{
	ifstream is(filename);
	return readBENCH(G, hypernodes, shell, is);
}

bool GraphIO::readPLA(Graph &G, List<node>& hypernodes, List<edge>* shell, const string &filename)
{
	ifstream is(filename);
	return readPLA(G, hypernodes, shell, is);
}

bool GraphIO::readChallengeGraph(Graph &G, GridLayout &gl, const string &filename)
{
	ifstream is(filename);
	return readChallengeGraph(G, gl, is);
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

bool GraphIO::writeChallengeGraph(const Graph &G, const GridLayout &gl, const string &filename)
{
	ofstream os(filename);
	return writeChallengeGraph(G, gl, os);
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

bool GraphIO::readEdgeListSubgraph(Graph &G, List<edge> &delEdges, const string &filename)
{
	ifstream is(filename);
	return readEdgeListSubgraph(G, delEdges, is);
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

bool GraphIO::writeEdgeListSubgraph(const Graph &G, const List<edge> &delEdges, const string &filename)
{
	ofstream os(filename);
	return writeEdgeListSubgraph(G, delEdges, os);
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

bool GraphIO::drawSVG(const GraphAttributes &A, const string &filename, const SVGSettings &settings)
{
	ofstream os(filename);
	return drawSVG(A, os, settings);
}

bool GraphIO::drawSVG(const ClusterGraphAttributes &A, const string &filename, const SVGSettings &settings)
{
	ofstream os(filename);
	return drawSVG(A, os, settings);
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

bool GraphIO::readGraphML(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGraphML(G, is);
}

bool GraphIO::readGraphML(Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	GraphMLParser parser(is);
	return parser.read(G);
}

bool GraphIO::writeGraphML(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writeGraphML(G, os);
}

bool GraphIO::readGraphML(ClusterGraph &C, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGraphML(C, G, is);
}

bool GraphIO::readGraphML(ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	GraphMLParser parser(is);
	return parser.read(G, C);
}

bool GraphIO::writeGraphML(const ClusterGraph &C, const string &filename)
{
	ofstream os(filename);
	return writeGraphML(C, os);
}

bool GraphIO::readGraphML(GraphAttributes &A, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGraphML(A, G, is);
}

bool GraphIO::readGraphML(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	GraphMLParser parser(is);
	return parser.read(G, A);
}

bool GraphIO::writeGraphML(const GraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeGraphML(A, os);
}

bool GraphIO::readGraphML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename)
{
	ifstream is(filename);
	return is.is_open() && readGraphML(A, C, G, is);
}

bool GraphIO::readGraphML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
{
	GraphMLParser parser(is);
	return parser.read(G, C, A);
}

bool GraphIO::writeGraphML(const ClusterGraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeGraphML(A, os);
}

bool GraphIO::readDOT(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readDOT(G, is);
}

bool GraphIO::readDOT(Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	dot::Parser parser(is);
	return parser.read(G);
}

bool GraphIO::writeDOT(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writeDOT(G, os);
}

bool GraphIO::readDOT(ClusterGraph &C, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readDOT(C, G, is);
}

bool GraphIO::readDOT(ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	dot::Parser parser(is);
	return parser.read(G, C);
}

bool GraphIO::writeDOT(const ClusterGraph &C, const string &filename)
{
	ofstream os(filename);
	return writeDOT(C, os);
}

bool GraphIO::readDOT(GraphAttributes &A, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readDOT(A, G, is);
}

bool GraphIO::readDOT(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	dot::Parser parser(is);
	return parser.read(G, A);
}

bool GraphIO::writeDOT(const GraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeDOT(A, os);
}

bool GraphIO::readDOT(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readDOT(A, C, G, is);
}

bool GraphIO::readDOT(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	dot::Parser parser(is);
	return parser.read(G, C, A);
}

bool GraphIO::writeDOT(const ClusterGraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeDOT(A, os);
}

bool GraphIO::readGEXF(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGEXF(G, is);
}

bool GraphIO::readGEXF(Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	gexf::Parser parser(is);
	return parser.read(G);
}

bool GraphIO::writeGEXF(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writeGEXF(G, os);
}

bool GraphIO::readGEXF(ClusterGraph &C, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGEXF(C, G, is);
}

bool GraphIO::readGEXF(ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	gexf::Parser parser(is);
	return parser.read(G, C);
}

bool GraphIO::writeGEXF(const ClusterGraph &C, const string &filename)
{
	ofstream os(filename);
	return writeGEXF(C, os);
}

bool GraphIO::readGEXF(GraphAttributes &A, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGEXF(A, G, is);
}

bool GraphIO::readGEXF(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	gexf::Parser parser(is);
	return parser.read(G, A);
}

bool GraphIO::writeGEXF(const GraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeGEXF(A, os);
}

bool GraphIO::readGEXF(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGEXF(A, C, G, is);
}

bool GraphIO::readGEXF(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	gexf::Parser parser(is);
	return parser.read(G, C, A);
}

bool GraphIO::writeGEXF(const ClusterGraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeGEXF(A, os);
}

bool GraphIO::readGDF(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGDF(G, is);
}

bool GraphIO::readGDF(Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	gdf::Parser parser(is);
	return parser.read(G);
}

bool GraphIO::writeGDF(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writeGDF(G, os);
}

bool GraphIO::readGDF(GraphAttributes &A, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readGDF(A, G, is);
}

bool GraphIO::readGDF(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	gdf::Parser parser(is);
	return parser.read(G, A);
}

bool GraphIO::writeGDF(const GraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeGDF(A, os);
}

bool GraphIO::readTLP(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readTLP(G, is);
}

bool GraphIO::readTLP(Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	tlp::Parser parser(is);
	return parser.read(G);
}

bool GraphIO::writeTLP(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writeTLP(G, os);
}

bool GraphIO::readTLP(ClusterGraph &C, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readTLP(C, G, is);
}

bool GraphIO::readTLP(ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	tlp::Parser parser(is);
	return parser.read(G, C);
}

bool GraphIO::writeTLP(const ClusterGraph &C, const string &filename)
{
	ofstream os(filename);
	return os.is_open() && writeTLP(C, os);
}

bool GraphIO::readTLP(GraphAttributes &A, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readTLP(A, G, is);
}

bool GraphIO::readTLP(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	tlp::Parser parser(is);
	return parser.read(G, A);
}

bool GraphIO::writeTLP(const GraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeTLP(A, os);
}

bool GraphIO::readTLP(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readTLP(A, C, G, is);
}

bool GraphIO::readTLP(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	tlp::Parser parser(is);
	return parser.read(G, C, A);
}

bool GraphIO::writeTLP(const ClusterGraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeTLP(A, os);
}

bool GraphIO::readDL(Graph &G, const string &filename)
{
	ifstream is(filename);
	return readDL(G, is);
}

bool GraphIO::readDL(Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	DLParser parser(is);
	return parser.read(G);
}

bool GraphIO::writeDL(const Graph &G, const string &filename)
{
	ofstream os(filename);
	return writeDL(G, os);
}

bool GraphIO::readDL(GraphAttributes &A, Graph &G, const string &filename)
{
	ifstream is(filename);
	return readDL(A, G, is);
}

bool GraphIO::readDL(GraphAttributes &A, Graph &G, std::istream &is)
{
	if(!is.good()) {
		return false;
	}
	DLParser parser(is);
	return parser.read(G, A);
}

bool GraphIO::writeDL(const GraphAttributes &A, const string &filename)
{
	ofstream os(filename);
	return writeDL(A, os);
}

}
