/** \file
 * \brief Implementation of class VarEdgeInserterCore and VarEdgeInserterUMLCore,
 * which are the implementation classes for edge insertion with variable embedding.
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

#include <ogdf/planarity/embedding_inserter/CrossingsBucket.h>
#include <ogdf/planarity/embedding_inserter/VarEdgeInserterCore.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/decomposition/StaticPlanarSPQRTree.h>

namespace ogdf {

// actual algorithm call
Module::ReturnType VarEdgeInserterCore::call(
	const Array<edge> &origEdges,
	RemoveReinsertType rrPost,
	double percentMostCrossed)
{
	double T;
	usedTime(T);

	Module::ReturnType retValue = Module::ReturnType::Feasible;
	m_runsPostprocessing = 0;

	if (origEdges.size() == 0)
		return Module::ReturnType::Optimal;  // nothing to do

	SListPure<edge> currentOrigEdges;
	if(rrPost == RemoveReinsertType::Incremental) {
		for(edge e : m_pr.edges)
			currentOrigEdges.pushBack(m_pr.original(e));
	}

	// insertion of edges
	bool doIncrementalPostprocessing =
		( rrPost == RemoveReinsertType::Incremental || rrPost == RemoveReinsertType::IncInserted );
	for(int i = origEdges.low(); i <= origEdges.high(); ++i)
	{
		edge eOrig = origEdges[i];
		storeTypeOfCurrentEdge(eOrig);

		SList<adjEntry> eip;
		m_st = eOrig; // save original edge for simdraw cost calculation in dfsvertex
		insert(m_pr.copy(eOrig->source()), m_pr.copy(eOrig->target()), eip);

		m_pr.insertEdgePath(eOrig,eip);

		if(doIncrementalPostprocessing) {
			currentOrigEdges.pushBack(eOrig);

			bool improved;
			do {
				++m_runsPostprocessing;
				improved = false;

				for (edge eOrigRR : currentOrigEdges)
				{
					int pathLength = (m_pCost != nullptr) ? costCrossed(eOrigRR) : (m_pr.chain(eOrigRR).size() - 1);
					if (pathLength == 0) continue; // cannot improve

					m_pr.removeEdgePath(eOrigRR);

					storeTypeOfCurrentEdge(eOrigRR);
					//m_typeOfCurrentEdge = m_forbidCrossingGens ? PG.typeOrig(eOrigRR) : Graph::association;

					SList<adjEntry> iep;
					m_st = eOrigRR;
					insert(m_pr.copy(eOrigRR->source()), m_pr.copy(eOrigRR->target()), iep);
					m_pr.insertEdgePath(eOrigRR,iep);

					int newPathLength = (m_pCost != nullptr) ? costCrossed(eOrigRR) : (m_pr.chain(eOrigRR).size() - 1);
					OGDF_ASSERT(newPathLength <= pathLength);

					if(newPathLength < pathLength)
						improved = true;
				}
			} while (improved);
		}
	}

	if(!doIncrementalPostprocessing) {
		// postprocessing (remove-reinsert heuristc)
		const int m = m_pr.original().numberOfEdges();
		SListPure<edge> rrEdges;

		switch(rrPost)
		{
		case RemoveReinsertType::All:
		case RemoveReinsertType::MostCrossed:
			for(int i = m_pr.startEdge(); i < m_pr.stopEdge(); ++i)
				rrEdges.pushBack(m_pr.e(i));
			break;

		case RemoveReinsertType::Inserted:
			for(int i = origEdges.low(); i <= origEdges.high(); ++i)
				rrEdges.pushBack(origEdges[i]);
			break;

		case RemoveReinsertType::None:
		case RemoveReinsertType::Incremental:
		case RemoveReinsertType::IncInserted:
			break;
		}

		// marks the end of the interval of rrEdges over which we iterate
		// initially set to invalid iterator which means all edges
		SListConstIterator<edge> itStop;

		bool improved;
		do {
			// abort postprocessing if time limit reached
			if (m_timeLimit >= 0 && m_timeLimit <= usedTime(T)) {
				retValue = Module::ReturnType::TimeoutFeasible;
				break;
			}

			++m_runsPostprocessing;
			improved = false;

			if(rrPost == RemoveReinsertType::MostCrossed)
			{
				embedding_inserter::CrossingsBucket<PlanRepLight> bucket(&m_pr);
				rrEdges.bucketSort(bucket);

				const int num = int(0.01 * percentMostCrossed * m);
				itStop = rrEdges.get(num);
			}

			SListConstIterator<edge> it;
			for(it = rrEdges.begin(); it != itStop; ++it)
			{
				edge eOrig = *it;

				int pathLength = (m_pCost != nullptr) ? costCrossed(eOrig) : (m_pr.chain(eOrig).size() - 1);
				if (pathLength == 0) continue; // cannot improve

				m_pr.removeEdgePath(eOrig);

				storeTypeOfCurrentEdge(eOrig);

				SList<adjEntry> eip;
				m_st = eOrig;
				insert(m_pr.copy(eOrig->source()), m_pr.copy(eOrig->target()), eip);
				m_pr.insertEdgePath(eOrig,eip);

				// we cannot find a shortest path that is longer than before!
				int newPathLength = (m_pCost != nullptr) ? costCrossed(eOrig) : (m_pr.chain(eOrig).size() - 1);
				OGDF_ASSERT(newPathLength <= pathLength);

				if(newPathLength < pathLength)
					improved = true;
			}
		} while (improved);
	}

#ifdef OGDF_DEBUG
	bool isPlanar =
#endif
		planarEmbed(m_pr);

	OGDF_ASSERT(isPlanar);

	m_pr.removePseudoCrossings();
	OGDF_ASSERT(m_pr.representsCombEmbedding());

	return retValue;
}


// call for postprocessing only
Module::ReturnType VarEdgeInserterCore::callPostprocessing(
	const Array<edge> &origEdges,
	RemoveReinsertType rrPost,
	double percentMostCrossed)
{
	double T;
	usedTime(T);

	Module::ReturnType retValue = Module::ReturnType::Feasible;
	m_runsPostprocessing = 0;

	if (origEdges.size() == 0)
		return Module::ReturnType::Optimal;  // nothing to do

	if(rrPost == RemoveReinsertType::Incremental || rrPost == RemoveReinsertType::IncInserted)
		return Module::ReturnType::Feasible;

	SListPure<edge> currentOrigEdges;

	// postprocessing (remove-reinsert heuristc)
	const int m = m_pr.original().numberOfEdges();
	SListPure<edge> rrEdges;

	switch(rrPost)
	{
	case RemoveReinsertType::All:
	case RemoveReinsertType::MostCrossed:
		for(int i = m_pr.startEdge(); i < m_pr.stopEdge(); ++i)
			rrEdges.pushBack(m_pr.e(i));
		break;

	case RemoveReinsertType::Inserted:
		for(int i = origEdges.low(); i <= origEdges.high(); ++i)
			rrEdges.pushBack(origEdges[i]);
		break;

	case RemoveReinsertType::None:
	case RemoveReinsertType::Incremental:
	case RemoveReinsertType::IncInserted:
		break;
	}

	// marks the end of the interval of rrEdges over which we iterate
	// initially set to invalid iterator which means all edges
	SListConstIterator<edge> itStop;

	bool improved;
	do {
		// abort postprocessing if time limit reached
		if (m_timeLimit >= 0 && m_timeLimit <= usedTime(T)) {
			retValue = Module::ReturnType::TimeoutFeasible;
			break;
		}

		++m_runsPostprocessing;
		improved = false;

		if(rrPost == RemoveReinsertType::MostCrossed)
		{
			embedding_inserter::CrossingsBucket<PlanRepLight> bucket(&m_pr);
			rrEdges.bucketSort(bucket);

			const int num = int(0.01 * percentMostCrossed * m);
			itStop = rrEdges.get(num);
		}

		SListConstIterator<edge> it;
		for(it = rrEdges.begin(); it != itStop; ++it)
		{
			edge eOrig = *it;

			int pathLength = (m_pCost != nullptr) ? costCrossed(eOrig) : (m_pr.chain(eOrig).size() - 1);
			if (pathLength == 0) continue; // cannot improve

			m_pr.removeEdgePath(eOrig);

			storeTypeOfCurrentEdge(eOrig);

			SList<adjEntry> eip;
			m_st = eOrig;
			insert(m_pr.copy(eOrig->source()), m_pr.copy(eOrig->target()), eip);
			m_pr.insertEdgePath(eOrig,eip);

			// we cannot find a shortest path that is longer than before!
			int newPathLength = (m_pCost != nullptr) ? costCrossed(eOrig) : (m_pr.chain(eOrig).size() - 1);
			OGDF_ASSERT(newPathLength <= pathLength);

			if(newPathLength < pathLength)
				improved = true;
		}
	} while (improved);

#ifdef OGDF_DEBUG
	bool isPlanar =
#endif
		planarEmbed(m_pr);

	OGDF_ASSERT(isPlanar);

	m_pr.removePseudoCrossings();
	OGDF_ASSERT(m_pr.representsCombEmbedding());

	return retValue;
}


static edge crossedEdge(adjEntry adj)
{
	edge e = adj->theEdge();

	adj = adj->cyclicSucc();
	while(adj->theEdge() == e)
		adj = adj->cyclicSucc();

	return adj->theEdge();
}


int VarEdgeInserterCore::costCrossed(edge eOrig) const
{
	int c = 0;

	const List<edge> &L = m_pr.chain(eOrig);

	ListConstIterator<edge> it = L.begin();
	if(m_pSubgraph != nullptr) {
		for(++it; it.valid(); ++it) {
			int counter = 0;
			edge e = m_pr.original(crossedEdge((*it)->adjSource()));
			for(int i = 0; i < 32; i++)
				if((*m_pSubgraph)[eOrig] & (*m_pSubgraph)[e] & (1<<i))
					counter++;
			c += counter * (*m_pCost)[e];
		}
		c *= c_bigM;
		if(c == 0)
			c = 1;
	} else {
		for(++it; it.valid(); ++it) {
			c += (*m_pCost)[m_pr.original(crossedEdge((*it)->adjSource()))];
		}
	}

	return c;
}


// find optimal edge insertion path from s to t in connected
// graph G
void VarEdgeInserterCore::insert(node s, node t, SList<adjEntry> &eip)
{
	eip.clear();

	m_s = s; m_t = t;
	m_pEip = &eip;

	// compute biconnected components of PG
	EdgeArray<int> compnum(m_pr);
	int c = biconnectedComponents(m_pr,compnum);

	m_compV.init(m_pr);
	m_nodeB.init(c);

	// edgeB[i] = list of edges in component i
	m_edgeB.init(c);
	for(edge e : m_pr.edges)
		m_edgeB[compnum[e]].pushBack(e);

	// construct arrays compV and nodeB such that
	// m_compV[v] = list of components containing v
	// m_nodeB[i] = list of vertices in component i
	NodeArray<bool> mark(m_pr,false);

	int i;
	for(i = 0; i < c; ++i) {
		for(edge e : m_edgeB[i])
		{
			if (!mark[e->source()]) {
				mark[e->source()] = true;
				m_nodeB[i].pushBack(e->source());
			}
			if (!mark[e->target()]) {
				mark[e->target()] = true;
				m_nodeB[i].pushBack(e->target());
			}
		}

		for(node v : m_nodeB[i])
		{
			m_compV[v].pushBack(i);
			mark[v] = false;
		}
	}
	mark.init();

	// find path from s to t in BC-tree
	// call of blockInsert() is done in dfs_vertex() when we have found the
	// path and we return from the recursion.
	// if no path is found, s and t are in different connected components
	// and thus an empty edge insertion path is correct!
	m_GtoBC.init(m_pr,nullptr);
	dfsVertex(s,-1);

	// deallocate resources used by insert()
	m_GtoBC.init();
	m_edgeB.init();
	m_nodeB.init();
	m_compV.init();
}


class VarEdgeInserterCore::BiconnectedComponent : public Graph
{
public:
	BiconnectedComponent() : m_BCtoG(*this), m_cost(*this,1) { }

	void cost(edge e, int c) {
		m_cost[e] = c;
	}

	int cost(edge e) const {
		return m_cost[e];
	}

	AdjEntryArray<adjEntry> m_BCtoG;

protected:
	EdgeArray<int> m_cost;
};


class VarEdgeInserterUMLCore::BiconnectedComponentUML : public VarEdgeInserterCore::BiconnectedComponent
{
public:
	explicit BiconnectedComponentUML(const PlanRepLight &pr) : m_pr(pr) { }

	EdgeType typeOf(edge e) const {
		return m_pr.typeOf(m_BCtoG[e->adjSource()]->theEdge());
	}

protected:
	const PlanRepLight &m_pr;
};


VarEdgeInserterCore::BiconnectedComponent *VarEdgeInserterCore::createBlock()
{
	return new BiconnectedComponent;
}

VarEdgeInserterCore::BiconnectedComponent *VarEdgeInserterUMLCore::createBlock()
{
	return new BiconnectedComponentUML(m_pr);
}


// recursive path search from s to t in BC-tree (vertex case)
bool VarEdgeInserterCore::dfsVertex(node v, int parent)
{
	// forall biconnected components containing v (except predecessor parent)
	for(int i : m_compV[v])
	{
		if (i == parent) continue;

		// representative of t in B(i)
		node repT = dfsComp(i, v);
		if (repT != nullptr) { // path found?
			// build graph BC of biconnected component B(i)
			SList<node> nodesG;
			BiconnectedComponent *BC = createBlock();

			for(edge e : m_edgeB[i])
			{
				if (m_GtoBC[e->source()] == nullptr) {
					m_GtoBC[e->source()] = BC->newNode();
					nodesG.pushBack(e->source());
				}
				if (m_GtoBC[e->target()] == nullptr) {
					m_GtoBC[e->target()] = BC->newNode();
					nodesG.pushBack(e->target());
				}

				edge eBC = BC->newEdge(m_GtoBC[e->source()],m_GtoBC[e->target()]);
				BC->m_BCtoG[eBC->adjSource()] = e->adjSource();
				BC->m_BCtoG[eBC->adjTarget()] = e->adjTarget();

				//BC.typeOf(eBC, m_forbidCrossingGens ? m_pPG->typeOf(e) : Graph::association);
				edge eOrig = m_pr.original(e);
				if(m_pCost != nullptr) {
					if(m_pSubgraph != nullptr) {
						int counter = 0;
						for(int iter = 0; iter<32; iter++)
							if((*m_pSubgraph)[m_st] & (*m_pSubgraph)[eOrig] & (1<<iter))
								counter++;
						counter *= c_bigM;
						int cost = counter * (*m_pCost)[eOrig];
						if(cost == 0)
							cost = 1;
						BC->cost(eBC, cost);
					} else
						BC->cost(eBC, (eOrig == nullptr) ? 0 : (*m_pCost)[eOrig]);
				}
			}

			// less than 3 nodes requires no crossings (cannot build SPQR-tree
			// for a graph with less than 3 nodes!)
			if (nodesG.size() >= 3) {
				List<adjEntry> L;
				blockInsert(*BC,m_GtoBC[v],m_GtoBC[repT],L); // call biconnected case

				// transform crossed edges to edges in G
				for(adjEntry adj : reverse(L)) {
					m_pEip->pushFront(BC->m_BCtoG[adj]);
				}
			}

			// set entries of GtoBC back to nil (GtoBC allocated only once
			// in insert()!)
			for(node u : nodesG)
				m_GtoBC[u] = nullptr;

			delete BC;
			return true; // path found
		}
	}

	return false; // path not found
}


// recursive path search from s to t in BC-tree (component case)
node VarEdgeInserterCore::dfsComp(int i, node parent)
{
	// forall nodes in biconected component B(i) (except predecessor parent)
	for(node repT : m_nodeB[i])
	{
		if (repT == parent) continue;
		if (repT == m_t) { // t found?
			return repT;
		}
		if (dfsVertex(repT,i)) {
			return repT; // path found
		}
	}

	return nullptr; // path not found
}


// ExpandedGraphLight represents the (partially) expanded graph with
// its augmented dual
class VarEdgeInserterCore::ExpandedGraph
{
public:
	ExpandedGraph(const BiconnectedComponent &BC, const StaticSPQRTree &T, const GraphCopy &gc, const EdgeArray<bool> *pForbidden = nullptr)
		: m_T(T), m_BC(BC), m_gc(gc), m_pForbidden(pForbidden), m_GtoExp(T.originalGraph(),nullptr), m_expToG(m_exp,nullptr), m_primalEdge(m_dual,nullptr)
	{ }

	virtual ~ExpandedGraph() { }

	void expand(node v, edge eIn, edge eOut);

	virtual void constructDual(node s, node t);

	void findShortestPath(List<adjEntry> &L, Graph::EdgeType eType = Graph::EdgeType::association);
	void findWeightedShortestPath(List<adjEntry> &L, Graph::EdgeType eType = Graph::EdgeType::association);

	int costDual(edge eDual) const {
		adjEntry adjExp = m_primalEdge[eDual];
		return (adjExp == nullptr) ? 0 : m_BC.cost(m_expToG[adjExp]->theEdge());
	}

protected:
	virtual void appendCandidates(List<edge> &queue, node v, Graph::EdgeType eType);
	virtual void appendCandidates(Array<SListPure<edge> > &nodesAtDist, int maxCost, node v, Graph::EdgeType eType, int currentDist);

	edge insertEdge(node vG, node wG, edge eG);
	void expandSkeleton(node v, edge e1, edge e2);

	ExpandedGraph &operator=(const ExpandedGraph &) = delete;

	const StaticSPQRTree		&m_T;
	const BiconnectedComponent	&m_BC;
	const GraphCopy				&m_gc;
	const EdgeArray<bool>		*m_pForbidden;

	NodeArray<node> m_GtoExp;
	List<node>      m_nodesG;
	Graph           m_exp;   // expanded graph
	ConstCombinatorialEmbedding m_E;
	AdjEntryArray<adjEntry> m_expToG;
	edge            m_eS, m_eT; // (virtual) edges in exp representing s and t (if any)

	Graph           m_dual;  // augmented dual graph of exp
	EdgeArray<adjEntry> m_primalEdge;

	node m_vS, m_vT; // augmented nodes in dual representing s and t
};


class VarEdgeInserterUMLCore::ExpandedGraphUML : public VarEdgeInserterCore::ExpandedGraph
{
public:
	ExpandedGraphUML(const BiconnectedComponentUML &BC, const StaticSPQRTree &T, const GraphCopy &gc)
		: ExpandedGraph(BC,T,gc), m_primalIsGen(m_dual,false) { }

	~ExpandedGraphUML() { }

	void constructDual(node s, node t) override;

protected:
	void appendCandidates(List<edge> &queue, node v, Graph::EdgeType eType) override;
	void appendCandidates(Array<SListPure<edge> > &nodesAtDist, int maxCost, node v, Graph::EdgeType eType, int currentDist) override;

	EdgeArray<bool> m_primalIsGen; // true iff corresponding primal edge is a generalization
};


// build expanded graph (by expanding skeleton(v), edges eIn and eOut
// are the adjacent tree edges on the path from v1 to v2
void VarEdgeInserterCore::ExpandedGraph::expand(node v, edge eIn, edge eOut)
{
	m_exp.clear();
	while (!m_nodesG.empty())
		m_GtoExp[m_nodesG.popBackRet()] = nullptr;

	const Skeleton &S = m_T.skeleton(v);

	if (eIn != nullptr) {
		edge eInS = (v != eIn->source()) ? m_T.skeletonEdgeTgt(eIn) :
			m_T.skeletonEdgeSrc(eIn);
		node x = S.original(eInS->source()), y = S.original(eInS->target());
		m_eS = insertEdge(x,y,nullptr);
	}
	if (eOut != nullptr) {
		edge eOutS = (v != eOut->source()) ? m_T.skeletonEdgeTgt(eOut) :
			m_T.skeletonEdgeSrc(eOut);
		node x = S.original(eOutS->source()), y = S.original(eOutS->target());
		m_eT = insertEdge(x,y,nullptr);
	}

	expandSkeleton(v, eIn, eOut);

	planarEmbed(m_exp);
	m_E.init(m_exp);
}


// expand one skeleton (recursive construction)
void VarEdgeInserterCore::ExpandedGraph::expandSkeleton(node v, edge e1, edge e2)
{
	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_T.skeleton(v));
	const Graph          &M = S.getGraph();

	for(edge e : M.edges)
	{
		edge eG = S.realEdge(e);
		if (eG != nullptr) {
			insertEdge(eG->source(),eG->target(),eG);

		} else {
			edge eT = S.treeEdge(e);

			// do not expand virtual edges corresponding to tree edges e1 or e2
			if (eT != e1 && eT != e2) {
				expandSkeleton((v == eT->source()) ? eT->target() : eT->source(), eT , nullptr);
			}
		}
	}
}


// insert edge in exp (from a node corresponding to vG in G to a node
// corresponding to wG)
edge VarEdgeInserterCore::ExpandedGraph::insertEdge(node vG, node wG, edge eG)
{
	node &rVG = m_GtoExp[vG];
	node &rWG = m_GtoExp[wG];

	if (rVG == nullptr) {
		rVG = m_exp.newNode();
		m_nodesG.pushBack(vG);
	}
	if (rWG == nullptr) {
		rWG = m_exp.newNode();
		m_nodesG.pushBack(wG);
	}

	edge e1 = m_exp.newEdge(rVG,rWG);

	if(eG != nullptr) {
		m_expToG[e1->adjSource()] = eG->adjSource();
		m_expToG[e1->adjTarget()] = eG->adjTarget();
	} else {
		m_expToG[e1->adjSource()] = nullptr;
		m_expToG[e1->adjTarget()] = nullptr;
	}

	return e1;
}


// construct augmented dual of exp
void VarEdgeInserterCore::ExpandedGraph::constructDual(node s, node t)
{
	m_dual.clear();

	FaceArray<node> faceNode(m_E);

	// constructs nodes (for faces in exp)
	for(face f : m_E.faces) {
		faceNode[f] = m_dual.newNode();
	}

	// construct dual edges (for primal edges in exp)
	for(node v : m_exp.nodes)
	{
		for(adjEntry adj : v->adjEntries)
		{
			// cannot cross edges that does not correspond to real edges
			adjEntry adjG = m_expToG[adj];
			if(adjG == nullptr)
				continue;

			// Do not insert edges into dual if crossing the original edge
			// is forbidden
			if(m_pForbidden && (*m_pForbidden)[m_gc.original(m_BC.m_BCtoG[m_expToG[adj]]->theEdge())])
				continue;

			node vLeft  = faceNode[m_E.leftFace (adj)];
			node vRight = faceNode[m_E.rightFace(adj)];

			m_primalEdge[m_dual.newEdge(vLeft,vRight)] = adj;
		}
	}

	// augment dual by m_vS and m_vT
	m_vS = m_dual.newNode();
	if (m_GtoExp[s] != nullptr)
	{
		for(adjEntry adj : m_GtoExp[s]->adjEntries)
			m_dual.newEdge(m_vS,faceNode[m_E.rightFace(adj)]);
	}
	else
	{
		m_dual.newEdge(m_vS,faceNode[m_E.rightFace(m_eS->adjSource())]);
		m_dual.newEdge(m_vS,faceNode[m_E.rightFace(m_eS->adjTarget())]);
	}

	m_vT = m_dual.newNode();
	if (m_GtoExp[t] != nullptr)
	{
		for(adjEntry adj : m_GtoExp[t]->adjEntries)
			m_dual.newEdge(faceNode[m_E.rightFace(adj)], m_vT);
	}
	else
	{
		m_dual.newEdge(faceNode[m_E.rightFace(m_eT->adjSource())], m_vT);
		m_dual.newEdge(faceNode[m_E.rightFace(m_eT->adjTarget())], m_vT);
	}
}


void VarEdgeInserterUMLCore::ExpandedGraphUML::constructDual(node s, node t)
{
	const BiconnectedComponentUML &BC = dynamic_cast<const BiconnectedComponentUML &>(m_BC);
	m_dual.clear();

	FaceArray<node> faceNode(m_E);

	// constructs nodes (for faces in exp)
	for(face f : m_E.faces) {
		faceNode[f] = m_dual.newNode();
	}

	// construct dual edges (for primal edges in exp)
	for(node v : m_exp.nodes)
	{
		for(adjEntry adj : v->adjEntries)
		{
			// cannot cross edges that does not correspond to real edges
			adjEntry adjG = m_expToG[adj];
			if(adjG == nullptr)
				continue;

			node vLeft  = faceNode[m_E.leftFace (adj)];
			node vRight = faceNode[m_E.rightFace(adj)];

			edge e = m_dual.newEdge(vLeft,vRight);
			m_primalEdge[e] = adj;

			// mark dual edges corresponding to generalizations
			if (adjG && BC.typeOf(adjG->theEdge()) == Graph::EdgeType::generalization)
				m_primalIsGen[e] = true;

			OGDF_ASSERT(m_primalEdge[e] == nullptr || m_expToG[m_primalEdge[e]] != nullptr);
		}
	}

	// augment dual by m_vS and m_vT
	m_vS = m_dual.newNode();
	if (m_GtoExp[s] != nullptr) {
		for (adjEntry adj : m_GtoExp[s]->adjEntries) {
#ifdef OGDF_DEBUG
			edge eDual =
#endif
			m_dual.newEdge(m_vS,faceNode[m_E.rightFace(adj)]);
			OGDF_ASSERT(m_primalEdge[eDual] == nullptr || m_expToG[m_primalEdge[eDual]] != nullptr);
		}
	} else {
#ifdef OGDF_DEBUG
		edge eDual =
#endif
		m_dual.newEdge(m_vS,faceNode[m_E.rightFace(m_eS->adjSource())]);
		OGDF_ASSERT(m_primalEdge[eDual] == nullptr || m_expToG[m_primalEdge[eDual]] != nullptr);

#ifdef OGDF_DEBUG
		eDual =
#endif
		m_dual.newEdge(m_vS,faceNode[m_E.rightFace(m_eS->adjTarget())]);
		OGDF_ASSERT(m_primalEdge[eDual] == nullptr || m_expToG[m_primalEdge[eDual]] != nullptr);
	}

	m_vT = m_dual.newNode();
	if (m_GtoExp[t] != nullptr) {
		for (adjEntry adj : m_GtoExp[t]->adjEntries) {
#ifdef OGDF_DEBUG
			edge eDual =
#endif
			m_dual.newEdge(faceNode[m_E.rightFace(adj)], m_vT);
			OGDF_ASSERT(m_primalEdge[eDual] == nullptr || m_expToG[m_primalEdge[eDual]] != nullptr);
		}
	} else {
#ifdef OGDF_DEBUG
		edge eDual =
#endif
		m_dual.newEdge(faceNode[m_E.rightFace(m_eT->adjSource())], m_vT);
		OGDF_ASSERT(m_primalEdge[eDual] == nullptr || m_expToG[m_primalEdge[eDual]] != nullptr);

#ifdef OGDF_DEBUG
		eDual =
#endif
		m_dual.newEdge(faceNode[m_E.rightFace(m_eT->adjTarget())], m_vT);
		OGDF_ASSERT(m_primalEdge[eDual] == nullptr || m_expToG[m_primalEdge[eDual]] != nullptr);
	}
}

// find shortest path in dual from m_vS to m_vT; output this path
// in L by omitting first and last edge, and translating edges to G
void VarEdgeInserterCore::ExpandedGraph::appendCandidates(List<edge> &queue, node v, Graph::EdgeType /* eType */)
{
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if(v == e->source())
			queue.pushBack(e);
	}
}

