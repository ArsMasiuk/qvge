/** \file
 * \brief Implementation of PlanRep base class for planar rep.
 *
 * \author Hoi-Ming Wong
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

#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/upward/UpwardPlanRep.h>
#include <ogdf/upward/FaceSinkGraph.h>
#include <ogdf/basic/FaceSet.h>
#include <ogdf/basic/tuples.h>



namespace ogdf {


UpwardPlanRep::UpwardPlanRep(const CombinatorialEmbedding &Gamma) :
	GraphCopy(Gamma.getGraph()),
	isAugmented(false),
	t_hat(nullptr),
	extFaceHandle(nullptr),
	crossings(0)
{
	OGDF_ASSERT(Gamma.externalFace() != nullptr);
	OGDF_ASSERT(hasSingleSource(*this));
	OGDF_ASSERT(isSimple(*this));

	m_isSourceArc.init(*this, false);
	m_isSinkArc.init(*this, false);
	hasSingleSource(*this, s_hat);
	m_Gamma.init(*this);

	//compute the ext. face;
	adjEntry adj;
	node v = this->original(s_hat);
	adj = getAdjEntry(Gamma, v, Gamma.externalFace());
	adj = this->copy(adj->theEdge())->adjSource();
	m_Gamma.setExternalFace(m_Gamma.rightFace(adj));

	//outputFaces(Gamma);

	computeSinkSwitches();
}


UpwardPlanRep::UpwardPlanRep(const GraphCopy &GC, ogdf::adjEntry adj_ext) :
	GraphCopy(GC),
	isAugmented(false),
	t_hat(nullptr),
	extFaceHandle(nullptr),
	crossings(0)
{
	OGDF_ASSERT(adj_ext != nullptr);
	OGDF_ASSERT(hasSingleSource(*this));

	m_isSourceArc.init(*this, false);
	m_isSinkArc.init(*this, false);
	hasSingleSource(*this, s_hat);
	m_Gamma.init(*this);

	//compute the ext. face;
	node v = copy(GC.original(adj_ext->theNode()));
	extFaceHandle = copy(GC.original(adj_ext->theEdge()))->adjSource();
	if (extFaceHandle->theNode() != v)
		extFaceHandle = extFaceHandle->twin();
	m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));

	for(adjEntry adj : s_hat->adjEntries)
		m_isSourceArc[adj->theEdge()] = true;

	computeSinkSwitches();
}


//copy constructor
UpwardPlanRep::UpwardPlanRep(const UpwardPlanRep &UPR) :
	GraphCopy(),
	isAugmented(UPR.isAugmented),
	crossings(UPR.crossings)
{
	copyMe(UPR);
}


void UpwardPlanRep::copyMe(const UpwardPlanRep &UPR)
{
	NodeArray<node> vCopy;
	EdgeArray<edge> eCopy;

	Graph::construct(UPR, vCopy, eCopy);

	// initGC
	m_pGraph = UPR.m_pGraph;

	m_vOrig.init(*this, nullptr); m_eOrig.init(*this, nullptr);
	m_vCopy.init(*m_pGraph, nullptr); m_eCopy.init(*m_pGraph);
	m_eIterator.init(*this, nullptr);

	for (node v : UPR.nodes)
		m_vOrig[vCopy[v]] = UPR.m_vOrig[v];

	for (edge e : UPR.edges)
		m_eOrig[eCopy[e]] = UPR.m_eOrig[e];

	for (node v : nodes) {
		node w = m_vOrig[v];
		if (w != nullptr) m_vCopy[w] = v;
	}

	for(edge e : m_pGraph->edges) {
		ListConstIterator<edge> it;
		for (it = UPR.m_eCopy[e].begin(); it.valid(); ++it)
			m_eIterator[eCopy[*it]] = m_eCopy[e].pushBack(eCopy[*it]);
	}

	//GraphCopy::initGC(UPR,vCopy,eCopy);
	m_Gamma.init(*this);
	m_isSinkArc.init(*this, false);
	m_isSourceArc.init(*this, false);

	if (UPR.numberOfNodes() == 0)
		return;

	s_hat = vCopy[UPR.getSuperSource()];
	if (UPR.augmented())
		t_hat = vCopy[UPR.getSuperSink()];

	OGDF_ASSERT(UPR.extFaceHandle != nullptr);

	edge eC = eCopy[UPR.extFaceHandle->theEdge()];
	node vC = vCopy[UPR.extFaceHandle->theNode()];
	if (eC->adjSource()->theNode() == vC)
		extFaceHandle = eC->adjSource();
	else
		extFaceHandle = eC->adjTarget();

	m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));

	for(edge e : UPR.edges) {
		edge a = eCopy[e];
		if (UPR.isSinkArc(e))
			m_isSinkArc[a] = true;
		if (UPR.isSourceArc(e))
			m_isSourceArc[a] = true;
	}

	computeSinkSwitches();
}



UpwardPlanRep & UpwardPlanRep::operator =(const UpwardPlanRep &cp)
{
	clear();
	createEmpty(cp.original());
	isAugmented = cp.isAugmented;
	extFaceHandle = nullptr;
	crossings = cp.crossings;
	copyMe(cp);
	return *this;
}


void UpwardPlanRep::augment()
{
	if (isAugmented)
		return;

	OGDF_ASSERT(hasSingleSource(*this));

	List<adjEntry> switches;

	hasSingleSource(*this, s_hat);
	OGDF_ASSERT(this == &m_Gamma.getGraph());

	for(adjEntry adj : s_hat->adjEntries)
		m_isSourceArc[adj->theEdge()] = true;

	FaceSinkGraph fsg(m_Gamma, s_hat);
	List<adjEntry> dummyList;
	FaceArray< List<adjEntry> > sinkSwitches(m_Gamma, dummyList);
	fsg.sinkSwitches(sinkSwitches);
	m_sinkSwitchOf.init(*this, nullptr);

	List<Tuple2<adjEntry, adjEntry>> list;
	for(face f : m_Gamma.faces) {
		adjEntry adj_top;
		switches = sinkSwitches[f];
		if (switches.empty() || f == m_Gamma.externalFace())
			continue;
		else
			adj_top = switches.popFrontRet(); // first switch in the list is a top sink switch

		while (!switches.empty()) {
			adjEntry adj = switches.popFrontRet();
			Tuple2<adjEntry, adjEntry> pair(adj, adj_top);
			list.pushBack(pair);
		}
	}
	// construct sink arcs
	// for the ext. face
	extFaceHandle = getAdjEntry(m_Gamma, s_hat, m_Gamma.externalFace());
	node t = this->newNode();
	switches = sinkSwitches[m_Gamma.externalFace()];

	OGDF_ASSERT(!switches.empty());

	while (!switches.empty()) {
		adjEntry adj = switches.popFrontRet();
		edge e_new;
		if (t->degree() == 0) {
			e_new = m_Gamma.addEdgeToIsolatedNode(adj, t);
		}
		else {
			adjEntry adjTgt = getAdjEntry(m_Gamma, t, m_Gamma.rightFace(adj));
			e_new = m_Gamma.splitFace(adj, adjTgt);
		}
		m_isSinkArc[e_new] = true;
		m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));
	}

	/*
	* set ext. face handle
	* we add a additional node t_hat and an addtional edge e=(t, t_hat)
	* e will never been crossed. we use e as the ext. face handle
	*/
	t_hat = this->newNode();
	adjEntry adjSource = getAdjEntry(m_Gamma, t, m_Gamma.externalFace());
	extFaceHandle = m_Gamma.addEdgeToIsolatedNode(adjSource, t_hat)->adjTarget();
	m_isSinkArc[extFaceHandle->theEdge()] = true; // not really a sink arc !! TODO??

	m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));

	//for int. faces
	while (!list.empty()) {
		Tuple2<adjEntry, adjEntry> pair = list.popFrontRet();

		edge e_new = nullptr;
		if (pair.x2()->theNode()->degree() == 0 ) {
			e_new = m_Gamma.addEdgeToIsolatedNode(pair.x1(), pair.x2()->theNode());
		}
		else {
			adjEntry adjTgt = getAdjEntry(m_Gamma, pair.x2()->theNode(), m_Gamma.rightFace(pair.x1()));
			if(!m_Gamma.getGraph().searchEdge(pair.x1()->theNode(),adjTgt->theNode())) // post-hoi-ming bugfix: prohibit the same edge twice...
				e_new = m_Gamma.splitFace(pair.x1(), adjTgt);
		}
		if(e_new!=nullptr)
			m_isSinkArc[e_new] = true;
	}

	isAugmented = true;

	OGDF_ASSERT(isSimple(*this));

	computeSinkSwitches();
}


