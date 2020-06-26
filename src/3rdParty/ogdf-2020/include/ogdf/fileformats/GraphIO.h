/** \file
 * \brief Declares class GraphIO which provides access to all
 *        graph read and write functionality.
 *
 * \author Carsten Gutwenger, Markus Chimani, Karsten Klein, Matthias Woste, Łukasz Hanuszczak
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

#include <ogdf/basic/exceptions.h>
#include <ogdf/basic/GridLayout.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraph.h>
#include <sstream>


namespace ogdf {


//! Utility class providing graph I/O in various exchange formats.
/**
 * @ingroup graphs graph-drawing
 */
class GraphIO
{
public:
	static Logger OGDF_EXPORT logger;

	//! Type of simple graph reader functions working on streams
	using ReaderFunc = bool (*)(Graph&, std::istream&);

	//! Type of simple graph writer functions working on streams
	using WriterFunc = bool (*)(const Graph&, std::ostream&);

	//! Type of simple graph attributes reader functions working on streams
	using AttrReaderFunc = bool (*)(GraphAttributes&, Graph&, std::istream&);

	//! Type of simple graph attributes writer functions working on streams
	using AttrWriterFunc = bool (*)(const GraphAttributes&, std::ostream&);

	//! Type of cluster graph reader functions working on streams
	using ClusterReaderFunc = bool (*)(ClusterGraph&, Graph&, std::istream&);

	//! Type of cluster graph writer functions working on streams
	using ClusterWriterFunc = bool (*)(const ClusterGraph&, std::ostream&);

	//! Type of cluster graph attributes reader functions working on streams
	using ClusterAttrReaderFunc = bool (*)(ClusterGraphAttributes&, ClusterGraph&, Graph&, std::istream&);

	//! Type of cluster graph attributes writer functions working on streams
	using ClusterAttrWriterFunc = bool (*)(const ClusterGraphAttributes&, std::ostream&);

	//! Condensed settings for drawing SVGs
	class OGDF_EXPORT SVGSettings
	{
		double m_margin;
		int    m_fontSize;
		double m_curviness;
		bool m_bezierInterpolation;
		string m_fontColor;
		string m_fontFamily;
		string m_width;
		string m_height;

	public:
		SVGSettings();

		//! Returns whether Bézier-interpolation for curved edges is enabled.
		bool bezierInterpolation() const { return m_bezierInterpolation; }

		//! Returns the size of the margin around the drawing.
		double margin() const { return m_margin; }

		//! Returns the curviness of the edges (value ranges from 0 to 1).
		double curviness() const { return m_curviness; }

		//! Returns the default font size (font height in pixels).
		int fontSize() const { return m_fontSize; }

		//! Returns the default font color.
		const string &fontColor() const { return m_fontColor; }

		//! Returns the default font family.
		const string &fontFamily() const { return m_fontFamily; }

		//! Returns the default width
		const string &width() const { return m_width; }

		//! Returns the default height
		const string &height() const { return m_height; }

		//! Sets the size of the margin around the drawing to \p m.
		void margin(double m) { m_margin = m; }

		//! Sets the curviness of all edges (value ranges from 0 to 1).
		void curviness(double value) {
			OGDF_ASSERT(value >= 0);
			OGDF_ASSERT(value <= 1);

			m_curviness = value;
		}

		//! Enables or disables Bézier-interpolation.
		void bezierInterpolation(bool enable) { m_bezierInterpolation = enable; }

		//! Sets the default font size (font height in pixels) to \p fs.
		void fontSize(int fs) { m_fontSize = fs; }

		//! Sets the default font color to \p fc.
		void fontColor(const string &fc) { m_fontColor = fc; }

		//! Sets the default font family to \p fm.
		void fontFamily(const string &fm) { m_fontFamily = fm; }

		//! Sets the width.
		/**
		 * The value should include a unit of measure (e.g., percentage for relative width or pixel values).
		 */
		void width(const string &width) { m_width = width; }

		//! Sets the height.
		/**
		 * The value should include a unit of measure (e.g., percentage for relative height or pixel values).
		 */
		void height(const string &height) { m_height = height; }
	};

	/**
	 * @name Graphs
	 * These functions read and write graphs (instances of type Graph) in various graph formats.
	 */
	//@{

	//! Reads arbitrary format from a file specified by name.
	/**
	 * @param G graph to be read.
	 * @param filename name of the file to read from.
	 * @param reader format to be used (e.g. #readGML), use #read(Graph &G, std::istream &is) for automated detection
	 * @return true if successful, false otherwise.
	 */
	static inline bool read(Graph &G, const string &filename, ReaderFunc reader = GraphIO::read) {
		std::ifstream is(filename);
		return is.good() && reader(G, is);
	}

	//! Writes arbitrary format to a file specified by name.
	/**
	 * @param G graph to be written.
	 * @param filename name of the file to write to.
	 * @param writer format to be used (e.g. #writeGML)
	 * @return true if successful, false otherwise.
	 */
	static inline bool write(const Graph &G, const string &filename, WriterFunc writer) {
		std::ofstream os(filename);
		return os.good() && writer(G, os);
	}

	//! Writes \p G to a file specified by name.
	/**
	 * The format of the file is deduced from the file extension of \p filename.
	 *
	 * @param G graph to be written.
	 * @param filename name of the file to write to.
	 * @return true if successful (including successful file format deduction),
	 * false otherwise.
	 */
	static OGDF_EXPORT bool write(const Graph &G, const string &filename);

