/** \file
 * \brief Declaration of a base class for planar representations
 *        of graphs and cluster graphs.
 *
 * \author Karsten Klein, Carsten Gutwenger
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


// PlanRep should not know about generalizations and association,
// but we already set types in Attributedgraph, therefore set them
// in PlanRep, too

#pragma once

#include <ogdf/basic/GraphCopy.h>
#include <ogdf/planarity/EdgeTypePatterns.h>
#include <ogdf/planarity/NodeTypePatterns.h>
#include <ogdf/basic/Layout.h>
#include <ogdf/basic/GridLayout.h>


namespace ogdf {

class OrthoRep;


//! Planarized representations (of a connected component) of a graph.
/**
 * @ingroup plan-rep
 *
 * Maintains types of edges (generalization, association) and nodes,
 * and the connected components of the graph.
 */
class OGDF_EXPORT PlanRep : public GraphCopy
{
public:
	//! Information for restoring degree-1 nodes.
	struct Deg1RestoreInfo
	{
		Deg1RestoreInfo() : m_eOriginal(nullptr), m_deg1Original(nullptr), m_adjRef(nullptr) { }
		Deg1RestoreInfo(edge eOrig, node deg1Orig, adjEntry adjRef)
			: m_eOriginal(eOrig), m_deg1Original(deg1Orig), m_adjRef(adjRef) { }

		edge m_eOriginal;    //!< the original edge leading to the deg-1 node
		node m_deg1Original; //!< the original deg-1 node
		adjEntry m_adjRef;   //!< the reference adjacency entry for restoring the edge
	};


	//@{
	//! Creates a planarized representation of graph \p G.
	explicit PlanRep(const Graph& G);

	//! Creates a planarized representation of graph \p AG.
	explicit PlanRep(const GraphAttributes& AG);

	virtual ~PlanRep() { }

	//@}

	/**
	 * @name Processing connected components
	 * Planarized representations provide a mechanism for always representing
	 * a copy of a single component of the original graph.
	 */
	//@{

	//! Returns the number of connected components in the original graph.
	int numberOfCCs() const {
		return m_ccInfo.numberOfCCs();
	}

	//! Returns the index of the current connected component (-1 if not yet initialized).
	int currentCC() const {
		return m_currentCC;
	}

	//! Returns the connected components info structure.
	const CCsInfo &ccInfo() const { return m_ccInfo; }

	//! Returns the number of nodes in the current connected component.
	int numberOfNodesInCC() const { return numberOfNodesInCC(m_currentCC); }

	//! Returns the number of nodes in connected component \p cc.
	int numberOfNodesInCC(int cc) const {
		return stopNode(cc) - startNode(cc);
	}

	//! Returns node \p i in the list of all original nodes.
	node v(int i) const { return m_ccInfo.v(i); }

	//! Returns edge \p i in the list of all original edges.
	edge e(int i) const { return m_ccInfo.e(i); }

	//! Returns the index of the first node in this connected component.
	int startNode() const { return m_ccInfo.startNode(m_currentCC); }

	//! Returns the index of the first node in connected component \p cc.
	int startNode(int cc) const { return m_ccInfo.startNode(cc); }

	//! Returns the index of (one past) the last node in this connected component.
	int stopNode() const { return m_ccInfo.stopNode(m_currentCC); }

	//! Returns the index of (one past) the last node in connected component \p cc.
	int stopNode(int cc) const { return m_ccInfo.stopNode(cc); }

	//! Returns the index of the first edge in this connected component.
	int startEdge() const { return m_ccInfo.startEdge(m_currentCC); }

	//! Returns the index of (one past) the last edge in this connected component.
	int stopEdge() const { return m_ccInfo.stopEdge(m_currentCC); }

	//! Initializes the planarized representation for connected component \p cc.
	/**
	 * This initialization is always required. After performing this initialization,
	 * the planarized representation represents a copy of the connected
	 * component \p cc of the original graph, where connected components are numbered
	 * 0,1,2,...
	 */
	void initCC(int cc);

	//@}

	/**
	 * @name Node expansion
	 */
	//@{

	/**
	 * \brief Returns the adjacency entry of a node of an expanded face.
	 *
	 * If no such entry is stored at node \p v, 0 is returned.
	 */
	adjEntry expandAdj(node v) const {
		return m_expandAdj[v];
	}

	adjEntry& expandAdj(node v) {
		return m_expandAdj[v];
	}

