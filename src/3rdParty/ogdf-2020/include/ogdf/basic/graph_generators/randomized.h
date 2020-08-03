/** \file
 * \brief Declaration of randomized graph generators.
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

#include <ogdf/basic/graph_generators/randomGeographicalThresholdGraph.h>
#include <ogdf/basic/graph_generators/randomHierarchy.h>
#include <ogdf/cluster/ClusterGraph.h>

namespace ogdf {

/**
 * @addtogroup graph-generators
 * @{
 */

//! @name Randomized graph generators
//! @{

//! Creates a random <tt>d</tt>-regular graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param d is the degree of each vertex
 * @pre \p n * \p d must be even
 * @warning This method is not guaranteed to terminate!
 */
OGDF_EXPORT void randomRegularGraph(Graph &G, int n, int d);

//! Creates a random graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 */
OGDF_EXPORT void randomGraph(Graph &G, int n, int m);

//! Creates a random simple graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 */
OGDF_EXPORT bool randomSimpleGraph(Graph &G, int n, int m);

//! Creates a random simple graph.
/**
 * Algorithm based on PreZER/LogZER from:
 * Sadegh Nobari, Xuesong Lu, Panagiotis Karras, and Stéphane Bressan. 2011. Fast random graph generation.
 * In Proceedings of the 14th International Conference on Extending Database Technology (EDBT/ICDT '11),
 * ACM, New York, NY, USA, 331-342. DOI=http://dx.doi.org/10.1145/1951365.1951406
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param pEdge is the probability for each edge to be added into the graph.
 * @pre /p pEdge is in [0, 1]
 */
OGDF_EXPORT bool randomSimpleGraphByProbability(Graph &G, int n, double pEdge);

//! Creates a random simple and connected graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 */
OGDF_EXPORT bool randomSimpleConnectedGraph(Graph &G, int n, int m);

//! Creates a random biconnected graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 * @note \p n has a lower bound of 3, and \p m a lower bound of \p n.
 * If the parameters are smaller than that, they get increased prior
 * to the algorithm.
 */
OGDF_EXPORT void randomBiconnectedGraph(Graph &G, int n, int m);

//! Creates a random connected (simple) planar (embedded) graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 * @note \p n has a lower bound of 1, and \p m has a lower bound of
 * \p n and an upper bound of \f$3n-6\f$. The supplied values are
 * adjusted if they are out of these bounds.
 */
OGDF_EXPORT void randomPlanarConnectedGraph(Graph &G, int n, int m);

//! Creates a random planar biconnected (embedded) graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 * @param multiEdges determines if the generated graph may contain
 *        multi-edges.
 * @note \p n has a lower bound of 3, and \p m has a lower bound of
 * \p n and an upper bound of \f$3n-6\f$. The supplied values are
 * adjusted if they are out of these bounds.
 */
OGDF_EXPORT void randomPlanarBiconnectedGraph(Graph &G, int n, int m, bool multiEdges = false);

//! Creates a random planar biconnected acyclic (embedded) digraph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 * @param p up to \p m * \p p edges will be reversed preversing acyclicity; default = 0.0.
 * @param multiEdges determines if the generated graph may contain
 *        multi-edges; default = false.
 * @pre \p d is between 0.0 and 1.0
 * @note \p n has a lower bound of 3, and \p m has a lower bound of
 * \p n and an upper bound of \f$3n-6\f$. The supplied values are
 * adjusted if they are out of these bounds.
 */
OGDF_EXPORT void randomPlanarBiconnectedDigraph(Graph &G, int n, int m, double p = 0, bool multiEdges = false);

//! Creates a random upward planar biconnected (embedded) digraph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 * @note \p n has a lower bound of 3, and \p m has a lower bound of
 * \p n and an upper bound of \f$3n-6\f$. The supplied values are
 * adjusted if they are out of these bounds.
 */
OGDF_EXPORT void randomUpwardPlanarBiconnectedDigraph(Graph &G, int n, int m);

//! Creates a random planar graph, that is connected, but not biconnected.
/**
 * @param G is assigned the generated graph.
 * @param n is the max. number of nodes in each biconnected component
 * @param m is the max. number of edges in each biconnected component
 * @param b is the number of biconnected components
 *
 * @pre It holds that n > 1, m >= n (unless n = 2, m = 1) and b > 1.
 */