	//! Reads graph \p G of arbitrary graph format from \p is.
	/**
	 * The following file formats are currently supported:
	 *  - DOT
	 *  - GML
	 *  - TLP
	 *  - LEDA
	 *  - Chaco
	 *  - DL
	 *  - GDF
	 *  - GraphML
	 *  - GEXF
	 *  - SteinLib
	 *  - Graph6 (with enforced header)
	 *  - Digraph6 (with enforced header)
	 *  - Sparse6 (with enforced header)
	 *  - DMF
	 *  - PMDissGraph
	 *  - Rudy
	 *
	 * @param G        is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool read(Graph &G, std::istream &is);

	//! Reads graph \p G including its attributes \p GA of arbitrary graph
	//! format from \p is.
	/**
	 * The following file formats are currently supported:
	 *  - DOT
	 *  - GML
	 *  - TLP
	 *  - DL
	 *  - GDF
	 *  - GraphML
	 *  - GEXF
	 *  - STP
	 *  - DMF
	 *  - Rudy
	 *
	 * @param GA is assigned the read graph attributes.
	 * @param G is assigned the read graph.
	 * @param is is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool read(GraphAttributes &GA, Graph &G, std::istream &is);

	//! Reads arbitrary format from a file specified by name.
	/**
	 * @param GA graph attributes to be read.
	 * @param G graph to be read.
	 * @param filename name of the file to read from.
	 * @param reader format to be used (e.g. #readGML).
	 * @return true if successful, false otherwise.
	 */
	static bool read(GraphAttributes &GA, Graph &G, const string &filename, AttrReaderFunc reader) {
		std::ifstream is(filename);
		return is.good() && reader(GA, G, is);
	}

	//! Writes arbitrary format to a file specified by name.
	/**
	 * @param GA graph attributes to be written.
	 * @param filename name of the file to write to.
	 * @param writer format to be used (e.g. #writeGML).
	 * @return true if successful, false otherwise.
	 */
	static bool write(const GraphAttributes &GA, const string &filename, AttrWriterFunc writer) {
		std::ofstream os(filename);
		return os.good() && writer(GA, os);
	}

	//@}

#pragma mark GML
	/**
	 * @name GML
	 *
	 * %Graph Modelling Language: https://en.wikipedia.org/wiki/Graph_Modelling_Language
	 */
	//@{

	//! Reads graph \p G in GML format from input stream \p is.
	/**
	 * The GML (<i>%Graph Modelling Language</i>) file format is an Ascii-based format that has been
	 * developed by Michael Himsolt at the University of Passau. Its full specification can be found in
	 * <a href="http://www.fim.uni-passau.de/fileadmin/files/lehrstuhl/brandenburg/projekte/gml/gml-technical-report.pdf">this
	 * technical report</a>.
	 *
	 * \sa writeGML(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGML(Graph &G, std::istream &is);

	//! Writes graph \p G in GML format to output stream \p os.
	/**
	 * The GML (<i>%Graph Modelling Language</i>) file format is an Ascii-based format that has been
	 * developed by Michael Himsolt at the University of Passau. Its full specification can be found in
	 * <a href="http://www.fim.uni-passau.de/fileadmin/files/lehrstuhl/brandenburg/projekte/gml/gml-technical-report.pdf">this
	 * technical report</a>. The GML format stores the basic graph structure, i.e., nodes and edges.
	 *
	 * \sa readGML(Graph &G, std::istream &is)
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGML(const Graph &G, std::ostream &os);

	//! Reads clustered graph (\p C, \p G) in GML format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with clustered graph \p C.
	 * \sa writeGML(const ClusterGraph &C, std::ostream &os)
	 *
	 * @param C   is assigned the read clustered graph (cluster structure).
	 * @param G   is assigned the read clustered graph (graph structure).
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGML(ClusterGraph &C, Graph &G, std::istream &is);

	//! Writes clustered graph \p C in GML format to output stream \p os.
	/**
	 * \sa readGML(ClusterGraph &C, Graph &G, std::istream &is)
	 *
	 * @param C   is the clustered graph to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGML(const ClusterGraph &C, std::ostream &os);

	//! Reads clustered graph (\p C, \p G) with attributes \p A in GML format from input stream \p is.
	/**
	 * \pre \p C is the clustered graph associated with attributes \p A, and \p G is the graph associated with \p C.
	 * \sa writeGML(const ClusterGraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param C   is assigned the read clustered graph (cluster structure).
	 * @param G   is assigned the read clustered graph (graph structure).
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is);

	//! Writes graph with attributes \p A in GML format to output stream \p os.
	/**
	 * \sa readGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
	 *
	 * @param A   specifies the clustered graph and its attributes to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGML(const ClusterGraphAttributes &A, std::ostream &os);

	//! Reads graph \p G with attributes \p A in GML format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa writeGML(const GraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGML(GraphAttributes &A, Graph &G, std::istream &is);

	//! Writes graph with attributes \p A in GML format to output stream \p os.
	/**
	 * \sa readGML(GraphAttributes &A, Graph &G, std::istream &is)
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGML(const GraphAttributes &A, std::ostream &os);

	//@}

#pragma mark Rome
	/**
	 * @name Rome
	 *
	 * Rome-Lib format: http://www.graphdrawing.org/data/
	 */
	//@{

	//! Reads graph \p G in Rome-Lib format from input stream \p is.
	/**
	 * The Rome-Lib format contains n "node-lines", 1 "separator-line", m "edge-lines" (in this order).
	 * These lines are as follows (whereby all IDs are integer numbers):
	 *  - <b>node-line:</b> <i>NodeId</i> <tt>0</TT>
	 *  - <b>separator-line:</b> starts with a <tt>#</tt>-sign
	 *  - <b>edge-line:</b> <i>EdgeId</i> <tt>0</tt> <i>SourceNodeId</i> <i>TargetNodeId</i>
	 *
	 * \sa writeRome(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readRome(Graph &G, std::istream &is);

	//! Writes graph \p G in Rome-Lib format to output stream \p os.
	/**
	 * \sa readRome(Graph &G, std::istream &is) for more details about the format.
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeRome(const Graph &G, std::ostream &os);

	//@}

#pragma mark LEDA
	/**
	 * @name LEDA
	 *
	 * LEDA Native File Format for Graphs: http://www.algorithmic-solutions.info/leda_guide/graphs/leda_native_graph_fileformat.html
	 */
	//@{

