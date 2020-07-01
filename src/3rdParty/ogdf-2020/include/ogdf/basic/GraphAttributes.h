/** \file
 * \brief Declaration of class GraphAttributes which extends a Graph
 *        by additional attributes.
 *
 * \author Carsten Gutwenger
 *         Karsten Klein
 *         Joachim Kupke
 *         Sebastian Leipert
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

#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/basic/LayoutStandards.h>
#include <ogdf/basic/exceptions.h>


namespace ogdf {

//! Stores additional attributes of a graph (like layout information).
/**
 * @ingroup graph-drawing
 *
 * It is frequently necessary to associate additional attributes with a graph.
 * The class GraphAttributes provides various such attributes and is the
 * central place were such attributes are stored.
 *
 * Attributes are simply stored in node or edge arrays; for memory consumption
 * reasons, only a subset of these arrays is in fact initialized for the graph;
 * non-initialized arrays require only a few bytes of extra memory.
 *
 * Which arrays are initialized is specified by a bit vector; each bit in this
 * bit vector corresponds to one or more attributes.
 * See the available flags for a detailed description.
 *
 * Attributes can be enabled/disabled by the constructor #GraphAttributes(const Graph &,long),
 * the re-initialization procedure #init, #addAttributes, and #destroyAttributes.
 */

class OGDF_EXPORT GraphAttributes {

protected:
	const Graph *m_pGraph; //!< associated graph

	bool m_directed; //!< whether or not the graph is directed

	// graphical representation of nodes
	NodeArray<double>       m_x;				//!< x-coordinate of a node
	NodeArray<double>       m_y;				//!< y-coordinate of a node
	NodeArray<double>       m_z;				//!< z-coordinate of a node
	NodeArray<double>       m_nodeLabelPosX;		//!< x-coordinate of a node label
	NodeArray<double>       m_nodeLabelPosY;		//!< y-coordinate of a node label
	NodeArray<double>       m_nodeLabelPosZ;		//!< z-coordinate of a node label
	NodeArray<double>       m_width;			//!< width of a node's bounding box
	NodeArray<double>       m_height;			//!< height of a nodes's bounding box
	NodeArray<Shape>        m_nodeShape;		//!< shape of a node
	NodeArray<string>       m_nodeLabel;		//!< label of a node
	NodeArray<Stroke>       m_nodeStroke;		//!< stroke of a node
	NodeArray<Fill>         m_nodeFill;			//!< fill of a node
	NodeArray<string>       m_nodeTemplate;		//!< name of template of a node

	// other node attributes
	NodeArray<int>             m_nodeId;		//!< user ID of a node
	NodeArray<int>             m_nodeIntWeight;	//!< (integer) weight of a node
	NodeArray<Graph::NodeType> m_vType;			//!< type (vertex, dummy, generalizationMerger)

	// graphical representation of edges
	EdgeArray<DPolyline>       m_bends;			//!< list of bend points of an edge
	EdgeArray<string>          m_edgeLabel;		//!< label of an edge
	EdgeArray<EdgeArrow>       m_edgeArrow;		//!< arrow type of an edge
	EdgeArray<Stroke>          m_edgeStroke;	//!< stroke of an edge

	// other edge attributes
	EdgeArray<int>             m_intWeight;		//!< (integer) weight of an edge
	EdgeArray<double>          m_doubleWeight;	//!< (real number) weight of an edge
	EdgeArray<Graph::EdgeType> m_eType;			//!< type of an edge (association or generalization)
	EdgeArray<uint32_t>        m_subGraph;		//!< is element of subgraphs given by bitvector

	long m_attributes;	//!< bit vector of currently used attributes

public:

	/**
	 * @name Flags for enabling attributes.
	 */
	//!@{

	//! Corresponds to node attributes #x(node), #y(node), #width(node), #height(node), and #shape(node).
	static const long nodeGraphics;

	//! Corresponds to edge attribute #bends(edge).
	static const long edgeGraphics;

	//! Corresponds to edge attribute #intWeight(edge).
	static const long edgeIntWeight;

	//! Corresponds to edge attribute #doubleWeight(edge).
	static const long edgeDoubleWeight;

	//! Corresponds to edge attribute #label(edge).
	static const long edgeLabel;

	//! Corresponds to node attribute #label(node).
	static const long nodeLabel;

