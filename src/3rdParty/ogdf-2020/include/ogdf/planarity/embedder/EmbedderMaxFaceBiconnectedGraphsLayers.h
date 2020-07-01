/** \file
 * \brief Computes an embedding of a biconnected graph with maximum
 * external face.
 *
 * \author Thorsten Kerkhof
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

#include <ogdf/planarity/embedder/EmbedderMaxFaceBiconnectedGraphs.h>

namespace ogdf {

//! Embedder that maximizes the external face (plus layers approach).
/**
 * \pre Input graphs need to be biconnected.
 *
 * See the paper "Graph Embedding with Minimum Depth and
 * Maximum External Face" by C. Gutwenger and P. Mutzel (2004) for
 * details.
 * The algorithm for maximum external face is combined with the
 * algorithm for maximum external layers which defines how to embed
 * blocks into inner faces. See diploma thesis "Algorithmen zur
 * Bestimmung von guten Graph-Einbettungen für orthogonale
 * Zeichnungen" (in German) by Thorsten Kerkhof (2007) for details.
 */
template<class T>
class EmbedderMaxFaceBiconnectedGraphsLayers : private EmbedderMaxFaceBiconnectedGraphs<T>
{
public:
	//! @copydoc ogdf::EmbedderMaxFaceBiconnectedGraphs::embed
	static void embed(
		Graph& G,
		adjEntry& adjExternal,
		const NodeArray<T>& nodeLength,
		const EdgeArray<T>& edgeLength,
		const node& n = nullptr);

	using EmbedderMaxFaceBiconnectedGraphs<T>::compute;
	using EmbedderMaxFaceBiconnectedGraphs<T>::computeSize;

private:
	using EmbedderMaxFaceBiconnectedGraphs<T>::largestFaceContainingNode;
	using EmbedderMaxFaceBiconnectedGraphs<T>::largestFaceInSkeleton;

	/* \brief Computes recursively the thickness of the skeleton graph of all
	 *   nodes in the SPQR-tree.
	 *
	 * \param spqrTree The SPQR-tree of the treated graph.
	 * \param mu a node in the SPQR-tree.
	 * \param thickness saving the computed results - the thickness of each
	 *   skeleton graph.
	 * \param nodeLength is saving for each node of the original graph \p G its
	 *   length.
	 * \param edgeLength is saving the edge lengths of all edges in each skeleton
	 *   graph of all tree nodes.
	 */
	static void bottomUpThickness(
		const StaticSPQRTree& spqrTree,
		const node& mu,
		NodeArray<T>& thickness,
		const NodeArray<T>& nodeLength,
		const NodeArray< EdgeArray<T> >& edgeLength);

	/* \brief ExpandEdge embeds all edges in the skeleton graph \a S into an
	 *   existing embedding and calls recursively itself for all virtual edges
	 *   in S.
	 *
	 * \param spqrTree The SPQR-tree of the treated graph.
	 * \param treeNodeTreated is an array saving for each SPQR-tree node \p mu
	 *   whether it was already treated by any call of ExpandEdge or not. Every
	 *   \p mu should only be treated once.
	 * \param mu is a node in the SPQR-tree.
	 * \param leftNode is the node adjacent to referenceEdge, which should be "left"
	 *   in the embedding
	 * \param nodeLength is an array saving the lengths of the nodes of \p G.
	 * \param edgeLength is saving the edge lengths of all edges in each skeleton
	 *   graph of all tree nodes.
	 * \param thickness of each skeleton graph.
	 * \param newOrder is saving for each node \p n in \p G the new adjacency
	 *   list. This is an output parameter.
	 * \param adjBeforeNodeArraySource is saving for the source of the reference edge
	 *   of the skeleton of mu the adjacency entry, before which new entries have
	 *   to be inserted.
	 * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
	 *   of the skeleton of mu the adjacency entry, before which new entries have
	 *   to be inserted.
	 * \param delta_u the distance from the second adjacent face of the reference edge
	 *   except the external face to the external face of G.
	 * \param delta_d the distance from the external face to the external face of G.
	 * \param adjExternal is an adjacency entry in the external face.
	 * \param n is only set, if ExpandEdge is called for the first time, because
	 *   then there is no virtual edge which has to be expanded, but the max face
	 *   has to contain a certain node \p n.
	 */
	static void expandEdge(
		const StaticSPQRTree& spqrTree,
		NodeArray<bool>& treeNodeTreated,
		const node& mu,
		const node& leftNode,
		const NodeArray<T>& nodeLength,
		const NodeArray< EdgeArray<T> >& edgeLength,
		const NodeArray<T>& thickness,
		NodeArray< List<adjEntry> >& newOrder,
		NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArraySource,
		NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArrayTarget,
		const T& delta_u,
		const T& delta_d,
		adjEntry& adjExternal,
		const node& n = nullptr);

	/* \brief Embeds all edges in the skeleton graph \a S of an S-node of the
	 *   SPQR-tree into an existing embedding and calls recursively itself for
	 *   all virtual edges in S.
	 *
	 * \param spqrTree The SPQR-tree of the treated graph.
	 * \param treeNodeTreated is an array saving for each SPQR-tree node \p mu
	 *   whether it was already treated by any call of ExpandEdge or not. Every
	 *   \p mu should only be treated once.
	 * \param mu is a node in the SPQR-tree.
	 * \param leftNode is the node adjacent to referenceEdge, which should be "left"
	 *   in the embedding
	 * \param nodeLength is an array saving the lengths of the nodes of \p G.
	 * \param edgeLength is saving the edge lengths of all edges in each skeleton
	 *   graph of all tree nodes.
	 * \param thickness of each skeleton graph.
	 * \param newOrder is saving for each node \p n in \p G the new adjacency
	 *   list. This is an output parameter.
	 * \param adjBeforeNodeArraySource is saving for the source of the reference edge
	 *   of the skeleton of mu the adjacency entry, before which new entries have
	 *   to be inserted.
	 * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
	 *   of the skeleton of mu the adjacency entry, before which new entries have
	 *   to be inserted.
	 * \param delta_u the distance from the second adjacent face of the reference edge
	 *   except the external face to the external face of G.
	 * \param delta_d the distance from the external face to the external face of G.
	 * \param adjExternal is an adjacency entry in the external face.
	 */
	static void expandEdgeSNode(
		const StaticSPQRTree& spqrTree,
		NodeArray<bool>& treeNodeTreated,
		const node& mu,
		const node& leftNode,
		const NodeArray<T>& nodeLength,
		const NodeArray< EdgeArray<T> >& edgeLength,
		const NodeArray<T>& thickness,
		NodeArray< List<adjEntry> >& newOrder,
		NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArraySource,
		NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArrayTarget,
		const T& delta_u,
		const T& delta_d,
		adjEntry& adjExternal);

	/* \brief Embeds all edges in the skeleton graph \a S of an P-node of the
	 *   SPQR-tree into an existing embedding and calls recursively itself for
	 *   all virtual edges in S.
	 *
	 * \param spqrTree The SPQR-tree of the treated graph.
	 * \param treeNodeTreated is an array saving for each SPQR-tree node \p mu
	 *   whether it was already treated by any call of ExpandEdge or not. Every
	 *   \p mu should only be treated once.
	 * \param mu is a node in the SPQR-tree.
	 * \param leftNode is the node adjacent to referenceEdge, which should be "left"
	 *   in the embedding
	 * \param nodeLength is an array saving the lengths of the nodes of \p G.
	 * \param edgeLength is saving the edge lengths of all edges in each skeleton
	 *   graph of all tree nodes.
	 * \param thickness of each skeleton graph.
	 * \param newOrder is saving for each node \p n in \p G the new adjacency
	 *   list. This is an output parameter.
	 * \param adjBeforeNodeArraySource is saving for the source of the reference edge
	 *   of the skeleton of mu the adjacency entry, before which new entries have
	 *   to be inserted.
	 * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
	 *   of the skeleton of mu the adjacency entry, before which new entries have
	 *   to be inserted.
	 * \param delta_u the distance from the second adjacent face of the reference edge
	 *   except the external face to the external face of G.
	 * \param delta_d the distance from the external face to the external face of G.
	 * \param adjExternal is an adjacency entry in the external face.
	 */
	static void expandEdgePNode(
		const StaticSPQRTree& spqrTree,
		NodeArray<bool>& treeNodeTreated,
		const node& mu,
		const node& leftNode,
		const NodeArray<T>& nodeLength,
		const NodeArray< EdgeArray<T> >& edgeLength,
		const NodeArray<T>& thickness,
		NodeArray< List<adjEntry> >& newOrder,
		NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArraySource,
		NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArrayTarget,
		const T& delta_u,
		const T& delta_d,
		adjEntry& adjExternal);