	//@}
	/**
	 * @name Clique boundary
	 */
	//@{

	/**
	 * Returns the adjacency entry of the first edge of the inserted boundary
	 * at a center node (original) of a clique, 0 if no boundary exists
	 */
	adjEntry boundaryAdj(node v) const {
		return m_boundaryAdj[v];
	}

	/**
	 * Returns a reference to the adjacency entry of the first edge of the inserted boundary
	 * at a center node (original) of a clique, 0 if no boundary exists
	 */
	adjEntry& boundaryAdj(node v){
		return m_boundaryAdj[v];
	}

	//edge on the clique boundary, adjSource
	void setCliqueBoundary(edge e) {
		setEdgeTypeOf(e, edgeTypeOf(e) | cliquePattern());
	}
	bool isCliqueBoundary(edge e) const {
		return (edgeTypeOf(e) & cliquePattern()) == cliquePattern();
	}


	//@}
	/**
	 * @name Node types
	 */
	//@{

	/**
	 * \brief Returns the type of node \p v.
	 * @param v is a node in the planarized representation.
	 */
	Graph::NodeType typeOf(node v) const {
		return m_vType[v];
	}

	/**
	 * \brief Returns a reference to the type of node \p v.
	 * @param v is a node in the planarized representation.
	 */
	Graph::NodeType& typeOf(node v) {
		return m_vType[v];
	}

	/**
	 * \brief Returns true if the node represents a "real" object in the original graph.
	 *
	 * \todo It is necessary to check for several different possible types.
	 * This should be solved by combining representation types (vertex, dummy,...)
	 * with semantic types (class, interface,...) within GraphAttributes;
	 * we then can return to vertex only.
	 */
	inline bool isVertex(node v) const
	{
		return typeOf(v) == Graph::NodeType::vertex
		    || typeOf(v) == Graph::NodeType::associationClass;
	}

	/**
	 * \brief Returns the extended node type of \p v.
	 * @param v is a node in the planarized representation.
	 */
	nodeType nodeTypeOf(node v)
	{
		return m_nodeTypes[v];
	}

	/**
	 * \brief Classifies node \p v as a crossing.
	 * @param v is a node in the planarized representation.
	 */
	void setCrossingType(node v)
	{
		m_nodeTypes[v] |= UMLNodeTypeConstants::TerCrossing << UMLNodeTypeOffsets::Tertiary;
	}

	/**
	 * \brief Returns true iff node \p v is classified as a crossing.
	 * @param v is a node in the planarized representation.
	 */
	bool isCrossingType(node v) const
	{
		return (m_nodeTypes[v] & (UMLNodeTypeConstants::TerCrossing << UMLNodeTypeOffsets::Tertiary)) != 0;
	}

	//@}
	/**
	 * @name Edge types
	 */
	//@{

	/**
	 * \brief Returns the type of edge \p e.
	 * @param e is an edge in the planarized representation.
	 */
	EdgeType typeOf(edge e) const {
		return m_eType[e];
	}

	/**
	 * \brief Returns a reference to the type of edge \p e.
	 * @param e is an edge in the planarized representation.
	 */
	EdgeType& typeOf(edge e) {
		return m_eType[e];
	}

	/**
	 * \brief Returns a reference to the type of original edge \p e.
	 * @param e is an edge in the original graph.
	 */
	edgeType& oriEdgeTypes(edge e)
	{
		return m_oriEdgeTypes[e];
	}

	/**
	 * \brief Returns the new type field of \p e.
	 * @param e is an edge in the planarized representation.
	 */
	edgeType edgeTypeOf(edge e) const
	{
		return m_edgeTypes[e];
	}

	/**
	 * \brief Returns a reference to the new type field of \p e.
	 * @param e is an edge in the planarized representation.
	 */
	edgeType& edgeTypes(edge e)
	{
		return m_edgeTypes[e];
	}

	/**
	 * \brief Sets the new type field of edge \p e to \p et.
	 * @param e is an edge in the planarized representation.
	 * @param et is the type assigned to \p e.
	 */
	void setEdgeTypeOf(edge e, edgeType et)
	{
		m_edgeTypes[e] = et;
	}

