/** \file
 * \brief Implementation of the class UpwardPlanarity.
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

#include <ogdf/upward/UpwardPlanarity.h>

#include <ogdf/upward/internal/UpwardPlanarityEmbeddedDigraph.h>
#include <ogdf/upward/internal/UpwardPlanaritySingleSource.h>
#include <ogdf/upward/internal/UpSAT.h>

namespace ogdf {

//
// General digraphs
//

bool UpwardPlanarity::isUpwardPlanar(Graph &G) {
	UpSAT tester(G);
	return tester.testUpwardPlanarity();
}

bool UpwardPlanarity::embedUpwardPlanar(Graph &G, adjEntry& externalToItsRight) {
	UpSAT embedder(G);
	return embedder.embedUpwardPlanar(externalToItsRight);
}

#if 0
int UpwardPlanarity::maximalFeasibleUpwardPlanarSubgraph(const Graph &G, GraphCopy &GC) {
	MaximalFUPS m(G,0);
	return m.computeMFUPS(GC);
}
#endif


//
// Biconnected digraphs
//

bool UpwardPlanarity::isUpwardPlanar_embedded(const Graph &G)
{
	if (isBiconnected(G) && G.representsCombEmbedding() && isAcyclic(G)) {
		UpwardPlanarityEmbeddedDigraph p(G);
		return p.isUpwardPlanarEmbedded();
	}
	return false;
}


bool UpwardPlanarity::isUpwardPlanar_embedded(const Graph &G, List<adjEntry> &possibleExternalFaces)
{
	if (isBiconnected(G) && G.representsCombEmbedding() && isAcyclic(G)) {
		UpwardPlanarityEmbeddedDigraph p(G);
		return p.isUpwardPlanarEmbedded(possibleExternalFaces);
	}
	return false;
}

//
// Triconnected digraphs
//


bool UpwardPlanarity::isUpwardPlanar_triconnected(const Graph &G)
{
	if (isTriconnected(G) && isAcyclic(G)) {
		Graph H(G);
		BoyerMyrvold p;
		if (!p.planarEmbed(H)) return false;
		return isUpwardPlanar_embedded(H);
	}
	return false;
}


bool UpwardPlanarity::upwardPlanarEmbed_triconnected(Graph &G)
{
	if (isTriconnected(G) && isAcyclic(G)) {
		BoyerMyrvold p;
		if (!p.planarEmbed(G)) return false;
		return isUpwardPlanar_embedded(G);
	}
	return false;
}

//
// Single-source digraphs
//

bool UpwardPlanarity::isUpwardPlanar_singleSource(const Graph &G)
{
	NodeArray<SListPure<adjEntry> > adjacentEdges;
	return UpwardPlanaritySingleSource::testAndFindEmbedding(G, false, adjacentEdges);
}


bool UpwardPlanarity::upwardPlanarEmbed_singleSource(Graph &G)
{
	NodeArray<SListPure<adjEntry> > adjacentEdges(G);
	if(!UpwardPlanaritySingleSource::testAndFindEmbedding(G, true, adjacentEdges))
		return false;

	node superSink;
	SList<edge> augmentedEdges;
	UpwardPlanaritySingleSource::embedAndAugment(G, adjacentEdges, false, superSink, augmentedEdges);

	return true;
}


bool UpwardPlanarity::upwardPlanarAugment_singleSource(Graph &G)
{
	node superSink;
	SList<edge> augmentedEdges;

	return upwardPlanarAugment_singleSource(G, superSink, augmentedEdges);
}


bool UpwardPlanarity::upwardPlanarAugment_singleSource(
	Graph &G,
	node &superSink,
	SList<edge> &augmentedEdges)
{
	NodeArray<SListPure<adjEntry> > adjacentEdges(G);
	if(!UpwardPlanaritySingleSource::testAndFindEmbedding(G, true, adjacentEdges))
		return false;

	UpwardPlanaritySingleSource::embedAndAugment(G, adjacentEdges, true, superSink, augmentedEdges);
	return true;
}


bool UpwardPlanarity::isUpwardPlanar_singleSource_embedded(
	const ConstCombinatorialEmbedding &E,
	SList<face> &externalFaces)
{
	const Graph &G = E;
	OGDF_ASSERT(G.representsCombEmbedding());

	externalFaces.clear();

	// trivial cases
	if(G.empty())
		return true;

	if(!isAcyclic(G))
		return false;

	// determine the single source in G
	node s;
	if(!hasSingleSource(G,s))
		return false;

	// construct face-sink graph anf find possible external faces
	FaceSinkGraph F(E,s);
	F.possibleExternalFaces(externalFaces);

	return !externalFaces.empty();
}


bool UpwardPlanarity::upwardPlanarAugment_singleSource_embedded(
	Graph &G,
	node  &superSink,
	SList<edge> &augmentedEdges)
{
	OGDF_ASSERT(G.representsCombEmbedding());

	// trivial cases
	if(G.empty())
		return true;

	if(!isAcyclic(G))
		return false;

	// determine the single source in G
	node s;
	if(!hasSingleSource(G,s))
		return false;

	// construct embedding represented by G and face-sink graph
	ConstCombinatorialEmbedding E(G);
	FaceSinkGraph F(E,s);

	// find possible external faces
	SList<face> externalFaces;
	F.possibleExternalFaces(externalFaces);

	if (externalFaces.empty())
		return false;

	else {
		F.stAugmentation(F.faceNodeOf(externalFaces.front()), G, superSink, augmentedEdges);
		return true;
	}
}

 }
