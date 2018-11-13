/** \file
 * \brief Declaration of class DynamicSPQRTree
 *
 * \author Jan Papenfuß
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
#include <ogdf/decomposition/DynamicSPQRForest.h>
#include <ogdf/decomposition/DynamicSkeleton.h>

namespace ogdf {

/**
 * \brief Linear-time implementation of dynamic SPQR-trees.
 *
 * @ingroup decomp
 *
 * The class DynamicSPQRTree maintains the arrangement of the triconnected
 * components of a biconnected multi-graph \a G [Hopcroft, Tarjan 1973]
 * as a so-called SPQR tree \a T [Fi Battista, Tamassia, 1996]. We
 * call \a G the original graph of \a T.
 * The class DynamicSPQRTree supports the statical construction of
 * an SPQR-tree for a given graph G, and supports dynamic updates,
 * too.
 *
 * Each node of the tree has an associated type (represented by
 * SPQRTree::NodeType), which is either SNode, PNode, or
 * RNode, and a skeleton (represented by the class DynamicSkeleton).
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
class OGDF_EXPORT DynamicSPQRTree : public virtual SPQRTree, public DynamicSPQRForest
{
public:
	friend class DynamicSkeleton;

	// constructors

	/**
	 * \brief Creates an SPQR tree \a T for graph \p G rooted at the first edge of \p G.
	 * \pre \p G is biconnected and contains at least 3 nodes,
	 *      or \p G has exactly 2 nodes and at least 3 edges.
	 */
	explicit DynamicSPQRTree (Graph& G) : DynamicSPQRForest(G) { init(G.firstEdge()); }

	/**
	 * \brief Creates an SPQR tree \a T for graph \p G rooted at the edge \p e.
	 * \pre \p e is in \p G, \p G is biconnected and contains at least 3 nodes,
	 *      or \p G has exactly 2 nodes and at least 3 edges.
	 */
	DynamicSPQRTree (Graph& G, edge e) : DynamicSPQRForest(G) { init(e); }


	// destructor

	~DynamicSPQRTree();


	//
	// a) Access operations
	//

	//! Returns a reference to the original graph \a G.
	const Graph& originalGraph () const override { return m_G; }

	//! Returns a reference to the tree \a T.
	const Graph& tree () const override { return m_T; }

	//! Returns the edge of \a G at which \a T is rooted.
	edge rootEdge () const override { return m_rootEdge; }

	//! Returns the root node of \a T.
	node rootNode () const override { return findSPQR(m_bNode_SPQR[m_B.firstNode()]); }

	//! Returns the number of S-nodes in \a T.
	int numberOfSNodes () const override { return m_bNode_numS[m_B.firstNode()]; }

	//! Returns the number of P-nodes in \a T.
	int numberOfPNodes () const override { return m_bNode_numP[m_B.firstNode()]; }

	//! Returns the number of R-nodes in \a T.
	int numberOfRNodes () const override { return m_bNode_numR[m_B.firstNode()]; }

	/**
	 * \brief Returns the type of node \p v.
	 * \pre \p v is a node in \a T
	 */
	NodeType typeOf (node v) const override
	{
		return (NodeType)m_tNode_type[findSPQR(v)];
	}

	//! Returns the list of all nodes with type \p t.
	List<node> nodesOfType (NodeType t) const override;

	//! Finds the shortest path between the two sets of vertices of \a T which \p s and \p t of \a G belong to.
	SList<node>& findPath (node s, node t) { return findPathSPQR(m_gNode_hNode[s],m_gNode_hNode[t]); }

	/**
	 * \brief Returns the skeleton of node \p v.
	 * \pre \p v is a node in \a T
	 */
	Skeleton& skeleton (node v) const override
	{
		v = findSPQR(v);
		if (!m_sk[v]) return createSkeleton(v);
		return *m_sk[v];
	}

	/**
	 * \brief Returns the skeleton that contains the real edge \p e.
	 * \pre \p e is an edge in \a G
	 */
	const Skeleton& skeletonOfReal (edge e) const override { return skeleton(spqrproper(m_gEdge_hEdge[e])); }

	/**
	 * \brief Returns the skeleton edge that corresponds to the real edge \p e.
	 * \pre \p e is an edge in \a G
	 */
	edge copyOfReal (edge e) const override
	{
		e = m_gEdge_hEdge[e];
		skeleton(spqrproper(e));
		return m_skelEdge[e];
	}

	/**
	 * \brief Returns the virtual edge in the skeleton of \p w that
	 * corresponds to the tree edge between \p v and \p w.
	 * \pre \p v and \p w are adjacent nodes in \a T
	 */
	edge skeletonEdge (node v, node w) const
	{
		edge e = virtualEdge(v,w);
		if (!e) return e;
		skeleton(w);
		return m_skelEdge[e];
	}


	//
	// b) Update operations
	//

	/**
	 * \brief Roots \a T at edge \p e and returns the new root node of \a T.
	 * \pre \p e is an edge in \a G
	 */
	node rootTreeAt (edge e) override;

	/**
	 * \brief Roots \a T at node \p v and returns \p v.
	 * \pre \p v is a node in \a T
	 */
	node rootTreeAt (node v) override;

	/**
	 * \brief Updates the whole data structure after a new edge \p e has
	 * been inserted into \a G.
	 */
	edge updateInsertedEdge (edge e) override;

	/**
	 * \brief Updates the whole data structure after a new vertex has been
	 * inserted into \a G by splitting an edge into \p e and \p f.
	 */
	node updateInsertedNode (edge e, edge f) override;


protected:

	//! Initialization (called by constructors).
	void init (edge e);

	//! Creates the skeleton graph belonging to a given vertex \p vT of \a T.
	DynamicSkeleton& createSkeleton (node vT) const;

	/**
	 * \brief Recursively performs the task of adding edges (and nodes)
	 * to the pertinent graph \p Gp for each involved skeleton graph.
	 */
	void cpRec (node v, PertinentGraph& Gp) const override
	{
		v = findSPQR(v);
		for (ListConstIterator<edge> i=m_tNode_hEdges[v]->begin(); i.valid(); ++i) {
			edge e = m_hEdge_gEdge[*i];
			if (e) cpAddEdge(e,Gp);
			else if (*i!=m_tNode_hRefEdge[v]) cpRec(spqrproper(*i),Gp);
		}
	}


	edge m_rootEdge;  //!< edge of \a G at which \a T is rooted

	mutable NodeArray<DynamicSkeleton*> m_sk;        //!< pointer to skeleton of a node in \a T
	mutable EdgeArray<edge>             m_skelEdge;  //!< copies of real and virtual edges in their skeleton graphs (invalid, if the skeleton does not actually exist)
	mutable NodeArray<node>             m_mapV;      //!< temporary array used by \a createSkeleton()

};

}
