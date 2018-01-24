/** \file
 * \brief Declaration of class PlanRepExpansion representing a
 *        planarized representation of the expansion of a graph.
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

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/tuples.h>
#include <ogdf/basic/SList.h>

#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceSet.h>
#include <ogdf/basic/NodeSet.h>


namespace ogdf {

/**
 * \brief Planarized representations (of a connected component) of a graph.
 *
 * @ingroup plan-rep
 *
 * Maintains types of edges (generalization, association) and nodes,
 * and the connected components of the graph.
 */
class OGDF_EXPORT PlanRepExpansion : public Graph
{
public:
	struct Crossing {
		Crossing() { m_adj = nullptr; }
		explicit Crossing(adjEntry adj) { m_adj = adj; }

		adjEntry m_adj;
		SList<adjEntry> m_partitionLeft;
		SList<adjEntry> m_partitionRight;

		friend std::ostream &operator<<(std::ostream &os, const Crossing &c) {
			os << "(" << c.m_adj << ")";
			return os;
		}
	};


	/**
	 * \brief Representation of a node split in a planarized expansion.
	 *
	 * Stores in particular the insertion path of the node split (just like the
	 * chain of an original edge).
	 */
	class NodeSplit
	{
	public:
		/**
		 * \brief Creates an empty node split.
		 */
		NodeSplit() { }

		/**
		 * \brief Creates a node split and sets its iterator in the list of all node splits.
		 */
		explicit NodeSplit(ListIterator<NodeSplit> it) : m_nsIterator(it) { }

		/**
		 * \brief Returns the first node on the node split's insertion path.
		 */
		node source() const { return m_path.front()->source(); }

		/**
		 * \brief Returns the last node on the node split's insertion path.
		 */
		node target() const { return m_path.back ()->target(); }

		List<edge> m_path; //!< The insertion path of the node split.
		ListIterator<NodeSplit> m_nsIterator; //!< This node split's iterator in the list of all node splits.
	};

	//! Pointer to a node split.
	using nodeSplit = PlanRepExpansion::NodeSplit*;


	/**
	 * \brief Creates a planarized expansion of graph \p G.
	 *
	 * All nodes are allowed to be split.
	 */
	PlanRepExpansion(const Graph& G);

	/**
	 * \brief Creates a planarized expansion of graph \p G with given splittable nodes.
	 *
	 * Only the node in \p splittableNodes are allowed to be split.
	 * @param G is the original graph of this planarized expansion.
	 * @param splittableNodes contains all the nodes in \p G that are splittable.
	 */
	PlanRepExpansion(const Graph& G, const List<node> &splittableNodes);

	~PlanRepExpansion() {}


	/**
	 * @name Acess methods
	 */
	//@{

	//! Returns a reference to the original graph.
	const Graph &original() const { return *m_pGraph; }

	//! Returns the original node of \p v, or 0 if \p v is a dummy.
	node original(node v) const { return m_vOrig[v]; }

	//! Returns the list of copy nodes of \p vOrig.
	const List<node> &expansion(node vOrig) const { return m_vCopy[vOrig]; }

	//! Returns the first copy node of \p vOrig.
	node copy(node vOrig) const { return m_vCopy[vOrig].front(); }

	//! Returns the original edge of \p e, or 0 if \p e has none (e.g., \p e belongs to a node split).
	edge originalEdge(edge e) const { return m_eOrig[e]; }

	//! Returns the insertion path of edge \p eOrig.
	const List<edge> &chain(edge eOrig) const { return m_eCopy[eOrig]; }

	//! Returns the first edge in \p eOrig's insertion path.
	edge copy(edge eOrig) const { return m_eCopy[eOrig].front(); }

	//! Returns true iff \p v is splittable.
	bool splittable(node v) const { return m_splittable[v]; }

	//! Returns true iff \p vOrig is splittable.
	bool splittableOrig(node vOrig) const { return m_splittableOrig[vOrig]; }