void VarEdgeInserterUMLCore::ExpandedGraphUML::appendCandidates(List<edge> &queue, node v, Graph::EdgeType eType)
{
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if(v == e->source() && (eType != Graph::EdgeType::generalization || !m_primalIsGen[e])) {
			queue.pushBack(e);
		}
	}
}

void VarEdgeInserterCore::ExpandedGraph::findShortestPath(List<adjEntry> &L, Graph::EdgeType eType)
{
	NodeArray<edge> spPred(m_dual,nullptr); // predecessor in shortest path tree
	List<edge> queue; // candidate edges

	// start with all edges leaving from m_vS
	for(adjEntry adj : m_vS->adjEntries) {
		edge e = adj->theEdge();
		queue.pushBack(e);
	}

	for( ; ; ) {
		edge eCand = queue.popFrontRet(); // next candidate from front of queue
		node v = eCand->target();

		// hit an unvisited node ?
		if (spPred[v] == nullptr) {
			spPred[v] = eCand;

			// if it is m_vT, we have found the shortest path
			if (v == m_vT) {
				// build path from shortest path tree
				while(v != m_vS) {
					adjEntry adjExp = m_primalEdge[spPred[v]];
					if (adjExp != nullptr) // == nil for first and last edge
						L.pushFront(m_expToG[adjExp]);
					v = spPred[v]->source();
				}
				return;
			}

			// append next candidates to end of queue
			appendCandidates(queue, v, eType);
		}
	}
}


