/** \file
 * \brief Pure declaration header, find template implementation in
 *        Graph.h
 *
 * Declaration of NodeElement, EdgeElement, and Graph classes.
 *
 * \author Carsten Gutwenger
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

#include <ogdf/basic/GraphList.h>
#include <ogdf/basic/internal/graph_iterators.h>
#include <array>
#include <mutex>

#ifdef OGDF_DEBUG
# include <set>
#endif

namespace ogdf {

//
// in embedded graphs, adjacency lists are given in clockwise order.
//


class OGDF_EXPORT Graph;
class OGDF_EXPORT NodeElement;
class OGDF_EXPORT EdgeElement;
class OGDF_EXPORT AdjElement;
class OGDF_EXPORT FaceElement;
class OGDF_EXPORT ClusterElement;


//! The type of nodes.
//! @ingroup graphs
using node = NodeElement*;

//! The type of edges.
//! @ingroup graphs
using edge = EdgeElement*;

//! The type of adjacency entries.
//! @ingroup graphs
using adjEntry = AdjElement*;


//! Class for adjacency list elements.
/**
 * Adjacency list elements represent the occurrence of an edges in
 * the adjacency list of a node.
 */
class OGDF_EXPORT AdjElement : private internal::GraphElement {
	friend class Graph;
	friend class internal::GraphListBase;
	friend class internal::GraphList<AdjElement>;

	AdjElement *m_twin; //!< The corresponding adjacency entry (same edge)
	edge m_edge; //!< The associated edge.
	node m_node; //!< The node whose adjacency list contains this entry.
	int m_id;    //!< The (unique) index of the adjacency entry.

	//! Constructs an adjacency element for a given node.
	explicit AdjElement(node v) : m_node(v) { }
	//! Constructs an adjacency entry for a given edge and index.
	AdjElement(edge e, int id) : m_edge(e), m_id(id) { }

public:
	//! Returns the edge associated with this adjacency entry.
	edge theEdge() const { return m_edge; }
	//! Conversion to edge.
	operator edge() const { return m_edge; }
	//! Returns the node whose adjacency list contains this element.
	node theNode() const { return m_node; }
	//! Casts to the node whose adjacency list contains this element.
	operator node() const { return m_node; }

	//! Returns the corresponding adjacency element associated with the same edge.
	adjEntry twin() const { return m_twin; }

	//! Returns the associated node of the corresponding adjacency entry (shorthand for twin()->theNode()).
	node twinNode() const { return m_twin->m_node; }

	//! Returns the index of this adjacency element.
	int index() const { return m_id; }

	//! Returns \c true iff this is the source adjacency entry of the corresponding edge.
	bool isSource() const;

	/**
	 * Returns whether this adjacency entry lies between \c adjBefore and \c adjAfter
	 * in clockwise rotation.
	 *
	 * Note that this operation takes time linear in the degree of the node.
	 *
	 * @param adjBefore First adjacency entry. Must be at the same node as this.
	 * @param adjAfter Last adjacency entry. Must be at the same node as this.
	 * @return \c true iff this adjacency entry is in between
	 */
	bool isBetween(adjEntry adjBefore, adjEntry adjAfter) const;

	// traversing faces in clockwise (resp. counter-clockwise) order
	// (if face is an interior face)

	//! Returns the clockwise successor in face. Use faceCycleSucc instead!
	adjEntry clockwiseFaceSucc() const { return m_twin->cyclicPred(); }
	//! Returns the clockwise predecessor in face.  Use faceCycleSucc instead!
	adjEntry clockwiseFacePred() const { return cyclicSucc()->m_twin; }
	//! Returns the counter-clockwise successor in face.
	adjEntry counterClockwiseFaceSucc() const { return m_twin->cyclicSucc(); }
	//! Returns the counter-clockwise predecessor in face.
	adjEntry counterClockwiseFacePred() const { return cyclicPred()->m_twin; }

	// default is traversing faces in clockwise order
	//! Returns the cyclic successor in face.
	adjEntry faceCycleSucc() const { return clockwiseFaceSucc(); }
	//! Returns the cyclic predecessor in face.
	adjEntry faceCyclePred() const { return clockwiseFacePred(); }


	//! Returns the successor in the adjacency list.
	adjEntry succ() const { return static_cast<adjEntry>(m_next); }
	//! Returns the predecessor in the adjacency list.
	adjEntry pred() const { return static_cast<adjEntry>(m_prev); }

	//! Returns the cyclic successor in the adjacency list.
	adjEntry cyclicSucc() const;
	//! Returns the cyclic predecessor in the adjacency list.
	adjEntry cyclicPred() const;

#ifdef OGDF_DEBUG
	const Graph *graphOf() const;
#endif

	//! Standard Comparer
	static int compare(const AdjElement& x,const AdjElement& y) { return x.m_id-y.m_id; }
	OGDF_AUGMENT_STATICCOMPARER(AdjElement)

	OGDF_NEW_DELETE
};

//! Class for the representation of nodes.
class OGDF_EXPORT NodeElement : private internal::GraphElement {
	friend class Graph;
	friend class internal::GraphList<NodeElement>;

	//GraphList<AdjElement> m_adjEdges; //!< The adjacency list of the node.
	int m_indeg;  //!< The indegree of the node.
	int m_outdeg; //!< The outdegree of the node.
	int m_id;     //!< The (unique) index of the node.

#ifdef OGDF_DEBUG
	// we store the graph containing this node for debugging purposes
	const Graph *m_pGraph; //!< The graph containg this node (debug only).
#endif


	//! Constructs a node element with index \p id.
#ifdef OGDF_DEBUG
	/**
	 * \remarks The parameter \p pGraph is only passed in a debug build.
	 * It is used, e.g., by NodeArray for checking if a node belongs to
	 * the correct graph.
	 */
	NodeElement(const Graph *pGraph, int id) :
		m_indeg(0), m_outdeg(0), m_id(id), m_pGraph(pGraph) { }
#else
	NodeElement(int id) : m_indeg(0), m_outdeg(0), m_id(id) { }
#endif


public:
	//! The container containing all entries in the adjacency list of this node.
	internal::GraphObjectContainer<AdjElement> adjEntries;

	//! Returns the (unique) node index.
	int index() const { return m_id; }

	//! Returns the indegree of the node.
	int indeg() const { return m_indeg; }
	//! Returns the outdegree of the node.
	int outdeg() const { return m_outdeg; }
	//! Returns the degree of the node (indegree + outdegree).
	int degree() const { return m_indeg + m_outdeg; }