	//! Reads graph \p G in LEDA graph format from input stream \p is.
	/**
	 * The LEDA graph format is a simple, Ascii-based file format used by the
	 * <a href="http://www.algorithmic-solutions.com/leda/">LEDA library</a>.
	 * Its specification is described in the
	 * <a href="http://www.algorithmic-solutions.info/leda_guide/graphs/leda_native_graph_fileformat.html">
	 * LEDA Guide</a>.
	 *
	 * \sa  writeLEDA(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readLEDA(Graph &G, std::istream &is);

	//! Writes graph \p G in LEDA graph format to output stream \p os.
	/**
	 * The LEDA graph format is a simple, Ascii-based file format used by the
	 * <a href="http://www.algorithmic-solutions.com/leda/">LEDA library</a>.
	 * Its specification is described in the
	 * <a href="http://www.algorithmic-solutions.info/leda_guide/graphs/leda_native_graph_fileformat.html">
	 * LEDA Guide</a>.
	 *
	 * \sa readLEDA(Graph &G, std::istream &is)
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeLEDA(const Graph &G, std::ostream &os);

	//@}

#pragma mark Chaco
	/**
	 * @name Chaco
	 *
	 * https://cfwebprod.sandia.gov/cfdocs/CompResearch/docs/guide.pdf
	 */
	//@{

	//! Reads graph \p G in Chaco format from input stream \p is.
	/**
	 * This simple graph format is used by graph partitioning tools like
	 * Chaco, Metis, or Jostle.
	 * Its specification is described in the
	 * <a href="http://staffweb.cms.gre.ac.uk/~wc06/jostle/jostle-exe.pdf">
	 * Jostle User Guide</a>.
	 *
	 * \sa writeChaco(const Graph &G, std::ostream &os)
	 *
	 * @param G  is assigned the read graph.
	 * @param is is the input stream to be read.
	 * \return true if successful, false otherwise.
	 * */
	static OGDF_EXPORT bool readChaco(Graph &G, std::istream &is);

	//! Writes graph \p G in Chaco format to output stream \p os.
	/**
	 * This simple graph format is used by graph partitioning tools like
	 * Chaco, Metis, or Jostle.
	 * Its specification is described in the
	 * <a href="http://staffweb.cms.gre.ac.uk/~wc06/jostle/jostle-exe.pdf">
	 * Jostle User Guide</a>.
	 *
	 * \sa readChaco(Graph &G, std::istream &is)
	 *
	 * @param G  is the graph to be written.
	 * @param os is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeChaco(const Graph &G, std::ostream &os);

	//@}

#pragma mark PMDissGraph
	/**
	 * @name PMDissGraph
	 *
	 * %Graph file format from [Petra Mutzel, The maximum planar subgraph problem, PhD Thesis, Köln University, 1994]
	 */
	//@{

	//! Reads graph \p G in a simple format as used in Petra Mutzel's thesis from input stream \p is.
	/**
	 * This simple graph format has a leading line stating the name of the graph
	 * and a following line stating the size of the graph:
	 *
	 * <pre>
	 * *BEGIN unknown_name.numN.numE
	 * *GRAPH numN numE UNDIRECTED UNWEIGHTED
	 * </pre>
	 *
	 * \sa writePMDissGraph(const Graph &G, std::ostream &os)
	 *
	 * @param G  is assigned the read graph.
	 * @param is is the input stream to be read.
	 * \return true if successful, false otherwise.
	 * */
	static OGDF_EXPORT bool readPMDissGraph(Graph &G, std::istream &is);

	//! Writes graph \p G in a simple format as used in Petra Mutzel's thesis to output stream \p os.
	/**
	 * This simple graph format has a leading line stating the name of the graph
	 * and a following line stating the size of the graph:
	 *
	 * <pre>
	 * *BEGIN unknown_name.numN.numE
	 * *GRAPH numN numE UNDIRECTED UNWEIGHTED
	 * </pre>
	 *
	 * \sa readPMDissGraph(Graph &G, std::istream &is)
	 *
	 * @param G  is the graph to be written.
	 * @param os is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writePMDissGraph(const Graph &G, std::ostream &os);

	//@}

#pragma mark YGraph
	/**
	 * @name YGraph
	 *
	 * http://www3.cs.stonybrook.edu/~algorith/implement/nauty/distrib/makebg.c
	 */
	//@{

	//! Reads graph \p G in Y-graph format from input stream \p is.
	/**
	 * This format is e.g. produced by NAUTY (http://www.cs.sunysb.edu/~algorith/implement/nauty/implement.shtml).
	 *
	 * Details  on the format, as given in NAUTYs graph generator (see above link):
	 * "[A] graph occupies one line with a terminating newline.
	 * Except for the newline, each byte has the format  01xxxxxx, where
	 * each "x" represents one bit of data.
	 *
	 * First byte:  xxxxxx is the number of vertices n
	 *
	 * Other ceiling(n(n-1)/12) bytes:  These contain the upper triangle of
	 * the adjacency matrix in column major order.  That is, the entries
	 * appear in the order (0,1),(0,2),(1,2),(0,3),(1,3),(2,3),(0,4),... .
	 * The bits are used in left to right order within each byte.
	 * Any unused bits on the end are set to zero.
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readYGraph(Graph &G, std::istream &is);

	//@}

#pragma mark Graph6
	/**
	 * @name Graph6
	 *
	 * The Graph6 format represents an (preferable dense or small)
	 * simple undirected graph as a string containing printable characters
	 * between 0x3F and 0x7E.
	 * <a href="http://cs.anu.edu.au/~bdm/data/formats.txt">See the specification for more information.</a>
	 */
	//@{

	//! Reads graph \p G in Graph6 format from input stream \p is.
	/**
	 * \sa writeGraph6(const Graph &G, std::ostream &os)
	 *
	 * @param G           is assigned the read graph.
	 * @param is          is the input stream to be read.
	 * @param forceHeader if the file has to start with '>>graph6<<'.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGraph6(Graph &G, std::istream &is, bool forceHeader = false);

	//! Writes graph \p G in Graph6 format to output stream \p os.
	/**
	 * \sa readGraph6(Graph &G, std::istream &is)
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGraph6(const Graph &G, std::ostream &os);

	//@}

#pragma mark Digraph6
	/**
	 * @name Digraph6
	 *
	 * The Digraph6 format represents simple directed graphs (allowing loops)
	 * as a string containing printable characters between 0x3F and 0x7E.
	 * <a href="http://cs.anu.edu.au/~bdm/data/formats.txt">See the specification for more information.</a>
	 */
	//@{

