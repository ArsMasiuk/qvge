/** \file
 * \brief Declaration of graph copy classes.
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

#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/SList.h>
#include <ogdf/basic/CombinatorialEmbedding.h>


namespace ogdf {

template<bool b> class FaceSet;


/**
 * \brief Copies of graphs with mapping between nodes and edges
 *
 * @ingroup graphs
 *
 * The class GraphCopySimple represents a copy of a graph and
 * maintains a mapping between the nodes and edges of the original
 * graph to the copy and vice versa.
 *
 * New nodes and edges can be added to the copy; the counterpart
 * of those nodes and edges is 0 indicating that there is no counterpart.
 * This class <b>does not</b> support splitting of edges in such a way
 * that both edges resulting from the split are mapped to the same
 * original edge; this feature is provided by GraphCopy.
 */
class OGDF_EXPORT GraphCopySimple : public Graph
{
	const Graph *m_pGraph;   //!< The original graph.
	NodeArray<node> m_vOrig; //!< The corresponding node in the original graph.
	NodeArray<node> m_vCopy; //!< The corresponding node in the graph copy.
	EdgeArray<edge> m_eOrig; //!< The corresponding edge in the original graph.
	EdgeArray<edge> m_eCopy; //!< The corresponding edge in the graph copy.

public:
	//! Constructs a copy of graph \p G.
	explicit GraphCopySimple(const Graph &G);

	//! Copy constructor.
	GraphCopySimple(const GraphCopySimple &GC);

	virtual ~GraphCopySimple() { }

	//! Re-initializes the copy using \p G.
	void init(const Graph &G);

	//! Returns a reference to the original graph.
	const Graph &original() const { return *m_pGraph; }

	/**
	 * \brief Returns the node in the original graph corresponding to \p v.
	 * @param v is a node in the graph copy.
	 * \return the corresponding node in the original graph, or 0 if no
	 *         such node exists.
	 */
	node original(node v) const { return m_vOrig[v]; }

	/**
	 * \brief Returns the edge in the original graph corresponding to \p e.
	 * @param e is an edge in the graph copy.
	 * \return the corresponding edge in the original graph, or 0 if no
	 *         such edge exists.
	 */
	edge original(edge e) const { return m_eOrig[e]; }

	/**
	* Returns the adjacency entry in the original graph corresponding to \p adj.
	*
	* Note that this method does not pay attention to reversed edges.
	* Given a source (target) adjacency entry, the source (target) adjacency entry of the
	* original edge is returned.
	*
	* @param adj is an adjacency entry in the copy graph.
	* \return the corresponding adjacency entry in the original graph.
	*/
	adjEntry original(adjEntry adj) const {
		edge f = m_eOrig[adj->theEdge()];
		return adj->isSource() ? f->adjSource() : f->adjTarget();
	}

	/**
	 * \brief Returns the node in the graph copy corresponding to \p v.
	 * @param v is a node in the original graph.
	 * \return the corresponding node in the graph copy,
	 * or \c nullptr if it doesn't exists.
	 */
	node copy(node v) const { return m_vCopy[v]; }

	/**
	 * \brief Returns the edge in the graph copy corresponding to \p e.
	 * @param e is an edge in the original graph.
	 * \return the corresponding edge in the graph copy,
	 * or \c nullptr if it doesn't exists.
	 */
	edge copy(edge e) const { return m_eCopy[e]; }

	/**
	 * Returns the adjacency entry in the graph copy corresponding to \p adj.
	 *
	 * Note that this method does not pay attention to reversed edges.
	 * Given a source (target) adjacency entry, the source (target) adjacency entry of the
	 * copy edge is returned.
	 *
	 * @param adj is an adjacency entry in the original graph.
	 * \return the corresponding adjacency entry in the graph copy,
	 * or \c nullptr if it doesn't exists.
	 */
	adjEntry copy(adjEntry adj) const {
		edge f = m_eCopy[adj->theEdge()];
		if (f == nullptr) { return nullptr; }
		return adj->isSource() ? f->adjSource() : f->adjTarget();
	}

	/**
	 * \brief Returns true iff \p v has no corresponding node in the original graph.
	 * @param v is a node in the graph copy.
	 */
	bool isDummy(node v) const { return m_vOrig[v] == nullptr; }

