/** \file
 * \brief Declaration of class UpwardPlanarity, which implements different types
 * of algorithms testing upward planarity of graphs with different restrictions.
 * Actually, restrictions are:
 *   - general acyclic digraphs (using satisfiability)
 *   - fixed embedding
 *   - single source
 *   - triconnected
 *
 * Also allows to compute a maximal feasible upward planar subgraph.
 *
 * \author Robert Zeranski
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
#include <ogdf/basic/SList.h>
#include <ogdf/basic/CombinatorialEmbedding.h>


namespace ogdf {


//! Upward planarity testing and embedding.
/**
 * This class provides various static functions for upward planarity testing
 * and embedding. These functions perform different tasks (testing, embedding,
 * augmenting) and pose different restrictions on the input graph (general,
 * biconnected, single source).
 *
 * We use a strict naming scheme to make it clear what the functions are doing
 * and which restrictions they have. The prefix of the function name denotes
 * the <em>task</em>:
 *   - <tt>isUpwardPlanar</tt>: Tests if the input graph is upward planar.
 *   - <tt>upwardPlanarEmbed</tt>: First tests if the input graph is upward planar, and if yes
 *     upward planarly embeds it.
 *   - <tt>upwardPlanarAugment</tt>: Adds additional edges to the input graph such that the graph
 *     remains upward planar and fulfills a special property like single source.
 *
 * The suffix of the function name (if present) describes <em>additional restrictions</em>:
 *   - <tt>singleSource</tt>: The input graph has exactly one source (but possibly several sinks).
 *   - <tt>seriesParallel</tt>: The input graph is a series-parallel graph.
 *
 * Some of the functions take a combinatorial embedding (e.g., given by the order in the adjacency
 * lists) as input and test this given embedding. These functions are appended by <tt>_embedded</tt>.
 */
class OGDF_EXPORT UpwardPlanarity
{
public:
	/**
	 * @name General digraphs
	 */
	//@{

	//! Tests whether graph \p G is upward planar (using satisfiability).
	/**
	 * \param G is the input graph to be tested.
	 * \return true if \p G is upward planar, false otherwise.
	 */
	static bool isUpwardPlanar(Graph &G);

	//! Tests whether graph \p G is upward planar and embeds the graph by a upward planar embedding if possible (using satisfiability).
	/**
	 * \param G is the input graph to be embedded if it allows an upward planar embedding.
	 * \param externalToItsRight indicates the external face (on its right side)
	 * \return true if \p G is upward planar, false otherwise.
	 */
	static bool embedUpwardPlanar(Graph &G, adjEntry& externalToItsRight);

#if 0
	//! Computes a maximal feasible upward planar subgraph (MFUPS).
	/**
	 * \param G is the input graph, GC has to be a GraphCopy of G.
	 * \return number of edges of the MFUPS.
	 * 		   GC is the graph copy of G containing the upward planar embedding of the MFUPS
	 */
	static int maximalFeasibleUpwardPlanarSubgraph(const Graph &G, GraphCopy &GC);
#endif

	//@}
	/**
	 * @name Biconnected digraphs
	 */
	//@{

	//! Tests whether a biconnected graph \p G is upward planarly embedded.
	/**
	 * The fixed embedding of \p G is given by the order of \p G's adjacency lists.
	 *
	 * \param G is the input graph to be tested.
	 * \return true if \p G is upward planarly embedded, false otherwise.
	 */
	static bool isUpwardPlanar_embedded(const Graph &G);

	//! Tests whether a biconnected graph \p G is upward planarly embedded and computes the set of possible external faces.
	static bool isUpwardPlanar_embedded(const Graph &G, List<adjEntry> &possibleExternalFaces);

	//@}
	/**
	 * @name Triconnected digraphs
	 */
	//@{

	//! Tests whether the triconnected digraph \p G is upward planar.
	/**
	 * \remark If \p G is not triconnected the function returns false.
	 *
	 * \param G is the (triconnected) input digraph.
	 * \return true if \p G was triconnected and upward planar, false otherwise.
	 */
	static bool isUpwardPlanar_triconnected(const Graph &G);

	//! Upward planarly embeds the triconnected digraph \p G.
	/**
	 * \remark If \p G is not triconnected the function returns false.
	 *
	 * \param G is the (triconnected) input digraph.
	 * \return true if \p G was triconnected and upward planar, false otherwise.
	 */
	static bool upwardPlanarEmbed_triconnected(Graph &G);


	//@}
	/**
	 * @name Single-source digraphs
	 */
	//@{

	//! Tests whether the single-source digraph \p G is upward planar.
	/**
	 * \remark If \p G is not single-source the function returns false.
	 *
	 * \param G is the (single-source) input digraph.
	 * \return true if \p G was single-source and upward planar, false otherwise.
	 */
	static bool isUpwardPlanar_singleSource(const Graph &G);

	//! Upward planarly embeds the single-source digraph \p G.
	/**
	 * \remark If \p G is not single-source the function returns false.
	 *
	 * \param G is the (single-source) input digraph.
	 * \return true if \p G was single-source and upward planar, false otherwise.
	 */
	static bool upwardPlanarEmbed_singleSource(Graph &G);

	//! Tests whether single-source digraph \p G is upward planar, and if yes augments it to a planar st-digraph.
	/**
	 * \remark If \p G is not single-source the function returns false.
	 *
	 * If \p G is upward planar, this method adds a super sink node \a t and adds further edges such that the
	 * resulting digraph is a planar st-digraph.
	 *
	 * \param G is the input digraph which gets augmented.
	 * \return true if \p G is single-source and upward planar, false otherwise.
	 */
	static bool upwardPlanarAugment_singleSource(Graph &G);

	//! Tests whether single-source digraph \p G is upward planar, and if yes augments it to a planar st-digraph.
	/**
	 * \remark If \p G is not single-source the function returns false.
	 *
	 * If \p G is upward planar, this method adds a super sink node \p superSink and adds further edges such that the
	 * resulting digraph is a planar st-digraph.
	 *
	 * \param G              is the input digraph which gets augmented.
	 * \param superSink      is assigned the inserted super sink node.
	 * \param augmentedEdges is assigned the list of inserted edges.
	 * \return true if \p G is single-source and upward planar, false otherwise.
	 */
	static bool upwardPlanarAugment_singleSource(
		Graph &G,
		node &superSink,
		SList<edge> &augmentedEdges);


	//! Tests whether the embedding \p E of a single-source digraph is upward planar.
	/**
	 * \param E             is the given combinatorial embedding to be tested.
	 * \param externalFaces is assigned the list of possible external faces such that \p E is upward planar.
	 * \return true if \p E is upward planar, false otherwise.
	 */
	static bool isUpwardPlanar_singleSource_embedded(
		const ConstCombinatorialEmbedding &E,
		SList<face> &externalFaces);

	//! Tests if single-source digraph \p G is upward planarly embedded and augments it to a planar st-digraph.
	/**
	 * \param G              is the embedded input graph.
	 * \param superSink      is assigned the added super sink.
	 * \param augmentedEdges is assigned the list of added edges.
	 * \return true if \p G is upward planarly embedded (in this case \p G is also augmented by adding a
	 *         super sink and additional edges), false otherwise.
	 */
	static bool upwardPlanarAugment_singleSource_embedded(
		Graph &G,
		node  &superSink,
		SList<edge> &augmentedEdges);

	//@}
};

}
