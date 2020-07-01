/** \file
 * \brief Implementation of ExtendedNestingGraph and helper classes
 *
 * \author Carsten Gutwenger
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

#include <ogdf/layered/ExtendedNestingGraph.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/layered/OptimalRanking.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/basic/Array2D.h>
#include <ogdf/basic/Math.h>
#include <ogdf/cluster/ClusterSet.h>

using std::tuple;

namespace ogdf {

std::ostream& operator<<(std::ostream &os, const RCCrossings &cr)
{
	os << "(" << cr.m_cnClusters << "," << cr.m_cnEdges << ")";
	return os;
}

void LHTreeNode::setPos()
{
	for(int i = 0; i <= m_child.high(); ++i)
		m_child[i]->m_pos = i;
}


void LHTreeNode::removeAuxChildren()
{
	OGDF_ASSERT(isCompound());

	int j = 0;
	int i;
	for(i = 0; i <= m_child.high(); ++i)
		if(m_child[i]->m_type != Type::AuxNode)
			m_child[j++] = m_child[i];
		else
			delete m_child[i];

	int add = j-i;
	if(add != 0)
		m_child.grow(add, nullptr);
}


std::ostream &operator<<(std::ostream &os, const LHTreeNode *n)
{
	if(n->isCompound()) {
		os << "C" << n->originalCluster();

		os << " [";
		for(int i = 0; i < n->numberOfChildren(); ++i)
			os << " " << n->child(i);
		os << " ]";

	} else {
		os << "N" << n->getNode() << " ";
	}

	return os;
}


//! Compares adjacency entries in an LHTreeNode
class AdjacencyComparer
{
	static int compare(const LHTreeNode::Adjacency &x, const LHTreeNode::Adjacency &y)
	{
		if(x.m_u->index() < y.m_u->index())
			return -1;

		else if(x.m_u == y.m_u) {
			if(x.m_v->isCompound()) {
				if(!y.m_v->isCompound()) return -1;
				return (x.m_v->originalCluster()->index() < y.m_v->originalCluster()->index()) ? -1 : +1;

			} else if(y.m_v->isCompound())
				return +1;

			else
				return (x.m_v->getNode()->index() < y.m_v->getNode()->index()) ? -1 : +1;

		} else
			return +1;
	}

	OGDF_AUGMENT_STATICCOMPARER(LHTreeNode::Adjacency)
};


ENGLayer::~ENGLayer()
{
	Queue<LHTreeNode*> Q;
	Q.append(m_root);

	while(!Q.empty()) {
		LHTreeNode *p = Q.pop();

		for(int i = 0; i < p->numberOfChildren(); ++i)
			Q.append(p->child(i));

		delete p;
	}
}


void ENGLayer::store() {
	Queue<LHTreeNode*> Q;
	Q.append(m_root);

	while(!Q.empty()) {
		LHTreeNode *p = Q.pop();

		if(p->isCompound()) {
			p->store();

			for(int i = 0; i < p->numberOfChildren(); ++i)
				Q.append(p->child(i));
		}
	}
}


void ENGLayer::restore() {
	Queue<LHTreeNode*> Q;
	Q.append(m_root);

	while(!Q.empty()) {
		LHTreeNode *p = Q.pop();

		if(p->isCompound()) {
			p->restore();

			for(int i = 0; i < p->numberOfChildren(); ++i)
				Q.append(p->child(i));
		}
	}
}


void ENGLayer::permute() {
	Queue<LHTreeNode*> Q;
	Q.append(m_root);

	while(!Q.empty()) {
		LHTreeNode *p = Q.pop();

		if(p->isCompound()) {
			p->permute();

			for(int i = 0; i < p->numberOfChildren(); ++i)
				Q.append(p->child(i));
		}
	}
}


void ENGLayer::removeAuxNodes() {
	Queue<LHTreeNode*> Q;
	Q.append(m_root);

	while(!Q.empty()) {
		LHTreeNode *p = Q.pop();

		if(p->isCompound()) {
			p->removeAuxChildren();

			for(int i = 0; i < p->numberOfChildren(); ++i)
				Q.append(p->child(i));
		}
	}
}


void ENGLayer::simplifyAdjacencies(List<LHTreeNode::Adjacency> &adjs)
{
	AdjacencyComparer cmp;

	if(!adjs.empty()) {
		adjs.quicksort(cmp);

		ListIterator<LHTreeNode::Adjacency> it = adjs.begin();
		ListIterator<LHTreeNode::Adjacency> itNext = it.succ();

		while(itNext.valid()) {
			if((*it).m_u == (*itNext).m_u && (*it).m_v == (*itNext).m_v) {
				(*it).m_weight += (*itNext).m_weight;

				adjs.del(itNext);
				itNext = it.succ();

			} else {
				it = itNext;
				++itNext;
			}
		}
	}
}


void ENGLayer::simplifyAdjacencies()
{
	Queue<LHTreeNode*> Q;
	Q.append(m_root);

	while(!Q.empty()) {
		LHTreeNode *p = Q.pop();

		simplifyAdjacencies(p->m_upperAdj);
		simplifyAdjacencies(p->m_lowerAdj);

		for(int i = 0; i < p->numberOfChildren(); ++i)
			Q.append(p->child(i));
	}
}


ClusterGraphCopy::ClusterGraphCopy()
{
	m_pCG = nullptr;
	m_pH  = nullptr;
}

ClusterGraphCopy::ClusterGraphCopy(const ExtendedNestingGraph &H, const ClusterGraph &CG)
: ClusterGraph(H), m_pCG(&CG), m_pH(&H), m_copy(CG,nullptr)
{
	m_original.init(*this,nullptr);
	m_copy    [CG.rootCluster()] = rootCluster();
	m_original[rootCluster()]    = CG.rootCluster();

	createClusterTree(CG.rootCluster());
}


void ClusterGraphCopy::init(const ExtendedNestingGraph &H, const ClusterGraph &CG)
{
	ClusterGraph::init(H);
	m_pCG = &CG;
	m_pH  = &H;
	m_copy    .init(CG,nullptr);
	m_original.init(*this,nullptr);

	m_copy    [CG.rootCluster()] = rootCluster();
	m_original[rootCluster()]    = CG.rootCluster();

	createClusterTree(CG.rootCluster());
}


void ClusterGraphCopy::createClusterTree(cluster cOrig)
{
	cluster c = m_copy[cOrig];

	for (cluster childOrig : cOrig->children) {
		cluster child = newCluster(c);
		m_copy    [childOrig] = child;
		m_original[child]     = childOrig;

		createClusterTree(childOrig);
	}

	ListConstIterator<node> itV;
	for(itV = cOrig->nBegin(); itV.valid(); ++itV) {
		reassignNode(m_pH->copy(*itV), c);
	}
}


void ClusterGraphCopy::setParent(node v, cluster c)
{
	reassignNode(v, c);
}


ExtendedNestingGraph::ExtendedNestingGraph(const ClusterGraph &CG) :
	m_copy(CG),
	m_topNode(CG),
	m_bottomNode(CG),
	m_copyEdge(CG),
	m_mark(CG, nullptr)
{
	const Graph &G = CG;

	m_origNode.init(*this, nullptr);
	m_type    .init(*this, NodeType::Dummy);
	m_origEdge.init(*this, nullptr);

	// Create nodes
	for(node v : G.nodes) {
		node vH = newNode();
		m_copy    [v]  = vH;
		m_origNode[vH] = v;
		m_type[vH] = NodeType::Node;
	}

	m_CGC.init(*this,CG);

	for(cluster c : CG.clusters) {
		m_type[m_topNode   [c] = newNode()] = NodeType::ClusterTop;
		m_type[m_bottomNode[c] = newNode()] = NodeType::ClusterBottom;

		m_CGC.setParent(m_topNode   [c], m_CGC.copy(c));
		m_CGC.setParent(m_bottomNode[c], m_CGC.copy(c));
	}

	// Create edges
	for(node v : G.nodes) {
		node    vH = m_copy[v];
		cluster c  = CG.clusterOf(v);

		newEdge(m_topNode[c], vH);
		newEdge(vH, m_bottomNode[c]);
	}

	for(cluster c : CG.clusters) {
		if(c != CG.rootCluster()) {
			cluster u = c->parent();

			newEdge(m_topNode[u], m_topNode[c]);
			newEdge(m_bottomNode[c], m_bottomNode[u]);

			newEdge(m_topNode[c], m_bottomNode[c]);
		}
	}

	OGDF_ASSERT(isAcyclic(*this));


	// preparation for improved test for cycles
	m_aeLevel.init(*this, -1);
	int count = 0;
	assignAeLevel(CG.rootCluster(), count);
	m_aeVisited.init(*this, false);


	// Add adjacency edges
	for(edge e : G.edges) {
		edge eH = addEdge(m_copy[e->source()], m_copy[e->target()], true);
		m_copyEdge[e].pushBack(eH);
		m_origEdge[eH] = e;
	}

	// Add additional edges between nodes and clusters to reflect adjacency hierarchy also
	// with respect to clusters
	for(edge e : G.edges) {
		node u = e->source();
		node v = e->target();

		// e was reversed?
		if(m_copyEdge[e].front()->source() != m_copy[e->source()])
			std::swap(u, v);

		if(CG.clusterOf(u) != CG.clusterOf(v)) {
			cluster c = lca(u, v);
			cluster cTo, cFrom;

			if(m_secondPathTo == v) {
				cTo   = m_secondPath;
				cFrom = m_mark[c];
			} else {
				cFrom = m_secondPath;
				cTo   = m_mark[c];
			}

			// Transfer adjacency relationship to a relationship between clusters
			// "clusters shall be above each other"
			edge eH = nullptr;
			if(cFrom != c && cTo != c)
				eH = addEdge(m_bottomNode[cFrom], m_topNode[cTo]);

			// if this is not possible, try to relax it to a relationship between node and cluster
			if(eH == nullptr) {
				addEdge(m_copy[u], m_topNode[cTo]);
				addEdge(m_bottomNode[cFrom], m_copy[v]);
			}
		}
	}

	OGDF_ASSERT(isAcyclic(*this));

	// cleanup
	m_aeVisited.init();
	m_aeLevel.init();

	// compute ranking and proper hierarchy
	computeRanking();
	createDummyNodes();
#if 0
	createVirtualClusters();
#endif
	buildLayers();

	// assign positions on top layer
	m_pos.init(*this);
	count = 0;
	assignPos(m_layer[0].root(), count);
}

void ExtendedNestingGraph::computeRanking()
{
	// Compute ranking
	OptimalRanking ranking;
	ranking.separateMultiEdges(false);

	EdgeArray<int> length(*this);
	EdgeArray<int> cost(*this);
	for(edge e : edges) {
		NodeType typeSrc = type(e->source());
		NodeType typeTgt = type(e->target());

		if(typeSrc == NodeType::Node && typeTgt == NodeType::Node)
			length[e] = 2; // Node -> Node
		else if (typeSrc != NodeType::Node && typeTgt != NodeType::Node)
			length[e] = 2; // Cluster -> Cluster
		else
			length[e] = 1; // Node <-> Cluster

		cost[e] = (origEdge(e) != nullptr) ? 2 : 1;
	}

	ranking.call(*this, length, cost, m_rank);

	// adjust ranks of top / bottom node
	for(cluster c = m_CGC.firstPostOrderCluster(); c != nullptr; c = c->pSucc())
	{
		int t = std::numeric_limits<int>::max();
		int b = std::numeric_limits<int>::min();

		ListConstIterator<node> itV;
		for(itV = c->nBegin(); itV.valid();  ++itV) {
			if(type(*itV) != NodeType::Node)
				continue;

			int r = m_rank[*itV];
			if(r-1 < t)
				t = r-1;
			if(r+1 > b)
				b = r+1;
		}

		for (cluster child : c->children) {
			int rb = m_rank[bottom(m_CGC.original(child))];
			if(rb+2 > b)
				b = rb+2;
			int rt = m_rank[top(m_CGC.original(child))];
			if(rt-2 < t)
				t = rt-2;
		}

		cluster cOrig = m_CGC.original(c);
		OGDF_ASSERT(m_rank[top(cOrig)] <= t);
		OGDF_ASSERT(b <= m_rank[bottom(cOrig)]);

		if(t < std::numeric_limits<int>::max()) {
			m_rank[top   (cOrig)] = t;
			m_rank[bottom(cOrig)] = b;
		}
	}

	// Remove all non-adjacency edges
	edge eNext;
	for(edge e = firstEdge(); e != nullptr; e = eNext) {
		eNext = e->succ();
		if(m_origEdge[e] == nullptr) {
			cluster c = originalCluster(e->source());
			// we do not remove edges from top(c)->bottom(c)
			if(e->source() != top(c) || e->target() != bottom(c))
				delEdge(e);
		}
	}

	// Remove nodes for root cluster
	cluster r = getOriginalClusterGraph().rootCluster();
	int high = m_rank[m_bottomNode[r]];
	int low  = m_rank[m_topNode[r]];

	delNode(m_topNode[r]);
	delNode(m_bottomNode[r]);
	m_topNode   [r] = nullptr;
	m_bottomNode[r] = nullptr;

	// Reassign ranks
	Array<SListPure<node> > levels(low,high);

	for(node v : nodes)
		levels[m_rank[v]].pushBack(v);

	int currentRank = 0;
	for(int i = low+1; i < high; ++i)
	{
		SListPure<node> &L = levels[i];
		if(L.empty())
			continue;

		for(node v : L)
			m_rank[v] = currentRank;

		++currentRank;
	}

	m_numLayers = currentRank;
}


void ExtendedNestingGraph::createDummyNodes()
{
	const ClusterGraph &CG = getOriginalClusterGraph();
	const Graph        &G  = CG;

	for(edge e : G.edges)
	{
		edge eH = m_copyEdge[e].front();
		node uH = eH->source();
		node vH = eH->target();

		int span = m_rank[vH] - m_rank[uH];
		OGDF_ASSERT(span >= 1);
		if(span < 2)
			continue;

		// find cluster cTop containing both u and v
		node u = m_origNode[uH];
		node v = m_origNode[vH];

		cluster cTop = lca(u,v);

		// create split nodes
		for(int i = m_rank[uH]+1; i < m_rank[vH]; ++i) {
			eH = split(eH);
			m_copyEdge[e].pushBack(eH);
			m_origEdge[eH] = e;
			m_rank[eH->source()] = i;
			// assign preliminary cTop to all dummies since this is ok
			// try to aesthetically improve this later
			m_CGC.setParent(eH->source(), m_CGC.copy(cTop));
		}

		// improve cluster assignment
		cluster c_1 = CG.clusterOf(u);
		cluster c_2 = CG.clusterOf(v);
		cluster root = CG.rootCluster();

		if(c_1 == root || c_2 == root || m_rank[m_bottomNode[c_1]] >= m_rank[m_topNode[c_2]]) {
			if(c_2 != root && m_rank[uH] < m_rank[m_topNode[c_2]])
			{
				c_1 = nullptr;
				while(c_2->parent() != root && m_rank[uH] < m_rank[m_topNode[c_2->parent()]])
					c_2 = c_2->parent();
			}
			else if(c_1 != root && m_rank[vH] > m_rank[m_bottomNode[c_1]])
			{
				c_2 = nullptr;
				while(c_1->parent() != root && m_rank[vH] > m_rank[m_bottomNode[c_1->parent()]])
					c_1 = c_1->parent();

			} else {
				continue; // leave all dummies in cTop
			}

		} else {
			bool cont;
			do {
				cont = false;

				cluster parent = c_1->parent();
				if(parent != root && m_rank[m_bottomNode[parent]] < m_rank[m_topNode[c_2]]) {
					c_1 = parent;
					cont = true;
				}

				parent = c_2->parent();
				if(parent != root && m_rank[m_bottomNode[c_1]] < m_rank[m_topNode[parent]]) {
					c_2 = parent;
					cont = true;
				}

			} while(cont);
		}

		if(c_1 != nullptr) {
			ListConstIterator<edge> it = m_copyEdge[e].begin();
			for(cluster c = CG.clusterOf(u); c != c_1->parent(); c = c->parent()) {
				while(m_rank[(*it)->target()] <= m_rank[m_bottomNode[c]]) {
					m_CGC.setParent((*it)->target(), m_CGC.copy(c));
					++it;
				}
			}
		}

		if(c_2 != nullptr) {
			ListConstReverseIterator<edge> it = m_copyEdge[e].rbegin();
			for(cluster c = CG.clusterOf(v); c != c_2->parent(); c = c->parent()) {
				while(m_rank[(*it)->source()] >= m_rank[m_topNode[c]]) {
					m_CGC.setParent((*it)->source(), m_CGC.copy(c));
					++it;
				}
			}
		}
	}

	// create dummy nodes for edges top(c)->bottom(c)
	for(cluster c : CG.clusters)
	{
		if(c == CG.rootCluster())
			continue;

		node vTop = top(c);
		node vBottom = bottom(c);

		for(adjEntry adj : vTop->adjEntries) {
			edge e = adj->theEdge();
			if(e->target() == vBottom) {
				int span = m_rank[vBottom] - m_rank[vTop];
				OGDF_ASSERT(span >= 1);
				if(span < 2)
					continue;

				// create split nodes
				edge eH = e;
				for(int i = m_rank[vTop]+1; i < m_rank[vBottom]; ++i) {
					eH = split(eH);
					m_rank[eH->source()] = i;
					m_type[eH->source()] = NodeType::ClusterTopBottom;
					m_CGC.setParent(eH->source(), m_CGC.copy(c));
				}
				break;
			}
		}
	}
}


void ExtendedNestingGraph::createVirtualClusters()
{
	NodeArray   <node> vCopy(*this);
	ClusterArray<node> cCopy(m_CGC);

	createVirtualClusters(m_CGC.rootCluster(), vCopy, cCopy);

	// for each original edge, put the edge segments that are in the same cluster
	// into a separate cluster
	for(edge eOrig : m_CGC.getOriginalClusterGraph().constGraph().edges)
	{
		const List<edge> &L = m_copyEdge[eOrig];
		if(L.size() >= 3) {
			ListConstIterator<edge> it = L.begin().succ();
			node v = (*it)->source();

			cluster c = parent(v);
			SList<node> nextCluster;
			nextCluster.pushBack(v);

			for(++it; it.valid(); ++it) {
				node u = (*it)->source();
				cluster cu = parent(u);

				if(cu != c) {
					if(nextCluster.size() > 1)
						m_CGC.createCluster(nextCluster, c);

					nextCluster.clear();
					c = cu;
				}

				nextCluster.pushBack(u);
			}

			if(nextCluster.size() > 1)
				m_CGC.createCluster(nextCluster, c);
		}
	}
}


void ExtendedNestingGraph::createVirtualClusters(
	cluster c,
	NodeArray<node>    &vCopy,
	ClusterArray<node> &cCopy)
{
	if(c->cCount() >= 1 && c->nCount() >= 1)
	{
		// build auxiliaray graph G
		Graph G;

		ListConstIterator<node> itV;
		for(itV = c->nBegin(); itV.valid(); ++itV) {
			vCopy[*itV] = G.newNode();
		}

		for (cluster child : c->children)
			cCopy[child] = G.newNode();

		for(itV = c->nBegin(); itV.valid(); ++itV) {
			node v = *itV;
			for(adjEntry adj : v->adjEntries) {
				if(origEdge(adj->theEdge()) == nullptr)
					continue;

				node w = adj->twinNode();
				cluster cw = parent(w);
				if(cw == c)
					G.newEdge(vCopy[v],vCopy[w]);

				else if(cw->parent() == c) {
					cluster cwOrig = m_CGC.original(cw);
					OGDF_ASSERT(cwOrig != nullptr);
					if(rank(w) == rank(top(cwOrig)) || rank(w) == rank(bottom(cwOrig)))
						G.newEdge(vCopy[v],cCopy[cw]);
				}
			}
		}

		// find connect components in G
		NodeArray<int> component(G);
		int k = connectedComponents(G, component);

		// create virtual clusters
		if(k > 1) {
			Array<SList<node   > > nodeList(k);
			Array<SList<cluster> > clusters(k);

			for(itV = c->nBegin(); itV.valid(); ++itV)
				nodeList[component[vCopy[*itV]]].pushBack(*itV);

			for(cluster child : c->children)
				clusters[component[cCopy[child]]].pushBack(child);

			for(int i = 0; i < k; ++i) {
				if(nodeList[i].size() + clusters[i].size() > 1) {
					cluster cVirt = m_CGC.createCluster(nodeList[i], c);
					for(cluster ci : clusters[i])
						m_CGC.moveCluster(ci,cVirt);
				}
			}
		}
	}

	// recursive call
	for(cluster child : c->children)
		createVirtualClusters(child, vCopy, cCopy);
}


void ExtendedNestingGraph::buildLayers()
{
	m_layer.init(m_numLayers);

	Array<List<node> > L(m_numLayers);

	for(node v : nodes) {
		L[rank(v)].pushBack(v);
	}

	// compute minimum and maximum level of each cluster
	m_topRank.init(m_CGC,m_numLayers);
	m_bottomRank.init(m_CGC,0);
	cluster c;
	forall_postOrderClusters(c, m_CGC) {
		ListConstIterator<node> itV;
		for(itV = c->nBegin(); itV.valid(); ++itV) {
			int r = rank(*itV);
			if(r > m_bottomRank[c])
				m_bottomRank[c] = r;
			if(r < m_topRank[c])
				m_topRank[c] = r;
		}
		for (cluster child : c->children) {
			if(m_topRank[child] < m_topRank[c])
				m_topRank[c] = m_topRank[child];
			if(m_bottomRank[child] > m_bottomRank[c])
				m_bottomRank[c] = m_bottomRank[child];
		}
	}

	Array<SListPure<cluster> > clusterBegin(m_numLayers);
	Array<SListPure<cluster> > clusterEnd(m_numLayers);

	for(cluster cl : m_CGC.clusters) {
		clusterBegin[m_topRank   [cl]].pushBack(cl);
		clusterEnd  [m_bottomRank[cl]].pushBack(cl);
	}


	ClusterSetPure activeClusters(m_CGC);
	activeClusters.insert(m_CGC.rootCluster());

	ClusterArray<LHTreeNode*> clusterToTreeNode(m_CGC, nullptr);
	ClusterArray<int>         numChildren(m_CGC, 0);
	NodeArray<LHTreeNode*>    treeNode(*this, nullptr);

	int i;
	for(i = 0; i < m_numLayers; ++i) {
		// identify new clusters on this layer
		for(node v : L[i]) {
			++numChildren[parent(v)];
		}

		for(cluster cActive : clusterBegin[i])
			activeClusters.insert(cActive);

		// create compound tree nodes
		for(cluster cl : activeClusters.clusters()) {
			clusterToTreeNode[cl] = new LHTreeNode(cl, clusterToTreeNode[cl]);
			if(cl != m_CGC.rootCluster())
				++numChildren[cl->parent()];
		}

		// initialize child arrays
		for(cluster cl : activeClusters.clusters()) {
			clusterToTreeNode[cl]->initChild(numChildren[cl]);
		}

		// set parent and children of compound tree nodes
		for(cluster cl : activeClusters.clusters()) {
			if(cl != m_CGC.rootCluster()) {
				LHTreeNode *cNode = clusterToTreeNode[cl];
				LHTreeNode *pNode = clusterToTreeNode[cl->parent()];

				cNode->setParent(pNode);
				pNode->setChild(--numChildren[cl->parent()], cNode);
			}
		}

		// set root of layer
		m_layer[i].setRoot(clusterToTreeNode[m_CGC.rootCluster()]);

		// create tree nodes for nodes on this layer
		for(node v : L[i]) {
			LHTreeNode *cNode = clusterToTreeNode[parent(v)];
			LHTreeNode::Type type = (m_type[v] == NodeType::ClusterTopBottom) ?
				LHTreeNode::Type::AuxNode : LHTreeNode::Type::Node;
			LHTreeNode *vNode =  new LHTreeNode(cNode, v, type);
			treeNode[v] = vNode;
			cNode->setChild(--numChildren[parent(v)], vNode);
		}

		// clean-up
		for(cluster cl : activeClusters.clusters()) {
			numChildren[cl] = 0;
		}

		// identify clusters that are not on next layer
		for(cluster cActive : clusterEnd[i])
			activeClusters.remove(cActive);
	}

	// identify adjacencies between nodes and tree nodes
	for(edge e : edges)
	{
		node u = e->source();
		node v = e->target();
		bool isTopBottomEdge = (origEdge(e) == nullptr);
		int weight = (isTopBottomEdge) ? 100 : 1;

		if(isTopBottomEdge)
			continue;

		LHTreeNode *nd = treeNode[v];
		LHTreeNode *parent = nd->parent();
		if(isTopBottomEdge) {
			nd = parent;
			parent = parent->parent();
		}

		while(parent != nullptr) {
			parent->m_upperAdj.pushBack(LHTreeNode::Adjacency(u,nd,weight));

			nd = parent;
			parent = parent->parent();
		}

		nd = treeNode[u];
		parent = nd->parent();
		if(isTopBottomEdge) {
			nd = parent;
			parent = parent->parent();
		}

		while(parent != nullptr) {
			parent->m_lowerAdj.pushBack(LHTreeNode::Adjacency(v,nd,weight));

			nd = parent;
			parent = parent->parent();
		}
	}

	for(i = 0; i < m_numLayers; ++i)
		m_layer[i].simplifyAdjacencies();


	// identify relevant pairs for crossings between top->bottom edges
	// and foreign edges
	m_markTree.init(m_CGC,nullptr);
	ClusterArray<List<tuple<edge,LHTreeNode*,LHTreeNode*> > > edgeArray(m_CGC);
	ClusterSetSimple C(m_CGC);
	for(i = 0; i < m_numLayers-1; ++i)
	{
		for(node u : L[i]) {
			for(adjEntry adj : u->adjEntries) {
				edge e = adj->theEdge();
				if(origEdge(e) == nullptr)
					continue;
				if(e->source() == u) {
					node v = e->target();

					LHTreeNode *uChild, *vChild;
					cluster cl = lca(treeNode[u], treeNode[v], &uChild, &vChild)->originalCluster();

					edgeArray[cl].pushBack(std::make_tuple(e,uChild,vChild));
					C.insert(c);
				}
			}
		}

		for(node u : L[i]) {
			for(adjEntry adj : u->adjEntries) {
				edge e = adj->theEdge();
				if(e->source() == u && origEdge(e) == nullptr) {
					LHTreeNode *aNode = treeNode[e->target()];
					cluster ca = aNode->parent()->originalCluster();
					LHTreeNode *aParent = aNode->parent()->parent();

					for(; aParent != nullptr; aParent = aParent->parent()) {
						for(const auto &tup : edgeArray[aParent->originalCluster()])
						{
							edge e_tup = std::get<0>(tup);

							LHTreeNode *aChild, *vChild, *h1, *h2;
							LHTreeNode *cNode = lca(aNode, treeNode[e_tup->target()], &aChild, &vChild);
							if(cNode != aNode->parent() &&
								lca(aNode,treeNode[e_tup->source()],&h1,&h2)->originalCluster() != ca)
								cNode->m_upperClusterCrossing.pushBack(
									LHTreeNode::ClusterCrossing(e->source(), aChild, e_tup->source(), vChild, e_tup));
						}
					}

					aNode = treeNode[e->source()];
					ca = aNode->parent()->originalCluster();
					aParent = aNode->parent()->parent();

					for(; aParent != nullptr; aParent = aParent->parent()) {
						for(const auto &tup : edgeArray[aParent->originalCluster()])
						{
							edge e_tup = std::get<0>(tup);

							LHTreeNode *aChild, *vChild, *h1, *h2;
							LHTreeNode *cNode = lca(aNode, treeNode[e_tup->source()],
								&aChild, &vChild);
							if(cNode != aNode->parent() &&
								lca(aNode,treeNode[e_tup->target()],&h1,&h2)->originalCluster() != ca)
								cNode->m_lowerClusterCrossing.pushBack(
									LHTreeNode::ClusterCrossing(e->target(), aChild, e_tup->target(), vChild, e_tup));
						}
					}
				}
			}
		}

		// get rid of edges in edgeArray[c]
		for(cluster cl : C.clusters())
			edgeArray[cl].clear();
		C.clear();
	}

	// clean-up
	m_markTree.init();
}


void ExtendedNestingGraph::storeCurrentPos()
{
	for(int i = 0; i < m_numLayers; ++i)
		m_layer[i].store();
}


void ExtendedNestingGraph::restorePos()
{
	for(int i = 0; i < m_numLayers; ++i) {
		m_layer[i].restore();

		int count = 0;
		assignPos(m_layer[i].root(), count);
	}
}


void ExtendedNestingGraph::permute()
{
	for(int i = 0; i < m_numLayers; ++i)
		m_layer[i].permute();

	int count = 0;
	assignPos(m_layer[0].root(), count);
}


RCCrossings ExtendedNestingGraph::reduceCrossings(int i, bool dirTopDown)
{
	//std::cout << "Layer " << i << ":\n";
	LHTreeNode *root = m_layer[i].root();

	ArrayBuffer<LHTreeNode*> S;
	S.push(root);

	RCCrossings numCrossings;
	while(!S.empty()) {
		LHTreeNode *cNode = S.popRet();
		numCrossings += reduceCrossings(cNode, dirTopDown);

		for(int j = 0; j < cNode->numberOfChildren(); ++j) {
			if(cNode->child(j)->isCompound())
				S.push(cNode->child(j));
		}
	}

	// set positions
	int count = 0;
	assignPos(root, count);

	return numCrossings;
}


struct RCEdge
{
	RCEdge() { }
	RCEdge(node src, node tgt, RCCrossings cr, RCCrossings crReverse)
		: m_src(src), m_tgt(tgt), m_cr(cr), m_crReverse(crReverse) { }

	RCCrossings weight() const { return m_crReverse - m_cr; }

	node        m_src;
	node        m_tgt;
	RCCrossings m_cr;
	RCCrossings m_crReverse;
};


class LocationRelationshipComparer
{
public:
	static int compare(const RCEdge &x, const RCEdge &y)
	{
		return RCCrossings::compare(x.weight(), y.weight());
	}
	OGDF_AUGMENT_STATICCOMPARER(RCEdge)
};


bool ExtendedNestingGraph::tryEdge(node u, node v, Graph &G, NodeArray<int> &level)
{
	const int n = G.numberOfNodes();

	if(level[u] == -1) {
		if(level[v] == -1) {
			level[v] = n;
			level[u] = n-1;
		} else
			level[u] = level[v]-1;

	} else if(level[v] == -1)
		level[v] = level[u]+1;

	else if(level[u] >= level[v]) {
		SListPure<node> successors;
		if(reachable(v, u, successors))
			return false;
		else {
			level[v] = level[u] + 1;
			moveDown(v, successors, level);
		}
	}

	G.newEdge(u,v);

	return true;
}


RCCrossings ExtendedNestingGraph::reduceCrossings(LHTreeNode *cNode, bool dirTopDown)
{
	const int n = cNode->numberOfChildren();
	if(n < 2)
		return RCCrossings(); // nothing to do

	cNode->setPos();

	// Build
	// crossings matrix
	Array2D<RCCrossings> cn(0,n-1,0,n-1);

	// crossings between adjacency edges
	Array<List<LHTreeNode::Adjacency> > adj(n);
	ListConstIterator<LHTreeNode::Adjacency> it;
	for(it = (dirTopDown) ? cNode->m_upperAdj.begin() : cNode->m_lowerAdj.begin(); it.valid(); ++it)
		adj[(*it).m_v->pos()].pushBack(*it);

	int j;
	for(j = 0; j < n; ++j) {
		for (const LHTreeNode::Adjacency &adjJ : adj[j]) {
			int posJ = m_pos[(adjJ).m_u];

			for(int k = j+1; k < n; ++k) {
				for (const LHTreeNode::Adjacency &adjK : adj[k]) {
					int posK   = m_pos[adjK.m_u];
					int weight = adjJ.m_weight * adjK.m_weight;

					if(posJ > posK)
						cn(j,k).incEdges(weight);
					if(posK > posJ)
						cn(k,j).incEdges(weight);
				}
			}
		}
	}

	// crossings between clusters and foreign adjacency edges
	ListConstIterator<LHTreeNode::ClusterCrossing> itCC;
	for(itCC = (dirTopDown) ?
		cNode->m_upperClusterCrossing.begin() : cNode->m_lowerClusterCrossing.begin();
		itCC.valid(); ++itCC)
	{
		/*std::cout << "crossing: C" << m_CGC.clusterOf((*itCC).m_edgeCluster->source()) <<
			" e=" << (*itCC).m_edgeCluster << "  with edge " << (*itCC).m_edge <<
			" cluster: C" << m_CGC.clusterOf((*itCC).m_edge->source()) <<
			", C" << m_CGC.clusterOf((*itCC).m_edge->target()) << "\n";*/

		int cPos = (*itCC).m_cNode->pos();
		int uPos = (*itCC).m_uNode->pos();

		int posJ = m_pos[(*itCC).m_uc];
		int posK = m_pos[(*itCC).m_u];

		OGDF_ASSERT(cPos != uPos);
		OGDF_ASSERT(posJ != posK);

		if(posJ > posK)
			cn(cPos,uPos).incClusters();
		else
			cn(uPos,cPos).incClusters();
	}


	Graph G; // crossing reduction graph
	NodeArray<int>  level(G,-1);
	m_aeVisited.init(G,false);
	m_auxDeg.init(G,0);

	// create nodes
	NodeArray<LHTreeNode*> fromG(G);
	Array<node>            toG(n);

	for(j = 0; j < n; ++j)
		fromG[toG[j] = G.newNode()] = cNode->child(j);

	// create edges for l-r constraints
	const LHTreeNode *neighbourParent = (dirTopDown) ? cNode->up() : cNode->down();
	if(neighbourParent != nullptr) {
		node src = nullptr;
		for(int i = 0; i < neighbourParent->numberOfChildren(); ++i) {
			const LHTreeNode *vNode =
				(dirTopDown) ?
				neighbourParent->child(i)->down() : neighbourParent->child(i)->up();

			if(vNode != nullptr) {
				node tgt = toG[vNode->pos()];
				if(src != nullptr) {
#ifdef OGDF_DEBUG
					bool result =
#endif
						tryEdge(src, tgt, G, level);
					OGDF_ASSERT(result);
				}
				src = tgt;
			}
		}
	}

	// list of location relationships
	List<RCEdge> edgeList;
	for(j = 0; j < n; ++j)
		for(int k = j+1; k < n; ++k) {
			if(cn(j,k) <= cn(k,j))
				edgeList.pushBack(RCEdge(toG[j], toG[k], cn(j,k), cn(k,j)));
			else
				edgeList.pushBack(RCEdge(toG[k], toG[j], cn(k,j), cn(j,k)));
		}

	// sort list according to weights
	LocationRelationshipComparer cmp;
	edgeList.quicksort(cmp);

	// build acyclic graph
	RCCrossings numCrossings;
	for(const RCEdge &rce : edgeList)
	{
		node u = rce.m_src;
		node v = rce.m_tgt;

		if(tryEdge(u, v, G, level)) {
			numCrossings += rce.m_cr;

		} else {
			numCrossings += rce.m_crReverse;
		}
	}

	OGDF_ASSERT(isAcyclic(G));

	// sort nodes in G topological
	topologicalNumbering(G,level);

	// sort children of cNode according to topological numbering
	for(node v : G.nodes)
		cNode->setChild(level[v], fromG[v]);