	/**
	 * \brief Set both type values of \p e at once.
	 *
	 * This is a temporary solution that sets both type values; this way, all
	 * additional edge types in the new field are lost.
	 * @param e is an edge in the planarized representation.
	 * @param et is the type assigned to \p e.
	 */
	void setType(edge e, EdgeType et)
	{
		m_eType[e] = et;
		switch (et)
		{
			case Graph::EdgeType::association: m_edgeTypes[e] = static_cast<edgeType>(UMLEdgeTypeConstants::PrimAssociation);break;
			case Graph::EdgeType::generalization: m_edgeTypes[e] = static_cast<edgeType>(UMLEdgeTypeConstants::PrimGeneralization);
				break;
			case Graph::EdgeType::dependency: m_edgeTypes[e] = static_cast<edgeType>(UMLEdgeTypeConstants::PrimDependency); break;
			default: break;
		}
	}

	// new edge types
	// to set or check edge types use the pattern function in the private section

	// primary level types

	//! Returns true iff edge \p e is classified as generalization.
	bool isGeneralization(edge e) const {
		bool check = (((m_edgeTypes[e] & UMLEdgeTypePatterns::Primary) & UMLEdgeTypeConstants::PrimGeneralization) == UMLEdgeTypeConstants::PrimGeneralization);
		return check;
	}

	//! Classifies edge \p e as generalization (primary type).
	void setGeneralization(edge e) {
		setPrimaryType(e, UMLEdgeTypeConstants::PrimGeneralization);

		//preliminary set old array too
		m_eType[e] = EdgeType::generalization; //can be removed if edgetypes work properly
	}

	//! Returns true iff edge \p e is classified as dependency.
	bool isDependency(edge e) const {
		bool check = (((m_edgeTypes[e] & UMLEdgeTypePatterns::Primary) & UMLEdgeTypeConstants::PrimDependency) == UMLEdgeTypeConstants::PrimDependency);
		return check;
	}

	//! Classifies edge \p e as dependency (primary type).
	void setDependency(edge e) {
		setPrimaryType(e, UMLEdgeTypeConstants::PrimDependency);

		//preliminary set old array too
		m_eType[e] = EdgeType::dependency; //can be removed if edgetypes work properly
	}

	//! Classifies edge \p e as association (primary type).
	void setAssociation(edge e) {
		setPrimaryType(e, UMLEdgeTypeConstants::PrimAssociation);

		//preliminary set old array too
		m_eType[e] = EdgeType::association; //can be removed if edgetypes work properly
	}

	//second level types

	//in contrast to setsecondarytype: do not delete old value

	//! Classifies edge \p e as expansion edge (secondary type).
	void setExpansion(edge e) {
		m_edgeTypes[e] |= expansionPattern();

		//preliminary set old array too
		m_expansionEdge[e] = 1;//can be removed if edgetypes work properly
	}

	//! Returns true iff edge \p e is classified as expansion edge.
	bool isExpansion(edge e) const {
		return (m_edgeTypes[e] & expansionPattern()) == expansionPattern();
	}

	//should add things like cluster and clique boundaries that need rectangle shape

	//! Returns true iff edge \p e is a clique boundary.
	bool isBoundary(edge e) const {
		return isCliqueBoundary(e);
	}

	//tertiary types

	//! Classifies edge \p e as connection at an association class (tertiary type).
	void setAssClass(edge e)
	{
		m_edgeTypes[e] |= assClassPattern();
	}

	//! Returns true iff edge \p e is classified as connection at an association class.
	bool isAssClass(edge e) const
	{
		return (m_edgeTypes[e] & assClassPattern()) == assClassPattern();
	}

	//fourth level types

	//! Classifies edge \p e as connection between hierarchy neighbours (fourth level type).
	void setBrother(edge e) {
		m_edgeTypes[e] |= brotherPattern();
	}

	//! Classifies edge \p e as connection between ...  (fourth level type).
	void setHalfBrother(edge e) {
		m_edgeTypes[e] |= halfBrotherPattern();
	}

	//! Returns true if edge \p e is classified as brother.
	bool isBrother(edge e) const {
		return ( (m_edgeTypes[e] & UMLEdgeTypePatterns::Fourth & brotherPattern()) >> UMLEdgeTypeOffsets::Fourth) == UMLEdgeTypeConstants::Brother;
	}

	//! Returns true if edge \p e is classified as half-brother.
	bool isHalfBrother(edge e) const {
		return ( (m_edgeTypes[e] & UMLEdgeTypePatterns::Fourth & halfBrotherPattern()) >> UMLEdgeTypeOffsets::Fourth) == UMLEdgeTypeConstants::HalfBrother;
	}

	//set generic types

