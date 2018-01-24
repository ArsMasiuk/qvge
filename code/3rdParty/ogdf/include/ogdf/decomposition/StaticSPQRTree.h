/** \file
 * \brief Declaration of class StaticSPQRTree
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

#include <ogdf/decomposition/SPQRTree.h>
#include <ogdf/decomposition/StaticSkeleton.h>
#include <ogdf/graphalg/Triconnectivity.h>

namespace ogdf {

/**
 * \brief Linear-time implementation of static SPQR-trees.
 *
 * @ingroup decomp
 *
 * The class StaticSPQRTree maintains the arrangement of the triconnected
 * components of a biconnected multi-graph \a G [Hopcroft, Tarjan 1973]
 * as a so-called SPQR tree \a T [Fi Battista, Tamassia, 1996]. We
 * call \a G the original graph of \a T.
 * The class StaticSPQRTree supports only the statical construction
 * of an SPQR-tree for a given graph \a G, dynamic updates are
 * not supported.
 *
 * Each node of the tree has an associated type (represented by
 * SPQRTree::NodeType), which is either SNode, PNode, or
 * RNode, and a skeleton (represented by the class StaticSkeleton).
 * The skeletons of the nodes of \a T are in one-to-one
 * correspondence to the triconnected components of \a G, i.e.,
 * S-nodes correspond to polygons, P-nodes to bonds, and
 * R-nodes to triconnected graphs.
 *
 * In our representation of SPQR-trees, Q-nodes are omitted. Instead,
 * the skeleton S of a node \a v in \a T contains two types of edges:
 * real edges, which correspond to edges in \a G, and virtual edges, which
 * correspond to edges in \a T having \a v as an endpoint.
 * There is a special edge \a er in G at which \a T is rooted, i.e., the
 * root node of \a T is the node whose skeleton contains the real edge
 * corresponding to \a er.
 *
 * The reference edge of the skeleton of the root node is \a er, the
 * reference edge of the skeleton \a S of a non-root node \a v is the virtual
 * edge in \a S that corresponds to the tree edge (parent(\a v),\a v).
 */
class OGDF_EXPORT StaticSPQRTree : public virtual SPQRTree
{
public:
	friend class StaticSkeleton;

	// constructors

	/**
	 * \brief Creates an SPQR tree \a T for graph \p G rooted at the first edge of \p G.
	 * \pre \p G is biconnected and contains at least 3 nodes,
	 *      or \p G has exactly 2 nodes and at least 3 edges.
	 */
	explicit StaticSPQRTree(const Graph &G) : m_skOf(G), m_copyOf(G) { OGDF_ASSERT(G.numberOfEdges() > 0); m_pGraph = &G; init(G.firstEdge()); }

	/**
	 * \brief Creates an SPQR tree \a T for graph \p G rooted at the edge \p e.
	 * \pre \p e is in \p G, \p G is biconnected and contains at least 3 nodes,
	 *      or \p G has exactly 2 nodes and at least 3 edges.
	 */
	StaticSPQRTree(const Graph &G, edge e) : m_skOf(G), m_copyOf(G) { m_pGraph = &G; init(e); }

	/**
	 * \brief Creates an SPQR tree \a T for graph \p G rooted at the first edge of \p G.
	 * \pre \p G is biconnected and contains at least 3 nodes,
	 *      or \p G has exactly 2 nodes and at least 3 edges.
	 */
	StaticSPQRTree(const Graph &G, Triconnectivity &tricComp) : m_skOf(G), m_copyOf(G) { m_pGraph = &G; init(G.firstEdge(),tricComp); }

	//! Destructor.
	~StaticSPQRTree();

	//! \name Access operations
	//! @{

	//! Returns a reference to the original graph \a G.
	const Graph &originalGraph() const override { return *m_pGraph; }

	//! Returns a reference to the tree \a T.
	const Graph &tree() const override { return m_tree; }

	//! Returns the edge of \a G at which \a T is rooted.
	edge rootEdge() const override { return m_rootEdge; }

	//! Returns the root node of \a T.
	node rootNode() const override { return m_rootNode; }