	//! Returns the first entry in the adjaceny list.
	adjEntry firstAdj() const { return adjEntries.head();  }
	//! Returns the last entry in the adjacency list.
	adjEntry lastAdj () const { return adjEntries.tail(); }

	//! Returns the successor in the list of all nodes.
	node succ() const { return static_cast<node>(m_next); }
	//! Returns the predecessor in the list of all nodes.
	node pred() const { return static_cast<node>(m_prev); }

	//! Returns a list with all adjacency entries of this node.
	/**
	 * @tparam ADJLIST is the type of adjacency entry list, which is returned.
	 * @param  adjList is assigned the list of all adjacency entries of this node.
	 */
	template<class ADJLIST>
	void allAdjEntries(ADJLIST &adjList) const {
		adjList.clear();
		for(adjEntry adj : this->adjEntries) {
			adjList.pushBack(adj);
		}
	}

	//! Returns a list with all edges incident to this node.
	/**
	 * Note that each self-loop of this node is contained twice in the list.
	 *
	 * @tparam EDGELIST is the type of edge list, which is returned.
	 * @param  edgeList is assigned the list of all edges incident to this node
	 *                  (including incoming and outcoming edges).
	 */
	template<class EDGELIST>
	void adjEdges(EDGELIST &edgeList) const {
		edgeList.clear();
		for(adjEntry adj : this->adjEntries) {
			edgeList.pushBack(adj->theEdge());
		}
	}

	//! Returns a list with all incoming edges of this node.
	/**
	 * @tparam EDGELIST is the type of edge list, which is returned.
	 * @param  edgeList is assigned the list of all incoming edges incident to this node.
	 */
	template<class EDGELIST>
	void inEdges(EDGELIST &edgeList) const;

	//! Returns a list with all outgoing edges of this node.
	/**
	 * @tparam EDGELIST is the type of edge list, which is returned.
	 * @param  edgeList is assigned the list of all outgoing edges incident to this node.
	 */
	template<class EDGELIST>
	void outEdges(EDGELIST &edgeList) const;

#ifdef OGDF_DEBUG
	//! Returns the graph containing this node (debug only).
	const Graph *graphOf() const { return m_pGraph; }
#endif

	//! Standard Comparer
	static int compare(const NodeElement& x,const NodeElement& y) { return x.m_id-y.m_id; }
	OGDF_AUGMENT_STATICCOMPARER(NodeElement)

	OGDF_NEW_DELETE
};

inline adjEntry AdjElement::cyclicSucc() const
{
	return (m_next) ? static_cast<adjEntry>(m_next) : m_node->firstAdj();
}

inline adjEntry AdjElement::cyclicPred() const
{
	return (m_prev) ? static_cast<adjEntry>(m_prev) : m_node->lastAdj();
}



//! Class for the representation of edges.
class OGDF_EXPORT EdgeElement : private internal::GraphElement {
	friend class Graph;
	friend class internal::GraphList<EdgeElement>;

	node m_src; //!< The source node of the edge.
	node m_tgt; //!< The target node of the edge.
	AdjElement *m_adjSrc; //!< Corresponding adjacancy entry at source node.
	AdjElement *m_adjTgt; //!< Corresponding adjacancy entry at target node.
	int m_id; // The (unique) index of the node.

	//! Constructs an edge element (\p src,\p tgt).
	/**
	 * @param src is the source node of the edge.
	 * @param tgt is the target node of the edge.
	 * @param adjSrc is the corresponding adjacency entry at source node.
	 * @param adjTgt is the corresponding adjacency entry at target node.
	 * @param id is the index of the edge.
	 */
	EdgeElement(node src, node tgt, AdjElement *adjSrc, AdjElement *adjTgt, int id) :
		m_src(src), m_tgt(tgt), m_adjSrc(adjSrc), m_adjTgt(adjTgt), m_id(id) { }

	//! Constructs an edge element (\p src,\p tgt).
	/**
	 * @param src is the source node of the edge.
	 * @param tgt is the target node of the edge.
	 * @param id is the index of the edge.
	 */
	EdgeElement(node src, node tgt, int id) :
		m_src(src), m_tgt(tgt), m_id(id) { }

public:
	//! Returns the index of the edge.
	int index() const { return m_id; }
	//! Returns the source node of the edge.
	node source() const { return m_src; }
	//! Returns the target node of the edge.
	node target() const { return m_tgt; }
	//! Returns a list of adjacent nodes. If this edge is a self-loop, both entries will be the same node.
	std::array<node, 2> nodes() const { return std::array<node, 2>{{m_src, m_tgt}}; }

	//! Returns the corresponding adjacancy entry at source node.
	adjEntry adjSource() const { return m_adjSrc; }
	//! Returns the corresponding adjacancy entry at target node.
	adjEntry adjTarget() const { return m_adjTgt; }

	//! Returns the adjacent node different from \p v.
	node opposite(node v) const { return (v == m_src) ? m_tgt : m_src; }

	//! Returns true iff the edge is a self-loop (source node = target node).
	bool isSelfLoop() const { return m_src == m_tgt; }

	//! Returns true iff edge \p e is an inverted edge to this (directed) edge
	bool isInvertedDirected(edge e) const { return m_src == e->target() && m_tgt == e->source(); }

	//! Returns true iff edge \p e is parallel to this (directed) edge (or if it is the same edge)
	bool isParallelDirected(edge e) const { return m_src == e->source() && m_tgt == e->target(); }

	//! Returns true iff edge \p e is parallel to this (undirected) edge (or if it is the same edge)
	bool isParallelUndirected(edge e) const { return isParallelDirected(e) || isInvertedDirected(e); }

	//! Returns the successor in the list of all edges.
	edge succ() const { return static_cast<edge>(m_next); }
	//! Returns the predecessor in the list of all edges.
	edge pred() const { return static_cast<edge>(m_prev); }

#ifdef OGDF_DEBUG
	//! Returns the graph containing this node (debug only).
	const Graph *graphOf() const { return m_src->graphOf(); }
#endif

	//! Returns true iff \p v is incident to the edge.
	bool isIncident(node v) const { return v == m_src || v == m_tgt; }

	//! Returns true iff \p e is adjacent to the edge.
	bool isAdjacent(edge e) const { return isIncident(e->m_src) || isIncident(e->m_tgt); }

	//! Returns the common node of the edge and \p e. Returns nullptr if the two edges are not adjacent.
	node commonNode(edge e) const { return (m_src==e->m_src || m_src==e->m_tgt) ? m_src : ((m_tgt==e->m_src || m_tgt==e->m_tgt) ? m_tgt: nullptr); }