void UpwardPlanRep::removeSinkArcs(SList<adjEntry> &crossedEdges) {

	if (crossedEdges.size() == 2)
		return;


	SListIterator<adjEntry> itPred = crossedEdges.begin(), it;
	for(it = itPred.succ(); it.valid() && it.succ().valid(); ++it)	{
		adjEntry adj = *it;
		if (m_isSinkArc[adj->theEdge()]) {
			m_Gamma.joinFaces(adj->theEdge());
			crossedEdges.delSucc(itPred);
			it = itPred;
			continue;
		}
		itPred = it;
	}
	m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));
}


void UpwardPlanRep::insertEdgePathEmbedded(edge eOrig, SList<adjEntry> crossedEdges, EdgeArray<int> &costOrig)
{
	removeSinkArcs(crossedEdges);

	//case the copy v of eOrig->source() is a sink switch
	//we muss remove the sink arcs incident to v, since after inserting eOrig, v is not a sink witch
	node v =  crossedEdges.front()->theNode();
	List<edge> outEdges;
	if (v->outdeg() == 1)
		v->outEdges(outEdges); // we delete these edges later

	m_eCopy[eOrig].clear();

	adjEntry adjSrc, adjTgt;
	SListConstIterator<adjEntry> it = crossedEdges.begin();

	// iterate over all adjacency entries in crossedEdges except for first
	// and last
	adjSrc = *it;
	List<adjEntry> dirtyList; // left and right face of the element of this list are modified
	for(++it; it.valid() && it.succ().valid(); ++it)
	{
		adjEntry adj = *it;

		bool isASourceArc = false, isASinkArc = false;
		if (m_isSinkArc[adj->theEdge()])
			isASinkArc = true;
		if (m_isSourceArc[adj->theEdge()])
			isASourceArc = true;

		int c = 0;
		if (original(adj->theEdge()) != nullptr)
			c = costOrig[original(adj->theEdge())];

		// split edge
		node u = m_Gamma.split(adj->theEdge())->source();
		if (!m_isSinkArc[adj->theEdge()] && !m_isSourceArc[adj->theEdge()])
			crossings = crossings + c; // crossing sink/source arcs cost nothing

		// determine target adjacency entry and source adjacency entry
		// in the next iteration step
		adjTgt = u->firstAdj();
		adjEntry adjSrcNext = adjTgt->succ();

		if (adjTgt != adj->twin())
			std::swap(adjTgt, adjSrcNext);

		edge e_split = adjTgt->theEdge(); // the new split edge
		if (e_split->source() != u)
			e_split = adjSrcNext->theEdge();

		if (isASinkArc)
			m_isSinkArc[e_split] = true;
		if (isASourceArc)
			m_isSourceArc[e_split] = true;

		// insert a new edge into the face
		edge eNew = m_Gamma.splitFace(adjSrc,adjTgt);
		m_eIterator[eNew] = GraphCopy::m_eCopy[eOrig].pushBack(eNew);
		m_eOrig[eNew] = eOrig;
		dirtyList.pushBack(eNew->adjSource());

		adjSrc = adjSrcNext;
	}

	// insert last edge
	edge eNew = m_Gamma.splitFace(adjSrc,*it);
	m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
	m_eOrig[eNew] = eOrig;
	dirtyList.pushBack(eNew->adjSource());

	// remove the sink arc incident to v
	if(!outEdges.empty()) {
		edge e = outEdges.popFrontRet();
		if (m_isSinkArc[e])
			m_Gamma.joinFaces(e);
	}

	m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));

	//computeSinkSwitches();
	FaceSinkGraph fsg(m_Gamma, s_hat);
	List<adjEntry> dummyList;
	FaceArray< List<adjEntry> > sinkSwitches(m_Gamma, dummyList);
	fsg.sinkSwitches(sinkSwitches);

	//construct sinkArc for the dirty faces
	for(adjEntry adj : dirtyList) {
		face fLeft = m_Gamma.leftFace(adj);
		face fRight = m_Gamma.rightFace(adj);
		List<adjEntry> switches = sinkSwitches[fLeft];

		OGDF_ASSERT(!switches.empty());

		constructSinkArcs(fLeft, switches.front()->theNode());

		OGDF_ASSERT(!switches.empty());

		switches = sinkSwitches[fRight];
		constructSinkArcs(fRight, switches.front()->theNode());
	}

	m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));
	computeSinkSwitches();
}



