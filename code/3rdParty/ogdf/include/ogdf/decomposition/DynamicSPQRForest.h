/** \file
 * \brief Declaration of class DynamicSPQRForest
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

#include <ogdf/decomposition/DynamicBCTree.h>
#include <ogdf/decomposition/SPQRTree.h>

namespace ogdf {

/**
 * Dynamic SPQR-forest.
 *
 * @ingroup decomp
 *
 * This class is an extension of DynamicBCTree.\n
 * It provides a set of SPQR-trees for each B-component of a BC-tree.
 * These SPQR-trees are dynamic, i.e. there are member functions for
 * dynamic updates (edge insertion and node insertion).
 */
class OGDF_EXPORT DynamicSPQRForest : public DynamicBCTree {
public:
	//! Enumeration type for characterizing the SPQR-tree-vertices.
	enum class TNodeType {
		SComp = static_cast<int>(SPQRTree::NodeType::SNode), //!< denotes a vertex representing an S-component
		PComp = static_cast<int>(SPQRTree::NodeType::PNode), //!< denotes a vertex representing a P-component
		RComp = static_cast<int>(SPQRTree::NodeType::RNode) //!< denotes a vertex representing an R-component
	};

protected:
	//! A \a Graph structure containing all SPQR-trees.
	mutable Graph m_T;

	/** @{
	 * The root vertices of the SPQR-trees.
	 *
	 * For each vertex of the BC-tree representing a B-component, this
	 * array contains the root vertex of the respective SPQR-tree, or
	 * \a nullptr, if the SPQR-tree does not exist.
	 */
	mutable NodeArray<node> m_bNode_SPQR;

	/**
	 * The numbers of S-components.
	 *
	 * For each vertex of the BC-tree representing a B-component,
	 * this array contains the number of S-components of the respective
	 * SPQR-tree. If the SPQR-tree does not exist, then the array member
	 * is undefined.
	 */
	mutable NodeArray<int> m_bNode_numS;

	/**
	 * The numbers of P-components.
	 *
	 * For each vertex of the BC-tree representing a B-component,
	 * this array contains the number of P-components of the respective
	 * SPQR-tree. If the SPQR-tree does not exist, then the array member
	 * is undefined.
	 */
	mutable NodeArray<int> m_bNode_numP;

	/**
	 * The numbers of R-components.
	 *
	 * For each vertex of the BC-tree representing a B-component,
	 * this array contains the number of R-components of the respective
	 * SPQR-tree. If the SPQR-tree does not exist, then the array member
	 * is undefined.
	 */
	mutable NodeArray<int> m_bNode_numR;
	//! @}

	//! @{
	//! The types of the SPQR-tree-vertices.
	mutable NodeArray<TNodeType> m_tNode_type;

	//! The owners of the SPQR-tree-vertices in the UNION/FIND structure.
	mutable NodeArray<node> m_tNode_owner;

	//! The virtual edges leading to the parents of the SPQR-tree vertices.
	mutable NodeArray<edge> m_tNode_hRefEdge;

	//! Lists of real and virtual edges belonging to SPQR-tree vertices.
	mutable NodeArray<List<edge>*> m_tNode_hEdges;
	//! @}

	//! @{
	//! The positions of real and virtual edges in their \a m_tNode_hEdges lists.
	mutable EdgeArray<ListIterator<edge> > m_hEdge_position;

	//! The SPQR-tree-vertices which the real and virtual edges are belonging to.
	mutable EdgeArray<node> m_hEdge_tNode;

	//! The partners of virtual edges (\a nullptr if real).
	mutable EdgeArray<edge> m_hEdge_twinEdge;
	//! @}

	//! @{
	//! Auxiliary array used by \a createSPQR().
	mutable NodeArray<node> m_htogc;

	//! Auxiliary array used by \a findNCASPQR()
	mutable NodeArray<bool> m_tNode_isMarked;
	//! @}

	//! Initialization.
	void init();