	//! Returns the node split associated with \p e, or 0 if none (e.g., \p e belongs to an original edge).
	NodeSplit *nodeSplitOf(edge e) const { return m_eNodeSplit[e]; }

	//! Returns the number of node splits.
	int numberOfNodeSplits() const { return m_nodeSplits.size(); }
	int numberOfSplittedNodes() const;

	//! Returns the list of node splits.
	List<NodeSplit> &nodeSplits() { return m_nodeSplits; }

	/**
	 * \brief Sets the original edge and corresponding node split of \p e and returns the corresponding insertion path.
	 *
	 * @param e is an edge in the planarized expansion.
	 * @param eOrig is assigned the original edge of \p e (or 0 if none).
	 * @param ns is assigned the node split corresponding to \p e (or 0 if none).
	 * @return a reference to the insertion path containing \p e; this is either the insertion
	 *         path of \p eOrig (if this is not 0), or of \p ns.
	 */
	List<edge> &setOrigs(edge e, edge &eOrig, nodeSplit &ns);

	ListConstIterator<edge> position(edge e) const { return m_eIterator[e]; }

	bool isPseudoCrossing(node v) const;

	//! Computes the number of crossings.
	int computeNumberOfCrossings() const;

	//@}
	/**
	 * @name Processing connected components
	 */
	//@{


	 /**
	 * \brief Returns the number of connected components in the original graph.
	 */
	int numberOfCCs() const {
		return m_nodesInCC.size();
	}

	/**
	 * \brief Returns the index of the current connected component (-1 if not yet initialized).
	 */
	int currentCC() const {
		return m_currentCC;
	}

	/**
	 * \brief Returns the list of (original) nodes in connected component \p i.
	 *
	 * Note that connected components are numbered 0,1,...
	 */
	const List<node> &nodesInCC(int i) const {
		return m_nodesInCC[i];
	}

	/**
	 * \brief Returns the list of (original) nodes in the current connected component.
	 */
	const List<node> &nodesInCC() const {
		return m_nodesInCC[m_currentCC];
	}

	/**
	 * \brief Initializes the planarized representation for connected component \p i.
	 *
	 * This initialization is always required. After performing this initialization,
	 * the planarized representation represents a copy of the <i>i</i>-th connected
	 * component of the original graph, where connected components are numbered
	 * 0,1,2,...
	 */
	void initCC(int i);


	//@}
	/**
	 * @name Update operations
	 */
	//@{

	edge split(edge e) override;

	void unsplit(edge eIn, edge eOut) override;

	//! Removes edge \p e from the planarized expansion.
	virtual void delEdge(edge e) override;

	//! Embeds the planarized expansion; returns true iff it is planar.
	bool embed();

	void insertEdgePath(
		edge eOrig,
		nodeSplit ns,
		node vStart,
		node vEnd,
		List<Crossing> &eip,
		edge eSrc,
		edge eTgt);

	/**
	 * \brief Inserts an edge or a node split according to insertion path \p crossedEdges.
	 *
	 * If \p eOrig is not 0, a new insertion path for \p eOrig is inserted; otherwise,
	 * a new insertion path for node split \p ns is inserted.
	 * @param eOrig is an original edge in the graph (or 0).
	 * @param ns is a node split in the planarized expansion.
	 * @param E is an embedding of the planarized expansion.
	 * @param crossedEdges defines the insertion path.
	 * \pre Not both \p eOrig and \p ns may be 0.
	 * \see GraphCopy::insertEdgePathEmbedded() for a specification of \p crossedEdges.
	 */
	void insertEdgePathEmbedded(
		edge eOrig,
		nodeSplit ns,
		CombinatorialEmbedding &E,
		const List<Tuple2<adjEntry,adjEntry> > &crossedEdges);