	//! Returns an adjacency entry of this edge at node \c v.
	//! If this is a self-loop the source adjacency entry will always be returned.
	adjEntry getAdj(node v) const {
		OGDF_ASSERT(this->isIncident(v));
		return v == m_src ? m_adjSrc : m_adjTgt;
	}

	//! Standard Comparer
	static int compare(const EdgeElement& x,const EdgeElement& y) { return x.m_id-y.m_id; }
	OGDF_AUGMENT_STATICCOMPARER(EdgeElement)

	OGDF_NEW_DELETE

#ifdef OGDF_DEBUG
private:
	bool m_hidden = false;
#endif
};

#ifdef OGDF_DEBUG
inline const Graph *AdjElement::graphOf() const {
	return m_node->graphOf();
}
#endif

inline bool AdjElement::isSource() const {
	return this == m_edge->adjSource();
}

inline bool AdjElement::isBetween(adjEntry adjBefore, adjEntry adjAfter) const {
#ifdef OGDF_DEBUG
	node v = this->theNode();
	OGDF_ASSERT(adjBefore->theNode() == v);
	OGDF_ASSERT(adjAfter->theNode() == v);
#endif
	bool result = this != adjBefore && this != adjAfter && adjBefore != adjAfter;

	if (result) {
		adjEntry adj = adjBefore;
		for (; adj != this && adj != adjAfter; adj = adj->cyclicSucc());
		result = adj == this;
	}

	return result;
}

template<class EDGELIST>
void NodeElement::inEdges(EDGELIST &edgeList) const {
	edgeList.clear();
	for(adjEntry adj : this->adjEntries) {
		edge e = adj->theEdge();
		if (adj == e->adjTarget()) edgeList.pushBack(e);
	}
}

template<class EDGELIST>
void NodeElement::outEdges(EDGELIST &edgeList) const {
	edgeList.clear();
	for(adjEntry adj : this->adjEntries) {
		edge e = adj->theEdge();
		if (adj == e->adjSource()) edgeList.pushBack(e);
	}
}

class NodeArrayBase;
class EdgeArrayBase;
class AdjEntryArrayBase;
template<class T> class NodeArray;
template<class T> class EdgeArray;
template<class T> class AdjEntryArray;
class OGDF_EXPORT GraphObserver;

namespace internal {
template<typename CONTAINER> inline void getAllNodes(const Graph& G, CONTAINER& nodes);
template<typename CONTAINER> inline void getAllEdges(const Graph& G, CONTAINER& edges);
}

//! Data type for general directed graphs (adjacency list representation).
/**
 * @ingroup graphs
 *
 * <H3>Thread Safety</H3>
 * The class Graph allows shared access of threads to const methods only.
 * If one thread executes a non-const method, shared access is no longer thread-safe.
 *
 * <H3>Iteration</H3>
 * You may iterate over the nodes and edges of a graph using C++11 range-based for loops.
 * Find some examples below.
 *
 * <ul>
 *   <li>Iterate over all nodes \a v of graph \a G using c++11 syntax :
 *     \code
 *  for(node v : G.nodes) {
 *    // do stuff with node v
 *  }
 *     \endcode
 *
 *   <li>Iterate over all nodes \a v of graph \a G :
 *     \code
 *  for(node v = G.firstNode(); v != nullptr; v = v->succ()) {
 *    // do stuff with node v
 *  }
 *     \endcode
 *
 *   <li>Iterate over all edges \a e of graph \a G using c++11 syntax :
 *     \code
 *  for(edge e : G.edges) {
 *    // do stuff with node v
 *  }
 *     \endcode
 *
 *   <li>Iterate over all incident edges of node \a v using c++11 syntax:
 *     \code
 *  for(adjEntry adj : v->adjEntries) {
 *    edge e = adj->theEdge();
 *    // do stuff with edge e
 *  }
 *     \endcode
 * </ul>
 */

class OGDF_EXPORT Graph
{
public:
	class HiddenEdgeSet;
private:
	int m_nodeIdCount; //!< The Index that will be assigned to the next created node.
	int m_edgeIdCount; //!< The Index that will be assigned to the next created edge.

	int m_nodeArrayTableSize; //!< The current table size of node arrays associated with this graph.
	int m_edgeArrayTableSize; //!< The current table size of edge arrays associated with this graph.

	mutable ListPure<NodeArrayBase*> m_regNodeArrays; //!< The registered node arrays.
	mutable ListPure<EdgeArrayBase*> m_regEdgeArrays; //!< The registered edge arrays.
	mutable ListPure<AdjEntryArrayBase*> m_regAdjArrays;  //!< The registered adjEntry arrays.
	mutable ListPure<GraphObserver*> m_regStructures; //!< The registered graph structures.

#ifndef OGDF_MEMORY_POOL_NTS
	mutable std::mutex m_mutexRegArrays; //!< The critical section for protecting shared acces to register/unregister methods.
#endif

	List<HiddenEdgeSet*> m_hiddenEdgeSets; //!< The list of hidden edges.

public:

	/**
	* @name Iterators
	* These types are used for graph object iterators, which are returned by graph object containers
	* like nodes and edges.
	*/
	//@{

	//! Provides a bidirectional iterator to a node in a graph.
	using node_iterator = internal::GraphIterator<node>;
	//! Provides a bidirectional iterator to an edge in a graph.
	using edge_iterator = internal::GraphIterator<edge>;
	//! Provides a bidirectional iterator to an entry in an adjacency list.
	using adjEntry_iterator = internal::GraphIterator<adjEntry>;

	//@}
	/**
	* @name Enumerations
	* These enumerations are mainly meant for advanced or internal usage scenarios.
	*/
	//@{

	//! The type of edges (only used in derived classes).
	enum class EdgeType {
		association = 0,
		generalization = 1,
		dependency = 2
	};

	//! The type of nodes.
	enum class NodeType {
		vertex = 0,
		dummy = 1,
		generalizationMerger = 2,
		generalizationExpander = 3,
		highDegreeExpander = 4,
		lowDegreeExpander = 5,
		associationClass = 6
	};

	//@}


	/**
	* @name Graph object containers
	* These containers maintain the nodes and edges of the graph, and provide node and edge iterators.
	*/
	//@{

	//! The container containing all node objects.
	internal::GraphObjectContainer<NodeElement> nodes;

	//! The container containing all edge objects.
	internal::GraphObjectContainer<EdgeElement> edges;

	//@}


	//! Constructs an empty graph.
	Graph();

	//! Constructs a graph that is a copy of \p G.
	/**
	 * The constructor assures that the adjacency lists of nodes in the
	 * constructed graph are in the same order as the adjacency lists in \p G.
	 * This is in particular important when dealing with embedded graphs.
	 *
	 * @param G is the graph that will be copied.
	 */
	Graph(const Graph &G);

