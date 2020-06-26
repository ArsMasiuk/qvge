/** \file
 * \brief Implementation of class CombinatorialEmbedding
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


#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/Math.h>
#ifdef OGDF_DEBUG
# include <ogdf/basic/simple_graph_alg.h>
#endif

using std::mutex;
using std::lock_guard;


#define MIN_FACE_TABLE_SIZE (1 << 4)


namespace ogdf {

ConstCombinatorialEmbedding::ConstCombinatorialEmbedding()
{
	m_cpGraph = nullptr;
	m_externalFace = nullptr;
	m_faceIdCount = 0;
	m_faceArrayTableSize = MIN_FACE_TABLE_SIZE;
}


ConstCombinatorialEmbedding::ConstCombinatorialEmbedding(const Graph &G) :
	m_cpGraph(&G), m_rightFace(G,nullptr)
{
	OGDF_ASSERT(isConnected(G));
	OGDF_ASSERT(G.representsCombEmbedding());
	computeFaces();
}

ConstCombinatorialEmbedding::ConstCombinatorialEmbedding(
	const ConstCombinatorialEmbedding &C)
	: m_cpGraph(C.m_cpGraph), m_rightFace(*C.m_cpGraph,nullptr)
{
	computeFaces();

	if(C.m_externalFace == nullptr)
		m_externalFace = nullptr;
	else
		m_externalFace = m_rightFace[C.m_externalFace->firstAdj()];
}

ConstCombinatorialEmbedding &ConstCombinatorialEmbedding::operator=(
	const ConstCombinatorialEmbedding &C)
{
	init(*C.m_cpGraph);

	if(C.m_externalFace == nullptr)
		m_externalFace = nullptr;
	else
		m_externalFace = m_rightFace[C.m_externalFace->firstAdj()];

	return *this;
}

ConstCombinatorialEmbedding::~ConstCombinatorialEmbedding() {
	faces.clear();
}

void ConstCombinatorialEmbedding::init(const Graph &G)
{
	OGDF_ASSERT(isConnected(G));
	OGDF_ASSERT(G.representsCombEmbedding());
	m_cpGraph = &G;
	m_rightFace.init(G,nullptr);
	computeFaces();
}


void ConstCombinatorialEmbedding::init()
{
	m_cpGraph = nullptr;
	m_externalFace = nullptr;
	m_faceIdCount = 0;
	m_faceArrayTableSize = MIN_FACE_TABLE_SIZE;
	m_rightFace.init();
	faces.clear();

	reinitArrays();
}


void ConstCombinatorialEmbedding::computeFaces()
{
	m_externalFace = nullptr; // no longer valid!
	m_faceIdCount = 0;
	faces.clear();

	m_rightFace.fill(nullptr);

	for(node v : m_cpGraph->nodes) {
		for(adjEntry adj : v->adjEntries) {
			if (m_rightFace[adj]) continue;

#ifdef OGDF_DEBUG
			face f = new FaceElement(this,adj,m_faceIdCount++);
#else
			face f = new FaceElement(adj,m_faceIdCount++);
#endif

			faces.pushBack(f);

			adjEntry adj2 = adj;
			do {
				m_rightFace[adj2] = f;
				f->m_size++;
				adj2 = adj2->faceCycleSucc();
			} while (adj2 != adj);
		}
	}

	m_faceArrayTableSize = Math::nextPower2(MIN_FACE_TABLE_SIZE, m_faceIdCount + 1);
	reinitArrays();

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


face ConstCombinatorialEmbedding::createFaceElement(adjEntry adjFirst)
{
	if (m_faceIdCount == m_faceArrayTableSize) {
		m_faceArrayTableSize <<= 1;
		for(FaceArrayBase *fab : m_regFaceArrays)
			fab->enlargeTable(m_faceArrayTableSize);
	}

#ifdef OGDF_DEBUG
	face f = new FaceElement(this,adjFirst,m_faceIdCount++);
#else
	face f = new FaceElement(adjFirst,m_faceIdCount++);
#endif

	faces.pushBack(f);

	return f;
}


edge CombinatorialEmbedding::split(edge e)
{
	face f1 = m_rightFace[e->adjSource()];
	face f2 = m_rightFace[e->adjTarget()];

	edge e2 = m_pGraph->split(e);

	m_rightFace[e->adjSource()] = m_rightFace[e2->adjSource()] = f1;
	f1->m_size++;
	m_rightFace[e->adjTarget()] = m_rightFace[e2->adjTarget()] = f2;
	f2->m_size++;

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif

	return e2;
}


void CombinatorialEmbedding::unsplit(edge eIn, edge eOut)
{
	face f1 = m_rightFace[eIn->adjSource()];
	face f2 = m_rightFace[eIn->adjTarget()];

	--f1->m_size;
	--f2->m_size;

	if (f1->entries.m_adjFirst == eOut->adjSource())
		f1->entries.m_adjFirst = eIn->adjSource();

	if (f2->entries.m_adjFirst == eIn->adjTarget())
		f2->entries.m_adjFirst = eOut->adjTarget();

	m_pGraph->unsplit(eIn,eOut);
}


node CombinatorialEmbedding::splitNode(adjEntry adjStartLeft, adjEntry adjStartRight)
{
	face fL = leftFace(adjStartLeft);
	face fR = leftFace(adjStartRight);

	node u = m_pGraph->splitNode(adjStartLeft,adjStartRight);

	adjEntry adj = adjStartLeft->cyclicPred();

	m_rightFace[adj] = fL;
	++fL->m_size;
	m_rightFace[adj->twin()] = fR;
	++fR->m_size;

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif

	return u;
}


node CombinatorialEmbedding::contract(edge e)
{
	// Since we remove edge e, we also remove adjSrc and adjTgt.
	// We make sure that node of them is stored as first adjacency
	// entry of a face.
	adjEntry adjSrc = e->adjSource();
	adjEntry adjTgt = e->adjTarget();

	face fSrc = m_rightFace[adjSrc];
	face fTgt = m_rightFace[adjTgt];

	if (fSrc->entries.m_adjFirst == adjSrc) {
		adjEntry adj = adjSrc->faceCycleSucc();
		fSrc->entries.m_adjFirst = (adj != adjTgt) ? adj : adj->faceCycleSucc();
	}

	if (fTgt->entries.m_adjFirst == adjTgt) {
		adjEntry adj = adjTgt->faceCycleSucc();
		fTgt->entries.m_adjFirst = (adj != adjSrc) ? adj : adj->faceCycleSucc();
	}

	node v = m_pGraph->contract(e);
	--fSrc->m_size;
	--fTgt->m_size;

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif

	return v;
}


edge CombinatorialEmbedding::splitFace(adjEntry adjSrc, adjEntry adjTgt)
{
	OGDF_ASSERT(m_rightFace[adjSrc] == m_rightFace[adjTgt]);
	OGDF_ASSERT(adjSrc != adjTgt);

	edge e = m_pGraph->newEdge(adjSrc,adjTgt);

	face f1 = m_rightFace[adjTgt];
	face f2 = createFaceElement(adjSrc);

	adjEntry adj = adjSrc;
	do {
		m_rightFace[adj] = f2;
		f2->m_size++;
		adj = adj->faceCycleSucc();
	} while (adj != adjSrc);

	f1->entries.m_adjFirst = adjTgt;
	f1->m_size += (2 - f2->m_size);
	m_rightFace[e->adjSource()] = f1;

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif

	return e;
}

edge CombinatorialEmbedding::addEdgeToIsolatedNode(node v, adjEntry adjTgt)
{
	return addEdgeToIsolatedNode(adjTgt, v, false);
}

edge CombinatorialEmbedding::addEdgeToIsolatedNode(adjEntry adjSrc, node v)
{
	return addEdgeToIsolatedNode(adjSrc, v, true);
}

edge CombinatorialEmbedding::addEdgeToIsolatedNode(adjEntry adj, node v, bool adjSrc)
{
	OGDF_ASSERT(v->degree() == 0);

	edge e = adjSrc ? m_pGraph->newEdge(adj, v) : m_pGraph->newEdge(v, adj);
	face f = m_rightFace[adj];
	m_rightFace[e->adjSource()] = f;
	f->m_size += 2;
	m_rightFace[e->adjTarget()] = f;

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif

	return e;
}

//update face information after inserting a merger ith edge e in a copy graph
void CombinatorialEmbedding::updateMerger(edge e, face fRight, face fLeft)
{
	//two cases: a single face/two faces
	fRight->m_size++;
	fLeft->m_size++;
	m_rightFace[e->adjSource()] = fRight;
	m_rightFace[e->adjTarget()] = fLeft;
	//check for first adjacency entry
	if (fRight != fLeft)
	{
		fRight->entries.m_adjFirst = e->adjSource();
		fLeft->entries.m_adjFirst = e->adjTarget();
	}
}

face CombinatorialEmbedding::joinFaces(edge e)
{
	face f = joinFacesPure(e);
	m_pGraph->delEdge(e);

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif

	return f;
}

face CombinatorialEmbedding::joinFacesPure(edge e)
{
	OGDF_ASSERT(e->graphOf() == m_pGraph);

	// get the two faces adjacent to e
	face f1 = m_rightFace[e->adjSource()];
	face f2 = m_rightFace[e->adjTarget()];

	OGDF_ASSERT(f1 != f2);

	// we will reuse the largest face and delete the other one
	if (f2->m_size > f1->m_size)
		std::swap(f1,f2);

	// the size of the joined face is the sum of the sizes of the two faces
	// f1 and f2 minus the two adjacency entries of e
	f1->m_size += f2->m_size - 2;

	// If the stored (first) adjacency entry of f1 belongs to e, we must set
	// it to the next entry in the face, because we will remove it by deleting
	// edge e
	if (f1->entries.m_adjFirst->theEdge() == e)
		f1->entries.m_adjFirst = f1->entries.m_adjFirst->faceCycleSucc();

	// each adjacency entry in f2 belongs now to f1
	adjEntry adj1 = f2->firstAdj(), adj = adj1;
	do {
		m_rightFace[adj] = f1;
	} while((adj = adj->faceCycleSucc()) != adj1);

	faces.del(f2);

	return f1;
}


void CombinatorialEmbedding::reverseEdge(edge e)
{
	// reverse edge in graph
	m_pGraph->reverseEdge(e);

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


void CombinatorialEmbedding::moveBridge(adjEntry adjBridge, adjEntry adjBefore)
{
	OGDF_ASSERT(m_rightFace[adjBridge] == m_rightFace[adjBridge->twin()]);
	OGDF_ASSERT(m_rightFace[adjBridge] != m_rightFace[adjBefore]);

	face fOld = m_rightFace[adjBridge];
	face fNew = m_rightFace[adjBefore];

	adjEntry adjCand = adjBridge->faceCycleSucc();

	int sz = 0;
	adjEntry adj;
	for(adj = adjBridge->twin(); adj != adjCand; adj = adj->faceCycleSucc()) {
		if (fOld->entries.m_adjFirst == adj)
			fOld->entries.m_adjFirst = adjCand;
		m_rightFace[adj] = fNew;
		++sz;
	}

	fOld->m_size -= sz;
	fNew->m_size += sz;

	edge e = adjBridge->theEdge();
	if(e->source() == adjBridge->twinNode())
		m_pGraph->moveSource(e, adjBefore, Direction::after);
	else
		m_pGraph->moveTarget(e, adjBefore, Direction::after);

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


void CombinatorialEmbedding::removeDeg1(node v)
{
	OGDF_ASSERT(v->degree() == 1);

	adjEntry adj = v->firstAdj();
	face     f   = m_rightFace[adj];

	if (f->entries.m_adjFirst == adj || f->entries.m_adjFirst == adj->twin())
		f->entries.m_adjFirst = adj->faceCycleSucc();
	f->m_size -= 2;

	m_pGraph->delNode(v);

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


void CombinatorialEmbedding::clear()
{
	m_pGraph->clear();

	faces.clear();

	m_faceIdCount = 0;
	m_faceArrayTableSize = MIN_FACE_TABLE_SIZE;
	m_externalFace = nullptr;

	reinitArrays();

#ifdef OGDF_HEAVY_DEBUG
	consistencyCheck();
#endif
}


face ConstCombinatorialEmbedding::chooseFace(std::function<bool(face)> includeFace, bool isFastTest) const
{
	return *chooseIteratorFrom<internal::GraphObjectContainer<FaceElement>, face>(
			const_cast<internal::GraphObjectContainer<FaceElement>&>(faces),
			[&](const face &f) { return includeFace(f); },
			isFastTest
	);
}


face ConstCombinatorialEmbedding::maximalFace() const
{
	if (numberOfFaces() == 0) return nullptr;

	face fMax = firstFace();
	int max = fMax->size();

	for(face f = fMax->succ(); f != nullptr; f = f->succ())
	{
		if (f->size() > max) {
			max = f->size();
			fMax = f;
		}
	}

	return fMax;
}


ListIterator<FaceArrayBase*> ConstCombinatorialEmbedding::registerArray(
	FaceArrayBase *pFaceArray) const
{
#ifndef OGDF_MEMORY_POOL_NTS
	lock_guard<mutex> guard(m_mutexRegArrays);
#endif
	return m_regFaceArrays.pushBack(pFaceArray);
}


void ConstCombinatorialEmbedding::unregisterArray(
	ListIterator<FaceArrayBase*> it) const
{
#ifndef OGDF_MEMORY_POOL_NTS
	lock_guard<mutex> guard(m_mutexRegArrays);
#endif
	m_regFaceArrays.del(it);
}


void ConstCombinatorialEmbedding::moveRegisterArray(
	ListIterator<FaceArrayBase*> it, FaceArrayBase *pFaceArray) const
{
#ifndef OGDF_MEMORY_POOL_NTS
	lock_guard<mutex> guard(m_mutexRegArrays);
#endif
	*it = pFaceArray;
}


void ConstCombinatorialEmbedding::reinitArrays()
{
	for(FaceArrayBase *fab : m_regFaceArrays)
		fab->reinit(m_faceArrayTableSize);
}

#ifdef OGDF_DEBUG
void ConstCombinatorialEmbedding::consistencyCheck() const
{
	OGDF_ASSERT(isConnected(*m_cpGraph));
	m_cpGraph->consistencyCheck();

	OGDF_ASSERT(m_cpGraph->representsCombEmbedding());

	AdjEntryArray<bool> visited(*m_cpGraph, false);
	int nF = 0;

	for(face f : faces) {
		OGDF_ASSERT(f->embeddingOf() == this);

		nF++;

		adjEntry adj = f->firstAdj(), adj2 = adj;
		int sz = 0;
		do {
			sz++;
			OGDF_ASSERT(!visited[adj2]);
			visited[adj2] = true;
			OGDF_ASSERT(m_rightFace[adj2] == f);
			adj2 = adj2->faceCycleSucc();
		} while(adj2 != adj);

		OGDF_ASSERT(f->size() == sz);
	}

	OGDF_ASSERT(nF == faces.size());

	for(node v : m_cpGraph->nodes) {
		for(adjEntry adj : v->adjEntries) {
			OGDF_ASSERT(visited[adj]);
		}
	}
}
#endif

adjEntry ConstCombinatorialEmbedding::findCommonFace(const node v, const node w, adjEntry &adjW, bool left) const {
	OGDF_ASSERT(v != w);

	for(adjEntry adjV = v->firstAdj(); adjV != nullptr; adjV = adjV->succ()) {
		face f = (left ? leftFace(adjV) : rightFace(adjV));
		for(adjW = w->firstAdj(); adjW != nullptr; adjW = adjW->succ()) {
			if(f == (left ? leftFace(adjW) : rightFace(adjW))) {
				return adjV;
			}
		}
	}
	return nullptr;
}

}
