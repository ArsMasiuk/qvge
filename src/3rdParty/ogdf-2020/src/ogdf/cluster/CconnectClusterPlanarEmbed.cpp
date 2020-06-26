/** \file
 * \brief Implementation of Cluster Planarity tests and Cluster
 * Planar embedding for C-connected Cluster Graphs
 *
 * \author Sebastian Leipert
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

#include <ogdf/basic/Graph.h>
#include <ogdf/cluster/CconnectClusterPlanarEmbed.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/STNumbering.h>
#include <ogdf/fileformats/GraphIO.h>

namespace ogdf {

using namespace booth_lueker;

// Constructor
CconnectClusterPlanarEmbed::CconnectClusterPlanarEmbed()
{
	m_errorCode = ErrorCode::none;
}

// Destructor
CconnectClusterPlanarEmbed::~CconnectClusterPlanarEmbed()
{
}

// Tests if a ClusterGraph is c-planar and embedds it.
bool CconnectClusterPlanarEmbed::embed(ClusterGraph &C, Graph &G)
{
#ifdef OGDF_DEBUG
	C.consistencyCheck();
#endif

	if (G.numberOfNodes() <= 1) return true;

	// Initialize Node and cluster arrays associated with original graph.
	m_instance = &C;
	m_nodeTableOrig2Copy.init(G,nullptr);
	m_clusterTableOrig2Copy.init(C,nullptr);
	m_clusterEmbedding.init(C,nullptr);
	m_clusterSubgraph.init(C,nullptr);
	m_clusterSubgraphHubs.init(C,nullptr);
	m_clusterSubgraphWheelGraph.init(C,nullptr);
	m_clusterClusterGraph.init(C,nullptr);
	m_clusterNodeTableNew2Orig.init(C,nullptr);
	m_clusterOutgoingEdgesAnker.init(C,nullptr);
	m_clusterSuperSink.init(C,nullptr);
	m_clusterPQContainer.init(C);
	m_unsatisfiedCluster.init(C,false);

	// Copy the graph (necessary, since we modify it throughout the planarity test)
	Graph Gcopy;
	ClusterGraph Ccopy(C,Gcopy,m_clusterTableOrig2Copy,m_nodeTableOrig2Copy);

	// Initialize translation tables for nodes and clusters
	m_clusterTableCopy2Orig.init(Ccopy,nullptr);
#if 0
	cluster c;
#endif
	for(cluster c : C.clusters)
	{
		cluster c1 = m_clusterTableOrig2Copy[c];
		m_clusterTableCopy2Orig[c1] = c;
	}
	m_nodeTableCopy2Orig.init(Gcopy,nullptr);
	for(node v : G.nodes)
	{
		node w = m_nodeTableOrig2Copy[v];
		m_nodeTableCopy2Orig[w] = v;
	}
	// Remove empty clusters
	SList<cluster> removeCluster;
	for(cluster c : Ccopy.clusters)
	{
		if (c->cCount() == 0 && c->nCount() == 0)
			removeCluster.pushBack(c);
	}
	while (!removeCluster.empty())
	{
		cluster c = removeCluster.popFrontRet();
		m_unsatisfiedCluster[m_clusterTableCopy2Orig[c]] = true;
		cluster parent = c->parent();
		Ccopy.delCluster(c);
		if (parent->cCount() == 0 && parent->nCount() == 0)
			removeCluster.pushBack(parent);
	}
	while (Ccopy.rootCluster()->cCount() == 1 && Ccopy.rootCluster()->nCount() == 0)
	{
		cluster c = *Ccopy.rootCluster()->cBegin();
		m_unsatisfiedCluster[m_clusterTableCopy2Orig[c]] = true;
		Ccopy.delCluster(c);
	}

#ifdef OGDF_DEBUG
	Ccopy.consistencyCheck();
#endif

	// Initialize node and cluster arrays associated with copied graph.
	m_clusterPQTree.init(Ccopy,nullptr);
	m_currentHubs.init(Gcopy,false);
	m_wheelGraphNodes.init(Gcopy,nullptr);
	m_outgoingEdgesAnker.init(Gcopy,nullptr);

	// Planarity test
	bool cPlanar = preProcess(Ccopy,Gcopy);

	if (cPlanar)
	{
		OGDF_ASSERT(Gcopy.representsCombEmbedding());
#if 0
		Ccopy.consistencyCheck();
#endif

		recursiveEmbed(Ccopy,Gcopy);
#ifdef OGDF_DEBUG
		Ccopy.consistencyCheck();
#endif

		copyEmbedding(Ccopy,Gcopy,C,G);

		C.adjAvailable(true);

	}
	else
		nonPlanarCleanup(Ccopy,Gcopy);

	// Cleanup
	for(cluster c : C.clusters)
	{
		if (m_clusterSubgraph[c] != nullptr && c != C.rootCluster())
			delete m_clusterSubgraph[c];
	}

	// Deinitialize all node and cluster arrays
	m_parallelEdges.init();
	m_isParallel.init();
	m_clusterPQTree.init();
	m_clusterEmbedding.init();
	m_clusterSubgraph.init();
	m_clusterSubgraphHubs.init();
	m_clusterSubgraphWheelGraph.init();
	m_clusterClusterGraph.init();
	m_clusterNodeTableNew2Orig.init();
	m_clusterOutgoingEdgesAnker.init();
	m_clusterSuperSink.init();
	m_clusterPQContainer.init();

	m_clusterTableOrig2Copy.init();
	m_clusterTableCopy2Orig.init();
	m_nodeTableOrig2Copy.init();
	m_nodeTableCopy2Orig.init();
	m_currentHubs.init();
	m_wheelGraphNodes.init();
	m_outgoingEdgesAnker.init();

	return cPlanar;
}

//
//	CallTree:
//
//  call(ClusterGraph &C)
//
//		preProcess(ClusterGraph &C,Graph &G)
//
//			planarityTest(ClusterGraph &C,cluster &act,Graph &G)
//
//				foreach ChildCluster
//					planarityTest(ClusterGraph &C,cluster &act,Graph &G)
//
//				preparation(Graph  &G,cluster &origCluster)
//
//					foreach biconnected Component
//						doTest(Graph &G,NodeArray<int> &numbering,cluster &origCluster)
//

// Copies the embedding of Ccopy to C
void CconnectClusterPlanarEmbed::copyEmbedding(
	ClusterGraph &Ccopy,
	Graph &Gcopy,
	ClusterGraph &C,
	Graph &G)
{
	OGDF_ASSERT(Gcopy.representsCombEmbedding());
	OGDF_ASSERT(Ccopy.representsCombEmbedding());

	AdjEntryArray<adjEntry> adjTableCopy2Orig(Gcopy);
	AdjEntryArray<adjEntry> adjTableOrig2Copy(G);
	AdjEntryArray<bool>     visited(G, false);				 // For parallel edges
	EdgeArray<edge>         edgeTableCopy2Orig(Gcopy, nullptr);     // Translation table for parallel edges
	EdgeArray<bool>         parallelEdge(Gcopy, false);		 // Marks parallel edges in copy Graph
	AdjEntryArray<adjEntry>	parallelEntryPoint(G, nullptr);		 // For storing information on parallel
	// edges for cluster adjlistst.
	AdjEntryArray<bool>		parallelToBeIgnored(Gcopy, false);// For storing information on parallel
	// edges for cluster adjlistst.

	// prepare parallel Edges
	prepareParallelEdges(G);
	NodeArray<SListPure<adjEntry> > entireEmbedding(G);

	//process over all copy nodes
	for (node vCopy : Gcopy.nodes)
	{
		//get the original node
		node wOrig = m_nodeTableCopy2Orig[vCopy];

		//process over all adjacent copy edges
		for (adjEntry vAdj : vCopy->adjEntries)
		{
			node vN = vAdj->twinNode();
			node wN = m_nodeTableCopy2Orig[vN];
			m_nodeTableOrig2Copy[wN] = vN;

			for (adjEntry wAdj : wOrig->adjEntries)
			{

				if (edgeTableCopy2Orig[vAdj->theEdge()] != nullptr &&
					m_isParallel[edgeTableCopy2Orig[vAdj->theEdge()]])
					// Break if parallel edge (not a reference edge) that has already been assigned.
					break;
				if (wAdj->twinNode() == wN
					&& !visited[wAdj] && !m_isParallel[wAdj->theEdge()])
					//					&& !m_isParallel[wAdj->theEdge()])
					// Either a non parallel edge or the reference edge of a set of
					// parallel edges.
				{
					adjTableCopy2Orig[vAdj] = wAdj;
					adjTableOrig2Copy[wAdj] = vAdj;
#if 0
					adjTableCopy2Orig[vAdj->twin()] = wAdj->twin();
					adjTableOrig2Copy[wAdj->twin()] = vAdj->twin();
#endif
					edgeTableCopy2Orig[vAdj->theEdge()] = wAdj->theEdge();
#ifdef OGDF_HEAVY_DEBUG
					Logger::slout() << "Orig " << wAdj << " " << wAdj->index() << "\t twin " << wAdj->twin()->index() << std::endl;
					Logger::slout() << "Copy " << vAdj << " " << vAdj->index() << "\t twin " << vAdj->twin()->index() << std::endl << std::endl;
#endif
					entireEmbedding[wOrig].pushBack(wAdj);	// if no parallel edges exist,
					// this will be our embedding.
#if 0
					entireEmbedding[wN].pushFront(wAdj->twin());
#endif
					visited[wAdj] = true; // for multi-edges
					//					visited[wAdj->twin()] = true; // for multi-edges
					break;
				}
				else if (wAdj->twinNode() == wN  && !visited[wAdj])
					// A parallel edge that is not the reference edge.
					// We need to set the translation table
				{
					adjTableCopy2Orig[vAdj] = wAdj;
					adjTableOrig2Copy[wAdj] = vAdj;
					adjTableCopy2Orig[vAdj->twin()] = wAdj->twin();
					adjTableOrig2Copy[wAdj->twin()] = vAdj->twin();
					edgeTableCopy2Orig[vAdj->theEdge()] = wAdj->theEdge();
					visited[wAdj] = true; // So we do not consider parallel edges twice.
					visited[wAdj->twin()] = true; // So we do not consider parallel edges twice.
				}
			}
		}
	}

	// Locate all parallel edges
	// Sort them within the adjacency lists,
	// such that they appear consecutively.
	NodeArray<SListPure<adjEntry> > newEntireEmbedding(G);
	NodeArray<SListPure<adjEntry> > newEntireEmbeddingCopy(Gcopy);

	if (m_parallelCount > 0)
	{
		for (node v : G.nodes)
		{
			for (adjEntry ae : entireEmbedding[v])
			{
				edge e = ae->theEdge();

				if (!m_parallelEdges[e].empty())
				{
					// This edge is the reference edge
					// of a bundle of parallel edges

					// If v is source of e, insert the parallel edges
					// in the order stored in the list.
					if (e->adjSource()->theNode() == v)
					{
						adjEntry adj = e->adjSource();

						newEntireEmbedding[v].pushBack(adj);
						newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]].pushBack(adjTableOrig2Copy[adj]);

						parallelEntryPoint[e->adjSource()] = adj;
						parallelToBeIgnored[adjTableOrig2Copy[adj]] = true;

						for (edge parallel : m_parallelEdges[e])
						{
							adjEntry adjP = parallel->adjSource()->theNode() == v ?
								parallel->adjSource() : parallel->adjTarget();
							parallelToBeIgnored[adjTableOrig2Copy[adjP]] = true;
#ifdef OGDF_HEAVY_DEBUG
							Logger::slout() << adjP << " " << adjP->index() << "\t twin " << adjP->twin()->index() << std::endl;
#endif
							newEntireEmbedding[v].pushBack(adjP);
							newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]].pushBack(adjTableOrig2Copy[adjP]);
						}
					} else {
						// v is target of e, insert the parallel edges
						// in the opposite order stored in the list.
						// This keeps the embedding.
						bool first = true;
						for (edge parallel : reverse(m_parallelEdges[e])) {
							adjEntry adj = parallel->adjSource()->theNode() == v ?
								parallel->adjSource() : parallel->adjTarget();
							parallelToBeIgnored[adjTableOrig2Copy[adj]] = true;

							newEntireEmbedding[v].pushBack(adj);
							newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]].pushBack(adjTableOrig2Copy[adj]);
							if (first)
							{
#if 0
								parallelEntryPoint[adjTableOrig2Copy[adj]] = adj;
#endif
								parallelEntryPoint[e->adjTarget()] = adj;
								first = false;
							}
						}
						adjEntry adj = e->adjTarget();

						newEntireEmbedding[v].pushBack(adj);
						newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]];//.pushBack(adjTableOrig2Copy[adj]);
						newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]].pushBack(adjTableOrig2Copy[adj]);
						parallelToBeIgnored[adjTableOrig2Copy[adj]] = true;
					}
				} else if (!m_isParallel[e]) {
					// normal non-multi-edge
					adjEntry adj = e->adjSource()->theNode() == v ?
						e->adjSource() : e->adjTarget();

					newEntireEmbedding[v].pushBack(adj);
					newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]];//pushBack(adjTableOrig2Copy[adj]);
					adjTableOrig2Copy[adj];
					newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]].pushBack(adjTableOrig2Copy[adj]);
				}
				// else e is a multi-edge but not the reference edge
			}
		}

		for (node v : G.nodes)
			G.sort(v, newEntireEmbedding[v]);
		for (node v : Gcopy.nodes)
			Gcopy.sort(v, newEntireEmbeddingCopy[v]);
	} else {
		for (node v : G.nodes)
			G.sort(v, entireEmbedding[v]);
		OGDF_ASSERT(G.representsCombEmbedding());
	}

	OGDF_ASSERT(G.representsCombEmbedding());

	for (cluster c : Ccopy.clusters)
	{
		SListPure<adjEntry>	embedding;

		for (adjEntry adj : c->adjEntries)
		{
			edge e = adj->theEdge();

			if (!m_parallelEdges[edgeTableCopy2Orig[e]].empty())
			{
				adjEntry padj = parallelEntryPoint[adjTableCopy2Orig[adj]];

				node target = padj->twinNode();

				// Scan the parallel edges of e
				// in the original graph along the embedded
				// adjacency list of its target
				while (true) {
					if (padj->twinNode() == target) { // is a multi-edge
						embedding.pushBack(padj);
						padj = padj->succ();
						if (!padj) break; // only multi-edges
					} else { // not a multi-edge
						break;
					}
				}
			}
			else if (!parallelToBeIgnored[adj])
			{
				embedding.pushBack(adjTableCopy2Orig[adj]);
			}
		}

		C.makeAdjEntries(m_clusterTableCopy2Orig[c], embedding.begin());
	}
}

// Deallocates all memory, if the cluster graph is not cluster planar
void CconnectClusterPlanarEmbed::nonPlanarCleanup(ClusterGraph &Ccopy,Graph &Gcopy)
{
	while (!m_callStack.empty())
	{
		cluster	act	= m_callStack.popRet();

		Graph *subGraph	= m_clusterSubgraph[act];

		node superSink = m_clusterPQContainer[act].m_superSink;
		if (superSink)
		{
			for(edge e : subGraph->edges)
			{
				if (e->source() != superSink && e->target() != superSink)
					delete (*m_clusterOutgoingEdgesAnker[act])[e];
			}
		}

		delete m_clusterEmbedding[act];
		delete m_clusterSubgraphHubs[act];
		delete m_clusterSubgraphWheelGraph[act];
		delete m_clusterNodeTableNew2Orig[act];
		delete m_clusterOutgoingEdgesAnker[act];

		m_clusterPQContainer[act].Cleanup();
	}

	for(edge e : Gcopy.edges)
	{
		delete m_outgoingEdgesAnker[e];
	}
}

// This function is called by recursiveEmbed only. It fixes
// the adjacency lists of the hubs in Gcopy after a cluster has been
// reembedded.
void CconnectClusterPlanarEmbed::hubControl(Graph &G,NodeArray<bool> &hubs)
{
	for(node hub : G.nodes)
	{
		if (hubs[hub]) // hub is a hub
		{
			adjEntry startAdj = hub->firstAdj();
			adjEntry firstAdj = nullptr;
			adjEntry secAdj = nullptr;
			while (firstAdj != startAdj)
			{
				if (firstAdj == nullptr)
					firstAdj = startAdj;
				secAdj = firstAdj->cyclicSucc();
				node firstNode = firstAdj->twinNode();
				node secNode = secAdj->twinNode();

				adjEntry cyclicPredOfFirst = firstAdj->twin()->cyclicPred();
				while(cyclicPredOfFirst->twinNode()
						!= secNode)
				{
					cyclicPredOfFirst = cyclicPredOfFirst->cyclicPred();
				}
				G.moveAdjBefore(cyclicPredOfFirst,firstAdj->twin());

				adjEntry cyclicSuccOfSec= secAdj->twin()->cyclicSucc();
				while(cyclicSuccOfSec->twinNode()
						!= firstNode)
				{
					cyclicSuccOfSec = cyclicSuccOfSec->cyclicSucc();
				}
				G.moveAdjAfter(cyclicSuccOfSec,secAdj->twin());

				firstAdj = secAdj;
			}
		}
	}
}

// Function computes the cluster planar embedding of a cluster graph
// by recursively reinserting the clusters back into Gcopy and embedding
// their corresponding subgraphs within the planar embedding of Gcopy.
void CconnectClusterPlanarEmbed::recursiveEmbed(ClusterGraph &Ccopy,Graph &Gcopy)
{
	// Remove root cluster from stack.
	// Induced subgraph of root cluster corresponds to Gcopy
	cluster root = m_callStack.popRet();

	OGDF_ASSERT(Gcopy.representsCombEmbedding());

	hubControl(Gcopy,m_currentHubs);

	while (!m_callStack.empty())
	{

		// Cluster act is reinserted into Gcopy.
		cluster act = m_callStack.popRet();
		if (m_unsatisfiedCluster[act]) {
			continue;
		}

		// subgraph is the graph that replaces the wheelGraph of act in Gcopy
		Graph* subGraph = m_clusterSubgraph[act];
		// embedding contains the (partial) embedding of all biconnected components
		// that do not have outgoing edges of the cluster act.
		NodeArray<SListPure<adjEntry>>* embedding = m_clusterEmbedding[act];
		// For every node of subGraph hubs is true if the node is a hub in subGraph
		NodeArray<bool>* hubs = m_clusterSubgraphHubs[act];
		// For every node in subGraph wheelGraphNodes stores the corresponding
		// cluster, if the node is a node of a wheel graph
		NodeArray<cluster>* wheelGraphNodes = m_clusterSubgraphWheelGraph[act];
		EmbedPQTree* T = m_clusterPQContainer[act].m_T;
		EdgeArray<ArrayBuffer<edge>*>* outgoingAnker = m_clusterOutgoingEdgesAnker[act];

		// What else do we have:
		//
		// 1. In m_wheelGraphNodes we have for every node of Gcopy that
		//    is a wheel graph node its corresponding cluster.
		//    Must UPDATE this information after we have replaced the current
		//    wheel graph by subGraph.

		// Make sure that:
		//
		// 1. When inserting new Nodes to Gcopy, that correspond to nodes of subGraph
		//    copy the information on the wheel graphs (stored in wheelGraphNodes)
		// 2. When inserting new Nodes to Gcopy, that correspond to nodes of subGraph
		//    copy the information if it is a hub (stored in hubs)


		// Translation tables between the subgraph and
		// its corresponding subgraph in Gcopy
		AdjEntryArray<adjEntry> tableAdjEntrySubGraph2Gcopy(*subGraph);
		NodeArray<node> nodeTableGcopy2SubGraph(Gcopy,nullptr);
		NodeArray<node> nodeTableSubGraph2Gcopy(*subGraph,nullptr);


		// Identify all wheelgraph nodes in Gcopy that correspond to act.
		// These nodes have to be removed and replaced by subGraph.

		SList<node> replaceNodes;
		for(node v : Gcopy.nodes)
			if (m_wheelGraphNodes[v] == act)
				replaceNodes.pushBack(v);


		// Introduce a new cluster in Gcopy
		cluster newCluster = nullptr;
		if (m_unsatisfiedCluster[act->parent()])
			newCluster = Ccopy.newCluster(Ccopy.rootCluster());
		else
			newCluster = Ccopy.newCluster(m_clusterTableOrig2Copy[act->parent()]);
		m_clusterTableOrig2Copy[act] = newCluster;
		m_clusterTableCopy2Orig[newCluster] = act;


		// Insert for every node of subGraph
		// a new node in Gcopy.
		for(node v : subGraph->nodes)
		{
			if (v != m_clusterSuperSink[act])
			{
				node newNode = Gcopy.newNode();
				Ccopy.reassignNode(newNode,newCluster);
				nodeTableGcopy2SubGraph[newNode] = v;
				nodeTableSubGraph2Gcopy[v] = newNode;

				// Copy information from subGraph nodes to new Gcopy nodes.
				if ((*wheelGraphNodes)[v])
					m_wheelGraphNodes[newNode] = (*wheelGraphNodes)[v];
				if ((*hubs)[v])
					m_currentHubs[newNode] = (*hubs)[v];
				m_nodeTableCopy2Orig[newNode] = (*m_clusterNodeTableNew2Orig[act])[v];
			}
		}


		// Insert the edges between the new nodes
		EdgeArray<bool> visited((*subGraph),false);
		for(node v : subGraph->nodes)
		{
			node newV = nodeTableSubGraph2Gcopy[v];

			if (v != m_clusterSuperSink[act])
			{
				for(adjEntry adj : v->adjEntries) {
					edge e = adj->theEdge();
					node w = e->opposite(v);

					if (w != m_clusterSuperSink[act] && !visited[e])
					{
						node newW = nodeTableSubGraph2Gcopy[w];
						edge eNew = Gcopy.newEdge(newV,newW);
						if ((e->adjSource()->theNode() == v
						  && eNew->adjSource()->theNode() == nodeTableSubGraph2Gcopy[v])
						 || (e->adjTarget()->theNode() == v
						  && eNew->adjTarget()->theNode() == nodeTableSubGraph2Gcopy[v])) {
							tableAdjEntrySubGraph2Gcopy[e->adjSource()] = eNew->adjSource();
							tableAdjEntrySubGraph2Gcopy[e->adjTarget()] = eNew->adjTarget();
						} else {
							tableAdjEntrySubGraph2Gcopy[e->adjTarget()] = eNew->adjSource();
							tableAdjEntrySubGraph2Gcopy[e->adjSource()] = eNew->adjTarget();
						}

						// Copy the information of outgoing edges
						// to the new edge.
						m_outgoingEdgesAnker[eNew] = (*outgoingAnker)[e];
						visited[e] = true;
					}
				}
			}
		}
#if 0
		edge borderEdge = m_clusterPQContainer[act].m_stEdgeLeaf->userStructKey();
#endif


		// start embedding here
		// first outgoing edge of cluster
		node startVertex = nullptr;
		edge startEdge   = nullptr;
		for (node v : replaceNodes)
		{
			// Assert that v is a node of the wheelgraph belonging
			// to cluster child.
			OGDF_ASSERT(m_wheelGraphNodes[v] == act);

			// Traverse all edges adajcent to v to locate an outgoing edge.
			for(adjEntry adj : v->adjEntries) {
				if (act != m_wheelGraphNodes[adj->twinNode()])
				{
					// Outgoing Edge of wheelgraph detected.
					startVertex = v;
					startEdge   = adj->theEdge();
					goto breakForLoop;
				}
			}
		}
	breakForLoop:

		// Stack outgoing edges according to embedding

		// Assert that there is an outgoing edge of the cluster
		OGDF_ASSERT(startEdge);
		List<edge> outgoingEdges;
		outgoingEdges.pushBack(startEdge);

		adjEntry adj =  startEdge->adjSource()->theNode() == startVertex ?
						startEdge->adjSource() : startEdge->adjTarget();
		edge currentEdge = nullptr;
		while (currentEdge != startEdge)
		{
			adjEntry newAdj = adj->cyclicSucc();
			newAdj = newAdj->twin();
			currentEdge = newAdj->theEdge();
			if (act != m_wheelGraphNodes[newAdj->theNode()])
			{
				// Outgoing Edge of wheelgraph detected.
				if (currentEdge != startEdge)
					outgoingEdges.pushBack(currentEdge);
				adj = adj->cyclicSucc();
			}
			else
				adj = newAdj;

		}

		// Insert the edges between the new nodes and
		// the existing nodes of Gcopy.

		PlanarLeafKey<IndInfo*>* leftKey = nullptr;
		PlanarLeafKey<IndInfo*>* rightKey = nullptr;
		edge firstEdge = nullptr;
		node t = m_clusterPQContainer[act].m_superSink;
		SListPure<PlanarLeafKey<IndInfo*>*> allOutgoing;

		#ifdef OGDF_DEBUG
		EdgeArray<edge> debugTableOutgoingSubGraph2Gcopy(*subGraph,nullptr);
		#endif

		ListIterator<edge> ite;
		for (ite = outgoingEdges.begin(); ite.valid();)
		{
			edge e = (*ite);
			ListIterator<edge> succ = ite.succ();

			// Assert that stack for anker nodes is not empty
			OGDF_ASSERT(!m_outgoingEdgesAnker[e]->empty());

			node nonWheelNode; // The node of Gcopy that does not correspond to cluster act
			if (act != m_wheelGraphNodes[e->source()])
				nonWheelNode = e->source();
			else {
				OGDF_ASSERT(act != m_wheelGraphNodes[e->target()]);
				nonWheelNode = e->target();
			}

			edge subGraphEdge = m_outgoingEdgesAnker[e]->popRet();
			node subGraphNode = subGraphEdge->opposite(t);

#ifdef OGDF_HEAVY_DEBUG
			debugTableOutgoingSubGraph2Gcopy[subGraphEdge] = e;
#endif

			rightKey = (*m_clusterPQContainer[act].m_edge2Key)[subGraphEdge];
			allOutgoing.pushBack(rightKey);
			if (leftKey)
			{
				SListPure<PlanarLeafKey<IndInfo*>*> pair;
				pair.pushBack(leftKey);
				pair.pushBack(rightKey);
#ifdef OGDF_DEBUG
				bool planar =
#endif
					T->Reduction(pair);
				// Assert that the Reduction did not fail
				OGDF_ASSERT(planar);
				T->PQTree<edge,IndInfo*,bool>::emptyAllPertinentNodes();
			}
			else
				firstEdge = subGraphEdge;

			leftKey = rightKey;

			// Assert that the anker node is a node
			// of the subgraph.
			OGDF_ASSERT(subGraphNode->graphOf() == subGraph);

			// Redirect the edge to the new node.
			// This keeps the embedding of Gcopy.
			if (nonWheelNode == e->source())
			{
				Gcopy.moveTarget(e, nodeTableSubGraph2Gcopy[subGraphNode]);

				if (subGraphEdge->source() == subGraphNode)
				{
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjSource()] = e->adjTarget();
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjTarget()] = e->adjSource();
				}
				else
				{
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjSource()] = e->adjSource();
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjTarget()] = e->adjTarget();
				}
			}
			else
			{
				Gcopy.moveSource(e,nodeTableSubGraph2Gcopy[subGraphNode]);

				if (subGraphEdge->target() == subGraphNode)
				{
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjSource()] = e->adjTarget();
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjTarget()] = e->adjSource();
				}
				else
				{
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjSource()] = e->adjSource();
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjTarget()] = e->adjTarget();
				}
			}

			ite = succ;
		}


		// Compute an embedding of the subgraph

		// Mark all leaves as relevant
#ifdef OGDF_DEBUG
		bool planar =
#endif
			T->Reduction(allOutgoing);
		// Assert that the Reduction did not fail
		OGDF_ASSERT(planar);

		// Stores for every node v the keys corresponding to the incoming edges of v
		NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > >* inLeaves
			= m_clusterPQContainer[act].m_inLeaves;

		// Stores for every node v the keys corresponding to the outgoing edges of v
		/*NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > >* outLeaves
			= m_clusterPQContainer[act].m_outLeaves;*/

		// Stores for every node v the sequence of incoming edges of v according
		// to the embedding
		NodeArray<SListPure<edge> >* frontier
			= m_clusterPQContainer[act].m_frontier;

		// Stores for every node v the nodes corresponding to the
		// opposed sink indicators found in the frontier of v.
		NodeArray<SListPure<node> >* opposed
			= m_clusterPQContainer[act].m_opposed;

		// Stores for every node v the nodes corresponding to the
		// opposed sink indicators found in the frontier of v.
		NodeArray<SListPure<node> >* nonOpposed
			= m_clusterPQContainer[act].m_nonOpposed;

		// Stores for every node the st-number
		NodeArray<int>* numbering = m_clusterPQContainer[act].m_numbering;

		// Stores for every st-Number the corresponding node
		Array<node>* tableNumber2Node = m_clusterPQContainer[act].m_tableNumber2Node;

		Array<bool> toReverse(1,(*numbering)[t],false);

		// Get necessary embedding information
		T->ReplaceRoot((*inLeaves)[t], (*frontier)[t], (*opposed)[t], (*nonOpposed)[t],t);


		// Compute a regular embedding of the biconnected component.

		// Reverse adjacency lists if necessary
		edge check = (*frontier)[t].front();

		// Check if the order of edges around t has to be reversed.
		if (firstEdge == check)
			toReverse[(*numbering)[t]] = true;

		int i;
		for (i = (*numbering)[t]; i >= 2; i--)
		{
			if (toReverse[i])
			{
				while (!(*nonOpposed)[(*tableNumber2Node)[i]].empty())
				{
					node v = (*nonOpposed)[(*tableNumber2Node)[i]].popFrontRet();
					OGDF_ASSERT(!toReverse[(*numbering)[v]]);
					toReverse[(*numbering)[v]] =  true;
				}
				(*frontier)[(*tableNumber2Node)[i]].reverse();
			}
			else
			{
				while (!(*opposed)[(*tableNumber2Node)[i]].empty())
				{
					node v = (*opposed)[(*tableNumber2Node)[i]].popFrontRet();
					OGDF_ASSERT(!toReverse[(*numbering)[v]]);
					toReverse[(*numbering)[v]] =  true;
				}
			}
			(*nonOpposed)[(*tableNumber2Node)[i]].clear();
			(*opposed)[(*tableNumber2Node)[i]].clear();
		}