	/**
	 * \brief Returns true iff \p e has no corresponding edge in the original graph.
	 * @param e is an edge in the graph copy.
	 */
	bool isDummy(edge e) const { return m_eOrig[e] == nullptr; }

	//! Assignment operator.
	GraphCopySimple &operator=(const GraphCopySimple &GC);

	/**
	 * \brief Creates a new node in the graph copy with original node \p vOrig.
	 * \warning You have to make sure that the original node makes sense, in
	 *   particular that \p vOrig is not the original node of another node in the copy.
	 */
	node newNode(node vOrig) {
		OGDF_ASSERT(vOrig != nullptr);
		OGDF_ASSERT(vOrig->graphOf() == m_pGraph);
		node v = Graph::newNode();
		m_vCopy[m_vOrig[v] = vOrig] = v;
		return v;
	}

	using Graph::newNode;

	/**
	 * \brief Creates a new edge in the graph copy with original edge \p eOrig.
	 * \warning You have to make sure that the original edge makes sense, in
	 *   particular that \p eOrig is not the original edge of another edge in the copy.
	 */
	edge newEdge(edge eOrig) {
		OGDF_ASSERT(eOrig != nullptr);
		OGDF_ASSERT(eOrig->graphOf() == m_pGraph);

		edge e = Graph::newEdge(m_vCopy[eOrig->source()], m_vCopy[eOrig->target()]);
		m_eCopy[m_eOrig[e] = eOrig] = e;

		return e;
	}

	using Graph::newEdge;

	/**
	 * \brief Removes edge \p e.
	 *
	 * \param e is an edge in the graph copy.
	 */
	virtual void delEdge(edge e) override;

	/**
	 * \brief Removes node \p v.
	 *
	 * \param v is a node in the graph copy.
	 */
	virtual void delNode(node v) override;

private:
	void initGC(const GraphCopySimple &GC, NodeArray<node> &vCopy,
		EdgeArray<edge> &eCopy);
};

/**
 * \brief Copies of graphs supporting edge splitting
 *
 * @ingroup graphs
 *
 * The class GraphCopy represents a copy of a graph and
 * maintains a mapping between the nodes and edges of the original
 * graph to the copy and vice versa.
 *
 * New nodes and edges can be added to the copy; the counterpart
 * of those nodes and edges is 0 indicating that there is no counterpart.
 * GraphCopy also support splitting of edges in such a way
 * that both edges resulting from the split are mapped to the same
 * original edge, and each edge of the original graph is mapped to a list
 * of edges. Furthermore, it is allowed to reverse edges in the graph copy.
 *
 * <h3>Do's and Dont's</h3>
 * Here is a short summary, what can be done with GraphCopy, and what should not
 * be done. The following operations are safely supported:
 *   - Splitting of edges such that an original edge is represented by a path
 *     in the copy (split(), unsplit()).
 *   - Reversing edges in the copy (Graph::reverseEdge(), Graph::reverseAllEdges()).
 *   - Reinsertion of original edges such that they are represented by a path
 *     in the copy (insertEdgePath(), insertEdgePathEmbedded(), removeEdgePath(),
 *     removeEdgePathEmbedded()).
 *   - Inserting and removing dummy edges in the copy which are not associated
 *     with edges in the original graph.
 *
 * The following operations are <b>not supported</b> and are thus dangerous:
 *   - Any modifications on the original graph, since the copy will not be
 *     notified.
 *   - Moving the source or target node of an edge in the copy to a different node.
 *   - Removing edges in the graph copy that belong to a path representing an
 *     original edge.
 *   - ... (better think first!)
 */
class OGDF_EXPORT GraphCopy : public Graph {
protected:

	const Graph *m_pGraph;   //!< The original graph.
	NodeArray<node> m_vOrig; //!< The corresponding node in the original graph.
	EdgeArray<edge> m_eOrig; //!< The corresponding edge in the original graph.
	EdgeArray<ListIterator<edge> > m_eIterator; //!< The position of copy edge in the list.

	NodeArray<node> m_vCopy; //!< The corresponding node in the graph copy.
	EdgeArray<List<edge> > m_eCopy; //!< The corresponding list of edges in the graph copy.

public:
	//! Creates a graph copy of \p G.
	/**
	 * See #init for details.
	 */
	explicit GraphCopy(const Graph &G);