	/**
	 * \brief Removes the insertion path of \p eOrig or \p ns.
	 *
	 * @param E is an embedding of the planarized expansion.
	 * @param eOrig is an original edge in the graph (or 0).
	 * @param ns is a node split in the planarized expansion.
	 * @param newFaces is assigned the set of new faces resulting from joining faces when removing edges.
	 * @param mergedNodes is assigned the set of nodes in the planarized expansion that resulted
	 *        from merging (splittable) nodes.
	 * @param oldSrc is assigned the source node of the removed insertion path.
	 * @param oldTgt is assigned the target node of the removed insertion path.
	 * \pre Not both \p eOrig and \p ns may be 0.
	 */
	void removeEdgePathEmbedded(
		CombinatorialEmbedding &E,
		edge eOrig,
		nodeSplit ns,
		FaceSet<false> &newFaces,
		NodeSet<false> &mergedNodes,
		node &oldSrc,
		node &oldTgt);

	/**
	 * \brief Removes the insertion path of \p eOrig or \p ns.
	 *
	 * @param eOrig is an original edge in the graph (or 0).
	 * @param ns is a node split in the planarized expansion.
	 * @param oldSrc is assigned the source node of the removed insertion path.
	 * @param oldTgt is assigned the target node of the removed insertion path.
	 * \pre Not both \p eOrig and \p ns may be 0.
	 */
	void removeEdgePath(
		edge eOrig,
		nodeSplit ns,
		node &oldSrc,
		node &oldTgt);

	/**
	 * \brief Removes an (unneccessary) node split consisting of a single edge.
	 *
	 * Nodes splits consisting of a single edge are superfluous and canbe removed
	 * by contracting the respective edge.
	 * @param ns is the node split to be removed.
	 * @param E is an embedding of the planarized expansion.
	 */
	void contractSplit(nodeSplit ns, CombinatorialEmbedding &E);

	/**
	 * \brief Removes an (unneccessary) node split consisting of a single edge.
	 *
	 * Nodes splits consisting of a single edge are superfluous and canbe removed
	 * by contracting the respective edge.
	 * @param ns is the node split to be removed.
	 */
	void contractSplit(nodeSplit ns);

	/**
	 * \brief Unsplits a superfluous expansion node of degree 2.
	 * @param u is a node in the planarized expansion which has degree 2 and is part of the
	 *        expansion of an original node.
	 * @param eContract is the edge incident to \p u which is part of a node split; this edge
	 *        gets contracted.
	 * @param eExpand is the edge incident to \p u which belongs to the insertion path
	 *        that gets enlarged.
	 * @param E is an embedding of the planarized expansion.
	 */
	edge unsplitExpandNode(
		node u,
		edge eContract,
		edge eExpand,
		CombinatorialEmbedding &E);

	/**
	 * \brief Unsplits a superfluous expansion node of degree 2.
	 * @param u is a node in the planarized expansion which has degree 2 and is part of the
	 *        expansion of an original node.
	 * @param eContract is the edge incident to \p u which is part of a node split; this edge
	 *        gets contracted.
	 * @param eExpand is the edge incident to \p u which belongs to the insertion path
	 *        that gets enlarged.
	 */
	edge unsplitExpandNode(
		node u,
		edge eContract,
		edge eExpand);

	/**
	 * \brief Splits edge \p e and introduces a new node split starting at \p v.
	 *
	 * @param v is a node in the planarized expansion; the expansion of \p v's original
	 *        node is enlarged.
	 * @param e is the edge to be split; the insertion path of \p e's original edge
	 *        must start or end at \p v.
	 * @param E is an embedding of the planarized expansion.
	 * @return the newly created edge; the node resulting from splitting \p e is the
	 *         target node of this edge.
	 */
	edge enlargeSplit(node v, edge e, CombinatorialEmbedding &E);