#ifdef OGDF_HEAVY_DEBUG
		Logger::slout() << std::endl << "New Lists after Reversing " << std::endl;
		for (i = 1; i <= (*numbering)[t]; i++) {
			node v = (*tableNumber2Node)[i];
			Logger::slout() << "v = " << v << " : " << " ";
			for (edge e : (*frontier)[v])
				Logger::slout() << e << " ";
			Logger::slout() << std::endl;
		}
#endif

		// Compute the upward embedding

		NodeArray<SListPure<adjEntry> > biCompEmbedding(*subGraph);
		for (i = 1; i <= (*numbering)[t]; i++)
		{
			node v = (*tableNumber2Node)[i];
			while (!(*frontier)[v].empty())
			{
				edge e = (*frontier)[v].popFrontRet();
				biCompEmbedding[v].pushBack(
					(e->adjSource()->theNode() == v)? e->adjSource() : e->adjTarget());
			}
		}

		// Compute the entire embedding of the subGraph

		NodeArray<bool> mark(*subGraph,false);
		NodeArray<SListIterator<adjEntry> > adjMarker(*subGraph,nullptr);
		for (i = 1; i <= (*numbering)[t]; i++)
		{
			node v = (*tableNumber2Node)[i];
			adjMarker[v] = biCompEmbedding[v].begin();
		}
		entireEmbed(*subGraph, biCompEmbedding, adjMarker, mark, (*tableNumber2Node)[(*numbering)[t]]);


		// Sort the adjacency list of the new nodes in Gcopy
		// using the entire embedding of subGraph

		NodeArray<SListPure<adjEntry> >	embeddingGcopy(Gcopy);

		// Copy Embedding of biconnected Componts with no outging edges first

		for(node v : subGraph->nodes)
		{
			for (adjEntry ae : (*embedding)[v])
				embeddingGcopy[nodeTableSubGraph2Gcopy[v]].pushBack(
					tableAdjEntrySubGraph2Gcopy[ae]);
		}

		// Copy Embedding of the biconnected componts
		// with outging edges. Don't add the outgoing edges

		for (i = 1; i < (*numbering)[t]; i++)
		{
			node v = (*tableNumber2Node)[i];
			SListIterator<adjEntry> it;
			while (!biCompEmbedding[v].empty())
			{
				adjEntry adjNext = biCompEmbedding[v].popFrontRet();
				(*embedding)[v].pushBack(adjNext);
				embeddingGcopy[nodeTableSubGraph2Gcopy[v]].pushBack(
					tableAdjEntrySubGraph2Gcopy[adjNext]);
			}
		}

		for(node v : subGraph->nodes)
			if (v != t)
				Gcopy.sort(nodeTableSubGraph2Gcopy[v], embeddingGcopy[nodeTableSubGraph2Gcopy[v]]);


		// Sort the adjacency list of the new cluster nodes in Gcopy
		// using the adjacency list of t

		SListPure<adjEntry> embeddingClusterList;
		while (!biCompEmbedding[t].empty())
		{
			adjEntry adjNext = biCompEmbedding[t].popFrontRet();
			(*embedding)[t].pushBack(adjNext);
			// Choose the twin of adjNext, since adjNext is associated with t
			// which is the outside of the cluster.
			embeddingClusterList.pushFront(tableAdjEntrySubGraph2Gcopy[adjNext->twin()]);
		}

		Ccopy.makeAdjEntries(newCluster,embeddingClusterList.begin());


		// Delete the wheelGraph nodes from Gcopy
		while (!replaceNodes.empty())
		{
			node v = replaceNodes.popFrontRet();
#if 0
			Ccopy.unassignNode(v);
#endif
			Gcopy.delNode(v);
		}

		OGDF_ASSERT(Gcopy.representsCombEmbedding());

		delete m_clusterEmbedding[act];
		delete m_clusterSubgraphHubs[act];
		delete m_clusterSubgraphWheelGraph[act];
		delete m_clusterNodeTableNew2Orig[act];
		delete m_clusterOutgoingEdgesAnker[act];

		m_clusterPQContainer[act].Cleanup();

		hubControl(Gcopy,m_currentHubs);

	}

	for(edge e : Gcopy.edges)
	{
		delete m_outgoingEdgesAnker[e];
	}

	delete m_clusterSubgraphHubs[root];
	delete m_clusterSubgraphWheelGraph[root];
	delete m_clusterOutgoingEdgesAnker[root];

	Ccopy.adjAvailable(true);
}