	//! Default constructor (does nothing!).
	GraphCopy() : Graph(), m_pGraph(nullptr) { }

	//! Copy constructor.
	/**
	 * Creates a graph copy that is a copy of \p GC and represents a graph
	 * copy of the original graph of \p GC.
	 */
	GraphCopy(const GraphCopy &GC);

	virtual ~GraphCopy() { }


	/**
	 * @name Mapping between original graph and copy
	 */
	//@{

	//! Returns a reference to the original graph.
	const Graph &original() const { return *m_pGraph; }

	/**
	 * \brief Returns the node in the original graph corresponding to \p v.
	 * @param v is a node in the graph copy.
	 * \return the corresponding node in the original graph, or 0 if no
	 *         such node exists.
	 */
	node original(node v) const { return m_vOrig[v]; }

	/**
	 * \brief Returns the edge in the original graph corresponding to \p e.
	 * @param e is an edge in the graph copy.
	 * \return the corresponding edge in the original graph, or 0 if no
	 *         such edge exists.
	 */
	edge original(edge e) const { return m_eOrig[e]; }

	/**
	* Returns the adjacency entry in the original graph corresponding to \p adj.
	*
	* Note that this method does not pay attention to reversed edges.
	* Given a source (target) adjacency entry, the source (target) adjacency entry of the
	* original edge is returned.
	*
	* This method must not be called on inner adjacency entries of a
	* copy chain but only on a chain's source/target entry.
	*
	* @param adj is an adjacency entry in the copy graph.
	* \return the corresponding adjacency entry in the original graph.
	*/
	adjEntry original(adjEntry adj) const {
		edge e = adj->theEdge();
		edge f = m_eOrig[e];

		if (adj->isSource()) {
			OGDF_ASSERT(m_eCopy[f].front() == e);
			return f->adjSource();
		} else {
			OGDF_ASSERT(m_eCopy[f].back() == e);
			return f->adjTarget();
		}
	}

	/**
	 * \brief Returns the node in the graph copy corresponding to \p v.
	 * @param v is a node in the original graph.
	 * \return the corresponding node in the graph copy.
	 */
	node copy(node v) const { return m_vCopy[v]; }

	/**
	 * \brief Returns the list of edges coresponding to edge \p e.
	 * \param e is an edge in the original graph.
	 * \return the corresponding list of edges in the graph copy.
	 */
	const List<edge> &chain(edge e) const { return m_eCopy[e]; }

	// returns first edge in chain(e)
	/**
	 * \brief Returns the first edge in the list of edges coresponding to edge \p e.
	 * @param e is an edge in the original graph.
	 * \return the first edge in the corresponding list of edges in
	 * the graph copy or nullptr if it does not exist.
	 */
	edge copy(edge e) const { return m_eCopy[e].empty() ? nullptr : m_eCopy[e].front(); }

	/**
	* Returns the adjacency entry in the copy graph corresponding to \p adj.
	*
	* Note that this method does not pay attention to reversed edges.
	* Given a source (target) adjacency entry, the first (last) source (target) adjacency entry of the
	* copy chain is returned.
	*
	* @param adj is an adjacency entry in the copy graph.
	* \return the corresponding adjacency entry in the original graph.
	*/
	adjEntry copy(adjEntry adj) const {
		edge e = adj->theEdge();

		if (adj->isSource()) {
			return m_eCopy[e].front()->adjSource();
		} else {
			return m_eCopy[e].back()->adjTarget();
		}
	}

	/**
	 * \brief Returns true iff \p v has no corresponding node in the original graph.
	 * @param v is a node in the graph copy.
	 */
	bool isDummy(node v) const { return m_vOrig[v] == nullptr; }

	/**
	 * \brief Returns true iff \p e has no corresponding edge in the original graph.
	 * @param e is an edge in the graph copy.
	 */
	bool isDummy(edge e) const { return m_eOrig[e] == nullptr; }

	/**
	 * \brief Returns true iff edge \p e has been reversed.
	 * @param e is an edge in the original graph.
	 */
	bool isReversed (edge e) const {
		return e->source() != original(copy(e)->source());
	}