	//! Returns the number of S-nodes in \a T.
	int numberOfSNodes() const override { return m_numS; }

	//! Returns the number of P-nodes in \a T.
	int numberOfPNodes() const override { return m_numP; }

	//! Returns the number of R-nodes in \a T.
	int numberOfRNodes() const override { return m_numR; }

	/**
	 * \brief Returns the type of node \p v.
	 * \pre \p v is a node in \a T
	 */
	NodeType typeOf(node v) const override { return m_type[v]; }

	//! Returns the list of all nodes with type \p t.
	List<node> nodesOfType(NodeType t) const override;

	/**
	 * \brief Returns the skeleton of node \p v.
	 * \pre \p v is a node in \a T
	 */
	Skeleton &skeleton(node v) const override { return *m_sk[v]; }

	/**
	 * \brief Returns the edge in skeleton of source(\p e) that corresponds to tree edge \p e.
	 * \pre \p e is an edge in \a T
	 */
	edge skeletonEdgeSrc(edge e) const { return m_skEdgeSrc[e]; }

	/**
	 * \brief Returns the edge in skeleton of target(\p e) that corresponds to tree edge \p e.
	 * \pre \p e is an edge in \a T
	 */
	edge skeletonEdgeTgt(edge e) const { return m_skEdgeTgt[e]; }

	/**
	 * \brief Returns the skeleton that contains the real edge \p e.
	 * \pre \p e is an edge in \a G
	 */
	const Skeleton &skeletonOfReal(edge e) const override { return *m_skOf[e]; }

	/**
	 * \brief Returns the skeleton edge that corresponds to the real edge \p e.
	 * \pre \p e is an edge in \a G
	 */
	edge copyOfReal(edge e) const override { return m_copyOf[e]; }

	//! @}
	//! \name Update operations
	//! @{

	/**
	 * \brief Roots \a T at edge \p e and returns the new root node of \a T.
	 * \pre \p e is an edge in \a G
	 */
	node rootTreeAt(edge e) override;

	/**
	 * \brief Roots \a T at node \p v and returns \p v.
	 * \pre \p v is a node in \a T
	 */
	node rootTreeAt(node v) override;

	//! @}

protected:

	//! Initialization (called by constructor).
	void init(edge e);

	//! Initialization (called by constructor).
	void init(edge eRef, Triconnectivity &tricComp);

	//! Recursively performs rooting of tree.
	void rootRec(node v, edge ef);

	/**
	 * \brief Recursively performs the task of adding edges (and nodes)
	 * to the pertinent graph \p Gp for each involved skeleton graph.
	 */
	void cpRec(node v, PertinentGraph &Gp) const override
	{
		const Skeleton &S = skeleton(v);

		for(edge e : S.getGraph().edges) {
			edge eOrig = S.realEdge(e);
			if (eOrig != nullptr) cpAddEdge(eOrig,Gp);
		}

		for(adjEntry adj : v->adjEntries) {
			node w = adj->theEdge()->target();
			if (w != v) cpRec(w,Gp);
		}
	}


	const Graph *m_pGraph;  //!< pointer to original graph
	Graph        m_tree;    //!< underlying tree graph

	edge m_rootEdge;  //!< edge of \a G at which \a T is rooted
	node m_rootNode;  //!< root node of \a T

	int m_numS;  //!< number of S-nodes
	int m_numP;  //!< number of P-nodes
	int m_numR;  //!< number of R-nodes

	NodeArray<NodeType> m_type;  //!< type of nodes in \a T

	NodeArray<StaticSkeleton *> m_sk;         //!< pointer to skeleton of a node in \a T
	EdgeArray<edge>             m_skEdgeSrc;  //!< corresponding edge in skeleton(source(\a e))
	EdgeArray<edge>             m_skEdgeTgt;  //!< corresponding edge in skeleton(target(\a e))

	EdgeArray<StaticSkeleton *> m_skOf;    //!< skeleton containing real edge \a e
	EdgeArray<edge>             m_copyOf;  //!< skeleton edge corresponding to real edge \a e
};

}