#if 0
	for(j = 0; j < n; ++j) {
		LHTreeNode *vNode = cNode->child(j);
	}
#endif

	return numCrossings;
}

void ExtendedNestingGraph::assignPos(const LHTreeNode *vNode, int &count)
{
	if(vNode->isCompound()) {
		for(int i = 0; i < vNode->numberOfChildren(); ++i)
			assignPos(vNode->child(i), count);

	} else {
		m_pos[vNode->getNode()] = count++;
	}
}


void ExtendedNestingGraph::removeAuxNodes()
{
	for(int i = 0; i < m_numLayers; ++i)
		m_layer[i].removeAuxNodes();
}


void ExtendedNestingGraph::removeTopBottomEdges()
{
	// compute m_vertical
	m_vertical.init(*this);

#if 0
	cluster rootOrig = getOriginalClusterGraph().rootCluster();
#endif
	for(edge e : edges)
	{
		if(origEdge(e) == nullptr)
			continue;

		bool vert = false;
		node u = e->source();
		node v = e->target();

		// if we do not use virtual clusters, cu and cv are simply the
		// clusters containing u and v (=> no while-loop required)
		cluster cu = parent(u);
		while(isVirtual(cu))
			cu = cu->parent();
		cluster cv = parent(v);
		while(isVirtual(cv))
			cv = cv->parent();

		if(isLongEdgeDummy(u) && isLongEdgeDummy(v)) {
			if(cu != cv) {
				cluster cuOrig = m_CGC.original(cu);
				cluster cvOrig = m_CGC.original(cv);
				cluster cuOrigParent = cuOrig->parent();
				cluster cvOrigParent = cvOrig->parent();

				if((cvOrig == cuOrigParent && rank(u) == rank(bottom(cuOrig))) ||
					(cuOrig == cvOrigParent && rank(v) == rank(top   (cvOrig))) ||
					(cuOrigParent == cvOrigParent &&
					rank(u) == rank(bottom(cuOrig)) &&
					rank(v) == rank(top   (cvOrig))
					))
				{
					vert = true;
				}
			} else
				vert = true;
		}

		m_vertical[e] = vert;
	}

	for(int i = 1; i < m_numLayers; ++i)
	{
		LHTreeNode *root = m_layer[i].root();

		ArrayBuffer<LHTreeNode*> S;
		S.push(root);

		while(!S.empty()) {
			LHTreeNode *cNode = S.popRet();

			cNode->setPos();
			for (const LHTreeNode::ClusterCrossing &cc : cNode->m_upperClusterCrossing) {
				int j = cc.m_cNode->pos();
				int k = cc.m_uNode->pos();

				int posJ = m_pos[cc.m_uc];
				int posK = m_pos[cc.m_u];

				OGDF_ASSERT(j != k);
				OGDF_ASSERT(posJ != posK);

				// do we have a cluster-edge crossing?
				if((j < k && posJ > posK) || (j > k && posJ < posK))
					m_vertical[cc.m_edge] = false;
			}


			for(int j = 0; j < cNode->numberOfChildren(); ++j) {
				if(cNode->child(j)->isCompound())
					S.push(cNode->child(j));
			}
		}
	}

	// delete nodes in hierarchy tree
	removeAuxNodes();

	// delete nodes in graph
	node v, vNext;
	for(v = firstNode(); v != nullptr; v = vNext) {
		vNext = v->succ();
		if(type(v) == NodeType::ClusterTopBottom)
			delNode(v);
	}
}


