/** \file
 * \brief Implementation of class VarEdgeInserterCore and VarEdgeInserterUMLCore,
 * which are the implementation classes for edge insertion with variable embedding.
 *
 * \author Carsten Gutwenger, Jan Papenfuß
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
#include <ogdf/planarity/embedding_inserter/VarEdgeInserterDynCore.h>
#include <ogdf/decomposition/DynamicSPQRForest.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/FaceArray.h>

namespace ogdf {

class VarEdgeInserterDynCore::BCandSPQRtrees
{
protected:
	PlanRepLight              &m_pr;
	DynamicSPQRForest          m_dynamicSPQRForest;

	const EdgeArray<int>*      m_costOrig;
	EdgeArray<int>             m_cost;

public:

	BCandSPQRtrees(PlanRepLight &pr, const EdgeArray<int>* costOrig);
	virtual ~BCandSPQRtrees() { }

	DynamicSPQRForest& dynamicSPQRForest() { return m_dynamicSPQRForest; }
	virtual void insertEdgePath(edge eOrig, const SList<adjEntry>& crossedEdges);

	void cost(edge e, int c) { m_cost[e] = c; }
	int cost(edge e) const { return m_cost[e]; }
};


class VarEdgeInserterDynUMLCore::BCandSPQRtreesUML : public VarEdgeInserterDynCore::BCandSPQRtrees
{
private:
	EdgeArray<Graph::EdgeType> m_typeOf;

public:
	BCandSPQRtreesUML(PlanRepLight &pr, const EdgeArray<int>* costOrig);

	void insertEdgePath(edge eOrig, const SList<adjEntry>& crossedEdges) override;

	void typeOf(edge e, Graph::EdgeType et) { m_typeOf[e] = et; }
	Graph::EdgeType typeOf(edge e) const { return m_typeOf[e]; }
};


VarEdgeInserterDynCore::BCandSPQRtrees::BCandSPQRtrees(
	PlanRepLight &pr, const EdgeArray<int>* costOrig)
	: m_pr(pr), m_dynamicSPQRForest(pr), m_costOrig(costOrig)
{
	const Graph& H = m_dynamicSPQRForest.auxiliaryGraph();
	m_cost.init(H);
	for (edge f : H.edges) {
		edge e = m_dynamicSPQRForest.original(f);
		if (m_costOrig) {
			edge eOrig = m_pr.original(e);
			m_cost[f] = eOrig ? (*m_costOrig)[eOrig] : 0;
		}
		else m_cost[f] = 1;
	}
}


VarEdgeInserterDynUMLCore::BCandSPQRtreesUML::BCandSPQRtreesUML(
	PlanRepLight &pr, const EdgeArray<int>* costOrig)
	: BCandSPQRtrees(pr, costOrig)
{
	const Graph& H = m_dynamicSPQRForest.auxiliaryGraph();
	m_typeOf.init(H);
	for (edge f : H.edges) {
		edge e = m_dynamicSPQRForest.original(f);
		m_typeOf[f] = m_pr.typeOf(e);
	}
}


void VarEdgeInserterDynCore::BCandSPQRtrees::insertEdgePath(
	edge eOrig, const SList<adjEntry>& crossedEdges)
{
	SList<edge> ti;
	SList<node> tj;
	for (adjEntry adj : crossedEdges) {
		ti.pushBack(adj->theEdge());
		tj.pushBack(adj->theEdge()->target());
	}

	m_pr.insertEdgePath(eOrig, crossedEdges);

	int costOfEOrig = m_costOrig ? eOrig ? (*m_costOrig)[eOrig] : 0 : 1;

	node v = m_pr.copy(eOrig->source());

	SListConstIterator<edge> it = ti.begin();
	SListConstIterator<node> jt = tj.begin();
	SListConstIterator<adjEntry> kt;
	for (kt = crossedEdges.begin(); it.valid(); ++it, ++jt, ++kt) {
		edge e = *it;
		node u = e->target();
		adjEntry a;
		for (a = u->firstAdj(); a->theEdge()->target() != *jt; a = a->succ())
			;
		edge f = a->theEdge();
		m_dynamicSPQRForest.updateInsertedNode(e, f);
		e = m_dynamicSPQRForest.rep(e);
		f = m_dynamicSPQRForest.rep(f);
		m_cost[f] = m_cost[e];
		for (a = u->firstAdj(); a->theEdge()->source() != v; a = a->succ());
		f = a->theEdge();
		m_dynamicSPQRForest.updateInsertedEdge(f);
		f = m_dynamicSPQRForest.rep(f);
		m_cost[f] = costOfEOrig;
		v = u;
	}
	node u = m_pr.copy(eOrig->target());
	adjEntry a;
	for (a = v->firstAdj(); a->theEdge()->target() != u; a = a->succ())
		;
	edge f = a->theEdge();
	m_dynamicSPQRForest.updateInsertedEdge(f);
	f = m_dynamicSPQRForest.rep(f);
	m_cost[f] = costOfEOrig;
}


void VarEdgeInserterDynUMLCore::BCandSPQRtreesUML::insertEdgePath(
	edge eOrig, const SList<adjEntry>& crossedEdges)
{
	SList<edge> ti;
	SList<node> tj;
	for (adjEntry adj : crossedEdges) {
		ti.pushBack(adj->theEdge());
		tj.pushBack(adj->theEdge()->target());
	}

	m_pr.insertEdgePath(eOrig, crossedEdges);

	Graph::EdgeType typeOfEOrig = m_pr.typeOrig(eOrig);
	int costOfEOrig = m_costOrig ? eOrig ? (*m_costOrig)[eOrig] : 0 : 1;

	node v = m_pr.copy(eOrig->source());
	SListConstIterator<edge> it = ti.begin();
	SListConstIterator<node> jt = tj.begin();
	SListConstIterator<adjEntry> kt;
	for (kt = crossedEdges.begin(); it.valid(); ++it, ++jt, ++kt) {
		edge e = *it;
		node u = e->target();
		adjEntry a;
		for (a = u->firstAdj(); a->theEdge()->target() != *jt; a = a->succ())
			;
		edge f = a->theEdge();
		m_dynamicSPQRForest.updateInsertedNode(e, f);
		e = m_dynamicSPQRForest.rep(e);
		f = m_dynamicSPQRForest.rep(f);
		m_typeOf[f] = m_typeOf[e];
		m_cost[f] = m_cost[e];
		for (a = u->firstAdj(); a->theEdge()->source() != v; a = a->succ());
		f = a->theEdge();
		m_dynamicSPQRForest.updateInsertedEdge(f);
		f = m_dynamicSPQRForest.rep(f);
		m_typeOf[f] = typeOfEOrig;
		m_cost[f] = costOfEOrig;
		v = u;
	}
	node u = m_pr.copy(eOrig->target());
	adjEntry a;
	for (a = v->firstAdj(); a->theEdge()->target() != u; a = a->succ())
		;
	edge f = a->theEdge();
	m_dynamicSPQRForest.updateInsertedEdge(f);
	f = m_dynamicSPQRForest.rep(f);
	m_typeOf[f] = typeOfEOrig;
	m_cost[f] = costOfEOrig;
}


// ExpandedGraph represents the (partially) expanded graph with
// its augmented dual
class VarEdgeInserterDynCore::ExpandedGraph
{
protected:
	BCandSPQRtrees &m_BC;
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

	node            m_vS, m_vT; // augmented nodes in dual representing s and t

public:
	ExpandedGraph(BCandSPQRtrees &BC, const GraphCopy &gc, const EdgeArray<bool> *pForbidden = nullptr)
		: m_BC(BC), m_gc(gc), m_pForbidden(pForbidden),
		m_GtoExp(BC.dynamicSPQRForest().auxiliaryGraph(), nullptr),
		m_expToG(m_exp, nullptr),
		m_primalEdge(m_dual, nullptr) { }

	virtual ~ExpandedGraph() { }

	void expand(node v, node vPred, node vSucc);

	virtual void constructDual(node s, node t);

	void findShortestPath(List<adjEntry> &L, Graph::EdgeType eType = Graph::EdgeType::association);
	void findWeightedShortestPath(List<adjEntry> &L, Graph::EdgeType eType = Graph::EdgeType::association);

	int costDual(edge eDual) const {
		adjEntry adjExp = m_primalEdge[eDual];
		return (adjExp == nullptr) ? 0 : m_BC.cost(m_expToG[adjExp]->theEdge());
	}

	// avoid automatic creation of assignment operator
	ExpandedGraph &operator=(const ExpandedGraph &);

protected:
	virtual void appendCandidates(List<edge> &queue, node v, Graph::EdgeType eType);
	virtual void appendCandidates(Array<SListPure<edge> > &nodesAtDist, int maxCost, node v, Graph::EdgeType eType, int currentDist);

	edge insertEdge(node vG, node wG, edge eG);
	void expandSkeleton(node v, edge e1, edge e2);
};


class VarEdgeInserterDynUMLCore::ExpandedGraphUML : public VarEdgeInserterDynCore::ExpandedGraph
{
public:
	ExpandedGraphUML(BCandSPQRtreesUML &BC, const GraphCopy &gc)
		: ExpandedGraph(BC, gc), m_primalIsGen(m_dual, false) { }

	void constructDual(node s, node t) override;

protected:
	void appendCandidates(List<edge> &queue, node v, Graph::EdgeType eType) override;
	void appendCandidates(Array<SListPure<edge> > &nodesAtDist, int maxCost, node v, Graph::EdgeType eType, int currentDist) override;

	EdgeArray<bool>     m_primalIsGen; // true iff corresponding primal edge is a generalization
};


// build expanded graph (by expanding skeleton(v), nodes vPred and
// vSucc are the predecessor and successor tree nodes of v on the
// path from v1 to v2
void VarEdgeInserterDynCore::ExpandedGraph::expand(node v, node vPred, node vSucc)
{
	m_exp.clear();
	while (!m_nodesG.empty())
		m_GtoExp[m_nodesG.popBackRet()] = nullptr;

	edge eInS = nullptr;
	if (vPred != nullptr) {
		eInS = m_BC.dynamicSPQRForest().virtualEdge(vPred, v);
		m_eS = insertEdge(eInS->source(), eInS->target(), nullptr);
	}
	edge eOutS = nullptr;
	if (vSucc != nullptr) {
		eOutS = m_BC.dynamicSPQRForest().virtualEdge(vSucc, v);
		m_eT = insertEdge(eOutS->source(), eOutS->target(), nullptr);
	}

	expandSkeleton(v, eInS, eOutS);

	planarEmbed(m_exp);
	m_E.init(m_exp);
}


// expand one skeleton (recursive construction)
void VarEdgeInserterDynCore::ExpandedGraph::expandSkeleton(node v, edge e1, edge e2)
{
	for (edge ei : m_BC.dynamicSPQRForest().hEdgesSPQR(v))
	{
		edge et = m_BC.dynamicSPQRForest().twinEdge(ei);

		if (et == nullptr)
			insertEdge(ei->source(), ei->target(), ei);

		// do not expand virtual edges corresponding to tree edges e1 or e2
		else if (ei != e1 && ei != e2)
			expandSkeleton(m_BC.dynamicSPQRForest().spqrproper(et), et, nullptr);
	}
}


// insert edge in exp (from a node corresponding to vG in G to a node
// corresponding to wG)
edge VarEdgeInserterDynCore::ExpandedGraph::insertEdge(node vG, node wG, edge eG)
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

	edge e1 = m_exp.newEdge(rVG, rWG);

	if (eG != nullptr) {
		m_expToG[e1->adjSource()] = eG->adjSource();
		m_expToG[e1->adjTarget()] = eG->adjTarget();
	}
	else {
		m_expToG[e1->adjSource()] = nullptr;
		m_expToG[e1->adjTarget()] = nullptr;
	}

	return e1;
}


// construct augmented dual of exp
void VarEdgeInserterDynCore::ExpandedGraph::constructDual(node s, node t)
{
	m_dual.clear();

	FaceArray<node> faceNode(m_E);

	// constructs nodes (for faces in exp)
	for (face f : m_E.faces) {
		faceNode[f] = m_dual.newNode();
	}

	// construct dual edges (for primal edges in exp)
	for (node v : m_exp.nodes)
	{
		for (adjEntry adj : v->adjEntries)
		{
			// cannot cross edges that does not correspond to real edges
			adjEntry adjG = m_expToG[adj];
			if (adjG == nullptr)
				continue;

			// Do not insert edges into dual if crossing the original edge
			// is forbidden
			if (m_pForbidden && (*m_pForbidden)[m_gc.original(m_BC.dynamicSPQRForest().original(m_expToG[adj]->theEdge()))])
				continue;

			node vLeft = faceNode[m_E.leftFace(adj)];
			node vRight = faceNode[m_E.rightFace(adj)];

			m_primalEdge[m_dual.newEdge(vLeft, vRight)] = adj;
		}
	}

	// augment dual by m_vS and m_vT
	m_vS = m_dual.newNode();
	if (m_GtoExp[s] != nullptr)
	{
		for (adjEntry adj : m_GtoExp[s]->adjEntries)
			m_dual.newEdge(m_vS, faceNode[m_E.rightFace(adj)]);
	}
	else
	{
		m_dual.newEdge(m_vS, faceNode[m_E.rightFace(m_eS->adjSource())]);
		m_dual.newEdge(m_vS, faceNode[m_E.rightFace(m_eS->adjTarget())]);
	}

	m_vT = m_dual.newNode();
	if (m_GtoExp[t] != nullptr)
	{
		for (adjEntry adj : m_GtoExp[t]->adjEntries)
			m_dual.newEdge(faceNode[m_E.rightFace(adj)], m_vT);
	}
	else
	{
		m_dual.newEdge(faceNode[m_E.rightFace(m_eT->adjSource())], m_vT);
		m_dual.newEdge(faceNode[m_E.rightFace(m_eT->adjTarget())], m_vT);
	}
}


void VarEdgeInserterDynUMLCore::ExpandedGraphUML::constructDual(node s, node t)
{
	VarEdgeInserterDynUMLCore::BCandSPQRtreesUML &BC
		= dynamic_cast<VarEdgeInserterDynUMLCore::BCandSPQRtreesUML&>(m_BC);
	m_dual.clear();

	FaceArray<node> faceNode(m_E);

	// constructs nodes (for faces in exp)
	for (face f : m_E.faces) {
		faceNode[f] = m_dual.newNode();
	}

#ifdef OGDF_DEBUG
	edge eDual;
#endif
	// construct dual edges (for primal edges in exp)
	for (node v : m_exp.nodes) {
		for (adjEntry adj : v->adjEntries) {
			// cannot cross edges that does not correspond to real edges
			adjEntry adjG = m_expToG[adj];
			if (adjG == nullptr)
				continue;

			node vLeft = faceNode[m_E.leftFace(adj)];
			node vRight = faceNode[m_E.rightFace(adj)];

			edge e = m_dual.newEdge(vLeft, vRight);
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
			eDual =
#endif
			m_dual.newEdge(m_vS, faceNode[m_E.rightFace(adj)]);
			OGDF_ASSERT(m_primalEdge[eDual] == nullptr || m_expToG[m_primalEdge[eDual]] != nullptr);
		}
	} else {
#ifdef OGDF_DEBUG
		eDual =
#endif
		m_dual.newEdge(m_vS, faceNode[m_E.rightFace(m_eS->adjSource())]);
		OGDF_ASSERT(m_primalEdge[eDual] == nullptr || m_expToG[m_primalEdge[eDual]] != nullptr);

#ifdef OGDF_DEBUG
		eDual =
#endif
		m_dual.newEdge(m_vS, faceNode[m_E.rightFace(m_eS->adjTarget())]);
		OGDF_ASSERT(m_primalEdge[eDual] == nullptr || m_expToG[m_primalEdge[eDual]] != nullptr);
	}

	m_vT = m_dual.newNode();
	if (m_GtoExp[t] != nullptr) {
		for (adjEntry adj : m_GtoExp[t]->adjEntries) {
#ifdef OGDF_DEBUG
			eDual =
#endif
			m_dual.newEdge(faceNode[m_E.rightFace(adj)], m_vT);
			OGDF_ASSERT(m_primalEdge[eDual] == nullptr || m_expToG[m_primalEdge[eDual]] != nullptr);
		}
	} else {
#ifdef OGDF_DEBUG
		eDual =
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
void VarEdgeInserterDynCore::ExpandedGraph::appendCandidates(List<edge> &queue, node v, Graph::EdgeType /* eType */)
{
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if (v == e->source())
			queue.pushBack(e);
	}
}