	//! Destructor.
	virtual ~Graph();

	/**
	 * @name Access methods
	 */
	//@{

	//! Returns true iff the graph is empty, i.e., contains no nodes.
	bool empty() const { return nodes.empty(); }

	//! Returns the number of nodes in the graph.
	int numberOfNodes() const { return nodes.size(); }

	//! Returns the number of edges in the graph.
	int numberOfEdges() const { return edges.size(); }

	//! Returns the largest used node index.
	int maxNodeIndex() const { return m_nodeIdCount-1; }
	//! Returns the largest used edge index.
	int maxEdgeIndex() const { return m_edgeIdCount-1; }
	//! Returns the largest used adjEntry index.
	int maxAdjEntryIndex() const { return (m_edgeIdCount<<1)-1; }

	//! Returns the table size of node arrays associated with this graph.
	int nodeArrayTableSize() const { return m_nodeArrayTableSize; }
	//! Returns the table size of edge arrays associated with this graph.
	int edgeArrayTableSize() const { return m_edgeArrayTableSize; }
	//! Returns the table size of adjEntry arrays associated with this graph.
	int adjEntryArrayTableSize() const { return m_edgeArrayTableSize << 1; }

	//! Returns the first node in the list of all nodes.
	node firstNode() const { return nodes.head(); }
	//! Returns the last node in the list of all nodes.
	node lastNode () const { return nodes.tail(); }

	//! Returns the first edge in the list of all edges.
	edge firstEdge() const { return edges.head(); }
	//! Returns the last edge in the list of all edges.
	edge lastEdge () const { return edges.tail(); }

	/**
	 * Returns a random node.
	 *
	 * \c nullptr is returned if no feasible node exists.
	 *
	 * @see chooseIteratorFrom
	 */
	node chooseNode(std::function<bool(node)> includeNode = [](node) { return true; }, bool isFastTest = true) const;

	/**
	 * Returns a random edge.
	 *
	 * \c nullptr is returned if no feasible edge exists.
	 *
	 * @see chooseIteratorFrom
	 */
	edge chooseEdge(std::function<bool(edge)> includeEdge = [](edge) { return true; }, bool isFastTest = true) const;

	//! Returns a container with all nodes of the graph.
	/**
	 * @tparam CONTAINER is the type of node container which is returned.
	 * @param nodeContainer is assigned the container of all nodes.
	 */
	template<class CONTAINER>
	void allNodes(CONTAINER& nodeContainer) const {
		internal::getAllNodes<CONTAINER>(*this, nodeContainer);
	}

	//! Returns a container with all edges of the graph.
	/**
	 * @tparam CONTAINER is the type of the edge container which is returned.
	 * @param edgeContainer is assigned the list of all edges.
	 */
	template<class CONTAINER>
	void allEdges(CONTAINER &edgeContainer) const {
		internal::getAllEdges<CONTAINER>(*this, edgeContainer);
	}

	//@}
	/**
	 * @name Creation of new nodes and edges
	 */
	//@{

	//! Creates a new node and returns it.
	node newNode();

	//! Creates a new node with predefined index and returns it.
	/**
	 * \pre \p index is currently not the index of any other node in the graph.
	 *
	 * \attention Passing a node index that is already in use results in an inconsistent
	 *            data structure. Only use this method if you know what you're doing!
	 *
	 * @param index is the index that will be assigned to the newly created node.
	 * @return the newly created node.
	 */
	node newNode(int index);

	//! Creates a new edge (\p v,\p w) and returns it.
	/**
	 * @param v is the source node of the newly created edge.
	 * @param w is the target node of the newly created edge.
	 * @return the newly created edge.
	 */
	edge newEdge(node v, node w);

	//! Creates a new edge (\p v,\p w) with predefined index and returns it.
	/**
	 * \pre \p index is currently not the index of any other edge in the graph.
	 *
	 * \attention  Passing an edge index that is already in use results in an inconsistent
	 *             data structure. Only use this method if you know what you're doing!
	 *
	 * @param v     is the source node of the newly created edge.
	 * @param w     is the target node of the newly created edge.
	 * @param index is the index that will be assigned to the newly created edge.
	 * @return the newly created edge.
	 */
	edge newEdge(node v, node w, int index);

	//! Creates a new edge at predefined positions in the adjacency lists.
	/**
	 * Let \a v be the node whose adjacency list contains \p adjSrc,
	 * and \a w the node whose adjacency list contains \p adjTgt. Then,
	 * the created edge is (\a v,\a w).
	 *
	 * @param adjSrc is the adjacency entry after which the new edge is inserted
	 *               in the adjacency list of \a v.
	 * @param adjTgt is the adjacency entry after which the new edge is inserted
	 *               in the adjacency list of \a w.
	 * @param dir    specifies if the edge is inserted before or after the given
	 *               adjacency entries.
	 * @return the newly created edge.
	 */
	edge newEdge(adjEntry adjSrc, adjEntry adjTgt, Direction dir = Direction::after);

	//! Creates a new edge at predefined positions in the adjacency lists.
	/**
	 * Let \a w be the node whose adjacency list contains \p adjTgt. Then,
	 * the created edge is (\p v,\a w).
	 *
	 * @param v      is the source node of the new edge; the edge is added at the end
	 *               of the adjacency list of \p v.
	 * @param adjTgt is the adjacency entry after which the new edge is inserted
	 *               in the adjacency list of \a w.
	 * @return the newly created edge.
	 */
	edge newEdge(node v, adjEntry adjTgt);

	//! Creates a new edge at predefined positions in the adjacency lists.
	/**
	 * Let \a v be the node whose adjacency list contains \p adjSrc. Then,
	 * the created edge is (\a v,\p w).
	 *
	 * @param adjSrc is the adjacency entry after which the new edge is inserted
	 *               in the adjacency list of \a v.
	 * @param w      is the source node of the new edge; the edge is added at the end
	 *               of the adjacency list of \p w.
	 * @return the newly created edge.
	 */
	edge newEdge(adjEntry adjSrc, node w);


	//@}
	/**
	 * @name Removing nodes and edges
	 */
	//@{

	//! Removes node \p v and all incident edges from the graph.
	virtual void delNode(node v);

	//! Removes edge \p e from the graph.
	virtual void delEdge(edge e);

	//! Removes all nodes and all edges from the graph.
	virtual void clear();

	//@}

