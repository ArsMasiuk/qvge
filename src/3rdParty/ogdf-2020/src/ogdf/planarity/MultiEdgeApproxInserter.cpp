/** \file
 * \brief implements class MultiEdgeApproxInserter
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


#include <ogdf/planarity/MultiEdgeApproxInserter.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/planarity/FixedEmbeddingInserter.h>
#include <ogdf/planarity/VariableEmbeddingInserter.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/tuples.h>


namespace ogdf {

class MultiEdgeApproxInserter::EmbeddingPreference
{
public:
	enum class Type { None, RNode, PNode };

	// constructs an embedding preference of type none (= irrelevant)
	EmbeddingPreference() : m_type(Type::None), m_mirror(false) { }

	// constructs an embedding preference for an R-node
	explicit EmbeddingPreference(bool mirror) : m_type(Type::RNode), m_mirror(mirror) { }

	EmbeddingPreference(adjEntry a1, adjEntry a2) : m_type(Type::PNode), m_mirror(false), m_adj1(a1), m_adj2(a2) { }
	// constructs an embedding preference for a P-node

	Type type() const { return m_type; }
	bool isNull() const { return m_type == Type::None; }

	bool mirror() const { return m_mirror; }
	adjEntry adj1() const { return m_adj1; }
	adjEntry adj2() const { return m_adj2; }

	// flip embedding preference
	void flip() {
		m_mirror = !m_mirror;
		std::swap(m_adj1, m_adj2);
	}

	static const EmbeddingPreference s_none;

	std::ostream &print(std::ostream &os) const {
		switch(type()) {
		case MultiEdgeApproxInserter::EmbeddingPreference::Type::None:
			os << "none";
			break;
		case MultiEdgeApproxInserter::EmbeddingPreference::Type::PNode:
			os << "PNode: " << adj1()->index() << "->" << adj2()->index();
			break;
		case MultiEdgeApproxInserter::EmbeddingPreference::Type::RNode:
			os << "RNode: " << mirror();
			break;
		}
		return os;
	}

private:
	Type m_type;
	bool m_mirror;  // skeleton of an R-node must be mirrored (yes/no)

	adjEntry m_adj1, m_adj2;
	// these two adj entries of first node in skeleton of a P-node must be in clockwise order adj1 -> adj2
};

const MultiEdgeApproxInserter::EmbeddingPreference MultiEdgeApproxInserter::EmbeddingPreference::s_none;


class MultiEdgeApproxInserter::Block : public Graph
{
public:
	struct SPQRPath {
		SPQRPath() : m_start(nullptr) { }
		node       m_start;  // first node in SPQR-tree (its skeleton contains v1)
		List<edge> m_edges;  // actual path (empty if v1 and v2 are in skeleton of m_start)
		List<EmbeddingPreference> m_prefs;  // embeding preferences along the path
	};

	struct PathElement {
		PathElement() : m_node(nullptr), m_pref(&EmbeddingPreference::s_none) { }

		node m_node;
		const EmbeddingPreference *m_pref;
	};


	// constructor
	Block() :  m_spqr(nullptr), m_embB(nullptr), m_dualB(nullptr), m_faceNodeB(nullptr), m_primalAdjB(nullptr), m_vS(nullptr), m_vT(nullptr)
	{
		m_BCtoG.init(*this);
		m_cost .init(*this,1);
	}

	// destructoe
	~Block() {
		delete m_primalAdjB;
		delete m_faceNodeB;
		delete m_dualB;
		delete m_embB;
		delete m_spqr;
	}

	// returns true iff block is just a bridge (in this case, there is no SPQR-tree!)
	bool isBridge() const { return numberOfEdges() < 3; }

	int cost(edge e) const { return m_cost[e]; }

	// initialize SPQR-tree; compute allocation nodes
	void initSPQR(int m);

	// returns SPQR-tree
	const StaticPlanarSPQRTree &spqr() const { return *m_spqr; }
	StaticPlanarSPQRTree &spqr() { return *m_spqr; }

	// compute traversing costs in skeleton of n; omit skeleton edges e1, e2
	void computeTraversingCosts(node n, edge e1, edge e2);

	int findShortestPath(node n, edge eRef);

	// compute costs of a subpath through skeleton(n) while connecting s with t
	int costsSubpath(node n, edge eIn, edge eOut, node s, node t, PathDir &dirFrom, PathDir &dirTo);

	void pathToArray(int i, Array<PathElement> &path);

	bool embPrefAgree(node n, const EmbeddingPreference &p_pick, const EmbeddingPreference &p_e);

	bool switchingPair(node n, node m,
		const EmbeddingPreference &p_pick_n, const EmbeddingPreference &p_n,
		const EmbeddingPreference &p_pick_m, const EmbeddingPreference &p_m);

	int findBestFaces(node s, node t, adjEntry &adj_s, adjEntry &adj_t);
	adjEntry findBestFace(node s, node t, int &len);

	AdjEntryArray<adjEntry> m_BCtoG; //!< maps adjacency entries in block to original graph
	EdgeArray<int> m_cost; //!< costs of an edge (as given for edges in original graph)
	NodeArray<ArrayBuffer<node>> m_allocNodes; //!< allocation nodes
	Array<SPQRPath> m_pathSPQR; //!< insertion path in SPQR-tree

private:
	struct RNodeInfo {
		RNodeInfo() : m_emb(nullptr), m_dual(nullptr), m_faceNode(nullptr), m_primalAdj(nullptr) { }
		~RNodeInfo() {
			delete m_primalAdj;
			delete m_faceNode;
			delete m_dual;
			delete m_emb;
		}

		ConstCombinatorialEmbedding *m_emb;       // combinatorial embedding of skeleton graph
		Graph                       *m_dual;      // dual graph
		FaceArray<node>             *m_faceNode;  // mapping dual node -> face
		AdjEntryArray<adjEntry>     *m_primalAdj; // mapping dual adjEntry -> primal adjEntry
	};

	int recTC(node n, edge eRef);
	void constructDual(node n);

public:
	void constructDualBlock();
private:

	StaticPlanarSPQRTree *m_spqr;
	NodeArray<EdgeArray<int> > m_tc;   // traversing costs

	NodeArray<RNodeInfo> m_info;  // additional data for R-node skeletons

	ConstCombinatorialEmbedding *m_embB;
	Graph                       *m_dualB;
	FaceArray<node>             *m_faceNodeB;
	AdjEntryArray<adjEntry>     *m_primalAdjB;
	node                         m_vS, m_vT;
};


void MultiEdgeApproxInserter::Block::pathToArray(int i, Array<PathElement> &path)
{
	SPQRPath &sp = m_pathSPQR[i];

	if(sp.m_start == nullptr) {
		path.init();
		return;
	}

	path.init(1+sp.m_edges.size());

	ListConstIterator<edge> itE = sp.m_edges.begin();
	ListConstIterator<EmbeddingPreference> itP = sp.m_prefs.begin();

	node n = sp.m_start;

	path[0].m_node = n;
	if(m_spqr->typeOf(n) != SPQRTree::NodeType::SNode)
		path[0].m_pref = &(*itP++);

	int j;
	for(j = 1; itE.valid(); ++j)
	{
		n = (*itE++)->opposite(n);
		path[j].m_node = n;

		if(m_spqr->typeOf(n) != SPQRTree::NodeType::SNode)
			path[j].m_pref = &(*itP++);
	}

	OGDF_ASSERT(j == path.size());
}


bool MultiEdgeApproxInserter::Block::embPrefAgree(node n, const EmbeddingPreference &p_pick, const EmbeddingPreference &p_e)
{
	switch(m_spqr->typeOf(n)) {
	case SPQRTree::NodeType::RNode:
		return p_pick.mirror() == p_e.mirror();  // check if mirroring is the same

	case SPQRTree::NodeType::PNode:
		// if p_e okay: check if adj entries in (embedded) P-Node are in the right order
		return p_e.isNull() ? true : p_e.adj1()->cyclicSucc() == p_e.adj2();

	default:
		return true;  // any other case "agrees"
	}
}


bool MultiEdgeApproxInserter::Block::switchingPair(
	node n, node m,
	const EmbeddingPreference &p_pick_n, const EmbeddingPreference &p_n,
	const EmbeddingPreference &p_pick_m, const EmbeddingPreference &p_m)
{
	EmbeddingPreference p_n_f = p_n;
	EmbeddingPreference p_m_f = p_m;

	p_n_f.flip();
	p_m_f.flip();

	return
		(embPrefAgree(n, p_pick_n, p_n) && embPrefAgree(m, p_pick_m, p_m_f)) ||
		(embPrefAgree(n, p_pick_n, p_n_f) && embPrefAgree(m, p_pick_m, p_m));
}


// create SPQR-tree and compute allocation nodes
void MultiEdgeApproxInserter::Block::initSPQR(int m)
{
	if(m_spqr == nullptr) {
		m_spqr = new StaticPlanarSPQRTree(*this,true);
		m_pathSPQR.init(m);

		const Graph &tree = m_spqr->tree();
		m_tc.init(tree);
		m_info.init(tree);

		// compute allocation nodes
		m_allocNodes.init(*this);

		for(node n : tree.nodes)
		{
			const Skeleton &S = m_spqr->skeleton(n);
			const Graph &M = S.getGraph();

			EdgeArray<int> &tcS = m_tc[n];
			tcS.init(M,-1);

			for(node x : M.nodes)
				m_allocNodes[S.original(x)].push(n);

			for(edge e : M.edges) {
				edge eOrig = S.realEdge(e);
				if(eOrig != nullptr) tcS[e] = 1;
			}
		}
	}
}


// construct dual graph of skeleton of n
void MultiEdgeApproxInserter::Block::constructDual(node n)
{
	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_spqr->skeleton(n));
	const Graph &M = S.getGraph();

	OGDF_ASSERT(m_info[n].m_dual == nullptr);

	ConstCombinatorialEmbedding *emb = m_info[n].m_emb = new ConstCombinatorialEmbedding(M);
	Graph *dual = m_info[n].m_dual = new Graph;
	FaceArray<node> *faceNode = m_info[n].m_faceNode = new FaceArray<node>(*emb);
	AdjEntryArray<adjEntry> *primalAdj = m_info[n].m_primalAdj = new AdjEntryArray<adjEntry>(*dual);

	// constructs nodes (for faces in emb)
	for(face f : emb->faces) {
		(*faceNode)[f] = dual->newNode();
	}

	// construct dual edges (for primal edges in M)
	for(node v : M.nodes)
	{
		for(adjEntry adj : v->adjEntries)
		{
			if(adj->index() & 1) {
				node vLeft  = (*faceNode)[emb->leftFace (adj)];
				node vRight = (*faceNode)[emb->rightFace(adj)];

				edge eDual = dual->newEdge(vLeft,vRight);
				(*primalAdj)[eDual->adjSource()] = adj;
				(*primalAdj)[eDual->adjTarget()] = adj->twin();
			}
		}
	}
}


void MultiEdgeApproxInserter::Block::constructDualBlock()
{
	m_embB = new ConstCombinatorialEmbedding(*this);
	m_dualB = new Graph;
	m_faceNodeB = new FaceArray<node>(*m_embB);
	m_primalAdjB = new AdjEntryArray<adjEntry>(*m_dualB);

	// constructs nodes (for faces in m_embB)
	for(face f : m_embB->faces) {
		(*m_faceNodeB)[f] = m_dualB->newNode();
	}

	// construct dual edges (for primal edges in block)
	for(node v : nodes)
	{
		for(adjEntry adj : v->adjEntries)
		{
			if(adj->index() & 1) {
				node vLeft  = (*m_faceNodeB)[m_embB->leftFace (adj)];
				node vRight = (*m_faceNodeB)[m_embB->rightFace(adj)];

				edge eDual = m_dualB->newEdge(vLeft,vRight);
				(*m_primalAdjB)[eDual->adjSource()] = adj;
				(*m_primalAdjB)[eDual->adjTarget()] = adj->twin();
			}
		}
	}

	m_vS = m_dualB->newNode();
	m_vT = m_dualB->newNode();
}


adjEntry MultiEdgeApproxInserter::Block::findBestFace(node s, node t, int &len)
{
	if(isBridge()) {
		len = 0;
		return s->firstAdj();
	}

	adjEntry adj_s, adj_t;
	len = findBestFaces(s,t,adj_s,adj_t);
	return adj_s;
}


int MultiEdgeApproxInserter::Block::findBestFaces(
	node s, node t, adjEntry &adj_s, adjEntry &adj_t)
{
	if(m_dualB == nullptr)
		constructDualBlock();

	NodeArray<adjEntry> spPred(*m_dualB, nullptr);
	QueuePure<adjEntry> queue;
	int oldIdCount = m_dualB->maxEdgeIndex();

	// augment dual by edges from s to all adjacent faces of s ...
	for(adjEntry adj : s->adjEntries) {
		// starting edges of bfs-search are all edges leaving s
		edge eDual = m_dualB->newEdge(m_vS, (*m_faceNodeB)[m_embB->rightFace(adj)]);
		(*m_primalAdjB)[eDual->adjSource()] = adj;
		(*m_primalAdjB)[eDual->adjTarget()] = nullptr;
		queue.append(eDual->adjSource());
	}

	// ... and from all adjacent faces of t to t
	for(adjEntry adj : t->adjEntries) {
		edge eDual = m_dualB->newEdge((*m_faceNodeB)[m_embB->rightFace(adj)], m_vT);
		(*m_primalAdjB)[eDual->adjSource()] = adj;
		(*m_primalAdjB)[eDual->adjTarget()] = nullptr;
	}

	// actual search (using bfs on directed dual)
	int len = -2;
	for( ; ;)
	{
		// next candidate edge
		adjEntry adjCand = queue.pop();
		node v = adjCand->twinNode();

		// leads to an unvisited node?
		if (spPred[v] == nullptr)
		{
			// yes, then we set v's predecessor in search tree
			spPred[v] = adjCand;

			// have we reached t ...
			if (v == m_vT)
			{
				// ... then search is done.
				adj_t = (*m_primalAdjB)[adjCand];

				adjEntry adj;
				do {
					adj = spPred[v];
					++len;
					v = adj->theNode();
				} while(v != m_vS);
				adj_s = (*m_primalAdjB)[adj];

				break;
			}

			// append next candidate edges to queue
			// (all edges leaving v)
			for(adjEntry adj : v->adjEntries) {
				if(adj->twinNode() != m_vS)
					queue.append(adj);
			}
		}
	}

	// remove augmented edges again
	adjEntry adj;
	while ((adj = m_vS->firstAdj()) != nullptr)
		m_dualB->delEdge(adj->theEdge());

	while ((adj = m_vT->firstAdj()) != nullptr)
		m_dualB->delEdge(adj->theEdge());

	m_dualB->resetEdgeIdCount(oldIdCount);

	return len;
}


// compute traversing costs in skelton
int MultiEdgeApproxInserter::Block::recTC(node n, edge eRef)
{
	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_spqr->skeleton(n));
	const Graph &M = S.getGraph();
	EdgeArray<int> &tcS = m_tc[n];

	for(edge e : M.edges) {
		if(tcS[e] == -1 && e != eRef) {
			edge eT = S.treeEdge(e);

			node nC;
			edge eRefC;
			if(n == eT->source()) {
				nC = eT->target(); eRefC = m_spqr->skeletonEdgeTgt(eT);
			} else {
				nC = eT->source(); eRefC = m_spqr->skeletonEdgeSrc(eT);
			}

			tcS[e] = recTC(nC,eRefC);
		}
	}

	int c = 1;
	switch(m_spqr->typeOf(n))
	{
	case SPQRTree::NodeType::SNode:
		c = std::numeric_limits<int>::max();
		for(edge e : M.edges)
			if(e != eRef) c = min(c, tcS[e]);
		break;

	case SPQRTree::NodeType::PNode:
		c = 0;
		for(edge e : M.edges)
			if(e != eRef) c += tcS[e];
		break;

	case SPQRTree::NodeType::RNode:
		if(m_info[n].m_dual == nullptr)
			constructDual(n);

		c = findShortestPath(n, eRef);
		break;
	}

	return c;
}

void MultiEdgeApproxInserter::Block::computeTraversingCosts(node n, edge e1, edge e2)
{
	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_spqr->skeleton(n));
	const Graph &M = S.getGraph();
	EdgeArray<int> &tcS = m_tc[n];

	for(edge e : M.edges) {
		if(tcS[e] == -1 && e != e1 && e != e2) {
			edge eT = S.treeEdge(e);

			node nC;
			edge eRef;
			if(n == eT->source()) {
				nC = eT->target(); eRef = m_spqr->skeletonEdgeTgt(eT);
			} else {
				nC = eT->source(); eRef = m_spqr->skeletonEdgeSrc(eT);
			}

			tcS[e] = recTC(nC,eRef);
		}
	}
}


int  MultiEdgeApproxInserter::Block::findShortestPath(node n, edge eRef)
{
	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_spqr->skeleton(n));
	const Graph &M = S.getGraph();
	const EdgeArray<int> &tcS = m_tc[n];

	const Graph &dual = *m_info[n].m_dual;
	const ConstCombinatorialEmbedding &emb = *m_info[n].m_emb;
	const FaceArray<node> faceNode = *m_info[n].m_faceNode;
	const AdjEntryArray<adjEntry> primalAdj = *m_info[n].m_primalAdj;

	int maxTC = 0;
	for(edge e : M.edges)
		Math::updateMax(maxTC, tcS[e]);

	++maxTC;
	Array<SListPure<adjEntry> > nodesAtDist(maxTC);

	NodeArray<adjEntry> spPred(dual,nullptr); // predecessor in shortest path tree

	node vS = faceNode[emb.rightFace(eRef->adjSource())];
	node vT = faceNode[emb.rightFace(eRef->adjTarget())];

	// start with all edges leaving from vS
	for(adjEntry adj : vS->adjEntries) {
		edge eOrig = primalAdj[adj]->theEdge();
		if(eOrig != eRef)
			nodesAtDist[tcS[eOrig]].pushBack(adj);
	}

	int currentDist = 0;
	for( ; ; ) {
		// next candidate edge
		while(nodesAtDist[currentDist % maxTC].empty())
			++currentDist;

		adjEntry adjCand = nodesAtDist[currentDist % maxTC].popFrontRet();
		node v = adjCand->twinNode();

		// leads to an unvisited node ?
		if (spPred[v] == nullptr) {
			// yes, then we set v's predecessor in search tree
			spPred[v] = adjCand;

			// have we reached t ...
			if (v == vT) {
				// ... then search is done.
				return currentDist;
			}

			// append next candidates to end of queue
			for(adjEntry adj : v->adjEntries) {
				int listPos = (currentDist + tcS[primalAdj[adj]->theEdge()]) % maxTC;
				nodesAtDist[listPos].pushBack(adj);
			}
		}
	}
}


int MultiEdgeApproxInserter::Block::costsSubpath(node n, edge eIn, edge eOut, node s, node t, PathDir &dirFrom, PathDir &dirTo)
{
	if(m_info[n].m_dual == nullptr)
		constructDual(n);

	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_spqr->skeleton(n));
	const Graph &M = S.getGraph();
	const EdgeArray<int> &tcS = m_tc[n];

	const Graph &dual = *m_info[n].m_dual;
	const ConstCombinatorialEmbedding &emb = *m_info[n].m_emb;
	const FaceArray<node> faceNode = *m_info[n].m_faceNode;
	const AdjEntryArray<adjEntry> primalAdj = *m_info[n].m_primalAdj;

	node v1 = nullptr, v2 = nullptr;
	if(eIn == nullptr || eOut == nullptr) {
		for(node v : M.nodes) {
			node vOrig = S.original(v);
			if(vOrig == s) v1 = v;
			if(vOrig == t) v2 = v;
		}
	}

	edge e1 = (eIn  == nullptr) ? nullptr : ((n != eIn ->source()) ? m_spqr->skeletonEdgeTgt(eIn)  : m_spqr->skeletonEdgeSrc(eIn));
	edge e2 = (eOut == nullptr) ? nullptr : ((n != eOut->source()) ? m_spqr->skeletonEdgeTgt(eOut) : m_spqr->skeletonEdgeSrc(eOut));

	computeTraversingCosts(n, e1, e2);

	int maxTC = 0;
	for(edge e : M.edges)
		Math::updateMax(maxTC, tcS[e]);

	++maxTC;
	Array<SListPure<Tuple2<node,node> > > nodesAtDist(maxTC);

	// start vertices
	if(e1 != nullptr) {
		nodesAtDist[0].pushBack( Tuple2<node,node>(faceNode[emb.rightFace(e1->adjSource())], nullptr) );
		nodesAtDist[0].pushBack( Tuple2<node,node>(faceNode[emb.rightFace(e1->adjTarget())], nullptr) );
	} else {
		for(adjEntry adj : v1->adjEntries)
			nodesAtDist[0].pushBack( Tuple2<node,node>(faceNode[emb.rightFace(adj)], nullptr) );
	}

	// stop vertices
	NodeArray<bool> stopVertex(dual,false);

	if(e2 != nullptr) {
		stopVertex[faceNode[emb.rightFace(e2->adjSource())]] = true;
		stopVertex[faceNode[emb.rightFace(e2->adjTarget())]] = true;
	} else {
		for(adjEntry adj : v2->adjEntries)
			stopVertex[faceNode[emb.rightFace(adj)]] = true;
	}

	// actual shortest path search
	NodeArray<bool> visited(dual,false);
	NodeArray<node> spPred(dual,nullptr);

	int currentDist = 0;
	for( ; ; ) {
		// next candidate
		while(nodesAtDist[currentDist % maxTC].empty())
			++currentDist;

		Tuple2<node,node> pair = nodesAtDist[currentDist % maxTC].popFrontRet();
		node v = pair.x1();

		// leads to an unvisited node ?
		if (!visited[v]) {
			visited[v] = true;
			spPred[v] = pair.x2();

			// have we reached t ...
			if (stopVertex[v]) {
				// ... then search is done.

				// find start node w
				node w = v;
				while(spPred[w] != nullptr)
					w = spPred[w];

				// in which direction to we start
				if(e1 == nullptr)
					dirFrom = PathDir::None;  // doesn't matter
				else if(w == faceNode[emb.rightFace(e1->adjSource())])
					dirFrom = PathDir::Right; // right face of eIn
				else
					dirFrom = PathDir::Left;  // left face of eIn

				// from which direction to we enter
				if(e2 == nullptr)
					dirTo = PathDir::None;  // doesn't matter
				else if(v == faceNode[emb.rightFace(e2->adjSource())])
					dirTo = PathDir::Left; // right face of eOut (leaving to the right)
				else
					dirTo = PathDir::Right; // left face of eOut (leaving to the left)

				return currentDist;
			}

			// append next candidates to end of queue
			for(adjEntry adj : v->adjEntries) {
				edge eM = primalAdj[adj]->theEdge();
				if(eM != e1 && eM != e2) {
					int listPos = (currentDist + tcS[eM]) % maxTC;
					nodesAtDist[listPos].pushBack(
						Tuple2<node,node>(adj->twinNode(), v) );
				}
			}
		}
	}
}


// sets default values for options
MultiEdgeApproxInserter::MultiEdgeApproxInserter()
{
	// options
	m_rrOptionFix = RemoveReinsertType::None;
	m_rrOptionVar = RemoveReinsertType::None;
	m_percentMostCrossedFix = 25;
	m_percentMostCrossedVar = 25;
	m_statistics = false;

	// initialization
	m_edge = nullptr;
	m_sumInsertionCosts = 0;
	m_sumFEInsertionCosts = 0;
}


// copy constructor
MultiEdgeApproxInserter::MultiEdgeApproxInserter(const MultiEdgeApproxInserter &inserter)
	: EdgeInsertionModule()
{
	// options
	m_rrOptionFix = inserter.m_rrOptionFix;
	m_rrOptionVar = inserter.m_rrOptionVar;
	m_percentMostCrossedFix = inserter.m_percentMostCrossedFix;
	m_percentMostCrossedVar = inserter.m_percentMostCrossedVar;
	m_statistics = inserter.m_statistics;

	// initialization
	m_edge = nullptr;
	m_sumInsertionCosts = 0;
	m_sumFEInsertionCosts = 0;
}


// clone method
EdgeInsertionModule *MultiEdgeApproxInserter::clone() const
{
	return new MultiEdgeApproxInserter(*this);
}


// assignment operator
MultiEdgeApproxInserter &MultiEdgeApproxInserter::operator=(const MultiEdgeApproxInserter &inserter)
{
	// options
	m_rrOptionFix = inserter.m_rrOptionFix;
	m_rrOptionVar = inserter.m_rrOptionVar;
	m_percentMostCrossedFix = inserter.m_percentMostCrossedFix;
	m_percentMostCrossedVar = inserter.m_percentMostCrossedVar;
	m_statistics = inserter.m_statistics;

	return *this;
}


// construct graph of block i
// store mapping of original nodes to copies in blocks
MultiEdgeApproxInserter::Block *MultiEdgeApproxInserter::constructBlock(int i)
{
	Block *b = new Block;
	SList<node> nodesG;

	SListConstIterator<edge> itE;
	for(itE = m_edgesB[i].begin(); itE.valid(); ++itE)
	{
		edge e = *itE;

		if (m_GtoBC[e->source()] == nullptr) {
			m_GtoBC[e->source()] = b->newNode();
			nodesG.pushBack(e->source());
		}
		if (m_GtoBC[e->target()] == nullptr) {
			m_GtoBC[e->target()] = b->newNode();
			nodesG.pushBack(e->target());
		}

		edge eBC = b->newEdge(m_GtoBC[e->source()],m_GtoBC[e->target()]);
		b->m_BCtoG[eBC->adjSource()] = e->adjSource();
		b->m_BCtoG[eBC->adjTarget()] = e->adjTarget();

		edge eOrig = m_pPG->original(e);
		if(m_costOrig != nullptr)
			b->m_cost[eBC] = (eOrig == nullptr) ? 0 : (*m_costOrig)[eOrig];
	}

	// store mapping orginal nodes -> copy in blocks and reset entries of GtoBC
	SListConstIterator<node> itV;
	for(itV = nodesG.begin(); itV.valid(); ++itV) {
		node v = *itV;
		m_copyInBlocks[v].pushBack(VertexBlock(m_GtoBC[v],i));
		m_GtoBC[v] = nullptr;
	}

	planarEmbed(*b);

	return b;
}


// returns copy of node vOrig in block b
node MultiEdgeApproxInserter::copy(node vOrig, int b)
{
	SListConstIterator<VertexBlock> it;
	for(it = m_copyInBlocks[vOrig].begin(); it.valid(); ++it) {
		if((*it).m_block == b)
			return (*it).m_vertex;
	}

	return nullptr;
}


// DFS-traversal for computing insertion path in SPQR-tree
bool MultiEdgeApproxInserter::dfsPathSPQR(node v, node v2, edge eParent, List<edge> &path)
{
	if(v == v2)
		return true;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if(e == eParent) continue;

		if(dfsPathSPQR(e->opposite(v), v2, e, path)) {
			path.pushFront(e);
			return true;
		}
	}

	return false;
}


// compute insertion path of edge edge k in SPQR-tree of block b
// from node vOrig to node wOrig
int MultiEdgeApproxInserter::computePathSPQR(int b, node vOrig, node wOrig, int k)
{
	Block &B = *m_block[b];

	node v = copy(vOrig,b);
	node w = copy(wOrig,b);
	OGDF_ASSERT(v != nullptr);
	OGDF_ASSERT(w != nullptr);

	B.initSPQR(m_edge->size());

	const auto &vAllocNodes = B.m_allocNodes[v];
	const auto &wAllocNodes = B.m_allocNodes[w];
	List<edge> &path = B.m_pathSPQR[k].m_edges;

	node v1 = vAllocNodes[0];
	node v2 = wAllocNodes[0];

	dfsPathSPQR( v1, v2, nullptr, path );

	node x;
	while(!path.empty() && vAllocNodes.linearSearch(x = path.front()->opposite(v1)) != -1) {
		v1 = x;
		path.popFront();
	}

	B.m_pathSPQR[k].m_start = v1;

	while(!path.empty() && wAllocNodes.linearSearch(x = path.back()->opposite(v2)) != -1) {
		v2 = x;
		path.popBack();
	}

	// compute insertion costs
	const StaticPlanarSPQRTree &spqr = B.spqr();
	List<EmbeddingPreference> &prefs = B.m_pathSPQR[k].m_prefs;

	PathDir dirFrom, dirTo;
	PathDir curDir = PathDir::Right;
	int c = 0;

	switch(spqr.typeOf(v1)) {
	case SPQRTree::NodeType::RNode:
		c += B.costsSubpath(v1, nullptr, (path.empty()) ? nullptr : path.front(), v, w, dirFrom, dirTo);
		prefs.pushBack(EmbeddingPreference(false));
		curDir = dirTo;
		break;

	case SPQRTree::NodeType::PNode:
		// in this case, the insertion path is a single P-node and we have no preference
		prefs.pushBack(EmbeddingPreference());
		break;

	case SPQRTree::NodeType::SNode:
		break; // nothing to do
	}

	node n = v1;
	ListConstIterator<edge> it;
	for(it = path.begin(); it.valid(); ++it)
	{
		edge e = *it;
		n = e->opposite(n);

		switch(spqr.typeOf(n)) {
		case SPQRTree::NodeType::RNode:
			c += B.costsSubpath(n, e, it.succ().valid() ? *it.succ() : nullptr, v, w, dirFrom, dirTo);

			// do we need to mirror embedding of R-node?
			prefs.pushBack(EmbeddingPreference(dirFrom != curDir));
			curDir = dirFrom == curDir ? dirTo : oppDir(dirTo);
			break;

		case SPQRTree::NodeType::PNode:
			{
			edge eIn = e;
			edge eOut = *(it.succ());

			edge e1 = (n != eIn ->source()) ? spqr.skeletonEdgeTgt(eIn)  : spqr.skeletonEdgeSrc(eIn);
			edge e2 = (n != eOut->source()) ? spqr.skeletonEdgeTgt(eOut) : spqr.skeletonEdgeSrc(eOut);

			const Graph &M = spqr.skeleton(n).getGraph();
			node first = M.firstNode();

			adjEntry a1 = (e1->source() == first) ? e1->adjSource() : e1->adjTarget();
			adjEntry a2 = (e2->source() == first) ? e2->adjSource() : e2->adjTarget();

			bool srcFits = (e1->source() == first);
			if( (curDir == PathDir::Left && srcFits) || (curDir == PathDir::Right && !srcFits) )
				std::swap(a1, a2);
			prefs.pushBack(EmbeddingPreference(a1,a2));

			if(e1->source() != e2->source())
				curDir = oppDir(curDir);
			}
			break;

		case SPQRTree::NodeType::SNode:
			if(it.succ().valid()) {
				edge eIn = e;
				edge eOut = *(it.succ());

				edge e1 = (n != eIn ->source()) ? spqr.skeletonEdgeTgt(eIn)  : spqr.skeletonEdgeSrc(eIn);
				edge e2 = (n != eOut->source()) ? spqr.skeletonEdgeTgt(eOut) : spqr.skeletonEdgeSrc(eOut);

				adjEntry at = e1->adjSource();
				while(at->theEdge() != e2)
					at = at->twin()->cyclicSucc();

				if(at == e2->adjSource())
					curDir = oppDir(curDir);
			}
			break; // nothing to do
		}
	}

	return c;
}


// DFS-traversal for computing insertion path in BC-tree
// (case vertex v)
bool MultiEdgeApproxInserter::dfsPathVertex(node v, int parent, int k, node t)
{
	if(v == t) return true;

	SListConstIterator<int> it;
	for(it = m_compV[v].begin(); it.valid(); ++it) {
		if(*it == parent) continue;
		if(dfsPathBlock(*it,v,k,t)) return true;
	}
	return false;
}


// DFS-traversal for computing insertion path in BC-tree
// (case block b)
bool MultiEdgeApproxInserter::dfsPathBlock(int b, node parent, int k, node t)
{
	SListConstIterator<node> it;
	for(it = m_verticesB[b].begin(); it.valid(); ++it)
	{
		node c = *it;
		if(c == parent) continue;
		if(dfsPathVertex(c,b,k,t)) {
			m_pathBCs[k].pushFront(VertexBlock(parent,b));

			if(!m_block[b]->isBridge())
			{
				// find path from parent to c in block b and compute embedding preferences
				m_insertionCosts[k] += computePathSPQR(b, parent, c, k);
			}

			return true;
		}
	}
	return false;
}


// compute insertion path of edge edge k in BC-tree
void MultiEdgeApproxInserter::computePathBC(int k)
{
	node s = m_pPG->copy((*m_edge)[k]->source());
	node t = m_pPG->copy((*m_edge)[k]->target());

	bool found = dfsPathVertex(s, -1, k, t);
	if(!found) std::cout << "Could not find path in BC-tree!" << std::endl;
}


// Embeds block b according to combined embedding
// preference s of the k insertion paths
void MultiEdgeApproxInserter::embedBlock(int b, int m)
{
	// Algorithm 3.7.

	Block &B = *m_block[b];
	if(B.isBridge())
		return;
	StaticPlanarSPQRTree &spqr = B.spqr();

	// A.
	NodeArray<EmbeddingPreference> pi_pick(spqr.tree());
	NodeArray<bool> visited(spqr.tree(), false);  // mark nodes with embedding preferences
	NodeArray<bool> markFlipped(spqr.tree(), false);  // mark nodes on current path to get flipped
	Array<Block::PathElement> p;

	// B.
	for(int i = 0; i < m; ++i)
	{
		// (a)
		B.pathToArray(i, p);
		const int len = p.size();
		if(len == 0) continue;  // this path does not go through B

		int j = 0;
		do {
			node n = p[j].m_node;

			// (b)
			bool newlySet = false;
			if(pi_pick[n].isNull()) {
				newlySet = true;
				pi_pick[n] = *p[j].m_pref;

				if(spqr.typeOf(n) == SPQRTree::NodeType::PNode) {
					adjEntry a1 = pi_pick[n].adj1();
					adjEntry a2 = pi_pick[n].adj2();
					adjEntry adj = a1->cyclicSucc();

					if(adj != a2)
						spqr.swap(n,adj,a2);

					OGDF_ASSERT(a1->cyclicSucc() == a2);
				}
			}

			// (c)

			// determine non-S-node predecessor of n
			int j_mu = -1;
			if(j > 0) {
				node x = p[j-1].m_node;
				if(spqr.typeOf(x) != SPQRTree::NodeType::SNode)
					j_mu = j-1;
				else if(j > 1)
					j_mu = j-2;
			}

			if(j_mu >= 0)
			{
				node mu = p[j_mu].m_node;

				// do we have a switching pair (mu,n)?
				if(B.switchingPair(mu, n, pi_pick[mu], *p[j_mu].m_pref, pi_pick[n], *p[j].m_pref))
					markFlipped[mu] = true;  // flip embedding at mu
			}

			// (d)
			if(!newlySet) {
				// skip nodes in same embedding partition
				while(j+1 < len && visited[p[j+1].m_node])
					++j;
			}

			// next node
			if(++j >= len)
				break; // end of insertion path

			// skip S-node (can only be one)
			if(spqr.typeOf(p[j].m_node) == SPQRTree::NodeType::SNode)
				++j;

		} while(j < len);

		// flip embedding preferences
		bool flipping = false;
		for(int iter = len-1; iter >= 0; --iter)
		{
			node n = p[iter].m_node;
			if(markFlipped[n]) {
				flipping = !flipping;
				markFlipped[n] = false;
			}

			if(flipping) {
				pi_pick[n].flip();
				if(pi_pick[n].type() == EmbeddingPreference::Type::PNode)
					spqr.reverse(n);

				if(visited[n]) {
					node n_succ = (iter+1 < len) ? p[iter+1].m_node : nullptr;
					node n_pred = (iter-1 >=  0) ? p[iter-1].m_node : nullptr;

					for(adjEntry adj : n->adjEntries) {
						node x = adj->twinNode();
						if(x != n_succ && x != n_pred && visited[x])
							recFlipPref(adj->twin(), pi_pick, visited, spqr);
					}
				}
			}

			visited[n] = true;
		}
	}

	// C.
	for(node n : spqr.tree().nodes)
	{
		const EmbeddingPreference &ep = pi_pick[n];
		if(ep.isNull())
			continue;

		switch(spqr.typeOf(n)) {
		case SPQRTree::NodeType::SNode:
			break;  // nothing to do

		case SPQRTree::NodeType::PNode:
			break;  // already embedded as desired

		case SPQRTree::NodeType::RNode:
			if(ep.mirror())
				spqr.reverse(n);
			break;
		}
	}

	spqr.embed(B);
}


void MultiEdgeApproxInserter::recFlipPref(
	adjEntry adjP,
	NodeArray<EmbeddingPreference> &pi_pick,
	const NodeArray<bool> &visited,
	StaticPlanarSPQRTree &spqr)
{
	node n = adjP->theNode();

	EmbeddingPreference &pref = pi_pick[n];
	pref.flip();
	if(pref.type() == EmbeddingPreference::Type::PNode)
		spqr.reverse(n);

	for(adjEntry adj : n->adjEntries) {
		if(adj != adjP && visited[adj->twinNode()])
			recFlipPref(adj->twin(), pi_pick, visited, spqr);
	}
}


// actual call method
// currently not supported:
//   frobidCrossingGens, forbiddenEdgeOrig, edgeSubGraphs
const char *strType[] = { "S", "P", "R" };
//#define MEAI_OUTPUT

struct CutvertexPreference {
	CutvertexPreference(node v1, int b1, node v2, int b2)
		: m_v1(v1), m_v2(v2), m_b1(b1), m_b2(b2), m_len1(-1), m_len2(-1) { }

	node m_v1, m_v2;
	int  m_b1, m_b2;
	int m_len1, m_len2;
};

void appendToList(SListPure<adjEntry> &adjList, adjEntry adj1,
	const AdjEntryArray<adjEntry> &BCtoG,
	AdjEntryArray<SListIterator<adjEntry> > &pos)
{
	adjEntry adj = adj1;
	do {
		adj = adj->cyclicSucc();
		adjEntry adjG = BCtoG[adj];
		pos[adjG] = adjList.pushBack(adjG);
	} while(adj != adj1);
}

void insertAfterList(SListPure<adjEntry> &adjList, SListIterator<adjEntry> itBefore, adjEntry adj1,
	const AdjEntryArray<adjEntry> &BCtoG,
	AdjEntryArray<SListIterator<adjEntry> > &pos)
{
	adjEntry adj = adj1;
	do {
		adj = adj->cyclicSucc();
		adjEntry adjG = BCtoG[adj];
		itBefore = pos[adjG] = adjList.insertAfter(adjG, itBefore);
	} while(adj != adj1);
}

Module::ReturnType MultiEdgeApproxInserter::doCall(
	PlanRepLight&                 PG,
	const Array<edge>&            origEdges,
	const EdgeArray<int>*         costOrig,
	const EdgeArray<bool>*        /* unused parameter */,
	const EdgeArray<uint32_t>*    /* unused parameter */)
{
	m_pPG = &PG;
	m_costOrig = costOrig;
	const int m = origEdges.size();

	m_edge = &origEdges;
	OGDF_ASSERT(m_edge->low() == 0);
	m_pathBCs.init(m);
	m_insertionCosts.init(0,m-1,0);

	//
	// PHASE 1: Fix a good embedding
	//

	// compute biconnected components of PG
	EdgeArray<int> compnum(PG);
	int c = biconnectedComponents(PG,compnum);

	m_compV.init(PG);
	m_verticesB.init(c);

	// edgesB[i] = list of edges in component i
	m_edgesB.init(c);
	for(edge e : PG.edges)
		m_edgesB[compnum[e]].pushBack(e);

	// construct arrays compV and nodeB such that
	// m_compV[v] = list of components containing v
	// m_verticesB[i] = list of vertices in component i
	NodeArray<bool> mark(PG,false);

	for(int i = 0; i < c; ++i) {
		for (edge e : m_edgesB[i]) {
			if (!mark[e->source()]) {
				mark[e->source()] = true;
				m_verticesB[i].pushBack(e->source());
			}
			if (!mark[e->target()]) {
				mark[e->target()] = true;
				m_verticesB[i].pushBack(e->target());
			}
		}

		for (node v : m_verticesB[i]) {
			m_compV[v].pushBack(i);
			mark[v] = false;
		}
	}
	mark.init();
	m_GtoBC.init(PG,nullptr);
	m_copyInBlocks.init(PG);

	m_block.init(c);
	for(int i = 0; i < c; ++i)
		m_block[i] = constructBlock(i);

	m_sumInsertionCosts = 0;
	for(int i = 0; i < m; ++i) {
		computePathBC(i);
		m_sumInsertionCosts += m_insertionCosts[i];

#ifdef MEAI_OUTPUT
		std::cout << "(" << PG.copy((*m_edge)[i]->source()) << "," << PG.copy((*m_edge)[i]->target()) << ")  c = " << m_insertionCosts[i] << ":\n";
		for (const auto &vblock : m_pathBCs[i]) {
			int b = vblock.m_block;
			const StaticSPQRTree &spqr = m_block[b]->spqr();

			std::cout << "   [ " << "V_" << vblock.m_vertex << ", B_" << b << " ]\n";
			std::cout << "      ";
			if(m_block[b]->isBridge()) {
				std::cout << "BRIDGE";
			} else {
				node x = m_block[b]->m_pathSPQR[i].m_start;
				ListConstIterator<EmbeddingPreference> itP = m_block[b]->m_pathSPQR[i].m_prefs.begin();
				auto output = [&](node v) {
					SPQRTree::NodeType t = spqr.typeOf(v);
					std::cout << strType[static_cast<int>(t)] << "_" << v;
					if (t == SPQRTree::NodeType::RNode) {
						if ((*itP).type() == EmbeddingPreference::Type::None) {
							std::cout << " (NONE)";
						} else if ((*itP).mirror()) {
							std::cout << " (MIRROR)";
						} else {
							std::cout << " (KEEP)";
						}
						++itP;
					} else if (t == SPQRTree::NodeType::PNode) {
						if ((*itP).type() == EmbeddingPreference::Type::None) {
							std::cout << " (NONE)";
						} else {
							std::cout << "(ADJ:" << (*itP).adj1()->index() << ";" << (*itP).adj2()->index() << ")";
						}
						++itP;
					}
				};
				output(x);

				for (edge e : m_block[b]->m_pathSPQR[i].m_edges) {
					node y = e->opposite(x);
					std::cout << " -> ";
					output(y);
					x = y;
				}
			}
			std::cout << std::endl;
		}
#endif
	}

	// embed blocks
	for(int i = 0; i < c; ++i)
		embedBlock(i,m);


	// find embedding preferences at cutvertices
	NodeArray<SList<CutvertexPreference> > cvPref(PG);

	for(int i = 0; i < m; ++i)
	{
		const List<VertexBlock> &L =  m_pathBCs[i];
		if(L.size() < 2)
			continue;

		ListConstIterator<VertexBlock> it = L.begin();
		node last_v = (*it).m_vertex;
		int  last_b = (*it).m_block;
		++it;
		do {
			node v2 = (it.succ().valid()) ? (*it.succ()).m_vertex : PG.copy((*m_edge)[i]->target());
			cvPref[(*it).m_vertex].pushBack(CutvertexPreference(last_v,last_b,v2,(*it).m_block));

			last_v = (*it).m_vertex;
			last_b = (*it).m_block;
			++it;
		} while(it.valid());
	}


	// embedding graph
	Array<bool> blockSet(0,c-1,false);
	AdjEntryArray<SListIterator<adjEntry> > pos(PG,nullptr);

	for(node v : PG.nodes)
	{
		SListPure<adjEntry> adjList;
		SList<VertexBlock> &copies = m_copyInBlocks[v];

		// not a cut vertex?
		if(copies.size() == 1) {
			OGDF_ASSERT(cvPref[v].empty());
			const Block &B = *m_block[copies.front().m_block];
			node vB = copies.front().m_vertex;

			for(adjEntry adj : vB->adjEntries)
				adjList.pushBack(B.m_BCtoG[adj]);

		} else {
			SList<CutvertexPreference> &prefs = cvPref[v];

			if(!prefs.empty()) {
				// always realize first cutvertex preference
				SListIterator<CutvertexPreference> it = prefs.begin();

				int b1 = (*it).m_b1;
				int b2 = (*it).m_b2;
				blockSet[b1] = blockSet[b2] = true;

				adjEntry adj1 = m_block[b1]->findBestFace(copy(v,b1),copy((*it).m_v1,b1), (*it).m_len1);
				appendToList(adjList, adj1, m_block[b1]->m_BCtoG, pos);

				adjEntry adj2 = m_block[b2]->findBestFace(copy(v,b2),copy((*it).m_v2,b2), (*it).m_len2);
				appendToList(adjList, adj2, m_block[b2]->m_BCtoG, pos);

				for(++it; it.valid(); ++it) {
					b1 = (*it).m_b1;
					b2 = (*it).m_b2;

					if(!blockSet[b1] && !blockSet[b2]) {
						// none of the two blocks set yet
						adjEntry bestFace1 = m_block[b1]->findBestFace(copy(v,b1),copy((*it).m_v1,b1), (*it).m_len1);
						appendToList(adjList, bestFace1, m_block[b1]->m_BCtoG, pos);

						adjEntry bestFace2 = m_block[b2]->findBestFace(copy(v,b2),copy((*it).m_v2,b2), (*it).m_len2);
						appendToList(adjList, bestFace2, m_block[b2]->m_BCtoG, pos);

						blockSet[b1] = blockSet[b2] = true;

					} else if(!blockSet[b1]) {
						// b2 is set, but b1 is not yet set
						adjEntry bestFace1 = m_block[b1]->findBestFace(copy(v,b1),copy((*it).m_v1,b1), (*it).m_len1);
						adjEntry bestFace2 = m_block[b2]->findBestFace(copy(v,b2),copy((*it).m_v2,b2), (*it).m_len2);

						insertAfterList(adjList, pos[m_block[b2]->m_BCtoG[bestFace2]], bestFace1, m_block[b1]->m_BCtoG, pos);
						blockSet[b1] = true;

					} else if(!blockSet[b2]) {
						// b1 is set, but b2 is not yet set
						adjEntry bestFace1 = m_block[b1]->findBestFace(copy(v,b1),copy((*it).m_v1,b1), (*it).m_len1);
						adjEntry bestFace2 = m_block[b2]->findBestFace(copy(v,b2),copy((*it).m_v2,b2), (*it).m_len2);

						insertAfterList(adjList, pos[m_block[b1]->m_BCtoG[bestFace1]], bestFace2, m_block[b2]->m_BCtoG, pos);
						blockSet[b2] = true;
					}
				}
			}

			// embed remaining blocks (if any)
			for (const auto &vblock : copies) {
				int b = vblock.m_block;
				if(blockSet[b])
					continue;
				appendToList(adjList, vblock.m_vertex->firstAdj(), m_block[b]->m_BCtoG, pos);
			}

			// cleanup
			for (auto pref : prefs) {
				blockSet[pref.m_b1] = false;
				blockSet[pref.m_b2] = false;
			}

			OGDF_ASSERT(adjList.size() == v->degree());
		}

		PG.sort(v, adjList);
	}

	OGDF_ASSERT(PG.representsCombEmbedding());

#if 0
	// arbitrary embedding for testing
	planarEmbed(PG);
#endif

	// generate further statistic information
	if(m_statistics) {
		constructDual(PG);

#if 0
		std::cout << "\ncutvertex preferences:\n" << std::endl;
		for(node v : PG.nodes)
		{
			SListConstIterator<CutvertexPreference> it = cvPref[v].begin();
			if(!it.valid()) continue;

			std::cout << v << ":\n";
			for(; it.valid(); ++it) {
				const CutvertexPreference &p = *it;

				int sp1 = findShortestPath(p.m_v1, v);
				int sp2 = findShortestPath(v, p.m_v2);
				std::cout << "   ( v" << p.m_v1 << ", b" << p.m_b1 << "; v" << p.m_v2 << ", b" << p.m_b2 << " )  ";
				std::cout << "[ " << p.m_len1 << " / " << sp1 << " ; " << p.m_len2 << " / " << sp2 << " ]" << std::endl;
			}
		}
#endif

		m_sumFEInsertionCosts = 0;
		for(int i = 0; i < m; ++i) {
			node s = PG.copy((*m_edge)[i]->source());
			node t = PG.copy((*m_edge)[i]->target());
			int len = findShortestPath(s,t);
			m_sumFEInsertionCosts += len;
		}
	} else
		m_sumFEInsertionCosts = -1;


	// release no longer needed resources
	cleanup();


	//
	// PHASE 2: Perform edge insertion with fixed emebdding
	//

	FixedEmbeddingInserter fei;
	fei.keepEmbedding(true);
	fei.removeReinsert(m_rrOptionFix);
	fei.percentMostCrossed(m_percentMostCrossedFix);

	fei.call( PG, origEdges );

	if(m_rrOptionVar != RemoveReinsertType::None && m_rrOptionVar != RemoveReinsertType::Incremental) {
		VariableEmbeddingInserter vei;
		vei.removeReinsert(m_rrOptionVar);
		vei.percentMostCrossed(m_percentMostCrossedFix);
		vei.callPostprocessing( PG, origEdges );
	}

	return ReturnType::Feasible;
}