//Checks if the algorithm is applicable (input is c-connected and planar) and
//then calls the planarity test method
bool CconnectClusterPlanarEmbed::preProcess(ClusterGraph &Ccopy,Graph &Gcopy)
{
	m_errorCode = ErrorCode::none;
	if (!isCConnected(Ccopy))
	{
		m_errorCode = ErrorCode::nonCConnected;
		return false;
	}

	if (!isPlanar(Ccopy))
	{
		m_errorCode = ErrorCode::nonPlanar;
		return false;
	}

	cluster c;

	SListPure<node> selfLoops;
	makeLoopFree(Gcopy,selfLoops);

	c = Ccopy.rootCluster();

	bool cPlanar = planarityTest(Ccopy,c,Gcopy);

	return cPlanar;
}

// Recursive call for testing Planarity of a Cluster
bool CconnectClusterPlanarEmbed::planarityTest(
	ClusterGraph &Ccopy,
	const cluster act,
	Graph &Gcopy)
{
	cluster origOfAct = m_clusterTableCopy2Orig[act];

	// Test children first
	if (!safeTestForEach(act->children, [&](cluster child) {
		return planarityTest(Ccopy, child, Gcopy);
	})) {
		return false;
	}

	m_callStack.push(origOfAct);

	// Get induced subgraph of cluster act and test it for planarity

#ifdef OGDF_HEAVY_DEBUG
		Logger::slout() << std::endl << std::endl << "Testing cluster " << origOfAct->index() << std::endl;
#endif

	List<node> subGraphNodes;
	for (node s : act->nodes)
		subGraphNodes.pushBack(s);

	Graph			*subGraph =  new Graph();
	NodeArray<node> nodeTableOrig2New;
	EdgeArray<edge> edgeTableOrig2New;
	inducedSubGraph(Gcopy, subGraphNodes.begin(), (*subGraph), nodeTableOrig2New, edgeTableOrig2New);
	NodeArray<node> nodeTableNew2Orig((*subGraph),nullptr);

	// Necessary only for root cluster.
	EdgeArray<edge> edgeTableNew2Orig(*subGraph,nullptr);

	if (act != Ccopy.rootCluster())
	{
		m_clusterSubgraph[origOfAct]			= subGraph;
		m_clusterNodeTableNew2Orig[origOfAct]	= new NodeArray<node>((*subGraph),nullptr);
		m_clusterSubgraphHubs[origOfAct]		= new NodeArray<bool>((*subGraph),0);
		m_clusterSubgraphWheelGraph[origOfAct]	= new NodeArray<cluster>((*subGraph),nullptr);
		m_clusterOutgoingEdgesAnker[origOfAct]  = new EdgeArray<ArrayBuffer<edge>*>((*subGraph),nullptr);
		for (node w : act->nodes)
		{
			(*m_clusterNodeTableNew2Orig[origOfAct])[nodeTableOrig2New[w]]
				= m_nodeTableCopy2Orig[w];
		}
		for(edge e : Gcopy.edges)
		{
			if (edgeTableOrig2New[e] && m_outgoingEdgesAnker[e])
				(*m_clusterOutgoingEdgesAnker[origOfAct])[edgeTableOrig2New[e]]
					= m_outgoingEdgesAnker[e];
		}
	}
	else
	{
		m_clusterSubgraph[origOfAct]			= &Gcopy;
		m_clusterSubgraphHubs[origOfAct]		= new NodeArray<bool>(Gcopy,0);
		m_clusterSubgraphWheelGraph[origOfAct]	= new NodeArray<cluster>(Gcopy,nullptr);
		m_clusterOutgoingEdgesAnker[origOfAct]  = new EdgeArray<ArrayBuffer<edge>*>(Gcopy,nullptr);
		for (node w : act->nodes)
		{
			node ttt = nodeTableOrig2New[w];
			nodeTableNew2Orig[ttt] = w;
		}
		for(edge e : Gcopy.edges)
		{
			edgeTableNew2Orig[edgeTableOrig2New[e]] = e;
			if (m_outgoingEdgesAnker[e])
				(*m_clusterOutgoingEdgesAnker[origOfAct])[e]
					= m_outgoingEdgesAnker[e];
		}
	}

	// Introduce super sink and add edges corresponding
	// to outgoing edges of the cluster

	node superSink = subGraph->newNode();
	EdgeArray<node> outgoingTable((*subGraph),nullptr);

	for (node w : act->nodes)
	{
#if 0
		adjEntry adj = w->firstAdj();
#endif
		for(adjEntry adj : w->adjEntries)
		{
			edge e = adj->theEdge();
			edge cor = nullptr;
			if (nodeTableOrig2New[e->source()] == nullptr)
				// edge is connected to a node outside the cluster
			{
				cor = subGraph->newEdge(nodeTableOrig2New[e->target()],superSink);
				outgoingTable[cor] = e->source();
				if (m_outgoingEdgesAnker[e])
					(*m_clusterOutgoingEdgesAnker[origOfAct])[cor]
						= m_outgoingEdgesAnker[e];
			}
			else if (nodeTableOrig2New[e->target()] == nullptr) // dito
			{
				cor = subGraph->newEdge(nodeTableOrig2New[e->source()],superSink);
				outgoingTable[cor] = e->target();
				if (m_outgoingEdgesAnker[e])
					(*m_clusterOutgoingEdgesAnker[origOfAct])[cor]
						= m_outgoingEdgesAnker[e];			}

			// else edge connects two nodes of the cluster
		}
	}
	if (superSink->degree() == 0) // root cluster is not connected to outside clusters
	{
		subGraph->delNode(superSink);
		superSink = nullptr;
	}
	else
		m_clusterSuperSink[origOfAct] = superSink;

#ifdef OGDF_CPLANAR_DEBUG_OUTPUT
		string filename = string("Ccopy") + to_string(origOfAct->index()) + ".gml";
		GraphIO::write(*subGraph, filename, GraphIO::writeGML);
#endif

	bool cPlanar = preparation((*subGraph),origOfAct,superSink);

	if (cPlanar && act != Ccopy.rootCluster())
	{
		// Remove induced subgraph and the cluster act.
		// Replace it by a wheel graph
		while (!subGraphNodes.empty())
		{
			node w = subGraphNodes.popFrontRet();
			if (m_currentHubs[w])
				(*m_clusterSubgraphHubs[origOfAct])[nodeTableOrig2New[w]]
					= true;
			if (m_wheelGraphNodes[w])
				(*m_clusterSubgraphWheelGraph[origOfAct])[nodeTableOrig2New[w]]
					= m_wheelGraphNodes[w];

#if 0
			Ccopy.unassignNode(w);
#endif
			Gcopy.delNode(w);
		}

		cluster parent = act->parent();

		if (superSink && m_clusterPQContainer[origOfAct].m_T)
			constructWheelGraph(Ccopy,Gcopy,parent,origOfAct,
								m_clusterPQContainer[origOfAct].m_T,
								outgoingTable,superSink);

		m_clusterTableOrig2Copy[origOfAct] = nullptr;
		Ccopy.delCluster(act);
	}

	else if (cPlanar && act == Ccopy.rootCluster())
	{
		for(node w : Gcopy.nodes)
		{
			if (m_currentHubs[w])
				(*m_clusterSubgraphHubs[origOfAct])[w] = true;
			if (m_wheelGraphNodes[w])
				(*m_clusterSubgraphWheelGraph[origOfAct])[w] = m_wheelGraphNodes[w];
		}

		for(node w : subGraph->nodes)
			subGraph->sort(w,(*m_clusterEmbedding[origOfAct])[w]);

		for(node w : subGraph->nodes)
		{
			node originalOfw = nodeTableNew2Orig[w];

			SListPure<adjEntry> adjList;

			for(adjEntry a : w->adjEntries)
			{
				edge e = edgeTableNew2Orig[a->theEdge()];
				adjEntry adj = (e->adjSource()->theNode() == originalOfw)?
								e->adjSource() : e->adjTarget();
				adjList.pushBack(adj);
			}

			Gcopy.sort(originalOfw,adjList);
		}

		// Test if embedding was determined correctly.
		OGDF_ASSERT(subGraph->representsCombEmbedding());

		edgeTableNew2Orig.init();
		outgoingTable.init();
		nodeTableNew2Orig.init();
		delete m_clusterEmbedding[origOfAct];
		m_clusterEmbedding[origOfAct] = nullptr;
		delete subGraph;

	}

	else if (!cPlanar && act == Ccopy.rootCluster())
	{
		edgeTableNew2Orig.init();
		outgoingTable.init();
		nodeTableNew2Orig.init();
		delete m_clusterEmbedding[origOfAct];
		m_clusterEmbedding[origOfAct] = nullptr;
		delete subGraph;
	}

	if (!cPlanar)
	{
		m_errorCode = ErrorCode::nonCPlanar;
	}

	return cPlanar;
}