	/**
	 * @brief Functionality for temporarily hiding edges in constant time.
	 *
	 * Hidden edges are removed from the
	 * list of all edges and their corresponding adjacency entries from the repsective
	 * adjacency lists, but the edge objects themselves are not destroyed. Hidden edges
	 * can later be reactivated using #restore(). Restoring edges will not preserve the adjacency order.
	 *
	 * Hiding or restoring an edge takes constant time.
	 * Thus, hiding edges may be more performant than creating a ogdf::GraphCopy and modifying it.
	 *
	 * Hidden edge sets can be restored as a whole. Alternatively a single edge of a such a set can be restored.
	 *
	 * Note that all hidden edges are restored when the set of hidden edges is destroyed.

	 * Do not delete any nodes incident to hidden edges.
	 * Do not hide edges while iterating over the edges of a ogdf::Graph.
	 * Instead, iterate over a copied list of all edges.
	 */
	class OGDF_EXPORT HiddenEdgeSet
	{
		friend class Graph;
		friend class EdgeElement;

	public:
		/**
		* Creates a new set of hidden edges.
		*
		* @param graph the graph to be modified
		*/
		explicit HiddenEdgeSet(Graph &graph) : m_graph(&graph)
		{
			m_it = m_graph->m_hiddenEdgeSets.pushFront(this);
		}

		/**
		* Restores all hidden edges.
		*/
		~HiddenEdgeSet()
		{
			if (m_graph) {
				restore();
				m_graph->m_hiddenEdgeSets.del(m_it);
			}
		}

		/**
		* Hides the given edge.
		*
		* \pre the edge is currently not hidden.
		* \pre the graph associated with this set does still exist.
		*/
		void hide(edge e);

		/**
		* Reveals the given edge.
		*
		* \pre the edge is currently hidden using this set.
		* \pre the graph associated with this set does still exist.
		*/
		void restore(edge e);

		/**
		* Restores all edges contained in this set.
		* The set will remain valid.
		*
		* \pre the graph associated with this set does still exist.
		*/
		void restore();

		/**
		* Returns the number of edges contained in this set.
		*/
		int size();

	private:
		internal::GraphList<EdgeElement> m_edges;
		ListIterator<HiddenEdgeSet*> m_it;
		Graph *m_graph;

		// prevent copying
		HiddenEdgeSet(const HiddenEdgeSet&);
		HiddenEdgeSet& operator=(const HiddenEdgeSet&);
	};

	/**
	 * @name Advanced modification methods
	 */
	//@{

	/**
	 * @copydoc ogdf::Graph::insert(const Graph&)
	 * @param nodeMap is assigned a mapping from nodes in \p G to nodes in this Graph.
	 */
	void insert(const Graph &G, NodeArray<node> &nodeMap);

	//! Inserts Graph \p G as a subgraph into this Graph.
	/**
	 * @param G is the Graph to be inserted into this Graph.
	 */
	void insert(const Graph &G);

	//! Splits edge \p e into two edges introducing a new node.
	/**
	 * Let \p e=(\a v,\a w). Then, the resulting two edges are \a e=(\a v,\a u)
	 * and \a e'=(\a u,\a w), where \a u is a new node.
	 *
	 * \note The edge \p e is modified by this operation.
	 *
	 * @param e is the edge to be split.
	 * @return The edge \a e'.
	 */
	virtual edge split(edge e);

	//! Undoes a split operation.
	/**
	 * Removes node \p u by joining the two edges adjacent to \p u. The
	 * outgoing edge of \p u is removed and the incoming edge \a e is reused
	 *
	 * \pre \p u has exactly one incoming and one outgoing edge, and
	 *    none of them is a self-loop.
	 *
	 * @param u is the node to be unsplit.
	 * @return The edge \a e.
	 */
	void unsplit(node u);

	//! Undoes a split operation.
	/**
	 * For two edges \p eIn = (\a x,\a u) and \p eOut = (\a u,\a y), removes
	 * node \a u by joining \p eIn and \p eOut. Edge \p eOut is removed and
	 * \p eIn is reused.
	 *
	 * \pre \p eIn and \p eOut are the only edges incident with \a u and
	 *      none of them is a self-loop.
	 *
	 * @param eIn  is the (only) incoming edge of \a u.
	 * @param eOut is the (only) outgoing edge of \a u.
	 */
	virtual void unsplit(edge eIn, edge eOut);

	//! Splits a node while preserving the order of adjacency entries.
	/**
	 * This method splits a node \a v into two nodes \a vl and \a vr. Node
	 * \a vl receives all adjacent edges of \a v from \p adjStartLeft until
	 * the edge preceding \p adjStartRight, and \a vr the remaining nodes
	 * (thus \p adjStartRight is the first edge that goes to \a vr). The
	 * order of adjacency entries is preserved. Additionally, a new edge
	 * (\a vl,\a vr) is created, such that this edge is inserted before
	 * \p adjStartLeft and \p adjStartRight in the the adjacency lists of
	 * \a vl and \a vr.
	 *
	 * Node \a v is modified to become node \a vl, and node \a vr is returned.
	 * This method is useful when modifying combinatorial embeddings.
	 *
	 * @param adjStartLeft  is the first entry that goes to the left node.
	 * @param adjStartRight is the first entry that goes to the right node.
	 * @return the newly created node.
	 */
	node splitNode(adjEntry adjStartLeft, adjEntry adjStartRight);

	//! Contracts edge \p e while preserving the order of adjacency entries.
	/**
	 * @attention Edges parallel to \p e will also be contracted (they do not result in self-loops).
	 * @param e is the edge to be contracted.
	 * @return The endpoint of \p e to which all edges have been moved. The implementation ensures this to be the source of the former edge \p e.
	 */
	node contract(edge e);

	//! Moves edge \p e to a different adjacency list.
	/**
	 * The source adjacency entry of \p e is moved to the adjacency list containing
	 * \p adjSrc and is inserted before or after \p adjSrc, and its target adjacency entry
	 * to the adjacency list containing \p adjTgt and is inserted before or after
	 * \p adjTgt; \p e is afterwards an edge from owner(\p adjSrc) to owner(\p adjTgt).
	 *
	 * @param e      is the edge to be moved.
	 * @param adjSrc is the adjaceny entry before or after which the source adjacency entry
	 *               of \p e will be inserted.
	 * @param dirSrc specifies if the source adjacency entry of \p e will be inserted before or after \p adjSrc.
	 * @param adjTgt is the adjaceny entry before or after which the target adjacency entry
	 *               of \p e will be inserted.
	 * @param dirTgt specifies if the target adjacency entry of \p e will be inserted before or after \p adjTgt.
	 */
	void move(edge e, adjEntry adjSrc, Direction dirSrc,
		adjEntry adjTgt, Direction dirTgt);

