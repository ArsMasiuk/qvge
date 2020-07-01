/** \file
 * \brief Declaration of the wrapper class of the Boyer-Myrvold planarity test
 *
 * \author Jens Schmidt, Markus Chimani
 *
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

#include <ogdf/planarity/PlanarityModule.h>
#include <ogdf/planarity/boyer_myrvold/BoyerMyrvoldPlanar.h>
#include <ogdf/planarity/ExtractKuratowskis.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/planarity/KuratowskiSubdivision.h>

namespace ogdf {

class KuratowskiWrapper;

//! Wrapper class used for preprocessing and valid invocation of the planarity test.
/**
 * @ingroup ga-planembed
 *
 * This class is part of the extended Boyer-Myrvold planarity embedding algorithm
 * to simplify invocation besides adding standard parameters (see classes in
 * BoyerMyrvoldInit.h and BoyerMyrvoldPlanar.h). In addition the linear-time
 * Boyer-Myrvold embedding algorithm was extended to extract multiple Kuratowski
 * Subdivisions, whose number can be limited as desired (see classes in
 * FindKuratowskis.h and ExtractKuratowskis.h). Furthermore all extracted
 * subdivisions are unique.
 *
 * <b>Input graph:</b>\n
 * There are no restrictions on the input graph \a G (except that it has to be finite,
 * but if you do not have infinite RAM, that should do it :) ). In particular, \a G hasn't
 * to be biconnected nor connected. Self-loops and multigraphs are possible, too.\n
 * Note: But if you want to use the extraction of Kuratowski Subdivisions, \a G has to be simple!
 *
 * <b>How to use it:</b>\n
 * There exist two main functions, consisting of the planarity test itself and the
 * planar embedding algorithm, which embeds the graph on the plane if possible.
 * Each one has a fast but destructive variant, designed to
 * be used on graphs that may be modified and a slower variant, which is for graphs
 * that must not be altered. Embeddings on the plane are given by a (genus 0)
 * cyclic ordering of adjacency lists in the graph. Functions for constant graphs
 * are available, too, if that makes sense for the function.
 *
 * <b>Examples:</b>\n
 * BoyerMyrvold::isPlanarDestructive(G), BoyerMyrvold::isPlanar(G):\n
 * Tests graph \a G for planarity with the Boyer-Myrvold planarity test.
 *
 * BoyerMyrvold::planarEmbedDestructive(G), BoyerMyrvold::planarEmbed(G), BoyerMyrvold::planarEmbed(G,H):\n
 * Tests graph \a G for planarity and returns a planar embedding in \a G,
 * if \a G is planar. If \a G is a constant graph, the embedding is given in the GraphCopySimple
 * \a H, so that both, the constant input graph and the resulting planar embedding are available.
 *
 * BoyerMyrvold::planarEmbedDestructive(G,output,i), BoyerMyrvold::planarEmbed(G,output,i):\n
 * Tests graph \a G for planarity and returns a planar embedding,
 * if \a G is planar. Otherwise up to \a i Kuratowski subdivisions are returned to the list
 * \a output. Use \a i = -1 for extraction of all subdivisions.
 *
 * BoyerMyrvold::planarEmbedDestructive(G,output,i), BoyerMyrvold::planarEmbed(G,output,i):\n
 * Tests graph \a G for planarity and returns a planar embedding,
 * if \a G is planar. Otherwise up to \a i Kuratowski subdivisions are returned to the list
 * \a output. Use \a i = -1 for extraction of all subdivisions. The extraction algorithm
 * doesn't use sets of \a bundles instead of subdivisions paths, so this is designed for
 * a fast computation while extracting some, but not a huge amount of Kuratowski Subdivisions.
 *
 * BoyerMyrvold::planarEmbedDestructive(G,output,i,true), BoyerMyrvold::planarEmbed(G,output,i,true):\n
 * This is the same as above, but now \a bundles are used to compute much more subdivisions.
 * Naturally the computation is slower than the function above, especially on large graphs.
 *
 * <b>Complete list of parameters for embedding functions:</b>\n
 * e.g. #planarEmbedDestructive(
 * - <b>Graph& g,</b>\n
 * This is the input Graph.
 * - <b>SList<KuratowskiWrapper>& output,</b>\n
 * All subdivisions are returned in this list.
 * - <b>int embeddingGrade,</b>\n
 * This flag has 5 options dependent on value \p i: \n
 * \a i = -3: no Embedding is computed\n
 * \a i = -2: no FindKuratowskiProcedure is performed\n
 * \a i = -1: all Kuratowski Subdivisions are extracted\n
 * \a i =  0: no Kuratowski Subdivision is extracted, but FindKuratowskiProcedure is started\n
 * \a i >  0: up to \a i Kuratowski Subdivisions are extracted.
 * - <b>bool bundles,</b>\n
 * If \a bundles are used, some paths between two Kuratowski nodes are replaced by whole
 * bundles of paths. On the one hand much more subdivisions can be extracted on the other
 * computation time growths.
 * - <b>bool limitStructures,</b>\n
 * If Kuratowski Structures are \a limited, all subdivisions in that Structures are extracted.
 * Thereby none of the efforts made in FindKuratowskiProcedure are lost, which is creditable
 * in comparison with limiting the number of extracted subdivisions. Note that the number
 * of extracted subdivisions can highly vary.
 * - <b>bool randomDFSTree,</b>\n
 * Iff \c true, a completely random DFS-Tree (the list of nodes and the adjacency-lists for
 * each node are permuted at random) is created each time the planarity test is called.
 * This is important for extracting huge amounts of Kuratowski subdivisions of
 * one single Graph, since randomizing the DFSTree yields to new unknown subdivisions.
 * Note that computation time growths up to 20 percent longer.
 * - <b>bool avoidE2Minors</b>\n
 * Two minortypes, namely \a E2/AE2 and \a A, construct identical subdivisions on some graphs.
 * To avoid this, set this flag \c true, otherwise \c false.
 *
 * )
 *
 * To experience the computation time difference to the PQTree-Planarity test please
 * switch to release-mode, since a lot of slow testroutines and assertions
 * were implemented in debug-mode. Also note the ability to transform KuratowskiWrapperLists
 * to Lists of KuratowskiSubdivision through calling the function \a transform. Within
 * this transformation is a switch to filter all similar or not similar Kuratowski
 * Subdivisions.
 */