	/**
	 * \brief Splits edge \p e and introduces a new node split starting at \p v.
	 *
	 * @param v is a node in the planarized expansion; the expansion of \p v's original
	 *        node is enlarged.
	 * @param e is the edge to be split; the insertion path of \p e's original edge
	 *        must start or end at \p v.
	 * @return the newly created edge; the node resulting from splitting \p e is the
	 *         target node of this edge.
	 */
	edge enlargeSplit(node v, edge e);

	/**
	 * \brief Introduces a new node split by splitting an exisiting node split.
	 *
	 * @param e is the edge to be split; the node split corresponding to \p e is split
	 *        into two node splits.
	 * @param E is an embedding of the planarized expansion.
	 * @return the newly created edge;  the node resulting from splitting \p e is the
	 *         target node of this edge.
	 */
	edge splitNodeSplit(edge e, CombinatorialEmbedding &E);

	/**
	 * \brief Introduces a new node split by splitting an exisiting node split.
	 *
	 * @param e is the edge to be split; the node split corresponding to \p e is split
	 *        into two node splits.
	 * @return the newly created edge;  the node resulting from splitting \p e is the
	 *         target node of this edge.
	 */
	edge splitNodeSplit(edge e);

	/**
	 * \brief Removes a self-loop \p e = (\a u,\a u).
	 *
	 * \a u must be a dummy node; hence, \a u has degree 2 node after removing
	 * \p e, and \a u is unsplit afterwards.
	 * @param e must be a self loop in the planarized expansion.
	 * @param E is an embedding of the planarized expansion.
	 */
	void removeSelfLoop(edge e, CombinatorialEmbedding &E);

	void removeSelfLoop(edge e);

	/**
	 * \brief Converts a dummy node \p u to a copy of an original node \p vOrig.
	 *
	 * This method is used if two copy nodes \a x and \a y of the same original node \p vOrig
	 * can be connected by converting a dummy node \p u into a copy node of \p vOrig, since an
	 * insertion path starting (or ending) at \a x crosses an insertion path starting (or
	 * ending) at \a y.
	 * @param u is a dummy node in the planarized expansion.
	 * @param vOrig is an original node.
	 * @param ns is a node split that can be reused for connecting \a x with \p u.
	 */
	PlanRepExpansion::nodeSplit convertDummy(
		node u,
		node vOrig,
		PlanRepExpansion::nodeSplit ns);

	edge separateDummy(
		adjEntry adj_1,
		adjEntry adj_2,
		node vStraight,
		bool isSrc);

	void resolvePseudoCrossing(node v);

	//@}
	/**
	 * @name Miscelleaneous
	 */
	//@{

#ifdef OGDF_DEBUG
	void consistencyCheck() const;
#endif

	//@}

private:
	void doInit(const Graph &G, const List<node> &splittableNodes);
	void prepareNodeSplit(
		const SList<adjEntry> &partitionLeft,
		adjEntry &adjLeft,
		adjEntry &adjRight);

	const Graph *m_pGraph;                      //!< The original graph.
	NodeArray<node> m_vOrig;                    //!< The corresponding node in the original graph.
	EdgeArray<edge> m_eOrig;                    //!< The corresponding edge in the original graph.
	EdgeArray<ListIterator<edge> > m_eIterator; //!< The position of copy edge in the list.
	EdgeArray<List<edge> > m_eCopy;             //!< The corresponding list of edges in the graph copy.
	NodeArray<ListIterator<node> > m_vIterator; //!< The position of copy node in the list.
	NodeArray<List<node> > m_vCopy;             //!< The corresponding list of nodes in the graph copy.

	NodeArray<bool> m_splittable;
	NodeArray<bool> m_splittableOrig;
	EdgeArray<NodeSplit *> m_eNodeSplit;
	List<NodeSplit> m_nodeSplits;

	int m_currentCC; //!< The index of the current component.
	int m_numCC;     //!< The number of components in the original graph.

	Array<List<node> >  m_nodesInCC; //!< The list of original nodes in each component.
	EdgeArray<edge>     m_eAuxCopy; // auxiliary
};

}