	/* \brief Embeds all edges in the skeleton graph \a S of an R-node of the
	 *   SPQR-tree into an existing embedding and calls recursively itself for
	 *   all virtual edges in S.
	 *
	 * \param spqrTree The SPQR-tree of the treated graph.
	 * \param treeNodeTreated is an array saving for each SPQR-tree node \p mu
	 *   whether it was already treated by any call of ExpandEdge or not. Every
	 *   \p mu should only be treated once.
	 * \param mu is a node in the SPQR-tree.
	 * \param leftNode is the node adjacent to referenceEdge, which should be "left"
	 *   in the embedding
	 * \param nodeLength is an array saving the lengths of the nodes of \p G.
	 * \param edgeLength is saving the edge lengths of all edges in each skeleton
	 *   graph of all tree nodes.
	 * \param thickness of each skeleton graph.
	 * \param newOrder is saving for each node \p n in \p G the new adjacency
	 *   list. This is an output parameter.
	 * \param adjBeforeNodeArraySource is saving for the source of the reference edge
	 *   of the skeleton of mu the adjacency entry, before which new entries have
	 *   to be inserted.
	 * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
	 *   of the skeleton of mu the adjacency entry, before which new entries have
	 *   to be inserted.
	 * \param delta_u the distance from the second adjacent face of the reference edge
	 *   except the external face to the external face of G.
	 * \param delta_d the distance from the external face to the external face of G.
	 * \param adjExternal is an adjacency entry in the external face.
	 * \param n is only set, if ExpandEdge is called for the first time, because
	 *   then there is no virtual edge which has to be expanded, but the max face
	 *   has to contain a certain node \p n.
	 */
	static void expandEdgeRNode(
		const StaticSPQRTree& spqrTree,
		NodeArray<bool>& treeNodeTreated,
		const node& mu,
		const node& leftNode,
		const NodeArray<T>& nodeLength,
		const NodeArray< EdgeArray<T> >& edgeLength,
		const NodeArray<T>& thickness,
		NodeArray< List<adjEntry> >& newOrder,
		NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArraySource,
		NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArrayTarget,
		const T& delta_u,
		const T& delta_d,
		adjEntry& adjExternal,
		const node& n = nullptr);

	/* \brief Writes a given adjacency entry into the newOrder. If the edge
	 *   belonging to ae is a virtual edge, it is expanded.
	 *
	 * \param ae is the adjacency entry which has to be expanded.
	 * \param before is the adjacency entry of the node in \p G, before
	 *   which ae has to be inserted.
	 * \param spqrTree The SPQR-tree of the treated graph.
	 * \param treeNodeTreated is an array saving for each SPQR-tree node \p mu
	 *   whether it was already treated by any call of ExpandEdge or not. Every
	 *   \p mu should only be treated once.
	 * \param mu is a node in the SPQR-tree.
	 * \param leftNode is the node adjacent to referenceEdge, which should be "left"
	 *   in the embedding
	 * \param nodeLength is an array saving the lengths of the nodes of \p G.
	 * \param edgeLength is saving the edge lengths of all edges in each skeleton
	 *   graph of all tree nodes.
	 * \param thickness of each skeleton graph.
	 * \param newOrder is saving for each node \p n in \p G the new adjacency
	 *   list. This is an output parameter.
	 * \param adjBeforeNodeArraySource is saving for the source of the reference edge
	 *   of the skeleton of mu the adjacency entry, before which new entries have
	 *   to be inserted.
	 * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
	 *   of the skeleton of mu the adjacency entry, before which new entries have
	 *   to be inserted.
	 * \param delta_u the distance from the second adjacent face of the reference edge
	 *   except the external face to the external face of G.
	 * \param delta_d the distance from the external face to the external face of G.
	 * \param adjExternal is an adjacency entry in the external face.
	 */
	static void adjEntryForNode(
		adjEntry& ae,
		ListIterator<adjEntry>& before,
		const StaticSPQRTree& spqrTree,
		NodeArray<bool>& treeNodeTreated,
		const node& mu,
		const node& leftNode,
		const NodeArray<T>& nodeLength,
		const NodeArray< EdgeArray<T> >& edgeLength,
		const NodeArray<T>& thickness,
		NodeArray< List<adjEntry> >& newOrder,
		NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArraySource,
		NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArrayTarget,
		const T& delta_u,
		const T& delta_d,
		adjEntry& adjExternal);