	//! Reads graph \p G in Digraph6 format from input stream \p is.
	/**
	 * \sa writeDigraph6(const Graph &G, std::ostream &os)
	 *
	 * @param G           is assigned the read graph.
	 * @param is          is the input stream to be read.
	 * @param forceHeader if the file has to start with '>>digraph6<<'.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readDigraph6(Graph &G, std::istream &is, bool forceHeader = false);

	//! Writes graph \p G in Digraph6 format to output stream \p os.
	/**
	 * \sa readDigraph6(Graph &G, std::istream &is)
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDigraph6(const Graph &G, std::ostream &os);

	//@}

#pragma mark Sparse6
	/**
	 * @name Sparse6
	 *
	 * The Sparse6 format represents a (preferably sparse) undirected graph
	 * as a string containing printable characters between 0x3F and 0x7E.
	 * <a href="http://cs.anu.edu.au/~bdm/data/formats.txt">See the specification for more information.</a>
	 */
	//@{

	//! Reads graph \p G in Sparse6 format from input stream \p is.
	/**
	 * \sa writeSparse6(const Graph &G, std::ostream &os)
	 *
	 * @param G           is assigned the read graph.
	 * @param is          is the input stream to be read.
	 * @param forceHeader if the file has to start with '>>sparse6<<'.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readSparse6(Graph &G, std::istream &is, bool forceHeader = false);

	//! Writes graph \p G in Sparse6 format to output stream \p os.
	/**
	 * \sa readSparse6(Graph &G, std::istream &is)
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeSparse6(const Graph &G, std::ostream &os);

	//@}

#pragma mark MatrixMarket
	/**
	 * @name MatrixMarket
	 *
	 * http://math.nist.gov/MatrixMarket/formats.html
	 */
	//@{

	//! Reads graph \p G in Matrix Market exchange format from stream \p inStream.
	/**
	 * @param G        is assigned the read graph.
	 * @param inStream is the input stream to read from
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readMatrixMarket(Graph& G, std::istream &inStream);

	//@}

#pragma mark Rudy
	/**
	 * @name Rudy
	 */
	//@{

	//! Reads graph \p G with edge weights stored in \p A in Rudy format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa writeRudy(const GraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes (only edge weights (as doubles) are used).
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readRudy(GraphAttributes &A, Graph &G, std::istream &is);

	//! Reads graph \p G in Rudy format from input stream \p is.
	/**
	 * \warning The edge weights are ignored.
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readRudy(Graph &G, std::istream &is) {
		GraphAttributes A(G, GraphAttributes::edgeDoubleWeight);
		return readRudy(A, G, is);
	}

	//! Writes graph with edge weights stored in \p A in Rudy format to output stream \p os.
	/**
	 * \sa readRudy(GraphAttributes &A, Graph &G, std::istream &is)
	 *
	 * @param A   specifies the graph and its attributes to be written (only edge weights
	 *            (as doubles) are stored in this format).
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeRudy(const GraphAttributes &A, std::ostream &os);

	//@}

#pragma mark BENCH
	/**
	 * @name BENCH
	 */
	//@{

	//!  Reads a hypergraph (as point-based expansion) in BENCH format from input stream \p is.
	/**
	 * A hypergraph in OGDF is represented by its point-based expansion, i.e., for each
	 * hyperedge <i>h</i> we have a corresponding hypernode <i>n</i>. All nodes originally
	 * incident to <i>h</i> are incident to <i>n</i>, i.e., have regular edges to <i>n</i>.
	 *
	 * \warning
	 * This is a very simple implementation only usable for very properly formatted files!
	 *
	 * @param G          is assigned the read graph (point-based expansion of the hypergraph).
	 * @param hypernodes is assigned the list of nodes which have to be interpreted as hypernodes.
	 * @param shell      if 0 only the BENCH-hypergraph is read. Otherwise we extend the graph
	 *                   by a simple edge <i>e=(i,o)</i> and two hyperedges: one hyperedges groups all input nodes and
	 *                   <i>i</i> together, the other hyperedge groups all output edges and <i>o</i>.
	 *                   These additional edges are then also collocated in shell.
	 * @param is         is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readBENCH(Graph &G, List<node>& hypernodes, List<edge> *shell, std::istream &is);

	//@}

#pragma mark PLA
	/**
	 * @name PLA
	 */
	//@{

	//!  Reads a hypergraph (as point-based expansion) in PLA format from input stream \p is.
	/**
	 * A hypergraph in OGDF is represented by its point-based expansion, i.e., for each
	 * hyperedge <i>h</i> we have a corresponding hypernode <i>n</i>. All nodes originally
	 * incident to <i>h</i> are incident to <i>n</i>, i.e., have regular edges to <i>n</i>.
	 *
	 * \warning
	 * This is a very simple implementation only usable for very properly formatted files!
	 *
	 * @param G          is assigned the read graph (point-based expansion of the hypergraph).
	 * @param hypernodes is assigned the list of nodes which have to be interpreted as hypernodes.
	 * @param shell      if 0 only the PLA-hypergraph is read. Otherwise we extend the graph
	 *                   by a simple edge <i>e=(i,o)</i> and two hyperedges: one hyperedges groups all input nodes and
	 *                   <i>i</i> together, the other hyperedge groups all output edges and <i>o</i>.
	 *                   These additional edges are then also collocated in shell.
	 * @param is         is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readPLA(Graph &G, List<node>& hypernodes, List<edge> *shell, std::istream &is);

	//@}

#pragma mark ChallengeGraph
	/**
	 * @name GD-Challenge
	 *
	 * %Graph %Drawing %Challenge: %Area %Minimization for %Orthogonal %Grid %Layouts
	 * http://graphdrawing.de/contest2013/challenge.html
	 */
	//@{