class OGDF_EXPORT BoyerMyrvold : public PlanarityModule
{
protected:
	//! Class BoyerMyrvoldPlanar on heap
	BoyerMyrvoldPlanar* pBMP;

	//! Deletes BoyerMyrvoldPlanar on heap
	void clear() {
		delete pBMP;
		pBMP = nullptr;
	}

	//! The number of extracted Structures for statistical purposes
	int nOfStructures;

public:
	//! Constructor
	BoyerMyrvold() { pBMP = nullptr; }
	//! Destructor
	~BoyerMyrvold() { clear(); }

	//! The number of extracted Structures for statistical purposes
	int numberOfStructures() { return nOfStructures; }

	//! Returns true, iff \p g is planar
	/** This is the routine, which avoids the overhead of copying the input graph.
	 * It is therefore not suitable, if your graph must not be alterated!
	 */
	virtual bool isPlanarDestructive(Graph& g) override;

	//! Returns true, iff a copy of the constant graph \p g is planar
	/** Use this slower routine, if your graph must not be alterated.
	 */
	virtual bool isPlanar(const Graph& g) override;

	//! Returns true, if G is planar, false otherwise. If true, G contains a planar embedding.
	virtual bool planarEmbed(Graph &G) override {
		SList<KuratowskiWrapper> list;
		return planarEmbed(G,list);
	}

	//! Constructs a planar embedding of G. \p G \b has to be planar!
	/**
	 * Returns true if the embedding was successful.
	 * Returns false, if the given graph was non-planar (and leaves the
	 * graph in an at least partially deleted state)
	 *
	 * This routine is slightly faster than planarEmbed, but requires \p G to be planar.
	 * If \p G is not planar, the graph will be destroyed while trying to embed it!
	 */
	virtual bool planarEmbedPlanarGraph(Graph &G) override {
		SList<KuratowskiWrapper> list;
		return planarEmbedDestructive(G,list);
	}


	//! Transforms KuratowskiWrapper in KuratowskiSubdivision
	void transform(
		const KuratowskiWrapper& source,
		KuratowskiSubdivision& target,
		NodeArray<int>& count,
		EdgeArray<int>& countEdge);

	//! Transforms KuratowskiWrapper-List in KuratowskiSubdivision-List with respect to sieving constraints
	/**
	 * This method is called if BoyerMyrvold::planarEmbed was called with the Graph g previously.
	 */
	void transform(
		const SList<KuratowskiWrapper>& sourceList,
		SList<KuratowskiSubdivision>& targetList,
		const Graph& g,
		const bool onlyDifferent = false);


	//! Transforms KuratowskiWrapper-List in KuratowskiSubdivision-List with respect to sieving constraints
	/**
	 * This method is called if BoyerMyrvold::planarEmbed was called with the GraphCopySimple h previously.
	 */
	void transform(
		const SList<KuratowskiWrapper>& sourceList,
		SList<KuratowskiSubdivision>& targetList,
		const GraphCopySimple& h,
		const bool onlyDifferent = false)
	{
		transform(sourceList, targetList, h.original(), onlyDifferent);
	}