	//! Moves the target node of edge \p e to node \p w.
	/**
	 * If \p e=(\a v,\a u) before, then \p e=(\a v,\p w) afterwards.
	 *
	 * @param e is the edge whose target node is moved.
	 * @param w is the new target node of \p e.
	 */
	void moveTarget(edge e, node w);

	//! Moves the target node of edge \p e to a specific position in an adjacency list.
	/**
	 * Let \a w be the node containing \p adjTgt. If \p e=(\a v,\a u) before, then \a e=(\a v,\a w) afterwards.
	 * Inserts the adjacency entry before or after \p adjTgt according to \p dir.
	 *
	 * @param e is the edge whose target node is moved.
	 * @param adjTgt is the adjacency entry before or after which the target adjacency entry of \p e is inserted.
	 * @param dir specifies if the target adjacency entry of \p e is inserted before or after \p adjTgt.
	 */
	void moveTarget(edge e, adjEntry adjTgt, Direction dir);

	//! Moves the source node of edge \p e to node \p w.
	/**
	 * If \p e=(\a v,\a u) before, then \p e=(\p w,\a u) afterwards.
	 *
	 * @param e is the edge whose source node is moved.
	 * @param w is the new source node of \p e.
	 */
	void moveSource(edge e, node w);

	//! Moves the source node of edge \p e to a specific position in an adjacency list.
	/**
	 * Let \a w be the node containing \p adjSrc. If \p e=(\a v,\a u) before, then \a e=(\a w,\a u) afterwards.
	 * Inserts the adjacency entry before or after \p adjSrc according to \p dir.
	 *
	 * @param e is the edge whose source node is moved.
	 * @param adjSrc is the adjacency entry before or after which the source adjacency entry of \p e is inserted.
	 * @param dir specifies if the source adjacency entry of \p e is inserted before or after \p adjSrc.
	 */
	void moveSource(edge e, adjEntry adjSrc, Direction dir);

	//! Searches and returns an edge connecting nodes \p v and \p w in time \a O( min(deg(\p v ), deg(\p w ))).
	/**
	 * @param v is the first endpoint of the edge to be searched.
	 * @param w is the second endpoint of the edge to be searched.
	 * @param directed iff set to true, enforces that
	 * \p v must be the source node and \p w the target node of the edge.
	 * @return an edge (\p v,\p w) (or (\p w,\p v) for !\p directed)
	 * if such an edge exists, nullptr otherwise.
	 */
	edge searchEdge (node v, node w, bool directed = false) const;

	//! Reverses the edge \p e, i.e., exchanges source and target node.
	void reverseEdge(edge e);

	//! Reverses all edges in the graph.
	void reverseAllEdges();

	//! Collapses all nodes in the list \p nodesToCollapse to the first node in the list.
	/**
	 * Parallel edges are removed.
	 *
	 * @tparam NODELIST        is the type of input node list.
	 * @param  nodesToCollapse is the list of nodes that will be collapsed. This list will be empty after the call.
	 */
	template<class NODELIST>
	void collapse(NODELIST &nodesToCollapse){
		node v = nodesToCollapse.popFrontRet();
		while (!nodesToCollapse.empty())
		{
			node w = nodesToCollapse.popFrontRet();
			adjEntry adj = w->firstAdj();
			while (adj != nullptr)
			{
				adjEntry succ = adj->succ();
				edge e = adj->theEdge();
				if (e->source() == v || e->target() == v)
					delEdge(e);
				else if (e->source() == w)
					moveSource(e,v);
				else
					moveTarget(e,v);
				adj = succ;
			}
			delNode(w);
		}
	}

	//! Sorts the adjacency list of node \p v according to \p newOrder.
	/**
	 * \pre \p newOrder contains exactly the adjacency entries of \p v!
	 *
	 * @tparam ADJ_ENTRY_LIST is the type of the input adjacency entry list.
	 * @param  v              is the node whose adjacency list will be sorted.
	 * @param  newOrder       is the list of adjacency entries of \p v in the new order.
	 */
	template<class ADJ_ENTRY_LIST>
	void sort(node v, const ADJ_ENTRY_LIST &newOrder) {
#ifdef OGDF_DEBUG
		std::set<int> entries;
		int counter = 0;

		for(adjEntry adj : newOrder) {
			entries.insert(adj->index());
			OGDF_ASSERT(adj->theNode() == v);
			counter++;
		}

		OGDF_ASSERT(counter == v->degree());
		OGDF_ASSERT(entries.size() == static_cast<unsigned int>(v->degree()));
#endif
		v->adjEntries.sort(newOrder);
	}

	//! Reverses the adjacency list of \p v.
	/**
	 * @param v is the node whose adjacency list will be reveresed.
	 */
	void reverseAdjEdges(node v) {
		v->adjEntries.reverse();
	}

	//! Moves adjacency entry \p adjMove before or after \p adjPos.
	/**
	 * \pre \p adjMove and adjAfter are distinct entries in the same adjacency list.
	 *
	 * @param adjMove is an entry in the adjacency list of a node in this graph.
	 * @param adjPos  is an entry in the same adjacency list as \p adjMove.
	 * @param dir     specifies if \p adjMove is moved before or after \p adjPos.
	 */
	void moveAdj(adjEntry adjMove, Direction dir, adjEntry adjPos) {
		OGDF_ASSERT(adjMove != nullptr);
		OGDF_ASSERT(adjPos != nullptr);
		OGDF_ASSERT(adjMove->graphOf() == this);
		OGDF_ASSERT(adjPos->graphOf() == this);
		internal::GraphList<AdjElement> &adjList = adjMove->m_node->adjEntries;
		adjList.move(adjMove, adjList, adjPos, dir);
	}

	//! Moves adjacency entry \p adjMove after \p adjAfter.
	/**
	 * \pre \p adjMove and \p adjAfter are distinct entries in the same adjacency list.
	 *
	 * @param adjMove  is an entry in the adjacency list of a node in this graph.
	 * @param adjAfter is an entry in the same adjacency list as \p adjMove.
	 */
	void moveAdjAfter(adjEntry adjMove, adjEntry adjAfter) {
		OGDF_ASSERT(adjMove != nullptr);
		OGDF_ASSERT(adjAfter != nullptr);
		OGDF_ASSERT(adjMove->graphOf() == this);
		OGDF_ASSERT(adjAfter->graphOf() == this);
		adjMove->m_node->adjEntries.moveAfter(adjMove,adjAfter);
	}