	/* \brief Single source shortest path.
	 *
	 * \param G directed graph
	 * \param s source node
	 * \param length length of an edge
	 * \param d contains shortest path distances after call
	 */
	static bool sssp(
		const Graph& G,
		const node& s,
		const EdgeArray<T>& length,
		NodeArray<T>& d);
};


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::embed(
	Graph& G,
	adjEntry& adjExternal,
	const NodeArray<T>& nodeLength,
	const EdgeArray<T>& edgeLength,
	const node& n /* = 0*/)
{
	//Base cases (SPQR-Tree-implementatioin would crash with these inputs):
	OGDF_ASSERT(G.numberOfNodes() >= 2);
	if (G.numberOfEdges() <= 2)
	{
		edge e = G.firstEdge();
		adjExternal = e->adjSource();
		return;
	}

	//First step: calculate maximum face and edge lengths for virtual edges
	StaticSPQRTree spqrTree(G);
	NodeArray< EdgeArray<T> > edgeLengthSkel;
	compute(G, nodeLength, edgeLength, &spqrTree, edgeLengthSkel);

	//Second step: Embed G
	T biggestFace = -1;
	node bigFaceMu;
	if (n == nullptr)
	{
		for(node mu : spqrTree.tree().nodes)
		{
			//Expand all faces in skeleton(mu) and get size of the largest of them:
			T sizeMu = largestFaceInSkeleton(spqrTree, mu, nodeLength, edgeLengthSkel);
			if (sizeMu > biggestFace)
			{
				biggestFace = sizeMu;
				bigFaceMu = mu;
			}
		}
	}
	else
	{
		node* mus = new node[n->degree()];
		int i = 0;
		for(adjEntry adj : n->adjEntries) {
			mus[i] = spqrTree.skeletonOfReal(adj->theEdge()).treeNode();
			bool alreadySeenMu = false;
			for (int j = 0; j < i && !alreadySeenMu; j++)
			{
				if (mus[i] == mus[j])
					alreadySeenMu = true;
			}
			if (alreadySeenMu)
			{
				i++;
				continue;
			}
			else
			{
				//Expand all faces in skeleton(mu) containing n and get size of
				//the largest of them:
				T sizeInMu = largestFaceContainingNode(spqrTree, mus[i], n, nodeLength, edgeLengthSkel);
				if (sizeInMu > biggestFace)
				{
					biggestFace = sizeInMu;
					bigFaceMu = mus[i];
				}

				i++;
			}
		}
		delete[] mus;
	}

	bigFaceMu = spqrTree.rootTreeAt(bigFaceMu);

	NodeArray<T> thickness(spqrTree.tree());
	bottomUpThickness(spqrTree, bigFaceMu, thickness, nodeLength, edgeLengthSkel);

	NodeArray< List<adjEntry> > newOrder(G);
	NodeArray<bool> treeNodeTreated(spqrTree.tree(), false);
	ListIterator<adjEntry> it;
	adjExternal = nullptr;
	NodeArray< ListIterator<adjEntry> > adjBeforeNodeArraySource(spqrTree.tree());
	NodeArray< ListIterator<adjEntry> > adjBeforeNodeArrayTarget(spqrTree.tree());
	expandEdge(spqrTree, treeNodeTreated, bigFaceMu, nullptr, nodeLength,
		edgeLengthSkel, thickness, newOrder, adjBeforeNodeArraySource,
		adjBeforeNodeArrayTarget, 0, 0, adjExternal, n);

	for(node v : G.nodes)
		G.sort(v, newOrder[v]);
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::adjEntryForNode(
	adjEntry& ae,
	ListIterator<adjEntry>& before,
	const StaticSPQRTree& spqrTree,
	NodeArray<bool>& treeNodeTreated,
	const node& mu,
	const node& leftNode,
	const NodeArray<T>& nodeLength,
	const NodeArray< EdgeArray<T> >& edgeLength,
	const NodeArray<T>& thickness,
	NodeArray< List<adjEntry> >& newOrder,
	NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArraySource,
	NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArrayTarget,
	const T& delta_u,
	const T& delta_d,
	adjEntry& adjExternal)
{
	Skeleton& S = spqrTree.skeleton(mu);
	edge referenceEdge = S.referenceEdge();
	if (S.isVirtual(ae->theEdge()))
	{
		edge twinE = S.twinEdge(ae->theEdge());
		node twinNT = S.twinTreeNode(ae->theEdge());
#if 0
		Skeleton& twinS = spqrTree.skeleton(twinNT);
#endif

		if (!treeNodeTreated[twinNT])
		{
			node m_leftNode;
			if (ae->theEdge()->source() == leftNode)
				m_leftNode = twinE->source();
			else
				m_leftNode = twinE->target();

			if (ae->theEdge()->source() == ae->theNode())
				adjBeforeNodeArraySource[twinNT] = before;
			else
				adjBeforeNodeArrayTarget[twinNT] = before;

			//recursion call:
			expandEdge(spqrTree, treeNodeTreated, twinNT, m_leftNode,
				nodeLength, edgeLength, thickness, newOrder,
				adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
				delta_u, delta_d, adjExternal);
		}

		if (ae->theEdge() == referenceEdge)
		{
			if (ae->theNode() == ae->theEdge()->source())
			{
				ListIterator<adjEntry> tmpBefore = adjBeforeNodeArraySource[mu];
				adjBeforeNodeArraySource[mu] = before;
				before = tmpBefore;
			}
			else
			{
				ListIterator<adjEntry> tmpBefore = adjBeforeNodeArrayTarget[mu];
				adjBeforeNodeArrayTarget[mu] = before;
				before = tmpBefore;
			}
		}
		else //!(ae->theEdge() == referenceEdge)
		{
			if (ae->theNode() == ae->theEdge()->source())
				before = adjBeforeNodeArraySource[twinNT];
			else
				before = adjBeforeNodeArrayTarget[twinNT];
		}
	}
	else //!(S.isVirtual(ae->theEdge()))
	{
		node origNode = S.original(ae->theNode());
		edge origEdge = S.realEdge(ae->theEdge());

		if (origNode == origEdge->source())
		{
			if (!before.valid())
				before = newOrder[origNode].pushBack(origEdge->adjSource());
			else
				before = newOrder[origNode].insertBefore(origEdge->adjSource(), before);
		}
		else
		{
			if (!before.valid())
				before = newOrder[origNode].pushBack(origEdge->adjTarget());
			else
				before = newOrder[origNode].insertBefore(origEdge->adjTarget(), before);
		}
	}
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::expandEdge(
	const StaticSPQRTree& spqrTree,
	NodeArray<bool>& treeNodeTreated,
	const node& mu,
	const node& leftNode,
	const NodeArray<T>& nodeLength,
	const NodeArray< EdgeArray<T> >& edgeLength,
	const NodeArray<T>& thickness,
	NodeArray< List<adjEntry> >& newOrder,
	NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArraySource,
	NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArrayTarget,
	const T& delta_u,
	const T& delta_d,
	adjEntry& adjExternal,
	const node& n /*= 0*/)
{
	treeNodeTreated[mu] = true;

	switch(spqrTree.typeOf(mu))
	{
	case SPQRTree::NodeType::SNode:
		expandEdgeSNode(spqrTree, treeNodeTreated, mu, leftNode,
			nodeLength, edgeLength, thickness, newOrder, adjBeforeNodeArraySource,
			adjBeforeNodeArrayTarget, delta_u, delta_d, adjExternal);
		break;
	case SPQRTree::NodeType::PNode:
		expandEdgePNode(spqrTree, treeNodeTreated, mu, leftNode,
			nodeLength, edgeLength, thickness, newOrder, adjBeforeNodeArraySource,
			adjBeforeNodeArrayTarget, delta_u, delta_d, adjExternal);
		break;
	case SPQRTree::NodeType::RNode:
		expandEdgeRNode(spqrTree, treeNodeTreated, mu, leftNode,
			nodeLength, edgeLength, thickness, newOrder, adjBeforeNodeArraySource,
			adjBeforeNodeArrayTarget, delta_u, delta_d, adjExternal, n);
		break;
	default:
		OGDF_ASSERT(false);
	}
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::expandEdgeSNode(
	const StaticSPQRTree& spqrTree,
	NodeArray<bool>& treeNodeTreated,
	const node& mu,
	const node& leftNode,
	const NodeArray<T>& nodeLength,
	const NodeArray< EdgeArray<T> >& edgeLength,
	const NodeArray<T>& thickness,
	NodeArray< List<adjEntry> >& newOrder,
	NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArraySource,
	NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArrayTarget,
	const T& delta_u,
	const T& delta_d,
	adjEntry& adjExternal)
{
	Skeleton& S = spqrTree.skeleton(mu);
	edge referenceEdge = S.referenceEdge();
	adjEntry startAdjEntry = nullptr;
	if (leftNode == nullptr)
	{
		for(edge e : S.getGraph().edges)
		{
			if (!S.isVirtual(e))
			{
				startAdjEntry = e->adjSource();
				break;
			}
		}
		OGDF_ASSERT(startAdjEntry != nullptr);
	}
	else if (leftNode->firstAdj()->theEdge() == referenceEdge)
		startAdjEntry = leftNode->lastAdj();
	else
		startAdjEntry = leftNode->firstAdj();

	adjEntry ae = startAdjEntry;
	if (adjExternal == nullptr)
	{
		edge orgEdge = S.realEdge(ae->theEdge());
		if (orgEdge->source() == S.original(ae->theNode()))
			adjExternal = orgEdge->adjSource()->twin();
		else
			adjExternal = orgEdge->adjTarget()->twin();
	}

	ListIterator<adjEntry> before;
	if (referenceEdge && leftNode == referenceEdge->source())
		before = adjBeforeNodeArraySource[mu];
	else if (referenceEdge)
		before = adjBeforeNodeArrayTarget[mu];
	ListIterator<adjEntry> beforeSource;

	bool firstStep = true;
	while (firstStep || ae != startAdjEntry)
	{
		//first treat ae with ae->theNode() is left node, then treat its twin:
		node m_leftNode = ae->theNode();

		if (ae->theEdge() == referenceEdge)
		{
			if (ae->theNode() == referenceEdge->source())
				adjBeforeNodeArraySource[mu] = before;
			else
				adjBeforeNodeArrayTarget[mu] = before;
		}
		else
		{
			if (S.isVirtual(ae->theEdge()) && referenceEdge)
			{
				node twinTN = S.twinTreeNode(ae->theEdge());
				if (ae->theEdge()->source() == ae->theNode())
				{
					if (ae->theEdge()->target() == referenceEdge->source())
						adjBeforeNodeArrayTarget[twinTN] = adjBeforeNodeArraySource[mu];
					else if (ae->theEdge()->target() == referenceEdge->target())
						adjBeforeNodeArrayTarget[twinTN] = adjBeforeNodeArrayTarget[mu];
				}
				else
				{
					if (ae->theEdge()->source() == referenceEdge->source())
						adjBeforeNodeArraySource[twinTN] = adjBeforeNodeArraySource[mu];
					else if (ae->theEdge()->source() == referenceEdge->target())
						adjBeforeNodeArraySource[twinTN] = adjBeforeNodeArrayTarget[mu];
				}
			}

			adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
				m_leftNode, nodeLength, edgeLength, thickness, newOrder,
				adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
				delta_u, delta_d, adjExternal);
		}

		if (firstStep) {
			beforeSource = before;
			firstStep = false;
		}

		ae = ae->twin();
		if (!referenceEdge)
			before = nullptr;
		else if (ae->theNode() == referenceEdge->source())
			before = adjBeforeNodeArraySource[mu];
		else if (ae->theNode() == referenceEdge->target())
			before = adjBeforeNodeArrayTarget[mu];
		else
			before = nullptr;
		if (ae->theEdge() == referenceEdge)
		{
			if (ae->theNode() == referenceEdge->source())
				adjBeforeNodeArraySource[mu] = beforeSource;
			else
				adjBeforeNodeArrayTarget[mu] = beforeSource;
		}
		else
			adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
				m_leftNode, nodeLength, edgeLength, thickness, newOrder,
				adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
				delta_u, delta_d, adjExternal);

		//set new adjacency entry pair (ae and its twin):
		if (ae->theNode()->firstAdj() == ae)
			ae = ae->theNode()->lastAdj();
		else
			ae = ae->theNode()->firstAdj();
	}
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::expandEdgePNode(
	const StaticSPQRTree& spqrTree,
	NodeArray<bool>& treeNodeTreated,
	const node& mu,
	const node& leftNode,
	const NodeArray<T>& nodeLength,
	const NodeArray< EdgeArray<T> >& edgeLength,
	const NodeArray<T>& thickness,
	NodeArray< List<adjEntry> >& newOrder,
	NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArraySource,
	NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArrayTarget,
	const T& delta_u,
	const T& delta_d,
	adjEntry& adjExternal)
{
	//Choose face defined by virtual edge and the longest edge different from it.
	Skeleton& S = spqrTree.skeleton(mu);
	edge referenceEdge = S.referenceEdge();
	edge altReferenceEdge = nullptr;

	node m_leftNode = leftNode;
	if (!m_leftNode) {
		List<node> nodeList;
		S.getGraph().allNodes(nodeList);
		m_leftNode = *(nodeList.begin());
	}
	node m_rightNode = m_leftNode->firstAdj()->twinNode();

	if (referenceEdge == nullptr)
	{
		for(edge e : S.getGraph().edges)
		{
			if (!S.isVirtual(e))
			{
				altReferenceEdge = e;
				edge orgEdge = S.realEdge(e);
				if (orgEdge->source() == S.original(m_leftNode))
					adjExternal = orgEdge->adjSource();
				else
					adjExternal = orgEdge->adjTarget();
				break;
			}
		}
	}

	//sort edges by length:
	List<edge> graphEdges;
	for(edge e : S.getGraph().edges)
	{
		if (e == referenceEdge || e == altReferenceEdge)
			continue;

		if (!graphEdges.begin().valid())
			graphEdges.pushBack(e);
		else
		{
			for (ListIterator<edge> it = graphEdges.begin(); it.valid(); ++it)
			{
				if (edgeLength[mu][e] > edgeLength[mu][*it])
				{
					graphEdges.insertBefore(e, it);
					break;
				}
				ListIterator<edge> next = it;
				++next;
				if (!next.valid())
				{
					graphEdges.pushBack(e);
					break;
				}
			}
		}
	}

	List<edge> rightEdgeOrder;
	ListIterator<adjEntry> beforeAltRefEdge;
	ListIterator<adjEntry> beforeLeft;

	//begin with left node and longest edge:
	for (int i = 0; i < 2; ++i) {
		ListIterator<adjEntry> before;
		node n;
		if (i == 0)
			n = m_leftNode;
		else {
			n = m_rightNode;
			before = beforeAltRefEdge;
		}

		if (referenceEdge) {
			if (n == referenceEdge->source())
				before = adjBeforeNodeArraySource[mu];
			else
				before = adjBeforeNodeArrayTarget[mu];
		}

		adjEntry ae;

		//all edges except reference edge:
		if (i == 0) {
			ListIterator<edge> lastPos;
			ListIterator<adjEntry> beforeRight;
			if (referenceEdge)  {
				if (referenceEdge->source() == m_rightNode)
					beforeRight = adjBeforeNodeArraySource[mu];
				else //referenceEdge->target() == rightnode
					beforeRight = adjBeforeNodeArrayTarget[mu];
			}
			bool insertBeforeLast = false;
			bool oneEdgeInE_a = false;
			T sum_E_a = 0;
			T sum_E_b = 0;

			for (int looper = 0; looper < graphEdges.size(); looper++)
			{
				edge e = *(graphEdges.get(looper));

				if (!lastPos.valid())
					lastPos = rightEdgeOrder.pushBack(e);
				else if (insertBeforeLast)
					lastPos = rightEdgeOrder.insertBefore(e, lastPos);
				else
					lastPos = rightEdgeOrder.insertAfter(e, lastPos);

				//decide whether to choose face f_a or f_b as f_{mu, mu'}:
				if (delta_u + sum_E_a < delta_d + sum_E_b)
				{
					ListIterator<adjEntry> beforeU = before;

					if (e->source() == n)
						ae = e->adjSource();
					else
						ae = e->adjTarget();

					if (S.isVirtual(e))
					{
						node nu = S.twinTreeNode(e);

						T delta_u_nu = delta_u + sum_E_a;
						T delta_d_nu = delta_d + sum_E_b;

						//buffer computed embedding:
						NodeArray< List<adjEntry> > tmp_newOrder(spqrTree.originalGraph());
						ListIterator<adjEntry> tmp_before;

						adjEntryForNode(ae, tmp_before, spqrTree, treeNodeTreated, mu,
							m_leftNode, nodeLength, edgeLength, thickness,
							tmp_newOrder, adjBeforeNodeArraySource,
							adjBeforeNodeArrayTarget, delta_d_nu, delta_u_nu, adjExternal);

						//copy buffered embedding reversed into newOrder:
						node leftOrg = S.original(m_leftNode);
						node rightOrg = S.original(m_rightNode);
						for(node nOG : spqrTree.originalGraph().nodes)
						{
							List<adjEntry> nOG_tmp_adjList = tmp_newOrder[nOG];
							if (nOG_tmp_adjList.size() == 0)
								continue;

							ListIterator<adjEntry>* m_before;
							if (nOG == leftOrg)
								m_before = &beforeU;
							else if (nOG == rightOrg && referenceEdge)
								m_before = &beforeRight;
							else
								m_before = new ListIterator<adjEntry>();

							for (ListIterator<adjEntry> ae_it = nOG_tmp_adjList.begin(); ae_it.valid(); ++ae_it) {
								adjEntry adjaEnt = *ae_it;
								if (!m_before->valid())
									*m_before = newOrder[nOG].pushBack(adjaEnt);
								else
									*m_before = newOrder[nOG].insertBefore(adjaEnt, *m_before);

								if (nOG == leftOrg || nOG == rightOrg)
								{
									if (S.original(e->source()) == nOG)
										adjBeforeNodeArraySource[nu] = *m_before;
									else
										adjBeforeNodeArrayTarget[nu] = *m_before;
								}
							}

							if (nOG != leftOrg && (nOG != rightOrg || !referenceEdge))
								delete m_before;
						}

						sum_E_a += thickness[nu];
					}
					else //!(S.isVirtual(e))
					{
						adjEntryForNode(ae, beforeU, spqrTree, treeNodeTreated, mu,
							m_leftNode, nodeLength, edgeLength, thickness,
							newOrder, adjBeforeNodeArraySource,
							adjBeforeNodeArrayTarget, 0, 0, adjExternal);

						sum_E_a += 1;
					}

					insertBeforeLast = false;
					if (!oneEdgeInE_a)
					{
						beforeLeft = beforeU;
						oneEdgeInE_a = true;
					}
				}
				else //!(delta_u + sum_E_a <= delta_d + sum_E_b)
				{
					if (S.isVirtual(e)) {
						node nu = S.twinTreeNode(e);
						if (referenceEdge) {
							if (e->source() == n)
								adjBeforeNodeArrayTarget[nu] = beforeRight;
							else
								adjBeforeNodeArraySource[nu] = beforeRight;
						}
					}

					T delta_u_nu = delta_u + sum_E_a;
					T delta_d_nu = delta_d + sum_E_b;

					if (e->source() == n)
						ae = e->adjSource();
					else
						ae = e->adjTarget();

					adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
						m_leftNode, nodeLength, edgeLength, thickness, newOrder,
						adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
						delta_u_nu, delta_d_nu, adjExternal);

					if (S.isVirtual(e))
						sum_E_b += thickness[S.twinTreeNode(e)];
					else
						sum_E_b += 1;

					insertBeforeLast = true;
					if (!oneEdgeInE_a)
						beforeLeft = before;
				}
			}
		}
		else
		{
			for (ListIterator<edge> it = rightEdgeOrder.begin(); it.valid(); ++it)
			{
				if ((*it)->source() == n)
					ae = (*it)->adjSource();
				else
					ae = (*it)->adjTarget();

				adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
					m_leftNode, nodeLength, edgeLength, thickness, newOrder,
					adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
					0, 0, adjExternal);
			}
		}

		//(alternative) reference edge at last:
		if (referenceEdge) {
			if (i == 0) {
				if (n == referenceEdge->source())
					adjBeforeNodeArraySource[mu] = beforeLeft;
				else
					adjBeforeNodeArrayTarget[mu] = beforeLeft;
			}
			else
			{
				if (n == referenceEdge->source())
					adjBeforeNodeArraySource[mu] = before;
				else
					adjBeforeNodeArrayTarget[mu] = before;
			}
		}
		else
		{
			if (altReferenceEdge->source() == n)
				ae = altReferenceEdge->adjSource();
			else
				ae = altReferenceEdge->adjTarget();

			adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
				m_leftNode, nodeLength, edgeLength, thickness, newOrder,
				adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
				0, 0, adjExternal);
		}
	}
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::expandEdgeRNode(
	const StaticSPQRTree& spqrTree,
	NodeArray<bool>& treeNodeTreated,
	const node& mu,
	const node& leftNode,
	const NodeArray<T>& nodeLength,
	const NodeArray< EdgeArray<T> >& edgeLength,
	const NodeArray<T>& thickness,
	NodeArray< List<adjEntry> >& newOrder,
	NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArraySource,
	NodeArray< ListIterator<adjEntry> >& adjBeforeNodeArrayTarget,
	const T& delta_u,
	const T& delta_d,
	adjEntry& adjExternal,
	const node& n /* = 0 */)
{
	Skeleton& S = spqrTree.skeleton(mu);
	edge referenceEdge = S.referenceEdge();

	//compute biggest face containing the reference edge:
	face maxFaceContEdge = nullptr;
	List<node> maxFaceNodes;
	planarEmbed(S.getGraph());
	CombinatorialEmbedding combinatorialEmbedding(S.getGraph());
	T bigFaceSize = -1;
	adjEntry m_adjExternal = nullptr;
	for(face f : combinatorialEmbedding.faces)
	{
		bool containsVirtualEdgeOrN = false;
		adjEntry this_m_adjExternal = nullptr;
		T sizeOfFace = 0;
		List<node> faceNodes;
		for(adjEntry ae : f->entries)
		{
			faceNodes.pushBack(ae->theNode());
			if ((n == nullptr && (ae->theEdge() == referenceEdge || !referenceEdge))
			 || S.original(ae->theNode()) == n) {
				containsVirtualEdgeOrN = true;
				if (referenceEdge)
					this_m_adjExternal = ae;
			}

			if (!referenceEdge && !S.isVirtual(ae->theEdge()))
				this_m_adjExternal = ae;

			sizeOfFace += edgeLength[mu][ae->theEdge()]
								 +  nodeLength[S.original(ae->theNode())];
		}

		if (containsVirtualEdgeOrN && this_m_adjExternal && sizeOfFace > bigFaceSize) {
			maxFaceNodes = faceNodes;
			bigFaceSize = sizeOfFace;
			maxFaceContEdge = f;
			m_adjExternal = this_m_adjExternal;
		}
	}
	OGDF_ASSERT(maxFaceContEdge);

	if (!adjExternal) {
		edge orgEdge = S.realEdge(m_adjExternal->theEdge());
		if (orgEdge->source() == S.original(m_adjExternal->theNode()))
			adjExternal = orgEdge->adjSource();
		else
			adjExternal = orgEdge->adjTarget();
	}

	adjEntry adjMaxFace = m_adjExternal;

	//if embedding is mirror symmetrical embedding of desired embedding,
	//invert adjacency list of all nodes:
	if (referenceEdge) {
		//successor of adjEntry of virtual edge in adjacency list of leftNode:
		adjEntry succ_virtualEdge_leftNode;
		if (leftNode == referenceEdge->source())
			succ_virtualEdge_leftNode = referenceEdge->adjSource()->succ();
		else
			succ_virtualEdge_leftNode = referenceEdge->adjTarget()->succ();
		if (!succ_virtualEdge_leftNode)
			succ_virtualEdge_leftNode = leftNode->firstAdj();

		bool succVELNAEInExtFace = false;
		for(adjEntry aeExtFace : maxFaceContEdge->entries)
		{
			if (aeExtFace->theEdge() == succ_virtualEdge_leftNode->theEdge())
			{
				succVELNAEInExtFace = true;
				break;
			}
		}

		if (!succVELNAEInExtFace)
		{
			for(node v : S.getGraph().nodes)
			{
				List<adjEntry> newAdjOrder;
				for (adjEntry ae = v->firstAdj(); ae; ae = ae->succ())
					newAdjOrder.pushFront(ae);
				S.getGraph().sort(v, newAdjOrder);
			}
			adjMaxFace = adjMaxFace->twin();
		}
	}

	NodeArray<bool> nodeTreated(S.getGraph(), false);
	adjEntry start_ae;
	if (referenceEdge) {
		start_ae = adjMaxFace;
		do
		{
			if (start_ae->theEdge() == referenceEdge) {
				start_ae = start_ae->faceCycleSucc();
				break;
			}
			start_ae = start_ae->faceCycleSucc();
		} while(start_ae != adjMaxFace);
	}
	else
		start_ae = adjMaxFace;

	//For every edge a buffer saving adjacency entries written in embedding step
	//for nodes on the maximum face, needed in step for other nodes.
	EdgeArray< List<adjEntry> > buffer(S.getGraph());

	bool firstStep = true;
	bool after_start_ae = true;
	for (adjEntry ae = start_ae;
		firstStep || ae != start_ae;
		after_start_ae = after_start_ae && ae->succ(),
			ae = after_start_ae ? ae->faceCycleSucc()
			: (!ae->faceCycleSucc() ? adjMaxFace : ae->faceCycleSucc()))
	{
		firstStep = false;
#if 0
		node nodeG = S.original(ae->theNode());
#endif
		nodeTreated[ae->theNode()] = true;

		//copy adjacency list of nodes into newOrder:
		ListIterator<adjEntry> before;
		edge vE = (ae->theEdge() == referenceEdge) ? referenceEdge : ae->theEdge();
		node nu = (ae->theEdge() == referenceEdge) ? mu : S.twinTreeNode(ae->theEdge());
		if (S.isVirtual(vE)) {
			if (ae->theNode() == vE->source())
				before = adjBeforeNodeArraySource[nu];
			else
				before = adjBeforeNodeArrayTarget[nu];
		}

		bool after_ae = true;
		adjEntry m_start_ae;
		if (referenceEdge) {
			if (ae->theNode() == referenceEdge->source())
				m_start_ae = referenceEdge->adjSource();
			else if (ae->theNode() == referenceEdge->target())
				m_start_ae = referenceEdge->adjTarget();
			else
				m_start_ae = ae;
		}
		else
			m_start_ae = ae;

		adjEntry m_stop_ae;
		bool hit_stop_twice = false;
		int numOfHits = 0;
		if (referenceEdge
		 && (ae->theNode() == referenceEdge->source()
		  || ae->theNode() == referenceEdge->target())) {
			if (m_start_ae->succ())
				m_stop_ae = m_start_ae->succ();
			else {
				m_stop_ae = m_start_ae->theNode()->firstAdj();
				hit_stop_twice = true;
			}
		}
		else
			m_stop_ae = m_start_ae;

		for (adjEntry aeN = m_start_ae;
		     after_ae || (hit_stop_twice && numOfHits != 2) || aeN != m_stop_ae;
		     after_ae = after_ae && aeN->succ(),
		     aeN = after_ae ? aeN->succ()
		                    : (!aeN->succ() ? ae->theNode()->firstAdj() : aeN->succ()),
		     numOfHits = (aeN == m_stop_ae) ? numOfHits + 1 : numOfHits) {
			node m_leftNode = nullptr;
			if (S.isVirtual(aeN->theEdge()) && aeN->theEdge() != referenceEdge)
			{
				//Compute left node of aeN->theNode(). First get adjacency entry in ext. face
				//(if edge is in ext. face) and compare face cycle successor with successor
				//in node adjacency list. If it is the same, it is the right node, otherwise
				//the left.
				adjEntry aeExtFace = nullptr;
				bool succInExtFace = false;
				bool aeNInExtFace = false;
				adjEntry aeNSucc = (aeN->succ()) ? aeN->succ() : ae->theNode()->firstAdj();
				aeExtFace = adjMaxFace;
				do
				{
					if (aeExtFace->theEdge() == aeNSucc->theEdge())
					{
						succInExtFace = true;
						if (aeNInExtFace)
							break;
					}
					if (aeExtFace->theEdge() == aeN->theEdge())
					{
						aeNInExtFace = true;
						if (succInExtFace)
							break;
					}
					aeExtFace = aeExtFace->faceCycleSucc();
				} while(aeExtFace != adjMaxFace);
				if (aeNInExtFace && succInExtFace)
					m_leftNode = aeN->twinNode();
				else
					m_leftNode = aeN->theNode();

				node twinTN = S.twinTreeNode(aeN->theEdge());
				if (referenceEdge) {
					if (aeN->theEdge()->source() == aeN->theNode()) {
						if (aeN->theEdge()->target() == referenceEdge->source())
							adjBeforeNodeArrayTarget[twinTN] = adjBeforeNodeArraySource[mu];
						else if (aeN->theEdge()->target() == referenceEdge->target())
							adjBeforeNodeArrayTarget[twinTN] = adjBeforeNodeArrayTarget[mu];
					}
					else
					{
						if (aeN->theEdge()->source() == referenceEdge->source())
							adjBeforeNodeArraySource[twinTN] = adjBeforeNodeArraySource[mu];
						else if (aeN->theEdge()->source() == referenceEdge->target())
							adjBeforeNodeArraySource[twinTN] = adjBeforeNodeArrayTarget[mu];
					}
				}
			}

			adjEntryForNode(aeN, before, spqrTree, treeNodeTreated, mu,
				m_leftNode, nodeLength, edgeLength, thickness, newOrder,
				adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
				0, 0, adjExternal);

			//if the other node adjacent to the current treated edge is not in the
			//max face, put written edges into an buffer and clear the adjacency
			//list of that node.
			if (!maxFaceNodes.search(aeN->twinNode()).valid())
			{
				node orig_aeN_twin_theNode = S.original(aeN->twinNode());
				buffer[aeN->theEdge()] = newOrder[orig_aeN_twin_theNode];
				newOrder[orig_aeN_twin_theNode].clear();
			}
		}
	}

	//Copy of not treated node's adjacency lists (internal nodes). Setting
	//of left node depending on minimal distance to external face of the
	//face defined by left node.
	bool DGcomputed = false;
	int f_ext_id = 0;
	int f_ref_id = 0;
	Graph DG;
	ArrayBuffer<node> fPG_to_nDG;
	NodeArray< List<adjEntry> > adjacencyList;
	List< List<adjEntry> > faces;
	NodeArray<T> dist_f_ref;
	NodeArray<T> dist_f_ext;
	AdjEntryArray<int> ae_to_face;

	for(node v : S.getGraph().nodes)
	{
		if (nodeTreated[v])
			continue;

		node v_original = S.original(v);
		nodeTreated[v] = true;
		ListIterator<adjEntry> before;
		for (adjEntry ae = v->firstAdj(); ae; ae = ae->succ())
		{
			if (buffer[ae->theEdge()].empty())
			{
				T delta_u_nu = 0;
				T delta_d_nu = 0;
				bool frauenlinks = false;
				if (S.isVirtual(ae->theEdge()))
				{
					if (!DGcomputed) {
						ae_to_face.init(S.getGraph());
						EdgeArray<T> edgeLengthDG(DG);
						DGcomputed = true;

						//compute dual graph of skeleton graph:
						adjacencyList.init(S.getGraph());
						for(node nBG : S.getGraph().nodes)
						{
							for(adjEntry ae_nBG : nBG->adjEntries)
								adjacencyList[nBG].pushBack(ae_nBG);
						}

						NodeArray< List<adjEntry> > adjEntryTreated(S.getGraph());
						for(node nBG : S.getGraph().nodes)
						{
							for(adjEntry adj : nBG->adjEntries)
							{
								if (adjEntryTreated[nBG].search(adj).valid())
									continue;

								List<adjEntry> newFace;
								adjEntry adj2 = adj;
								do {
									newFace.pushBack(adj2);
									ae_to_face[adj2] = faces.size();
									adjEntryTreated[adj2->theNode()].pushBack(adj2);
									List<adjEntry> &ladj = adjacencyList[adj2->twinNode()];
									adj2 = *ladj.cyclicPred(ladj.search(adj2->twin()));
								} while (adj2 != adj);
								faces.pushBack(newFace);
							}
						}

						for (ListIterator< List<adjEntry> > it = faces.begin(); it.valid(); ++it) {
							fPG_to_nDG.push(DG.newNode());
						}

						NodeArray< List<node> > adjFaces(DG);
						int i = 0;
						for (ListIterator< List<adjEntry> > it = faces.begin(); it.valid(); ++it) {
							int f1_id = i;
							for (ListIterator<adjEntry> it2 = (*it).begin(); it2.valid(); ++it2) {
								int f2_id = 0;
								int j = 0;
								for (ListIterator< List<adjEntry> > it3 = faces.begin(); it3.valid(); ++it3)
								{
									bool do_break = false;
									for (ListIterator<adjEntry> it4 = (*it3).begin(); it4.valid(); ++it4)
									{
										if ((*it4) == (*it2)->twin())
										{
											f2_id = j;
											do_break = true;
											break;
										}
									}
									if (do_break)
										break;
									j++;
								}

								if (f1_id != f2_id
								 && !adjFaces[fPG_to_nDG[f1_id]].search(fPG_to_nDG[f2_id]).valid()
								 && !adjFaces[fPG_to_nDG[f2_id]].search(fPG_to_nDG[f1_id]).valid())
								{
									adjFaces[fPG_to_nDG[f1_id]].pushBack(fPG_to_nDG[f2_id]);
									edge e1 = DG.newEdge(fPG_to_nDG[f1_id], fPG_to_nDG[f2_id]);
									edge e2 = DG.newEdge(fPG_to_nDG[f2_id], fPG_to_nDG[f1_id]);

									//set edge length:
									T e_length = -1;
									for (auto f1 : *it) {
										edge e = f1->theEdge();
										for (auto f2 : *faces.get(f2_id)) {
											if (f2->theEdge() == e
											 && (e_length == -1
											  || edgeLength[mu][e] < e_length)) {
												e_length = edgeLength[mu][e];
											}
										}
									}
									edgeLengthDG[e1] = e_length;
									edgeLengthDG[e2] = e_length;
								}

								if (*it2 == adjMaxFace)
									f_ext_id = f1_id;
								if (referenceEdge && *it2 == adjMaxFace->twin())
									f_ref_id = f1_id;
							}
							i++;
						}

						//compute shortest path from every face to the external face:
						node f_ext_DG = fPG_to_nDG[f_ext_id];
						dist_f_ext.init(DG);
						sssp(DG, f_ext_DG, edgeLengthDG, dist_f_ext);
						if (referenceEdge) {
							node f_ref_DG = fPG_to_nDG[f_ref_id];
							dist_f_ref.init(DG);
							sssp(DG, f_ref_DG, edgeLengthDG, dist_f_ref);
						}
					}

					//choose face with minimal shortest path:
					T min1, min2;
					T pi_f_0_f_ext = dist_f_ext[fPG_to_nDG[ae_to_face[ae]]];
					T pi_f_1_f_ext = dist_f_ext[fPG_to_nDG[ae_to_face[ae->twin()]]];
					if (referenceEdge) {
						T pi_f_0_f_ref = dist_f_ref[fPG_to_nDG[ae_to_face[ae]]];
						T pi_f_1_f_ref = dist_f_ref[fPG_to_nDG[ae_to_face[ae->twin()]]];

						if (delta_u + pi_f_0_f_ref < delta_d + pi_f_0_f_ext)
							min1 = delta_u + pi_f_0_f_ref;
						else
							min1 = delta_d + pi_f_0_f_ext;

						if (delta_u + pi_f_1_f_ref < delta_d + pi_f_1_f_ext)
							min2 = delta_u + pi_f_1_f_ref;
						else
							min2 = delta_d + pi_f_1_f_ext;

						if (min1 > min2)
						{
							delta_u_nu = delta_u;
							if (pi_f_0_f_ref < pi_f_0_f_ext)
								delta_u_nu += pi_f_0_f_ref;
							else
								delta_u_nu += pi_f_0_f_ext;
							delta_d_nu = delta_d;
							if (pi_f_1_f_ref < pi_f_1_f_ext)
								delta_d_nu += pi_f_1_f_ref;
							else
								delta_d_nu += pi_f_1_f_ext;
						}
						else
						{
							frauenlinks = true;
							delta_u_nu = delta_u;
							if (pi_f_1_f_ref < pi_f_1_f_ext)
								delta_u_nu += pi_f_1_f_ref;
							else
								delta_u_nu += pi_f_1_f_ext;
							delta_d_nu = delta_d;
							if (pi_f_0_f_ref < pi_f_0_f_ext)
								delta_d_nu += pi_f_0_f_ref;
							else
								delta_d_nu += pi_f_0_f_ext;
						}
					}
					else
					{
						min1 = delta_d + pi_f_0_f_ext;
						min2 = delta_d + pi_f_1_f_ext;

						if (min1 > min2)
						{
							delta_u_nu = delta_u + pi_f_0_f_ext;
							delta_d_nu = delta_d + pi_f_1_f_ext;
						}
						else
						{
							frauenlinks = true;
							delta_u_nu = delta_u + pi_f_1_f_ext;
							delta_d_nu = delta_d + pi_f_0_f_ext;
						}
					}
				}

				if (frauenlinks)
				{
					node nu = S.twinTreeNode(ae->theEdge());

					//buffer computed embedding:
					NodeArray< List<adjEntry> > tmp_newOrder(spqrTree.originalGraph());
					ListIterator<adjEntry> tmp_before;

					adjEntryForNode(
						ae, tmp_before, spqrTree, treeNodeTreated, mu,
						v, nodeLength, edgeLength, thickness, tmp_newOrder,
						adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
						delta_u_nu, delta_d_nu, adjExternal);

					//copy buffered embedding reversed into newOrder:
#if 0
					node m_leftNode = v;
#endif
					node m_rightNode = ae->twinNode();
					node leftOrg = v_original;
					node rightOrg = S.original(m_rightNode);
					for(node nOG : spqrTree.originalGraph().nodes)
					{
						List<adjEntry> nOG_tmp_adjList = tmp_newOrder[nOG];
						if (nOG_tmp_adjList.size() == 0)
							continue;

						ListIterator<adjEntry>* m_before;
						if (nOG == leftOrg)
							m_before = &before;
						else
							m_before = new ListIterator<adjEntry>();

						for (ListIterator<adjEntry> ae_it = nOG_tmp_adjList.begin(); ae_it.valid(); ++ae_it)
						{
							adjEntry adjaEnt = *ae_it;
							if (!m_before->valid())
								*m_before = newOrder[nOG].pushBack(adjaEnt);
							else
								*m_before = newOrder[nOG].insertBefore(adjaEnt, *m_before);

							if (nOG == leftOrg || nOG == rightOrg)
							{
								if (S.original(ae->theEdge()->source()) == nOG)
									adjBeforeNodeArraySource[nu] = *m_before;
								else
									adjBeforeNodeArrayTarget[nu] = *m_before;
							}
						}

						if (nOG != leftOrg)
							delete m_before;
					}
				}
				else
					adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
						v, nodeLength, edgeLength, thickness, newOrder,
						adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
						delta_u_nu, delta_d_nu, adjExternal);

				if (!nodeTreated[ae->twinNode()])
				{
					node orig_ae_twin_theNode = S.original(ae->twinNode());
					buffer[ae->theEdge()] = newOrder[orig_ae_twin_theNode];
					newOrder[orig_ae_twin_theNode].clear();
				}
			}
			else
			{
				buffer[ae->theEdge()].reverse();
				for (ListIterator<adjEntry> it = buffer[ae->theEdge()].begin(); it.valid(); ++it)
				{
					if (!before.valid())
						before = newOrder[v_original].pushFront(*it);
					else
						before = newOrder[v_original].insertBefore(*it, before);
				}
			}
		}
	}
}

