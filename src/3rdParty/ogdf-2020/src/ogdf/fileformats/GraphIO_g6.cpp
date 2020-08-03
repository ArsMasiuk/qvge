/** \file
 * \brief Implements read and write functionality for the
 *        Graph6 / Digraph6 / Sparse6 file formats.
 *
 * \author Jöran Schierbaum
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
#include <ogdf/basic/AdjacencyOracle.h>

namespace ogdf {

//! Abstract base class for implementations
class G6Abstract {
protected:
	G6Abstract(string header_, unsigned char startCharacter_ = static_cast<unsigned char>(0))
	: startCharacter(startCharacter_), header(header_) { }

	virtual ~G6Abstract() = default;

public:
	//! Returns true if a start character has been set by the child class
	virtual bool hasStartCharacter() const {
		return startCharacter != static_cast<unsigned char>(0);
	}

	//! Writes header information of the child class to a a stream \p os
	virtual void writeHeader(std::ostream &os) const {
		os << ">>" << header << "<<";
		if (hasStartCharacter()) {
			os << startCharacter;
		}
	}

	const unsigned char startCharacter; //!< The start character for this specific graph type
	const string header; //!< The name (header string) of this graph type
};

//! Abstract base class for reader/writer instances
template<typename Implementation>
class G6AbstractInstance {
protected:
	static const int c_asciishift = 63;

	G6AbstractInstance() = default;

	virtual ~G6AbstractInstance() = default;

	Implementation m_implementation; //!< File format specification
};

//! Abstract base class for readers
template<typename Implementation>
class G6AbstractReader : public G6AbstractInstance<Implementation> {
public:
	//! Internal reader state
	enum class State {
		Start,         //!< Initial reader state. Reads header if applicable, then starts the graph.
		GraphStart,    //!< Extra intermediary state to support graphs with a starting character.
		EighteenBit,   //!< Header: size of graph encoded in multiple bytes.
		RemainingBits, //!< Header: working state if we do have a multi-byte size.
		Body           //!< Main graph body.
	};

	G6AbstractReader(Graph &G, std::istream &is, bool forceHeader)
	: G6AbstractInstance<Implementation>(), m_forceHeader(forceHeader), m_G(G), m_is(is) { }

	//! Executes the read.
	/**
	 * If instance member forceHeader is set to true, fail if no header was found.
	 *
	 * @return true on success
	 */
	bool read() {
		if (!m_is.good()) {
			return false;
		}

		m_G.clear();
		init();

		if (m_forceHeader) {
			if (!detectHeader()) {
				return false;
			}
		}

		for (unsigned char readbyte; m_is >> readbyte;) {
			if (!parseByte(readbyte)) {
				return false;
			}
		}

		if (!finalize()) {
			return false;
		}

		return good();
	}

	//! Returns true if the parsed number of nodes equal the number in the graph
	bool good() const {
		return m_numberOfNodes == m_G.numberOfNodes();
	}