	//! Moves adjacency entry \p adjMove before \p adjBefore.
	/**
	 * \pre \p adjMove and \p adjBefore are distinct entries in the same adjacency list.
	 *
	 * @param adjMove   is an entry in the adjacency list of a node in this graph.
	 * @param adjBefore is an entry in the same adjacency list as \p adjMove.
	 */
	void moveAdjBefore(adjEntry adjMove, adjEntry adjBefore) {
		OGDF_ASSERT(adjMove != nullptr);
		OGDF_ASSERT(adjBefore != nullptr);
		OGDF_ASSERT(adjMove->graphOf() == this);
		OGDF_ASSERT(adjBefore->graphOf() == this);
		adjMove->m_node->adjEntries.moveBefore(adjMove,adjBefore);
	}

	//! Reverses all adjacency lists.
	void reverseAdjEdges();

	//! Exchanges two entries in an adjacency list.
	/**
	 * \pre \p adj1 and \p adj2 must be belong to the same adjacency list.
	 *
	 * @param adj1 the first adjacency entry to be swapped.
	 * @param adj2 the secomd adjacency entry to be swapped.
	 */
	void swapAdjEdges(adjEntry adj1, adjEntry adj2) {
		OGDF_ASSERT(adj1->theNode() == adj2->theNode());
		OGDF_ASSERT(adj1->graphOf() == this);

		adj1->theNode()->adjEntries.swap(adj1,adj2);
	}


	//@}
	/**
	 * @name Miscellaneous
	 */
	//@{

	//! Returns the genus of the graph's embedding.
	/**
	 * The genus of a graph is defined as follows. Let \a G be a graph
	 * with \a m edges, \a n nodes, \a c connected components, \a nz
	 * isolated vertices, and \a fc face cycles. Then,
	 * \f[
	 *   genus(G) = (m/2 + 2c - n -nz -fc)/2
	 * \f]
	 *
	 * @return the genus of the graph's current embedding; if this is 0, then the graph is planarly embedded.
	 */
	int genus() const;

	//! Returns true iff the graph represents a combinatorial embedding.
	/**
	 * @return true if the current embedding (given by the adjacency lists) represents a combinatorial embedding, false otherwise.
	 */
	bool representsCombEmbedding() const {
		return genus() == 0;
	}

#ifdef OGDF_DEBUG
	//! Asserts that this graph is consistent.
	void consistencyCheck() const;
#endif

	//@}
	/**
	 * @name Registering arrays and observers
	 * These methods are used by various graph array types like NodeArray or EdgeArray.
	 * There should be no need to use them directly in user code.
	 */
	//@{

	//! Registers a node array.
	/**
	 * \remark This method is automatically called by node arrays; it should not be called manually.
	 *
	 * @param pNodeArray is a pointer to the node array's base; this node array must be associated with this graph.
	 * @return an iterator pointing to the entry for the registered node array in the list of registered node arrays.
	 *         This iterator is required for unregistering the node array again.
	 */
	ListIterator<NodeArrayBase*> registerArray(NodeArrayBase *pNodeArray) const;

	//! Registers an edge array.
	/**
	 * \remark This method is automatically called by edge arrays; it should not be called manually.
	 *
	 * @param pEdgeArray is a pointer to the edge array's base; this edge array must be associated with this graph.
	 * @return an iterator pointing to the entry for the registered edge array in the list of registered edge arrays.
	 *         This iterator is required for unregistering the edge array again.
	 */
	ListIterator<EdgeArrayBase*> registerArray(EdgeArrayBase *pEdgeArray) const;

	//! Registers an adjEntry array.
	/**
	 * \remark This method is automatically called by adjacency entry arrays; it should not be called manually.
	 *
	 * @param pAdjArray is a pointer to the adjacency entry array's base; this adjacency entry array must be
	 *                  associated with this graph.
	 * @return an iterator pointing to the entry for the registered adjacency entry array in the list of registered
	 *         adjacency entry arrays. This iterator is required for unregistering the adjacency entry array again.
	 */
	ListIterator<AdjEntryArrayBase*> registerArray(AdjEntryArrayBase *pAdjArray) const;

	//! Registers a graph observer (e.g. a ClusterGraph).
	/**
	 * @param pStructure is a pointer to the graph observer that shall be registered; this graph observer must be
	 *                   associated with this graph.
	 * @return an iterator pointing to the entry for the registered graph observer in the list of registered
	 *         graph observers. This iterator is required for unregistering the graph observer again.
	 */
	ListIterator<GraphObserver*> registerStructure(GraphObserver *pStructure) const;

	//! Unregisters a node array.
	/**
	 * @param it is an iterator pointing to the entry in the list of registered node arrays for the node array to
	 *        be unregistered.
	 */
	void unregisterArray(ListIterator<NodeArrayBase*> it) const;

	//! Unregisters an edge array.
	/**
	 * @param it is an iterator pointing to the entry in the list of registered edge arrays for the edge array to
	 *        be unregistered.
	 */
	void unregisterArray(ListIterator<EdgeArrayBase*> it) const;

	//! Unregisters an adjEntry array.
	/**
	 * @param it is an iterator pointing to the entry in the list of registered adjacency entry arrays for the
	 *           adjacency entry array to be unregistered.
	 */
	void unregisterArray(ListIterator<AdjEntryArrayBase*> it) const;

	//! Unregisters a graph observer.
	/**
	 * @param it is an iterator pointing to the entry in the list of registered graph observers for the graph
	 *           observer to be unregistered.
	 */
	void unregisterStructure(ListIterator<GraphObserver*> it) const;

	//! Move the registration \p it of an graph element array to \p pArray (used with move semantics for graph element arrays).
	template<class ArrayBase>
	void moveRegisterArray(ListIterator<ArrayBase*> it, ArrayBase *pArray) const {
#ifndef OGDF_MEMORY_POOL_NTS
		std::lock_guard<std::mutex> guard(m_mutexRegArrays);
#endif
		*it = pArray;
	}

	//! Resets the edge id count to \p maxId.
	/**
	 * The next edge will get edge id \p maxId +1. Use this function with caution!
	 * It is provided as an efficient way to reduce the edge id count. The Graph class
	 * increments the edge id count whenever an edge is created; free edge ids resulting
	 * from removing edges are not reused (there is not something like a freelist).
	 *
	 * This function is , e.g., useful, when a lot of edges has been added and
	 * <em>all</em> these edges are removed again (without creating other new edges
	 * meanwile). Then, it is safe to reduce the edge id count to the value it had
	 * before, cf. the following code snippet:
	 * \code
	 *   int oldIdCount = G.maxEdgeIndex();
	 *   Create some edges
	 *   ...
	 *   Remove all these edges again
	 *   G.resetEdgeIdCount(oldIdCount);
	 * \endcode
	 *
	 * Reducing the edge id count will reduce the memory consumption of edge arrays
	 * associated with the graph.
	 *
	 * \pre -1 <= \p maxId <= maximal edge id in the graph.
	 *
	 * @param maxId is an upper bound of the edge ids in the graph.
	 */
	void resetEdgeIdCount(int maxId);