	//! Reads graph \p G with grid layout \p gl in GD-Challenge-format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with grid layout \p gl.
	 * \sa writeChallengeGraph(const Graph &G, const GridLayout &gl, std::ostream &os)
	 *
	 * @param G  is assigned the read graph.
	 * @param gl is assigned the grid layout.
	 * @param is is the input stream from which the graph is read.
	 * \return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readChallengeGraph(Graph &G, GridLayout &gl, std::istream &is);

	//! Writes graph \p G with grid layout \p gl in GD-Challenge-format to output stream \p os.
	/**
	 * \pre \p G is the graph associated with grid layout \p gl.
	 * \sa readChallengeGraph(Graph &G, GridLayout &gl, std::istream &is)
	 *
	 * @param G  is the graph to be written.
	 * @param gl specifies the grid layout of \p G to be written.
	 * @param os is the output stream to which the graph is written.
	 * \return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeChallengeGraph(const Graph &G, const GridLayout &gl, std::ostream &os);

	//@}

#pragma mark GraphML
	/**
	 * @name GraphML
	 *
	 * %Graph %Markup %Language: http://graphml.graphdrawing.org/
	 */
	//@{

	//! Reads graph \p G in GraphML format from input stream \p is.
	/**
	 * \sa writeGraphML(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGraphML(Graph &G, std::istream &is);

	//! Reads clustered graph (\p C, \p G) in GraphML format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with clustered graph \p C.
	 * \sa writeGraphML(const ClusterGraph &C, std::ostream &os)
	 *
	 * @param C   is assigned the read clustered graph (cluster structure).
	 * @param G   is assigned the read clustered graph (graph structure).
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGraphML(ClusterGraph &C, Graph &G, std::istream &is);

	//! Reads graph \p G with attributes \p A in GraphML format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa writeGraphML(const GraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGraphML(GraphAttributes &A, Graph &G, std::istream &is);

	//! Reads clustered graph (\p C, \p G) with attributes \p A in GraphML format from input stream \p is.
	/**
	 * \pre \p C is the clustered graph associated with attributes \p A, and \p G is the graph associated with \p C.
	 * \sa writeGraphML(const ClusterGraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param C   is assigned the read clustered graph (cluster structure).
	 * @param G   is assigned the read clustered graph (graph structure).
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGraphML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is);

	//! Writes graph \p G in GraphML format to output stream \p os.
	/**
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGraphML(const Graph &G, std::ostream &os);

	//! Writes clustered graph \p C in GraphML format to output stream \p os.
	/**
	 *
	 * @param C   is the clustered graph to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGraphML(const ClusterGraph &C, std::ostream &os);

	//! Writes graph with attributes \p A in GraphML format to output stream \p os.
	/**
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGraphML(const GraphAttributes &A, std::ostream &os);

	//! Writes graph with attributes \p A in GraphML format to output stream \p os.
	/**
	 *
	 * @param A   specifies the clustered graph and its attributes to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGraphML(const ClusterGraphAttributes &A, std::ostream &os);

	//@}

#pragma mark DOT
	/**
	 * @name DOT
	 *
	 * http://www.graphviz.org/doc/info/lang.html
	 */
	//@{

	//! Reads graph \p G in DOT format from input stream \p is.
	/**
	 * \sa writeDOT(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readDOT(Graph &G, std::istream &is);

	//! Reads clustered graph (\p C, \p G) in DOT format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with clustered graph \p C.
	 * \sa writeDOT(const ClusterGraph &C, std::ostream &os)
	 *
	 * @param C   is assigned the read clustered graph (cluster structure).
	 * @param G   is assigned the read clustered graph (graph structure).
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readDOT(ClusterGraph &C, Graph &G, std::istream &is);

	//! Reads graph \p G with attributes \p A in DOT format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa writeDOT(const GraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readDOT(GraphAttributes &A, Graph &G, std::istream &is);

	//! Reads clustered graph (\p C, \p G) with attributes \p A in DOT format from input stream \p is.
	/**
	 * \pre \p C is the clustered graph associated with attributes \p A, and \p G is the graph associated with \p C.
	 * \sa writeDOT(const ClusterGraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param C   is assigned the read clustered graph (cluster structure).
	 * @param G   is assigned the read clustered graph (graph structure).
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readDOT(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is);

	//! Writes graph \p G in DOT format to output stream \p os.
	/**
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDOT(const Graph &G, std::ostream &os);

	//! Writes clustered graph \p C in DOT format to output stream \p os.
	/**
	 *
	 * @param C   is the clustered graph to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDOT(const ClusterGraph &C, std::ostream &os);

	//! Writes graph with attributes \p A in DOT format to output stream \p os.
	/**
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDOT(const GraphAttributes &A, std::ostream &os);

	//! Writes graph with attributes \p A in DOT format to output stream \p os.
	/**
	 *
	 * @param A   specifies the clustered graph and its attributes to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDOT(const ClusterGraphAttributes &A, std::ostream &os);

	//@}

#pragma mark GEXF
	/**
	 * @name GEXF
	 *
	 * %Graph %Exchange %XML %Format: https://gephi.org/gexf/format/
	 */
	//@{

	//! Reads graph \p G in GEXF format from input stream \p is.
	/**
	 * \sa writeGEXF(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGEXF(Graph &G, std::istream &is);

	//! Reads clustered graph (\p C, \p G) in GEXF format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with clustered graph \p C.
	 * \sa writeGEXF(const ClusterGraph &C, std::ostream &os)
	 *
	 * @param C   is assigned the read clustered graph (cluster structure).
	 * @param G   is assigned the read clustered graph (graph structure).
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGEXF(ClusterGraph &C, Graph &G, std::istream &is);

	//! Reads graph \p G with attributes \p A in GEXF format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa writeGEXF(const GraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGEXF(GraphAttributes &A, Graph &G, std::istream &is);

	//! Reads clustered graph (\p C, \p G) with attributes \p A in GEXF format from input stream \p is.
	/**
	 * \pre \p C is the clustered graph associated with attributes \p A, and \p G is the graph associated with \p C.
	 * \sa writeGEXF(const ClusterGraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param C   is assigned the read clustered graph (cluster structure).
	 * @param G   is assigned the read clustered graph (graph structure).
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGEXF(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is);

	//! Writes graph \p G in GEXF format to output stream \p os.
	/**
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGEXF(const Graph &G, std::ostream &os);

	//! Writes clustered graph \p C in GEXF format to output stream \p os.
	/**
	 *
	 * @param C   is the clustered graph to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGEXF(const ClusterGraph &C, std::ostream &os);

	//! Writes graph with attributes \p A in GEXF format to output stream \p os.
	/**
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGEXF(const GraphAttributes &A, std::ostream &os);

	//! Writes graph with attributes \p A in GEXF format to output stream \p os.
	/**
	 *
	 * @param A   specifies the clustered graph and its attributes to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGEXF(const ClusterGraphAttributes &A, std::ostream &os);

	//@}

#pragma mark GDF
	/**
	 * @name GDF
	 *
	 * %GUESS %Database %File: http://graphexploration.cond.org/manual.html
	 */
	//@{

