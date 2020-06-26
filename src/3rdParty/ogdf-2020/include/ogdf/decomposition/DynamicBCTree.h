/** \file
 * \brief Declaration of class DynamicBCTree
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

#include <ogdf/decomposition/BCTree.h>

namespace ogdf {

/**
 * Dynamic BC-trees.
 *
 * @ingroup decomp
 *
 * This class provides dynamic BC-trees.\n
 * The main difference of the dynamic BC-tree structure compared to the static
 * one implemented by the class BCTree is, that B- and C-components are not any
 * longer represented by single vertices of a BC-tree graph structure but by
 * root vertices of UNION/FIND-trees. This allows path condensation within the
 * BC-tree, when edges are inserted into the original graph. Path condensation
 * is done by gathering BC-tree-vertices into a UNION/FIND-tree. However, the
 * original vertices of the BC-tree remain in the \a m_B graph, but only those
 * being the roots of their respective UNION/FIND-tree are proper representants
 * of the biconnected components of the original graph.
 */
class OGDF_EXPORT DynamicBCTree : public BCTree {
	friend class PlanarAugmentation;
	friend class PlanarAugmentationFix;

protected:
	/**
	 * Array that contains for each BC-tree-vertex its parent in its
	 * UNION/FIND-tree structure.
	 *
	 * For each vertex \a vB of the BC-tree structure:
	 * - If \a vB is representing a biconnected component, then
	 *   m_bNode_owner[\a vB] points to the vertex \a vB itself.
	 * - If \a vB is not any longer representing a biconnected component due to
	 *   path condensation, then m_bNode_owner[\a vB] points to the parent of \a vB
	 *   in its UNION/FIND-tree.
	 */
	mutable NodeArray<node> m_bNode_owner;

	/**
	 * Array that contains for each proper BC-tree-vertex its degree.
	 *
	 * For each vertex \a vB of the BC-tree structure:
	 * - If \a vB is representing a biconnected component, then
	 *   m_bNode_degree[\a vB] is the degree of the vertex of the BC-tree.
	 * - If \a vB is not any longer representing a biconnected component due to
	 *   path condensation, then m_bNode_degree[\a vB] is undefined.
	 * This array is necessary, because the edges of the BC-tree are not updated
	 * during path condensation for efficiency reasons. Thus, <em>vB</em>->degree()
	 * != m_bNode_degree[\a vB]
	 */
	NodeArray<int> m_bNode_degree;

	/** @{
	 * Initialization of \a m_bNode_owner and \a m_bNode_degree.
	 */
	void init ();
	//! @}

	/** @{
	 * The UNION function of the UNION/FIND structure.
	 * \param uB is a vertex of the BC-tree representing a B-component.
	 * \param vB is a vertex of the BC-tree representing a C-component.
	 * \param wB is a vertex of the BC-tree representing a B-component.
	 * \pre \p uB and \p vB and \p wB have to be proper representants of their
	 * B-components, i.e. they have to be the root vertices of their respective
	 * UNION/FIND-trees.
	 * \pre \p uB and \p wB have to be adjacent to \p vB.
	 * \return the vertex properly representing the condensed B-component.
	 */
	node unite (node uB, node vB, node wB);

	/**
	 * The FIND function of the UNION/FIND structure.
	 * \param vB is any vertex of \a m_B.
	 * \return the owner of \p vB properly representing a biconnected component,
	 * i.e. the root of the UNION/FIND-tree of \p vB.
	 */
	node find (node vB) const;
	//! @}

	/** @{
	 * Returns the parent of a given BC-tree-vertex.
	 * \param vB is any vertex of \a m_B or \a nullptr.
	 * \return the parent of \p vB in the BC-tree structure, if \p vB is not the
	 * root of the BC-tree, and \a nullptr, if \p vB is \a nullptr or the root of the
	 * BC-tree. The UNION/FIND-tree structures are considered.
	 */
	node parent (node vB) const override;