private:
	//! Tries to read the header from the input stream.
	/**
	 * @param fullHeader if false, assume the first '>' character to have been consumed already.
	 * @return true if header found
	 */
	bool detectHeader(bool fullHeader = true) {
		string header;
		int expectedLength = m_implementation.header.length() + 3;
		if (fullHeader) {
			++expectedLength;
		}
		header.resize(expectedLength);
		m_is.read(&header[0], expectedLength);
		return (header == (fullHeader ? ">>" : ">") + m_implementation.header + "<<");
	}

	//! Parses a single read byte.
	/**
	 * @param byte the byte that was read
	 */
	bool parseByte(int byte) {
		switch (m_state) {
		/* Initial states reading graph type and size */
		case State::Start:
			if (byte == '>') {
				if (!detectHeader(false)) {
					return false;
				}
			} else if (m_implementation.hasStartCharacter()) {
				// Graph must start with '&' for digraph6 and ':' for sparse6
				if (byte == m_implementation.startCharacter) {
					m_state = State::GraphStart;
				} else {
					// Not a valid graph6 / digraph6 / sparse6 graph
					return false;
				}
			} else {
				// For graph6, we have no start symbol and read the length immediately
				readSize(byte);
			}
			break;
		case State::GraphStart:
			// Only used for digraph6 and sparse6
			// Auxiliary state to skip the start character
			readSize(byte);
			break;
		/* Auxiliary graph size states */
		case State::EighteenBit:
			// Multi-byte encoded graph size
			if (byte == '~') {
				m_state = State::RemainingBits;
				m_remainingBits = 6;
			} else if (byte >= '?' && byte < '~') {
				m_numberOfNodes |= ((byte - c_asciishift) << 12);
				m_state = State::RemainingBits;
				m_remainingBits = 2;
			}
			break;
		case State::RemainingBits:
			if (byte >= '?' && byte <= '~') {
				--m_remainingBits;
				m_numberOfNodes |= ((byte - c_asciishift) << (6*m_remainingBits));
				if (m_remainingBits == 0) {
					addNodes();
				}
			}
			break;
		/* Main part of the graph */
		case State::Body:
			if (m_finished) {
				return false;
			}
			if (!parseByteBody(byte)) {
				return false;
			}
			m_firstByteInBody = false;
			break;
		}
		return true;
	}

	//! Add the specified number of nodes to the graph and switch to the main state.
	void addNodes() {
		m_index.init(m_numberOfNodes);
		for (int i = 0; i < m_numberOfNodes; ++i) {
			m_index[i] = m_G.newNode();
		}
		m_state = State::Body;
	}

	//! Starts reading the graph size.
	/**
	 * For single-byte sizes, read the size directly and add nodes.
	 * For multi-byte sizes, switch to the appropriate state EighteenBit.
	 */
	void readSize(int byte) {
		if (byte == '~') {
			m_state = State::EighteenBit;
		} else if (byte >= '?' && byte < '~') {
			m_numberOfNodes = byte - c_asciishift;
			addNodes();
		}
	}

protected:
	//! Initializes a reader instance with proper starting values
	virtual void init() { }

	//! Called for every read byte in the graph body
	/**
	 * @param byte the current byte
	 * @return false on malformed input
	 */
	virtual bool parseByteBody(int byte) = 0;

	//! Called after every byte of the body has been read
	virtual bool finalize() { return true; }

	//! Number of nodes as parsed from the input
	int m_numberOfNodes = 0;
	//! Adjacency matrix source index. For Sparse6, this is the currently handled node.
	int m_sourceIdx = 0;
	//! Adjacency matrix target index. For Sparse6, this is the currently read information.
	int m_targetIdx = 0;
	//! Bit counter for header and Sparse6 parsing.
	int m_remainingBits = 0;
	//! Whether we fail reading if no header was found.
	bool m_forceHeader;
	//! Whether the currently read byte was the first for the graph body
	bool m_firstByteInBody = true;
	//! Whether we should be finished reading
	bool m_finished = false;
	State m_state = State::Start;
	//! Indices for every node
	Array<node> m_index;
	Graph &m_G;
	std::istream &m_is;
	using G6AbstractInstance<Implementation>::c_asciishift;
	using G6AbstractInstance<Implementation>::m_implementation;
};

template<typename Implementation>
class G6AbstractWriter : G6AbstractInstance<Implementation> {
public:
	G6AbstractWriter(const Graph &G, std::ostream &os)
	: G6AbstractInstance<Implementation>(), m_G(G), m_os(os) { }

	//! Execute the write.
	bool write() {
		if (!m_os.good()) {
			return false;
		}

		// Header
		m_implementation.writeHeader(m_os);
		writeSize(m_G.numberOfNodes(), m_os);

		// Body
		if (!writeBody()) {
			return false;
		}

		m_os << "\n";
		return true;
	}

	//! Convert the nth sixtet \p sixtet of the number of nodes to a printable ASCII character
	unsigned char sixtetChar(int sixtet) const {
		return static_cast<unsigned char>(((m_G.numberOfNodes() >> (6*sixtet)) & c_asciishift) + c_asciishift);
	}

	//! Convert an integer \p value to a printable ASCII character
	unsigned char asciiChar(int value) const {
		OGDF_ASSERT(value >= 0);
		OGDF_ASSERT(value < 64);
		return static_cast<unsigned char>(value + c_asciishift);
	}

