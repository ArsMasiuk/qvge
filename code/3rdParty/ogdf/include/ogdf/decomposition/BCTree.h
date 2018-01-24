/** \file
 * \brief Declaration of class BCTree
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

#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/SList.h>

namespace ogdf {

/**
 * Static BC-trees.
 *
 * @ingroup graph-decomp
 *
 * This class provides static BC-trees.\n
 * The data structure consists of three parts:
 * - The original graph itself (\a G) is represented by an ordinary ogdf::Graph
 *   structure.
 * - The BC-tree (\a B) is represented by an ogdf::Graph structure, each
 *   vertex representing a B-component or a C-component.
 * - The biconnected components graph (\a H), which contains a set of copies of
 *   the biconnected components and the cut-vertices of the original graph,
 *   combined but not interconnected within a single ogdf::Graph structure.
 */
class OGDF_EXPORT BCTree {

public:
	//! Enumeration type for characterizing the vertices of the original graph.
	enum class GNodeType {
		//! an ordinary vertex, i.e. not a cut-vertex
		Normal,
		//! a cut-vertex
		CutVertex
	};

	//! Enumeration type for characterizing the BC-tree-vertices.
	enum class BNodeType {
		//! a vertex representing a B-component
		BComp,
		//! a vertex representing a C-component
		CComp
	};

protected:
	//! The original graph.
	Graph& m_G;

	/**
	 * The BC-tree.
	 *
	 * Each vertex is representing a biconnected component (B-component) or a
	 * cut-vertex (C-component) of the original graph.
	 */
	Graph m_B;

	/**
	 * The biconnected components graph.
	 *
	 * This graph contains copies of the biconnected components (B-components)
	 * and the cut-vertices (C-components) of the original graph. The copies of the
	 * B- and C-components of the original graph are not interconnected, i.e. the
	 * biconnected components graph is representing B-components as isolated
	 * biconnected subgraphs and C-components as isolated single vertices. Thus the
	 * copies of the edges and non-cut-vertices of the original graph are
	 * unambiguous, but each cut-vertex of the original graph being common to a
	 * C-component and several B-components appears multiple times.
	 */
	mutable Graph m_H;

	//! @{
	//! The number of B-components.
	int m_numB;

	//! The number of C-components.
	int m_numC;
	//! @}

	/** @{
	 * Array of marks for the vertices of the original graph.
	 *
	 * They are needed during the generation of the BC-tree by DFS method.
	 */
	NodeArray<bool> m_gNode_isMarked;

	/**
	 * An injective mapping vertices(\a G) -> vertices(\a H).
	 *
	 * For each vertex \a vG of the original graph:
	 * - If \a vG is not a cut-vertex, then m_gNode_hNode[\a vG] is the very vertex
	 *   of the biconnected components graph corresponding to \a vG.
	 * - If \a vG is a cut-vertex, then m_gNode_hNode[\a vG] is the very vertex of
	 *   the biconnected components graph representing the C-component, which \a vG
	 *   is belonging to, as a single isolated vertex.
	 */
	NodeArray<node> m_gNode_hNode;

	/**
	 * A bijective mapping edges(\a G) -> edges(\a H).
	 *
	 * For each edge \a eG of the original graph, m_gEdge_hEdge[\a eG] is the very
	 * edge of the biconnected components graph corresponding to \a eG.
	 */
	EdgeArray<edge> m_gEdge_hEdge;
	//! @}

	//! @{
	//! Array that contains the type of each BC-tree-vertex.
	NodeArray<BNodeType> m_bNode_type;

	/**
	 * Array of marks for the BC-tree-vertices.
	 *
	 * They are needed for searching for the nearest common ancestor of two
	 * vertices of the BC-tree.
	 */
	mutable NodeArray<bool> m_bNode_isMarked;