void VarEdgeInserterDynUMLCore::ExpandedGraphUML::appendCandidates(List<edge> &queue, node v, Graph::EdgeType eType)
{
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if (v == e->source() && (eType != Graph::EdgeType::generalization || !m_primalIsGen[e])) {
			queue.pushBack(e);
		}
	}
}

void VarEdgeInserterDynCore::ExpandedGraph::findShortestPath(List<adjEntry> &L, Graph::EdgeType eType)
{
	NodeArray<edge> spPred(m_dual, nullptr); // predecessor in shortest path tree
	List<edge> queue; // candidate edges

	// start with all edges leaving from m_vS
	for(adjEntry adj : m_vS->adjEntries) {
		edge e = adj->theEdge();
		queue.pushBack(e);
	}

	for (;;) {
		edge eCand = queue.popFrontRet(); // next candidate from front of queue
		node v = eCand->target();

		// hit an unvisited node ?
		if (spPred[v] == nullptr) {
			spPred[v] = eCand;

			// if it is m_vT, we have found the shortest path
			if (v == m_vT) {
				// build path from shortest path tree
				while (v != m_vS) {
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
void VarEdgeInserterDynCore::ExpandedGraph::appendCandidates(
	Array<SListPure<edge> > &nodesAtDist, int maxCost, node v, Graph::EdgeType /* unused parameter */, int currentDist)
{
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if (v == e->source()) {
			int listPos = (currentDist + costDual(e)) % maxCost;
			nodesAtDist[listPos].pushBack(e);
		}
	}
}

void VarEdgeInserterDynUMLCore::ExpandedGraphUML::appendCandidates(
	Array<SListPure<edge> > &nodesAtDist, int maxCost, node v, Graph::EdgeType eType, int currentDist)
{
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if (v == e->source() && (eType != Graph::EdgeType::generalization || !m_primalIsGen[e])) {
			int listPos = (currentDist + costDual(e)) % maxCost;
			nodesAtDist[listPos].pushBack(e);
		}
	}
}

void VarEdgeInserterDynCore::ExpandedGraph::findWeightedShortestPath(List<adjEntry> &L, Graph::EdgeType eType)
{
	int maxCost = 0;
	for (edge eDual : m_dual.edges) {
		int c = costDual(eDual);
		if (c > maxCost) maxCost = c;
	}

	++maxCost;
	Array<SListPure<edge> > nodesAtDist(maxCost);

	NodeArray<edge> spPred(m_dual, nullptr); // predecessor in shortest path tree

	// start with all edges leaving from m_vS
	for(adjEntry adj : m_vS->adjEntries) {
		edge e = adj->theEdge();
		nodesAtDist[0].pushBack(e);
	}

	// actual search (using extended bfs on directed dual)
	int currentDist = 0;
	for (;;) {
		// next candidate edge
		while (nodesAtDist[currentDist % maxCost].empty())
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
				while (v != m_vS) {
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

VarEdgeInserterDynCore::BCandSPQRtrees *VarEdgeInserterDynCore::createBCandSPQRtrees()
{
	return new BCandSPQRtrees(m_pr, m_pCost);
}

VarEdgeInserterDynCore::BCandSPQRtrees *VarEdgeInserterDynUMLCore::createBCandSPQRtrees()
{
	return new BCandSPQRtreesUML(m_pr, m_pCost);
}


// actual algorithm call
Module::ReturnType VarEdgeInserterDynCore::call(
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

	if (rrPost == RemoveReinsertType::Incremental) {
		for (edge e : m_pr.edges)
			currentOrigEdges.pushBack(m_pr.original(e));

		// insertion of edges
		for (int i = origEdges.low(); i <= origEdges.high(); ++i)
		{
			edge eOrig = origEdges[i];
			storeTypeOfCurrentEdge(eOrig);

			m_pBC = createBCandSPQRtrees();
			SList<adjEntry> eip;
			insert(eOrig, eip);
			m_pr.insertEdgePath(eOrig, eip);
			delete m_pBC;

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

					m_pBC = createBCandSPQRtrees();
					SList<adjEntry> iep;
					insert(eOrigRR, iep);
					m_pr.insertEdgePath(eOrigRR, iep);
					delete m_pBC;

					int newPathLength = (m_pCost != nullptr) ? costCrossed(eOrigRR) : (m_pr.chain(eOrigRR).size() - 1);
					OGDF_ASSERT(newPathLength <= pathLength);

					if (newPathLength < pathLength)
						improved = true;
				}
			} while (improved);
		}

	}
	else {

		// insertion of edges
		m_pBC = createBCandSPQRtrees();

		for (int i = origEdges.low(); i <= origEdges.high(); ++i)
		{
			edge eOrig = origEdges[i];
			storeTypeOfCurrentEdge(eOrig);

			SList<adjEntry> eip;
			insert(eOrig, eip);
			m_pBC->insertEdgePath(eOrig, eip);
		}

		delete m_pBC;

		// postprocessing (remove-reinsert heuristc)
		const int m = m_pr.original().numberOfEdges();
		SListPure<edge> rrEdges;

		switch (rrPost)
		{
		case RemoveReinsertType::All:
		case RemoveReinsertType::MostCrossed:
			for (int i = m_pr.startEdge(); i < m_pr.stopEdge(); ++i)
				rrEdges.pushBack(m_pr.e(i));
			break;

		case RemoveReinsertType::Inserted:
			for (int i = origEdges.low(); i <= origEdges.high(); ++i)
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

			if (rrPost == RemoveReinsertType::MostCrossed)
			{
				embedding_inserter::CrossingsBucket<PlanRepLight> bucket(&m_pr);
				rrEdges.bucketSort(bucket);

				const int num = int(0.01 * percentMostCrossed * m);
				itStop = rrEdges.get(num);
			}

			SListConstIterator<edge> it;
			for (it = rrEdges.begin(); it != itStop; ++it)
			{
				edge eOrig = *it;

				int pathLength = (m_pCost != nullptr) ? costCrossed(eOrig) : (m_pr.chain(eOrig).size() - 1);
				if (pathLength == 0) continue; // cannot improve

				m_pr.removeEdgePath(eOrig);

				storeTypeOfCurrentEdge(eOrig);

				m_pBC = createBCandSPQRtrees();
				SList<adjEntry> eip;
				insert(eOrig, eip);
				m_pr.insertEdgePath(eOrig, eip);
				delete m_pBC;

				// we cannot find a shortest path that is longer than before!
				int newPathLength = (m_pCost != nullptr) ? costCrossed(eOrig) : (m_pr.chain(eOrig).size() - 1);
				OGDF_ASSERT(newPathLength <= pathLength);

				if (newPathLength < pathLength)
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

static edge crossedEdge(adjEntry adj)
{
	edge e = adj->theEdge();

	adj = adj->cyclicSucc();
	while (adj->theEdge() == e)
		adj = adj->cyclicSucc();

	return adj->theEdge();
}


int VarEdgeInserterDynCore::costCrossed(edge eOrig) const
{
	int c = 0;

	const List<edge> &L = m_pr.chain(eOrig);

	ListConstIterator<edge> it = L.begin();
	if (m_pSubgraph != nullptr) {
		for (++it; it.valid(); ++it) {
			int counter = 0;
			edge e = m_pr.original(crossedEdge((*it)->adjSource()));
			for (int i = 0; i < 32; i++)
			if ((*m_pSubgraph)[eOrig] & (*m_pSubgraph)[e] & (1 << i))
				counter++;
			c += counter * (*m_pCost)[e];
		}
		c *= c_bigM;
		if (c == 0)
			c = 1;
	}
	else {
		for (++it; it.valid(); ++it) {
			c += (*m_pCost)[m_pr.original(crossedEdge((*it)->adjSource()))];
		}
	}

	return c;
}


// find optimal edge insertion path from s to t in connected
// graph G
void VarEdgeInserterDynCore::insert(edge eOrig, SList<adjEntry>& eip)
{
	eip.clear();
	node s = m_pr.copy(eOrig->source());
	node t = m_pr.copy(eOrig->target());

	// find path from s to t in BC-tree
	// call of blockInsert() is done when we have found the path
	// if no path is found, s and t are in different connected components
	// and thus an empty edge insertion path is correct!
	DynamicSPQRForest& dSPQRF = m_pBC->dynamicSPQRForest();
	SList<node>& path = dSPQRF.findPath(s, t);
	if (!path.empty()) {
		SListIterator<node> it = path.begin();
		node repS = dSPQRF.repVertex(s, *it);
		for (SListIterator<node> jt = it; it.valid(); ++it) {
			node repT = (++jt).valid() ? dSPQRF.cutVertex(*jt, *it) : dSPQRF.repVertex(t, *it);

			// less than 3 nodes requires no crossings (cannot build SPQR-tree
			// for a graph with less than 3 nodes!)
			if (dSPQRF.numberOfNodes(*it) > 3) {
				List<adjEntry> L;
				blockInsert(repS, repT, L); // call biconnected case

				// transform crossed edges to edges in G
				for (adjEntry kt : L) {
					edge e = kt->theEdge();
					eip.pushBack(e->adjSource() == kt ? dSPQRF.original(e)->adjSource()
						: dSPQRF.original(e)->adjTarget());
				}
			}
			if (jt.valid()) repS = dSPQRF.cutVertex(*it, *jt);
		}
	}
	delete &path;
}


// find optimal edge insertion path from s to t for biconnected
// graph G (OptimalBlockInserter)
VarEdgeInserterDynCore::ExpandedGraph *VarEdgeInserterDynCore::createExpandedGraph(BCandSPQRtrees &BC)
{
	return new ExpandedGraph(BC, m_pr, m_pForbidden);
}

VarEdgeInserterDynCore::ExpandedGraph *VarEdgeInserterDynUMLCore::createExpandedGraph(BCandSPQRtrees &BC)
{
	return new ExpandedGraphUML(dynamic_cast<BCandSPQRtreesUML &>(BC), m_pr);
}


void VarEdgeInserterDynCore::blockInsert(node s, node t, List<adjEntry> &L)
{
	L.clear();

	// find path in SPQR-tree from an allocation node of s
	// to an allocation node of t
	SList<node>& path = m_pBC->dynamicSPQRForest().findPathSPQR(s, t);

	// call build_subpath for every R-node building the list L of crossed edges
	ExpandedGraph *pExp = createExpandedGraph(*m_pBC);

	node vPred = nullptr;
	path.pushBack(nullptr);
	SListConstIterator<node> it;
	for (it = path.begin(); *it; ++it)
	{
		node v = *it;
		node vSucc = *it.succ();

		if (m_pBC->dynamicSPQRForest().typeOfTNode(v) == DynamicSPQRForest::TNodeType::RComp)
			buildSubpath(v, vPred, vSucc, L, *pExp, s, t);

		vPred = v;
	}

	delete &path;
	delete pExp;
}


// find the shortest path from represent. of s to represent. of t in
// the dual of the (partially) expanded skeleton of v
void VarEdgeInserterDynCore::buildSubpath(
	node v,
	node vPred,
	node vSucc,
	List<adjEntry> &L,
	ExpandedGraph &Exp,
	node s,
	node t)
{
	// build expanded graph Exp
	Exp.expand(v, vPred, vSucc);

	// construct augmented dual of expanded graph
	Exp.constructDual(s, t);

	// find shortest path in augmented dual
	List<adjEntry> subpath;
	if (m_pCost != nullptr)
		Exp.findWeightedShortestPath(subpath);
	else
		Exp.findShortestPath(subpath);

	L.conc(subpath);
}

void VarEdgeInserterDynUMLCore::buildSubpath(
	node v,
	node vPred,
	node vSucc,
	List<adjEntry> &L,
	ExpandedGraph &Exp,
	node s,
	node t)
{
	// build expanded graph Exp
	Exp.expand(v, vPred, vSucc);

	// construct augmented dual of expanded graph
	Exp.constructDual(s, t);

	// find shortest path in augmented dual
	List<adjEntry> subpath;
	if (m_pCost != nullptr)
		Exp.findWeightedShortestPath(subpath, m_typeOfCurrentEdge);
	else
		Exp.findShortestPath(subpath, m_typeOfCurrentEdge);

	L.conc(subpath);
}

}