	/**
	 * \brief Returns true iff \p e is reversed w.r.t. the original edge of \a e.
	 * This method should be used, if the copy edge is split and \p e is part of the chain of the original edge.
	 * This method assumes that the list of copy edges forms a chain
	 * \param e is an edge in the graphcopy
	 */
	bool isReversedCopyEdge (edge e) const;


	/**
	 * @name Creation and deletion of nodes and edges
	 */
	//@{

	/**
	 * \brief Creates a new node in the graph copy with original node \p vOrig.
	 * \warning You have to make sure that the original node makes sense, in
	 *   particular that \p vOrig is not the original node of another node in the copy.
	 */
	node newNode(node vOrig) {
		OGDF_ASSERT(vOrig != nullptr);
		OGDF_ASSERT(vOrig->graphOf() == m_pGraph);
		node v = Graph::newNode();
		m_vCopy[m_vOrig[v] = vOrig] = v;
		return v;
	}

	using Graph::newNode;

	/**
	 * \brief Removes node \p v and all its adjacent edges cleaning-up their corresponding lists of original edges.
	 *
	 * \pre The corresponding lists oforiginal edges contain each only one edge.
	 * \param v is a node in the graph copy.
	 */
	virtual void delNode(node v) override;

	/**
	 * \brief Removes edge e and clears the list of edges corresponding to \p e's original edge.
	 *
	 * \pre The list of edges corresponding to \p e's original edge contains only \a e.
	 * \param e is an edge in the graph copy.
	 */
	virtual void delEdge(edge e) override;


	virtual void clear() override;

	/**
	 * \brief Splits edge \p e. See Graph::split for details.
	 * Both resulting edges have the same original edge.
	 * @param e is an edge in the graph copy.
	 */
	virtual edge split(edge e) override;


	/**
	 * \brief Undoes a previous split operation.
	 * The two edges \p eIn and \p eOut are merged to a single edge \a eIn.
	 * \pre The vertex \a u that was created by the previous split operation has
	 *      exactly one incoming edge \p eIn and one outgoing edge \p eOut.
	 * @param eIn is an edge (*,\a u) in the graph copy.
	 * @param eOut is an edge (\a u,*) in the graph copy.
	 */
	void unsplit(edge eIn, edge eOut) override;

	//! Creates a new edge (\a v,\a w) with original edge \a eOrig.
	edge newEdge(edge eOrig);

	using Graph::newEdge;

	//! sets eOrig to be the corresponding original edge of eCopy and vice versa
	/**
	 * @param eOrig is the original edge
	 * @param eCopy is the edge copy
	 */
	void setEdge(edge eOrig, edge eCopy);

	//! Embeds the graph copy.
	OGDF_DEPRECATED("Use ogdf::planarEmbed() instead.")
	bool embed();

	//! Removes all crossing nodes which are actually only two "touching" edges.
	void removePseudoCrossings();

	//! Re-inserts edge \p eOrig by "crossing" the edges in \p crossedEdges.
	/**
	 * Let \a v and \a w be the copies of the source and target nodes of \p eOrig.
	 * Each edge in \p crossedEdges is split creating a sequence
	 * \a u_1, ..., \a u_k of new nodes, and additional edges are inserted creating
	 * a path \a v, \a u_1, ..., \a u_k, \a w.
	 * @param eOrig is an edge in the original graph and becomes the original edge of
	 *        all edges in the path \a v, \a u_1, ..., \a u_k, \a w.
	 * @param crossedEdges are edges in the graph copy.
	 */
	void insertEdgePath(edge eOrig, const SList<adjEntry> &crossedEdges);

	//! Special version (for FixedEmbeddingUpwardEdgeInserter only).
	void insertEdgePath(node srcOrig, node tgtOrig, const SList<adjEntry> &crossedEdges);


	//! Removes the complete edge path for edge \p eOrig.
	/**
	 * \@param eOrig is an edge in the original graph.
	 */
	void removeEdgePath(edge eOrig);