	//! Corresponds to edge attribute #type(edge).
	static const long edgeType;

	//! Corresponds to node attribute #type(node).
	static const long nodeType;

	//! Corresponds to node attribute #idNode(node).
	static const long nodeId;

	//! Corresponds to edge attribute #arrowType(edge).
	static const long edgeArrow;

	//! Corresponds to edge attributes #strokeColor(edge), #strokeType(edge), and #strokeWidth(edge).
	static const long edgeStyle;

	//! Corresponds to node attributes #strokeColor(node), #strokeType(node),
	//! #strokeWidth(node), #fillPattern(node), #fillColor(node), and #fillBgColor(node).
	static const long nodeStyle;

	//! Corresponds to node attribute #templateNode(node).
	static const long nodeTemplate;

	//! Corresponds to edge attributes modified by
	//! #addSubGraph(edge, int), #inSubGraph(edge, int) const, and #removeSubGraph(edge, int).
	static const long edgeSubGraphs;

	//! Corresponds to node attribute #weight(node).
	static const long nodeWeight;

	//! Corresponds to node attribute #z(node). Note that all methods work on 2D coordinates only.
	static const long threeD;

	//! Corresponds to node attributes #xLabel(node), #yLabel(node), and #zLabel(node).
	static const long nodeLabelPosition;

	//! Enables all available flags.
	static const long all;

	//!@}

	/**
	 * @name Construction and management of attributes
	 */
	//!@{

	//! Constructs graph attributes for no associated graph (default constructor).
	/**
	 * The associated graph can be set later with the init() function.
	 */
	GraphAttributes();

	//! Constructs graph attributes associated with the graph \p G.
	/**
	 * @param G is the associated graph.
	 * @param attr specifies the set of attributes that can be accessed.
	 */
	explicit GraphAttributes(const Graph &G, long attr = nodeGraphics | edgeGraphics);

	//! Copy constructor.
	GraphAttributes(const GraphAttributes&) = default;

	//! Copy assignment operator.
	GraphAttributes& operator=(const GraphAttributes&) = default;

	virtual ~GraphAttributes() {
	}

	//! Returns currently accessible attributes.
	long attributes() const {
		return m_attributes;
	}

	//! Returns true iff all attributes in \p attr are available.
	inline bool has(long attr) const {
		return (m_attributes & attr) == attr;
	}

	//! Initializes the graph attributes for graph \p G.
	/**
	 * @param G is the new associated graph.
	 * @param attr specifies the set of attributes that can be accessed.
	 *
	 * \warning All attributes that were allocated before are destroyed by this function!
	 *  If you wish to extend the set of allocated attributes, use #addAttributes.
	 */
	virtual void init(const Graph &G, long attr);

	//! Re-initializes the graph attributes while maintaining the associated graph.
	//! @see init(const Graph&, long)
	void init(long attr);

	//! Enables attributes specified by \p attr and allocates required memory.
	void addAttributes(long attr);

	//! Disables attributes specified by \p attr and releases available memory.
	void destroyAttributes(long attr);

	//! Returns a reference to the associated graph.
	const Graph& constGraph() const {
		return *m_pGraph;
	}

	//!@}
	/**
	 * @name General attributes
	 */
	//!@{

	//! Returns if the graph is directed.
	bool directed() const {
		return m_directed;
	}

	//! Returns if the graph is directed.
	bool &directed() {
		return m_directed;
	}

	//!@}
	/**
	 * @name Node attributes
	 */
	//!@{