	//! Returns an embedding, if \p g is planar and Kuratowski Subdivisions otherwise
	/** If \p g is planar, the adjLists of \p g specify a planar embedding.
	 * Use this function, if \p g may be changed.
	 * @param g is the input graph.
	 * @param output contains a number of Kuratowski Subdivisions depending on the other parameters
	 * @param embeddingGrade is a flag bounding the number of extracted subdivisions
	 * @param bundles extracts much more subdivisions, if set
	 * @param limitStructures limits the number of Kuratowski Structures to \p embeddingGrade, if set
	 * @param randomDFSTree randomizes Kuratowski extraction through randomizing the DFSTree, if set
	 * @param avoidE2Minors avoids all \a E2-Minors and ensures unique subdivisions, if set
	 */
	bool planarEmbedDestructive(
		Graph& g,
		SList<KuratowskiWrapper>& output,
		int embeddingGrade,
		bool bundles = false,
		bool limitStructures = false,
		bool randomDFSTree = false,
		bool avoidE2Minors = true);

	//! @copydoc ogdf::BoyerMyrvold::planarEmbedDestructive(Graph&,SList<KuratowskiWrapper>&,int,bool,bool,bool,bool)
	bool planarEmbedDestructive(
		Graph& g,
		SList<KuratowskiWrapper>& output,
		BoyerMyrvoldPlanar::EmbeddingGrade embeddingGrade = BoyerMyrvoldPlanar::EmbeddingGrade::doNotFind,
		bool bundles = false,
		bool limitStructures = false,
		bool randomDFSTree = false,
		bool avoidE2Minors = true)
	{
		return planarEmbedDestructive(g, output, static_cast<int>(embeddingGrade), bundles, limitStructures, randomDFSTree, avoidE2Minors);
	}

	//! Returns an embedding, if \p g is planar and Kuratowski Subdivisions otherwise
	/** If \p g is planar, the adjLists of \p g specify a planar embedding. The function
	 * copies the graph before computation. Use this function, if \p g must not be changed in
	 * the non-planar case.
	 * @param g is the input graph.
	 * @param output contains a number of Kuratowski Subdivisions depending on the other parameters
	 * @param embeddingGrade is a flag bounding the number of extracted subdivisions
	 * @param bundles extracts much more subdivisions, if set
	 * @param limitStructures limits the number of Kuratowski Structures to \p embeddingGrade, if set
	 * @param randomDFSTree randomizes Kuratowski extraction through randomizing the DFSTree, if set
	 * @param avoidE2Minors avoids all \a E2-Minors and ensures unique subdivisions, if set
	 */
	bool planarEmbed(
		Graph& g,
		SList<KuratowskiWrapper>& output,
		int embeddingGrade,
		bool bundles = false,
		bool limitStructures = false,
		bool randomDFSTree = false,
		bool avoidE2Minors = true);

	//! @copydoc ogdf::BoyerMyrvold::planarEmbed(Graph&,SList<KuratowskiWrapper>&,int,bool,bool,bool,bool)
	bool planarEmbed(
		Graph& g,
		SList<KuratowskiWrapper>& output,
		BoyerMyrvoldPlanar::EmbeddingGrade embeddingGrade = BoyerMyrvoldPlanar::EmbeddingGrade::doNotFind,
		bool bundles = false,
		bool limitStructures = false,
		bool randomDFSTree = false,
		bool avoidE2Minors = true)
	{
		return planarEmbed(g, output, static_cast<int>(embeddingGrade), bundles, limitStructures, randomDFSTree, avoidE2Minors);
	}

	//! Returns an embedding, if graph copy \p h is planar and Kuratowski Subdivisions otherwise
	/** If \p h is planar, the adjLists of \p h specify a planar embedding. The function
	 * copies the graph before computation. Use this function, if \p g must not be changed.
	 * @param h is the input graph copy.
	 * @param output contains a number of Kuratowski Subdivisions depending on the other parameters
	 * @param embeddingGrade is a flag bounding the number of extracted subdivisions
	 * @param bundles extracts much more subdivisions, if set
	 * @param limitStructures limits the number of Kuratowski Structures to \p embeddingGrade, if set
	 * @param randomDFSTree randomizes Kuratowski extraction through randomizing the DFSTree, if set
	 * @param avoidE2Minors avoids all \a E2-Minors and ensures unique subdivisions, if set
	 */
	bool planarEmbed(
		GraphCopySimple& h,
		SList<KuratowskiWrapper>& output,
		int embeddingGrade,
		bool bundles = false,
		bool limitStructures = false,
		bool randomDFSTree = false,
		bool avoidE2Minors = true);

	//! @copydoc ogdf::BoyerMyrvold::planarEmbed(GraphCopySimple&,SList<KuratowskiWrapper>&,int,bool,bool,bool,bool)
	bool planarEmbed(
		GraphCopySimple& h,
		SList<KuratowskiWrapper>& output,
		BoyerMyrvoldPlanar::EmbeddingGrade embeddingGrade = BoyerMyrvoldPlanar::EmbeddingGrade::doNotFind,
		bool bundles = false,
		bool limitStructures = false,
		bool randomDFSTree = false,
		bool avoidE2Minors = true)
	{
		return planarEmbed(h, output, static_cast<int>(embeddingGrade), bundles, limitStructures, randomDFSTree, avoidE2Minors);
	}
};

}