	//! Writes the size of the graph to the output stream
	void writeSize(int n, std::ostream &os) const {
		if (n < 63) {
			os << sixtetChar(0);
		} else if (n < 258048) {
			os
			<< '~'
			<< sixtetChar(2)
			<< sixtetChar(1)
			<< sixtetChar(0);
		} else { // XXX: < 68719476736
			os
			<< "~~"
			<< sixtetChar(5)
			<< sixtetChar(4)
			<< sixtetChar(3)
			<< sixtetChar(2)
			<< sixtetChar(1)
			<< sixtetChar(0);
		}
	}

protected:
	virtual bool writeBody() = 0;

	const Graph &m_G;
	std::ostream &m_os;
	using G6AbstractInstance<Implementation>::c_asciishift;
	using G6AbstractInstance<Implementation>::m_implementation;
};

//! Common abstract base class for g6 formats based on adjacency matrix (graph6/digraph6)
template<typename Implementation>
class G6AbstractReaderWithAdjacencyMatrix : public G6AbstractReader<Implementation> {
public:
	bool parseByteBody(int byte) override {
		// Graph edges for graph6 and digraph6
		if (byte >= '?' && byte <= '~') {
			OGDF_ASSERT(good());
#ifdef OGDF_DEBUG
			additionalAssertions();
#endif
			if (m_targetIdx >= m_numberOfNodes) {
				return false;
			}
			byte -= c_asciishift;
			// Add edges if the given bits are set
			// addEdge(int) increases the internal counter
			tryAddEdge(byte & 040);
			tryAddEdge(byte & 020);
			tryAddEdge(byte & 010);
			tryAddEdge(byte & 04);
			tryAddEdge(byte & 02);
			tryAddEdge(byte & 01);
			return true;
		}
		return false;
	}

protected:
	G6AbstractReaderWithAdjacencyMatrix(Graph &G, std::istream &is, bool forceHeader)
	: G6AbstractReader<Implementation>(G, is, forceHeader) { }

	//! Adds an edge if \p add is true. In any case, increase internal matrix indices in \p reader.
	virtual void tryAddEdge(bool add) {
		if (add) {
			m_G.newEdge(m_index[m_sourceIdx], m_index[m_targetIdx]);
		}
		++m_sourceIdx;
		// Read upper triangle only for graph6, but full adjacency matrix for digraph6
		if (finishedRow()) {
			m_sourceIdx = 0;
			++m_targetIdx;
		}
	}

	//! Checks whether our current adjacency matrix row in \p reader is complete
	virtual bool finishedRow() const = 0;

#ifdef OGDF_DEBUG
	//! Checks additional assertions on \p reader in debug mode
	virtual void additionalAssertions() const { }
#endif

	// Make used fields of parent class visible here
	using G6AbstractReader<Implementation>::c_asciishift;
	using G6AbstractReader<Implementation>::m_G;
	using G6AbstractReader<Implementation>::m_index;
	using G6AbstractReader<Implementation>::m_sourceIdx;
	using G6AbstractReader<Implementation>::m_targetIdx;
	using G6AbstractReader<Implementation>::m_numberOfNodes;
	using G6AbstractReader<Implementation>::good;
};


class Graph6Implementation : public G6Abstract {
public:
	Graph6Implementation() : G6Abstract("graph6") { }
};

class Graph6Reader : public G6AbstractReaderWithAdjacencyMatrix<Graph6Implementation> {
public:
	Graph6Reader(Graph &G, std::istream &is, bool forceHeader)
	: G6AbstractReaderWithAdjacencyMatrix<Graph6Implementation>(G, is, forceHeader) { }

private:
	void init() override {
		m_targetIdx = 1;
	}

#ifdef OGDF_DEBUG
	void additionalAssertions() const override {
		// The following only holds true for graph6, since digraph6 can have
		// directed edges going "backwards" in its full adjacency matrix.
		OGDF_ASSERT(m_sourceIdx < m_targetIdx);
	}
#endif

	bool finishedRow() const override {
		return (m_sourceIdx == m_targetIdx);
	}
};