	/**
	 * Array that contains for each BC-tree-vertex the representantive of its
	 * parent within the subgraph in the biconnected components graph belonging to
	 * the biconnected component represented by the respective BC-tree-vertex.
	 *
	 * For each vertex \a vB of the BC-tree:
	 * - If \a vB is representing a B-component and \a vB is the root of the
	 *   BC-tree, then m_bNode_hRefNode[\a vB] is \a nullptr.
	 * - If \a vB is representing a B-component and \a vB is not the root of the
	 *   BC-tree, then m_bNode_hRefNode[\a vB] is the very vertex of the
	 *   biconnected components graph which is the duplicate of the cut-vertex
	 *   represented by the parent of \a vB <em>in the copy of the B-component
	 *   represented by</em> \a vB.
	 * - If \a vB is representing a C-component, then m_bNode_hRefNode[\a vB]
	 *   is the single isolated vertex of the biconnected components graph
	 *   corresponding to the cut-vertex which the C-component consists of,
	 *   irrespective of whether \a vB is the root of the BC-tree or not.
	 */
	NodeArray<node> m_bNode_hRefNode;

	/**
	 * Array that contains for each BC-tree-vertex the representant of
	 * itself within the subgraph in the biconnected components graph belonging to
	 * the biconnected component represented by the parent of the respective
	 * BC-tree-vertex.
	 *
	 * - If \a vB is the root of the BC-tree, then m_bNode_hParNode[\a vB] is
	 *   \a nullptr.
	 * - If \a vB is representing a B-component and \a vB is not the root of the
	 *   BC-tree, then m_bNode_hParNode[\a vB] is the single isolated vertex
	 *   of the biconnected components graph corresponding to the very cut-vertex,
	 *   which the C-component represented by <em>the parent of</em> \a vB consists
	 *   of.
	 * - If \a vB is representing to a C-component and \a vB is not the root of the
	 *   BC-tree, then m_bNode_hParNode[\a vB] is the very vertex of the
	 *   biconnected components graph, which is the duplicate of the cut-vertex,
	 *   which the C-component consists of, <em>in the copy of the B-component
	 *   represented by the parent of</em> \a vB.
	 */
	NodeArray<node> m_bNode_hParNode;

	/**
	 * Array that contains for each BC-tree-vertex a linear list of the
	 * edges of the biconnected components graph belonging to the biconnected
	 * component represented by the respective BC-tree-vertex.
	 *
	 * For each vertex \a vB of the BC-tree:
	 * - If \a vB is representing a B-component, then m_bNode_hEdges[\a vB] is a
	 *   linear list of the edges of the biconnected components graph corresponding
	 *   to the edges of the original graph belonging to the B-component.
	 * - If \a vB is representing a C-component, then m_bNode_hEdges[\a vB] is an
	 *   empty list.
	 */
	NodeArray<SList<edge> > m_bNode_hEdges;

	/**
	 * Array that contains for each BC-tree-vertex the number of vertices
	 * belonging to the biconnected component represented by the respective
	 * BC-tree-vertex.
	 *
	 * For each vertex \a vB of the BC-tree:
	 * - If \a vB is representing a B-component, then m_bNode_numNodes[\a vB] is
	 *   the number of vertices belonging to the B-component, cut-vertices
	 *   inclusive.
	 * - If \a vB is representing a C-component, then m_bNode_numNodes[\a vB] is 1.
	 */
	NodeArray<int> m_bNode_numNodes;
	//! @}

	/** @{
	 * A surjective mapping vertices(\a H) -> vertices(\a B).
	 *
	 * For each vertex \a vH of the biconnected components graph,
	 * m_hNode_bNode[\a vH] is the very BC-tree-vertex representing the B- or
	 * C-component with respect to the copy of the very block or representation
	 * of a cut-vertex, which vH is belonging to.
	 */
	mutable NodeArray<node> m_hNode_bNode;

	/**
	 * A surjective mapping edges(\a H) -> vertices(\a B).
	 *
	 * For each edge \a eH of the biconnected components graph,
	 * m_hEdge_bNode[\a eH] is the very BC-tree-vertex representing the unambiguous
	 * B-component, which \a eH is belonging to.
	 */
	mutable EdgeArray<node> m_hEdge_bNode;

	/**
	 * A surjective mapping vertices(\a H) -> vertices(\a G).
	 *
	 * For each vertex \a vH of the biconnected components graph,
	 * m_hNode_gNode[\a vH] is the vertex of the original graph which \a vH is
	 * corresponding to.
	 */
	NodeArray<node> m_hNode_gNode;