// Prepare planarity test for one cluster
bool CconnectClusterPlanarEmbed::preparation(
	Graph &subGraph,
	const cluster origCluster,
	node superSink)
{
	int  bcIdSuperSink = -1; // ID of biconnected component that contains superSink
							 // Initialization with -1 necessary for assertion
	bool cPlanar = true;

	NodeArray<node> tableNodesSubGraph2BiComp(subGraph,nullptr);
	EdgeArray<edge> tableEdgesSubGraph2BiComp(subGraph,nullptr);
	NodeArray<bool> mark(subGraph,0);

	EdgeArray<int> componentID(subGraph);

	// Generate datastructure for embedding, even if it is left empty.
	// Embedding either contains
	//		Embedding of the root cluster
	// or
	//		Partial Embedding of the biconnected components not having
	//		outgoing edges.

	NodeArray<SListPure<adjEntry> >
		*entireEmbedding = new NodeArray<SListPure<adjEntry> >(subGraph);
	m_clusterEmbedding[origCluster] = entireEmbedding;

	// Determine Biconnected Components
	int bcCount = biconnectedComponents(subGraph,componentID);

	// Determine edges per biconnected component
	Array<SList<edge> > blockEdges(0,bcCount-1);
	for(edge e : subGraph.edges)
	{
		blockEdges[componentID[e]].pushFront(e);
	}

	// Determine nodes per biconnected component.
	Array<SList<node> > blockNodes(0,bcCount-1);
	for (int i = 0; i < bcCount; i++)
	{
		for (edge e : blockEdges[i])
		{
			if (!mark[e->source()])
			{
				blockNodes[i].pushBack(e->source());
				mark[e->source()] = true;
			}
			if (!mark[e->target()])
			{
				blockNodes[i].pushBack(e->target());
				mark[e->target()] = true;
			}
		}

		if (superSink && mark[superSink]) {
			OGDF_ASSERT(bcIdSuperSink == -1);
			bcIdSuperSink = i;
		}

		for (node v : blockNodes[i])
		{
			if (mark[v])
				mark[v] = false;
			else {
				OGDF_ASSERT(mark[v]); // v has been placed two times on the list.
			}
		}
	}

	// Perform Planarity Test for every biconnected component

	if (bcCount == 1)
	{
		// Compute st-numbering
		NodeArray<int> numbering(subGraph,0);
#ifdef OGDF_HEAVY_DEBUG
		int n =
#endif
		superSink ? computeSTNumbering(subGraph, numbering, nullptr, superSink)
		          : computeSTNumbering(subGraph, numbering);
		OGDF_HEAVY_ASSERT(isSTNumbering(subGraph, numbering, n));

		EdgeArray<edge> tableEdgesBiComp2SubGraph(subGraph,nullptr);
		NodeArray<node> tableNodesBiComp2SubGraph(subGraph,nullptr);
		for(edge e : subGraph.edges)
			tableEdgesBiComp2SubGraph[e] = e;
		for(node v : subGraph.nodes)
			tableNodesBiComp2SubGraph[v] = v;

		// Initialize the container class for storing all information
		// if it does not belong to the root cluster.
		if (bcIdSuperSink == 0)
			m_clusterPQContainer[origCluster].init(&subGraph);

		cPlanar = doEmbed(
			&subGraph,
			numbering,
			origCluster,
			superSink,
			subGraph,
			tableEdgesBiComp2SubGraph,
			tableEdgesBiComp2SubGraph,
			tableNodesBiComp2SubGraph);

		// Do not save the embedding of the subgraph. It is not complete.
		if (bcIdSuperSink == -1)
		{
			// The root cluster is embedded.
			// Gather the embeddding of the biconnected graph, if it belongs to
			// the root cluster.
			// The embedding of the subgraph is saved, as it is the root cluster graph.
			for(node v : subGraph.nodes)
			{
				for(adjEntry a : v->adjEntries)
					(*entireEmbedding)[v].pushBack(a);
			}
		}

	}
	else
	{
		for (int i = 0; i < bcCount; i++)
		{
			Graph *biCompOfSubGraph = new Graph();

			for (node v : blockNodes[i])
			{
				node w = biCompOfSubGraph->newNode();
				tableNodesSubGraph2BiComp[v] = w;
			}

			NodeArray<node> tableNodesBiComp2SubGraph(*biCompOfSubGraph,nullptr);
			for (node v : blockNodes[i])
				tableNodesBiComp2SubGraph[tableNodesSubGraph2BiComp[v]] = v;

			for (edge e : blockEdges[i])
			{
				edge f = biCompOfSubGraph->newEdge(
					tableNodesSubGraph2BiComp[e->source()], tableNodesSubGraph2BiComp[e->target()]);
				tableEdgesSubGraph2BiComp[e] = f;
			}

			EdgeArray<edge> tableEdgesBiComp2SubGraph(*biCompOfSubGraph,nullptr);
			for (edge e : blockEdges[i])
				tableEdgesBiComp2SubGraph[tableEdgesSubGraph2BiComp[e]] = e;

			NodeArray<int> numbering(*biCompOfSubGraph,0);
			if (bcIdSuperSink == i) {
#ifdef OGDF_HEAVY_DEBUG
				int n =
#endif
				computeSTNumbering(*biCompOfSubGraph, numbering, nullptr, tableNodesSubGraph2BiComp[superSink]);
				OGDF_HEAVY_ASSERT(isSTNumbering(*biCompOfSubGraph, numbering, n));

				// Initialize the container class for storing all information
				m_clusterPQContainer[origCluster].init(&subGraph);

				cPlanar = doEmbed(
					biCompOfSubGraph,
					numbering,
					origCluster,
					tableNodesSubGraph2BiComp[superSink],
					subGraph,
					tableEdgesBiComp2SubGraph,
					tableEdgesSubGraph2BiComp,
					tableNodesBiComp2SubGraph);
			} else {
#ifdef OGDF_HEAVY_DEBUG
				int n =
#endif
				computeSTNumbering(*biCompOfSubGraph, numbering);
				OGDF_HEAVY_ASSERT(isSTNumbering(*biCompOfSubGraph, numbering, n));
				cPlanar = doEmbed(
					biCompOfSubGraph,
					numbering,
					origCluster,
					nullptr,
					subGraph,
					tableEdgesBiComp2SubGraph,
					tableEdgesSubGraph2BiComp,
					tableNodesBiComp2SubGraph);
			}

			if (!cPlanar)
			{
				numbering.init();
				tableEdgesBiComp2SubGraph.init();
				tableNodesBiComp2SubGraph.init();
				delete biCompOfSubGraph;
				break;
			}

			if (bcIdSuperSink == -1)
			{
				// The root cluster is embedded.
				// Gather the embedding of the biconnected graph, if it belongs to
				// the root cluster.
				// The embedding of the subgraph is saved, as it is the root cluster graph.
				for(node v : biCompOfSubGraph->nodes)
				{
					node w = tableNodesBiComp2SubGraph[v];
					for(adjEntry a : v->adjEntries)
					{
						edge e = tableEdgesBiComp2SubGraph[a->theEdge()];
						adjEntry adj = (e->adjSource()->theNode() == w)?
										e->adjSource() : e->adjTarget();
						(*entireEmbedding)[w].pushBack(adj);
					}
				}
			}
			else if (bcIdSuperSink != i)
			{
				// A non root cluster is embedded.
				// Gather the embeddings of the biconnected components
				// that do not have outgoing edges of the cluster.
				for(node v : biCompOfSubGraph->nodes)
				{
					node w = tableNodesBiComp2SubGraph[v];
					for(adjEntry a : v->adjEntries)
					{
						edge e = tableEdgesBiComp2SubGraph[a->theEdge()];
						adjEntry adj = (e->adjSource()->theNode() == w)?
										e->adjSource() : e->adjTarget();
						(*entireEmbedding)[w].pushBack(adj);
					}
				}

			}
			numbering.init();
			tableEdgesBiComp2SubGraph.init();
			tableNodesBiComp2SubGraph.init();
			delete biCompOfSubGraph;
		}

		// m_clusterEmbedding[origCluster] now contains the (partial) embedding
		// of all biconnected components that do not have outgoing edges
		// of the cluster origCluster.
	}

	return cPlanar;

}