class Graph6Writer : public G6AbstractWriter<Graph6Implementation> {
public:
	Graph6Writer(const Graph &G, std::ostream &os) : G6AbstractWriter<Graph6Implementation>(G, os) { }

private:
	bool writeBody() override {
		int sixtet = 0;
		int bit = 0x40; // 1 << 6

		AdjacencyOracle oracle(m_G);
		for (node v : m_G.nodes) {
			for (node w = m_G.firstNode(); w != v; w = w->succ()) {
				bit >>= 1;
				if (oracle.adjacent(v, w)) {
					sixtet |= bit;
				}
				if (bit == 1) {
					m_os << asciiChar(sixtet);
					bit = 0x40;
					sixtet = 0;
				}
			}
		}
		if (bit != 0x40) { // output remaining sixtet
			m_os << asciiChar(sixtet);
		}

		return true;
	}
};


class Sparse6Implementation : public G6Abstract {
public:
	Sparse6Implementation() : G6Abstract("sparse6", ':') { }
};

class Sparse6Reader : public G6AbstractReader<Sparse6Implementation> {
public:
	Sparse6Reader(Graph &G, std::istream &is, bool forceHeader)
	: G6AbstractReader<Sparse6Implementation>(G, is, forceHeader) { }

private:
	bool parseByteBody(int byte) override {
		if (m_firstByteInBody) {
			initReadBody();
		}

		if (byte == '\n') { // end condition
			m_finished = true;
			return true;
		}

		// Read bitwise
		for (int i = 5; i >= 0; --i) {
			int b = ((byte - c_asciishift) >> i) & 1; // (i+1)th bit
			if (m_remainingBits == 0) {
				if (m_targetIdx > m_sourceIdx) {
					m_sourceIdx = m_targetIdx;
				} else if (m_sourceIdx >= m_G.numberOfNodes()) {
					// last padding byte
					break;
				} else {
					m_G.newEdge(m_index[m_targetIdx], m_index[m_sourceIdx]);
				}

				if (b == 1) {
					++m_sourceIdx;
				}
				m_remainingBits = m_length;
				m_targetIdx = 0;
			} else {
				m_targetIdx = (m_targetIdx << 1) | b;
				--m_remainingBits;
			}
		}
		return true;
	}

	void init() override {
		m_sourceIdx = -1;
	}

	bool finalize() override {
		if (m_remainingBits == 0 && m_sourceIdx >= 0
		 && m_sourceIdx < m_G.numberOfNodes()
		 && m_targetIdx <= m_sourceIdx) {
			// Last edge has not been read yet
			m_G.newEdge(m_index[m_targetIdx], m_index[m_sourceIdx]);
		}

		return true;
	}

	//! Sets up the reader for the graph body based on the number of nodes we read.
	bool initReadBody() {
		// Set up read state
		OGDF_ASSERT(good());
		m_targetIdx = 0;
		if (m_numberOfNodes == 1) {
			m_length = 1;
		} else {
			m_length = static_cast<int>(std::log2(m_numberOfNodes-1)) + 1;
		}
		m_remainingBits = 0;
		return true;
	}

	//! Length in bits of each second entry (\c k in the format documentation)
	int m_length;
};

class Sparse6Writer : public G6AbstractWriter<Sparse6Implementation> {
public:
	Sparse6Writer(const Graph &G, std::ostream &os) : G6AbstractWriter<Sparse6Implementation>(G, os) { }

private:
	bool writeBody() override {
		int n = m_G.numberOfNodes();
		int sixtet = 0;
		int nbit = 6;   // current bit in buffer (counting starts at 1)
		// Maximum length of x in bit
		const int x_len = (n == 1 ? 1 : static_cast<int>(std::log2(n-1)) + 1);

		auto write_tuple = [&](bool b, int x) {
			sixtet |= (b << (nbit - 1));
			--nbit;
			int len = x_len;
			while (len >= nbit) {
				// We need more than one sixtet to store this tuple
				// Write the six next bits
				sixtet |= (x >> (len - nbit)) & 0x3F;
				m_os << asciiChar(sixtet);
				len -= nbit;
				nbit = 6;
				sixtet = 0;
			}
			// Write remainder if loop did not terminate on sixtet edge
			if (len > 0) {
				sixtet |= (x << (nbit - len)) & 0x3F;
				nbit -= len;
			}
		};

		int last = 0;
		NodeArray<int> index(m_G);
		int i = 0;
		for (node v : m_G.nodes) {
			index[v] = i;
			++i;
		}
		for (node v : m_G.nodes) {
			for (adjEntry adj : v->adjEntries) {
				node w = adj->twinNode();
				if (index[w] <= index[v]) {
					// Only store self-loops once
					if (w != v || adj->isSource()) {
						if (index[v] > last + 1) {
							// Index jump
							write_tuple(0, index[v]);
						}
						// bit b is 1 iff we increase by one from previous edge
						write_tuple((index[v] == last+1), index[w]);
						last = index[v];
					}
				}
			}
		}

		if (nbit != 6) { // pad and output remaining sixtet
			if ((n == 2 || n == 4 || n == 8 || n == 16)
			 && (last == n-2) /* vertex n-2 has edge and n-1 does not */
			 && (nbit >= x_len)) {
				// one 0 bit
				nbit--;
			}
			// pad with 1s
			sixtet |= (1 << nbit) - 1;
			m_os << asciiChar(sixtet);
		}

		return true;
	}
};


