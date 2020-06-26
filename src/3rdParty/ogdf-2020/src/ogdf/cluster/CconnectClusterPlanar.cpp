/** \file
 * \brief Implementation of cluster planarity tests and cluster
 * planar embedding for c-connected clustered graphs. Based on
 * the algorithm by Cohen, Feng and Eades which uses PQ-trees.
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

#include <ogdf/cluster/CconnectClusterPlanar.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/STNumbering.h>

namespace ogdf {

using namespace booth_lueker;

// Constructor
CconnectClusterPlanar::CconnectClusterPlanar()
{
	m_errorCode = ErrorCode::none;
}

// Tests if a ClusterGraph is C-planar
bool CconnectClusterPlanar::call(const ClusterGraph &C)
{
	Graph G;
	ClusterGraph Cp(C,G);
#ifdef OGDF_DEBUG
	Cp.consistencyCheck();
#endif
	OGDF_ASSERT(&G == &Cp.constGraph());

	m_clusterPQTree.init(Cp,nullptr);

	bool cPlanar = preProcess(Cp,G);

	m_parallelEdges.init();
	m_isParallel.init();
	m_clusterPQTree.init();

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
//				preparation(Graph  &G,cluster &cl)
//
//					foreach biconnected Component
//						doTest(Graph &G,NodeArray<int> &numbering,cluster &cl)
//

bool CconnectClusterPlanar::preProcess(ClusterGraph &C,Graph &G)
{
	if (!isCConnected(C))
	{
		m_errorCode = ErrorCode::nonCConnected;
		return false;
	}

	if (!isPlanar(C))
	{
		m_errorCode = ErrorCode::nonPlanar;
		return false;
	}

	cluster c;

	SListPure<node> selfLoops;
	makeLoopFree(G,selfLoops);

	c = C.rootCluster();

	bool cPlanar = planarityTest(C,c,G);

	return cPlanar;
}

// Recursive call for testing c-planarity of the clustered graph
// that is induced by cluster act
bool CconnectClusterPlanar::planarityTest(
	ClusterGraph &C,
	const cluster act,
	Graph &G)
{
	// Test children first
	if (!safeTestForEach(act->children, [&](cluster child) {
		return planarityTest(C, child, G);
	})) {
		return false;
	}

	// Get induced subgraph of cluster act and test it for planarity

	List<node> subGraphNodes;
	for (node s : act->nodes)
		subGraphNodes.pushBack(s);

	Graph subGraph;
	NodeArray<node> table;
	inducedSubGraph(G,subGraphNodes.begin(),subGraph,table);

	// Introduce super sink and add edges corresponding
	// to outgoing edges of the cluster

	node superSink = subGraph.newNode();
	EdgeArray<node> outgoingTable(subGraph,nullptr);

	for (node w : act->nodes)
	{
		for(adjEntry adj : w->adjEntries)
		{
			edge e = adj->theEdge();
			edge cor = nullptr;
			if (table[e->source()] == nullptr) // edge is connected to a node outside the cluster
			{
				cor = subGraph.newEdge(table[e->target()],superSink);
				outgoingTable[cor] = e->source();
			}
			else if (table[e->target()] == nullptr) // dito
			{
				cor = subGraph.newEdge(table[e->source()],superSink);
				outgoingTable[cor] = e->target();
			}

			// else edge connects two nodes of the cluster
		}
	}
	if (superSink->degree() == 0) // root cluster is not connected to outside clusters
	{
		subGraph.delNode(superSink);
		superSink = nullptr;
	}


	bool cPlanar = preparation(subGraph,act,superSink);


	if (cPlanar && act != C.rootCluster())
	{
		// Remove induced subgraph and the cluster act.
		// Replace it by a wheel graph
		while (!subGraphNodes.empty())
		{
			node w = subGraphNodes.popFrontRet();
#if 0
			C.unassignNode(w);
#endif
			G.delNode(w);
		}

		cluster parent = act->parent();

		if (superSink && m_clusterPQTree[act])
			constructWheelGraph(C,G,parent,m_clusterPQTree[act],outgoingTable);

		if (m_clusterPQTree[act] != nullptr) // if query necessary for clusters with just one child
		{
			m_clusterPQTree[act]->emptyAllPertinentNodes();
			delete m_clusterPQTree[act];
		}
		C.delCluster(act);

	} else if (!cPlanar) {
		m_errorCode = ErrorCode::nonCPlanar;
	}

	return cPlanar;
}

void CconnectClusterPlanar::constructWheelGraph(ClusterGraph &C,
												Graph &G,
												cluster &parent,
												PlanarPQTree* T,
												EdgeArray<node> &outgoingTable)
{
	const PQNode<edge,IndInfo*,bool>* root = T->root();
	const PQNode<edge,IndInfo*,bool>*  checkNode = nullptr;

	Queue<const PQNode<edge,IndInfo*,bool>*> treeNodes;
	treeNodes.append(root);

	node correspond = G.newNode(); // Corresponds to the root node.
								   // root node is either a Leaf or a P-node
	C.reassignNode(correspond,parent);

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
				newNode = G.newNode();
				C.reassignNode(newNode,parent);
				graphNodes.append(newNode);
				G.newEdge(correspond,newNode);
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
				G.newEdge(correspond,outgoingTable[f]);
				delete leaf->getKey();
			}

			nextSon = firstSon->getNextSib(oldSib);
			oldSib = firstSon;
			pre = next;
			while (nextSon && nextSon != firstSon)
			{
				if (nextSon->type() != PQNodeRoot::PQNodeType::Leaf)
				{
					treeNodes.append(nextSon);
					newNode = G.newNode(); // new node corresponding to anchor or cutnode
					C.reassignNode(newNode,parent);
					graphNodes.append(newNode);
					G.newEdge(correspond,newNode);
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
					G.newEdge(correspond,outgoingTable[f]);
					delete leaf->getKey();
				}
				holdSib = nextSon->getNextSib(oldSib);
				oldSib = nextSon;
				nextSon = holdSib;
			}

		}
		else if (checkNode->type() == PQNodeRoot::PQNodeType::QNode)
		{
			// correspond is the anchor of a hub
			OGDF_ASSERT(checkNode->getEndmost(PQNodeRoot::SibDirection::Left));
			firstSon = checkNode->getEndmost(PQNodeRoot::SibDirection::Left);

			hub = G.newNode();
			C.reassignNode(hub,parent);
			G.newEdge(hub,correspond); // link anchor and hub
			next = G.newNode();   // for first son
			C.reassignNode(next,parent);
			G.newEdge(hub,next);
			G.newEdge(correspond,next);

			if (firstSon->type() != PQNodeRoot::PQNodeType::Leaf)
			{
				treeNodes.append(firstSon);
				newNode = G.newNode();
				C.reassignNode(newNode,parent);
				graphNodes.append(newNode);
				G.newEdge(next,newNode);
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
				G.newEdge(next,outgoingTable[f]);
				delete leaf->getKey();
			}

			nextSon = firstSon->getNextSib(oldSib);
			oldSib = firstSon;
			pre = next;
			while (nextSon)
			{
				next = G.newNode();
				C.reassignNode(next,parent);
				G.newEdge(hub,next);
				G.newEdge(pre,next);
				if (nextSon->type() != PQNodeRoot::PQNodeType::Leaf)
				{
					treeNodes.append(nextSon);
					newNode = G.newNode(); // new node corresponding to anchor or cutnode
					C.reassignNode(newNode,parent);
					graphNodes.append(newNode);

					G.newEdge(next,newNode);
				}
				else
				{
					// insert Edge to the outside
					PQLeaf<edge,IndInfo*,bool>* leaf =
						(PQLeaf<edge,IndInfo*,bool>*) nextSon;
					edge f = leaf->getKey()->m_userStructKey;
					G.newEdge(next,outgoingTable[f]);
					delete leaf->getKey();
				}
				holdSib = nextSon->getNextSib(oldSib);
				oldSib = nextSon;
				nextSon = holdSib;
				pre = next;

			}
			G.newEdge(next,correspond);
		}
	}

#ifdef OGDF_DEBUG
	C.consistencyCheck();
#endif
}

//
// Prepare planarity test for one cluster
//
bool CconnectClusterPlanar::preparation(
	Graph  &G,
	const cluster cl,
	node superSink)
{
	int  bcIdSuperSink = -1; // ID of biconnected component that contains superSink
	// Initialization with -1 necessary for assertion
	bool cPlanar = true;


	NodeArray<node> tableNodes(G, nullptr);
	EdgeArray<edge> tableEdges(G, nullptr);
	NodeArray<bool> mark(G, 0);

	EdgeArray<int> componentID(G);


	// Determine Biconnected Components
	int bcCount = biconnectedComponents(G, componentID);

	// Determine edges per biconnected component
	Array<SList<edge> > blockEdges(0, bcCount - 1);
	for (edge e : G.edges) {
		blockEdges[componentID[e]].pushFront(e);
	}

	// Determine nodes per biconnected component.
	Array<SList<node> > blockNodes(0, bcCount - 1);
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

		for (node v : blockNodes[i]) {
			if (mark[v])
				mark[v] = false;
			else {
				OGDF_ASSERT(mark[v]); // v has been placed two times on the list.
			}
		}
	}

	// Perform planarity test for every biconnected component

	if (bcCount == 1)
	{
		// Compute st-numbering
		NodeArray<int> numbering(G,0);
#ifdef OGDF_HEAVY_DEBUG
		int n =
#endif
		(superSink) ? computeSTNumbering(G, numbering, nullptr, superSink)
		            : computeSTNumbering(G, numbering);
		OGDF_HEAVY_ASSERT(isSTNumbering(G, numbering, n));

		EdgeArray<edge> backTableEdges(G,nullptr);
		for(edge e : G.edges)
			backTableEdges[e] = e;

		cPlanar = doTest(G,numbering,cl,superSink,backTableEdges);
	}
	else
	{
		for (int i = 0; i < bcCount; i++)
		{
#ifdef OGDF_HEAVY_DEBUG
			Logger::slout() << std::endl << std::endl << "-----------------------------------";
			Logger::slout() << std::endl << std::endl << "Component " << i <<std::endl;
#endif

			Graph C;

			for (node v : blockNodes[i])
			{
				node w = C.newNode();
				tableNodes[v] = w;

#ifdef OGDF_HEAVY_DEBUG
			Logger::slout() << "Original: " << v << " New: " << w << std::endl;
#endif
			}

			NodeArray<node> backTableNodes(C,nullptr);

			for (edge e : blockEdges[i])
			{
				edge f = C.newEdge(tableNodes[e->source()],tableNodes[e->target()]);
				tableEdges[e] = f;
			}

			EdgeArray<edge> backTableEdges(C,nullptr);
			for (edge e : blockEdges[i])
				backTableEdges[tableEdges[e]] = e;

			// Compute st-numbering
			NodeArray<int> numbering(C,0);
			if (bcIdSuperSink == i) {
#ifdef OGDF_HEAVY_DEBUG
				int n =
#endif
				computeSTNumbering(C, numbering, nullptr, tableNodes[superSink]);
				OGDF_HEAVY_ASSERT(isSTNumbering(C ,numbering, n));
				cPlanar = doTest(C,numbering,cl,tableNodes[superSink],backTableEdges);
			} else {
#ifdef OGDF_HEAVY_DEBUG
				int n =
#endif
				computeSTNumbering(C, numbering);
				OGDF_HEAVY_ASSERT(isSTNumbering(C, numbering, n));
				cPlanar = doTest(C,numbering,cl,nullptr,backTableEdges);
			}

			if (!cPlanar)
				break;
		}
	}

	return cPlanar;
}


// Performs a planarity test on a biconnected component
// of G. numbering contains an st-numbering of the component.
bool CconnectClusterPlanar::doTest(
	Graph &G,
	NodeArray<int> &numbering,
	const cluster cl,
	node superSink,
	EdgeArray<edge> &edgeTable)
{
	bool cPlanar = true;

	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > inLeaves(G);
	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > outLeaves(G);
	Array<node> table(G.numberOfNodes()+1);

	for(node v : G.nodes)
	{
		for (adjEntry adj : v->adjEntries) {
			if (numbering[adj->twinNode()] > numbering[v]) { // side-effect: loops are ignored
				PlanarLeafKey<IndInfo*>* L = new PlanarLeafKey<IndInfo*>(adj->theEdge());
				inLeaves[v].pushFront(L);
			}
		}
		table[numbering[v]] = v;
	}

	for(node v : G.nodes)
	{
		for (PlanarLeafKey<IndInfo*>* L : inLeaves[v])
		{
			outLeaves[L->userStructKey()->opposite(v)].pushFront(L);
		}
	}

	PlanarPQTree* T = new PlanarPQTree();

	T->Initialize(inLeaves[table[1]]);
	for (int i = 2; i < G.numberOfNodes(); i++)
	{
		if (T->Reduction(outLeaves[table[i]]))
		{
			T->ReplaceRoot(inLeaves[table[i]]);
			T->emptyAllPertinentNodes();

		}
		else
		{
			cPlanar = false;
			break;
		}
	}
	if (cPlanar && cl && superSink)
	{
		// Keep the PQTree to construct a wheelgraph
		// Replace the edge stored in the keys of T
		// by the original edges.
		// Necessary, since the edges currently in T
		// correspond to a graph that mirrors a biconnected
		// component and thus is deallocated

		int n = G.numberOfNodes();

		for (PlanarLeafKey<IndInfo*>* info : outLeaves[table[n]])
		{
			PQLeafKey<edge,IndInfo*,bool>* key = (PQLeafKey<edge,IndInfo*,bool>*) info;
			key->m_userStructKey = edgeTable[key->m_userStructKey];
		}

		m_clusterPQTree[cl] = T;

	}
	else //if (cPlanar)
		delete T;

	// Cleanup
	for(node v : G.nodes)
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

	return cPlanar;
}

void CconnectClusterPlanar::prepareParallelEdges(Graph &G)
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
			for (edge f : m_parallelEdges[e])
			{
				m_isParallel[f] = true;
				m_parallelCount++;
			}
		}
	}
}

}