// Performs a planarity test on a biconnected component
// of subGraph and embedds it planar.
// numbering contains an st-numbering of the component.
bool CconnectClusterPlanarEmbed::doEmbed(
	Graph *biconComp,
	NodeArray<int>  &numbering,
	const cluster origCluster,
	node superSink,
	Graph &subGraph,
	EdgeArray<edge> &tableEdgesBiComp2SubGraph,
	EdgeArray<edge> &tableEdgesSubGraph2BiComp,
	NodeArray<node> &tableNodesBiComp2SubGraph)
{
	bool cPlanar = true;

	// Definition
	// incoming edge of v: an edge e = (v,w) with number(v) < number(w)

	// Stores for every node v the keys corresponding to the incoming edges of v
	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > inLeaves(*biconComp);

	// Stores for every node v the keys corresponding to the outgoing edges of v
	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > outLeaves(*biconComp);

	// Stores for every node v the sequence of incoming edges of v according
	// to the embedding
	NodeArray<SListPure<edge> > frontier(*biconComp);

	// Stores for every node v the nodes corresponding to the
	// opposed sink indicators found in the frontier of v.
	NodeArray<SListPure<node> > opposed(*biconComp);

	// Stores for every node v the nodes corresponding to the
	// non opposed sink indicators found in the frontier of v.
	NodeArray<SListPure<node> > nonOpposed(*biconComp);

	// Stores for every st-Number the corresponding node
	Array<node> tableNumber2Node(biconComp->numberOfNodes()+1);

	Array<bool> toReverse(1,biconComp->numberOfNodes()+1,false);

	PlanarLeafKey<IndInfo*>* stEdgeLeaf = nullptr;

	for(node v : biconComp->nodes)
	{
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();

			if (numbering[adj->twinNode()] > numbering[v])
			{
				PlanarLeafKey<IndInfo*>* L = new PlanarLeafKey<IndInfo*>(e);
				inLeaves[v].pushFront(L);
				if (numbering[v] == 1 && numbering[e->opposite(v)])
					stEdgeLeaf = L;
			}
		}
		tableNumber2Node[numbering[v]] = v;
	}

	for(node v : biconComp->nodes)
	{
		for (PlanarLeafKey<IndInfo*>* L : inLeaves[v])
		{
			outLeaves[L->userStructKey()->opposite(v)].pushFront(L);
		}
	}

	EmbedPQTree* T = new EmbedPQTree();

	T->Initialize(inLeaves[tableNumber2Node[1]]);

	for (int i = 2; i < biconComp->numberOfNodes(); i++)
	{
		if (T->Reduction(outLeaves[tableNumber2Node[i]]))
		{
			T->ReplaceRoot(
				inLeaves[tableNumber2Node[i]],
				frontier[tableNumber2Node[i]],
				opposed[tableNumber2Node[i]],
				nonOpposed[tableNumber2Node[i]],
				tableNumber2Node[i]);
			T->emptyAllPertinentNodes();
		}
		else
		{
			cPlanar = false;
			break;
		}
	}

	if (cPlanar && superSink)
	{
		// The tested component contains the outgoing edges
		// of the cluster.

		// Keep the PQTree to construct a Wheelgraph
		// Replace the edge stored in the keys of T
		// by the original edges.
		// Necessary, since the edges currently in T
		// correspond to a graph that mirrors a biconnected
		// component and thus is deallocated

		// For embedding the graph, we need to keep the
		// PQTree as well.

		// Replace the edge stored in the keys of T
		// by the original edges.


		// All information that we keep is dependend on subGraph.
		// Translate the information back from biconComp to subGraph.

		m_clusterPQContainer[origCluster].m_superSink
			= tableNodesBiComp2SubGraph[superSink];

		for(node v : biconComp->nodes)
		{
			// Replace the edge stored in the every key used for constructing T
			// by the original edges.
			// This implicity replaces the keys at the leaves and at inLeaves.

			node orig = tableNodesBiComp2SubGraph[v];

			// Assert that m_outLeaves is empty
			OGDF_ASSERT((*m_clusterPQContainer[origCluster].m_outLeaves)[orig].empty());
			for (PlanarLeafKey<IndInfo*>* key : outLeaves[v])
			{
				key->m_userStructKey = tableEdgesBiComp2SubGraph[key->m_userStructKey];
				(*m_clusterPQContainer[origCluster].m_edge2Key)[key->m_userStructKey] = key;
				(*m_clusterPQContainer[origCluster].m_outLeaves)[orig].pushBack(key);
			}

			// Assert that m_inLeaves is empty
			OGDF_ASSERT((*m_clusterPQContainer[origCluster].m_inLeaves)[orig].empty());
			for (PlanarLeafKey<IndInfo*>* key : inLeaves[v])
			{
				(*m_clusterPQContainer[origCluster].m_inLeaves)[orig].pushBack(key);
			}

			// Replace the nodes stored in the lists opposed and nonOpposed
			// by the original nodes

			// Assert that m_opposed and m_nonOpposed are empty
			OGDF_ASSERT((*m_clusterPQContainer[origCluster].m_opposed)[orig].empty());
			OGDF_ASSERT((*m_clusterPQContainer[origCluster].m_nonOpposed)[orig].empty());

			for (node u : nonOpposed[v])
			{
				node w = tableNodesBiComp2SubGraph[u];
				(*m_clusterPQContainer[origCluster].m_nonOpposed)[orig].pushBack(w);
			}
			for (node u : opposed[v])
			{
				node w = tableNodesBiComp2SubGraph[u];
				(*m_clusterPQContainer[origCluster].m_opposed)[orig].pushBack(w);
			}

			(*m_clusterPQContainer[origCluster].m_numbering)[orig] = numbering[v];
			(*m_clusterPQContainer[origCluster].m_tableNumber2Node)[numbering[v]] = orig;

			// Replace the edges stored in frontier
			// by the original edges of subgraph.

			OGDF_ASSERT((*m_clusterPQContainer[origCluster].m_frontier)[orig].empty());
			for (edge ei : frontier[v])
			{
				edge e = tableEdgesBiComp2SubGraph[ei];
				(*m_clusterPQContainer[origCluster].m_frontier)[orig].pushBack(e);
			}
		}

		m_clusterPQContainer[origCluster].m_T = T;
		m_clusterPQContainer[origCluster].m_stEdgeLeaf = stEdgeLeaf;
		SListPure<PQBasicKey<edge,IndInfo*,bool>*> leafKeys;
		T->getFront(T->root(),leafKeys);
		for (PQBasicKey<edge, IndInfo*, bool> *key : leafKeys)
		{
			if (key->nodePointer()->status() == PQNodeRoot::PQNodeStatus::Indicator)
			{
				node ofInd = key->nodePointer()->getNodeInfo()->userStructInfo()->getAssociatedNode();
				key->nodePointer()->getNodeInfo()->userStructInfo()->resetAssociatedNode(tableNodesBiComp2SubGraph[ofInd]);
			}
		}
	}
	else if (cPlanar)
	{
		// The tested component does not contain outgoing edges
		// of the cluster.
		// Compute a regular embedding of the biconnected component.
		int i = biconComp->numberOfNodes();
		if (T->Reduction(outLeaves[tableNumber2Node[i]]))
		{
			T->ReplaceRoot(
				inLeaves[tableNumber2Node[i]],
				frontier[tableNumber2Node[i]],
				opposed[tableNumber2Node[i]],
				nonOpposed[tableNumber2Node[i]],
				tableNumber2Node[i]);
		}
		delete T;
	}

	// Cleanup
	if (!origCluster || !superSink || !cPlanar) {
		// Do not cleanup information of component
		// with outgoing edges.
		for(node v : biconComp->nodes)
		{
			if (v != superSink || !cPlanar)
			{
				while (!outLeaves[v].empty())
				{
					PlanarLeafKey<IndInfo*>* L = outLeaves[v].popFrontRet();
					delete L;
				}
			}
		}
	}
	if (!cPlanar)
		delete T;

	if (cPlanar && (!origCluster || !superSink))
	{
		// The tested component does not contain outgoing edges
		// of the cluster.
		// Compute a regular embedding of the biconnected component.

		// Reverse adjacency lists if necessary
		// This gives an upward embedding
		for (int i = biconComp->numberOfNodes(); i >= 2; i--)
		{
			if (toReverse[i])
			{
				while (!nonOpposed[tableNumber2Node[i]].empty())
				{
					node v = nonOpposed[tableNumber2Node[i]].popFrontRet();
					OGDF_ASSERT(!toReverse[numbering[v]]);
					toReverse[numbering[v]] =  true;
				}
				frontier[tableNumber2Node[i]].reverse();
			}
			else
			{
				while (!opposed[tableNumber2Node[i]].empty())
				{
					node v = opposed[tableNumber2Node[i]].popFrontRet();
					OGDF_ASSERT(!toReverse[numbering[v]]);
					toReverse[numbering[v]] =  true;
				}
			}
			nonOpposed[tableNumber2Node[i]].clear();
			opposed[tableNumber2Node[i]].clear();
		}

		// Compute the entire embedding
		NodeArray<SListPure<adjEntry> > entireEmbedding(*biconComp);
		for(node v : biconComp->nodes)
		{
			while (!frontier[v].empty())
			{
				edge e = frontier[v].popFrontRet();
				entireEmbedding[v].pushBack(
					(e->adjSource()->theNode() == v)? e->adjSource() : e->adjTarget());
			}
		}

		NodeArray<bool> mark(*biconComp,false);
		NodeArray<SListIterator<adjEntry> > adjMarker(*biconComp,nullptr);
		for(node v : biconComp->nodes)
			adjMarker[v] = entireEmbedding[v].begin();
		entireEmbed(*biconComp, entireEmbedding, adjMarker, mark, tableNumber2Node[biconComp->numberOfNodes()]);

		for(node v : biconComp->nodes)
			biconComp->sort(v,entireEmbedding[v]);

		// Test if embedding was determined correctly.
		OGDF_ASSERT(biconComp->representsCombEmbedding());
	}

	return cPlanar;
}