// find weighted shortest path in dual from m_vS to m_vT; output this path
// in L by omitting first and last edge, and translating edges to G
void VarEdgeInserterCore::ExpandedGraph::appendCandidates(
	Array<SListPure<edge> > &nodesAtDist, int maxCost, node v, Graph::EdgeType /*unused parameter*/, int currentDist)
{
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if(v == e->source()) {
			int listPos = (currentDist + costDual(e)) % maxCost;
			nodesAtDist[listPos].pushBack(e);
		}
	}
}

void VarEdgeInserterUMLCore::ExpandedGraphUML::appendCandidates(
	Array<SListPure<edge> > &nodesAtDist, int maxCost, node v, Graph::EdgeType eType, int currentDist)
{
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if(v == e->source() && (eType != Graph::EdgeType::generalization || !m_primalIsGen[e])) {
			int listPos = (currentDist + costDual(e)) % maxCost;
			nodesAtDist[listPos].pushBack(e);
		}
	}
}

void VarEdgeInserterCore::ExpandedGraph::findWeightedShortestPath(List<adjEntry> &L, Graph::EdgeType eType)
{
	int maxCost = 0;
	for(edge eDual : m_dual.edges) {
		int c = costDual(eDual);
		if (c > maxCost) maxCost = c;
	}

	++maxCost;
	Array<SListPure<edge> > nodesAtDist(maxCost);

	NodeArray<edge> spPred(m_dual,nullptr); // predecessor in shortest path tree

	// start with all edges leaving from m_vS
	for(adjEntry adj : m_vS->adjEntries) {
		edge e = adj->theEdge();
		nodesAtDist[0].pushBack(e);
	}

	// actual search (using extended bfs on directed dual)
	int currentDist = 0;
	for( ; ; ) {
		// next candidate edge
		while(nodesAtDist[currentDist % maxCost].empty())
			++currentDist;

		edge eCand = nodesAtDist[currentDist % maxCost].popFrontRet();
		node v = eCand->target();

		// leads to an unvisited node ?
		if (spPred[v] == nullptr) {
			// yes, then we set v's predecessor in search tree
			spPred[v] = eCand;

			// have we reached t ...
			if (v == m_vT) {
				// ... then search is done.
				// construct list of used edges (translated to crossed
				// adjacency entries in G)
				while(v != m_vS) {
					adjEntry adjExp = m_primalEdge[spPred[v]];
					if (adjExp != nullptr) // == nil for first and last edge
						L.pushFront(m_expToG[adjExp]);
					v = spPred[v]->source();
				}
				return;
			}

			// append next candidates to end of queue
			appendCandidates(nodesAtDist, maxCost, v, eType, currentDist);
		}
	}
}