	/**
	 * Performs path condensation.
	 *
	 * This member function condenses the path from bcproper(\p sG) to
	 * bcproper(\p tG) in the BC-tree into one single B-component by calling
	 * findPath() and subsequently unite().
	 * \param sG is a vertex of the original graph.
	 * \param tG is a vertex of the original graph.
	 * \return the proper representant of the resulting B-component.
	 */
	node condensePath (node sG, node tG);
	//! @}

public:
	/** @{
	 * A constructor.
	 *
	 * This constructor does only call BCTree::BCTree() and DynamicBCTree::init().
	 * DynamicBCTree(\p G) is equivalent to DynamicBCTree(<em>G</em>,
	 * <em>G</em>.firstNode()).
	 * \param G is the original graph.
	 * \param callInitConnected decides which init is called, default call is init().
	 */
	explicit DynamicBCTree (Graph& G, bool callInitConnected = false) : BCTree(G, callInitConnected) { init(); }

	/**
	 * A constructor.
	 *
	 * This constructor does only call BCTree::BCTree() and DynamicBCTree::init().
	 * \param G is the original graph.
	 * \param vG is the vertex of the original graph which the DFS algorithm starts with.
	 * \param callInitConnected decides which init is called, default call is init().
	 */
	DynamicBCTree (Graph& G, node vG, bool callInitConnected = false) : BCTree(G,vG, callInitConnected) { init(); }
	//! @}

	/** @{
	 * Returns a BC-tree-vertex representing a biconnected component which a
	 * given vertex of the original graph is belonging to.
	 * \param vG is a vertex of the original graph.
	 * \return a vertex of the BC-tree:
	 * - If \p vG is not a cut-vertex, then typeOfGNode(\p vG) returns the very
	 *   vertex of the BC-tree representing the unambiguous B-component which \p vG
	 *   is belonging to.
	 * - If \p vG is a cut-vertex, then typeOfGNode(\p vG) returns the very vertex
	 *   of the BC-tree representing the unambiguous C-component which \p vG is
	 *   belonging to.
	 *
	 * The difference between BCTree::bcproper() and DynamicBCTree::bcproper() is,
	 * that the latter one considers the UNION/FIND-tree structures.
	 */
	node bcproper (node vG) const override;

	/**
	 * Returns the BC-tree-vertex representing the biconnected component
	 * which a given edge of the original graph is belonging to.
	 * \param eG is an edge of the original graph.
	 * \return the vertex of the BC-tree representing the B-component which \p eG
	 * is belonging to.
	 *
	 * The difference between BCTree::bcproper() and DynamicBCTree::bcproper() is,
	 * that the latter one considers the UNION/FIND-tree structures.
	 */
	node bcproper (edge eG) const override;
	//! @}

	/** @{
	 * Returns a vertex of the biconnected components graph corresponding to
	 * a given vertex of the original graph and belonging to the representation of
	 * a certain biconnected component given by a vertex of the BC-tree.
	 * \param uG is a vertex of the original graph.
	 * \param vB is any vertex of \a m_B.
	 * \return a vertex of the biconnected components graph:
	 * - If \p uG is belonging to the biconnected component represented by \p vB,
	 *   then rep(\p uG,\p vB) returns the very vertex of the biconnected
	 *   components graph corresponding to \p uG within the representation of
	 *   \p vB.
	 * - Otherwise, \a nullptr is returned.
	 *
	 * The difference between BCTree::repVertex() and DynamicBCTree::repVertex()
	 * is, that the latter one considers the UNION/FIND-tree structures.
	 */
	node repVertex (node uG, node vB) const override { return BCTree::repVertex(uG,find(vB)); }

	/**
	 * Returns the copy of a cut-vertex in the biconnected components graph
	 * which belongs to a certain B-component and leads to another B-component.
	 *
	 * If two BC-tree-vertices are neighbours, then the biconnected components
	 * represented by them have exactly one cut-vertex in common. But there are
	 * several copies of this cut-vertex in the biconnected components graph,
	 * namely one copy for each biconnected component which the cut-vertex is
	 * belonging to. The member function rep() had been designed for returning the
	 * very copy of the cut-vertex belonging to the copy of the unambiguous
	 * C-component which it is belonging to, whereas this member function is
	 * designed to return the very copy of the cut-vertex connecting two
	 * biconnected components which belongs to the copy of the second one.
	 * \param uB is any vertex of \a m_B.
	 * \param vB is any vertex of \a m_B.
	 * \return a vertex of the biconnected components graph:
	 * - If \p uB == \p vB and they are representing a B-component, then
	 *   cutVertex(\p uB,\p vB) returns \a nullptr.
	 * - If \p uB == \p vB and they are representing a C-component, then
	 *   cutVertex(\p uB,\p vB) returns the single isolated vertex in the
	 *   biconnected components graph which is the copy of the C-component.
	 * - If \p uB and \p vB are \a neighbours in the BC-tree, then there exists
	 *   a cut-vertex leading from the biconnected component represented by \p vB
	 *   to the biconnected component represented by \p uB. cutVertex(\p uB,\p vB)
	 *   returns the very copy of this vertex within the biconnected components
	 *   graph which belongs to the copy of the biconnected component represented
	 *   by \p vB.
	 * - Otherwise, cutVertex(\p uB,\p vB) returns \a nullptr.
	 *
	 * The difference between BCTree::cutVertex() and DynamicBCTree::cutVertex()
	 * is, that the latter one considers the UNION/FIND-tree structures.
	 */
	node cutVertex (node uB, node vB) const override { return BCTree::cutVertex(find(uB),find(vB)); }
	//! @}