OGDF_EXPORT void randomPlanarCNBGraph(Graph &G, int n, int m,	int b);

//! Creates a random triconnected (and simple) graph.
/**
 * The graph generator proceeds as follows. It starts with a \a K_4 and performs
 * then \p n -4 split node operations on randomly selected nodes of the graph
 * constructed so far. Each such operation splits a node \a v into two nodes
 * \a x and \a y and distributes \a v's neighbors to the two nodes such that each
 * node gets at least two neighbors. Additionally, the edge (\a x,\a y) is inserted.
 *
 * The neighbors are distributed such that a neighbor of \a v becomes
 *   - only a neighbor of \a x with probability \p p1;
 *   - only a neighbor of \a y with probability \p p1;
 *   - a neighbor of both \a x and \a y with probability 1.0 - \p p1 - \p p2.
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes in the generated graph.
 * @param p1 is the probability that an edge is moved only to the left
 *        node after splitting a node.
 * @param p2 is the probability that an edge is moved only to the right
 *        node after splitting a node.
 *
 * The probability for a neighbor to be moved to both split nodes is
 * 1.0 - \p p1 - \p p2. The higher this probability, the higher the density
 * of the resulting graph.
 *
 * \pre The probabilities \a p1 and \a p2 must lie between 0.0 and 1.0, and
 *      \p p1 + \p p2 <= 1.0.
 * @note \p n has a lower bound of 4 and will get increased to this if smaller.
 */
OGDF_EXPORT void randomTriconnectedGraph(Graph &G, int n, double p1, double p2);

//! Creates a random planar triconnected (and simple) graph.
/**
 * This graph generator works in two steps.
 *   -# A planar triconnected 3-regular graph is constructed using successive
 *      splitting of pairs of nodes. The constructed graph has \p n nodes and
 *      1.5\p n edges.
 *   -# The remaining edges are inserted by successive splitting of faces
 *      with degree four or greater.
 * The resulting graph also represents a combinatorial embedding.
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes in the generated graph.
 * @param m is the number of edges in the generated graph.
 *
 * @note
 *   - \p n >= 4 and \p n must be even; otherwise, \p n is adjusted
 *     to the next feasible integer.
 *   - 1.5\p n <= \p m <= 3\p n -6; otherwise, \p m is adjusted
 *     to a feasible value.
 */
OGDF_EXPORT void randomPlanarTriconnectedGraph(Graph &G, int n, int m);

//! Creates a random planar triconnected (and simple) graph.
/**
 * This graph generator creates a planar triconnected graph by successive
 * node splitting. It starts with the \a K_4 and performs \p n -4 node
 * splits. Each such split operation distributes a node's neighbors to the
 * two nodes resulting from the split. Aftewards, two further edges can be
 * added; the probability for adding these edges is given by \p p1 and \p p2.
 * The higher these probabilities, the denser the resulting graph. Note that
 * a simple planar triconnected graph has between 1.5\p n and 3\p n -6 edges.
 *
 * \pre 0.0 <= \p p1, \p p2 <= 1.0.
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes in the generated graph.
 * @param p1 is the probability for the first additional edge to be added.
 * @param p2 is the probability for the second additional edge to be added.
 * @note \p n has a lower bound of 4 and will get increased to this if smaller.
 */
OGDF_EXPORT void randomPlanarTriconnectedGraph(Graph &G, int n, double p1, double p2);

//! Creates a random tree (simpler version.
/**
 * @param G is assigned the tree.
 * @param n is the number of nodes of the tree.
 */
OGDF_EXPORT void randomTree(Graph& G, int n);

//! Creates a random tree.
/**
 * @param G is assigned the tree.
 * @param n is the number of nodes of the tree.
 * @param maxDeg is the maximal allowed node degree; 0 means no restriction.
 * @param maxWidth is the maximal allowed width of a level; 0 means no restriction.
 * @note if \p maxDeg or \p maxWidth are 0 (or negative), they are set to \p n
 */
OGDF_EXPORT void randomTree(Graph &G, int n, int maxDeg, int maxWidth);

