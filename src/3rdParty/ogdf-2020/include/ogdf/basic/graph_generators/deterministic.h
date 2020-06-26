/** \file
 * \brief Declaration of deterministic graph generators.
 *
 * \author Carsten Gutwenger, Markus Chimani, Jöran Schierbaum
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

namespace ogdf {

/**
 * @addtogroup graph-generators
 * @{
 */

//! @name Deterministic graph generators
//! @{

/**
 * @copydoc ogdf::customGraph(Graph &G, int n, List<std::pair<int,int>> edges)
 * @param nodes resulting array mapping node index to the actual node
 */
OGDF_EXPORT void customGraph(Graph &G, int n, List<std::pair<int,int>> edges, Array<node> &nodes);

//! Creates a custom graph using a list of pairs to determine the graph's edges.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param edges is a list of pairs, each one representing two nodes that should
 *        be connected by an edge in the generated graph.
 */
inline void customGraph(Graph &G, int n, List<std::pair<int,int>> edges) {
	Array<node> nodes;
	customGraph(G, n, edges, nodes);
}

//! Creates a circulant graph.
/**
 * Generates a simple, undirected graph on \f$n\f$ nodes \f$V := v_0,v_1,\ldots,v_{n-1}\f$
 * that contains exactly the edges \f$
 *  \{v_iv_{i+d}\colon v_i \in V, d \in \text{jumps}\}
 * \f$ where node indices are to be understood modulo \f$n\f$.
 * The order of nodes induced by \p G is the sequence \f$V\f$ given above.
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param jumps is the array of distances for edges to be created.
 * @code
 * ogdf::Graph G;
 * ogdf::circulantGraph(G, 11, ogdf::Array<int>({1,2,4}));
 * @endcode
 */
OGDF_EXPORT void circulantGraph (Graph &G, int n, Array<int> jumps);

//! Creates a regular lattice graph.
/**
 * Generates a cycle on \p n sequential nodes, where any two nodes whose
 * distance is at most \p k / 2 are connected by an additional edge.
 * @see ::circulantGraph(Graph&, int, Array<int>)
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes in the graph.
 * @param k is the degree of each node.
 * @pre \p n must be at least 4, \p k must be an even number between 0 and \p n-2.
 */
OGDF_EXPORT void regularLatticeGraph(Graph &G, int n, int k);

//! Creates a regular tree.
/**
 * @param G is assigned the tree.
 * @param n is the number of nodes of the tree.
 * @param children is the number of children per node. root has index 0, the next level has
 * indizes 1...children, the children of node 1 have indizes children+1...2*children, etc.
 * if number of nodes does not allow a regular node, the "last" node will have fewer children.
 */
OGDF_EXPORT void regularTree(Graph& G, int n, int children);

//! Creates the complete graph \a K_n.
/**
 * The returned graph is directed acyclic.
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 */
OGDF_EXPORT void completeGraph(Graph &G, int n);

//! Creates the complete k-partite graph \a K_{k1,k2,...,kn}.
/**
 * The returned graph is directed acyclic.
 *
 * @param G is assigned the generated graph.
 * @param signature contains the positive values k1, k2, ..., kn.
 */
OGDF_EXPORT void completeKPartiteGraph(Graph &G, const Array<int> &signature);

//! Creates the complete bipartite graph \a K_{n,m}.
/**
 * The returned graph is directed acyclic.
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the first partition set.
 * @param m is the number of nodes of the second partition set.
 */
OGDF_EXPORT void completeBipartiteGraph(Graph &G, int n, int m);

//! Creates the graph \a W_n: A wheel graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes on the rim of the wheel (W_n).
 * @pre \p n must be at least 2.
 */
OGDF_EXPORT void wheelGraph(Graph &G, int n);

//! Creates the graph \a Q^n: A <tt>n</tt>-cube graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of the cube's dimensions (n>=0).
 */
OGDF_EXPORT void cubeGraph(Graph &G, int n);

//! Modifies \p G by adding its <tt>s</tt>-th suspension.
/**
 * A suspension node is a node that is connected to all other nodes in the graph.
 * This function adds \p s such suspension nodes that will not be directly connected
 * to each other.
 *
 * @param G is the graph to extend.
 * @param s is the amount of suspension nodes to add.
 */
OGDF_EXPORT void suspension(Graph &G, int s);

//! Creates a (toroidal) grid graph on \p n x \p m nodes.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes on first axis.
 * @param m is the number of nodes on second axis.
 * @param loopN if the grid is cyclic on first axis
 * @param loopM if the grid is cyclic on second axis
 */
OGDF_EXPORT void gridGraph(Graph &G, int n, int m, bool loopN, bool loopM);

//! Creates a generalized Petersen graph.
/**
 * Creates an outer cycle of nodes \a 1, ..., \p n, each of which has a direct
 * neighbor (a corresponding inner node). For two outer nodes \a i, \a j, there
 * is an edge between their corresponding inner nodes if the absolute difference
 * of \a i and \a j equals the jump length \p m.
 *
 * If no values for \p n or \p m are given, assume the standard Petersen graph
 * of \c 5 nodes and a jump length of \c 2.
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes on the outer cycle.
 * @param m is the length of jumps for the inner part.
 */
OGDF_EXPORT void petersenGraph(Graph &G, int n = 5, int m = 2);

//! Creates a graph with \p nodes nodes and no edges.
/**
 * @param G is assigned the generated graph.
 * @param nodes is the number of nodes of the generated graph.
 */
OGDF_EXPORT void emptyGraph(Graph &G, int nodes);

//! @}

/** @} */

}