	/** @{
	 * Update of the dynamic BC-tree after edge insertion into the original
	 * graph.
	 *
	 * This member function performs on-line maintenance of the dynamic BC-tree
	 * according to J. Westbrook and R. E. Tarjan, Maintaining Bridge-Connected and
	 * Biconnected Components On-Line, Algorithmica (1992) 7:433-464.
	 * \param eG is a newly inserted edge of the original graph.
	 *
	 * After a new edge has been inserted into the original graph by calling
	 * Graph::newEdge(), this member function updates the corresponding BC-tree in
	 * \f$O(\alpha(k,n))\f$ amortized time and the coponents graph in
	 * \a O(1 + n/k) amortized time per insertEdge() operation, where k is the
	 * number of such operations.
	 * \return the new edge of the original graph.
	 */
	virtual edge updateInsertedEdge (edge eG);

	/**
	 * Update of the dynamic BC-tree after vertex insertion into the
	 * original graph.
	 *
	 * This member function performs on-line maintenance of the dynamic BC-tree
	 * according to J. Westbrook and R. E. Tarjan, Maintaining Bridge-Connected and
	 * Biconnected Components On-Line, Algorithmica (1992) 7:433-464.
	 * \param eG is the incoming edge of the newly inserted vertex which has been
	 * generated by a Graph::split() operation.
	 * \param fG is the outgoing edge of the newly inserted vertex which has been
	 * generated by a Graph::split() operation.
	 *
	 * After a new vertex has been inserted into an edge of the original graph by
	 * splitting the edge, all data structures of the DynamicBCTree class are
	 * updated by this member funtion. It takes \a O(1) time.
	 * \return the new vertex of the original graph.
	 */
	virtual node updateInsertedNode (edge eG, edge fG);

	/**
	 * Inserts a new edge into the original graph and updates the BC-tree.
	 *
	 * This member function inserts a new edge between \p sG and \p tG into the
	 * original graph and then calls updateInsertedEdge().
	 * \param sG is a vertex of the original graph.
	 * \param tG is a vertex of the original graph.
	 * \return the new edge of the original graph.
	 */
	edge insertEdge (node sG, node tG) { return updateInsertedEdge(m_G.newEdge(sG,tG)); }

	/**
	 * Inserts a new vertex into the original graph and updates the BC-tree.
	 *
	 * This member function inserts a new vertex into the original graph by
	 * splitting the edge \p eG and then calls updateInsertedNode().
	 * \param eG is an edge of the original graph.
	 * \return the new vertex of the original graph.
	 */
	node insertNode (edge eG) { return updateInsertedNode(eG,m_G.split(eG)); }
	//! @}

	/** @{
	 * Returns the BC-tree-vertex representing the B-component which two
	 * given vertices of the original graph are belonging to.
	 * \param uG is a vertex of the original graph.
	 * \param vG is a vertex of the original graph.
	 * \return If \p uG and \p vG are belonging to the same B-component, the very
	 * vertex of the BC-tree representing this B-component is returned. Otherwise,
	 * \a nullptr is returned. This member function returns the representant of the
	 * correct B-component even if \p uG or \p vG or either are cut-vertices and
	 * are therefore belonging to C-components, too.
	 *
	 * The difference between BCTree::bComponent() and DynamicBCTree::bComponent()
	 * is, that the latter one considers the UNION/FIND-tree structures.
	 */
	node bComponent (node uG, node vG) const;

	//! @}
};

}
