/** \file
 * \brief Implements the class ClusterGraph, providing
 * extra functionality for clustered graphs.
 * A clustered graph C=(G,T) consists of an undirected graph G
 * and a rooted tree T in which the leaves of T correspond
 * to the vertices of G=(V,E).
 *
 * \author Sebastian Leipert, Karsten Klein
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

#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterArray.h>
#include <ogdf/cluster/ClusterGraphObserver.h>
#include <ogdf/basic/AdjEntryArray.h>
#include <ogdf/basic/Math.h>

using std::mutex;
#ifndef OGDF_MEMORY_POOL_NTS
using std::lock_guard;
#endif

namespace ogdf {

using Math::nextPower2;

#define MIN_CLUSTER_TABLE_SIZE (1 << 4)

void ClusterElement::getClusterInducedNodes(List<node> &clusterNodes)
{
	for (node v : nodes)
		clusterNodes.pushBack(v);
	for (cluster c : children)
		c->getClusterInducedNodes(clusterNodes);
}


void ClusterElement::getClusterInducedNodes(NodeArray<bool> &clusterNode, int& num)
{
	for (node v : nodes) {
		clusterNode[v] = true;
	}
	num += nodes.size();

	for (cluster c : children)
		c->getClusterInducedNodes(clusterNode, num);
}


ClusterGraph::ClusterGraph() : m_pGraph(nullptr)
{
	m_clusterIdCount = 0;
	m_postOrderStart = nullptr;
	m_rootCluster    = nullptr;

	m_allowEmptyClusters = true;
	m_updateDepth   = false;
	m_depthUpToDate = false;

	m_clusterArrayTableSize = MIN_CLUSTER_TABLE_SIZE;
	m_adjAvailable = false;
	m_lcaNumber = 0;
	m_lcaSearch = nullptr;
	m_vAncestor = nullptr;
	m_wAncestor = nullptr;
}


// Construction of a new cluster graph. All nodes
// are children of the root cluster
ClusterGraph::ClusterGraph(const Graph &G) : GraphObserver(&G), m_pGraph(&G)
{
	m_clusterIdCount = 0;
	m_postOrderStart = nullptr;
	m_rootCluster    = nullptr;

	m_allowEmptyClusters = true;
	m_updateDepth   = false;
	m_depthUpToDate = false;

	m_lcaNumber = 0;
	// TODO does this really have to be larger than the node array? (same below)
	m_clusterArrayTableSize = nextPower2(MIN_CLUSTER_TABLE_SIZE, G.nodeArrayTableSize() + 1);
	initGraph(G);
}


ClusterGraph::ClusterGraph(const ClusterGraph &C) : GraphObserver(C.m_pGraph),
	m_lcaSearch(nullptr),
	m_vAncestor(nullptr),
	m_wAncestor(nullptr)
{
	m_clusterIdCount = 0;
	m_postOrderStart = nullptr;
	m_rootCluster    = nullptr;

	m_allowEmptyClusters = true;
	m_updateDepth   = false;
	m_depthUpToDate = false;

	m_lcaNumber = 0;

	m_clusterArrayTableSize = C.m_clusterArrayTableSize;
	shallowCopy(C);
}


ClusterGraph::ClusterGraph(
	const ClusterGraph &C,
	Graph &G,
	ClusterArray<cluster> &originalClusterTable,
	NodeArray<node> &originalNodeTable)
:
	GraphObserver(&G),
	m_lcaSearch(nullptr),
	m_vAncestor(nullptr),
	m_wAncestor(nullptr)
{
	m_clusterIdCount = 0;
	m_postOrderStart = nullptr;
	m_rootCluster    = nullptr;

	m_allowEmptyClusters = true;
	m_updateDepth   = false;
	m_depthUpToDate = false;

	m_lcaNumber = 0;

	m_clusterArrayTableSize = C.m_clusterArrayTableSize;
	deepCopy(C,G,originalClusterTable,originalNodeTable);
}


ClusterGraph::ClusterGraph(
	const ClusterGraph &C,
	Graph &G,
	ClusterArray<cluster> &originalClusterTable,
	NodeArray<node> &originalNodeTable,
	EdgeArray<edge> &edgeCopy)
:
	GraphObserver(&G),
	m_lcaSearch(nullptr),
	m_vAncestor(nullptr),
	m_wAncestor(nullptr)
{
	m_clusterIdCount = 0;
	m_postOrderStart = nullptr;
	m_rootCluster    = nullptr;

	m_allowEmptyClusters = true;
	m_updateDepth   = false;
	m_depthUpToDate = false;

	m_lcaNumber = 0;

	m_clusterArrayTableSize = C.m_clusterArrayTableSize;
	deepCopy(C, G, originalClusterTable, originalNodeTable, edgeCopy);
}


ClusterGraph::ClusterGraph(const ClusterGraph &C,Graph &G) :
	GraphObserver(&G),
	m_lcaSearch(nullptr),
	m_vAncestor(nullptr),
	m_wAncestor(nullptr)
{
	m_clusterIdCount = 0;
	m_postOrderStart = nullptr;
	m_rootCluster    = nullptr;

	m_allowEmptyClusters = true;
	m_updateDepth   = false;
	m_depthUpToDate = false;

	m_lcaNumber = 0;

	m_clusterArrayTableSize = C.m_clusterArrayTableSize;
	deepCopy(C,G);
}


ClusterGraph::~ClusterGraph()
{
	for (ClusterArrayBase *a : m_regClusterArrays)
		a->disconnect();

	doClear();
}


// Construction of a new cluster graph. All nodes
// are children of the root cluster
void ClusterGraph::init(const Graph &G)
{
	doClear();
	m_clusterIdCount = 0;
	m_postOrderStart = nullptr;
	m_pGraph = &G;

	m_lcaNumber = 0;
	m_clusterArrayTableSize = nextPower2(MIN_CLUSTER_TABLE_SIZE, G.nodeArrayTableSize() + 1);
	initGraph(G);
}


ClusterGraph &ClusterGraph::operator=(const ClusterGraph &C)
{
	doClear();
	shallowCopy(C);
	m_clusterArrayTableSize = C.m_clusterArrayTableSize;
	reinitArrays();

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
	return *this;
}

void ClusterGraph::constructClusterTree(
		const ClusterGraph &C,
		const Graph &G,
		ClusterArray<cluster> &originalClusterTable,
		std::function<node(node)> nodeMap)
{
	for (cluster c : C.clusters) {
		if (c == C.m_rootCluster) {
			originalClusterTable[c] = m_rootCluster;
			// does not really need to be assigned HERE in for
			m_rootCluster->m_depth = 1;
			OGDF_ASSERT(c->depth() == 1);
		} else {
			originalClusterTable[c] = newCluster();
			originalClusterTable[c]->m_depth = c->depth();
		}
	}

	for (cluster c : C.clusters) {
		if (c != C.m_rootCluster) {
			originalClusterTable[c]->m_parent = originalClusterTable[c->m_parent];
			originalClusterTable[c->m_parent]->children.pushBack(originalClusterTable[c]);
			originalClusterTable[c]->m_it = originalClusterTable[c->m_parent]->getChildren().rbegin();
		}
	}

	for (node v : G.nodes) {
		reassignNode(v, originalClusterTable[C.clusterOf(nodeMap(v))]);
	}

	copyLCA(C);
}


// Copy Function
void ClusterGraph::shallowCopy(const ClusterGraph &C)
{
	const Graph &G = C;
	m_pGraph = &G;

	initGraph(G);

	m_updateDepth = C.m_updateDepth;
	m_depthUpToDate = C.m_depthUpToDate;

	ClusterArray<cluster> originalClusterTable(C);
	constructClusterTree(C, G, originalClusterTable);
}



// Initialize the graph
void ClusterGraph::initGraph(const Graph &G)
{
	reregister(&G); //will in some constructors cause double registration

	m_lcaNumber = 0;
	m_lcaSearch = nullptr;
	m_vAncestor = nullptr;
	m_wAncestor = nullptr;

	m_adjAvailable = false;

	// root cluster must always get id 0
#ifdef OGDF_DEBUG
	m_rootCluster = new ClusterElement(this, 0);
#else
	m_rootCluster = new ClusterElement(0);
#endif

	OGDF_ASSERT(numberOfClusters() == 0);

	m_rootCluster->m_depth = 1;
	m_clusterIdCount++;
	m_nodeMap.init(G, m_rootCluster);
	m_itMap.init(G);

	// assign already existing nodes to root cluster (new nodes are assigned over nodeadded)
	for (node v : G.nodes)
		m_itMap[v] = m_rootCluster->getNodes().pushBack(v);

	clusters.pushBack(m_rootCluster);
}


void ClusterGraph::reinitGraph(const Graph &G)
{
	m_pGraph = &G;

#ifdef OGDF_HEAVY_DEBUG
	G.consistencyCheck();
#endif

	m_clusterArrayTableSize = nextPower2(MIN_CLUSTER_TABLE_SIZE, G.nodeArrayTableSize() + 1);

	if (numberOfClusters() != 0)
		doClear();

	initGraph(G); //already constructs root cluster, reassign
}


void ClusterGraph::reinitArrays()
{
	for (ClusterArrayBase *a : m_regClusterArrays)
		a->reinit(m_clusterArrayTableSize);
}


// Copy Function
void ClusterGraph::deepCopy(const ClusterGraph &C,Graph &G)
{
	const Graph &cG = C;	// original graph

	ClusterArray<cluster> originalClusterTable(C);
	NodeArray<node> originalNodeTable(cG);
	EdgeArray<edge> edgeCopy(cG);

	deepCopy(C,G,originalClusterTable, originalNodeTable, edgeCopy);
}


void ClusterGraph::deepCopy(
	const ClusterGraph &C,
	Graph &G,
	ClusterArray<cluster> &originalClusterTable,
	NodeArray<node> &originalNodeTable)
{
	const Graph &cG = C;	// original graph

	EdgeArray<edge> edgeCopy(cG);
	deepCopy(C, G, originalClusterTable, originalNodeTable, edgeCopy);
}


void ClusterGraph::deepCopy(
	const ClusterGraph &C,
	Graph &G,
	ClusterArray<cluster> &originalClusterTable,
	NodeArray<node> &originalNodeTable,
	EdgeArray<edge> &edgeCopy)
{
	G.clear();

	const Graph &cG = C;	// original graph

	m_pGraph = &G;

	initGraph(G); //arrays have already to be initialized for newnode

	m_updateDepth = C.m_updateDepth;
	m_depthUpToDate = C.m_depthUpToDate;

	NodeArray<node> orig(G);

	for(node v : cG.nodes) {
		node w = G.newNode();
		orig[w] = v;
		originalNodeTable[v] = w;
	}

	for(edge e : cG.edges) {
		edge eNew = G.newEdge(originalNodeTable[e->adjSource()->theNode()],
					originalNodeTable[e->adjTarget()->theNode()]);
		edgeCopy[e] = eNew;
	}

	constructClusterTree(C, G, originalClusterTable, orig);
}


//We search for the lowest common cluster of a set of nodes.
//We first compute the common path of two nodes, then update path if root
//path from other nodes hits it .
//We always stop if we encounter root cluster.
cluster ClusterGraph::commonCluster(SList<node>& nodes)
{
	//worst case running time #nodes x clustertreeheight-1
	//always <= complete tree run
	//we could even use pathcompression...
	//at any time, we stop if root is encountered as lowest
	//common cluster of a node subset


	if (nodes.empty()) return nullptr;

	//For simplicity, we use cluster arrays
	ClusterArray<int> commonPathHit(*this, 0); //count for clusters path hits
	cluster pathCluster;
	SListIterator<node> sIt = nodes.begin();
	node v1 = *sIt;
	if (nodes.size() == 1) {
		return clusterOf(v1);
	}

	++sIt;
	cluster lowestCommon = commonCluster(v1, *sIt);
	commonPathHit[lowestCommon] = 2;
	pathCluster = lowestCommon;
	while (pathCluster->parent()) {
		pathCluster = pathCluster->parent();
		commonPathHit[pathCluster] = 2;
	}

	// we save direct lca access, it also lies on a runs hit path from root
	for (int runs = 2; runs < nodes.size() && lowestCommon != m_rootCluster; ++runs) {
		// runs is the number of nodes already considered
		++sIt;
		pathCluster = clusterOf(*sIt);
		while (commonPathHit[pathCluster] == 0) {
			OGDF_ASSERT(pathCluster->parent() != nullptr);
			pathCluster = pathCluster->parent();
		}
		// assign new (maybe same) lowest common
		if (commonPathHit[pathCluster] == runs) {
			lowestCommon = pathCluster;
		}
		++commonPathHit[pathCluster];
		if (pathCluster == m_rootCluster) {
			return m_rootCluster;
		}
		// update hits in path to root
		while (pathCluster->parent()) {
			pathCluster = pathCluster->parent();
			++commonPathHit[pathCluster];
		}
	}

	return lowestCommon;
}

//note that eL is directed from v to w
cluster ClusterGraph::commonClusterAncestorsPath(
	node v,
	node w,
	cluster& c1,
	cluster& c2,
	List<cluster>& eL) const
{
	OGDF_ASSERT(v->graphOf() == m_pGraph);
	OGDF_ASSERT(w->graphOf() == m_pGraph);

	cluster cv = clusterOf(v);
	cluster cw = clusterOf(w);

	//clusters from v and w to common
	List<cluster> vList;
	List<cluster> wList;

	//CASE1 no search necessary
	//if both nodes are in the same cluster, we return this cluster
	//and have to check if c1 == c2 to have a (v,w) representation edge
	if (cv == cw)
	{
		c1 = c2 = cv;
		eL.pushBack(c1);
		return cv;
	}

	if (m_lcaNumber == std::numeric_limits<int>::max() - 1) m_lcaNumber = 0;
	else m_lcaNumber++;
	if (!m_lcaSearch)
	{
		m_lcaSearch = new ClusterArray<int>(*this, -1);
		m_vAncestor = new ClusterArray<cluster>(*this, nullptr);
		m_wAncestor = new ClusterArray<cluster>(*this, nullptr);
	}

	//CASE2: one of the nodes hangs at root: save root as ancestor
	//any other case: save cluster of node as ancestor, too, to check this
	//case:: common = xCluster != yCluster
#if 0
	(*m_vAncestor)[rootCluster()] = rootCluster();
	(*m_wAncestor)[rootCluster()] = rootCluster();
#endif
	(*m_vAncestor)[cv] = nullptr;
	(*m_wAncestor)[cw] = nullptr;

	//we rely on the fact all nodes are in the rootcluster or
	//that parent is initialized to zero to terminate

	//we start with different clusters due to CASE1
	//save the ancestor information
	(*m_lcaSearch)[cw] = m_lcaNumber; //not really necessary, we won't return
	(*m_lcaSearch)[cv] = m_lcaNumber;
	vList.pushBack(cv);
	wList.pushBack(cw);

	// we break and return if we find a common node
	// before we reach the rootcluster
	do {
		if (cv->parent()) { // if root not reached on cv-path
			(*m_vAncestor)[cv->parent()] = cv;
			cv = cv->parent();
			// was cv visited on path from w
			if ((*m_lcaSearch)[cv] == m_lcaNumber) {
				c1 = (*m_vAncestor)[cv];
				c2 = (*m_wAncestor)[cv];

				// setup list
				for (auto c : vList) {
					eL.pushBack(c);
				}

				ListReverseIterator<cluster> itC;
				for (itC = wList.rbegin(); itC.valid() && *itC != cv; ++itC);
				for (; itC.valid(); ++itC) {
					eL.pushBack(*itC);
				}

				return cv;
			}
			vList.pushBack(cv);
			(*m_lcaSearch)[cv] = m_lcaNumber;
		}

		if (cw->parent()) { // if root not reached on cw-path
			(*m_wAncestor)[cw->parent()] = cw;
			cw = cw->parent();
			// was cw visited on path from v
			if ((*m_lcaSearch)[cw] == m_lcaNumber) {
				c1 = (*m_vAncestor)[cw];
				c2 = (*m_wAncestor)[cw];

				// setup list
				for (auto itC = vList.begin(); itC.valid() && *itC != cw; ++itC) {
					eL.pushBack(*itC);
				}
				eL.pushBack(cw);

				for (cluster c : reverse(wList)) {
					eL.pushBack(c);
				}

				return cw;
			}
			wList.pushBack(cw);
			(*m_lcaSearch)[cw] = m_lcaNumber;
		}
	} while (cv->parent() || cw->parent());

	// v,w should be at least together in the rootcluster
	c1 = (*m_vAncestor)[rootCluster()];
	c2 = (*m_wAncestor)[rootCluster()];

	return rootCluster();
}


void ClusterGraph::copyLCA(
	const ClusterGraph &C)
{
	if (m_lcaSearch)
	{
		delete m_lcaSearch;
		delete m_vAncestor;
		delete m_wAncestor;
	}
	if (C.m_lcaSearch)
	{
		//otherwise, initialization won't work
		m_clusterArrayTableSize = C.m_clusterArrayTableSize;

		m_lcaSearch = new ClusterArray<int>(*this, -1);//(*C.m_lcaSearch);

		m_vAncestor = new ClusterArray<cluster>(*this, nullptr);
		m_wAncestor = new ClusterArray<cluster>(*this, nullptr);
		//setting of clusters is not necessary!
	}
}


// check the graph for empty clusters
// we never set rootcluster to be one of the empty clusters!!
void ClusterGraph::emptyClusters(
	SList<cluster>& emptyCluster,
	SList<cluster>* checkCluster)
{
	if (checkCluster) {
		fillEmptyClusters(emptyCluster, *checkCluster);
	} else {
		fillEmptyClusters(emptyCluster, clusters);
	}

	// other clusters can get empty, too, if we delete these
	ClusterArray<int> delCount(*this, 0);
	SList<cluster> emptyParent;
	for(cluster c : emptyCluster)
	{
		// count deleted children
		cluster runc = c->parent();
		if (runc)  // is always the case as long as root was not inserted to list
		{
			delCount[runc]++;
			while ((runc->nCount() == 0) && (runc->cCount() == delCount[runc]))
			{
				if (runc == rootCluster()) break;
				emptyParent.pushBack(runc);
				runc = runc->parent();
				delCount[runc]++;
			}
		}
	}

	emptyCluster.conc(emptyParent);
	// for reinsertion, start at emptycluster's back
}


// Inserts a new cluster prescribing its parent
cluster ClusterGraph::newCluster(cluster parent, int id)
{
	OGDF_ASSERT(parent);
	cluster c;
	if (id > 0)
		c = newCluster(id);
	else
		c = newCluster();
	parent->children.pushBack(c);
	c->m_it = parent->getChildren().rbegin();
	c->m_parent = parent;
	c->m_depth = parent->depth() + 1;

	return c;
}


//Insert a new cluster with given ID, precondition: id not used
//has to be updated in the same way as newcluster()
cluster ClusterGraph::newCluster(int id)
{
	m_adjAvailable = false;
	m_postOrderStart = nullptr;
	if (id >= m_clusterIdCount) m_clusterIdCount = id + 1;
	if (m_clusterIdCount >= m_clusterArrayTableSize)
	{
		m_clusterArrayTableSize = nextPower2(m_clusterArrayTableSize, id + 1);
		for(ClusterArrayBase *cab : m_regClusterArrays)
		{
			cab->enlargeTable(m_clusterArrayTableSize);
		}
	}
#ifdef OGDF_DEBUG
	cluster c = new ClusterElement(this, id);
#else
	cluster c = new ClusterElement(id);
#endif
	clusters.pushBack(c);

	// notify observers
	for (ClusterGraphObserver *obs : m_regObservers)
		obs->clusterAdded(c);

	return c;
}


// Inserts a new cluster
//has to be updated in the same way as newcluster(id)
cluster ClusterGraph::newCluster()
{
	m_adjAvailable = false;
	m_postOrderStart = nullptr;
	if (m_clusterIdCount == m_clusterArrayTableSize)
	{
		m_clusterArrayTableSize <<= 1;
		for(ListIterator<ClusterArrayBase*> it = m_regClusterArrays.begin();
			it.valid(); ++it)
		{
			(*it)->enlargeTable(m_clusterArrayTableSize);
		}
	}
#ifdef OGDF_DEBUG
	cluster c = new ClusterElement(this, m_clusterIdCount++);
#else
	cluster c = new ClusterElement(m_clusterIdCount++);
#endif
	clusters.pushBack(c);
	// notify observers
	for (ClusterGraphObserver *obs : m_regObservers)
		obs->clusterAdded(c);

	return c;
}


cluster ClusterGraph::createEmptyCluster(const cluster parent, int clusterId)
{
	//if no id given, use next free id
	if (clusterId < 0) clusterId = m_clusterIdCount;
	//create the new cluster
	cluster cnew;
	if (parent)
		cnew = newCluster(parent, clusterId);
	else
		cnew = newCluster(m_rootCluster, clusterId);
	return cnew;
}

cluster ClusterGraph::createCluster(SList<node>& nodes, const cluster parent)
{
	cluster c;
	if (m_allowEmptyClusters)
	{
		c = doCreateCluster(nodes, parent);
		return c;

	} else {
		SList<cluster> emptyCluster;

		c = doCreateCluster(nodes, emptyCluster, parent);

		for(cluster ec : emptyCluster)
		{
			delCluster(ec);
			//root cluster can never be empty, as we deleted a node
		}
	}

	return c;
}


cluster ClusterGraph::doCreateCluster(
	SList<node>& nodes,
	const cluster parent,
	int clusterId)
{
	if (nodes.empty()) return nullptr;

	//if no id given, use next free id
	if (clusterId < 0) clusterId = m_clusterIdCount;
	//create the new cluster
	cluster cnew;
	if (parent)
		cnew = newCluster(parent, clusterId);
	else
		cnew = newCluster(m_rootCluster, clusterId);

	//insert nodes in new cluster
	for(node v : nodes)
		reassignNode(v, cnew);

	return cnew;
}

cluster ClusterGraph::doCreateCluster(
	SList<node>& nodes,
	SList<cluster>& emptyCluster,
	const cluster parent,
	int clusterId)
{
	// Even if m_allowEmptyClusters is set we check if a cluster
	// looses all of its nodes and has
	// no more entries and childs. This can be used for special cluster
	// object handling or for deletion if m_allowEmptyClusters is not set
	// if it is not the new parent, it can be deleted
	// running time max(#cluster, length(nodelist))
	// TODO: Parameter, der dies auslaesst, da hohe Laufzeit
	// hier macht das nur Sinn, wenn es schneller ist als for all clusters,
	// sonst koennte man es ja auch aussen testen, aber bisher ist es nicht
	// schneller implementiert
	// Vorgehen: hash auf cluster index, falls nicht gesetzt, in liste einfuegen
	// und als checkcluster an emptycluster uebergeben

	if (nodes.empty()) return nullptr;

	//if no id given, use next free id
	if (clusterId < 0) clusterId = m_clusterIdCount;
	//create the new cluster
	cluster cnew;
	if (parent)
		cnew = newCluster(parent, clusterId);
	else
		cnew = newCluster(m_rootCluster, clusterId);

	//insert nodes in new cluster
	for(node v : nodes)
		reassignNode(v, cnew);

	//should be: only for changed clusters (see comment above)
	//it is important to save the cluster in an order
	//that allows deletion as well as reinsertion
	emptyClusters(emptyCluster);
	//for reinsertion, start at emptycluster's back

	return cnew;
}

// Deletes cluster c
// All subclusters become children of parent cluster
// Precondition: c is not the root cluster
// updating of cluster depth information pumps running time
// up to worst case O(#C)
void ClusterGraph::delCluster(cluster c)
{
	OGDF_ASSERT(c != nullptr);
	OGDF_ASSERT(c->graphOf() == this);
	OGDF_ASSERT(c != m_rootCluster);

	// notify observers
	for(ClusterGraphObserver *obs : m_regObservers)
		obs->clusterDeleted(c);

	m_adjAvailable = false;

	c->m_parent->children.del(c->m_it);
	c->m_it = ListIterator<cluster>();

	while (!c->children.empty())
	{
		cluster trace = c->children.popFrontRet();
		trace->m_parent =  c->m_parent;
		trace->m_parent->children.pushBack(trace);
		trace->m_it = trace->m_parent->getChildren().rbegin();

		//only recompute depth if option set and it makes sense
		if (m_updateDepth && m_depthUpToDate)
		{
			//update depth for all children in subtree
			OGDF_ASSERT(trace->depth() == trace->parent()->depth()+2);
			pullUpSubTree(trace);
			//could just set depth-1 here
#if 0
			trace->depth() = trace->parent()->depth()+1;
#endif
		} else {
			m_depthUpToDate = false;
		}
	}
	while (!c->nodes.empty())
	{
		node v = c->nodes.popFrontRet();
		m_nodeMap[v] = nullptr;
		reassignNode(v,c->m_parent);
	}

	clusters.del(c);
}


//pulls up depth of subtree located at c by one
//precondition: depth is consistent
//we dont ask for depthuptodate since the caller needs
//to know for himself if he wants the tree to be pulled
//for any special purpose
void ClusterGraph::pullUpSubTree(cluster c)
{
	c->m_depth = c->depth() - 1;
	for(cluster ci : c->getChildren())
		pullUpSubTree(ci);
}


void ClusterGraph::doClear()
{
	//split condition
	if (m_lcaSearch)
	{
		delete m_lcaSearch;
		delete m_vAncestor;
		delete m_wAncestor;
	}
	if (numberOfClusters() != 0)
	{
		clearClusterTree(m_rootCluster);
		clusters.del(m_rootCluster);
	}
	//no clusters, so we can restart at 0
	m_clusterIdCount = 0;
}


// Removes the Clustering of a Tree and frees the allocated memory
void ClusterGraph::clearClusterTree(cluster c)
{
	cluster parent = c->parent();
	m_postOrderStart = nullptr;

	List<node> attached;
	recurseClearClusterTreeOnChildren(c, attached);

	if (parent != nullptr)
	{
		for (node v : attached)
		{
			m_nodeMap[v] = parent;
			parent->nodes.pushBack(v);
			m_itMap[v] = parent->getNodes().rbegin();
		}
		clusters.del(c);
	}
	else if (c == m_rootCluster)
	{
		for (node v : attached)
		{
			m_nodeMap[v] = m_rootCluster;
			m_rootCluster->nodes.pushBack(v);
			m_itMap[v] = m_rootCluster->getNodes().rbegin();
		}
		m_rootCluster->children.clear();
	}
}


void ClusterGraph::clearClusterTree(cluster c,List<node> &attached)
{
	attached.conc(c->nodes);
	recurseClearClusterTreeOnChildren(c, attached);
	clusters.del(c);
}


//don't delete root cluster
void ClusterGraph::clear()
{
	//split condition
	if (m_lcaSearch)
	{
		delete m_lcaSearch;
		delete m_vAncestor;
		delete m_wAncestor;
	}
	if (numberOfClusters() != 0)
	{
		//clear the cluster structure under root cluster
		clearClusterTree(m_rootCluster);
		//now delete all rootcluster entries
		while (!m_rootCluster->nodes.empty())
		{
			node v = m_rootCluster->nodes.popFrontRet();
			m_nodeMap[v] = nullptr;
		}
	}
	//no child clusters, so we can restart at 1
	m_clusterIdCount = 1;
}


int ClusterGraph::treeDepth() const
{
	//initialize depth at first call
	if (m_updateDepth && !m_depthUpToDate)
		computeSubTreeDepth(rootCluster());
	if (!m_updateDepth) OGDF_THROW(AlgorithmFailureException);
	int l_depth = 1;

	for (cluster c : clusters)
		if (c->depth() > l_depth)
			l_depth = c->depth();

	return l_depth;
}


//reassign cluster depth for clusters in subtree rooted at c
void ClusterGraph::computeSubTreeDepth(cluster c) const
{
	if (c == rootCluster())
		m_depthUpToDate = true;

	c->m_depth = (c->parent() == nullptr) ? 1 : c->parent()->depth() + 1;

	for (cluster child : c->children)
		computeSubTreeDepth(child);
}


//move cluster from old parent to an other
void ClusterGraph::moveCluster(cluster c, cluster newParent)
{
	if (c == rootCluster()) return;
	if ((c == nullptr) || (newParent == nullptr)) return; //no cheap tricks
	if (c->parent() == newParent) return;     //no work to do

	cluster oldParent = c->parent();
	//we dont move root
	OGDF_ASSERT(oldParent);

	//check if we move to a descendant
	cluster crun = newParent->parent();
	bool descendant = false;
	while (crun)
	{
		if (crun == c)
		{
			descendant = true;
			break;
		}
		crun = crun->parent();
	}

	//do not allow to move empty clusters to descendants
	if (descendant && (c->nCount() == 0))
		return;

	//temporarily only recompute postorder for all clusters

	oldParent->children.del(c->m_it);
	newParent->children.pushBack(c);
	c->m_it = newParent->getChildren().rbegin();
	c->m_parent = newParent;

	//update the cluster depth information in the subtree
	//If moved to descendant, recompute
	//depth for parent (including all brother trees)
	if (descendant)
	{
		//how do we move:
		//only entries with c? => may be empty
		//we currently dont allow this, because it makes
		//no sense, you could just delete the cluster or move
		//the children
		//move all children to oldparent

		while (!c->children.empty())
		{
			cluster child = c->children.popFrontRet();
			child->m_parent =  oldParent;
			child->m_parent->children.pushBack(child);
			child->m_it = child->m_parent->getChildren().rbegin();
#if 0
			child++;
#endif
		}

		//recompute depth only if option set AND it makes sense at that point
		if (m_updateDepth && m_depthUpToDate)
			computeSubTreeDepth(oldParent);
		else m_depthUpToDate = false;
	} else {
		if (m_updateDepth && m_depthUpToDate)
			computeSubTreeDepth(c);
		else m_depthUpToDate = false;
	}

	// update postorder for new parent
	// we only recompute postorder for all clusters
	// because of special cases like move to descendant...
	postOrder();

	m_adjAvailable = false;

#if 0
	checkPostOrder();
#endif
}

//leftmostcluster in subtree rooted at c, has postorderpred for subtree
cluster ClusterGraph::leftMostCluster(cluster c) const
{
	cluster result = c;
	if (!c) return nullptr;
	while (!result->children.empty())
	{
		result = result->children.front();
	}
	return result;
}

//searches for predecessor of SUBTREE at c
cluster ClusterGraph::postOrderPredecessor(cluster c) const
{
	//all clusters on a path from root to leftmost cluster in tree
	//have no predecessor for their subtree
	cluster run = c;
	do
	{
		//predecessor of clustertree is	0
		if (run == m_rootCluster)
			return nullptr;

		ListConstIterator<cluster> it = run->m_it;
		//a child to the left is the immediate predecessor,
		//otherwise we go one level up
		if (it == (run->m_parent)->children.begin())
			run = run->parent();
		else
			return *(it.pred());

	} while (run);

	return nullptr;
}

void ClusterGraph::nodeDeleted(node v)
{
	bool cRemove = false;
	cluster c = clusterOf(v);
	if (!c) return;
	//never allow totally empty cluster
#if 0
	if ((emptyOnNodeDelete(c)) &&
		(c != rootCluster()) ) cRemove = true;
#endif
	unassignNode(v);
	if (cRemove && !m_allowEmptyClusters) //parent exists
	{
		cluster nonEmpty = c->parent();
		cluster cRun = nonEmpty;
		delCluster(c);
		while ((cRun != rootCluster()) && (cRun->nCount() + cRun->cCount() == 0))
		{
			nonEmpty = cRun->parent();
			delCluster(cRun);
			cRun = nonEmpty;
		}

	}
}


//node assignment
//Assigns a node to a new cluster
void ClusterGraph::assignNode(node v, cluster c)
{
	m_adjAvailable = false;
	m_postOrderStart = nullptr;
	m_nodeMap[v] = c;
	c->nodes.pushBack(v);
	m_itMap[v] = c->getNodes().rbegin();
}


//Reassigns a node to a new cluster
void ClusterGraph::reassignNode(node v, cluster c)
{
	OGDF_ASSERT(v->graphOf() == m_pGraph);
	OGDF_ASSERT(c->graphOf() == this);

	unassignNode(v);
	m_nodeMap[v] = c;
	c->nodes.pushBack(v);
	m_itMap[v] = c->getNodes().rbegin();
}


//Unassigns a node of cluster
//Note: Nodes can already be unassigned by the nodeDeleted function.
void ClusterGraph::unassignNode(node v)
{
	m_adjAvailable = false;
	m_postOrderStart = nullptr;

	removeNodeAssignment(v);
}


// Start function for post order
void ClusterGraph::postOrder() const
{
	SListPure<cluster> L;
	postOrder(m_rootCluster,L);
	cluster c = nullptr;
	cluster prev = L.popFrontRet();
	prev->m_pPrev = nullptr;
	m_postOrderStart = prev;
	while (!L.empty())
	{
		c = L.popFrontRet();
		prev->m_pNext = c;
		c->m_pPrev = prev;
		prev = c;
	}
	if (c != nullptr)
		c->m_pNext = nullptr;
	else
		m_postOrderStart->m_pNext = nullptr;

#ifdef OGDF_DEBUG
	for(cluster cl : clusters) {
		cluster cp = leftMostCluster(cl);
		OGDF_ASSERT(cp->pPred() == postOrderPredecessor(cl));
	}
#endif
}


#ifdef OGDF_DEBUG
void ClusterGraph::checkPostOrder() const
{
	SListPure<cluster> L;
	postOrder(m_rootCluster,L);
	cluster c = nullptr;
	cluster prev = L.popFrontRet();
	OGDF_ASSERT(prev->m_pPrev == nullptr);

	while (!L.empty()) {
		c = L.popFrontRet();
		OGDF_ASSERT(prev->m_pNext == c);
		OGDF_ASSERT(c->m_pPrev == prev);
		prev = c;
	}
	if (c != nullptr) {
		OGDF_ASSERT(c->m_pNext == nullptr);
	} else {
		OGDF_ASSERT(m_postOrderStart->m_pNext == nullptr);
	}
}
#endif


// Recursive function for post order
void ClusterGraph::postOrder(cluster c, SListPure<cluster> &L) const
{
	for (cluster ci : c->children)
		postOrder(ci,L);

	L.pushBack(c);
}


#ifdef OGDF_DEBUG
void ClusterGraph::consistencyCheck() const
{
	ClusterArray<bool> visitedClusters((*this),false);
	NodeArray<bool> visitedNodes((*m_pGraph),false);

	cluster c = nullptr;
	forall_postOrderClusters(c,(*this))
	{
		visitedClusters[c] = true;

		for (node v : c->nodes) {
			OGDF_ASSERT(m_nodeMap[v] == c);
			visitedNodes[v] = true;
		}
	}

	for (cluster cl : clusters) {
		OGDF_ASSERT(visitedClusters[cl]);
	}

	for (node v : m_pGraph->nodes) {
		OGDF_ASSERT(visitedNodes[v]);
	}
}
#endif


bool ClusterGraph::representsCombEmbedding() const
{
	if (!m_adjAvailable)
		return false;

#ifdef OGDF_DEBUG
	consistencyCheck();
#endif

	cluster c = nullptr;
	forall_postOrderClusters(c,(*this))
	{
#ifdef OGDF_HEAVY_DEBUG
		Logger::slout() << "__________________________________________________________________"
				<< std::endl << std::endl
				<< "Testing cluster " << c << std::endl
				<< "Check on AdjList of c" << std::endl;
			for(adjEntry adjDD : c->adjEntries)
				Logger::slout() << adjDD << ";  ";
			Logger::slout() << std::endl;
#endif

		if (c != m_rootCluster)
		{
			ListConstIterator<adjEntry> it;
			it = c->firstAdj();
			adjEntry start = *it;

#ifdef OGDF_HEAVY_DEBUG
			Logger::slout() << "firstAdj " << start << std::endl;
#endif

			while (it.valid())
			{
				AdjEntryArray<bool> visitedAdjEntries((*m_pGraph),false);

				ListConstIterator<adjEntry> succ = it.succ();
				adjEntry adj = *it;
				adjEntry succAdj;

				if (succ.valid())
					succAdj = *succ;
				else
					succAdj = start;  // reached the last outgoing edge

#ifdef OGDF_HEAVY_DEBUG
				Logger::slout() << "Check next " << std::endl;
				Logger::slout() << "current in adj list of" << adj << std::endl;
				Logger::slout() << "succ in adj list of c " << succAdj << std::endl;
				Logger::slout() << "cyclic succ in outer face " << adj->cyclicSucc() << std::endl;
#endif


				if (adj->cyclicSucc() != succAdj)
					// run along the outer face of the cluster
						// until you find the next outgoing edge
				{
					adjEntry next = adj->cyclicSucc();
					adjEntry twin = next->twin();

#ifdef OGDF_HEAVY_DEBUG
					Logger::slout() << "Running along the outer face ... " << std::endl;
					Logger::slout() << "next adj " << next << std::endl;
					Logger::slout() << "twin adj " << twin << std::endl;
#endif

					if (visitedAdjEntries[twin])
						return false;
					visitedAdjEntries[twin] = true;
					while ( next != succAdj)
					{
						next = twin->cyclicSucc();
						twin = next->twin();
#ifdef OGDF_HEAVY_DEBUG
						Logger::slout() << "Running along the outer face ... " << std::endl;
						Logger::slout() << "next adj " << next << std::endl;
						Logger::slout() << "twin adj " << twin << std::endl;
#endif
						if (visitedAdjEntries[twin])
							return false;
						visitedAdjEntries[twin] = true;
					}

				}
				// else
				// next edge is also outgoing

				it = succ;
			}
		}
	}

	return true;
}


// registers a cluster array
ListIterator<ClusterArrayBase*> ClusterGraph::registerArray(
	ClusterArrayBase *pClusterArray) const
{
#ifndef OGDF_MEMORY_POOL_NTS
	lock_guard<mutex> guard(m_mutexRegArrays);
#endif
	return m_regClusterArrays.pushBack(pClusterArray);
}


// unregisters a cluster array
void ClusterGraph::unregisterArray(ListIterator<ClusterArrayBase*> it) const
{
#ifndef OGDF_MEMORY_POOL_NTS
	lock_guard<mutex> guard(m_mutexRegArrays);
#endif
	m_regClusterArrays.del(it);
}


void ClusterGraph::moveRegisterArray(
	ListIterator<ClusterArrayBase*> it, ClusterArrayBase *pClusterArray) const
{
#ifndef OGDF_MEMORY_POOL_NTS
	lock_guard<mutex> guard(m_mutexRegArrays);
#endif
	*it = pClusterArray;
}


// registers a ClusterGraphObserver.
ListIterator<ClusterGraphObserver*> ClusterGraph::registerObserver(ClusterGraphObserver *pObserver) const
{
#ifndef OGDF_MEMORY_POOL_NTS
	lock_guard<mutex> guard(m_mutexRegArrays);
#endif
	return m_regObservers.pushBack(pObserver);
}


// unregisters a ClusterGraphObserver.
void ClusterGraph::unregisterObserver(ListIterator<ClusterGraphObserver*> it) const
{
#ifndef OGDF_MEMORY_POOL_NTS
	lock_guard<mutex> guard(m_mutexRegArrays);
#endif
	m_regObservers.del(it);
}

std::ostream &operator<<(std::ostream &os, cluster c) {
	if (c != nullptr) {
		os << c->index();
	} else {
		os << "nil";
	}
	return os;
}

}