	/**
	 * Creates the SPQR-tree for a given B-component of the
	 * BC-tree.
	 *
	 * An SPQR-tree belonging to a B-component of the BC-tree is only
	 * created on demand, i.e. this member function is only called by
	 * findSPQRTree() and - under certain circumstances - by
	 * updateInsertedEdge().
	 * \param vB is a vertex of the BC-tree representing a B-component.
	 * \pre \p vB has to be the proper representative of its B-component,
	 * i.e. it has to be the root vertex of its respective
	 * UNION/FIND-tree.
	 * \pre The B-component represented by \p vB must contain at least
	 * 3 edges.
	 */
	void createSPQR(node vB) const;

	/**
	 * Creates a new node in the SPQR-tree for a given B-component of the
	 * BC-tree.
	 *
	 * \param vB is a vertex of the BC-tree representing a B-component.
	 * \param spqrNodeType is the type of the new SPQR-node.
	 * \pre \p vB has to be the proper representative of its B-component, i.e.
	 * it has to be the root vertex of its respective UNION/FIND-tree.
	 * \return the newly created node in #m_T.
	 */
	inline node newSPQRNode(node vB, const TNodeType spqrNodeType) const {
		node vT = m_T.newNode();
		m_tNode_owner[vT] = vT;
		m_tNode_type[vT] = spqrNodeType;
		m_tNode_hEdges[vT] = new List<edge>;

		if (spqrNodeType == TNodeType::PComp) {
			m_bNode_numP[vB]++;
		} else if (spqrNodeType == TNodeType::SComp) {
			m_bNode_numS[vB]++;
		} else if (spqrNodeType == TNodeType::RComp) {
			m_bNode_numR[vB]++;
		}

		return vT;
	}

	/**
	 * Adds edge \p eH to a vertex \p vT of the SPQRForest.
	 *
	 * \param eH is an edge in #m_H.
	 * \param vT is a vertex in #m_T.
	 */
	inline void addHEdge(edge eH, node vT) const {
		m_hEdge_position[eH] = m_tNode_hEdges[vT]->pushBack(eH);
		m_hEdge_tNode[eH] = vT;
	}

	/**
	 * Deletes edge \p eH from #m_H and removes its connection to a vertex \p vT
	 * of the SPQRForest.
	 *
	 * \param eH is an edge in #m_H.
	 * \param vT is a vertex in #m_T.
	 */
	inline void delHEdge(edge eH, node vT) const {
		m_tNode_hEdges[vT]->del(m_hEdge_position[eH]);
		m_H.delEdge(eH);
	}

	/**
	 * Creates a twin edge for \p eH, adds it to \p vT and returns it.
	 *
	 * \param eH is an edge in #m_H, whose twin edge should be created.
	 * \param vT is a vertex in #m_T.
	 * \return the new twin edge.
	 */
	inline edge newTwinEdge(edge eH, node vT) const {
		edge fH = m_H.newEdge(eH->source(), eH->target());
		addHEdge(fH, vT);
		m_hEdge_twinEdge[eH] = fH;
		m_hEdge_twinEdge[fH] = eH;
		return fH;
	}

	/** @{
	 * Unites two SPQR-tree-vertices (UNION part of UNION/FIND).
	 * \param vB is a vertex of the BC-tree representing a B-component.
	 * \param sT is a vertex of the SPQR-tree belonging to \p vB.
	 * \param tT is a vertex of the SPQR-tree belonging to \p vB.
	 * \pre \p vB has to be the proper representative of its B-component,
	 * i.e. it has to be the root vertex of its respective
	 * UNION/FIND-tree.
	 * \pre \p sT and \p tT have to be proper representatives of their
	 * triconnected components, i.e. they have to be the root vertices of
	 * their respective UNION/FIND-trees.
	 * \return the proper representative of the united SPQR-tree-vertex.
	 */
	node uniteSPQR(node vB, node sT, node tT);

	/**
	 * Finds the proper representative of an SPQR-tree-vertex (FIND
	 * part of UNION/FIND).
	 * \param vT is any vertex of \a m_T.
	 * \return the owner of \p vT properly representing a triconnected
	 * component, i.e. the root of the UNION/FIND-tree of \p vT.
	 */
	node findSPQR(node vT) const;
	//! @}