	/**
	 * A bijective mapping edges(\a H) -> edges(\a G).
	 *
	 * For each edge \a eH of the biconnected components graph,
	 * m_hEdge_gEdge[\a eH] is the edge of the original graph which \a eH is
	 * corresponding to.
	 */
	EdgeArray<edge> m_hEdge_gEdge;
	//! @}

	/** @{
	 * Temporary variable.
	 *
	 * It is needed for the generation of the BC-tree by DFS method. It has to be a
	 * member of class BCTree due to recursive calls to biComp().
	 */
	int m_count;

	/**
	 * Temporary array.
	 *
	 * It is needed for the generation of the BC-tree by DFS method. It has to be a
	 * member of class BCTree due to recursive calls to biComp().
	*/

	NodeArray<int> m_number;
	/**
	 * Temporary array.
	 *
	 * It is needed for the generation of the BC-tree by DFS method. It has to be a
	 * member of class BCTree due to recursive calls to biComp().
	 */
	NodeArray<int> m_lowpt;

	/**
	 * Temporary stack.
	 *
	 * It is needed for the generation of the BC-tree by DFS method. It has to be a
	 * member of class BCTree due to recursive calls to biComp().
	 */
	ArrayBuffer<adjEntry> m_eStack;

	/**
	 * Temporary array.
	 *
	 * It is needed for the generation of the BC-tree by DFS method. It has to be a
	 * member of class BCTree due to recursive calls to biComp().
	 */
	NodeArray<node> m_gtoh;

	/**
	 * Temporary list.
	 *
	 * It is needed for the generation of the BC-tree by DFS method. It has to be a
	 * member of class BCTree due to recursive calls to biComp().
	 */
	SList<node> m_nodes;

	/** @}
	 * Initialization.
	 *
	 * initializes all data structures and generates the BC-tree and the
	 * biconnected components graph by call to biComp().
	 * \param vG is the vertex of the original graph which the DFS algorithm starts
	 * with.
	 */
	void init (node vG);
	//! @}

	/**
	 * Initialization for not connected graphs
	 *
	 * initializes all data structures and generates a forest of BC-trees and the
	 * biconnected components graph by call to biComp().
	 * \param vG is the vertex of the original graph which the DFS algorithm starts
	 * first with.
	 */
	void initNotConnected (node vG);

	/**
	 * Generates the BC-tree and the biconnected components graph
	 * recursively.
	 *
	 * The DFS algorithm is based on J. Hopcroft and R. E. Tarjan: Algorithm 447:
	 * Efficient algorithms for graph manipulation. <em>Comm. ACM</em>, 16:372-378
	 * (1973).
	 */
	void biComp (adjEntry adjuG, node vG);

	/** @{
	 * Returns the parent of a given BC-tree-vertex.
	 * \param vB is a vertex of the BC-tree or \a nullptr.
	 * \return the parent of \p vB in the BC-tree structure, if \p vB is not the
	 * root of the BC-tree, and \c nullptr, if \p vB is \c nullptr or the root of the
	 * BC-tree.
	 */
	virtual node parent (node vB) const;

	/**
	 * Calculates the nearest common ancestor of two vertices of the
	 * BC-tree.
	 * \param uB is a vertex of the BC-tree.
	 * \param vB is a vertex of the BC-tree.
	 * \return the nearest common ancestor of \p uB and \p vB.
	 */
	node findNCA (node uB, node vB) const;

	//! @}

public:
	/**
	 * A constructor.
	 *
	 * This constructor does only call init() or initNotConnected().
	 * BCTree(\p G) is equivalent to BCTree(\p G, \c G.firstNode()).
	 * \param G is the original graph.
	 * \param callInitConnected decides which init is called, default call is init()
	 */
	explicit BCTree (Graph& G, bool callInitConnected = false) : m_G(G), m_eStack(G.numberOfEdges()) {
		if (!callInitConnected) {
			init(G.firstNode());
		}
		else {
			initNotConnected(G.firstNode());
		}
	}

	/**
	 * A constructor.
	 *
	 * This constructor does only call init() or initNotConnected().
	 * \param G is the original graph.
	 * \param vG is the vertex of the original graph which the DFS algorithm starts
	 * \param callInitConnected decides which init is called, default call is init()
	 */
	BCTree (Graph& G, node vG, bool callInitConnected = false) : m_G(G), m_eStack(G.numberOfEdges()) {
		if (!callInitConnected)
			init(vG);
		else initNotConnected(vG);
	}