cluster ExtendedNestingGraph::lca(node u, node v) const
{
	const ClusterGraph &CG = getOriginalClusterGraph();

	for(cluster c : m_markedClustersTree)
		m_mark[c] = nullptr;
	m_markedClustersTree.clear();

	cluster c1 = CG.clusterOf(u);
	cluster pred1 = c1;
	cluster c2 = CG.clusterOf(v);
	cluster pred2 = c2;

	for( ; ; ) {
		if(c1 != nullptr) {
			if(m_mark[c1] != nullptr) {
				m_secondPath = pred1;
				m_secondPathTo = u;
				return c1;

			} else {
				m_mark[c1] = pred1;
				pred1 = c1;
				m_markedClustersTree.pushBack(c1);
				c1 = c1->parent();
			}
		}
		if(c2 != nullptr) {
			if(m_mark[c2] != nullptr) {
				m_secondPath = pred2;
				m_secondPathTo = v;
				return c2;

			} else {
				m_mark[c2] = pred2;
				pred2 = c2;
				m_markedClustersTree.pushBack(c2);
				c2 = c2->parent();
			}
		}
	}
}


LHTreeNode *ExtendedNestingGraph::lca(
	LHTreeNode *uNode,
	LHTreeNode *vNode,
	LHTreeNode **uChild,
	LHTreeNode **vChild) const
{
	OGDF_ASSERT(!uNode->isCompound());
	OGDF_ASSERT(!vNode->isCompound());

	for(cluster c : m_markedClusters)
		m_markTree[c] = nullptr;
	m_markedClusters.clear();

	LHTreeNode *cuNode = uNode->parent();
	LHTreeNode *cvNode = vNode->parent();

	LHTreeNode *uPred = uNode;
	LHTreeNode *vPred = vNode;

	while(cuNode != nullptr || cvNode != nullptr) {
		if(cuNode != nullptr) {
			if(m_markTree[cuNode->originalCluster()] != nullptr) {
				*uChild = uPred;
				*vChild = m_markTree[cuNode->originalCluster()];
				return cuNode;

			} else {
				m_markTree[cuNode->originalCluster()] = uPred;
				uPred = cuNode;
				m_markedClusters.pushBack(cuNode->originalCluster());
				cuNode = cuNode->parent();
			}
		}
		if(cvNode != nullptr) {
			if(m_markTree[cvNode->originalCluster()] != nullptr) {
				*uChild = m_markTree[cvNode->originalCluster()];
				*vChild = vPred;
				return cvNode;

			} else {
				m_markTree[cvNode->originalCluster()] = vPred;
				vPred = cvNode;
				m_markedClusters.pushBack(cvNode->originalCluster());
				cvNode = cvNode->parent();
			}
		}
	}

	return nullptr; // error; not found!
}