	//! Returns the x-coordinate of node \p v.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	double x(node v) const {
		OGDF_ASSERT(has(nodeGraphics));
		return m_x[v];
	}


	//! Returns the x-coordinate of node \p v.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	double &x(node v) {
		OGDF_ASSERT(has(nodeGraphics));
		return m_x[v];
	}


	//! Returns the y-coordinate of node \p v.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	double y(node v) const {
		OGDF_ASSERT(has(nodeGraphics));
		return m_y[v];
	}


	//! Returns the y-coordinate of node \p v.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	double &y(node v) {
		OGDF_ASSERT(has(nodeGraphics));
		return m_y[v];
	}


	//! Returns the z-coordinate of node \p v.
	/**
	 * \pre #threeD is enabled
	 */
	double z(node v) const {
		OGDF_ASSERT(has(threeD));
		return m_z[v];
	}


	//! Returns the z-coordinate of node \p v.
	/**
	 * \pre #threeD is enabled
	 */
	double &z(node v) {
		OGDF_ASSERT(has(threeD));
		return m_z[v];
	}


	//! Returns the label x-coordinate of node \p v.
	/**
	 * \pre #nodeLabelPosition is enabled
	 */
	double xLabel(node v) const {
		OGDF_ASSERT(has(nodeLabelPosition));
		return m_nodeLabelPosX[v];
	}

	//! Returns the label x-coordinate of node \p v.
	/**
	 * \pre #nodeLabelPosition is enabled
	 */
	double &xLabel(node v) {
		OGDF_ASSERT(has(nodeLabelPosition));
		return m_nodeLabelPosX[v];
	}


	//! Returns the label y-coordinate of node \p v.
	/**
	 * \pre #nodeLabelPosition is enabled
	 */
	double yLabel(node v) const {
		OGDF_ASSERT(has(nodeLabelPosition));
		return m_nodeLabelPosY[v];
	}

	//! Returns the label y-coordinate of node \p v.
	/**
	 * \pre #nodeLabelPosition is enabled
	 */
	double &yLabel(node v) {
		OGDF_ASSERT(has(nodeLabelPosition));
		return m_nodeLabelPosY[v];
	}


	//! Returns the label z-coordinate of node \p v.
	/**
	 * \pre #nodeLabelPosition and #threeD are enabled
	 */
	double zLabel(node v) const {
		OGDF_ASSERT(has(nodeLabelPosition));
		OGDF_ASSERT(has(threeD));
		return m_nodeLabelPosZ[v];
	}

	//! Returns the label z-coordinate of node \p v.
	/**
	 * \pre #nodeLabelPosition and #threeD are enabled
	 */
	double &zLabel(node v) {
		OGDF_ASSERT(has(nodeLabelPosition));
		OGDF_ASSERT(has(threeD));
		return m_nodeLabelPosZ[v];
	}


	//! Returns the width of the bounding box of node \p v.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	double width(node v) const {
		OGDF_ASSERT(has(nodeGraphics));
		return m_width[v];
	}

	//! Returns the width of the bounding box of node \p v.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	double &width(node v) {
		OGDF_ASSERT(has(nodeGraphics));
		return m_width[v];
	}


	//! Returns a reference to the node array #m_width.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	const NodeArray<double> &width() const {
		OGDF_ASSERT(has(nodeGraphics));
		return m_width;
	}

	//! Returns a reference to the node array \#m_width.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	NodeArray<double> &width() {
		OGDF_ASSERT(has(nodeGraphics));
		return m_width;
	}


	//! Returns the height of the bounding box of node \p v.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	double height(node v) const {
		OGDF_ASSERT(has(nodeGraphics));
		return m_height[v];
	}

	//! Returns the height of the bounding box of node \p v.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	double &height(node v) {
		OGDF_ASSERT(has(nodeGraphics));
		return m_height[v];
	}


	//! Returns a reference to the node array #m_height.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	const NodeArray<double> &height() const {
		OGDF_ASSERT(has(nodeGraphics));
		return m_height;
	}

	//! Returns a reference to the node array #m_height.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	NodeArray<double> &height() {
		OGDF_ASSERT(has(nodeGraphics));
		return m_height;
	}


	//! Returns the shape type of node \p v.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	Shape shape(node v) const {
		OGDF_ASSERT(has(nodeGraphics));
		return m_nodeShape[v];
	}

	//! Returns the shape type of node \p v.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	Shape &shape(node v) {
		OGDF_ASSERT(has(nodeGraphics));
		return m_nodeShape[v];
	}


	//! Returns the stroke type of node \p v.
	/**
	 * \pre #nodeStyle is enabled
	 */
	StrokeType strokeType(node v) const {
		OGDF_ASSERT(has(nodeStyle));
		return m_nodeStroke[v].m_type;
	}

	//! Returns the stroke type of node \p v.
	/**
	 * \pre #nodeStyle is enabled
	 */
	StrokeType &strokeType(node v) {
		OGDF_ASSERT(has(nodeStyle));
		return m_nodeStroke[v].m_type;
	}


	//! Returns the stroke color of node \p v.
	/**
	 * \pre #nodeStyle is enabled
	 */
	const Color &strokeColor(node v) const {
		OGDF_ASSERT(has(nodeStyle));
		return m_nodeStroke[v].m_color;
	}

	//! Returns the stroke color of node \p v.
	/**
	 * \pre #nodeStyle is enabled
	 */
	Color &strokeColor(node v) {
		OGDF_ASSERT(has(nodeStyle));
		return m_nodeStroke[v].m_color;
	}


	//! Returns the stroke width of node \p v.
	/**
	 * \pre #nodeStyle is enabled
	 */
	float strokeWidth(node v) const {
		OGDF_ASSERT(has(nodeStyle));
		return m_nodeStroke[v].m_width;
	}

	//! Returns the stroke width of node \p v.
	/**
	 * \pre #nodeStyle is enabled
	 */
	float &strokeWidth(node v) {
		OGDF_ASSERT(has(nodeStyle));
		return m_nodeStroke[v].m_width;
	}

	//! Returns the fill pattern of node \p v.
	/**
	 * \pre #nodeStyle is enabled
	 */
	FillPattern fillPattern(node v) const {
		OGDF_ASSERT(has(nodeStyle));
		return m_nodeFill[v].m_pattern;
	}

	//! Returns the fill pattern of node \p v.
	/**
	 * \pre #nodeStyle is enabled
	 */
	FillPattern &fillPattern(node v) {
		OGDF_ASSERT(has(nodeStyle));
		return m_nodeFill[v].m_pattern;
	}

	//! Returns the fill color of node \p v.
	/**
	 * \pre #nodeStyle is enabled
	 */
	const Color &fillColor(node v) const {
		OGDF_ASSERT(has(nodeStyle));
		return m_nodeFill[v].m_color;
	}

	//! Returns the fill color of node \p v.
	/**
	 * \pre #nodeStyle is enabled
	 */
	Color &fillColor(node v) {
		OGDF_ASSERT(has(nodeStyle));
		return m_nodeFill[v].m_color;
	}


	//! Returns the background color of fill patterns for node \p v.
	/**
	 * \pre #nodeStyle is enabled
	 */
	const Color &fillBgColor(node v) const {
		OGDF_ASSERT(has(nodeStyle));
		return m_nodeFill[v].m_bgColor;
	}

	//! Returns the background color of fill patterns for node \p v.
	/**
	 * \pre #nodeStyle is enabled
	 */
	Color &fillBgColor(node v) {
		OGDF_ASSERT(has(nodeStyle));
		return m_nodeFill[v].m_bgColor;
	}


	//! Returns the label of node \p v.
	/**
	 * \pre #nodeLabel is enabled
	 */
	const string &label(node v) const {
		OGDF_ASSERT(has(nodeLabel));
		return m_nodeLabel[v];
	}

	//! Returns the label of node \p v.
	/**
	 * \pre #nodeLabel is enabled
	 */
	string &label(node v) {
		OGDF_ASSERT(has(nodeLabel));
		return m_nodeLabel[v];
	}


	//! Returns the template name of node \p v.
	/**
	 * \pre #nodeTemplate is enabled
	 */
	const string &templateNode(node v) const {
		OGDF_ASSERT(has(nodeTemplate));
		return m_nodeTemplate[v];
	}

	//! Returns the template name of node \p v.
	/**
	 * \pre #nodeTemplate is enabled
	 */
	string &templateNode(node v) {
		OGDF_ASSERT(has(nodeTemplate));
		return m_nodeTemplate[v];
	}


	//! Returns the weight of node \p v.
	/**
	 * \pre #nodeWeight is enabled
	 */
	int weight(node v) const {
		OGDF_ASSERT(has(nodeWeight));
		return m_nodeIntWeight[v];
	}

	//! Returns the weight of node \p v.
	/**
	 * \pre #nodeWeight is enabled
	 */
	int &weight(node v) {
		OGDF_ASSERT(has(nodeWeight));
		return m_nodeIntWeight[v];
	}


	//! Returns the type of node \p v.
	/**
	 * \pre #nodeType is enabled
	 */
	Graph::NodeType type(node v) const {
		OGDF_ASSERT(has(nodeType));
		return m_vType.valid() ? m_vType[v] : Graph::NodeType::vertex;
	}

	//! Returns the type of node \p v.
	/**
	 * \pre #nodeType is enabled
	 */
	Graph::NodeType &type(node v) {
		OGDF_ASSERT(has(nodeType));
		return m_vType[v];
	}


	//! Returns the user ID of node \p v.
	/**
	 * \pre #nodeId is enabled
	 */
	int idNode(node v) const {
		OGDF_ASSERT(has(nodeId));
		return m_nodeId[v];
	}

	//! Returns the user ID of node \p v.
	/**
	 * \pre #nodeId is enabled
	 */
	int &idNode(node v) {
		OGDF_ASSERT(has(nodeId));
		return m_nodeId[v];
	}

	//!@}
	/**
	 * @name Edge attributes
	 */
	//!@{


	//! Returns the list of bend points of edge \p e.
	/**
	 * Note that bend points should not be co-linear.
	 * This can always be achieved by calling DPolyline::normalize().
	 * Similarly, bend points should never include the point of the
	 * edge's source or target node, even though the poly-line for the entire edge formally includes them.
	 *
	 * \pre #edgeGraphics is enabled
	 */
	const DPolyline &bends(edge e) const {
		OGDF_ASSERT(has(edgeGraphics));
		return m_bends[e];
	}

	//! Returns the list of bend points of edge \p e.
	/**
	 * @see bends(edge e) const
	 *
	 * \pre #edgeGraphics is enabled
	 */
	DPolyline &bends(edge e) {
		OGDF_ASSERT(has(edgeGraphics));
		return m_bends[e];
	}


	//! Returns the arrow type of edge \p e.
	/**
	 * \pre #edgeArrow is enabled
	 */
	EdgeArrow arrowType(edge e) const {
		OGDF_ASSERT(has(edgeArrow));
		return m_edgeArrow[e];
	}

	//! Returns the arrow type of edge \p e.
	/**
	 * \pre #edgeArrow is enabled
	 */
	EdgeArrow &arrowType(edge e) {
		OGDF_ASSERT(has(edgeArrow));
		return m_edgeArrow[e];
	}


	//! Returns the stroke type of edge \p e.
	/**
	 * \pre #edgeStyle is enabled
	 */
	StrokeType strokeType(edge e) const {
		OGDF_ASSERT(has(edgeStyle));
		return m_edgeStroke[e].m_type;
	}

	//! Returns the stroke type of edge \p e.
	/**
	 * \pre #edgeStyle is enabled
	 */
	StrokeType &strokeType(edge e) {
		OGDF_ASSERT(has(edgeStyle));
		return m_edgeStroke[e].m_type;
	}


	//! Returns the stroke color of edge \p e.
	/**
	 * \pre #edgeStyle is enabled
	 */
	const Color &strokeColor(edge e) const {
		OGDF_ASSERT(has(edgeStyle));
		return m_edgeStroke[e].m_color;
	}

	//! Returns the stroke color of edge \p e.
	/**
	 * \pre #edgeStyle is enabled
	 */
	Color &strokeColor(edge e) {
		OGDF_ASSERT(has(edgeStyle));
		return m_edgeStroke[e].m_color;
	}


	//! Returns the stroke width of edge \p e.
	/**
	 * \pre #edgeStyle is enabled
	 */
	float strokeWidth(edge e) const {
		OGDF_ASSERT(has(edgeStyle));
		return m_edgeStroke[e].m_width;
	}

	//! Returns the stroke width of edge \p e.
	/**
	 * \pre #edgeStyle is enabled
	 */
	float &strokeWidth(edge e) {
		OGDF_ASSERT(has(edgeStyle));
		return m_edgeStroke[e].m_width;
	}


	//! Returns the label of edge \p e.
	/**
	 * \pre #edgeLabel is enabled
	 */
	const string &label(edge e) const {
		OGDF_ASSERT(has(edgeLabel));
		return m_edgeLabel[e];
	}

	//! Returns the label of edge \p e.
	/**
	 * \pre #edgeLabel is enabled
	 */
	string &label(edge e) {
		OGDF_ASSERT(has(edgeLabel));
		return m_edgeLabel[e];
	}


	//! Returns the (integer) weight of edge \p e.
	/**
	 * \pre #edgeIntWeight is enabled
	 */
	int intWeight(edge e) const {
		OGDF_ASSERT(has(edgeIntWeight));
		return m_intWeight[e];
	}

	//! Returns the (integer) weight of edge \p e.
	/**
	 * \pre #edgeIntWeight is enabled
	 */
	int &intWeight(edge e) {
		OGDF_ASSERT(has(edgeIntWeight));
		return m_intWeight[e];
	}


	//! Returns the (real number) weight of edge \p e.
	/**
	 * \pre #edgeDoubleWeight is enabled
	 */
	double doubleWeight(edge e) const {
		OGDF_ASSERT(has(edgeDoubleWeight));
		return m_doubleWeight[e];
	}

	//! Returns the (real number) weight of edge \p e.
	/**
	 * \pre #edgeDoubleWeight is enabled
	 */
	double &doubleWeight(edge e) {
		OGDF_ASSERT(has(edgeDoubleWeight));
		return m_doubleWeight[e];
	}


	//! Returns the type of edge \p e.
	/**
	 * \pre #edgeType is enabled
	 */
	Graph::EdgeType type(edge e) const {
		OGDF_ASSERT(has(edgeType));
		return m_eType.valid() ? m_eType[e] : Graph::EdgeType::association;
	}

	//! Returns the type of edge \p e.
	/**
	 * \pre #edgeType is enabled
	 */
	Graph::EdgeType &type(edge e) {
		OGDF_ASSERT(has(edgeType));
		return m_eType[e];
	}


	//! Returns the edgesubgraph value of an edge \p e.
	/**
	 * \pre #edgeSubGraphs is enabled
	 */
	uint32_t subGraphBits(edge e) const {
		OGDF_ASSERT(has(edgeSubGraphs));
		return m_subGraph[e];
	}

	//! Returns the edgesubgraph value of an edge \p e.
	/**
	 * \pre #edgeSubGraphs is enabled
	 */
	uint32_t &subGraphBits(edge e) {
		OGDF_ASSERT(has(edgeSubGraphs));
		return m_subGraph[e];
	}


	//! Checks whether edge \p e belongs to basic graph \p n.
	/**
	 * \pre #edgeSubGraphs is enabled
	 */
	bool inSubGraph(edge e, int n) const {
		OGDF_ASSERT(has(edgeSubGraphs));
		OGDF_ASSERT(n>=0);
		OGDF_ASSERT(n<32);
		return (m_subGraph[e] & (1 << n)) != 0;
	}


	//! Adds edge \p e to basic graph \p n.
	/**
	 * \pre #edgeSubGraphs is enabled
	 */
	void addSubGraph(edge e, int n) {
		OGDF_ASSERT(has(edgeSubGraphs));
		OGDF_ASSERT(n>=0);
		OGDF_ASSERT(n<32);
		m_subGraph[e] |= (1 << n);
	}


	//! Removes edge \p e from basic graph \p n.
	/**
	 * \pre #edgeSubGraphs is enabled
	 */
	void removeSubGraph(edge e, int n) {
		OGDF_ASSERT(has(edgeSubGraphs));
		OGDF_ASSERT(n>=0);
		OGDF_ASSERT(n<32);
		m_subGraph[e] &= ~(1 << n);
	}

	//!@}
	/**
	* @name Layout transformations
	*/
	//!@{

	//! Scales the layout by (\p sx,\p sy).
	/**
	 * If \p scaleNodes is true, node sizes are scaled as well.
	 *
	 * \param sx         is the scaling factor for x-coordinates.
	 * \param sy         is the scaling factor for y-coordinates.
	 * \param scaleNodes determines if nodes size are scaled as well (true) or not.
	 *
	 * \pre #nodeGraphics and #edgeGraphics are enabled
	 */
	virtual void scale(double sx, double sy, bool scaleNodes = true);

	//! Scales the layout by \p s.
	/**
	* If \p scaleNodes is true, node sizes are scaled as well.
	*
	* \param s          is the scaling factor for both x- and y-coordinates.
	* \param scaleNodes determines if nodes size are scaled as well (true) or not.
	*
	* \pre #nodeGraphics and #edgeGraphics are enabled
	*/
	virtual void scale(double s, bool scaleNodes = true) { scale(s, s, scaleNodes); }

	//! Translates the layout by (\p dx,\p dy).
	/**
	 * \param dx is the translation in x-direction.
	 * \param dy is the translation in y-direction.
	 */
	virtual void translate(double dx, double dy);

	//! Translates the layout such that the lower left corner is at (0,0).
	virtual void translateToNonNeg();

	//! Flips the layout vertically within its bounding box.
	/**
	 * If preserving the bounding box is not required, the layout can also be flipped vertically
	 * by calling <tt>scale(1.0, -1.0, false)</tt>.
	 */
	virtual void flipVertical() { flipVertical(boundingBox()); }

	//! Flips the (whole) layout vertically such that the part in \p box remains in this area.
	/**
	 * The whole layout is flipped and then moved such that the part that was in \p box before
	 * flipping is moved to this area.
	 */
	virtual void flipVertical(const DRect &box);

	//! Flips the layout horizontally within its bounding box.
	/**
	* If preserving the bounding box is not required, the layout can also be flipped horizontally
	* by calling <tt>scale(-1.0, 1.0, false)</tt>.
	*/
	virtual void flipHorizontal() { flipHorizontal(boundingBox()); }

	//! Flips the (whole) layout horizontally such that the part in \p box remains in this area.
	/**
	* The whole layout is flipped and then moved such that the part that was in \p box before
	* flipping is moved to this area.
	*/
	virtual void flipHorizontal(const DRect &box);

	//! Scales the layout by (\p sx,\p sy) and then translates it by (\p dx,\p dy).
	/**
	* If \p scaleNodes is true, node sizes are scaled as well. A point (\a x,\a y) is
	* moved to (\p sx &sdot; \a x + \p dx, \p sy &sdot; \a y + \p dy).
	*
	* \param sx         is the scaling factor for x-coordinates.
	* \param sy         is the scaling factor for y-coordinates.
	* \param dx         is the translation in x-direction.
	* \param dy         is the translation in y-direction.
	* \param scaleNodes determines if nodes size are scaled as well (true) or not.
	*/
	virtual void scaleAndTranslate(double sx, double sy, double dx, double dy, bool scaleNodes = true);

	//! Scales the layout by \p s and then translates it by (\p dx,\p dy).
	/**
	* If \p scaleNodes is true, node sizes are scaled as well. A point (\a x,\a y) is
	* moved to (\p s &sdot; \a x + \p dx, \p s &sdot; \a y + \p dy).
	*
	* \param s          is the scaling factor for both x- and y-coordinates.
	* \param dx         is the translation in x-direction.
	* \param dy         is the translation in y-direction.
	* \param scaleNodes determines if nodes size are scaled as well (true) or not.
	*/
	virtual void scaleAndTranslate(double s, double dx, double dy, bool scaleNodes = true) {
		scaleAndTranslate(s, s, dx, dy, scaleNodes);
	}

	//! Rotates the layout by 90 degree (in clockwise direction) around the origin.
	virtual void rotateRight90();

	//! Rotates the layout by 90 degree (in counter-clockwise direction) around the origin.
	virtual void rotateLeft90();

	//!@}
	/**
	 * @name Utility functions
	 */
	//!@{

	//! Returns a DPoint corresponding to the x- and y-coordinates of \p v.
	inline DPoint point(node v) const { return DPoint(m_x[v], m_y[v]); }

	//! Copies attributes of this to \p origAttr.
	/**
	 * Only attributes which are enabled in both this and \p origAttr are
	 * copied.
	 * The edges of \p origAttr get attributes associated with the respective
	 * first edge in the chain of copy edges. Both dummy nodes and bends between
	 * dummy nodes are added as bends to \p origAttr.
	 *
	 * @pre This GraphAttributes is associated with a GraphCopy, which is
	 * a copy of the graph that \p origAttr is associated with.
	 *
	 * @param origAttr is the GraphAttributes of the original graph.
	 */
	void transferToOriginal(GraphAttributes &origAttr) const;

	//! Copies attributes of this to \p copyAttr.
	/**
	 * Only attributes which are enabled in both this and \p copyAttr are
	 * copied.
	 * The edges of \p copyAttr get attributes associated with the respective
	 * original edge. Bend points, however, are only transferred to the first
	 * edge in the chain of copy edges. Bend points of all other copy edges in
	 * the chain are cleared.
	 *
	 * @pre \p copyAttr is associated with a GraphCopy, which is a copy of the
	 * graph that this GraphAttributes is associated with.
	 *
	 * @param copyAttr is the GraphAttributes of the GraphCopy.
	 */
	void transferToCopy(GraphAttributes &copyAttr) const;

	//! Returns the bounding box of the graph.
	/**
	 * \pre #nodeGraphics and #edgeGraphics is enabled
	 */
	virtual DRect boundingBox() const;

	//! Computes the bounding rectangle for each node.
	/**
	 * \tparam Rectangle is the kind of rectangle that should be created.
	 * \param boundingBoxes is assigned the bounding rectangle for each node.
	 * \pre #nodeGraphics is enabled
	 */
	template<class Rectangle = DRect>
	void nodeBoundingBoxes(NodeArray<Rectangle> &boundingBoxes) const {
		for (node v : constGraph().nodes) {
			double vHalfWidth  = width(v)  / 2.0;
			double vHalfHeight = height(v) / 2.0;
			boundingBoxes[v] = Rectangle(x(v) - vHalfWidth, y(v) - vHalfHeight,
			                             x(v) + vHalfWidth, y(v) + vHalfHeight);
		}
	}

	//! Sets the width of all nodes to \p w.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	void setAllWidth(double w);

	//! Sets the height of all nodes to \p h.
	/**
	 * \pre #nodeGraphics is enabled
	 */
	void setAllHeight(double h);

	//! Removes all edge bends.
	/**
	 * \pre #edgeGraphics is enabled
	 */
	void clearAllBends();

	//! Removes unnecessary bend points in orthogonal segements.
	/**
	 * Processes all edges and removes unnecessary bend points in the bend point list
	 * of the edge, i.e., bend points such that the preceding and succeeding bend point
	 * form a horizontal or vertical segement containing this bend point. This function
	 * is useful to remove redundant bend points in an orthogonal layout.
	 *
	 * \pre #edgeGraphics is enabled
	 *
	 */
	void removeUnnecessaryBendsHV();

	//! Adds additional bend points to all edges for connecting their endpoints.
	/**
	 * According to \p mode switch add either the node center points to
	 * the bends or the anchor point on the node boundary
	 *   - \p mode = 0: only add node center
	 *   - \p mode = 1: compute intersection with the line segment to the center
	 *     and the boundary of the rectangular node
	 *   - \p mode = 2: compute intersection with the first/last line segment
	 *     and the boundary of the rectangular node
	 *
	 * \pre #nodeGraphics and #edgeGraphics is enabled
	 */
	void addNodeCenter2Bends(int mode = 1);

	//! Returns true iff \p v represents an association class.
	/**
	 * We hide the internal representation of semantic node types from
	 * the user to be able to change this later (semantic node type member array).
	 * We are not allowed to set association classes manually, only by calling
	 * createAssociationClass().
	 *
	 * \pre #nodeGraphics are enabled
	 */
	bool isAssociationClass(node v) const {
		return type(v) == Graph::NodeType::associationClass;
	}

	//! Returns a list of all inheritance hierarchies in the graph.
	/**
	 * Inheritance hierarchies are identified by edges with type Graph::generalization.
	 *
	 * @param list is a list of all hierarchies; each hierarchy is itself a list
	 *        of all nodes in this hierarchy.
	 *
	 * \return Returns the number of generalization hierarchies.
	 */
	int hierarchyList(List<List<node>*> &list) const;

	//! Returns a list of all inheritance hierarchies in the graph.
	/**
	 * Inheritance hierarchies are identified by edges with type Graph::generalization.
	 *
	 * @param list is a list of all hierarchies; each hierarchy is itself a list
	 *        of all edges in this hierarchy.
	 *
	 * \return Returns the number of generalization hierarchies.
	 */
	int hierarchyList(List<List<edge>*> &list) const;

	//!@}

private:
	//! Copies all attributes \p attrs of \p vFrom to \p toAttr for \p vTo.
	void copyNodeAttributes(GraphAttributes &toAttr, node vFrom, node vTo, long attrs) const;

	//! Copies all attributes \p attrs except bends (!) of \p eFrom to \p toAttr for \p eTo.
	void copyEdgeAttributes(GraphAttributes &toAttr, edge eFrom, edge eTo, long attrs) const;
};

}