template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::bottomUpThickness(
	const StaticSPQRTree& spqrTree,
	const node& mu,
	NodeArray<T>& thickness,
	const NodeArray<T>& nodeLength,
	const NodeArray< EdgeArray<T> >& edgeLength)
{
	//recursion:
	for(adjEntry adj : mu->adjEntries) {
		edge e_mu_to_nu = adj->theEdge();
		if (e_mu_to_nu->source() != mu)
			continue;
		else
		{
			node nu = e_mu_to_nu->target();
			bottomUpThickness(spqrTree, nu, thickness, nodeLength, edgeLength);
		}
	}

	Skeleton& S = spqrTree.skeleton(mu);
	edge referenceEdge = S.referenceEdge();

	if (!referenceEdge) { //root of SPQR-tree
		thickness[mu] = 0;
		return;
	}

	//set dLengths for all edges in skeleton graph:
	EdgeArray<T> dLength(S.getGraph());
	for(edge eSG : S.getGraph().edges)
	{
		if (eSG == referenceEdge)
			continue;

		if (S.isVirtual(eSG))
		{
			node twinTN = S.twinTreeNode(eSG);
			dLength[eSG] = thickness[twinTN];
		}
		else
			dLength[eSG] = edgeLength[mu][eSG];
	}

	//compute thickness of skeleton(mu):
	switch (spqrTree.typeOf(mu))
	{
	case SPQRTree::NodeType::SNode:
		{
			//thickness(mu) = min_{edges e != referenceEdge} dLength(e)
			T min_dLength = -1;
			for(edge eSG : S.getGraph().edges)
			{
				if (eSG == referenceEdge)
					continue;
				if (min_dLength == -1 || dLength[eSG] < min_dLength)
					min_dLength = dLength[eSG];
			}
			thickness[mu] = min_dLength;
		} break;
	case SPQRTree::NodeType::PNode:
		{
			//thickness(mu) = sum_{edges e != referenceEdge} dLength(e)
			T sumof_dLength = 0;
			for(edge eSG : S.getGraph().edges)
			{
				if (eSG == referenceEdge)
					continue;
				sumof_dLength += dLength[eSG];
			}
			thickness[mu] = sumof_dLength;
		} break;
	case SPQRTree::NodeType::RNode:
		{
			/* Let f^ref_0, ... , f^ref_k be the faces sharing at least one edge with
			 * f_ref - the face adjacent to the reference edge not equal to the
			 * external face f_ext. Compute the dual graph and set edge lengths for
			 * two faces f_i, f_j, i != j, with at least one shared edge, to the
			 * minimal dLength of the shared edges of f_i and f_j. Remove the node
			 * related to face f_ref. thickness(mu) is then the length of the shortest
			 * path from any of the faces f^ref_0, ... , f^ref_k to f_ext plus 1.
			 */
			CombinatorialEmbedding CE(S.getGraph());
			adjEntry ae_f_ext = referenceEdge->adjSource();
			adjEntry ae_f_ref = referenceEdge->adjTarget();
			T faceSize_f_ext = 0;
			adjEntry ae_f_ext_walker = ae_f_ext;
			do
			{
				faceSize_f_ext += nodeLength[S.original(ae_f_ext_walker->theNode())]
					+  edgeLength[mu][ae_f_ext_walker->theEdge()];
				ae_f_ext_walker = ae_f_ext_walker->faceCycleSucc();
			} while (ae_f_ext_walker != ae_f_ext);
			T faceSize_f_ref = 0;
			adjEntry ae_f_ref_walker = ae_f_ref;
			do
			{
				faceSize_f_ref += nodeLength[S.original(ae_f_ref_walker->theNode())]
					+  edgeLength[mu][ae_f_ref_walker->theEdge()];
				ae_f_ref_walker = ae_f_ref_walker->faceCycleSucc();
			} while (ae_f_ref_walker != ae_f_ref);
			if (faceSize_f_ext < faceSize_f_ref)
			{
				adjEntry ae_tmp = ae_f_ext;
				ae_f_ext = ae_f_ref;
				ae_f_ref = ae_tmp;
			}

			//compute dual graph:
			Graph DG;
			ArrayBuffer<node> fPG_to_nDG;
			NodeArray< List<adjEntry> > adjacencyList(S.getGraph());
			List< List<adjEntry> > faces;
			NodeArray<int> distances;
			EdgeArray<T> edgeLengthDG(DG);
			int f_ref_id = -1;
			int f_ext_id = -1;

			for(node nBG : S.getGraph().nodes)
			{
				for(adjEntry ae_nBG : nBG->adjEntries)
					adjacencyList[nBG].pushBack(ae_nBG);
			}

			NodeArray< List<adjEntry> > adjEntryTreated(S.getGraph());
			for(node nBG : S.getGraph().nodes)
			{
				for(adjEntry adj : nBG->adjEntries)
				{
					if (adjEntryTreated[nBG].search(adj).valid())
						continue;

					List<adjEntry> newFace;
					adjEntry adj2 = adj;
					do {
						newFace.pushBack(adj2);
						adjEntryTreated[adj2->theNode()].pushBack(adj2);
						List<adjEntry> &ladj = adjacencyList[adj2->twinNode()];
						adj2 = *ladj.cyclicPred(ladj.search(adj2->twin()));
					} while (adj2 != adj);
					faces.pushBack(newFace);
				}
			}

			for (ListIterator< List<adjEntry> > it = faces.begin(); it.valid(); ++it) {
				fPG_to_nDG.push(DG.newNode());
			}

			NodeArray< List<node> > adjFaces(DG);
			int i = 0;
			for (ListIterator< List<adjEntry> > it = faces.begin(); it.valid(); ++it)
			{
				int f1_id = i;
				for (ListIterator<adjEntry> it2 = (*it).begin(); it2.valid(); ++it2)
				{
					int f2_id = 0;
					int j = 0;
					for (ListIterator< List<adjEntry> > it3 = faces.begin(); it3.valid(); ++it3)
					{
						bool do_break = false;
						for (ListIterator<adjEntry> it4 = (*it3).begin(); it4.valid(); ++it4)
						{
							if ((*it4) == (*it2)->twin())
							{
								f2_id = j;
								do_break = true;
								break;
							}
						}
						if (do_break)
							break;
						j++;
					}

					if (f1_id != f2_id
					 && !adjFaces[fPG_to_nDG[f1_id]].search(fPG_to_nDG[f2_id]).valid()
					 && !adjFaces[fPG_to_nDG[f2_id]].search(fPG_to_nDG[f1_id]).valid())
					{
						adjFaces[fPG_to_nDG[f1_id]].pushBack(fPG_to_nDG[f2_id]);
						adjFaces[fPG_to_nDG[f2_id]].pushBack(fPG_to_nDG[f1_id]);
						edge e1 = DG.newEdge(fPG_to_nDG[f1_id], fPG_to_nDG[f2_id]);
						edge e2 = DG.newEdge(fPG_to_nDG[f2_id], fPG_to_nDG[f1_id]);

						//set edge length:
						T e_length = -1;
						for (ListIterator<adjEntry> it_f1 = (*it).begin(); it_f1.valid(); ++it_f1)
						{
							edge e = (*it_f1)->theEdge();
							for (ListIterator<adjEntry> it_f2 = (*(faces.get(f2_id))).begin();
								it_f2.valid();
								++it_f2)
							{
								if ((*it_f2)->theEdge() == e
								 && (e_length == -1
								  || edgeLength[mu][e] < e_length)) {
										e_length = edgeLength[mu][e];
								}
							}
						}
						edgeLengthDG[e1] = e_length;
						edgeLengthDG[e2] = e_length;
					}

					if (*it2 == ae_f_ext)
						f_ext_id = f1_id;
					if (*it2 == ae_f_ref)
						f_ref_id = f1_id;
				}
				i++;
			}

			//the faces sharing at least one edge with f_ref:
			node nDG_f_ref = fPG_to_nDG[f_ref_id];
			List<node>& f_ref_adj_faces = adjFaces[nDG_f_ref];

			//remove node related to f_ref from dual graph:
			DG.delNode(fPG_to_nDG[f_ref_id]);

			//compute shortest path and set thickness:
			NodeArray<T> dist(DG);
			node nDG_f_ext = fPG_to_nDG[f_ext_id];
			sssp(DG, nDG_f_ext, edgeLengthDG, dist);
			T minDist = -1;
			for (ListIterator<node> it_adj_faces = f_ref_adj_faces.begin();
				it_adj_faces.valid();
				++it_adj_faces)
			{
				node fDG = *it_adj_faces;
				if (fDG != nDG_f_ext)
				{
					T d = dist[fDG];
					if (minDist == -1 || d < minDist)
						minDist = d;
				}
			}
			thickness[mu] = minDist + 1;
		} break;
	default:
		OGDF_ASSERT(false);
	}
}


template<class T>
bool EmbedderMaxFaceBiconnectedGraphsLayers<T>::sssp(
	const Graph& G,
	const node& s,
	const EdgeArray<T>& length,
	NodeArray<T>& d)
{
	const T infinity = 20000000; // big number. danger. think about it.

	//Initialize-Single-Source(G, s):
	d.init(G);
	for(node v : G.nodes)
		d[v] = infinity;

	d[s] = 0;
	for (int i = 1; i < G.numberOfNodes(); ++i)
	{
		for(edge e : G.edges)
		{
			//relax(u, v, w): // e == (u, v), length == w
			if (d[e->target()] > d[e->source()] + length[e])
				d[e->target()] = d[e->source()] + length[e];
		}
	}

	//check for negative cycle:
	for(edge e : G.edges)
	{
		if (d[e->target()] > d[e->source()] + length[e])
			return false;
	}

	return true;
}

}