	//! Virtual destructor.
	virtual ~BCTree () { }

	//! Returns the original graph.
	const Graph& originalGraph () const { return m_G; }
	//! Returns the BC-tree graph.
	const Graph& bcTree () const { return m_B; }
	//! Returns the biconnected components graph.
	const Graph& auxiliaryGraph () const { return m_H; }
	//! @}

	//! Returns the number of B-components.
	int numberOfBComps () const { return m_numB; }
	//! Returns the number of C-components.
	int numberOfCComps () const { return m_numC; }
	//! @}

	/** @{
	 * Returns the type of a vertex of the original graph.
	 * \param vG is a vertex of the original graph.
	 * \return the type of \p vG.
	 */
	GNodeType typeOfGNode (node vG) const { return m_bNode_type[m_hNode_bNode[m_gNode_hNode[vG]]]==BNodeType::BComp ? GNodeType::Normal : GNodeType::CutVertex; }

	/**
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
	 */
	virtual node bcproper (node vG) const { return m_hNode_bNode[m_gNode_hNode[vG]]; }

	/**
	 * Returns the BC-tree-vertex representing the biconnected component
	 * which a given edge of the original graph is belonging to.
	 * \param eG is an edge of the original graph.
	 * \return the vertex of the BC-tree representing the B-component which \p eG
	 * is belonging to.
	 */
	virtual node bcproper (edge eG) const { return m_hEdge_bNode[m_gEdge_hEdge[eG]]; }

	/**
	 * Returns a vertex of the biconnected components graph corresponding to
	 * a given vertex of the original graph.
	 * \param vG is a vertex of the original graph.
	 * \return a vertex of the biconnected components graph:
	 * - If \p vG is not a cut-vertex, then rep(\p vG) returns the very vertex of
	 *   the biconnected components graph corresponding to \p vG.
	 * - If \p vG is a cut-vertex, then rep(\p vG) returns the very vertex of the
	 *   biconnected components graph representing the C-component which \p vG is
	 *   belonging to.
	 */
	node rep (node vG) const { return m_gNode_hNode[vG]; }

	/**
	 * Returns the edge of the biconnected components graph corresponding to
	 * a given edge of the original graph.
	 * \param eG is an edge of the original graph.
	 * \return the edge of the biconnected components graph corresponding to \p eG.
	 */
	edge rep (edge eG) const { return m_gEdge_hEdge[eG]; }
	//! @}

	/** @{
	 * returns the vertex of the original graph which a given vertex of the
	 * biconnected components graph is corresponding to.
	 * \param vH is a vertex of the biconnected components graph.
	 * \return the vertex of the original graph which \p vH is corresponding to.
	 */
	node original (node vH) { return m_hNode_gNode[vH]; }

	/**
	 * Returns the edge of the original graph which a given edge of the
	 * biconnected components graph is corresponding to.
	 * \param eH is an edge of the biconnected components graph.
	 * \return the edge of the original graph which \p eH is corresponding to.
	 */
	edge original (edge eH) const { return m_hEdge_gEdge[eH]; }
	//! @}

	/** @{
	 * Returns the type of the biconnected component represented by a given
	 * BC-tree-vertex.
	 * \param vB is a vertex of the BC-tree.
	 * \return the type of the biconnected component represented by \p vB.
	 */
	BNodeType typeOfBNode (node vB) const { return m_bNode_type[vB]; }

	/**
	 * Returns a linear list of the edges of the biconnected components
	 * graph belonging to the biconnected component represented by a given
	 * BC-tree-vertex.
	 * \param vB is a vertex of the BC-tree.
	 * \return a linear list of edges of the biconnected components graph:
	 * - If \p vB is representing a B-component, then the edges in the list are the
	 *   copies of the edges belonging to the B-component.
	 * - If \p vB is representing a C-component, then the list is empty.
	 */
	const SList<edge>& hEdges (node vB) const { return m_bNode_hEdges[vB]; }