class Digraph6Implementation : public G6Abstract {
public:
	Digraph6Implementation() : G6Abstract("digraph6", '&') { }
};

class Digraph6Reader : public G6AbstractReaderWithAdjacencyMatrix<Digraph6Implementation> {
public:
	Digraph6Reader(Graph &G, std::istream &is, bool forceHeader)
	: G6AbstractReaderWithAdjacencyMatrix<Digraph6Implementation>(G, is, forceHeader) { }

private:
	bool finishedRow() const override {
		return (m_sourceIdx == m_numberOfNodes);
	}
};

class Digraph6Writer : public G6AbstractWriter<Digraph6Implementation> {
public:
	Digraph6Writer(const Graph &G, std::ostream &os) : G6AbstractWriter<Digraph6Implementation>(G, os) { }

private:
	bool writeBody() override {
		int n = m_G.numberOfNodes();
		int sixtet = 0;
		int bit = 0x40; // 1 << 6
		NodeArray<int> index(m_G);
		int i = 0;
		for (node v : m_G.nodes) {
			index[v] = i;
			++i;
		}
		std::vector<bool> adjacencies(n, false);
		for (node v : m_G.nodes) {
			adjacencies.assign(n, false);
			for (adjEntry adj : v->adjEntries) {
				if (adj->isSource()) {
					adjacencies[index[adj->twinNode()]] = true;
				}
			}
			for (bool isEdge : adjacencies) {
				bit >>= 1;
				if (isEdge) {
					sixtet |= bit;
				}
				if (bit == 1) {
					m_os << asciiChar(sixtet);
					bit = 0x40;
					sixtet = 0;
				}
			}
		}
		if (bit != 0x40) { // output remaining sixtet
			m_os << asciiChar(sixtet);
		}
		return true;
	}
};


bool GraphIO::readGraph6(Graph &G, std::istream &is, bool forceHeader)
{
	Graph6Reader reader(G, is, forceHeader);
	return reader.read();
}

bool GraphIO::readGraph6WithForcedHeader(Graph &G, std::istream &is)
{
	return readGraph6(G, is, true);
}

bool GraphIO::writeGraph6(const Graph &G, std::ostream &os)
{
	Graph6Writer writer(G, os);
	return writer.write();
}

bool GraphIO::readDigraph6(Graph &G, std::istream &is, bool forceHeader)
{
	Digraph6Reader reader(G, is, forceHeader);
	return reader.read();
}

bool GraphIO::readDigraph6WithForcedHeader(Graph &G, std::istream &is)
{
	return readDigraph6(G, is, true);
}

bool GraphIO::writeDigraph6(const Graph &G, std::ostream &os)
{
	Digraph6Writer writer(G, os);
	return writer.write();
}

bool GraphIO::readSparse6(Graph &G, std::istream &is, bool forceHeader)
{
	Sparse6Reader reader(G, is, forceHeader);
	return reader.read();
}

bool GraphIO::readSparse6WithForcedHeader(Graph &G, std::istream &is)
{
	return readSparse6(G, is, true);
}

bool GraphIO::writeSparse6(const Graph &G, std::ostream &os)
{
	Sparse6Writer writer(G, os);
	return writer.write();
}

}