	//! Sets type of edge \p e to current type (bitwise) AND \p et.
	edgeType edgeTypeAND(edge e, edgeType et) {m_edgeTypes[e] &= et; return m_edgeTypes[e];}

	//! Sets type of edge \p e to current type (bitwise) OR \p et.
	edgeType edgeTypeOR(edge e, edgeType et) {m_edgeTypes[e] |= et; return m_edgeTypes[e];}

	//! Sets primary edge type of edge \p e to primary edge type in \p et (deletes old primary value).
	void setPrimaryType(edge e, edgeType et) {
		m_edgeTypes[e] &= 0xfffffff0;
		m_edgeTypes[e] |= (UMLEdgeTypePatterns::Primary & et);
	}

	//! Sets primary edge type of edge \p e to primary edge type in \p et (deletes old primary value).
	void setPrimaryType(edge e, UMLEdgeTypeConstants et) {
		setPrimaryType(e, static_cast<edgeType>(et));
	}

	//! Sets secondary edge type of edge \p e to primary edge type in \p et.
	void setSecondaryType(edge e, edgeType et) {
		m_edgeTypes[e] &= 0xffffff0f;
		m_edgeTypes[e] |= (UMLEdgeTypePatterns::Secondary & ( et << UMLEdgeTypeOffsets::Secondary));
	}

	//! Sets primary type of \p e to bitwise AND of \p et's primary value and old value.
	edgeType edgeTypePrimaryAND(edge e, edgeType et) {m_edgeTypes[e] &= (UMLEdgeTypePatterns::All & et); return m_edgeTypes[e];}

	//! Sets primary type of \p e to bitwise OR of \p et's primary value and old value.
	edgeType edgeTypePrimaryOR(edge e, edgeType et) {m_edgeTypes[e] |= et; return m_edgeTypes[e];}

	//! Sets user defined type locally.
	void setUserType(edge e, edgeType et)
	{
		OGDF_ASSERT( et < 147);
		m_edgeTypes[e] |= (et << UMLEdgeTypeOffsets::User);
	}

	//! Returns user defined type.
	bool isUserType(edge e, edgeType et) const
	{
		OGDF_ASSERT( et < 147);
		return (m_edgeTypes[e] & (et << UMLEdgeTypeOffsets::User)) == (et << UMLEdgeTypeOffsets::User);
	}

	// old edge types

	//this is pure nonsense, cause we have uml-edgetype and m_etype, and should be able to
	//use them with different types, but as long as they arent used correctly (switch instead of xor),
	//use this function to return if e is expansionedge
	//if it is implemented correctly later, delete the array and return m_etype == Graph::expand
	//(the whole function then is obsolete, cause you can check it directly, but for convenience...)
	//should use genexpand, nodeexpand, dissect instead of bool

	//! Sets the expansion edge type of \p e to \p expType.
	void setExpansionEdge(edge e, int expType) {
		m_expansionEdge[e] = expType;
	}

	//! Returns if \p e is an expansion edge.
	bool isExpansionEdge(edge e) const {
		return m_expansionEdge[e] > 0;
	}

	//! Returns the expansion edge type of \p e.
	int expansionType(edge e) const { return m_expansionEdge[e]; }

	//precondition normalized
	//! Returns if \p e is a degree expansion edge.
	bool isDegreeExpansionEdge(edge e) const {
#if 0
		return (m_eType[e] == Graph::expand);
#else
		return m_expansionEdge[e]  == 2;
#endif
	}


	//@}
	/**
	 * @name Access to attributes in original graph
	 * These methods provide easy access to attributes of original nodes and
	 * edges.
	 */
	//@{


	//! Gives access to the node array of the widths of original nodes.
	const NodeArray<double> &widthOrig() const {
		OGDF_ASSERT(m_pGraphAttributes != nullptr);
		return m_pGraphAttributes->width();
	}

	//! Returns the width of original node \p v.
	double widthOrig(node v) const {
		OGDF_ASSERT(m_pGraphAttributes != nullptr);
		return m_pGraphAttributes->width(v);
	}

	//! Gives access to the node array of the heights of original nodes.
	const NodeArray<double> &heightOrig() const {
		OGDF_ASSERT(m_pGraphAttributes != nullptr);
		return m_pGraphAttributes->height();
	}

	//! Returns the height of original node \p v.
	double heightOrig(node v) const {
		OGDF_ASSERT(m_pGraphAttributes != nullptr);
		return m_pGraphAttributes->height(v);
	}