// find optimal edge insertion path from s to t for biconnected
// graph G (OptimalBlockInserter)
VarEdgeInserterCore::ExpandedGraph *VarEdgeInserterCore::createExpandedGraph(const BiconnectedComponent &BC, const StaticSPQRTree &T)
{
	return new ExpandedGraph(BC, T, m_pr, m_pForbidden);
}

VarEdgeInserterCore::ExpandedGraph *VarEdgeInserterUMLCore::createExpandedGraph(const BiconnectedComponent &BC, const StaticSPQRTree &T)
{
	return new ExpandedGraphUML(dynamic_cast<const BiconnectedComponentUML &>(BC), T, m_pr);
}

void VarEdgeInserterCore::blockInsert(
	const BiconnectedComponent &BC,
	node s,
	node t,
	List<adjEntry> &L)
{
	L.clear();

	// construct SPQR-tree
	StaticPlanarSPQRTree T(BC);
	const Graph &tree = T.tree();


	// find allocation nodes of s and t and representatives in skeletons
	NodeArray<node> containsS(tree,nullptr);
	NodeArray<node> containsT(tree,nullptr);

	for(node v : tree.nodes) {
		const Skeleton &S = T.skeleton(v);
		const Graph &M = S.getGraph();

		for(node w : M.nodes) {
			if (S.original(w) == s)
				containsS[m_v1 = v] = w;
			if (S.original(w) == t)
				containsT[m_v2 = v] = w;
		}
	}

	// find path in tree from an allocation node m_v1 of s to an
	// allocation m_v2 of t
	List<edge> path;
	pathSearch(m_v1,nullptr,path);

	// remove unnecessary allocation nodes of s from start of path
	node w;
	while(!path.empty() && containsS[w = path.front()->opposite(m_v1)] != nullptr)
	{
		m_v1 = w;
		path.popFront();
	}

	// remove unnecessary allocation nodes of t from end of path
	while(!path.empty() && containsT[w = path.back()->opposite(m_v2)] != nullptr)
	{
		m_v2 = w;
		path.popBack();
	}

	// call build_subpath for every R-node building the list L of crossed edges
	ExpandedGraph *pExp = createExpandedGraph(BC,T);

	if (T.typeOf(m_v1) == SPQRTree::NodeType::RNode)
		buildSubpath(m_v1, nullptr, path.empty() ? nullptr : path.front(), L, *pExp, s, t);

	node v = m_v1;
	ListConstIterator<edge> it;
	for(it = path.begin(); it.valid(); ++it)
	{
		edge e = *it;
		v = e->opposite(v);

		if (T.typeOf(v) == SPQRTree::NodeType::RNode)
			buildSubpath(v, e, !it.succ().valid() ? nullptr : *it.succ(), L, *pExp, s, t);
	}

	delete pExp;
}