	/** @{
	 * Finds the nearest common ancestor of \p sT and \p tT.
	 * \param sT is a vertex of an SPQR-tree.
	 * \param tT is a vertex of an SPQR-tree.
	 * \pre \p sT and \p tT must belong to the same SPQR-tree.
	 * \pre \p sT and \p tT have to be proper representatives of their
	 * triconnected components, i.e. they have to be the root vertices of
	 * their respective UNION/FIND-trees.
	 * \return the proper representative of the nearest common ancestor of
	 * \p sT and \p tT.
	 */
	node findNCASPQR(node sT, node tT) const;

	/**
	 * Finds the shortest path between the two sets of
	 * SPQR-tree-vertices which \p sH and \p tH are belonging to.
	 * \param sH is a vertex of \a m_H.
	 * \param tH is a vertex of \a m_H.
	 * \param rT <b>is a reference!</b> It is set to the very vertex of
	 * the found path which is nearest to the root vertex of the SPQR-tree.
	 * \pre \p sH and \p tH must belong to the same B-component, i.e. to
	 * the same SPQR-tree. This SPQR-tree must exist!
	 * \return the path in the SPQR-tree as a linear list of vertices.
	 * \post <b>The SList<node> instance is created by this function and
	 * has to be destructed by the caller!</b>
	 */
	SList<node>& findPathSPQR(node sH, node tH, node& rT) const;
	//! @}

	/** @{
	 * Updates an SPQR-tree after a new edge has been inserted into
	 * the original graph.
	 * \param vB is a BC-tree-vertex representing a B-component. The
	 * SPQR-tree, which is to be updated is identified by it.
	 * \param eG is a new edge in the original graph.
	 * \pre \p vB has to be the proper representative of its B-component,
	 * i.e. it has to be the root vertex of its respective
	 * UNION/FIND-tree.
	 * \pre Both the source and the target vertices of \p eG must belong
	 * to the same B-component represented by \p vB.
	 * \return the new edge of the original graph.
	 */
	edge updateInsertedEdgeSPQR(node vB, edge eG);

	/**
	 * Updates an SPQR-tree after a new vertex has been inserted
	 * into the original graph by splitting an edge into \p eG and \p fG.
	 * \param vB is a BC-tree-vertex representing a B-component. The
	 * SPQR-tree, which is to be updated is identified by it.
	 * \param eG is the incoming edge of the newly inserted vertex which
	 * has been generated by a Graph::split() operation.
	 * \param fG is the outgoing edge of the newly inserted vertex which
	 * has been generated by a Graph::split() operation.
	 * \pre The split edge must belong to the B-component which is
	 * represented by \p vB.
	 * \return the new vertex of the original graph.
	 */
	node updateInsertedNodeSPQR(node vB, edge eG, edge fG);
	//! @}

public:
	/**
	 * A constructor.
	 *
	 * This constructor does only create the dynamic BC-tree rooted at the first
	 * edge of \p G. The data structure is prepared for dealing with SPQR-trees,
	 * but they will only be created on demand. Cf. member functions findPathSPQR()
	 * and updateInsertedEdge().
	 * \param G is the original graph.
	 */
	explicit DynamicSPQRForest(Graph& G) : DynamicBCTree(G) { init(); }

	~DynamicSPQRForest() { for (auto pList : m_tNode_hEdges) { delete pList; } }

	/** @{
	 * Finds the proper representative of the SPQR-tree-vertex which
	 * a given real or virtual edge is belonging to.
	 *
	 * This member function has to be used carefully (see <b>Precondition</b>)!
	 * \param eH is an edge of \a m_H.
	 * \pre The respective SPQR-tree belonging to the B-component represented by
	 * the BC-tree-vertex bcproper(\p eH) must exist! Notice, that this condition
	 * is fulfilled, if \p eH is a member of a list gained by the hEdgesSPQR()
	 * member function, because that member function needs an SPQR-tree-vertex as
	 * parameter, which might have been found (and eventually created) by the
	 * findPathSPQR() member function.
	 * \return the proper representative of the SPQR-tree-vertex which \p eH
	 * is belonging to.
	 */
	node spqrproper(edge eH) const { return m_hEdge_tNode[eH] = findSPQR(m_hEdge_tNode[eH]); }

	/**
	 * Returns the twin edge of a given edge of \a m_H, if it is
	 * virtual, or \a nullptr, if it is real.
	 * \param eH is an edge of \a m_H.
	 * \return the twin edge of \p eH, if it is virtual, or \a nullptr, if it
	 * is real.
	 */
	edge twinEdge(edge eH) const { return m_hEdge_twinEdge[eH]; }
	//! @}