	//! Returns the type of original edge \p e.
	EdgeType typeOrig(edge e) const {
		OGDF_ASSERT(m_pGraphAttributes != nullptr);
		return m_pGraphAttributes->type(e);
	}

	//! Returns the graph attributes of the original graph (the pointer may be 0).
	const GraphAttributes &getGraphAttributes() const {
		OGDF_ASSERT(m_pGraphAttributes != nullptr);
		return *m_pGraphAttributes;
	}

	//@}
	/**
	 * @name Structural alterations
	 */
	//@{

	// Expands nodes with degree > 4 and merge nodes for generalizations
	virtual void expand(bool lowDegreeExpand = false);

	void expandLowDegreeVertices(OrthoRep &OR);

	void collapseVertices(const OrthoRep &OR, Layout &drawing);
	void collapseVertices(const OrthoRep &OR, GridLayout &drawing);

	void removeCrossing(node v); //removes the crossing at node v

	//model a boundary around a star subgraph centered at center
	//and keep external face information (outside the clique
	void insertBoundary(node center, adjEntry& adjExternal);


	//@}
	/**
	 * @name Extension of methods defined by GraphCopys
	 */
	//@{

	//! Splits edge \p e.
	virtual edge split(edge e) override;


	//returns node which was expanded using v
	node expandedNode(node v) const { return m_expandedNode[v]; }

	void setExpandedNode(node v, node w) { m_expandedNode[v] = w; }


	//@}
	/**
	 * @name Creation of new nodes and edges
	 */
	//@{

	/**
	 * \brief Creates a new node with node type \p vType in the planarized representation.
	 * @param vOrig becomes the original node of the new node.
	 * @param vType becomes the type of the new node.
	 */
	node newCopy(node vOrig, Graph::NodeType vType);

	/**
	 * \brief Creates a new edge in the planarized representation.
	 * @param v is the source node of the new edge.
	 * @param adjAfter is the adjacency entry at the target node, after which the
	 *        new edge is inserted.
	 * @param eOrig becomes the original edge of the new edge.
	 */
	edge newCopy(node v, adjEntry adjAfter, edge eOrig);

	/**
	 * \brief Creates a new edge in the planarized representation while updating the embedding \p E.
	 * @param v is the source node of the new edge.
	 * @param adjAfter is the adjacency entry at the target node, after which the
	 *        new edge is inserted.
	 * @param eOrig becomes the original edge of the new edge.
	 * @param E is an embedding of the planarized representation.
	 */
	edge newCopy(node v, adjEntry adjAfter, edge eOrig, CombinatorialEmbedding &E);


	//@}
	/**
	 * @name Crossings
	 */
	//@{

	//! Re-inserts edge \p eOrig by "crossing" the edges in \p crossedEdges.
	/**
	 * Splits each edge in crossedEdges.
	 *
	 * \pre \p eOrig is an edge in the original graph and the edges in \p crossedEdges are in this graph.
	 */
	void insertEdgePath(edge eOrig, const SList<adjEntry> &crossedEdges);

	//! Same as insertEdgePath, but for embedded graphs.
	void insertEdgePathEmbedded(
		edge eOrig,
		CombinatorialEmbedding &E,
		const SList<adjEntry> &crossedEdges);

	//! Removes the complete edge path for edge \p eOrig while preserving the embedding.
	/**
	 * \pre \p eOrig s an edge in the original graph.
	 */
	void removeEdgePathEmbedded(CombinatorialEmbedding &E,
		edge eOrig,
		FaceSet<false> &newFaces) {
		GraphCopy::removeEdgePathEmbedded(E,eOrig,newFaces);
	}

	//! Inserts crossings between two copy edges.
	/**
	 * This method is used in TopologyModule.
	 *
	 * Let \p crossingEdge = (\a a, \a b) and \p crossedEdge = (\a v, \a w).
	 * Then \p crossedEdge is split creating two edges \p crossedEdge = (\a v, \a u)
	 * and (\a u, \a w), \p crossingEdge is removed and replaced by two new edges
	 * \a e1  = (\a a, \a u) and \a e1 = (\a u, \a b).
	 * Finally it sets \p crossingEdge to \a e2 and returns (\a u, \a w).
	 *
	 * @param crossingEdge is the edge that gets split.
	 * @param crossedEdge is the edge that is replaced by two new edges.
	 * @param topDown is used as follows: If set to true, \p crossingEdge will cross
	 *        \p crossedEdge from right to left, otherwise from left to right.
	*/
	edge insertCrossing(
		edge &crossingEdge,
		edge crossedEdge,
		bool topDown);