void UpwardPlanRep::constructSinkArcs(face f, node t)
{
	List<adjEntry> srcList;

	if (f != m_Gamma.externalFace()) {
		for(adjEntry adj : f->entries) {
			node v = adj->theNode();
			if (v == adj->theEdge()->target()
			 && v == adj->faceCyclePred()->theEdge()->target()
			 && v != t) {
				srcList.pushBack(adj);
				// XXX: where is adjTgt used later?
				// do we have to set adjTgt = adj (top-sink-switch of f) if v != t?
			}
		}
		// contruct the sink arcs
		while(!srcList.empty()) {
			adjEntry adjSrc = srcList.popFrontRet();
			edge eNew;
			if (t->degree() == 0)
				eNew = m_Gamma.addEdgeToIsolatedNode(adjSrc, t);
			else {
				adjEntry adjTgt = getAdjEntry(m_Gamma, t, m_Gamma.rightFace(adjSrc));
				eNew = m_Gamma.splitFace(adjSrc, adjTgt);
			}
			m_isSinkArc[eNew] = true;
		}
	}
	else {
		for(adjEntry adj : f->entries) {
			node v = adj->theNode();

			OGDF_ASSERT(s_hat != nullptr);

			if (v->outdeg() == 0 && v != t_hat)
				srcList.pushBack(adj);
		}

		// contruct the sink arcs
		while(!srcList.empty()) {
			adjEntry adjSrc = srcList.popFrontRet();
			adjEntry adjTgt;
			if (adjSrc->theNode() == adjSrc->theEdge()->source()) // on the right face part of the ext. face
				adjTgt = extFaceHandle;
			else
				adjTgt = extFaceHandle->cyclicPred(); // on the left face part

			auto eNew = m_Gamma.splitFace(adjSrc, adjTgt);
			m_isSinkArc[eNew] = true;
		}

	}
}