	//! Reads graph \p G in GDF format from input stream \p is.
	/**
	 * \sa writeGDF(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGDF(Graph &G, std::istream &is);

	//! Reads graph \p G with attributes \p A in GDF format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa writeGDF(const GraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGDF(GraphAttributes &A, Graph &G, std::istream &is);

	//! Writes graph \p G in GDF format to output stream \p os.
	/**
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGDF(const Graph &G, std::ostream &os);

	//! Writes graph with attributes \p A in GDF format to output stream \p os.
	/**
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGDF(const GraphAttributes &A, std::ostream &os);

	//@}

#pragma mark TLP
	/**
	 * @name TLP
	 *
	 * Tulip software graph format: http://tulip.labri.fr/TulipDrupal/?q=tlp-file-format
	 */
	//@{

	//! Reads graph \p G in TLP format from input stream \p is.
	/**
	 * \sa writeTLP(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readTLP(Graph &G, std::istream &is);

	//! Reads clustered graph (\p C, \p G) in TLP format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with clustered graph \p C.
	 * \sa writeTLP(const ClusterGraph &C, std::ostream &os)
	 *
	 * @param C   is assigned the read clustered graph (cluster structure).
	 * @param G   is assigned the read clustered graph (graph structure).
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readTLP(ClusterGraph &C, Graph &G, std::istream &is);

	//! Reads graph \p G with attributes \p A in TLP format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa writeTLP(const GraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readTLP(GraphAttributes &A, Graph &G, std::istream &is);

	//! Reads clustered graph (\p C, \p G) with attributes \p A in TLP format from input stream \p is.
	/**
	 * \pre \p C is the clustered graph associated with attributes \p A, and \p G is the graph associated with \p C.
	 * \sa writeTLP(const ClusterGraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param C   is assigned the read clustered graph (cluster structure).
	 * @param G   is assigned the read clustered graph (graph structure).
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readTLP(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is);

	//! Writes graph \p G in TLP format to output stream \p os.
	/**
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeTLP(const Graph &G, std::ostream &os);

	//! Writes clustered graph \p C in TLP format to output stream \p os.
	/**
	 *
	 * @param C   is the clustered graph to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeTLP(const ClusterGraph &C, std::ostream &os);

	//! Writes graph with attributes \p A in TLP format to output stream \p os.
	/**
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeTLP(const GraphAttributes &A, std::ostream &os);

	//! Writes graph with attributes \p A in TLP format to output stream \p os.
	/**
	 *
	 * @param A   specifies the clustered graph and its attributes to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeTLP(const ClusterGraphAttributes &A, std::ostream &os);

	//@}

#pragma mark DL
	/**
	 * @name DL
	 *
	 * %UCINET %DL format: https://sites.google.com/site/ucinetsoftware/document/ucinethelp.htm
	 */
	//@{

	//! Reads graph \p G in DL format from input stream \p is.
	/**
	 * \sa writeDL(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readDL(Graph &G, std::istream &is);

	//! Reads graph \p G with attributes \p A in DL format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa writeDL(const GraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readDL(GraphAttributes &A, Graph &G, std::istream &is);

	//! Writes graph \p G in DL format to output stream \p os.
	/**
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDL(const Graph &G, std::ostream &os);

	//! Writes graph with attributes \p A in DL format to output stream \p os.
	/**
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDL(const GraphAttributes &A, std::ostream &os);

	//@}

#pragma mark STP
	/**
	 * @name STP
	 *
	 * %SteinLib %STP %Data %Format: http://steinlib.zib.de/format.php
	 */
	//@{

	/**
	 * @copydoc readSTP(GraphAttributes&, Graph&, std::istream&)
	 *
	 * @param terminals will contain a list of all terminals in the graph,
	 * 		in case of a directed graph the root will be at the lists front position
	 * @param isTerminal maps whether each node is a terminal or a Steiner node
	 */
	static OGDF_EXPORT bool readSTP(GraphAttributes &attr, Graph &G, List<node> &terminals, NodeArray<bool> &isTerminal, std::istream &is);

	/**
	 * @copydoc readSTP(Graph&, std::istream&)
	 * The node shape graph attribute will be set according to the node type:
	 * Non-Terminal: Shape::Ellipse, Terminal: Shape::Rectangle,
	 * Root (directed STP): Shape::Triangle
	 *
	 * @param attr will contain the weights of the edges, node coordinates and type (encoded
	 * 		in shape as described above) as well as whether the read graph is directed
	 * @pre in \p attr GraphAttributes::intWeight Xor GraphAttributes::doubleWeight has to be set
	 */
	static OGDF_EXPORT bool readSTP(GraphAttributes &attr, Graph &G, std::istream &is) {
		List<node> terminals;
		NodeArray<bool> isTerminal;
		return readSTP(attr, G, terminals, isTerminal, is);
	}