// Used by doEmbed. Computes an entire embedding from an
// upward embedding.
void CconnectClusterPlanarEmbed::entireEmbed(
	Graph &biconComp,
	NodeArray<SListPure<adjEntry> > &entireEmbedding,
	NodeArray<SListIterator<adjEntry> > &adjMarker,
	NodeArray<bool> &mark,
	node v)
{
	mark[v] = true;
	SListIterator<adjEntry> it;
	for (it = adjMarker[v]; it.valid(); ++it)
	{
		adjEntry a = *it;
		edge e = a->theEdge();
		adjEntry adj = (e->adjSource()->theNode() == v)?
						e->adjTarget() : e->adjSource();
		node w = adj->theNode();
		entireEmbedding[w].pushFront(adj);
		if (!mark[w])
			entireEmbed(biconComp,entireEmbedding,adjMarker,mark,w);
	}
}

void CconnectClusterPlanarEmbed::prepareParallelEdges(Graph &G)
{
	// Stores for one reference edge all parallel edges.
	m_parallelEdges.init(G);
	// Is true for any multiedge, except for the reference edge.
	m_isParallel.init(G,false);
	getParallelFreeUndirected(G,m_parallelEdges);
	m_parallelCount = 0;
	for(edge e : G.edges)
	{
		if (!m_parallelEdges[e].empty())
		{
			ListIterator<edge> it;
			for (edge ei : m_parallelEdges[e])
			{
				m_isParallel[ei] = true;
				m_parallelCount++;
			}
		}
	}
}