	//@}
	/**
	 * @name Operators
	 */
	//@{
	//! Assignment operator.
	/**
	 * The assignment operature assures that the adjacency lists of nodes in the
	 * constructed graph are in the same order as the adjacency lists in \p G.
	 * This is in particular important when dealing with embedded graphs.
	 *
	 * @param G is the graph to be copied.
	 * @return this graph.
	 */
	Graph &operator=(const Graph &G);

	OGDF_MALLOC_NEW_DELETE

	//@}

public:

	//! Info structure for maintaining connected components.
	class OGDF_EXPORT CCsInfo {

		const Graph *m_graph; //!< points to the associated graph.
		int m_numCC; //!< the number of connected components.

		Array<node> m_nodes; //!< array of all nodes.
		Array<edge> m_edges; //!< array of all edges.
		Array<int>  m_startNode; //!< start node of each connected component in m_nodes.
		Array<int>  m_startEdge; //!< start edge of each connected component in m_edges.

	public:
		//! Creates a info structure associated with no graph.
		CCsInfo() : m_graph(nullptr), m_numCC(0) { }

		//! Creates a info structure associated with graph \p G.
		explicit CCsInfo(const Graph& G);

		//! Returns the associated graph.
		const Graph &constGraph() const { return *m_graph; }

		//! Returns the number of connected components.
		int numberOfCCs() const { return m_numCC; }

		//! Returns the number of nodes in connected component \p cc.
		int numberOfNodes(int cc) const { return stopNode(cc) - startNode(cc); }

		//! Returns the number of edges in connected component \p cc.
		int numberOfEdges(int cc) const { return stopEdge(cc) - startEdge(cc); }

		//! Returns the index of the first node in connected component \p cc.
		int startNode(int cc) const { return m_startNode[cc]; }

		//! Returns the index of (one past) the last node in connected component \p cc.
		int stopNode (int cc) const { return m_startNode[cc+1]; }

		//! Returns the index of the first edge in connected component \p cc.
		int startEdge(int cc) const { return m_startEdge[cc]; }

		//! Returns the index of (one past) the last edge in connected component \p cc.
		int stopEdge (int cc) const { return m_startEdge[cc+1]; }

		//! Returns the node with index \p i.
		node v(int i) const { return m_nodes[i]; }

		//! Returns the edge with index \p i.
		edge e(int i) const { return m_edges[i]; }
	};

protected:
	void construct(const Graph &G, NodeArray<node> &mapNode, EdgeArray<edge> &mapEdge);

	void assign(const Graph &G, NodeArray<node> &mapNode, EdgeArray<edge> &mapEdge);

	//! Constructs a copy of the subgraph of \p G induced by \p nodeList.
	/**
	 * This method preserves the order in the adjacency lists, i.e., if
	 * \p G is embedded, its embedding induces the embedding of the copy.
	 */
	void constructInitByNodes(
		const Graph &G,
		const List<node> &nodeList,
		NodeArray<node> &mapNode,
		EdgeArray<edge> &mapEdge);

	void constructInitByActiveNodes(
		const List<node> &nodeList,
		const NodeArray<bool> &activeNodes,
		NodeArray<node> &mapNode,
		EdgeArray<edge> &mapEdge);

	//! Constructs a copy of connected component \p cc in \p info.
	void constructInitByCC(
		const CCsInfo &info,
		int cc,
		NodeArray<node> &mapNode,
		EdgeArray<edge> &mapEdge);

private:
	void copy(const Graph &G, NodeArray<node> &mapNode,
		EdgeArray<edge> &mapEdge);
	void copy(const Graph &G);

	edge createEdgeElement(node v, node w, adjEntry adjSrc, adjEntry adjTgt);
	node pureNewNode();

	// moves adjacency entry to node w
	void moveAdj(adjEntry adj, node w);

	//! Sets the sizes of registered node and edge arrays to the
	//! next power of two that is no less than the current id counts.
	//! Respects the minimum table size constants.
	void resetTableSizes();

	//! Re-initializes registed arrays with respect to the current sizes.
	//! Calls #resetTableSizes() if \p doResetTableSizes is \c true (default).
	void reinitArrays(bool doResetTableSizes = true);
	void reinitStructures();
	void resetAdjEntryIndex(int newIndex, int oldIndex);

	/**
	 * Used to restore all hidden edges upon deleting the graph.
	 */
	void restoreAllEdges();
};

OGDF_EXPORT std::ostream & operator<<(std::ostream &os, const Graph::EdgeType &et);

//! Bucket function using the index of an edge's source node as bucket.
class OGDF_EXPORT BucketSourceIndex : public BucketFunc<edge> {
public:
	//! Returns source index of \p e.
	int getBucket(const edge &e) override { return e->source()->index(); }
};

//! Bucket function using the index of an edge's target node as bucket.
class OGDF_EXPORT BucketTargetIndex : public BucketFunc<edge> {
public:
	//! Returns target index of \p e.
	int getBucket(const edge &e) override { return e->target()->index(); }
};

namespace internal {

template<typename CONTAINER>
inline void getAllNodes(const Graph& G, CONTAINER& nodes) {
	nodes.clear();
	for (node v : G.nodes) {
		nodes.pushBack(v);
	}
}

template<>
inline void getAllNodes(const Graph& G, Array<node>& nodes) {
	nodes.init(G.numberOfNodes());
	int i = 0;
	for (node v : G.nodes) {
		nodes[i++] = v;
	}
}

template<typename CONTAINER>
inline void getAllEdges(const Graph& G, CONTAINER& edges) {
	edges.clear();
	for (edge v : G.edges) {
		edges.pushBack(v);
	}
}

template<>
inline void getAllEdges(const Graph& G, Array<edge>& edges) {
	edges.init(G.numberOfEdges());
	int i = 0;
	for (edge v : G.edges) {
		edges[i++] = v;
	}
}

}

struct NodePair {
	node source = nullptr;
	node target = nullptr;
	NodePair() = default;
	NodePair(node src, node tgt) : source(src), target(tgt) {}
};

inline std::ostream &operator<<(std::ostream &os, const NodePair& np) {
	os << "(" << np.source << "," << np.target << ")";
	return os;
}

}