	/**
	 * Reads a graph in SteinLib format from std::istream \p is.
	 *
	 * @param G is assigned the read graph.
	 * @param is is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readSTP(Graph &G, std::istream &is) {
		GraphAttributes attr(G);
		return readSTP(attr, G, is);
	}

	/**
	 * Reads a SteinLib instance from an inputstream \p is and converts it into
	 * a weighted graph \p wG and a set of terminal nodes \p terminals.
	 * \warning The coordinate section of the SteinLib instance is not read!
	 *
	 * @param wG the edge weighted graph
	 * @param terminals will contain a list of all terminals in the graph,
	 * 		in case of a directed graph the root will be at the lists front position
	 * @param isTerminal maps whether each node is a terminal or a Steiner node
	 * @param is the input stream to be read
	 *
	 * @return true if the STP was read successfully, false otherwise
	 */
	template<typename T>
	static bool readSTP(
		EdgeWeightedGraph<T> &wG,
		List<node> &terminals,
		NodeArray<bool> &isTerminal,
		std::istream &is)
	{
		wG.clear();
		GraphAttributes attr(wG, getEdgeWeightFlag<T>());
		bool res = readSTP(attr, wG, terminals, isTerminal, is);
		for (edge e : wG.edges) {
			wG.setWeight(e, getEdgeWeightAttribute<T>(attr, e));
		}
		return res;
	}

	/**
	 * Writes an Steiner problem instance to an STP file.
	 *
	 * Double edge weights will be preferred to integer edge weights if both
	 * GraphAttributes::intWeight and GraphAttributes::doubleWeight are set.
	 *
	 * @param attr contains the graph as well as its edge weights, node
	 * 		coordinates (if GraphAttributes::nodeGraphics is enabled) and
	 * 		whether the graph is directed
	 * @param terminals a list of all terminals in the graph,
	 * 		in the case of a directed graph, the root needs to be the front element
	 * @param os the output stream be written to
	 * @param comments a string containing all comments seperated by LF
	 *   if this is an empty string no comment section will be created
	 *
	 * @return true if the write operation succeeded, false otherwise
	 */
	static OGDF_EXPORT bool writeSTP(
		const GraphAttributes &attr,
		const List<node> &terminals,
		std::ostream &os,
		const string &comments = "");

	/**
	 * Writes an Steiner problem instance to an STP file.
	 *
	 * @param wG the edge weighted graph
	 * @param terminals a list of all terminals in the graph,
	 * 		in the case of a directed graph, the root needs to be the front element
	 * @param os the output stream be written to
	 * @param comments a string containing all comments seperated by LF
	 *   if this is an empty string no comment section will be created
	 *
	 * @return true if the write operation succeeded, false otherwise
	 */
	template<typename T>
	static bool writeSTP(
		const EdgeWeightedGraph<T> &wG,
		const List<node> &terminals,
		std::ostream &os,
		const string &comments = "")
	{
		GraphAttributes attr(wG, getEdgeWeightFlag<T>());
		for (edge e : wG.edges) {
			getEdgeWeightAttribute<T>(attr, e) = wG.weight(e);
		}
		return writeSTP(attr, terminals, os, comments);
	}

	//@}

#pragma mark DMF
	/**
	 * @name DMF
	 *
	 * DIMACS Max Flow Challenge: ftp://dimacs.rutgers.edu/pub/netflow/
	 */
	//@{

	/**
	 * @copydoc readDMF(GraphAttributes&, Graph&, std::istream&)
	 * @param source will contain the flow source
	 * @param sink will contain the flow sink
	 */
	static OGDF_EXPORT bool readDMF(GraphAttributes &attr, Graph &graph, node &source, node &sink, std::istream &is);

	/**
	 * Reads a maximum flow instance in DIMACS format.
	 *
	 * @param attr will contain the weights of the edges
	 *        (GraphAttributes::intWeight or GraphAttributes::doubleWeight has to be set)
	 * @param graph will contain the parsed graph
	 * @param is input stream
	 * @return \c true iff the instance was parsed successfully
	 */
	static OGDF_EXPORT bool readDMF(GraphAttributes &attr, Graph &graph, std::istream &is) {
		node sink;
		node source;
		return readDMF(attr, graph, sink, source, is);
	};

	/**
	 * Reads a maximum flow instance in DIMACS format.
	 *
	 * @param graph will contain the parsed graph
	 * @param weights will contain the weights of the edges
	 * @param source will contain the flow source
	 * @param sink will contain the flow sink
	 * @param is input stream
	 * @tparam the type of the edge weights
	 *
	 * @return \c true iff the instance was parsed successfully
	 */
	template<typename T>
	static bool readDMF(Graph &graph, EdgeArray<T> &weights, node &source, node &sink, std::istream &is) {
		GraphAttributes attr(graph, getEdgeWeightFlag<T>());
		bool result = readDMF(attr, graph, source, sink, is);
		weights.init(graph);
		for (edge e : graph.edges) {
			weights[e] = getEdgeWeightAttribute<T>(attr, e);
		}
		return result;
	}

	/**
	 * Reads a maximum flow instance in DIMACS format.
	 * \warning The weights as well as the source and sink nodes are ignored.
	 *
	 * @param graph is assigned the read graph.
	 * @param is is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readDMF(Graph &graph, std::istream &is) {
		GraphAttributes attr(graph);
		node source;
		node sink;
		return readDMF(attr, graph, source, sink, is);
	}

	/**
	 * Writes a maximum flow problem instance to a DIMACS maximum flow file.
	 *
	 * Double edge weights will be preferred to integer edge weights if both
	 * GraphAttributes::intWeight and GraphAttributes::doubleWeight are set.
	 *
	 * @param attr contains the graph as well as its edge weights
	 * @param source source of the maximum flow
	 * @param sink sink of the maximum flow
	 * @param os the output stream be written to
	 *
	 * @return \c true if the write operation succeeded, false otherwise
	 */
	static OGDF_EXPORT bool writeDMF(
			const GraphAttributes &attr,
			const node source,
			const node sink,
			std::ostream &os);

