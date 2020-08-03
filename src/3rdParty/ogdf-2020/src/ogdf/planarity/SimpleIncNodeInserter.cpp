/** \file
 * \brief Implementation of simple incremental node inserter
 *
 * \author Karsten Klein
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


//zwei Moeglichkeiten in PlanRep Elemente inkrementell zu
//bearbeiten: Elemente verstecken mit hide/activate
//oder Elemente, die nicht aktiv sind, loeschen

//Man kann noch spezialfaelle abfragen, zb braucht man nicht
//viel zu machen, wenn der Grad im insertionface 1 ist

#include <ogdf/planarity/SimpleIncNodeInserter.h>
#include <ogdf/basic/Queue.h>

namespace ogdf {

SimpleIncNodeInserter::SimpleIncNodeInserter(PlanRepInc &PG)
: IncNodeInserter(PG), m_incidentEdges(PG, nullptr), m_forbidCrossings(true)
{

}

SimpleIncNodeInserter::~SimpleIncNodeInserter()
{
	for(node v : m_planRep->nodes)
	{
		delete m_incidentEdges[v];
	}
}

//insert a copy for original node v
void SimpleIncNodeInserter::insertCopyNode(node v, Graph::NodeType vTyp)
{
	OGDF_ASSERT(m_planRep->copy(v) == nullptr);

	//insert a new node copy
	node vCopy = m_planRep->newCopy(v, vTyp);
	if (v->degree() == 0) return;
	//insert all adjacent edges to already inserted nodes
	adjEntry adjOrig = v->firstAdj();
	do
	{
		node wOrig = adjOrig->twinNode();
		node wCopy = m_planRep->copy(wOrig);
		edge e = adjOrig->theEdge();
		if (wCopy && (m_planRep->chain(e).size() == 0))
		{
			//inserted edge copy
			//edge eCopy;
			//newCopy can cope with zero value for adjEntry
			if (v == e->source())
				/* eCopy = */ m_planRep->newCopy(vCopy, wCopy->firstAdj(), e);
			else
				/* eCopy = */ m_planRep->newCopy(wCopy, vCopy->firstAdj(), e);

			//TODO: update component number in planrepinc
		}
		adjOrig = adjOrig->cyclicSucc();
	} while (adjOrig != v->firstAdj());
}

//insert a copy for original node v respecting the given
//embedding, i.e. inserting crossings at adjacent edges
//if necessary
//has currently running time of m*n because faces in the embedding
//are recomputed, speed up by updating them manually
void SimpleIncNodeInserter::insertCopyNode(
	node v,
	CombinatorialEmbedding &E,
	Graph::NodeType vTyp)
{
	m_nodeOf.init(E, nullptr);
	m_insertFaceNode.init(*m_planRep, false);
	m_vAdjNodes.init(*m_planRep, false);
	m_incidentEdges.init(*m_planRep, nullptr);

	m_primalAdj.init(m_dual);
	m_primalIsGen.init(m_dual, false);

	//first identify a face to insert the node into
	face f = nullptr;
	if (m_planRep->numberOfEdges() > 0)
	{
		f = getInsertionFace(v, E);//, insertAfterAdj);
		OGDF_ASSERT(f != nullptr);
	}

	//ListIterator<adjEntry> itAfter = insertAfterAdj.begin();
	//we insert v into f creating crossings for edges leading
	//to nodes outside of f (if necessary)
	node vCopy = m_planRep->newCopy(v, vTyp);

	//TODO: insert nodetypes here

	//after having selected a face for insertion, we insert
	//the crossing free edges
	adjEntry adExternal = nullptr;
	if ((f != nullptr) && (f == E.externalFace()))
	{
		//stop if only selfloops
		int count = 0;
		int eNum = max(10, m_planRep->numberOfEdges()+1);
		adExternal = E.externalFace()->firstAdj();
		while ( (adExternal->theNode() == adExternal->twinNode()) &&
			(count < eNum) )
		{
			adExternal = adExternal->faceCycleSucc();
			count++;
		}
		OGDF_ASSERT(count < eNum);
	}
	insertFaceEdges(v, vCopy, f, E, adExternal);
	//now the edges left are:
	//edges to nodes with not yet existing copies
	//edges to nodes outside the face f

	E.computeFaces();//should we manually update the face?

	if (adExternal)
	{
		E.setExternalFace(E.rightFace(adExternal));
	}

	//then we insert the edges leading to nodes outside
	//face f
#if 0
	if (v->degree() != vCopy->degree())
#endif
	insertCrossingEdges(v, vCopy, E, adExternal);

	//TODO: remove reinsert for cross edges? Not necessary
}

//protected members

