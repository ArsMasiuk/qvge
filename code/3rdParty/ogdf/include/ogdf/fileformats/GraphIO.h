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
 * @ingroup file-system graphs graph-drawing
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

	//! Reads graph \p G of arbitrary graph format from \p filename.
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
	 *  - OGML
	 *  - SteinLib
	 *  - Graph6 (with enforced header)
	 *
	 * @param G        is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool read(Graph &G, std::istream &is);

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

#pragma mark GML

	//@}
	/**
	 * @name GML
	 *
	 * %Graph Modelling Language: https://en.wikipedia.org/wiki/Graph_Modelling_Language
	 */
	//@{

	//! Reads graph \p G in GML format from file \p filename.
	/**
	 * \sa readGML(Graph &G, std::istream &is) for more details.<br>
	 *     writeGML(const Graph &G, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGML(Graph &G, const string &filename);

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

	//! Writes graph \p G in GML format to file \p filename.
	/**
	 * \sa writeGML(const Graph &G, std::ostream &os) for more details.<br>
	 *     readGML(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGML(const Graph &G, const string &filename);

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

	//! Reads clustered graph (\p C, \p G) in GML format from file \p filename.
	/**
	 * \pre \p G is the graph associated with clustered graph \p C.
	 * \sa readGML(ClusterGraph &C, Graph &G, std::istream &is) for more details.<br>
	 *     writeGML(const ClusterGraph &C, const string &filename)
	 *
	 * @param C        is assigned the read clustered graph (cluster structure).
	 * @param G        is assigned the read clustered graph (graph structure).
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGML(ClusterGraph &C, Graph &G, const string &filename);

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

	//! Writes clustered graph \p C in GML format to file \p filename.
	/**
	 * \sa writeGML(const ClusterGraph &C, std::ostream &os) for more details.<br>
	 *     readGML(ClusterGraph &C, Graph &G, const string &filename)
	 *
	 * @param C        is the clustered graph to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGML(const ClusterGraph &C, const string &filename);

	//! Writes clustered graph \p C in GML format to output stream \p os.
	/**
	 * \sa readGML(ClusterGraph &C, Graph &G, std::istream &is)
	 *
	 * @param C   is the clustered graph to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGML(const ClusterGraph &C, std::ostream &os);

	//! Reads clustered graph (\p C, \p G) with attributes \p A in GML format from file \p filename.
	/**
	 * \pre \p C is the clustered graph associated with attributes \p A, and \p G is the graph associated with \p C.
	 * \sa readGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is) for more details.<br>
	 *     writeGML(const ClusterGraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param C        is assigned the read clustered graph (cluster structure).
	 * @param G        is assigned the read clustered graph (graph structure).
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename);

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

	//! Writes graph with attributes \p A in GML format to file \p filename.
	/**
	 * \sa writeGML(const ClusterGraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename)
	 *
	 * @param A        specifies the clustered graph and its attributes to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGML(const ClusterGraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in GML format to output stream \p os.
	/**
	 * \sa readGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
	 *
	 * @param A   specifies the clustered graph and its attributes to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGML(const ClusterGraphAttributes &A, std::ostream &os);

	//! Reads graph \p G with attributes \p A in GML format from file \p filename.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa readGML(GraphAttributes &A, Graph &G, std::istream &is) for more details.<br>
	 *     writeGML(const GraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGML(GraphAttributes &A, Graph &G, const string &filename);

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

	//! Writes graph with attributes \p A in GML format to file \p filename.
	/**
	 * \sa writeGML(const GraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readGML(GraphAttributes &A, Graph &G, const string &filename)
	 *
	 * @param A        specifies the graph and its attributes to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGML(const GraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in GML format to output stream \p os.
	/**
	 * \sa readGML(GraphAttributes &A, Graph &G, std::istream &is)
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGML(const GraphAttributes &A, std::ostream &os);

#pragma mark OGML

	//@}
	/**
	 * @name OGML
	 *
	 * Open %Graph Markup Language: http://www.ogdf.net/lib/exe/fetch.php/documentation.pdf
	 */
	//@{

	//! Reads graph \p G in OGML format from file \p filename.
	/**
	 * \sa readOGML(Graph &G, std::istream &is) for more details.<br>
	 *     writeOGML(const Graph &G, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readOGML(Graph &G, const string &filename);

	//! Reads graph \p G in OGML format from input stream \p is.
	/**
	 * \sa writeOGML(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readOGML(Graph &G, std::istream &is);

	//! Writes graph \p G in OGML format to file \p filename.
	/**
	 * \sa writeOGML(const Graph &G, std::ostream &os) for more details.<br>
	 *     readOGML(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeOGML(const Graph &G, const string &filename);

	//! Writes graph \p G in OGML format to output stream \p os.
	/**
	 * \sa bool readOGML(Graph &G, std::istream &is)
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeOGML(const Graph &G, std::ostream &os);

	//! Reads clustered graph (\p C, \p G) in OGML format from file \p filename.
	/**
	 * \pre \p G is the graph associated with clustered graph \p C.
	 * \sa readOGML(ClusterGraph &C, Graph &G, std::istream &is) for more details.<br>
	 *     writeOGML(const ClusterGraph &C, const string &filename)
	 *
	 * @param C        is assigned the read clustered graph (cluster structure).
	 * @param G        is assigned the read clustered graph (graph structure).
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readOGML(ClusterGraph &C, Graph &G, const string &filename);

	//! Reads clustered graph (\p C, \p G) in OGML format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with clustered graph \p C.
	 * \sa writeOGML(const ClusterGraph &C, std::ostream &os)
	 *
	 * @param C   is assigned the read clustered graph (cluster structure).
	 * @param G   is assigned the read clustered graph (graph structure).
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readOGML(ClusterGraph &C, Graph &G, std::istream &is);

	//! Writes clustered graph \p C in OGML format to file \p filename.
	/**
	 * \sa writeOGML(const ClusterGraph &C, std::ostream &os) for more details.<br>
	 *     readOGML(ClusterGraph &C, Graph &G, const string &filename)
	 *
	 * @param C        is the clustered graph to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeOGML(const ClusterGraph &C, const string &filename);

	//! Writes clustered graph \p C in OGML format to output stream \p os.
	/**
	 * \sa readOGML(ClusterGraph &C, Graph &G, std::istream &is)
	 *
	 * @param C   is the clustered graph to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeOGML(const ClusterGraph &C, std::ostream &os);

	//! Reads graph \p G with attributes \p A in OGML format from file \p filename.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa readOGML(GraphAttributes &A, Graph &G, std::istream &is) for more details.<br>
	 *     writeOGML(const GraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readOGML(GraphAttributes &A, Graph &G, const string &filename);

	//! Reads graph \p G with attributes \p A in OGML format from input stream \p is.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa writeOGML(const GraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readOGML(GraphAttributes &A, Graph &G, std::istream &is);

	//! Writes graph with attributes \p A in OGML format to file \p filename.
	/**
	 * \sa writeOGML(const GraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readOGML(GraphAttributes &A, Graph &G, const string &filename)
	 *
	 * @param A        specifies the graph and its attributes to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeOGML(const GraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in OGML format to output stream \p os.
	/**
	 * \sa readOGML(GraphAttributes &A, Graph &G, std::istream &is)
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeOGML(const GraphAttributes &A, std::ostream &os);

	//! Reads clustered graph (\p C, \p G) with attributes \p A in OGML format from file \p filename.
	/**
	 * \pre \p C is the clustered graph associated with attributes \p A, and \p G is the graph associated with \p C.
	 * \sa readOGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is) for more details.<br>
	 *     writeOGML(const ClusterGraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param C        is assigned the read clustered graph (cluster structure).
	 * @param G        is assigned the read clustered graph (graph structure).
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readOGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename);

	//! Reads clustered graph (\p C, \p G) with attributes \p A in OGML format from input stream \p is.
	/**
	 * \pre \p C is the clustered graph associated with attributes \p A, and \p G is the graph associated with \p C.
	 * \sa writeOGML(const ClusterGraphAttributes &A, std::ostream &os)
	 *
	 * @param A   is assigned the graph's attributes.
	 * @param C   is assigned the read clustered graph (cluster structure).
	 * @param G   is assigned the read clustered graph (graph structure).
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readOGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is);

	//! Writes graph with attributes \p A in OGML format to file \p filename.
	/**
	 * \sa writeOGML(const ClusterGraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readOGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename)
	 *
	 * @param A        specifies the clustered graph and its attributes to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeOGML(const ClusterGraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in OGML format to output stream \p os.
	/**
	 * \sa readOGML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is)
	 *
	 * @param A   specifies the clustered graph and its attributes to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeOGML(const ClusterGraphAttributes &A, std::ostream &os);

#pragma mark Rome

	//@}
	/**
	 * @name Rome
	 *
	 * Rome-Lib format: http://www.graphdrawing.org/data/
	 */
	//@{

	//! Reads graph \p G in Rome-Lib format from file \p filename.
	/**
	 * \sa readRome(Graph &G, std::istream &is) for more details.<br>
	 *     writeRome(const Graph &G, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readRome(Graph &G, const string &filename);

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

	//! Writes graph \p G in Rome-Lib format to file \p filename.
	/**
	 * \sa writeRome(const Graph &G, std::ostream &os) for more details.<br>
	 *     readRome(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeRome(const Graph &G, const string &filename);

	//! Writes graph \p G in Rome-Lib format to output stream \p os.
	/**
	 * \sa readRome(Graph &G, std::istream &is) for more details about the format.
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeRome(const Graph &G, std::ostream &os);

#pragma mark LEDA

	//@}
	/**
	 * @name LEDA
	 *
	 * LEDA Native File Format for Graphs: http://www.algorithmic-solutions.info/leda_guide/graphs/leda_native_graph_fileformat.html
	 */
	//@{

	//! Reads graph \p G in LEDA graph format from file \p filename.
	/**
	 * \sa readLEDA(Graph &G, std::istream &is) for more details.<br>
	 *     writeLEDA(const Graph &G, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readLEDA(Graph &G, const string &filename);

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

	//! Writes graph \p G in LEDA graph format to file \p filename.
	/**
	 * \sa writeLEDA(const Graph &G, std::ostream &os) for more details.<br>
	 *     readLEDA(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeLEDA(const Graph &G, const string &filename);

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

#pragma mark Chaco

	//@}
	/**
	 * @name Chaco
	 *
	 * https://cfwebprod.sandia.gov/cfdocs/CompResearch/docs/guide.pdf
	 */
	//@{

	//! Reads graph \p G in Chaco format from file \p filename.
	/**
	 * \sa readChaco(Graph &G, std::istream &is) for more details.<br>
	 *     writeChaco(const Graph &G, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readChaco(Graph &G, const string &filename);

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

	//! Writes graph \p G in Chaco format to file \p filename.
	/**
	 * \sa writeChaco(const Graph &G, std::ostream &os) for more details.<br>
	 *     readChaco(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeChaco(const Graph &G, const string &filename);

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

#pragma mark PMDissGraph

	//@}
	/**
	 * @name PMDissGraph
	 *
	 * %Graph file format from [Petra Mutzel, The maximum planar subgraph problem, PhD Thesis, Köln University, 1994]
	 */
	//@{

	//! Reads graph \p G in a simple format as used in Petra Mutzel's thesis from file \p filename.
	/**
	 * \sa readPMDissGraph(Graph &G, std::istream &is) for more details.<br>
	 *     writePMDissGraph(const Graph &G, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readPMDissGraph(Graph &G, const string &filename);

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

	//! Writes graph \p G in a simple format as used in Petra Mutzel's thesis to file \p filename.
	/**
	 * \sa writePMDissGraph(const Graph &G, std::ostream &os) for more details.<br>
	 *     readPMDissGraph(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writePMDissGraph(const Graph &G, const string &filename);

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

#pragma mark YGraph

	//@}
	/**
	 * @name YGraph
	 *
	 * http://www3.cs.stonybrook.edu/~algorith/implement/nauty/distrib/makebg.c
	 */
	//@{

	//! Reads graph \p G in Y-graph format from file \p filename.
	/**
	 * \sa readYGraph(Graph &G, std::istream &is) for more details.
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readYGraph(Graph &G, const string &filename);

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

#pragma mark Graph6

	//@}
	/**
	 * @name Graph6
	 *
	 * The Graph6 format represents an (preferable dense or small)
	 * simple undirected graph as a string containing printable characters
	 * between 0x3F and 0x7E.
	 * <a href="http://cs.anu.edu.au/~bdm/data/formats.txt">See the specification for more information.</a>
	 */
	//@{

	//! Reads graph \p G in Graph6 format from file \p filename.
	/**
	 * \sa readGraph6(Graph &G, std::istream &is) for more details.<br>
	 *     writeGraph6(const Graph &G, const string &filename)
	 *
	 * @param G           is assigned the read graph.
	 * @param filename    is the name of the file to be read.
	 * @param forceHeader if the file has to start with '>>graph6<<'.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGraph6(Graph &G, const string &filename, bool forceHeader = false);

	//! Reads graph \p G in Graph6 format from input stream \p is.
	/**
	 * The Graph6 format represents an (preferable dense or small)
	 * simple undirected graph as a string containing printable characters
	 * between 0x3F and 0x7E.
	 * <a href="http://cs.anu.edu.au/~bdm/data/formats.txt">See the specification for more information.</a>
	 *
	 * \sa writeGraph6(const Graph &G, std::ostream &os)
	 *
	 * @param G           is assigned the read graph.
	 * @param is          is the input stream to be read.
	 * @param forceHeader if the file has to start with '>>graph6<<'.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGraph6(Graph &G, std::istream &is, bool forceHeader = false);

	//! @cond

	// the cond above suppresses doxygen for this method
	// readGraph6WithForcedHeader equals readGraph6(...,...,true)
	static OGDF_EXPORT bool readGraph6WithForcedHeader(Graph &G, std::istream &is);

	//! @endcond

	//! Writes graph \p G in Graph6 format to file \p filename.
	/**
	 * \sa writeGraph6(const Graph &G, std::ostream &os) for more details.<br>
	 *     readGraph6(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGraph6(const Graph &G, const string &filename);

	//! Writes graph \p G in Graph6 format to output stream \p os.
	/**
	 * \sa readGraph6(Graph &G, std::istream &is)
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGraph6(const Graph &G, std::ostream &os);

#pragma mark MatrixMarket

	//@}
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

	//! Reads graph \p G in Matrix Market exchange format from a file with the given \p filename
	/**
	 * @param G        is assigned the read graph.
	 * @param filename is the path to the input file to read from
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readMatrixMarket(Graph& G, const string &filename);

#pragma mark Rudy

	//@}
	/**
	 * @name Rudy
	 */
	//@{

	//! Reads graph \p G with edge weights stored in \p A in Rudy format from file \p filename.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa readRudy(GraphAttributes &A, Graph &G, std::istream &is) for more details.<br>
	 *     writeRudy(const GraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes (only edge weights (as doubles) are used).
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readRudy(GraphAttributes &A, Graph &G, const string &filename);

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

	//! Writes graph with edge weights stored in \p A in Rudy format to file \p filename.
	/**
	 * \sa writeRudy(const GraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readRudy(GraphAttributes &A, Graph &G, const string &filename)
	 *
	 * @param A        specifies the graph and its attributes to be written (only edge weights
	 *                 (as doubles) are stored in this format).
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeRudy(const GraphAttributes &A, const string &filename);

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

#pragma mark BENCH

	//@}
	/**
	 * @name BENCH
	 */
	//@{

	//!  Reads a hypergraph (as point-based expansion) in BENCH format from file \p filename.
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
	 * @param filename   is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readBENCH(Graph &G, List<node>& hypernodes, List<edge> *shell, const string &filename);

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

#pragma mark PLA

	//@}
	/**
	 * @name PLA
	 */
	//@{

	//! Reads a hypergraph (as point-based expansion) in PLA format from file \p filename.
	/**
	 * @param G          is assigned the read graph (point-based expansion of the hypergraph).
	 * @param hypernodes is assigned the list of nodes which have to be interpreted as hypernodes.
	 * @param shell      if 0 only the PLA-hypergraph is read. Otherwise we extend the graph
	 *                   by a simple edge <i>e=(i,o)</i> and two hyperedges: one hyperedges groups all input nodes and
	 *                   <i>i</i> together, the other hyperedge groups all output edges and <i>o</i>.
	 *                   These additional edges are then also collocated in shell.
	 * @param filename   is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 *
	 * \sa readPLA(Graph &G, List<node>& hypernodes, List<edge> *shell, std::istream &is) for details
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readPLA(Graph &G, List<node>& hypernodes, List<edge>* shell, const string &filename);

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

#pragma mark ChallengeGraph

	//@}
	/**
	 * @name GD-Challenge
	 *
	 * %Graph %Drawing %Challenge: %Area %Minimization for %Orthogonal %Grid %Layouts
	 * http://graphdrawing.de/contest2013/challenge.html
	 */
	//@{

	//! Reads graph \p G with grid layout \p gl in GD-Challenge-format from file \p filename.
	/**
	 * \pre \p G is the graph associated with grid layout \p gl.
	 * \sa writeChallengeGraph(const Graph &G, const GridLayout &gl, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param gl       is assigned the grid layout.
	 * @param filename is the name of the file to be read.
	 * \return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readChallengeGraph(Graph &G, GridLayout &gl, const string &filename);

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

	//! Writes graph \p G with grid layout \p gl in GD-Challenge-format to file \p filename.
	/**
	 * \pre \p G is the graph associated with grid layout \p gl.
	 * \sa readChallengeGraph(Graph &G, GridLayout &gl, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param gl       specifies the grid layout of \p G to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * \return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeChallengeGraph(const Graph &G, const GridLayout &gl, const string &filename);

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

#pragma mark GraphML

	//@}
	/**
	 * @name GraphML
	 *
	 * %Graph %Markup %Language: http://graphml.graphdrawing.org/
	 */
	//@{

	//! Reads graph \p G in GraphML format from file \p filename.
	/**
	 * \sa readGraphML(Graph &G, std::istream &is) for more details.<br>
	 *     writeGraphML(const Graph &G, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGraphML(Graph &G, const string &filename);

	//! Reads graph \p G in GraphML format from input stream \p is.
	/**
	 * \sa writeGraphML(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGraphML(Graph &G, std::istream &is);

	//! Reads clustered graph (\p C, \p G) in GraphML format from file \p filename.
	/**
	 * \pre \p G is the graph associated with clustered graph \p C.
	 * \sa readGraphML(ClusterGraph &C, Graph &G, std::istream &is) for more details.<br>
	 *     writeGraphML(const ClusterGraph &C, const string &filename)
	 *
	 * @param C        is assigned the read clustered graph (cluster structure).
	 * @param G        is assigned the read clustered graph (graph structure).
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGraphML(ClusterGraph &C, Graph &G, const string &filename);

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

	//! Reads graph \p G with attributes \p A in GraphML format from file \p filename.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa readGraphML(GraphAttributes &A, Graph &G, std::istream &is) for more details.<br>
	 *     writeGraphML(const GraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGraphML(GraphAttributes &A, Graph &G, const string &filename);

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

	//! Reads clustered graph (\p C, \p G) with attributes \p A in GraphML format from file \p filename.
	/**
	 * \pre \p C is the clustered graph associated with attributes \p A, and \p G is the graph associated with \p C.
	 * \sa readGraphML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is) for more details.<br>
	 *     writeGraphML(const ClusterGraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param C        is assigned the read clustered graph (cluster structure).
	 * @param G        is assigned the read clustered graph (graph structure).
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGraphML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename);

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

	//! Writes graph \p G in GraphML format to file \p filename.
	/**
	 * \sa writeGraphML(const Graph &G, std::ostream &os) for more details.<br>
	 *     readGraphML(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGraphML(const Graph &G, const string &filename);

	//! Writes graph \p G in GraphML format to output stream \p os.
	/**
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGraphML(const Graph &G, std::ostream &os);

	//! Writes clustered graph \p C in GraphML format to file \p filename.
	/**
	 * \sa writeGraphML(const ClusterGraph &C, std::ostream &os) for more details.<br>
	 *     readGraphML(ClusterGraph &C, Graph &G, const string &filename)
	 *
	 * @param C        is the clustered graph to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGraphML(const ClusterGraph &C, const string &filename);

	//! Writes clustered graph \p C in GraphML format to output stream \p os.
	/**
	 *
	 * @param C   is the clustered graph to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGraphML(const ClusterGraph &C, std::ostream &os);

	//! Writes graph with attributes \p A in GraphML format to file \p filename.
	/**
	 * \sa writeGraphML(const GraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readGraphML(GraphAttributes &A, Graph &G, const string &filename)
	 *
	 * @param A        specifies the graph and its attributes to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGraphML(const GraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in GraphML format to output stream \p os.
	/**
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGraphML(const GraphAttributes &A, std::ostream &os);

	//! Writes graph with attributes \p A in GraphML format to file \p filename.
	/**
	 * \sa writeGraphML(const ClusterGraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readGraphML(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename)
	 *
	 * @param A        specifies the clustered graph and its attributes to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGraphML(const ClusterGraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in GraphML format to output stream \p os.
	/**
	 *
	 * @param A   specifies the clustered graph and its attributes to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGraphML(const ClusterGraphAttributes &A, std::ostream &os);

#pragma mark DOT

	//@}
	/**
	 * @name DOT
	 *
	 * http://www.graphviz.org/doc/info/lang.html
	 */
	//@{

	//! Reads graph \p G in DOT format from file \p filename.
	/**
	 * \sa readDOT(Graph &G, std::istream &is) for more details.<br>
	 *     writeDOT(const Graph &G, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readDOT(Graph &G, const string &filename);

	//! Reads graph \p G in DOT format from input stream \p is.
	/**
	 * \sa writeDOT(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readDOT(Graph &G, std::istream &is);

	//! Reads clustered graph (\p C, \p G) in DOT format from file \p filename.
	/**
	 * \pre \p G is the graph associated with clustered graph \p C.
	 * \sa readDOT(ClusterGraph &C, Graph &G, std::istream &is) for more details.<br>
	 *     writeDOT(const ClusterGraph &C, const string &filename)
	 *
	 * @param C        is assigned the read clustered graph (cluster structure).
	 * @param G        is assigned the read clustered graph (graph structure).
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readDOT(ClusterGraph &C, Graph &G, const string &filename);

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

	//! Reads graph \p G with attributes \p A in DOT format from file \p filename.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa readDOT(GraphAttributes &A, Graph &G, std::istream &is) for more details.<br>
	 *     writeDOT(const GraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readDOT(GraphAttributes &A, Graph &G, const string &filename);

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

	//! Reads clustered graph (\p C, \p G) with attributes \p A in DOT format from file \p filename.
	/**
	 * \pre \p C is the clustered graph associated with attributes \p A, and \p G is the graph associated with \p C.
	 * \sa readDOT(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is) for more details.<br>
	 *     writeDOT(const ClusterGraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param C        is assigned the read clustered graph (cluster structure).
	 * @param G        is assigned the read clustered graph (graph structure).
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readDOT(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename);

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

	//! Writes graph \p G in DOT format to file \p filename.
	/**
	 * \sa writeDOT(const Graph &G, std::ostream &os) for more details.<br>
	 *     readDOT(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeDOT(const Graph &G, const string &filename);

	//! Writes graph \p G in DOT format to output stream \p os.
	/**
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDOT(const Graph &G, std::ostream &os);

	//! Writes clustered graph \p C in DOT format to file \p filename.
	/**
	 * \sa writeDOT(const ClusterGraph &C, std::ostream &os) for more details.<br>
	 *     readDOT(ClusterGraph &C, Graph &G, const string &filename)
	 *
	 * @param C        is the clustered graph to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeDOT(const ClusterGraph &C, const string &filename);

	//! Writes clustered graph \p C in DOT format to output stream \p os.
	/**
	 *
	 * @param C   is the clustered graph to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDOT(const ClusterGraph &C, std::ostream &os);

	//! Writes graph with attributes \p A in DOT format to file \p filename.
	/**
	 * \sa writeDOT(const GraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readDOT(GraphAttributes &A, Graph &G, const string &filename)
	 *
	 * @param A        specifies the graph and its attributes to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeDOT(const GraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in DOT format to output stream \p os.
	/**
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDOT(const GraphAttributes &A, std::ostream &os);

	//! Writes graph with attributes \p A in DOT format to file \p filename.
	/**
	 * \sa writeDOT(const ClusterGraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readDOT(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename)
	 *
	 * @param A        specifies the clustered graph and its attributes to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeDOT(const ClusterGraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in DOT format to output stream \p os.
	/**
	 *
	 * @param A   specifies the clustered graph and its attributes to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDOT(const ClusterGraphAttributes &A, std::ostream &os);

#pragma mark GEXF

	//@}
	/**
	 * @name GEXF
	 *
	 * %Graph %Exchange %XML %Format: https://gephi.org/gexf/format/
	 */
	//@{

	//! Reads graph \p G in GEXF format from file \p filename.
	/**
	 * \sa readGEXF(Graph &G, std::istream &is) for more details.<br>
	 *     writeGEXF(const Graph &G, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGEXF(Graph &G, const string &filename);

	//! Reads graph \p G in GEXF format from input stream \p is.
	/**
	 * \sa writeGEXF(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGEXF(Graph &G, std::istream &is);

	//! Reads clustered graph (\p C, \p G) in GEXF format from file \p filename.
	/**
	 * \pre \p G is the graph associated with clustered graph \p C.
	 * \sa readGEXF(ClusterGraph &C, Graph &G, std::istream &is) for more details.<br>
	 *     writeGEXF(const ClusterGraph &C, const string &filename)
	 *
	 * @param C        is assigned the read clustered graph (cluster structure).
	 * @param G        is assigned the read clustered graph (graph structure).
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGEXF(ClusterGraph &C, Graph &G, const string &filename);

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

	//! Reads graph \p G with attributes \p A in GEXF format from file \p filename.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa readGEXF(GraphAttributes &A, Graph &G, std::istream &is) for more details.<br>
	 *     writeGEXF(const GraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGEXF(GraphAttributes &A, Graph &G, const string &filename);

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

	//! Reads clustered graph (\p C, \p G) with attributes \p A in GEXF format from file \p filename.
	/**
	 * \pre \p C is the clustered graph associated with attributes \p A, and \p G is the graph associated with \p C.
	 * \sa readGEXF(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is) for more details.<br>
	 *     writeGEXF(const ClusterGraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param C        is assigned the read clustered graph (cluster structure).
	 * @param G        is assigned the read clustered graph (graph structure).
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGEXF(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename);

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

	//! Writes graph \p G in GEXF format to file \p filename.
	/**
	 * \sa writeGEXF(const Graph &G, std::ostream &os) for more details.<br>
	 *     readGEXF(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGEXF(const Graph &G, const string &filename);

	//! Writes graph \p G in GEXF format to output stream \p os.
	/**
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGEXF(const Graph &G, std::ostream &os);

	//! Writes clustered graph \p C in GEXF format to file \p filename.
	/**
	 * \sa writeGEXF(const ClusterGraph &C, std::ostream &os) for more details.<br>
	 *     readGEXF(ClusterGraph &C, Graph &G, const string &filename)
	 *
	 * @param C        is the clustered graph to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGEXF(const ClusterGraph &C, const string &filename);

	//! Writes clustered graph \p C in GEXF format to output stream \p os.
	/**
	 *
	 * @param C   is the clustered graph to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGEXF(const ClusterGraph &C, std::ostream &os);

	//! Writes graph with attributes \p A in GEXF format to file \p filename.
	/**
	 * \sa writeGEXF(const GraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readGEXF(GraphAttributes &A, Graph &G, const string &filename)
	 *
	 * @param A        specifies the graph and its attributes to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGEXF(const GraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in GEXF format to output stream \p os.
	/**
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGEXF(const GraphAttributes &A, std::ostream &os);

	//! Writes graph with attributes \p A in GEXF format to file \p filename.
	/**
	 * \sa writeGEXF(const ClusterGraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readGEXF(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename)
	 *
	 * @param A        specifies the clustered graph and its attributes to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGEXF(const ClusterGraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in GEXF format to output stream \p os.
	/**
	 *
	 * @param A   specifies the clustered graph and its attributes to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGEXF(const ClusterGraphAttributes &A, std::ostream &os);

#pragma mark GDF

	//@}
	/**
	 * @name GDF
	 *
	 * %GUESS %Database %File: http://graphexploration.cond.org/manual.html
	 */
	//@{

	//! Reads graph \p G in GDF format from file \p filename.
	/**
	 * \sa readGDF(Graph &G, std::istream &is) for more details.<br>
	 *     writeGDF(const Graph &G, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGDF(Graph &G, const string &filename);

	//! Reads graph \p G in GDF format from input stream \p is.
	/**
	 * \sa writeGDF(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readGDF(Graph &G, std::istream &is);

	//! Reads graph \p G with attributes \p A in GDF format from file \p filename.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa readGDF(GraphAttributes &A, Graph &G, std::istream &is) for more details.<br>
	 *     writeGDF(const GraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readGDF(GraphAttributes &A, Graph &G, const string &filename);

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

	//! Writes graph \p G in GDF format to file \p filename.
	/**
	 * \sa writeGDF(const Graph &G, std::ostream &os) for more details.<br>
	 *     readGDF(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGDF(const Graph &G, const string &filename);

	//! Writes graph \p G in GDF format to output stream \p os.
	/**
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGDF(const Graph &G, std::ostream &os);

	//! Writes graph with attributes \p A in GDF format to file \p filename.
	/**
	 * \sa writeGDF(const GraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readGDF(GraphAttributes &A, Graph &G, const string &filename)
	 *
	 * @param A        specifies the graph and its attributes to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeGDF(const GraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in GDF format to output stream \p os.
	/**
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeGDF(const GraphAttributes &A, std::ostream &os);

#pragma mark TLP

	//@}
	/**
	 * @name TLP
	 *
	 * Tulip software graph format: http://tulip.labri.fr/TulipDrupal/?q=tlp-file-format
	 */
	//@{

	//! Reads graph \p G in TLP format from file \p filename.
	/**
	 * \sa readTLP(Graph &G, std::istream &is) for more details.<br>
	 *     writeTLP(const Graph &G, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readTLP(Graph &G, const string &filename);

	//! Reads graph \p G in TLP format from input stream \p is.
	/**
	 * \sa writeTLP(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readTLP(Graph &G, std::istream &is);

	//! Reads clustered graph (\p C, \p G) in TLP format from file \p filename.
	/**
	 * \pre \p G is the graph associated with clustered graph \p C.
	 * \sa readTLP(ClusterGraph &C, Graph &G, std::istream &is) for more details.<br>
	 *     writeTLP(const ClusterGraph &C, const string &filename)
	 *
	 * @param C        is assigned the read clustered graph (cluster structure).
	 * @param G        is assigned the read clustered graph (graph structure).
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readTLP(ClusterGraph &C, Graph &G, const string &filename);

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

	//! Reads graph \p G with attributes \p A in TLP format from file \p filename.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa readTLP(GraphAttributes &A, Graph &G, std::istream &is) for more details.<br>
	 *     writeTLP(const GraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readTLP(GraphAttributes &A, Graph &G, const string &filename);

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

	//! Reads clustered graph (\p C, \p G) with attributes \p A in TLP format from file \p filename.
	/**
	 * \pre \p C is the clustered graph associated with attributes \p A, and \p G is the graph associated with \p C.
	 * \sa readTLP(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, std::istream &is) for more details.<br>
	 *     writeTLP(const ClusterGraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param C        is assigned the read clustered graph (cluster structure).
	 * @param G        is assigned the read clustered graph (graph structure).
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readTLP(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename);

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

	//! Writes graph \p G in TLP format to file \p filename.
	/**
	 * \sa writeTLP(const Graph &G, std::ostream &os) for more details.<br>
	 *     readTLP(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeTLP(const Graph &G, const string &filename);

	//! Writes graph \p G in TLP format to output stream \p os.
	/**
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeTLP(const Graph &G, std::ostream &os);

	//! Writes clustered graph \p C in TLP format to file \p filename.
	/**
	 * \sa writeTLP(const ClusterGraph &C, std::ostream &os) for more details.<br>
	 *     readTLP(ClusterGraph &C, Graph &G, const string &filename)
	 *
	 * @param C        is the clustered graph to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeTLP(const ClusterGraph &C, const string &filename);

	//! Writes clustered graph \p C in TLP format to output stream \p os.
	/**
	 *
	 * @param C   is the clustered graph to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeTLP(const ClusterGraph &C, std::ostream &os);

	//! Writes graph with attributes \p A in TLP format to file \p filename.
	/**
	 * \sa writeTLP(const GraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readTLP(GraphAttributes &A, Graph &G, const string &filename)
	 *
	 * @param A        specifies the graph and its attributes to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeTLP(const GraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in TLP format to output stream \p os.
	/**
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeTLP(const GraphAttributes &A, std::ostream &os);

	//! Writes graph with attributes \p A in TLP format to file \p filename.
	/**
	 * \sa writeTLP(const ClusterGraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readTLP(ClusterGraphAttributes &A, ClusterGraph &C, Graph &G, const string &filename)
	 *
	 * @param A        specifies the clustered graph and its attributes to be written.
	 * @param filename is the name of the file to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeTLP(const ClusterGraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in TLP format to output stream \p os.
	/**
	 *
	 * @param A   specifies the clustered graph and its attributes to be written.
	 * @param os  is the output stream to which the clustered graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeTLP(const ClusterGraphAttributes &A, std::ostream &os);

#pragma mark DL

	//@}
	/**
	 * @name DL
	 *
	 * %UCINET %DL format: https://sites.google.com/site/ucinetsoftware/document/ucinethelp.htm
	 */
	//@{

	//! Reads graph \p G in DL format from file \p filename.
	/**
	 * \sa readDL(Graph &G, std::istream &is) for more details.<br>
	 *     writeDL(const Graph &G, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readDL(Graph &G, const string &filename);

	//! Reads graph \p G in DL format from input stream \p is.
	/**
	 * \sa writeDL(const Graph &G, std::ostream &os)
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readDL(Graph &G, std::istream &is);

	//! Reads graph \p G with attributes \p A in DL format from file \p filename.
	/**
	 * \pre \p G is the graph associated with attributes \p A.
	 * \sa readDL(GraphAttributes &A, Graph &G, std::istream &is) for more details.<br>
	 *     writeDL(const GraphAttributes &A, const string &filename)
	 *
	 * @param A        is assigned the graph's attributes.
	 * @param G        is assigned the read graph.
	 * @param filename is the name of the file to be read.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readDL(GraphAttributes &A, Graph &G, const string &filename);

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

	//! Writes graph \p G in DL format to file \p filename.
	/**
	 * \sa writeDL(const Graph &G, std::ostream &os) for more details.<br>
	 *     readDL(Graph &G, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeDL(const Graph &G, const string &filename);

	//! Writes graph \p G in DL format to output stream \p os.
	/**
	 *
	 * @param G   is the graph to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDL(const Graph &G, std::ostream &os);

	//! Writes graph with attributes \p A in DL format to file \p filename.
	/**
	 * \sa writeDL(const GraphAttributes &A, std::ostream &os) for more details.<br>
	 *     readDL(GraphAttributes &A, Graph &G, const string &filename)
	 *
	 * @param A        specifies the graph and its attributes to be written.
	 * @param filename is the name of the file to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeDL(const GraphAttributes &A, const string &filename);

	//! Writes graph with attributes \p A in DL format to output stream \p os.
	/**
	 *
	 * @param A   specifies the graph and its attributes to be written.
	 * @param os  is the output stream to which the graph will be written.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool writeDL(const GraphAttributes &A, std::ostream &os);

	//@}
	/**
	 * @name SteinLib instances
	 * These functions read SteinLib instances stored in STP format and convert them into a weighted graph (represented by
	 * an EdgeWeightedGraph) and a list of terminal nodes.
	 */
	//@{

#pragma mark STP

	//@}
	/**
	 * @name STP
	 *
	 * %SteinLib %STP %Data %Format: http://steinlib.zib.de/format.php
	 */
	//@{

	/**
	 * Reads a graph in SteinLib format from std::istream \p is.
	 * \warning The coordinate section, weights and terminals of the SteinLib instance is not read!
	 *
	 * @param G   is assigned the read graph.
	 * @param is  is the input stream to be read.
	 * @return true if successful, false otherwise.
	 */
	static OGDF_EXPORT bool readSTP(Graph &G, std::istream &is) {
		// ignore everything except for the unweighted graph itself
		EdgeWeightedGraph<int> wG;
		List<node> terminals;
		NodeArray<bool> isTerminal;
		bool res = readSTP(wG, terminals, isTerminal, is);
		G = Graph(wG);
		return res;
	}

	/**
	 * Reads a SteinLib instance from file \a filename and converts it into a weighted graph \a wG and a set of terminal nodes \a terminals.
	 * \warning The coordinate section of the SteinLib instance is not read!
	 *
	 * @param wG the edge weighteed graph
	 * @param terminals will contain a list of all terminals in the graph
	 * @param isTerminal maps whether each node is a terminal or a Steiner node
	 * @param filename the name of the file to be read
	 *
	 * @return true if the STP was read successfully, false otherwise
	 */
	template<typename T>
	OGDF_DEPRECATED("Streams should be used instead.")
	static bool readSTP(
		EdgeWeightedGraph<T> &wG,
		List<node>           &terminals,
		NodeArray<bool>      &isTerminal,
		const string         &filename)
	{
		std::ifstream is(filename);
		return readSTP(wG, terminals, isTerminal, is);
	}

	/**
	 * Reads a SteinLib instance from an inputstream \p is and converts it into a weighted graph \p wG and a set of terminal nodes \p terminals.
	 * \warning The coordinate section of the SteinLib instance is not read!
	 *
	 * @param wG the edge weighteed graph
	 * @param terminals will contain a list of all terminals in the graph
	 * @param isTerminal maps whether each node is a terminal or a Steiner node
	 * @param is the input stream to be read
	 *
	 * @return true if the STP was read successfully, false otherwise
	 */
	template<typename T>
	static bool readSTP(
		EdgeWeightedGraph<T> &wG,
		List<node>           &terminals,
		NodeArray<bool>      &isTerminal,
		std::istream              &is)
	{
		wG.clear();
		terminals.clear();
		isTerminal.init(wG, false);
		int expectedNumberOfTerminals = -1;
		int expectedNumberOfEdges = -1;

		string buffer;

		enum class Section {
			None,
			Comment,
			Graph,
			Terminals,
			Ignore, // mostly: not implemented
		} section = Section::None;

		string key, value;

		Array<node> indexToNode;
#if 0
		node root; // root terminal (directed case)
#endif

		double version;

		// 1. line = identifier; ignore that it must be all on the first line
		std::vector<string> firstline{"33D32945", "STP", "File,", "STP", "Format", "Version"};
		for (int i = 0; i < 6; ++i) {
			is >> buffer;
			if (!is.good() || !equalIgnoreCase(buffer, firstline[i])) {
				logger.lout() << "Could not parse first line." << std::endl;
				return false;
			}
		}
		is >> version;
		if (!is.good() || version != 1.0) {
			logger.lout() << "Encountered unknwon STP format version." << std::endl;
			return false;
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
					}
					else
					if (equalIgnoreCase(what, "Graph")) {
						if (wG.numberOfNodes() != 0) {
							logger.lout(Logger::Level::Minor) << "Encountered duplicate graph section.";
							section = Section::Ignore;
						} else {
							section = Section::Graph;
						}
					}
					else
					if (equalIgnoreCase(what, "Terminals")) {
						if (!terminals.empty()) {
							logger.lout(Logger::Level::Minor) << "Encountered duplicate terminal section.";
							section = Section::Ignore;
						} else {
							section = Section::Terminals;
						}
					}
					else {
						section = Section::Ignore;
					}

					if (!iss.eof()) {
						iss >> what;
						if (equalIgnoreCase(what, "FROM")) {
							// loading not implemented, just ignore and end section
							section = Section::None;
						}
					}
				}
				else
				if (equalIgnoreCase(buffer, "EOF")) {
					if(expectedNumberOfTerminals != -1 && expectedNumberOfTerminals != terminals.size()) {
						logger.lout(Logger::Level::Minor)
						  << "Invalid number of terminals. Was " << terminals.size()
						  << " but expected " << expectedNumberOfTerminals << "." << std::endl;
					}

					if(expectedNumberOfEdges != -1 && expectedNumberOfEdges != wG.numberOfEdges()) {
						logger.lout(Logger::Level::Minor)
						  <<  "Invalid number of edges. Was " << wG.numberOfEdges()
						  << " but expected " << expectedNumberOfEdges << "." << std::endl;
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
						indexToNode[i] = wG.newNode();
					}
				} else if(equalIgnoreCase(key, "Edges") || equalIgnoreCase(key, "Arcs")) {
					iss >> expectedNumberOfEdges;
				} else if (equalIgnoreCase(key, "E")
				 || equalIgnoreCase(key, "A")) {
					int source = -1, target = -1;
					T weight = -1;
					iss >> source >> target >> weight;
					if (source <= 0 || source > wG.numberOfNodes()
					 || target <= 0 || target > wG.numberOfNodes()
					 || weight < 0) {
						logger.lout() << "Invalid edge given: " << source << "->" << target << "(weight: " << weight << ")" << std::endl;
						return false;
					}
					wG.newEdge(indexToNode[source], indexToNode[target], weight);
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
					if (v <= 0 || v > wG.numberOfNodes()) {
						logger.lout() << "Invalid terminal encountered: " << v << std::endl;
						return false;
					}
					terminals.pushBack(indexToNode[v]);
					isTerminal[indexToNode[v]] = true;
				} else if(!equalIgnoreCase(key, "Root")) {
					logger.lout(Logger::Level::Minor) << "Invalid terminal key encountered: " << key << std::endl;
				}
				break;
			}
		}
		logger.lout() << "Unexpected end of file." << std::endl;
		return false;
	}

	/**
	 * Writes an Steiner problem instance to an STP file.
	 *
	 * @param wG the edge weighteed graph
	 * @param terminals a list of all terminals in the graph
	 * @param filename the file to be written to
	 * @param comments a string containing all comments seperated by LF
	 *   if this is an empty string no comment section will be created
	 *
	 * @return true if the write operation succeeded, false otherwise
	 */
	template<typename T>
	OGDF_DEPRECATED("Streams should be used instead.")
	static bool writeSTP(
		const EdgeWeightedGraph<T> &wG,
		const List<node>           &terminals,
		const string               &filename,
		const string               &comments = "")
	{
		std::ofstream os(filename);
		return writeSTP(wG, terminals, os, comments);
	}

	/**
	 * Writes an Steiner problem instance to an STP file.
	 *
	 * @param wG the edge weighteed graph
	 * @param terminals a list of all terminals in the graph
	 * @param os the output stream be written to
	 * @param comments a string containing all comments seperated by LF
	 *   if this is an empty string no comment section will be created
	 *
	 * @return true if the write operation succeeded, false otherwise
	 */
	template<typename T>
	static bool writeSTP(
		const EdgeWeightedGraph<T> &wG,
		const List<node>           &terminals,
		std::ostream                    &os,
		const string               &comments = "")
	{
		if(!os.good()) return false;

		os << "33d32945 STP File, STP Format Version  1.00" << std::endl;

		os << std::endl << "Section Comment" << std::endl;
		if (comments.length() != 0) {
			os << comments << std::endl;
		}
		os << "End" << std::endl;

		os << std::endl << "Section Graph" << std::endl;
		os << "Nodes " << wG.numberOfNodes() << std::endl;
		os << "Edges " << wG.numberOfEdges() << std::endl;

		NodeArray<int> nodeToIndex(wG);
		int i = 1;
		for (node v : wG.nodes) {
			nodeToIndex[v] = i++;
		}
		for (edge e : wG.edges) {
			os << "E " << nodeToIndex[e->source()]
			   << " " << nodeToIndex[e->target()]
			   << " " << std::fixed << wG.weight(e) << std::endl;
		}
		os << "End" << std::endl
		   << std::endl
		   << "Section Terminals" << std::endl
		   << "Terminals " << terminals.size() << std::endl;
		for (node v : terminals) {
			os << "T " << nodeToIndex[v] << std::endl;
		}
		os << "End" << std::endl
		   << std::endl
		   << "EOF" << std::endl;

		return true;
	}