// recursive search for path from v1 to v2 in tree
bool VarEdgeInserterCore::pathSearch(node v, edge parent, List<edge> &path)
{
	if (v == m_v2)
		return true;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if (e == parent) continue;
		if (pathSearch(e->opposite(v),e,path)) {
			path.pushFront(e);
			return true;
		}
	}

	return false;
}


// find the shortest path from represent. of s to represent. of t in
// the dual of the (partially) expanded skeleton of v
void VarEdgeInserterCore::buildSubpath(
	node v,
	edge eIn,
	edge eOut,
	List<adjEntry> &L,
	ExpandedGraph &Exp,
	node s,
	node t)
{
	// build expanded graph Exp
	Exp.expand(v,eIn,eOut);

	// construct augmented dual of expanded graph
	Exp.constructDual(s,t);

	// find shortest path in augmented dual
	List<adjEntry> subpath;
	if(m_pCost != nullptr)
		Exp.findWeightedShortestPath(subpath);
	else
		Exp.findShortestPath(subpath);

	L.conc(subpath);
}

void VarEdgeInserterUMLCore::buildSubpath(
	node v,
	edge eIn,
	edge eOut,
	List<adjEntry> &L,
	ExpandedGraph &Exp,
	node s,
	node t)
{
	// build expanded graph Exp
	Exp.expand(v,eIn,eOut);

	// construct augmented dual of expanded graph
	Exp.constructDual(s,t);

	// find shortest path in augmented dual
	List<adjEntry> subpath;
	if(m_pCost != nullptr)
		Exp.findWeightedShortestPath(subpath, m_typeOfCurrentEdge);
	else
		Exp.findShortestPath(subpath, m_typeOfCurrentEdge);

	L.conc(subpath);
}

}