	/** @{
	 * Returns the type of the triconnected component represented by
	 * a given SPQR-tree-vertex.
	 * \param vT is a vertex of an SPQR-tree.
	 * \pre \p vT has to be the proper representative of its triconnected
	 * component, i.e. it has to be the root vertex of its respective
	 * UNION/FIND-tree. This condition is particularly fulfilled if \p vT
	 * is a member of a list gained by the findPathSPQR() member function.
	 * \return the type of the triconnected component represented by \p vT.
	 */
	TNodeType typeOfTNode(node vT) const { return m_tNode_type[vT]; }

	/**
	 * Returns a linear list of the edges in \a m_H belonging to
	 * the triconnected component represented by a given SPQR-tree-vertex.
	 * \param vT is a vertex of an SPQR-tree.
	 * \pre \p vT has to be the proper representative of its triconnected
	 * component, i.e. it has to be the root vertex of its respective
	 * UNION/FIND-tree. This condition is particularly fulfilled if \p vT
	 * is a member of a list gained by the findPathSPQR() member function.
	 * \return a linear list of the edges in \a m_H belonging to the
	 * triconnected component represented by \p vT.
	 */
	const List<edge>& hEdgesSPQR(node vT) const { return *m_tNode_hEdges[vT]; }

	/**
	 * Finds the shortest path between the two sets of
	 * SPQR-tree-vertices which \p sH and \p tH are belonging to.
	 * \param sH is a vertex of \a m_H.
	 * \param tH is a vertex of \a m_H.
	 * \pre \p sH and \p tH must belong to the same B-component, i.e. to
	 * the same SPQR-tree. This SPQR-tree does not need to exist. If it
	 * it does not exist, it will be created.
	 * \return the path in the SPQR-tree as a linear list of vertices.
	 * \post <b>The SList<node> instance is created by this function and
	 * has to be destructed by the caller!</b>
	 */
	SList<node>& findPathSPQR(node sH, node tH) const;

	/**
	 * Returns the virtual edge which leads from one vertex of an
	 * SPQR-tree to another one.
	 * \param vT is a vertex of an SPQR-tree.
	 * \param wT is a vertex of an SPQR-tree.
	 * \pre \p vT and \p wT must belong to the same SPQR-tree and must be
	 * adjacent.
	 * \pre \p vT and \p wT have to be proper representatives of their
	 * triconnected components, i.e. they have to be the root vertices of
	 * their respective UNION/FIND-trees. This condition is particularly
	 * fulfilled if \p vT and \p wT are members of a list gained by the
	 * findPathSPQR() member function.
	 * \return the virtual edge in \a m_H which belongs to \p wT and
	 * leads to \p vT.
	 */
	edge virtualEdge(node vT, node wT) const;
	//! @}

	/** @{
	 * Updates the whole data structure after a new edge has been
	 * inserted into the original graph.
	 *
	 * This member function generally updates both BC- and SPQR-trees. If
	 * any SPQR-tree of the B-components of the insertion path through
	 * the BC-tree exists, the SPQR-tree data structure of the resulting
	 * B-component will be valid afterwards. If none of the SPQR-trees
	 * does exist in advance, then only the BC-tree is updated and no
	 * SPQR-tree is created.
	 * \param eG is a new edge in the original graph.
	 * \return the new edge of the original graph.
	 */
	edge updateInsertedEdge(edge eG) override;

	/**
	 * Updates the whole data structure after a new vertex has been
	 * inserted into the original graph by splitting an edge into \p eG
	 * and \p fG.
	 *
	 * This member function updates the BC-tree at first. If the SPQR-tree
	 * of the B-component which the split edge is belonging to does exist,
	 * then it is updated, too. If it does not exist, it is not created.
	 * \param eG is the incoming edge of the newly inserted vertex which
	 * has been generated by a Graph::split() operation.
	 * \param fG is the outgoing edge of the newly inserted vertex which
	 * has been generated by a Graph::split() operation.
	 * \return the new vertex of the original graph.
	 */
	node updateInsertedNode(edge eG, edge fG) override;

	//! @}
};

}