	/**
	 * Writes a maximum flow problem instance to a DIMACS maximum flow file.
	 *
	 * @param graph graph to be written
	 * @param weights contains the weights of the edges
	 * @param source source of the maximum flow
	 * @param sink sink of the maximum flow
	 * @param os the output stream be written to
	 *
	 * @return \c true if the write operation succeeded, false otherwise
	 */
	template<typename T>
	static bool writeDMF(
			const Graph &graph,
			const EdgeArray<T> &weights,
			const node source,
			const node sink,
			std::ostream &os)
	{
		GraphAttributes attr(graph, getEdgeWeightFlag<T>());
		for (edge e : graph.edges) {
			getEdgeWeightAttribute<T>(attr, e) = weights[e];
		}
		return writeDMF(attr, source, sink, os);
	}

	//@}
	/**
	 * @name Graphs with subgraph
	 * These functions read and write graphs in a simple text-based file format that also specifies
	 * a subgraph (given as a list of edges).
	 */
	//@{

	//! Reads graph \p G with subgraph defined by \p delEdges from stream \p is.
	/**
	 * \sa writeEdgeListSubgraph(const Graph &G, const List<edge> &delEdges, std::ostream &os)
	 *
	 * @param G        is assigned the read graph.
	 * @param delEdges is assigned the edges of the subgraph.
	 * @param          is is the input stream from which the graph is read.
	 * \return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readEdgeListSubgraph(Graph &G, List<edge> &delEdges, std::istream &is);

	//! Writes graph \p G with subgraph defined by \p delEdges to stream \p os.
	/**
	 * \sa readEdgeListSubgraph(Graph &G, List<edge> &delEdges, std::istream &is)
	 *
	 * @param G        is the graph to be written.
	 * @param delEdges specifies the edges of the subgraph to be stored.
	 * @param os       is the output stream to which the graph will be written.
	 * \return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeEdgeListSubgraph(const Graph &G, const List<edge> &delEdges, std::ostream &os);

	//@}
	/**
	 * @name Graphics formats
	 * These functions draw graphs and export them as SVG (Scalable Vector Graphics) vectors graphics.
	 */
	//@{

	static OGDF_EXPORT bool drawSVG(const GraphAttributes &A, std::ostream &os, const SVGSettings &settings);
	static inline bool drawSVG(const GraphAttributes &A, std::ostream &os) {
		return drawSVG(A, os, svgSettings);
	}

	static OGDF_EXPORT bool drawSVG(const ClusterGraphAttributes &A, std::ostream &os, const SVGSettings &settings);
	static inline bool drawSVG(const ClusterGraphAttributes &A, std::ostream &os) {
		return drawSVG(A, os, svgSettings);
	}

	//@}
	/**
	 * @name Utility functions for indentation
	 * Text based write methods that use indentation for better readability of the produced text files
	 * apply a customizable indentation character and indentation width.
	 */
	//@{

	//! Returns the currently used indentation character.
	static char indentChar() { return s_indentChar; }

	//! Returns the currently used indentation width.
	static int indentWidth() { return s_indentWidth; }

	//! Sets the indentation character to \p c.
	/**
	 * \pre \p c must be a white-space character (e.g., a space or a tab).
	 */
	static void setIndentChar(char c) {
		OGDF_ASSERT(isspace((int)c));
		s_indentChar = c;
	}

	//! Sets the indentation width to \p w.
	/**
	 * \pre \p w must be non-negative.
	 * Setting the indentation width to 0 suppresses indentation.
	 */
	static void setIndentWidth(int w) {
		if(w >= 0) s_indentWidth = w;
	}

	//! Prints indentation for indentation \p depth to output stream \p os and returns \p os.
	static OGDF_EXPORT std::ostream &indent(std::ostream &os, int depth);

	//@}
	//! @name Other utility functions
	//@{

	//! Set a color value (R/G/B/A) based on an integer.
	//! Checks if the value is in the right range.
	static OGDF_EXPORT bool setColorValue(int value, std::function<void(uint8_t)> setFunction)
	{
		if (value < 0 || value > 255) {
			GraphIO::logger.lout() << "Error: color value is not between 0 and 255." << std::endl;
			return false;
		}
		setFunction(static_cast<uint8_t>(value));
		return true;
	}

	//! Returns GraphAttributes::edgeIntWeight.
	/**
	 * Helps with templated access to GraphAttributes edgeWeight type
	 */
	template<typename T, typename std::enable_if<std::is_integral<T>::value, bool>::type = 0>
	static int getEdgeWeightFlag() {
		return GraphAttributes::edgeIntWeight;
	}

	//! Returns GraphAttributes::edgeDoubleWeight.
	/**
	 * Helps with templated access to GraphAttributes edgeWeight type
	 */
	template<typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = 0>
	static int getEdgeWeightFlag() {
		return GraphAttributes::edgeDoubleWeight;
	}

	//! Returns a reference to the intWeight()-value of \p attr for \p e.
	/**
	 * \pre attribute edgeIntWeight must be enabled.
	 * Helps with templated access to GraphAttributes edgeWeight type by forwarding to
	 * the respective GraphAttributes::xxxWeight(edge e) function
	 */
	template<typename T, typename std::enable_if<std::is_integral<T>::value, bool>::type = 0>
	static int &getEdgeWeightAttribute(GraphAttributes &attr, edge e) {
		return attr.intWeight(e);
	}

	//! Returns a reference to the doubleWeight()-value of \p attr for \p e.
	/**
	 * \pre attribute edgeDoubleWeight must be enabled.
	 * Helps with templated access to GraphAttributes edgeWeight type by forwarding to
	 * the respective GraphAttributes::xxxWeight(edge e) function
	 */
	template<typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = 0>
	static double &getEdgeWeightAttribute(GraphAttributes &attr, edge e) {
		return attr.doubleWeight(e);
	}

	//@}

	static SVGSettings OGDF_EXPORT svgSettings;

private:
	static OGDF_EXPORT bool readGraph6WithForcedHeader(Graph &G, std::istream &is);
	static OGDF_EXPORT bool readDigraph6WithForcedHeader(Graph &G, std::istream &is);
	static OGDF_EXPORT bool readSparse6WithForcedHeader(Graph &G, std::istream &is);
	static OGDF_EXPORT char s_indentChar; //!< Character used for indentation.
	static OGDF_EXPORT int s_indentWidth; //!< Number of indent characters used for indentation.
};

}