	//@}
	/**
	 * @name Degree-1 nodes
	 * These methods realize a mechanism for temporarily removing degree-1 nodes.
	 */
	//@{

	/**
	 * \brief Removes all marked degree-1 nodes from the graph copy and stores restore information in \p S.
	 * @param S returns the restore information required by restoreDeg1Nodes().
	 * @param mark defines which nodes are marked for removal; all nodes \a v with
	 *        <I>mark</I>[<I>a</I>]=<B>true</B> are removed.
	 * \pre Only nodes with degree 1 may be marked.
	 */
	void removeDeg1Nodes(ArrayBuffer<Deg1RestoreInfo> &S, const NodeArray<bool> &mark);

	/**
	 * \brief Restores degree-1 nodes previously removed with removeDeg1Nodes().
	 * @param S contains the restore information.
	 * @param deg1s returns the list of newly created nodes in the copy.
	 */
	void restoreDeg1Nodes(ArrayBuffer<Deg1RestoreInfo> &S, List<node> &deg1s);

	//@}
	void writeGML(const char *fileName, const OrthoRep &OR, const GridLayout &drawing);
	void writeGML(std::ostream &os, const OrthoRep &OR, const GridLayout &drawing);

protected:

	int m_currentCC; //!< The index of the current component.
	Graph::CCsInfo m_ccInfo;
	//int m_numCC;     //!< The number of components in the original graph.

	//Array<List<node> >  m_nodesInCC; //!< The list of original nodes in each component.

	const GraphAttributes* m_pGraphAttributes; //!< Pointer to graph attributes of original graph.

	// object types

	//set the type of eCopy according to the type of eOrig
	//should be virtual if PlanRepUML gets its own
	void setCopyType(edge eCopy, edge eOrig);

	//helper to cope with the edge types, shifting to the right place
	edgeType generalizationPattern() const {return static_cast<edgeType>(UMLEdgeTypeConstants::PrimGeneralization);}
	edgeType associationPattern() const    {return static_cast<edgeType>(UMLEdgeTypeConstants::PrimAssociation);}
	edgeType expansionPattern() const      {return UMLEdgeTypeConstants::SecExpansion << UMLEdgeTypeOffsets::Secondary;}
	edgeType assClassPattern() const       {return UMLEdgeTypeConstants::AssClass << UMLEdgeTypeOffsets::Tertiary;}
	edgeType brotherPattern() const        {return UMLEdgeTypeConstants::Brother << UMLEdgeTypeOffsets::Fourth;}
	edgeType halfBrotherPattern() const    {return UMLEdgeTypeConstants::HalfBrother << UMLEdgeTypeOffsets::Fourth;}
	edgeType cliquePattern() const         {return UMLEdgeTypeConstants::SecClique << UMLEdgeTypeOffsets::Secondary;} //boundary

	NodeArray<NodeType> m_vType; //!< Simple node types.

	NodeArray<nodeType> m_nodeTypes; //!< Node types for extended semantic information.

	NodeArray<node>     m_expandedNode; //!< For all expansion nodes, save expanded node.
	NodeArray<adjEntry> m_expandAdj;

	//clique handling: We save an adjEntry of the first edge of an inserted
	//boundary around a clique at its center v
	NodeArray<adjEntry> m_boundaryAdj;

	//zusammenlegbare Typen
	EdgeArray<int>      m_expansionEdge; //1 genmerge, 2 degree (2 highdegree, 3 lowdegree)
	EdgeArray<EdgeType> m_eType;

	//m_edgeTypes stores semantic edge type information on several levels:
	//primary type: generalization, association,...
	//secondary type: merger,...
	//tertiary type: vertical in hierarchy, inner, outer, ...
	//fourth type: neighbour relation (brother, cousin in hierarchy)
	//user types: user defined for local changes
	EdgeArray<edgeType> m_edgeTypes; //store all type information

	//workaround fuer typsuche in insertedgepathembed
	//speichere kopietyp auf originalen
	//maybe it's enough to set gen/ass without extra array
	EdgeArray<edgeType> m_oriEdgeTypes;

	EdgeArray<edge>     m_eAuxCopy; // auxiliary (GraphCopy::initByNodes())
};

}