void ExtendedNestingGraph::assignAeLevel(cluster c, int &count)
{
	m_aeLevel[m_topNode[c]] = count++;

	ListConstIterator<node> itV;
	for(itV = c->nBegin(); itV.valid(); ++itV)
		m_aeLevel[m_copy[*itV]] = count++;

	for(cluster child : c->children)
		assignAeLevel(child, count);

	m_aeLevel[m_bottomNode[c]] = count++;
}


bool ExtendedNestingGraph::reachable(node v, node u, SListPure<node> &successors)
{
	if(u == v)
		return true;

	SListPure<node> Q;
	m_aeVisited[v] = true;
	Q.pushBack(v);

	while(!Q.empty())
	{
		node w = Q.popFrontRet();
		successors.pushBack(w);

		for(adjEntry adj : w->adjEntries) {
			edge e = adj->theEdge();
			node t = e->target();

			if(t == u) {
				// we've found u, so we do not need the list of successors
				Q.conc(successors);

				// reset all visited entries
				for(node vi : Q)
					m_aeVisited[vi] = false;

				return true;
			}

			if(!m_aeVisited[t]) {
				m_aeVisited[t] = true;
				Q.pushBack(t);
			}
		}
	}

	// reset all visited entries
	for(node vi : successors)
		m_aeVisited[vi] = false;

	return false;
}