// cleanup: delete blocks, reset all auxiliary arrays
void MultiEdgeApproxInserter::cleanup()
{
	int c = m_block.size();
	for(int  i = 0; i < c; ++i)
		delete m_block[i];
	m_block.init();

	m_GtoBC.init();
	m_edgesB.init();
	m_verticesB.init();
	m_compV.init();

	m_edge = nullptr;
	m_pathBCs.init();
	m_insertionCosts.init();
	m_copyInBlocks.init();

	m_primalAdj.init();
	m_faceNode.init();
	m_E.init();
	m_dual.clear();
}


// just for testing and additional statistics
void MultiEdgeApproxInserter::constructDual(const PlanRepLight &PG)
{
	m_E.init(PG);
	m_faceNode.init(m_E);
	m_primalAdj.init(m_dual);

	// constructs nodes (for faces in m_embB)
	for(face f : m_E.faces) {
		m_faceNode[f] = m_dual.newNode();
	}

	// construct dual edges (for primal edges in block)
	for(node v : PG.nodes)
	{
		for(adjEntry adj : v->adjEntries)
		{
			if(adj->index() & 1) {
				node vLeft  = m_faceNode[m_E.leftFace (adj)];
				node vRight = m_faceNode[m_E.rightFace(adj)];

				edge eDual = m_dual.newEdge(vLeft,vRight);
				m_primalAdj[eDual->adjSource()] = adj;
				m_primalAdj[eDual->adjTarget()] = adj->twin();
			}
		}
	}

	m_vS = m_dual.newNode();
	m_vT = m_dual.newNode();
}