void CconnectClusterPlanarEmbed::constructWheelGraph(ClusterGraph &Ccopy,
												Graph &Gcopy,
												cluster &parent,
												cluster &origOfAct,
												EmbedPQTree* T,
												EdgeArray<node> &outgoingTable,
												node superSink)
{
#ifdef OGDF_DEBUG
	Ccopy.consistencyCheck();
#endif
	PQNode<edge,IndInfo*,bool>* root = T->root();
	PQNode<edge,IndInfo*,bool>*  checkNode = nullptr;

	Queue<PQNode<edge,IndInfo*,bool>*> treeNodes;
	treeNodes.append(root);

	node correspond = Gcopy.newNode(); // Corresponds to the root node.
									   // root node is either a Leaf or a P-node
	m_nodeTableCopy2Orig[correspond] = nullptr; // Node does not correspond to a node
										 // in the original graph
	m_wheelGraphNodes[correspond] = origOfAct;
	Ccopy.reassignNode(correspond,parent);

	Queue<node> graphNodes;
	graphNodes.append(correspond);

	node hub;
	node next = nullptr;
	node pre;
	node newNode; // corresponds to anchor of a hub or a cut node

	while (!treeNodes.empty())
	{
		checkNode = treeNodes.pop();
		correspond = graphNodes.pop();

		PQNode<edge,IndInfo*,bool>*  firstSon  = nullptr;
		PQNode<edge,IndInfo*,bool>*  nextSon   = nullptr;
		PQNode<edge,IndInfo*,bool>*  oldSib    = nullptr;
		PQNode<edge,IndInfo*,bool>*  holdSib   = nullptr;

		if (checkNode->type() == PQNodeRoot::PQNodeType::PNode)
		{
			// correspond is a cut node

			OGDF_ASSERT(checkNode->referenceChild());
			firstSon = checkNode->referenceChild();

			if (firstSon->type() != PQNodeRoot::PQNodeType::Leaf)
			{
				treeNodes.append(firstSon);
				newNode = Gcopy.newNode();
				m_nodeTableCopy2Orig[newNode] = nullptr;
				m_wheelGraphNodes[newNode] = origOfAct;
				Ccopy.reassignNode(newNode,parent);
				graphNodes.append(newNode);
				Gcopy.newEdge(correspond,newNode);
			}
			else
			{
				// insert Edge to the outside
				PQLeaf<edge,IndInfo*,bool>* leaf =
					(PQLeaf<edge,IndInfo*,bool>*) firstSon;
				edge f = leaf->getKey()->m_userStructKey;
#if 0
				node x = outgoingTable[f];
#endif
				edge newEdge = Gcopy.newEdge(correspond,outgoingTable[f]);

				if ((*m_clusterOutgoingEdgesAnker[origOfAct])[f])
				{
					m_outgoingEdgesAnker[newEdge]
						= (*m_clusterOutgoingEdgesAnker[origOfAct])[f];
				}
				else
					m_outgoingEdgesAnker[newEdge] = new ArrayBuffer<edge>;
				m_outgoingEdgesAnker[newEdge]->push(f);
			}

			nextSon = firstSon->getNextSib(oldSib);
			oldSib = firstSon;
			pre = next;
			while (nextSon && nextSon != firstSon)
			{
				if (nextSon->type() != PQNodeRoot::PQNodeType::Leaf)
				{
					treeNodes.append(nextSon);
					newNode = Gcopy.newNode();  // new node corresponding to anchor or cutnode
					m_nodeTableCopy2Orig[newNode] = nullptr;
					m_wheelGraphNodes[newNode] = origOfAct;
					Ccopy.reassignNode(newNode,parent);
					graphNodes.append(newNode);
					Gcopy.newEdge(correspond,newNode);
				}
				else
				{
					// insert Edge to the outside
					PQLeaf<edge,IndInfo*,bool>* leaf =
						(PQLeaf<edge,IndInfo*,bool>*) nextSon;
					edge f = leaf->getKey()->m_userStructKey;
#if 0
					node x = outgoingTable[f];
#endif
					edge newEdge = Gcopy.newEdge(correspond,outgoingTable[f]);

					if ((*m_clusterOutgoingEdgesAnker[origOfAct])[f])
					{
						m_outgoingEdgesAnker[newEdge]
							= (*m_clusterOutgoingEdgesAnker[origOfAct])[f];
					}
					else
						m_outgoingEdgesAnker[newEdge] = new ArrayBuffer<edge>;
					m_outgoingEdgesAnker[newEdge]->push(f);
				}
				holdSib = nextSon->getNextSib(oldSib);
				oldSib = nextSon;
				nextSon = holdSib;
			}

		}
		else if (checkNode->type() == PQNodeRoot::PQNodeType::QNode)
		{

			// correspond is the achor of a hub
			OGDF_ASSERT(T->scanLeftEndmost(checkNode));
			firstSon = T->scanLeftEndmost(checkNode);

			hub = Gcopy.newNode();
			m_nodeTableCopy2Orig[hub] = nullptr;
			m_currentHubs[hub] = true;
			m_wheelGraphNodes[hub] = origOfAct;
			Ccopy.reassignNode(hub,parent);

			Gcopy.newEdge(hub,correspond); // link achor and hub
			next = Gcopy.newNode();   // for first son
			m_nodeTableCopy2Orig[next] = nullptr;
			m_wheelGraphNodes[next] = origOfAct;
			Ccopy.reassignNode(next,parent);
			Gcopy.newEdge(hub,next);
			Gcopy.newEdge(correspond,next);

			if (firstSon->type() != PQNodeRoot::PQNodeType::Leaf)
			{
				treeNodes.append(firstSon);
				newNode = Gcopy.newNode();
				m_nodeTableCopy2Orig[newNode] = nullptr;
				m_wheelGraphNodes[newNode] = origOfAct;
				Ccopy.reassignNode(newNode,parent);
				graphNodes.append(newNode);
				Gcopy.newEdge(next,newNode);
			}
			else
			{
				// insert Edge to the outside
				PQLeaf<edge,IndInfo*,bool>* leaf =
					(PQLeaf<edge,IndInfo*,bool>*) firstSon;
				edge f = leaf->getKey()->m_userStructKey;
#if 0
				node x = outgoingTable[f];
#endif
				edge newEdge = Gcopy.newEdge(next,outgoingTable[f]);

				if ((*m_clusterOutgoingEdgesAnker[origOfAct])[f])
				{
					m_outgoingEdgesAnker[newEdge]
						= (*m_clusterOutgoingEdgesAnker[origOfAct])[f];
				}
				else
					m_outgoingEdgesAnker[newEdge] = new ArrayBuffer<edge>;
				m_outgoingEdgesAnker[newEdge]->push(f);
			}

			nextSon = T->scanNextSib(firstSon,oldSib);
			oldSib = firstSon;
			pre = next;
			while (nextSon)
			{
				next = Gcopy.newNode();
				m_nodeTableCopy2Orig[next] = nullptr;
				m_wheelGraphNodes[next] = origOfAct;
				Ccopy.reassignNode(next,parent);
				Gcopy.newEdge(hub,next);
				Gcopy.newEdge(pre,next);
				if (nextSon->type() != PQNodeRoot::PQNodeType::Leaf)
				{
					treeNodes.append(nextSon);
					newNode = Gcopy.newNode();  // new node corresponding to anchor or cutnode
					m_nodeTableCopy2Orig[newNode] = nullptr;
					m_wheelGraphNodes[newNode] = origOfAct;
					Ccopy.reassignNode(newNode,parent);
					graphNodes.append(newNode);

					Gcopy.newEdge(next,newNode);
				}
				else
				{
					// insert Edge to the outside
					PQLeaf<edge,IndInfo*,bool>* leaf =
						(PQLeaf<edge,IndInfo*,bool>*) nextSon;
					edge f = leaf->getKey()->m_userStructKey;
#if 0
					node x = outgoingTable[f];
#endif
					edge newEdge = Gcopy.newEdge(next,outgoingTable[f]);

					if ((*m_clusterOutgoingEdgesAnker[origOfAct])[f])
					{
						m_outgoingEdgesAnker[newEdge]
							= (*m_clusterOutgoingEdgesAnker[origOfAct])[f];
					}
					else
						m_outgoingEdgesAnker[newEdge] = new ArrayBuffer<edge>;
					m_outgoingEdgesAnker[newEdge]->push(f);
				}
				holdSib = T->scanNextSib(nextSon,oldSib);
				oldSib = nextSon;
				nextSon = holdSib;
				pre = next;

			}
			Gcopy.newEdge(next,correspond);
		}
	}

#ifdef OGDF_DEBUG
	Ccopy.consistencyCheck();
#endif
}

}