void ExtendedNestingGraph::moveDown(node v, const SListPure<node> &successors, NodeArray<int> &level)
{
	for(node vi : successors) {
		m_aeVisited[vi] = true;
		m_auxDeg[vi] = 0;
	}

	for(node vi : successors) {
		for(adjEntry adj : vi->adjEntries) {
			edge e = adj->theEdge();
			node s = e->source();
			if(s != vi && m_aeVisited[s])
				++m_auxDeg[vi];
		}
	}

	SListPure<node> Q;
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		node t = e->target();
		if (t != v && --m_auxDeg[t] == 0) {
			Q.pushBack(t);
		}
	}

	while(!Q.empty())
	{
		node w = Q.popFrontRet();

		int maxLevel = 0;
		for(adjEntry adj : w->adjEntries) {
			edge e = adj->theEdge();
			node s = e->source();
			node t = e->target();

			if (s != w) {
				Math::updateMax(maxLevel, level[s]);
			}
			if (t != w && --m_auxDeg[t] == 0) {
				Q.pushBack(t);
			}
		}

		level[w] = maxLevel+1;
	}

	for(node vi : successors) {
		m_aeVisited[vi] = false;
	}
}


edge ExtendedNestingGraph::addEdge(node u, node v, bool addAlways)
{
	if(m_aeLevel[u] < m_aeLevel[v])
		return newEdge(u,v);

	SListPure<node> successors;
	if(!reachable(v, u, successors)) {
		int d = m_aeLevel[u] - m_aeLevel[v] + 1;
		OGDF_ASSERT(d > 0);

		for(node vi : successors)
			m_aeLevel[vi] += d;

		return newEdge(u,v);

	} else if(addAlways)
		return newEdge(v,u);

	return nullptr;
}

}