//! Assigns random clusters to a given graph \p G.
/**
 * This function is called with a graph \p G and creates randomly clusters.
 * The resulting cluster graph is always c-connected and,
 * if G is planar, also c-planar.
 * @param G is the input graph.
 * @param C is a cluster graph for \p G.
 * @param cNum is the maximal number of Clusters introduced.
 * \pre \p G is connected and not empty and \a C is initialized with \a G.
 */
OGDF_EXPORT void randomClusterPlanarGraph(ClusterGraph &C,Graph &G,int cNum);

//! Assigns random clusters to a given graph \p G.
/**
 * This function is called with a graph \p G and creates randomly clusters.
 * @param G is the input graph.
 * @param C is a cluster graph for \p G.
 * @param cNum is the maximal number of clusters introduced.
 * \pre \p G is connected and not empty and \p C is initialized with \p G.
 */
OGDF_EXPORT void randomClusterGraph(ClusterGraph &C,Graph &G,int cNum);

//! Assigns a specified cluster structure to a given graph \p G, and assigns vertices to clusters.
/**
 * This function is called with a graph \p G and the root of a second graph, resembling a tree,
 * that gives the cluster structure. Then, the vertices of G are randomly assigned to the clusters,
 * where we can guarantee that any leaf-cluster has (on average) <i>moreInLeaves</i>-times more vertices
 * than a non-leaf cluster. (E.g. if \p moreInLeaves = 5, any leaf will contain roughly 5 times more vertices than
 * an inner cluster)
 * @param C is a cluster graph for \p G, to be assigned the solution.
 * @param G is the input graph.
 * @param root is a node in some other graph (say \a T). \a T is a tree that we will consider rooted at \p root.
 *        \a T is the pattern for the cluster hierarchy.
 * @param moreInLeaves is a factor such that leaf-clusters have on average <i>moreInLeaves</i>-times more
 *        vertices than inner clusters
 * \pre \p G contains at least twice as many nodes as \a T has leaves.
 */
OGDF_EXPORT void randomClusterGraph(ClusterGraph& C, const Graph& G, const node root, int moreInLeaves);

//! Creates a random (simple) directed graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes in the generated graph.
 * @param p is the probability that an edge is created (for each node pair)
 */
OGDF_EXPORT void randomDigraph(Graph &G, int n, double p);

//! Creates a random (simple, biconnected) series parallel DAG.
/**
 * This function creates a random series parallel biconnected DAG.
 * Note, that the resulting graph is trivially upward planar!
 * To use this generator for experiments, e.g. concerning upward planarity,
 * you can fit the graph by reversing some edges with the parameter 0 < flt < 1.
 *
 * @param G is assigned the generated graph.
 * @param edges is the number of edges in the generated graph.
 * @param p   = probability of a series composition; default = 0.5
 * @param flt = up to edges*flt edges will be reversed preversing acyclicity; default = 0.0
 * @pre \p p is in \f$[0.0, 1.0]\f$, and \p flt is in \f$[0.0, 1.0)\f$.
 */
OGDF_EXPORT void randomSeriesParallelDAG(Graph &G, int edges, double p = 0.5, double flt = 0.0);

//! Creates a random geometric graph by laying out nodes in a unit n-cube.
//! Nodes with a distance < threshold are connected,
//! 0 <= threshold <= sqrt(dimension). The graph is simple.
/**
 * @param G is assigned the generated graph.
 * @param nodes is the number of nodes of the generated graph.
 * @param threshold is threshold radius of nodes which will be connected.
 * @param dimension is the dimension of the cube.
 */
OGDF_EXPORT void randomGeometricCubeGraph(Graph &G, int nodes, double threshold, int dimension = 2);

//! Generates a Waxman graph where nodes are uniformly randomly placed in a grid, then edges
//! are inserted based on nodes' euclidean distances.
/**
 *     Routing of Multipoint Connections
 *     Bernard M. Waxman (1988)
 *
 * After generating the nodes, edges are inserted between each pair of nodes \a v, \a w with
 * probability based on their euclidean distance \f$\beta \exp{\frac{-||v-w||}{m \, \alpha}}\f$
 * where \f$m:=\max\limits_{u,v}||u-v||\f$.
 *
 * @param G is assigned the generated graph.
 * @param nodes is the number of nodes of the generated graph.
 * @param alpha is a parameter for the probability in the range (0,1].
 *        Small values increase the density of short edges relative to longer ones.
 * @param beta is a parameter for the probability in the range (0,1].
 *        Large values result in a graph with higher edge density.
 * @param width is the width of the area the nodes are distributed in.
 * @param height is the height of the area the nodes are distributed in.
 */