//simple strategy: look for the face with the most nodes adjacent
//to v (in original) on its border
face SimpleIncNodeInserter::getInsertionFace(node v, CombinatorialEmbedding &E)
#if 0
	List<adjEntry> &insertAfterAdj)
#endif
{
	//we always return a face if one exists
	if (v->degree() < 1) return E.maximalFace();

	face bestFace = E.firstFace();

	//E is on m_planRep, the copy, v is an original node

	//construct the dual graph
#if 0
	Graph GDual;
	EdgeArray<int> flow(GDual, 0);
#endif

	FaceArray<int> numAdj(E, 0);

	//we iterate over all adjacent edges and over all
	//touching faces on the opposite nodes
	//in case of a tie, return the larger face
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		node wCopy = m_planRep->copy(e->opposite(v));
		if (wCopy == nullptr) continue; //not yet inserted

		m_vAdjNodes[wCopy] = true;
		if (m_incidentEdges[wCopy] == nullptr)
			m_incidentEdges[wCopy] = new List<edge>;
		m_incidentEdges[wCopy]->pushBack(e);
		OGDF_ASSERT(m_planRep->chain(e).size() == 0);
		for(adjEntry adRun : wCopy->adjEntries)
		{
			face f = E.rightFace(adRun);
			numAdj[f]++;

			if (numAdj[f] > numAdj[bestFace])
				bestFace = f;
			if ( (numAdj[f] == numAdj[bestFace]) &&
				((f->size() > bestFace->size()) || (f == E.externalFace())))
				bestFace = f;
		}
	}

	return bestFace;
}

void SimpleIncNodeInserter::updateComponentNumber(node vCopy, node wCopy, CombinatorialEmbedding &E, adjEntry adExternal)
{
	if (m_planRep->componentNumber(vCopy) == -1) {
		m_planRep->componentNumber(vCopy) = m_planRep->componentNumber(wCopy);
	} else
	if (m_planRep->componentNumber(vCopy) != m_planRep->componentNumber(wCopy)) {
		// we have to check the external face
		edge tEdge = m_planRep->treeEdge(m_planRep->componentNumber(vCopy),
		                                 m_planRep->componentNumber(wCopy));
		if (tEdge != nullptr
		 && (tEdge->adjSource() == adExternal
		  || tEdge->adjTarget() == adExternal)) {
			//can never be the only edge at this node, cyclic is safe
			if (tEdge->adjSource() == adExternal) {
				adExternal = tEdge->adjSource()->twin()->cyclicPred();
			} else {
				adExternal = tEdge->adjTarget()->cyclicSucc()->twin();
			}
		}
		m_planRep->deleteTreeConnection(m_planRep->componentNumber(vCopy),
						m_planRep->componentNumber(wCopy), E);
	}
}

//insert copy for edges incident to v into face f, making vCopy a node
//in this face (which means we split this face on vCopy)
void SimpleIncNodeInserter::insertFaceEdges(node v, node vCopy, face f,
											CombinatorialEmbedding &E,
											adjEntry &adExternal)
{
	//it is easy to insert the edges in f by running around the border
	//of f and process the edges in the m_incidentEdges list,
	//this keeps the embedding, but we do not want to check new inserted edges
	//we have to take care of the external face
#if 0
	NodeArray<bool> processed(*m_planRep, false);
#endif

	//if we have no face given, we assume that we only have one node so far
	//plus the new copy
	if ((f == nullptr) && (m_planRep->numberOfNodes() == 2))
	{
		node sv = m_planRep->firstNode();
		node svOriginal = m_planRep->original(sv);

		//m_vAdjNodes is not set for this case!
		//we first insert one single edge, compute faces and then insert the
		//remaining multiple edges
		bool firstEdge = true;
		adjEntry behindAdj = nullptr;
		for(adjEntry adj : svOriginal->adjEntries) {
			edge e = adj->theEdge();
			if (e->opposite(svOriginal) == v)
			{
				if (firstEdge)
				{
					//just insert the edge
					if (e->target() == v)
						m_planRep->newCopy(sv, nullptr, e);
					else m_planRep->newCopy(vCopy, nullptr, e);

					if (m_planRep->componentNumber(vCopy) == -1)
						m_planRep->componentNumber(vCopy) = m_planRep->componentNumber(sv);

					E.computeFaces();
					firstEdge = false;
					behindAdj = sv->firstAdj();
				}
				else
				{
					//preserve embedding
					m_planRep->newCopy(vCopy, behindAdj, e, E);
					behindAdj = behindAdj->cyclicSucc();
				}
			}
		}

		//we don't have to care about the external face, just set one
		return;
	}

	List<adjEntry> faceAdj;
	adjEntry adFace = f->firstAdj();
	do
	{
		faceAdj.pushBack(adFace);
		adFace = adFace->faceCycleSucc();
	} while (adFace != f->firstAdj());
	ListIterator<adjEntry> itAd = faceAdj.begin();
	//do
	while (itAd.valid())
	{
		adFace = (*itAd);

		//external face handling
		ListIterator<adjEntry> adLast = itAd.pred();
#if 0
		if (adLast != 0)
#endif
		{
			if ( adLast.valid() && ((*adLast) == adExternal) )
			{
				adExternal = adFace;
			}
		}

		node u = adFace->theNode();
		m_insertFaceNode[u] = true;
		//process all insertion edges at original(u)
		//is this node adjacent to v?
		if (m_vAdjNodes[u])
		{
			//insert egdes one time for multiple occurences
			m_vAdjNodes[u] = false;
			//save external face
#if 0
			adEntry adExt = 0;
			if (adFace == adExternal)
#endif
			OGDF_ASSERT(m_incidentEdges[u] != nullptr);
			ListIterator<edge> it = m_incidentEdges[u]->begin();
			OGDF_ASSERT(m_planRep->chain((*it)).size() == 0);
			while (it.valid())
			{
				//pushes edge onto the adjacency list of vCopy
				//correct planar embedding is therefore preserved
				//checks direction of inserted edge
				m_planRep->newCopy(vCopy, adFace, (*it), E);

				updateComponentNumber(vCopy, u, E, adExternal);
				++it;
			}

		}

		++itAd;
	}
	//} while (adFace != f->firstAdj());

#if 0
	edge e;
	bool *completed = new bool[v->degree()];
#endif
}