	//! Inserts crossings between two copy edges.
	/**
	 * This method is used in TopologyModule.
	 *
	 * Let \p crossingEdge = (\a a, \a b) and \p crossedEdge = (\a v, \a w).
	 * Then \p crossedEdge is split creating two edges \p crossedEdge = (\a v, \a u)
	 * and (\a u, \a w), \p crossingEdge is removed and replaced by two new edges
	 * \a e1  = (\a a, \a u) and \a e2 = (\a u, \a b).
	 * Finally it sets \p crossingEdge to \a e2 and returns (\a u, \a w).
	 *
	 * @param crossingEdge is the edge that is replaced by two new edges.
	 *                     Note that this parameter will be modified to equal the newly created edge (\a u, \a b).
	 * @param crossedEdge is the edge that gets split.
	 * @param rightToLeft is used as follows: If set to true, \p crossingEdge will cross
	 *        \p crossedEdge from right to left, otherwise from left to right.
	 * @return the rear edge resulting from the split operation: (\a u, \a w)
	*/
	edge insertCrossing(
		edge& crossingEdge,
		edge crossedEdge,
		bool rightToLeft);


	/**
	 * @name Combinatorial Embeddings
	 */
	//@{

	//! Creates a new edge with original edge \p eOrig in an embedding \p E.
	/**
	 * Let \a w be the node whose adjacency list contains \a adjTgt. The original
	 * edge \p eOrig must connect the original nodes of \p v and \a w. If \p eOrig =
	 * (original(\p v),original(\a w)), then the created edge is (\p v,\a w), otherwise
	 * it is (\a w,\p v). The new edge \a e must split a face in \p E, such that \a e
	 * comes after \p adj in the adjacency list of \p v and at the end of the adjacency
	 * list of \p v.
	 *
	 * @param v is a node in the graph copy.
	 * @param adj is an adjacency entry in the graph copy.
	 * @param eOrig is an edge in the original graph.
	 * @param E is an embedding of the graph copy.
	 * @return the created edge.
	 */
	edge newEdge(node v, adjEntry adj, edge eOrig, CombinatorialEmbedding &E);

	/**
	 * \brief Sets the embedding of the graph copy to the embedding of the original graph.
	 * \pre The graph copy has not been changed after construction, i.e., no new nodes
	 *      or edges have been added and no edges have been split.
	 */
	void setOriginalEmbedding();

	//! Re-inserts edge \p eOrig by "crossing" the edges in \p crossedEdges in embedding \p E.
	/**
	 * Let \a v and \a w be the copies of the source and target nodes of \p eOrig,
	 * and let \a e_0, \a e_1, ..., \a e_k, \a e_{k+1} be the sequence of edges corresponding
	 * to the adjacency entries in \p crossedEdges. The first edge needs to be incident
	 * to \a v and the last to \a w; the edges \a e_1, ..., \a e_k are each split
	 * creating a sequence \a u_1, ..., \a u_k of new nodes, and additional edges
	 * are inserted creating a path \a v, \a u_1, ..., \a u_k, \a w.
	 *
	 * The following figure illustrates, which adjacency entries need to be in the list
	 * \p crossedEdges. It inserts an edge connecting \a v and \a w by passing through
	 * the faces \a f_0, \a f_1, \a f_2; in this case, the list \p crossedEdges must contain
	 * the adjacency entries \a adj_0, ..., \a adj_3 (in this order).
	 * \image html insertEdgePathEmbedded.png
	 *
	 * @param eOrig is an edge in the original graph and becomes the original edge of
	 *        all edges in the path \a v, \a u_1, ..., \a u_k, \a w.
	 * @param E is an embedding of the graph copy.
	 * @param crossedEdges are a list of adjacency entries in the graph copy.
	 */
	void insertEdgePathEmbedded(
		edge eOrig,
		CombinatorialEmbedding &E,
		const SList<adjEntry> &crossedEdges);

	//! Removes the complete edge path for edge \p eOrig while preserving the embedding.
	/**
	 * @param E is an embedding of the graph copy.
	 * @param eOrig is an edge in the original graph.
	 * @param newFaces is assigned the set of new faces resulting from joining faces
	 *        when removing edges.
	 */
	void removeEdgePathEmbedded(
		CombinatorialEmbedding &E,
		edge                    eOrig,
		FaceSet<false>         &newFaces);


	//@}
	/**
	 * @name Miscellaneous
	 */
	//@{

#ifdef OGDF_DEBUG
	//! Asserts that this copy is consistent.
	void consistencyCheck() const;
#endif