OGDF_EXPORT void randomWaxmanGraph(Graph &G, int nodes, double alpha, double beta, double width = 1.0, double height = 1.0);

//! Creates a graph where new nodes are more likely to connect to nodes with high degree.
/**
 * Implements the Preferential Attachment algorithm as described in:
 *     Emergence of Scaling in Random Networks
 *     Albert-Laszlo Barabasi and Reka Albert
 *     https://arxiv.org/abs/cond-mat/9910332v1
 * This algorithm creates edges based on the degree of nodes,
 * so it is most useful to apply this to a pre-built graph.
 * If no graph is supplied, a complete graph of \p minDegree nodes
 * is generated and the algorithm adds \p nodes - \p minDegree nodes.
 * If a graph is supplied, it must contain at least \p minDegree
 * nodes of degree 1.
 *
 * @param G is the input graph (see above) and is assigned the expanded graph.
 * @param nodes is the number of nodes to be added to graph.
 * @param minDegree is the minimum degree of new nodes.
 */
OGDF_EXPORT void preferentialAttachmentGraph(Graph &G, int nodes, int minDegree);

//! Creates a "small world" graph as described by Watts & Strogatz
/**
 * Takes a regular lattice graph and, with given probability, rewires each
 * edge to a random other non-neighbor.
 *
 *   Collective dynamics of ‘small-world’ networks
 *   https://www.nature.com/articles/30918.pdf
 *
 * @warning
 * This implementation does not perform very well if \p k is close to half of \p n for large graphs.
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param k is the initial degree of each node and must be even and smaller than half of \p n.
 * @param probability determines how likely each edge is rewired. A probability of 0 will not
 *        modify the graph, while one of 1 will cause full randomness.
 */
OGDF_EXPORT void randomWattsStrogatzGraph(Graph &G, int n, int k, double probability);

//! Creates a graph where edges are inserted based on given weights
/**
 * Implements the algorithm described in:
 *     The average distance in a random graph with given expected degrees
 *     Fang Chung and Linyuan Lu
 *     http://www.math.ucsd.edu/~fan/wp/aveflong.pdf
 *
 * Given an expected degree distribution of length \a n: \f$w:=(w_1, ..., w_n)\f$ with
 * \f$0 < w_k < n\f$.
 *
 * Let \f$S:=\sum_{k=1}^{n}w_k\f$ be the sum over all expected degrees.
 * Consider each edge independently and insert it with probability
 * \f$p_{ij} := \frac{w_i \, w_j}{S}\f$.
 * Therefore, to get percentages in \f$(0,1)\f$ we assert that \f$\max\limits_k(w_k)^2 < S\f$.
 *
 * @pre
 * Each degree must be strictly between \a 0 and \a n, and the square of the maximal expected
 * degree must be lower than the sum of all expected degrees.
 *
 * @param G is assigned the generated graph.
 * @param expectedDegreeDistribution is a list of expected degrees, or weights,
 *        for the individual nodes. Its length defines the number of nodes \a n.
 */
OGDF_EXPORT void randomChungLuGraph(Graph &G, Array<int> expectedDegreeDistribution);

//! Inserts edges into the given graph based on probabilities given by a callback function
/**
 * Iterates through each distinct pair of nodes and inserts an edge with the probability returned
 * by the provided callback function.
 *
 * The resulting graph is guaranteed to be simple if:
 *  - the input graph had no edges, or
 *  - the input graph was simple and the callback function returns 0 for each pair of nodes that was
 *    connected before.
 *
 * @param G is a graph that should have at least two nodes (so edges can be generated)
 * @param probability is a callback function that, for any given pair of nodes, returns a probability
 *        between 0 and 1 for the two nodes to be connected.
 */
OGDF_EXPORT void randomEdgesGraph(Graph &G, std::function<double(node, node)> probability);

//! @}

/** @} */

}