//insert edge copies at vCopy for edges at v that cannot be
//inserted without crossings in the current embedding
void SimpleIncNodeInserter::insertCrossingEdges(node v, node vCopy,
												CombinatorialEmbedding &E,
												adjEntry &adExternal)
{
	OGDF_ASSERT(m_planRep->copy(v) == vCopy);
	//there can exist treeConnectivity edges
#if 0
	if (v->degree() == vCopy->degree()) return;
#endif
	bool processed = true;
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if (m_planRep->chain(e).size() == 0)
		{
			processed = false;
			break;
		}
	}

	if (processed) return;

	constructDual(*m_planRep, E, m_forbidCrossings);

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		//already inserted face edge, unten weglassen
		if (m_planRep->chain(e).size() != 0)
			continue;
		node wCopy = m_planRep->copy(e->opposite(v));
		//the node copy has to exist already
		if (wCopy == nullptr) continue;
		OGDF_ASSERT(wCopy != nullptr);

		node vs = vCopy, vt = wCopy;
		if (v == e->target())
		{
			vs = wCopy;
			vt = vCopy;
		}

		SList<adjEntry> crossedEdges;
		//we already inserted a copy for edge e
		if (m_planRep->chain(e).size() != 0)
			continue;
		findShortestPath(E, vs, vt, m_planRep->typeOrig(e), crossedEdges);

		insertEdge(E, e, crossedEdges, m_forbidCrossings);
		updateComponentNumber(vCopy, wCopy, E, adExternal);
	}

}

void SimpleIncNodeInserter::constructDual(const Graph &G,
	const CombinatorialEmbedding &E,
	//node vCopy,
	bool forbidCrossings)
{
	m_dual.clear();

	// insert a node in the dual graph for each face in E
	for(face f : E.faces)
		m_nodeOf[f] = m_dual.newNode();

	// Insert an edge into the dual graph for each adjacency entry in E.
	// The edges are directed from the left face to the right face.
	for(node v : G.nodes)
	{
		for(adjEntry adj : v->adjEntries)
		{
			node vLeft  = m_nodeOf[E.leftFace (adj)];
			node vRight = m_nodeOf[E.rightFace(adj)];

			edge e = m_dual.newEdge(vLeft,vRight);
			m_primalAdj[e] = adj;

			// mark dual edges corresponding to generalizations
			if (forbidCrossings
			 && m_planRep->typeOf(adj->theEdge()) == Graph::EdgeType::generalization) {
				m_primalIsGen[e] = true;
			}
		}
	}

	// Augment the dual graph by two new vertices. These are used temporarily
	// when searching for a shortest path in the dual graph.
	m_vS = m_dual.newNode();
	m_vT = m_dual.newNode();
}