int MultiEdgeApproxInserter::findShortestPath(node s, node t)
{
	NodeArray<adjEntry> spPred(m_dual, nullptr);
	QueuePure<adjEntry> queue;
	int oldIdCount = m_dual.maxEdgeIndex();

	// augment dual by edges from s to all adjacent faces of s ...
	for(adjEntry adj : s->adjEntries) {
		// starting edges of bfs-search are all edges leaving s
		edge eDual = m_dual.newEdge(m_vS, m_faceNode[m_E.rightFace(adj)]);
		m_primalAdj[eDual->adjSource()] = adj;
		m_primalAdj[eDual->adjTarget()] = nullptr;
		queue.append(eDual->adjSource());
	}

	// ... and from all adjacent faces of t to t
	for(adjEntry adj : t->adjEntries) {
		edge eDual = m_dual.newEdge(m_faceNode[m_E.rightFace(adj)], m_vT);
		m_primalAdj[eDual->adjSource()] = adj;
		m_primalAdj[eDual->adjTarget()] = nullptr;
	}

	// actual search (using bfs on directed dual)
	int len = -2;
	for( ; ;)
	{
		// next candidate edge
		adjEntry adjCand = queue.pop();
		node v = adjCand->twinNode();

		// leads to an unvisited node?
		if (spPred[v] == nullptr)
		{
			// yes, then we set v's predecessor in search tree
			spPred[v] = adjCand;

			// have we reached t ...
			if (v == m_vT)
			{
				// ... then search is done.

				adjEntry adj;
				do {
					adj = spPred[v];
					++len;
					v = adj->theNode();
				} while(v != m_vS);

				break;
			}

			// append next candidate edges to queue
			// (all edges leaving v)
			for(adjEntry adj : v->adjEntries) {
				if(adj->twinNode() != m_vS)
					queue.append(adj);
			}
		}
	}

	// remove augmented edges again
	adjEntry adj;
	while ((adj = m_vS->firstAdj()) != nullptr)
		m_dual.delEdge(adj->theEdge());

	while ((adj = m_vT->firstAdj()) != nullptr)
		m_dual.delEdge(adj->theEdge());

	m_dual.resetEdgeIdCount(oldIdCount);

	return len;
}

}
