/** \file
 * \brief Implementation of GraphCopySimple and GraphCopy classes
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


#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/FaceSet.h>
#include <ogdf/basic/extended_graph_alg.h>


namespace ogdf {


GraphCopySimple::GraphCopySimple(const Graph &G)
{
	init(G);
}

GraphCopySimple::GraphCopySimple(const GraphCopySimple &GC) : Graph()
{
	*this = GC;
}

void GraphCopySimple::init(const Graph &G)
{
	m_pGraph = &G;

	Graph::construct(G,m_vCopy,m_eCopy);

	m_vOrig.init(*this,nullptr);
	m_eOrig.init(*this,nullptr);

	for(node v : G.nodes)
		m_vOrig[m_vCopy[v]] = v;

	for(edge e : G.edges)
		m_eOrig[m_eCopy[e]] = e;
}


GraphCopySimple &GraphCopySimple::operator=(const GraphCopySimple &GC)
{
	NodeArray<node> vCopy;
	EdgeArray<edge> eCopy;

	Graph::assign(GC,vCopy,eCopy);
	initGC(GC,vCopy,eCopy);

	return *this;
}


void GraphCopySimple::initGC(const GraphCopySimple &GC,
	NodeArray<node> &vCopy,
	EdgeArray<edge> &eCopy)
{
	m_pGraph = GC.m_pGraph;

	m_vOrig.init(*this,nullptr); m_eOrig.init(*this,nullptr);
	m_vCopy.init(*m_pGraph,nullptr); m_eCopy.init(*m_pGraph,nullptr);

	for(node v : GC.nodes) {
		node w = GC.m_vOrig[v];
		m_vOrig[vCopy[v]] = w;
		if(w != nullptr) {
			m_vCopy[w] = vCopy[v];
		}
	}

	for(edge e : GC.edges) {
		edge eOrig = GC.m_eOrig[e];
		m_eOrig[eCopy[e]] = eOrig;
		if (eOrig)
			m_eCopy[eOrig] = eCopy[e];
	}
}


void GraphCopySimple::delEdge(edge e) {
	edge eOrig = m_eOrig[e];
	Graph::delEdge(e);
	if (eOrig != nullptr) { m_eCopy[eOrig] = nullptr; }
}


void GraphCopySimple::delNode(node v) {
	node vOrig = m_vOrig[v];
	Graph::delNode(v);
	if (vOrig != nullptr) { m_vCopy[vOrig] = nullptr; }
}


GraphCopy::GraphCopy(const Graph &G)
{
	init(G);
}


GraphCopy::GraphCopy(const GraphCopy &GC) : Graph()
{
	*this = GC;
}


void GraphCopy::initGC(const GraphCopy &GC,
	NodeArray<node> &vCopy,
	EdgeArray<edge> &eCopy)
{
	createEmpty(*GC.m_pGraph);

	for(node v : GC.nodes)
		m_vOrig[vCopy[v]] = GC.original(v);

	for(edge e : GC.edges)
		m_eOrig[eCopy[e]] = GC.original(e);

	for (node v : nodes) {
		node w = m_vOrig[v];
		if (w != nullptr)
			m_vCopy[w] = v;
	}

	for(edge e : m_pGraph->edges) {
		for (edge ei : GC.m_eCopy[e])
			m_eIterator[eCopy[ei]] = m_eCopy[e].pushBack(eCopy[ei]);
	}
}


void GraphCopy::init(const Graph &G)
{
	m_pGraph = &G;

	EdgeArray<edge> eCopy;
	Graph::construct(G,m_vCopy,eCopy);

	m_vOrig.init(*this,nullptr);
	m_eOrig.init(*this,nullptr);
	m_eCopy.init(G);
	m_eIterator.init(*this,nullptr);

	for(node v : G.nodes)
		m_vOrig[m_vCopy[v]] = v;

	for(edge e : G.edges) {
		m_eIterator[eCopy[e]] = m_eCopy[e].pushBack(eCopy[e]);
		m_eOrig[eCopy[e]] = e;
	}
}


void GraphCopy::createEmpty(const Graph &G)
{
	m_pGraph = &G;

	m_vCopy.init(G,nullptr);
	m_eCopy.init(G);
	m_vOrig.init(*this,nullptr);
	m_eOrig.init(*this,nullptr);
	m_eIterator.init(*this,nullptr);
}


void GraphCopy::initByCC(const CCsInfo &info, int cc, EdgeArray<edge> &eCopy)
{
	eCopy.init(*m_pGraph);

	m_vCopy.init(*m_pGraph, nullptr);
	m_eCopy.init(*m_pGraph);
	Graph::constructInitByCC(info, cc, m_vCopy, eCopy);

	for(int i = info.startNode(cc); i < info.stopNode(cc); ++i) {
		node v = info.v(i);
		m_vOrig[m_vCopy[v]] = v;
	}

	for(int i = info.startEdge(cc); i < info.stopEdge(cc); ++i) {
		edge e = info.e(i);
		m_eIterator[eCopy[e]] = m_eCopy[e].pushBack(eCopy[e]);
		m_eOrig[eCopy[e]] = e;
	}

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


void GraphCopy::initByNodes(const List<node> &origNodes, EdgeArray<edge> &eCopy)
{
#ifdef OGDF_DEBUG
	ArrayBuffer<node> stack;
	List<node> copyList(origNodes);
	while (!copyList.empty()) {
		stack.push(copyList.popFrontRet());
		node v;
		while (!stack.empty()) {
			v = stack.popRet();
			for (adjEntry adj : v->adjEntries) {
				node w = adj->twinNode();
				OGDF_ASSERT(origNodes.search(w).valid());
				auto it = copyList.search(w);
				if (it.valid()) {
					stack.push(w);
					copyList.del(it);
				}
			}
		}
	}
#endif

	m_vCopy.init(*m_pGraph, nullptr);
	m_eCopy.init(*m_pGraph);
	Graph::constructInitByNodes(*m_pGraph,origNodes,m_vCopy,eCopy);

	for(node v : origNodes)
	{
		m_vOrig[m_vCopy[v]] = v;

		for(adjEntry adj : v->adjEntries) {
			if ((adj->index() & 1) == 0) {
				edge e = adj->theEdge();
				m_eIterator[eCopy[e]] = m_eCopy[e].pushBack(eCopy[e]);
				m_eOrig[eCopy[e]] = e;
			}
		}
	}

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


void GraphCopy::initByActiveNodes(
	const List<node> &nodeList,
	const NodeArray<bool> &activeNodes,
	EdgeArray<edge> &eCopy)
{
	m_vCopy.init(*m_pGraph, nullptr);
	m_eCopy.init(*m_pGraph);
	Graph::constructInitByActiveNodes(nodeList, activeNodes, m_vCopy, eCopy);

	for(node v : nodeList)
	{
		m_vOrig[m_vCopy[v]] = v;

		for(adjEntry adj : v->adjEntries) {
			if ((adj->index() & 1) == 0) {
				edge e = adj->theEdge();
				OGDF_ASSERT(m_eCopy[e].size() == 0);
				if (activeNodes[e->opposite(v)])
				{
					m_eIterator[eCopy[e]] = m_eCopy[e].pushBack(eCopy[e]);
					m_eOrig[eCopy[e]] = e;
				}
			}
		}
	}

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


GraphCopy &GraphCopy::operator=(const GraphCopy &GC)
{
	m_pGraph = nullptr;

	NodeArray<node> vCopy;
	EdgeArray<edge> eCopy;

	Graph::assign(GC,vCopy,eCopy);
	if (GC.m_pGraph != nullptr) {
		initGC(GC,vCopy,eCopy);
	}

	return *this;
}


void GraphCopy::setOriginalEmbedding()
{
	OGDF_ASSERT(m_pGraph->numberOfNodes() == numberOfNodes());
	OGDF_ASSERT(m_pGraph->numberOfEdges() == numberOfEdges());
	for (node v : m_pGraph->nodes) {
		OGDF_ASSERT(m_vCopy[v] != nullptr);
		List<adjEntry> newAdjOrder;
		newAdjOrder.clear();

		for (adjEntry adjOr : v->adjEntries) {
			OGDF_ASSERT(m_eCopy[adjOr->theEdge()].size() > 0);
			// we have outgoing adjEntries for all
			// incoming and outgoing edges, check the direction
			// to find the correct copy adjEntry
			bool outEdge = (adjOr == (adjOr->theEdge()->adjSource()));

			OGDF_ASSERT(chain(adjOr->theEdge()).size() == 1);
			edge cEdge = chain(adjOr->theEdge()).front();
			adjEntry cAdj = (outEdge ? cEdge->adjSource() : cEdge->adjTarget());
			newAdjOrder.pushBack(cAdj);
		}
		sort(copy(v), newAdjOrder);
	}
}


edge GraphCopy::split(edge e)
{
	edge eNew  = Graph::split(e);
	edge eOrig = m_eOrig[e];

	if ((m_eOrig[eNew] = eOrig) != nullptr) {
		m_eIterator[eNew] = m_eCopy[eOrig].insert(eNew,m_eIterator[e],Direction::after);
	}

	return eNew;
}


void GraphCopy::unsplit(edge eIn, edge eOut)
{
	edge eOrig = m_eOrig[eOut];

	// update chain of eOrig if eOrig exists
	if (eOrig != nullptr) {
		m_eCopy[eOrig].del(m_eIterator[eOut]);
	}

	Graph::unsplit(eIn,eOut);
}


edge GraphCopy::newEdge(edge eOrig)
{
	OGDF_ASSERT(eOrig != nullptr);
	OGDF_ASSERT(eOrig->graphOf() == m_pGraph);
	OGDF_ASSERT(m_eCopy[eOrig].empty()); // no support for edge splitting!

	edge e = Graph::newEdge(m_vCopy[eOrig->source()], m_vCopy[eOrig->target()]);
	m_eIterator[e] = m_eCopy[m_eOrig[e] = eOrig].pushBack(e);

	return e;
}


//inserts edge preserving the embedding
//todo: rename adjEnd to show the symmetric character
edge GraphCopy::newEdge(node v, adjEntry adjEnd, edge eOrig, CombinatorialEmbedding &E)
{
	OGDF_ASSERT(v != nullptr);
	OGDF_ASSERT(adjEnd != nullptr);
	OGDF_ASSERT(v->graphOf() == this);
	OGDF_ASSERT(adjEnd->graphOf() == this);
	OGDF_ASSERT(&E.getGraph() == this);
	OGDF_ASSERT(m_eCopy[eOrig].empty());

	//check which direction is correct
	edge e;
	if (original(v) == eOrig->source())
		e = E.addEdgeToIsolatedNode(v, adjEnd);
	else
		e = E.addEdgeToIsolatedNode(adjEnd, v);
	m_eIterator[e] = m_eCopy[eOrig].pushBack(e);
	m_eOrig[e] = eOrig;

	return e;
}

void GraphCopy::setEdge(edge eOrig, edge eCopy){
	OGDF_ASSERT(eOrig != nullptr);
	OGDF_ASSERT(eOrig->graphOf() == m_pGraph);
	OGDF_ASSERT(eCopy != nullptr);
	OGDF_ASSERT(eCopy->graphOf() == this);
	OGDF_ASSERT(eCopy->target() == m_vCopy[eOrig->target()]);
	OGDF_ASSERT(eCopy->source() == m_vCopy[eOrig->source()]);
	OGDF_ASSERT(m_eCopy[eOrig].empty());

	m_eCopy[m_eOrig[eCopy] = eOrig].pushBack(eCopy);
}


void GraphCopy::insertEdgePathEmbedded(
	edge eOrig,
	CombinatorialEmbedding &E,
	const SList<adjEntry> &crossedEdges)
{
	if(m_eCopy[eOrig].size() != 0){
		FaceSet<false> fsp(E);
		removeEdgePathEmbedded(E, eOrig, fsp);
	}
	m_eCopy[eOrig].clear();

	adjEntry adjSrc, adjTgt;
	SListConstIterator<adjEntry> it = crossedEdges.begin();

	// iterate over all adjacency entries in crossedEdges except for first
	// and last
	adjSrc = *it;
	for(++it; it.valid() && it.succ().valid(); ++it)
	{
		adjEntry adj = *it;
		// split edge
		node u = E.split(adj->theEdge())->source();

		// determine target adjacency entry and source adjacency entry
		// in the next iteration step
		adjTgt = u->firstAdj();
		adjEntry adjSrcNext = adjTgt->succ();

		if (adjTgt != adj->twin())
			std::swap(adjTgt,adjSrcNext);

		// insert a new edge into the face
		edge eNew = E.splitFace(adjSrc,adjTgt);
		m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
		m_eOrig[eNew] = eOrig;

		adjSrc = adjSrcNext;
	}

	// insert last edge
	edge eNew = E.splitFace(adjSrc,*it);
	m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
	m_eOrig[eNew] = eOrig;

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


void GraphCopy::insertEdgePath(edge eOrig, const SList<adjEntry> &crossedEdges)
{
	if(m_eCopy[eOrig].size() != 0){
		removeEdgePath(eOrig);
	}
	node v = copy(eOrig->source());

	for(adjEntry adj : crossedEdges)
	{
		node u = split(adj->theEdge())->source();

		edge eNew = newEdge(v,u);
		m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
		m_eOrig[eNew] = eOrig;

		v = u;
	}

	edge eNew = newEdge(v,copy(eOrig->target()));
	m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
	m_eOrig[eNew] = eOrig;

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}

void GraphCopy::insertEdgePath(node srcOrig, node tgtOrig, const SList<adjEntry> &crossedEdges)
{
	node v = copy(srcOrig);

	for(adjEntry adj : crossedEdges)
	{
		node u = split(adj->theEdge())->source();

		edge eNew = newEdge(v,u);
#if 0
		m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
#endif
		m_eOrig[eNew] = nullptr;

		v = u;
	}

	edge eNew = newEdge(v,copy(tgtOrig));
#if 0
	m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
#endif
	m_eOrig[eNew] = nullptr;
}

edge GraphCopy::insertCrossing(
	edge& crossingEdge,
	edge crossedEdge,
	bool rightToLeft)
#if 0
	const SList<edge> &crossedCopies)
#endif
{
	edge e = split(crossedEdge);

	// insert edges replacing the crossing edge
	adjEntry adjIn = e->adjSource();
	adjEntry adjOut = e->adjSource()->cyclicPred();

	if (!rightToLeft) {
		std::swap(adjIn, adjOut);
	}

	edge eNew1 = newEdge(crossingEdge->adjSource(), adjIn);
	edge eNew2 = newEdge(adjOut, crossingEdge->adjTarget()->cyclicPred());

	// restore copy mapping
	edge eOrig = original(crossingEdge);

	if(eOrig != nullptr) {
		m_eIterator[eNew1] = m_eCopy[eOrig].insert(eNew1, m_eIterator[crossingEdge]);
		m_eIterator[eNew2] = m_eCopy[eOrig].insert(eNew2, m_eIterator[eNew1]);
	}

	m_eOrig[eNew1] = eOrig;
	m_eOrig[eNew2] = eOrig;

	// remove crossing edge
	if(eOrig != nullptr) {
		m_eCopy[eOrig].del(m_eIterator[crossingEdge]);
	}
	Graph::delEdge(crossingEdge);
	crossingEdge = eNew2;

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif

	return e;
}

void GraphCopy::delEdge(edge e)
{
	edge eOrig = m_eOrig[e];

	Graph::delEdge(e);
	if (eOrig == nullptr)	return;

	OGDF_ASSERT(m_eCopy[eOrig].size() == 1);
	m_eCopy[eOrig].clear();
}


void GraphCopy::delNode(node v)
{
	node w = m_vOrig[v];
	if (w != nullptr) m_vCopy[w] = nullptr;

	Graph::delNode(v);
}

void GraphCopy::clear()
{
	if(m_pGraph != nullptr) {
		m_vCopy.init(*m_pGraph, nullptr);
		m_eCopy.init(*m_pGraph);
	}

	Graph::clear();
}


void GraphCopy::removeEdgePathEmbedded(
	CombinatorialEmbedding &E,
	edge eOrig,
	FaceSet<false> &newFaces)
{
	const List<edge> &path = m_eCopy[eOrig];
#ifdef OGDF_DEBUG
	ListConstIterator<edge> testIt = path.begin();
	for(++testIt; testIt.valid(); ++testIt){
		node v = (*testIt)->source();
		OGDF_ASSERT(v->degree() == 4);
		OGDF_ASSERT(original(v->firstAdj()->theEdge()) == original(v->lastAdj()->pred()->theEdge()));
		OGDF_ASSERT(original(v->lastAdj()->theEdge()) == original(v->firstAdj()->succ()->theEdge()));
	}
#endif

	ListConstIterator<edge> it = path.begin();

	newFaces.insert(E.joinFacesPure(*it));
	Graph::delEdge(*it);

	for(++it; it.valid(); ++it)
	{
		edge e = *it;
		node u = e->source();

		newFaces.remove(E.rightFace(e->adjSource()));
		newFaces.remove(E.rightFace(e->adjTarget()));

		newFaces.insert(E.joinFacesPure(e));
		Graph::delEdge(e);

		edge eIn = u->firstAdj()->theEdge();
		edge eOut = u->lastAdj()->theEdge();
		if (eIn->target() != u)
			std::swap(eIn,eOut);

		E.unsplit(eIn,eOut);
	}

	m_eCopy[eOrig].clear();

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


void GraphCopy::removeEdgePath(edge eOrig)
{
	const List<edge> &path = m_eCopy[eOrig];
#ifdef OGDF_DEBUG
	ListConstIterator<edge> testIt = path.begin();
	for(++testIt; testIt.valid(); ++testIt){
		node v = (*testIt)->source();
		OGDF_ASSERT(v->degree() == 4);
	}
#endif
	ListConstIterator<edge> it = path.begin();

	Graph::delEdge(*it);

	for(++it; it.valid(); ++it)
	{
		edge e = *it;
		node u = e->source();

		Graph::delEdge(e);

		edge eIn = u->firstAdj()->theEdge();
		edge eOut = u->lastAdj()->theEdge();
		if (eIn->target() != u)
			std::swap(eIn,eOut);

		unsplit(eIn,eOut);
	}

	m_eCopy[eOrig].clear();

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


void GraphCopy::removeUnnecessaryCrossing(
	adjEntry adjA1,
	adjEntry adjA2,
	adjEntry adjB1,
	adjEntry adjB2)
{
	node v = adjA1->theNode();

	if(adjA1->theEdge()->source() == v)
		moveSource(adjA1->theEdge(), adjA2->twin(), Direction::before);
	else
		moveTarget(adjA1->theEdge(), adjA2->twin(), Direction::before);

	if(adjB1->theEdge()->source() == v)
		moveSource(adjB1->theEdge(), adjB2->twin(), Direction::before);
	else
		moveTarget(adjB1->theEdge(), adjB2->twin(), Direction::before);

	edge eOrigA = original(adjA1->theEdge());
	edge eOrigB = original(adjB1->theEdge());

	if (eOrigA != nullptr)
		m_eCopy[eOrigA].del(m_eIterator[adjA2->theEdge()]);
	if (eOrigB != nullptr)
		m_eCopy[eOrigB].del(m_eIterator[adjB2->theEdge()]);

	Graph::delEdge(adjB2->theEdge());
	Graph::delEdge(adjA2->theEdge());

	delNode(v);

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


bool GraphCopy::embed()
{
	return planarEmbed(*this);
}


void GraphCopy::removePseudoCrossings()
{
	node v, vSucc;
	for(v = firstNode(); v != nullptr; v = vSucc)
	{
		vSucc = v->succ();

		if (original(v) != nullptr || v->degree() != 4)
			continue;

		adjEntry adj1 = v->firstAdj();
		adjEntry adj2 = adj1->succ();
		adjEntry adj3 = adj2->succ();
		adjEntry adj4 = adj3->succ();

		if(original(adj1->theEdge()) == original(adj2->theEdge()))
			removeUnnecessaryCrossing(adj1,adj2,adj3,adj4);
		else if (original(adj2->theEdge()) == original(adj3->theEdge()))
			removeUnnecessaryCrossing(adj2,adj3,adj4,adj1);
	}

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


bool GraphCopy::isReversedCopyEdge (edge e) const {
	List<edge> chainOfE = chain(original(e));
#ifdef OGDF_DEBUG
	auto it = chainOfE.begin();
	edge prev = *it;
	OGDF_ASSERT(prev->isIncident(copy(original(e)->source())) ||
	            prev->isIncident(copy(original(e)->target())));
	for(++it; it.valid(); prev = *it, ++it){
		OGDF_ASSERT(prev->commonNode(*it));
	}
	OGDF_ASSERT(prev->isIncident(copy(original(e)->source())) ||
	            prev->isIncident(copy(original(e)->target())));
#endif
	int pos = chainOfE.pos(chainOfE.search(e));
	if(chainOfE.size() == 1){
		return isReversed(original(e));
	}
	if(pos == 0){
		return e->commonNode(*(chainOfE.get(pos + 1))) == e->source();
	} else {
		return e->commonNode(*(chainOfE.get(pos - 1))) == e->target();
	}
}

#ifdef OGDF_DEBUG
void GraphCopy::consistencyCheck() const
{
	Graph::consistencyCheck();

	const Graph &G = *m_pGraph;

	for (node vG : G.nodes) {
		node v = m_vCopy[vG];

		if(v != nullptr) {
			OGDF_ASSERT(v->graphOf() == this);
			OGDF_ASSERT(m_vOrig[v] == vG);
		}
	}

	for (node v : nodes) {
		node vG = m_vOrig[v];

		if(vG != nullptr) {
			OGDF_ASSERT(vG->graphOf() == &G);
			OGDF_ASSERT(m_vCopy[vG] == v);
		}
	}

	for (edge eG : G.edges) {
		const List<edge> &path = m_eCopy[eG];

		for (edge e : path) {
			OGDF_ASSERT(e->graphOf() == this);
			OGDF_ASSERT(m_eOrig[e] == eG);
		}
	}

	for (edge e : edges) {
		edge eG = m_eOrig[e];

		if (eG != nullptr) {
			OGDF_ASSERT(eG->graphOf() == &G);
		}
	}
}
#endif

}