#pragma mark DMF

	//@}
	/**
	 * @name DMF
	 *
	 * DIMACS Max Flow Challenge: ftp://dimacs.rutgers.edu/pub/netflow/
	 */
	//@{

	/**
	 * Reads a maximum flow instance in DIMACS format.
	 *
	 * @param graph will contain the parsed graph
	 * @param weights will contain the weights of the edges
	 * @param source will contain the flow source
	 * @param sink will contain the flow sink
	 * @param filename name of the file to be read
	 * @return \c true iff the instances was parsed successfully
	 */
	template<typename T>
	OGDF_DEPRECATED("Streams should be used instead.")
	static bool readDMF(
			Graph &graph,
			EdgeArray<T> &weights,
			node &source,
			node &sink,
			const string &filename) {
		std::ifstream is(filename);
		return readDMF(graph, weights, source, sink, is);
	}

	/**
	 * Reads a maximum flow instance in DIMACS format.
	 *
	 * @param graph will contain the parsed graph
	 * @param weights will contain the weights of the edges
	 * @param source will contain the flow source
	 * @param sink will contain the flow sink
	 * @param is input stream
	 * @return \c true iff the instances was parsed successfully
	 */
	template<typename T>
	static bool readDMF(
			Graph &graph,
			EdgeArray<T> &weights,
			node &source,
			node &sink,
			std::istream &is)
	{
		int expectedNumberOfEdges = -1;
		List<node> nodes;
		graph.clear();
		weights.init(graph, 0);
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
						logger.lout() << "Ambigious problem definition encountered." << std::endl;
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
					T cap = -1;

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
					weights(e) = cap;
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

	/**
	 * Writes a maximum flow problem instance to a DIMACS maximum flow file.
	 *
	 * @param graph graph to be written
	 * @param weights will contain the weights of the edges
	 * @param source source of the maximum flow
	 * @param sink sink of the maximum flow
	 * @param filename name of the file to be written to
	 *
	 * @return \c true if the write operation succeeded, false otherwise
	 */
	template<typename T>
	OGDF_DEPRECATED("Streams should be used instead.")
	static bool writeDMF(
			const Graph &graph,
			const EdgeArray<T> &weights,
			const node source,
			const node sink,
			const string &filename)
	{
		std::ofstream os(filename);
		return writeDMF(graph, weights, source, sink, os);
	}

	/**
	 * Writes a maximum flow problem instance to a DIMACS maximum flow file.
	 *
	 * @param graph graph to be written
	 * @param weights will contain the weights of the edges
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
		if(!os.good()) return false;

		NodeArray<int> nodeIndices(graph);

		int counter = 0;
		for(node v : graph.nodes) {
			nodeIndices[v] = ++counter;
		}

		os << "p max " << graph.numberOfNodes() << " "  << graph.numberOfEdges() << std::endl;
		os << "n " << nodeIndices[source] << " s" << std::endl;
		os << "n " << nodeIndices[sink] << " t" << std::endl;

		for(edge e : graph.edges) {
			os << "a " << nodeIndices[e->source()] << " " << nodeIndices[e->target()] << " " << weights(e) << std::endl;
		}

		return true;
	}

	//@}
	/**
	 * @name Graphs with subgraph
	 * These functions read and write graphs in a simple text-based file format that also specifies
	 * a subgraph (given as a list of edges).
	 */
	//@{

	//! Reads graph \p G with subgraph defined by \p delEdges from file \p filename.
	/**
	 * \sa writeEdgeListSubgraph(const Graph &G, const List<edge> &delEdges, const string &filename)
	 *
	 * @param G        is assigned the read graph.
	 * @param delEdges is assigned the edges of the subgraph.
	 * @param filename is the name of the file to be read.
	 * \return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool readEdgeListSubgraph(Graph &G, List<edge> &delEdges, const string &filename);

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

	//! Writes graph \p G with subgraph defined by \p delEdges to file \p filename.
	/**
	 * \sa readEdgeListSubgraph(Graph &G, List<edge> &delEdges, const string &filename)
	 *
	 * @param G        is the graph to be written.
	 * @param delEdges specifies the edges of the subgraph to be stored.
	 * @param filename is the name of the file to which the graph will be written.
	 * \return true if successful, false otherwise.
	 */
	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool writeEdgeListSubgraph(const Graph &G, const List<edge> &delEdges, const string &filename);

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

	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool drawSVG(const GraphAttributes &A, const string &filename, const SVGSettings &settings = svgSettings);
	static OGDF_EXPORT bool drawSVG(const GraphAttributes &A, std::ostream &os, const SVGSettings &settings);
	static inline bool drawSVG(const GraphAttributes &A, std::ostream &os) {
		return drawSVG(A, os, svgSettings);
	}

	OGDF_DEPRECATED("Streams should be used instead.")
	static OGDF_EXPORT bool drawSVG(const ClusterGraphAttributes &A, const string &filename, const SVGSettings &settings = svgSettings);
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

	//! @}
	//! @name Other utility functions
	//! @{

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

	//! @}

	static SVGSettings OGDF_EXPORT svgSettings;

private:
	static OGDF_EXPORT char s_indentChar;	//!< Character used for indentation.
	static OGDF_EXPORT int  s_indentWidth;	//!< Number of indent characters used for indentation.
};

}