	/**
	 * Returns the number of edges belonging to the biconnected component
	 * represented by a given BC-tree-vertex.
	 * \param vB is a vertex of the BC-tree.
	 * \return the number of edges belonging to the B- or C-component represented
	 * by \p vB, particularly 0 if it is a C-component.
	 */
	int numberOfEdges (node vB) const { return m_bNode_hEdges[vB].size(); }

	/**
	 * Returns the number of vertices belonging to the biconnected component
	 * represented by a given BC-tree-vertex.
	 * \param vB is a vertex of the BC-tree.
	 * \return the number of vertices belonging to the B- or C-component
	 * represented by \p vB, cut-vertices inclusive, particularly 1 if it is a
	 * C-component.
	 */
	int numberOfNodes (node vB) const { return m_bNode_numNodes[vB]; }
	//! @}

	/** @{
	 * Returns the BC-tree-vertex representing the B-component which two
	 * given vertices of the original graph are belonging to.
	 * \param uG is a vertex of the original graph.
	 * \param vG is a vertex of the original graph.
	 * \return If \p uG and \p vG are belonging to the same B-component, the very
	 * vertex of the BC-tree representing this B-component is returned. Otherwise,
	 * \a nullptr is returned. This member function returns the representative of the
	 * correct B-component even if \p uG or \p vG or either are cut-vertices and
	 * are therefore belonging to C-components, too.
	 */
	node bComponent (node uG, node vG) const;

	/**
	 * Calculates a path in the BC-tree.
	 * \param sG is a vertex of the original graph.
	 * \param tG is a vertex of the original graph.
	 * \return the path from bcproper(\p sG) to bcproper(\p tG) in the BC-tree as a
	 * linear list of vertices.
	 * \post <b>The SList<node> instance is created by this function and has to be
	 * destructed by the caller!</b>
	 */
	SList<node>& findPath (node sG, node tG) const;

	/**
	 * Calculates a path in the BC-tree.
	 * \param sB is a vertex of the BC-tree.
	 * \param tB is a vertex of the BC-tree.
	 * \return the path from (\p sB) to bcproper(\p tB) in the BC-tree as a
	 * linear list of vertices.
	 * \post <b>The SList<node> instance is created by this function and has to be
	 * destructed by the caller!</b>
	 */
	SList<node>* findPathBCTree (node sB, node tB) const;

	/**
	 * Returns a vertex of the biconnected components graph corresponding to
	 * a given vertex of the original graph and belonging to the representation of
	 * a certain biconnected component given by a vertex of the BC-tree.
	 * \param uG is a vertex of the original graph.
	 * \param vB is a vertex of the BC-tree.
	 * \return a vertex of the biconnected components graph:
	 * - If \p uG is belonging to the biconnected component represented by \p vB,
	 *   then repVertex(\p uG, \p vB) returns the very vertex of the biconnected
	 *   components graph corresponding to \p uG within the representation of
	 *   \p vB.
	 * - Otherwise, repVertex(\p uG, \p vB) returns \a nullptr.
	 */
	virtual node repVertex (node uG, node vB) const;

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
	 * \param uB is a vertex of the BC-tree.
	 * \param vB is a vertex of the BC-tree.
	 * \return a vertex of the biconnected components graph:
	 * - If \p uB == \p vB and they are representing a B-component, then
	 *   cutVertex(\p uB, \p vB) returns \a nullptr.
	 * - If \p uB == \p vB and they are representing a C-component, then
	 *   cutVertex(\p uB, \p vB) returns the single isolated vertex of the
	 *   biconnected components graph which is the copy of the C-component.
	 * - If \p uB and \p vB are \a neighbours in the BC-tree, then there exists
	 *   a cut-vertex leading from the biconnected component represented by \p vB
	 *   to the biconnected component represented by \p uB. cutVertex(\p uB, \p vB)
	 *   returns the very copy of this vertex within the biconnected components
	 *   graph which belongs to the copy of the biconnected component represented
	 *   by \p vB.
	 * - Otherwise, cutVertex(\p uB, \p vB) returns \a nullptr.
	 */
	virtual node cutVertex (node uB, node vB) const;

	//! @}
private:
	//! Copy constructor is undefined!
	BCTree(const BCTree &) = delete;

	//! Assignment operator is undefined!
	BCTree &operator=(const BCTree &) = delete;

	void initBasic(node vG);
	void initEdges();
};

}