// finds a shortest path in the dual graph augmented by s and t (represented
// by m_vS and m_vT); returns list of crossed adjacency entries (corresponding
// to used edges in the dual) in crossed.
// Used to insert edges with crossings adjacent to insertion node v
void SimpleIncNodeInserter::findShortestPath(
	const CombinatorialEmbedding &E,
	node s,
	node t,
	Graph::EdgeType eType,
	SList<adjEntry> &crossed)
{
	OGDF_ASSERT(s != t);
#if 0
	if (!isConnected(m_dual)) std::cout<<"Not connected\n"<<std::flush;
#endif

	NodeArray<edge> spPred(m_dual,nullptr);
	QueuePure<edge> queue;
	int oldIdCount = m_dual.maxEdgeIndex();

	// augment dual by edges from s to all adjacent faces of s ...
	for(adjEntry adj : s->adjEntries)
	{
		// starting edges of bfs-search are all edges leaving s
		edge eDual = m_dual.newEdge(m_vS, m_nodeOf[E.rightFace(adj)]);
		m_primalAdj[eDual] = adj;
		queue.append(eDual);
	}

	// ... and from all adjacent faces of t to t
	for(adjEntry adj : t->adjEntries) {
		edge eDual = m_dual.newEdge(m_nodeOf[E.rightFace(adj)], m_vT);
		m_primalAdj[eDual] = adj;
	}

	// actual search (using bfs on directed dual)
	for( ; ; )
	{
		// next candidate edge
		edge eCand = queue.pop();
		node v = eCand->target();

		// leads to an unvisited node?
		if (spPred[v] == nullptr)
		{
			// yes, then we set v's predecessor in search tree
			spPred[v] = eCand;

			// have we reached t ...
			if (v == m_vT)
			{
				// ... then search is done.
				// constructed list of used edges (translated to crossed
				// adjacency entries in PG) from t back to s (including first
				// and last!)

				do {
					edge eDual = spPred[v];
					crossed.pushFront(m_primalAdj[eDual]);
					v = eDual->source();
				} while(v != m_vS);

				break;
			}

			// append next candidate edges to queue
			// (all edges leaving v)
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();
				if (v == e->source() && (eType != Graph::EdgeType::generalization || !m_primalIsGen[e])) {
					queue.append(e);
				}
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
}

// inserts edge e according to insertion path crossed.
// updates embedding and dual graph
void SimpleIncNodeInserter::insertEdge(
	CombinatorialEmbedding &E,
	edge eOrig,
	const SList<adjEntry> &crossed,
	bool forbidCrossingGens)
{
	// remove dual nodes on insertion path
	SListConstIterator<adjEntry> it;
	for(it = crossed.begin(); it.valid() && it.succ().valid(); ++it) {
		m_dual.delNode(m_nodeOf[E.rightFace(*it)]);
	}

	// update primal
	m_planRep->insertEdgePathEmbedded(eOrig,E,crossed);

	// insert new face nodes into dual
	const List<edge> &path = m_planRep->chain(eOrig);
	ListConstIterator<edge> itEdge;
	for(itEdge = path.begin(); itEdge.valid(); ++itEdge)
	{
		adjEntry adj = (*itEdge)->adjSource();
		m_nodeOf[E.leftFace (adj)] = m_dual.newNode();
		m_nodeOf[E.rightFace(adj)] = m_dual.newNode();
	}

	// insert new edges into dual
	for(itEdge = path.begin(); itEdge.valid(); ++itEdge)
	{
		adjEntry adjSrc = (*itEdge)->adjSource();
		face f = E.rightFace(adjSrc);  // face to the right of adj in loop
		node vRight = m_nodeOf[f];

		adjEntry adj1 = f->firstAdj(), adj = adj1;
		do {
			node vLeft = m_nodeOf[E.leftFace(adj)];

			edge eLR = m_dual.newEdge(vLeft,vRight);
			m_primalAdj[eLR] = adj;

			edge eRL = m_dual.newEdge(vRight,vLeft);
			m_primalAdj[eRL] = adj->twin();

			if(forbidCrossingGens &&
				m_planRep->typeOf(adj->theEdge()) == Graph::EdgeType::generalization)
			{
				m_primalIsGen[eLR] = m_primalIsGen[eRL] = true;
			}
		}
		while((adj = adj->faceCycleSucc()) != adj1);

		// the other face adjacent to *itEdge ...
		f = E.rightFace(adjSrc->twin());
		vRight = m_nodeOf[f];

		adj1 = f->firstAdj();
		adj = adj1;
		do {
			node vLeft = m_nodeOf[E.leftFace(adj)];

			edge eLR = m_dual.newEdge(vLeft,vRight);
			m_primalAdj[eLR] = adj;

			edge eRL = m_dual.newEdge(vRight,vLeft);
			m_primalAdj[eRL] = adj->twin();

			if(forbidCrossingGens &&
				m_planRep->typeOf(adj->theEdge()) == Graph::EdgeType::generalization)
			{
				m_primalIsGen[eLR] = m_primalIsGen[eRL] = true;
			}
		}
		while((adj = adj->faceCycleSucc()) != adj1);
	}
}

}