	//! Re-initializes the copy using the graph \p G.
	/**
	 * This method assures that the adjacency lists of nodes in the
	 * constructed copy are in the same order as the adjacency lists in \p G.
	 * This is in particular important when dealing with embedded graphs.
	 *
	 * \param G the graph to be copied
	 */
	void init(const Graph &G);

	//! Associates the graph copy with \p G, but does not create any nodes or edges.
	/**
	 * By using this method, the graph copy gets associated with \p G.
	 * This does not modify the existing nodes and edges, but they become dummies.
	 *
	 * The following code snippet shows a typical application of this functionality:
	 * \code
	 *   GraphCopy GC;
	 *   GC.createEmpty(G);
	 *
	 *   // compute connected components of G
	 *   NodeArray<int> component(G);
	 *   int numCC = connectedComponents(G,component);
	 *
	 *   // intialize the array of lists of nodes contained in a CC
	 *   Array<List<node> > nodesInCC(numCC);
	 *
	 *   for(node v : G.nodes)
	 *	   nodesInCC[component[v]].pushBack(v);
	 *
	 *   EdgeArray<edge> auxCopy(G);
	 *   Array<DPoint> boundingBox(numCC);
	 *
	 *   for(int i = 0; i < numCC; ++i) {
	 *     GC.initByNodes(nodesInCC[i],auxCopy);
	 *     ...
	 *   }
	 * \endcode
	 * @param G is the graph of which this graph copy shall be a copy.
	 */
	void createEmpty(const Graph &G);

	//! Initializes the graph copy for the nodes in component \p cc.
	/**
	 * @param info  must be a connected component info structure for the original graph.
	 * @param cc    is the number of the connected component.
	 * @param eCopy is assigned a mapping from original to copy edges.
	 */
	void initByCC(const CCsInfo &info, int cc, EdgeArray<edge> &eCopy);

	//! Initializes the graph copy for the nodes in a component.
	/**
	 * Creates copies of all nodes in \p origNodes and their incident edges.
	 * Any nodes and edges allocated before are removed.
	 *
	 * The order of entries in the adjacency lists is preserved, i.e., if
	 * the original graph is embedded, its embedding induces the embedding
	 * of the created copy.
	 *
	 * It is important that \p origNodes is the complete list of nodes in
	 * a connected component. If you wish to initialize the graph copy for an
	 * arbitrary set of nodes, use the method initByActiveNodes().
	 * \see createEmpty() for an example.
	 * @param origNodes is the list of nodes in the original graph for which
	 *        copies are created in the graph copy.
	 * @param eCopy is assigned the copy of each original edge.
	 */
	void initByNodes(const List<node> &origNodes, EdgeArray<edge> &eCopy);

	//! Initializes the graph copy for the nodes in \p nodeList.
	/**
	 * Creates copies of all nodes in \p nodeList and edges between two nodes
	 * which are both contained in \p nodeList.
	 * Any nodes and edges allocated before are destroyed.
	 *
	 * \see createEmpty()
	 * @param nodeList is the list of nodes in the original graph for which
	 *        copies are created in the graph copy.
	 * @param activeNodes must be true for every node in \p nodeList, false
	 *        otherwise.
	 * @param eCopy is assigned the copy of each original edge.
	 */
	void initByActiveNodes(const List<node> &nodeList,
		const NodeArray<bool> &activeNodes, EdgeArray<edge> &eCopy);

	//@}
	/**
	 * @name Operators
	 */
	//@{

	//! Assignment operator.
	/**
	 * Creates a graph copy that is a copy of \p GC and represents a graph
	 * copy of the original graph of \p GC.
	 *
	 * The constructor assures that the adjacency lists of nodes in the
	 * constructed graph are in the same order as the adjacency lists in \a G.
	 * This is in particular important when dealing with embedded graphs.
	 */
	GraphCopy &operator=(const GraphCopy &GC);


	//@}

protected:
	void removeUnnecessaryCrossing(
		adjEntry adjA1,
		adjEntry adjA2,
		adjEntry adjB1,
		adjEntry adjB2);

private:
	void initGC(const GraphCopy &GC,
		NodeArray<node> &vCopy, EdgeArray<edge> &eCopy);
};

}