void UpwardPlanRep::computeSinkSwitches()
{
	OGDF_ASSERT(m_Gamma.externalFace() != nullptr);

	if (s_hat == nullptr)
		hasSingleSource(*this, s_hat);
	FaceSinkGraph fsg(m_Gamma, s_hat);
	List<adjEntry> dummyList;
	FaceArray< List<adjEntry> > sinkSwitches(m_Gamma, dummyList);
	fsg.sinkSwitches(sinkSwitches);
	m_sinkSwitchOf.init(*this, nullptr);

	for(face f : m_Gamma.faces) {
		List<adjEntry> switches = sinkSwitches[f];
		ListIterator<adjEntry> it = switches.begin();
		for (it = it.succ(); it.valid(); ++it) {
			m_sinkSwitchOf[(*it)->theNode()] = (*it);
		}
	}
}


void UpwardPlanRep::initMe()
{
	m_Gamma.init(*this);
	isAugmented = false;

	FaceSinkGraph fsg(m_Gamma, s_hat);
	SList<face> extFaces;
	fsg.possibleExternalFaces(extFaces);

	OGDF_ASSERT(!extFaces.empty());

	face f_ext = nullptr;
	for(face f : extFaces) {
		if (f_ext == nullptr)
			f_ext = f;
		else {
			if (f_ext->size() < f->size())
				f_ext = f;
		}
	}
	m_Gamma.setExternalFace(f_ext);
	for(adjEntry adj : s_hat->adjEntries) {
		if (m_Gamma.rightFace(adj) == m_Gamma.externalFace()) {
			extFaceHandle = adj;
			break;
		}
	}

	computeSinkSwitches();
}

adjEntry UpwardPlanRep::getAdjEntry(const CombinatorialEmbedding &Gamma, node v, face f) const {
	adjEntry adjFound = nullptr;
	for(adjEntry adj : v->adjEntries) {
		if (Gamma.rightFace(adj) == f) {
			adjFound = adj;
			break;
		}
	}
	OGDF_ASSERT(adjFound != nullptr);
	OGDF_ASSERT(Gamma.rightFace(adjFound) == f);

	return adjFound;
}



}
