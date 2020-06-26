/** \file
 * \brief Implementation of PlanRep base class for planar rep.
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


#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/orthogonal/OrthoRep.h>
#include <ogdf/planarity/PlanRep.h>


namespace ogdf {

PlanRep::PlanRep(const Graph& G) :
	GraphCopy(),
	m_ccInfo(G),
	m_pGraphAttributes(nullptr),
	m_boundaryAdj(G, nullptr),//*this, 0),
	m_oriEdgeTypes(G, 0),
	m_eAuxCopy(G)
{
	m_vType        .init(*this, Graph::NodeType::dummy);
	m_nodeTypes    .init(*this, 0); //the new node type info
	m_expandedNode .init(*this, nullptr);
	m_expandAdj    .init(*this, nullptr);
	m_expansionEdge.init(*this, 0);
	m_eType        .init(*this, Graph::EdgeType::association); //should be dummy  or standard, but isnt checked correctly,
	m_edgeTypes    .init(*this, 0); //the new edge type info

	// special way of initializing GraphCopy; we start with an empty copy
	// and add components by need
	GraphCopy::createEmpty(G);

	m_currentCC = -1;  // not yet initialized
}


PlanRep::PlanRep(const GraphAttributes& AG) :
	GraphCopy(),
	m_ccInfo(AG.constGraph()),
	m_pGraphAttributes(&AG),
	m_boundaryAdj(AG.constGraph(), nullptr),
	m_oriEdgeTypes(AG.constGraph(), 0),
	m_eAuxCopy(AG.constGraph())
{
	OGDF_ASSERT(m_pGraphAttributes->has(GraphAttributes::edgeType));
	OGDF_ASSERT(m_pGraphAttributes->has(GraphAttributes::nodeGraphics));

	m_vType        .init(*this,Graph::NodeType::dummy);
	m_nodeTypes    .init(*this, 0); //the new node type info
	m_expandedNode .init(*this,nullptr);
	m_expandAdj    .init(*this,nullptr);
	m_expansionEdge.init(*this, 0);
	m_eType        .init(*this, Graph::EdgeType::association); //should be dummy  or standard, but isnt checked correctly,
	m_edgeTypes    .init(*this, 0); //the new edge type info

	const Graph &G = AG.constGraph();

	// special way of initializing GraphCopy; we start with an empty copy
	// and add components by need
	GraphCopy::createEmpty(G);

	m_currentCC = -1;  // not yet initialized
}

void PlanRep::initCC(int cc)
{
	// delete copy / chain fields for originals of nodes in current cc
	// (since we remove all these copies in initByNodes(...)
	if (m_currentCC >= 0)
	{
		for(int i = m_ccInfo.startNode(m_currentCC); i < m_ccInfo.stopNode(m_currentCC); ++i)
			m_vCopy[m_ccInfo.v(i)] = nullptr;

		for(int i = m_ccInfo.startEdge(m_currentCC); i < m_ccInfo.stopEdge(m_currentCC); ++i)
			m_eCopy[m_ccInfo.e(i)].clear();
	}

	m_currentCC = cc;
	GraphCopy::initByCC(m_ccInfo, cc, m_eAuxCopy);

	// set type of edges (gen. or assoc.) in the current CC
	for(edge e : edges)
		setCopyType(e, original(e));

	if(m_pGraphAttributes == nullptr)
		return;

	// The following part is only relevant with given graph attributes!

	for(node v : nodes)
	{
		m_vType[v] = m_pGraphAttributes->type(original(v));
		if (m_pGraphAttributes->isAssociationClass(original(v))) {
			OGDF_ASSERT(v->degree() == 1);
			edge e = v->firstAdj()->theEdge();
			setAssClass(e);
		}
	}
}

//inserts Boundary for a given group of nodes
//can e.g. be used to split clique replacements from the rest of the graph
//precondition: is only applicable for planar embedded subgraphs
//
//void PlanRep::insertBoundary(List<node> &group)
//{
//TODO
//Difficulty: efficiently find the outgoing edges
//}
//special version for stars
//works on copy nodes
//precondition: center is the center of a star-like subgraph
//induced by the neighbours of center, subgraph has connection
//to the rest of the graph
//keeps the given embedding
void PlanRep::insertBoundary(node centerOrig, adjEntry& adjExternal)//, CombinatorialEmbedding &E)
{
	//we insert edges to represent the boundary
	//by splitting the outgoing edges and connecting the
	//split nodes
	node center = copy(centerOrig);
	OGDF_ASSERT(center != nullptr);

	if (center->degree() < 1) return;

	OGDF_ASSERT(original(center));

	//retrieve the outgoing edges
	//we run over all nodes adjacent to center and add their
	//adjacent edges
	SList<adjEntry> outAdj;

	for(adjEntry adj : center->adjEntries)
	{
		//check if external face was saved over adjEntry on center edges

		//we want to stay in the same (external) face, next adjEntry
		//may get split later on, succ(succ) can never be within this clique
		//(and split) because all clique node - clique node connections are deleted
		//IFF the target node is connnected to some non-clique part of the graph
		//the search in this case would loop if there is no connection to the rest of
		//the graph
		if (adjExternal == adj) //outgoing
		{
			if (adj->twinNode()->degree() == 1)
			{
				do {
					adjExternal = adjExternal->faceCycleSucc();
				} while ( (adjExternal->theNode() == center) ||
					(adjExternal->twinNode() == center));
			} else {
				adjExternal = adjExternal->faceCycleSucc()->faceCycleSucc();
			}
		}
		if (adjExternal == adj->twin()) { // incoming
			if (adj->twinNode()->degree() == 1)
			{
				do {
					adjExternal = adjExternal->faceCycleSucc();
				} while ( (adjExternal->theNode() == center) ||
					(adjExternal->twinNode() == center));
			} else {
				adjExternal = adjExternal->faceCyclePred()->faceCyclePred();
			}
		}
		adjEntry stopper = adj->twin();
		adjEntry runner = stopper->cyclicSucc();
		while (runner != stopper)
		{
			outAdj.pushBack(runner);
			runner = runner->cyclicSucc();
		}
	}

	//we do not insert a boundary if the subgraph is not
	//connected to the rest of the graph
	if (outAdj.empty()) return;

	//now split the edges and save adjEntries
	//we maintain two lists of adjentries
	List<adjEntry> targetEntries, sourceEntries;

	//outgoing edges of clique nodes
	for (adjEntry splitAdj : outAdj)
	{
		edge splitEdge = splitAdj->theEdge();

		//we need to find out if edge is outgoing
		bool isOut = (splitAdj->theNode() == splitEdge->source());

		//check if external face was saved over edges to be split
		bool splitOuter = false;
		bool splitInner = false;
		if (adjExternal == splitAdj)
		{
			splitOuter = true;
			//adjExternal = adjExternal->faceCycleSucc();
		}
		if (adjExternal == splitAdj->twin())
		{
			splitInner = true;
			//adjExternal = adjExternal->faceCyclePred();
		}


		//combinatorial version
		//edge newEdge = E.split(splitEdge);
		//simple version
		edge newEdge = split(splitEdge);
		setCrossingType(newEdge->source());

		//store the adjEntries depending on in/out direction
		if (isOut)
		{
			//splitresults "upper" edge to old target node is newEdge!
			sourceEntries.pushBack(newEdge->adjSource());
			targetEntries.pushBack(splitEdge->adjTarget());
			if (splitOuter) adjExternal = newEdge->adjSource();
			if (splitInner) adjExternal = newEdge->adjTarget();
		} else {
			sourceEntries.pushBack(splitEdge->adjTarget());
			targetEntries.pushBack(newEdge->adjSource());
			if (splitOuter) adjExternal = splitEdge->adjTarget();
			if (splitInner) adjExternal = splitEdge->adjSource();
		}
	}

	//we need pairs of adjEntries
	OGDF_ASSERT(targetEntries.size() == sourceEntries.size());
	//now flip first target entry to front
	//should be nonempty
	adjEntry flipper = targetEntries.popFrontRet();
	targetEntries.pushBack(flipper);

	edge e;
	OGDF_ASSERT( !targetEntries.empty() ); // otherwise e is not well defined (inside the loop)
	//connect the new nodes to form the boundary
	while (!targetEntries.empty())
	{
		//combinatorial version
		//edge e = E.splitFace(sourceEntries.popFrontRet(), targetEntries.popFrontRet());
		//simple version
		e = newEdge(sourceEntries.popFrontRet(), targetEntries.popFrontRet());
		this->typeOf(e) = Graph::EdgeType::association;
		setCliqueBoundary(e);
	}

	//keep it simple: just assign the last adjEntry to boundaryAdj
	//we have to save at the original, the copy may be replaced
	OGDF_ASSERT(m_boundaryAdj[original(center)] == nullptr);
	m_boundaryAdj[original(center)] = e->adjSource();
}

#if 0
void PlanRep::removePseudoCrossings()
{
	node v, vSucc;
	for(v = firstNode(); v != 0; v = vSucc)
	{
		vSucc = v->succ();

		if (typeOf(v) != PlanRep::dummy || v->degree() != 4)
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
}
#endif

void PlanRep::insertEdgePathEmbedded(
	edge eOrig,
	CombinatorialEmbedding &E,
	const SList<adjEntry> &crossedEdges)
{
	GraphCopy::insertEdgePathEmbedded(eOrig,E,crossedEdges);
	Graph::EdgeType type = m_pGraphAttributes ? m_pGraphAttributes->type(eOrig) : Graph::EdgeType::association;

	long et = m_oriEdgeTypes[eOrig];

	for(edge e : chain(eOrig))
	{
		m_eType[e] = type;
		m_edgeTypes[e] = et;
		if (!original(e->target()))
		{
			OGDF_ASSERT(e->target()->degree() == 4);
			setCrossingType(e->target());
		}
	}
}

void PlanRep::insertEdgePath(
	edge eOrig,
	const SList<adjEntry> &crossedEdges)
{
	GraphCopy::insertEdgePath(eOrig,crossedEdges);

	//old types
	Graph::EdgeType type = m_pGraphAttributes ? m_pGraphAttributes->type(eOrig) : Graph::EdgeType::association;

	//new types
	long et = m_oriEdgeTypes[eOrig];

	for(edge e : chain(eOrig))
	{
		m_eType[e] = type;
		m_edgeTypes[e] = et;
		if (!original(e->target()))
		{
			OGDF_ASSERT(e->target()->degree() == 4);
			setCrossingType(e->target());
		}
	}
}

edge PlanRep::insertCrossing(
	edge &crossingEdge,
	edge crossedEdge,
	bool topDown)
{
	EdgeType eTypi = m_eType[crossingEdge];
	EdgeType eTypd = m_eType[crossedEdge];
	edgeType eTypsi = m_edgeTypes[crossingEdge];
	edgeType eTypsd = m_edgeTypes[crossedEdge];

	edge newCopy = GraphCopy::insertCrossing(crossingEdge, crossedEdge, topDown);

	//Do not use original types, they may differ from the copy
	//type due to conflict resolution in preprocessing (expand crossings)
	m_eType[crossingEdge] = eTypi;
	m_eType[newCopy]      = eTypd;
	m_edgeTypes[crossingEdge] = eTypsi;
	m_edgeTypes[newCopy]      = eTypsd;

	setCrossingType(newCopy->source());
	OGDF_ASSERT(isCrossingType(newCopy->source()));

	//TODO: hier sollte man die NodeTypes setzen, d.h. crossing

	return newCopy;

}


void PlanRep::removeCrossing(node v)
{
	OGDF_ASSERT(v->degree() == 4);
	OGDF_ASSERT(isCrossingType(v));

	adjEntry a1 = v->firstAdj();
	adjEntry b1 = a1->cyclicSucc();
	adjEntry a2 = b1->cyclicSucc();
	adjEntry b2 = a2->cyclicSucc();

	removeUnnecessaryCrossing(a1, a2, b1, b2);
}

void PlanRep::expand(bool lowDegreeExpand)
{
	for(node v : nodes)
	{

		// Replace vertices with high degree by cages and
		// replace degree 4 vertices with two generalizations
		// adjacent in the embedding list by a cage.
		if ((v->degree() > 4)  && (typeOf(v) != Graph::NodeType::dummy) && !lowDegreeExpand)
		{
			//Set the type of the node v. It remains in the graph
			// as one of the nodes of the expanded face.
			typeOf(v) = Graph::NodeType::highDegreeExpander;

			// Scan the list of edges of v to find the adjacent edges of v
			// according to the planar embedding. All except one edge
			// will get a new adjacent node
			SList<edge> adjEdges;
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();
				adjEdges.pushBack(e);
			}

			//The first edge remains at v. remove it from the list.
			edge e = adjEdges.popFrontRet();

			// Create the list of high degree expanders
			// We need degree(v)-1 of them to construct a face.
			// and set expanded Node to v
			setExpandedNode(v, v);
			SListPure<node> expander;
			for (int i = 0; i < v->degree()-1; i++)
			{
				node u = newNode();
				typeOf(u) = Graph::NodeType::highDegreeExpander;
				setExpandedNode(u, v);
				expander.pushBack(u);
			}

			// We move the target node of each ingoing generalization of v to a new
			// node stored in expander.
			// Note that, for each such edge e, the target node of the original
			// edge is then different from the original of the target node of e
			// (the latter is 0 because u is a new (dummy) node)
			SListConstIterator<node> itn;

			NodeArray<adjEntry> ar(*this);

			itn = expander.begin();

			for (edge ei : adjEdges)
			{
				// Did we allocate enough dummy nodes?
				OGDF_ASSERT(itn.valid());

				if (ei->source() == v)
					moveSource(ei,*itn);
				else
					moveTarget(ei,*itn);
				ar[*itn] = (*itn)->firstAdj();
				++itn;
			}
			ar[v] = v->firstAdj();

			// Now introduce the circular list of new edges
			// forming the border of the merge face. Keep the embedding.
			adjEntry adjPrev = v->firstAdj();

//			std::cout <<std::endl << "INTRODUCING CIRCULAR EDGES" << std::endl;
			for (node n : expander)
			{
//				std::cout << adjPrev << " " << (*itn)->firstAdj() << std::endl;
				e = Graph::newEdge(adjPrev,n->firstAdj());
				setExpansionEdge(e, 2);//can be removed if edgetypes work properly

				setExpansion(e);
				setAssociation(e);

				typeOf(e) = EdgeType::association; //???

				if (!expandAdj(v))
					expandAdj(v) = e->adjSource();
				adjPrev = n->firstAdj();
			}

			e = newEdge(adjPrev,v->lastAdj());

			typeOf(e) = EdgeType::association; //???
			setExpansionEdge(e, 2);//can be removed if edgetypes work properly
			setAssociation(e);
		}

		// Replace all vertices with degree > 2 by cages.
		else if (v->degree() >= 2
		      && typeOf(v) != Graph::NodeType::dummy
		      && lowDegreeExpand) {
			//Set the type of the node v. It remains in the graph
			// as one of the nodes of the expanded face.
			typeOf(v) = Graph::NodeType::lowDegreeExpander; //high??

			// Scan the list of edges of v to find the adjacent edges of v
			// according to the planar embedding. All except one edge
			// will get a new adjacent node
			SList<edge> adjEdges;
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();
				adjEdges.pushBack(e);
			}

			//The first edge remains at v. remove it from the list.
			// Check if it is a generalization.
			edge e = adjEdges.popFrontRet();

			// Create the list of high degree expanders
			// We need degree(v)-1 of them to construct a face.
			// and set expanded Node to v
			setExpandedNode(v, v);
			SListPure<node> expander;
			for (int i = 0; i < v->degree()-1; i++)
			{
				node u = newNode();
				typeOf(u) = Graph::NodeType::highDegreeExpander;
				setExpandedNode(u, v);
				expander.pushBack(u);
			}

			// We move the target node of each ingoing generalization of v to a new
			// node stored in expander.
			// Note that, for each such edge e, the target node of the original
			// edge is then different from the original of the target node of e
			// (the latter is 0 because u is a new (dummy) node)

			NodeArray<adjEntry> ar(*this);

			SListConstIterator<node> itn = expander.begin();

			for (edge ei : adjEdges)
			{
				// Did we allocate enough dummy nodes?
				OGDF_ASSERT(itn.valid());

				if (ei->source() == v)
					moveSource(ei,*itn);
				else
					moveTarget(ei,*itn);
				ar[*itn] = (*itn)->firstAdj();
				++itn;
			}
			ar[v] = v->firstAdj();

			// Now introduce the circular list of new edges
			// forming the border of the merge face. Keep the embedding.
			adjEntry adjPrev = v->firstAdj();

			for (node n : expander)
			{
				e = newEdge(adjPrev,n->firstAdj());
				if (!expandAdj(v)) expandAdj(v) = e->adjSource();
				typeOf(e) = EdgeType::association; //???
				setExpansionEdge(e, 2);

				//new types
				setAssociation(e); //should be dummy type?
				setExpansion(e);

				adjPrev = n->firstAdj();
			}
			e = newEdge(adjPrev,v->lastAdj());
			typeOf(e) = EdgeType::association; //???
			setExpansionEdge(e, 2);
		}
	}
}

void PlanRep::expandLowDegreeVertices(OrthoRep &OR)
{
	for(node v : nodes)
	{
		if (!(isVertex(v)) || expandAdj(v) != nullptr)
			continue;

		SList<edge> adjEdges;
		SListPure<Tuple2<node,int> > expander;

		node u = v;
		bool firstTime = true;

		setExpandedNode(v, v);

		for(adjEntry adj : v->adjEntries) {
			adjEdges.pushBack(adj->theEdge());

			if(!firstTime)
				u = newNode();

			setExpandedNode(u, v);
			typeOf(u) = Graph::NodeType::lowDegreeExpander;
			expander.pushBack(Tuple2<node,int>(u,OR.angle(adj)));
			firstTime = false;
		}

		SListConstIterator<Tuple2<node,int>> itn = expander.begin().succ();

		for (SListConstIterator<edge> it = adjEdges.begin().succ(); it.valid(); ++it)
		{
			// Did we allocate enough dummy nodes?
			OGDF_ASSERT(itn.valid());

			if ((*it)->source() == v)
				moveSource(*it,(*itn).x1());
			else
				moveTarget(*it,(*itn).x1());
			++itn;
		}

		adjEntry adjPrev = v->firstAdj();
		itn = expander.begin();
		int nBends = (*itn).x2();

		for (++itn; itn.valid(); ++itn)
		{
			edge e = newEdge(adjPrev,(*itn).x1()->firstAdj());

			OR.bend(e->adjSource()).set(OrthoBendType::convexBend,nBends);
			OR.bend(e->adjTarget()).set(OrthoBendType::reflexBend,nBends);
			OR.angle(adjPrev) = 1;
			OR.angle(e->adjSource()) = 2;
			OR.angle(e->adjTarget()) = 1;

			nBends = (*itn).x2();

			typeOf(e) = EdgeType::association; //???
			setExpansionEdge(e, 2);

			adjPrev = (*itn).x1()->firstAdj();
		}

		edge e = newEdge(adjPrev,v->lastAdj());
		typeOf(e) = EdgeType::association; //???
		setExpansionEdge(e, 2);

		expandAdj(v) = e->adjSource();

		OR.bend(e->adjSource()).set(OrthoBendType::convexBend,nBends);
		OR.bend(e->adjTarget()).set(OrthoBendType::reflexBend,nBends);
		OR.angle(adjPrev) = 1;
		OR.angle(e->adjSource()) = 2;
		OR.angle(e->adjTarget()) = 1;
	}
}

void PlanRep::collapseVertices(const OrthoRep &OR, Layout &drawing)
{
	for(node v : nodes) {
		const OrthoRep::VertexInfoUML *vi = OR.cageInfo(v);

		if(vi == nullptr ||
			(typeOf(v) != Graph::NodeType::highDegreeExpander &&
			typeOf(v) != Graph::NodeType::lowDegreeExpander))
			continue;

		node vOrig = original(v);
		OGDF_ASSERT(vOrig != nullptr);

		node vCenter = newNode();
		m_vOrig[vCenter] = vOrig;
		m_vCopy[vOrig] = vCenter;
		m_vOrig[v] = nullptr;

		node lowerLeft  = vi->m_corner[static_cast<int>(OrthoDir::North)]->theNode();
		node lowerRight = vi->m_corner[static_cast<int>(OrthoDir::West) ]->theNode();
		node upperLeft  = vi->m_corner[static_cast<int>(OrthoDir::East) ]->theNode();
		drawing.x(vCenter) = 0.5*(drawing.x(lowerLeft)+drawing.x(lowerRight));
		drawing.y(vCenter) = 0.5*(drawing.y(lowerLeft)+drawing.y(upperLeft ));

		for(adjEntry adj : vOrig->adjEntries) {
			edge eOrig = adj->theEdge();
			if(eOrig->target() == vOrig) {
				node connect = m_eCopy[eOrig].back()->target();
				edge eNew = newEdge(connect,vCenter);
				m_eOrig[eNew] = eOrig;
				m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);

			} else {
				node connect = m_eCopy[eOrig].front()->source();
				edge eNew = newEdge(vCenter,connect);
				m_eOrig[eNew] = eOrig;
				m_eIterator[eNew] = m_eCopy[eOrig].pushFront(eNew);
			}
		}
	}
}

void PlanRep::collapseVertices(const OrthoRep &OR, GridLayout &drawing)
{
	for (node v : nodes) {
		const OrthoRep::VertexInfoUML *vi = OR.cageInfo(v);

		if(vi == nullptr ||
			(typeOf(v) != Graph::NodeType::highDegreeExpander &&
			typeOf(v) != Graph::NodeType::lowDegreeExpander))
			continue;

		node vOrig = original(v);
		OGDF_ASSERT(vOrig != nullptr);

		node vCenter = newNode();
		m_vOrig[vCenter] = vOrig;
		m_vCopy[vOrig] = vCenter;
		m_vOrig[v] = nullptr;

		node lowerLeft  = vi->m_corner[static_cast<int>(OrthoDir::North)]->theNode();
		node lowerRight = vi->m_corner[static_cast<int>(OrthoDir::West) ]->theNode();
		node upperLeft  = vi->m_corner[static_cast<int>(OrthoDir::East) ]->theNode();
		drawing.x(vCenter) = (drawing.x(lowerLeft)+drawing.x(lowerRight)) >> 1;
		drawing.y(vCenter) = (drawing.y(lowerLeft)+drawing.y(upperLeft )) >> 1;

		for(adjEntry adj : vOrig->adjEntries) {
			edge eOrig = adj->theEdge();
			if(eOrig->target() == vOrig) {
				node connect = m_eCopy[eOrig].back()->target();
				edge eNew = newEdge(connect,vCenter);
				m_eOrig[eNew] = eOrig;
				m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);

			} else {
				node connect = m_eCopy[eOrig].front()->source();
				edge eNew = newEdge(vCenter,connect);
				m_eOrig[eNew] = eOrig;
				m_eIterator[eNew] = m_eCopy[eOrig].pushFront(eNew);
			}
		}
	}
}

//object types
//set type of eCopy according to type of eOrig
void PlanRep::setCopyType(edge eCopy, edge eOrig)
{
	OGDF_ASSERT(original(eCopy) == eOrig);
	m_eType[eCopy] = m_pGraphAttributes ? m_pGraphAttributes->type(eOrig) : Graph::EdgeType::association;
	if (eOrig)
	{
		switch (m_pGraphAttributes ? m_pGraphAttributes->type(eOrig) : Graph::EdgeType::association)
		{
			case Graph::EdgeType::generalization: setGeneralization(eCopy); break;
			case Graph::EdgeType::association: setAssociation(eCopy); break;
			case Graph::EdgeType::dependency: setDependency(eCopy); break;
		}
	}
}

void PlanRep::removeDeg1Nodes(ArrayBuffer<Deg1RestoreInfo> &S, const NodeArray<bool> &mark)
{
	for(node v = firstNode(); v != nullptr; v = v->succ())
	{
		if(mark[v] || v->degree() == 0)
			continue;

		adjEntry adjRef;
		for(adjRef = v->firstAdj();
			adjRef != nullptr && mark[adjRef->twinNode()];
			adjRef = adjRef->succ()) ;

		if(adjRef == nullptr) {
			// only marked nodes adjacent with v (need no reference entry)
			for(adjEntry adj : v->adjEntries) {
				node x = adj->twinNode();
				S.push(Deg1RestoreInfo(m_eOrig[adj->theEdge()],m_vOrig[x],nullptr));
				delNode(x);
			}

		} else {
			adjEntry adj, adjNext, adjStart = adjRef;
			for(adj = adjRef->cyclicSucc(); adj != adjStart; adj = adjNext)
			{
				adjNext = adj->cyclicSucc();
				node x = adj->twinNode();
				if(mark[x]) {
					S.push(Deg1RestoreInfo(m_eOrig[adj->theEdge()],m_vOrig[x],adjRef));
					delNode(x);
				} else
					adjRef = adj;
			}
		}
	}
}


void PlanRep::restoreDeg1Nodes(ArrayBuffer<Deg1RestoreInfo> &S, List<node> &deg1s)
{
	while(!S.empty())
	{
		Deg1RestoreInfo info = S.popRet();
		adjEntry adjRef = info.m_adjRef;
		node     vOrig  = info.m_deg1Original;
		edge     eOrig  = info.m_eOriginal;

		node v = newNode(vOrig);

		if(adjRef) {
			edge e = nullptr;
			if(vOrig == eOrig->source()) {
				e = newEdge(v, adjRef);
			} else {
				e = newEdge(adjRef, v);
			}
			setEdge(eOrig, e);
		} else {
			if(vOrig == eOrig->source())
				newEdge(eOrig);
			else
				newEdge(eOrig);
		}
		deg1s.pushBack(v);
	}
}


node PlanRep::newCopy(node v, Graph::NodeType vTyp)
{
	OGDF_ASSERT(m_vCopy[v] == nullptr);

	node u = newNode();
	m_vCopy[v] = u;
	m_vOrig[u] = v;
	//TODO:Typ?
	m_vType[u] = vTyp;

	return u;
}

//inserts copy for original edge eOrig after adAfter
edge PlanRep::newCopy(node v, adjEntry adAfter, edge eOrig)
{
	OGDF_ASSERT(eOrig->graphOf() == &(original()));
	OGDF_ASSERT(m_eCopy[eOrig].size() == 0);
	edge e;
	if (adAfter != nullptr)
		e = Graph::newEdge(v, adAfter);
	else
	{
		node w = copy(eOrig->opposite(original(v)));
		OGDF_ASSERT(w);
		e = Graph::newEdge(v, w);
	}
	m_eOrig[e] = eOrig;
	m_eIterator[e] = m_eCopy[eOrig].pushBack(e);
	//set type of copy
	if (m_pGraphAttributes != nullptr)
		setCopyType(e, eOrig);

	return e;
}
//inserts copy for original edge eOrig preserving the embedding
edge PlanRep::newCopy(node v, adjEntry adAfter, edge eOrig, CombinatorialEmbedding &E)
{
	OGDF_ASSERT(eOrig->graphOf() == &(original()));
	OGDF_ASSERT(m_eCopy[eOrig].size() == 0);

	edge e;
	//GraphCopy checks direction for us
	e = GraphCopy::newEdge(v, adAfter, eOrig, E);
	//set type of copy
	if (m_pGraphAttributes != nullptr)
		setCopyType(e, eOrig);

	return e;
}


edge PlanRep::split(edge e)
{
	bool cageBound = (m_expandedNode[e->source()] && m_expandedNode[e->target()])
		&& (m_expandedNode[e->source()] == m_expandedNode[e->target()]);
	node expNode = (cageBound ? m_expandedNode[e->source()] : nullptr);

	edge eNew = GraphCopy::split(e);
	m_eType[eNew] = m_eType[e];
	m_edgeTypes[eNew] = m_edgeTypes[e];
	m_expansionEdge[eNew] = m_expansionEdge[e];

	m_expandedNode[eNew->source()] = expNode;

	return eNew;
}



void PlanRep::writeGML(const char *fileName, const OrthoRep &OR, const GridLayout &drawing)
{
	std::ofstream os(fileName);
	writeGML(os,OR,drawing);
}

void PlanRep::writeGML(std::ostream &os, const OrthoRep &OR, const GridLayout &drawing)
{
	const Graph &G = *this;

	NodeArray<int> id(*this);
	int nextId = 0;

	os.setf(std::ios::showpoint);
	os.precision(10);

	os << "Creator \"ogdf::GraphAttributes::writeGML\"\n";
	os << "graph [\n";
	os << "  directed 1\n";

	for(node v : G.nodes) {
		os << "  node [\n";

		os << "    id " << (id[v] = nextId++) << "\n";

		os << "    label \"" << v->index() << "\"\n";

		os << "    graphics [\n";
		os << "      x " << ((double) drawing.x(v)) << "\n";
		os << "      y " << ((double) drawing.y(v)) << "\n";
		os << "      w " << 3.0 << "\n";
		os << "      h " << 3.0 << "\n";
		os << "      type \"rectangle\"\n";
		os << "      width 1.0\n";
		if (typeOf(v) == Graph::NodeType::generalizationMerger) {
			os << "      type \"oval\"\n";
			os << "      fill \"#0000A0\"\n";
		}
		else if (typeOf(v) == Graph::NodeType::generalizationExpander) {
			os << "      type \"oval\"\n";
			os << "      fill \"#00FF00\"\n";
		}
		else if (typeOf(v) == Graph::NodeType::highDegreeExpander ||
			typeOf(v) == Graph::NodeType::lowDegreeExpander)
			os << "      fill \"#FFFF00\"\n";
		else if (typeOf(v) == Graph::NodeType::dummy)
			os << "      type \"oval\"\n";

		else if (v->degree() > 4)
			os << "      fill \"#FFFF00\"\n";

		else
			os << "      fill \"#000000\"\n";


		os << "    ]\n"; // graphics

		os << "  ]\n"; // node
	}

	for (node v : nodes)
	{
		if (expandAdj(v) != nullptr && (typeOf(v) == Graph::NodeType::highDegreeExpander ||
			typeOf(v) == Graph::NodeType::lowDegreeExpander))
		{
			node vOrig = original(v);
			const OrthoRep::VertexInfoUML &vi = *OR.cageInfo(v);
			node ll = vi.m_corner[static_cast<int>(OrthoDir::North)]->theNode();
			node ur = vi.m_corner[static_cast<int>(OrthoDir::South)]->theNode();

			os << "  node [\n";
			os << "    id " << nextId++ << "\n";

			if (m_pGraphAttributes->has(GraphAttributes::nodeLabel)) {
				os << "    label \"" << m_pGraphAttributes->label(vOrig) << "\"\n";
			}

			os << "    graphics [\n";
			os << "      x " << 0.5 * (drawing.x(ur) + drawing.x(ll)) << "\n";
			os << "      y " << 0.5 * (drawing.y(ur) + drawing.y(ll)) << "\n";
			os << "      w " << widthOrig(vOrig) << "\n";
			os << "      h " << heightOrig(vOrig) << "\n";
			os << "      type \"rectangle\"\n";
			os << "      width 1.0\n";
			os << "      fill \"#FFFF00\"\n";

			os << "    ]\n"; // graphics
			os << "  ]\n"; // node
		}
	}

	for(edge e : G.edges)
	{
		os << "  edge [\n";

		os << "    source " << id[e->source()] << "\n";
		os << "    target " << id[e->target()] << "\n";

		os << "    generalization " << typeOf(e) << "\n";

		os << "    graphics [\n";

		os << "      type \"line\"\n";

		if (typeOf(e) == Graph::EdgeType::generalization)
		{
			if (typeOf(e->target()) == Graph::NodeType::generalizationExpander)
				os << "      arrow \"none\"\n";
			else
				os << "      arrow \"last\"\n";

			os << "      fill \"#FF0000\"\n";
			os << "      width 2.0\n";
		}
		else
		{
			if (typeOf(e->source()) == Graph::NodeType::generalizationExpander ||
			    typeOf(e->source()) == Graph::NodeType::generalizationMerger ||
			    typeOf(e->target()) == Graph::NodeType::generalizationExpander ||
			    typeOf(e->target()) == Graph::NodeType::generalizationMerger)
			{
				os << "      arrow \"none\"\n";
				os << "      fill \"#FF0000\"\n";
			}
			else if (original(e) == nullptr)
			{
				os << "      arrow \"none\"\n";
				os << "      fill \"#AFAFAF\"\n";
			}
			else
				os << "      arrow \"none\"\n";
			if (isBrother(e))
				os << "      fill \"#00AF0F\"\n";
			if (isHalfBrother(e))
				os << "      fill \"#0F00AF\"\n";
			os << "      width 1.0\n";
		}

		os << "    ]\n"; // graphics

		os << "  ]\n"; // edge
	}

	os << "]\n"; // graph
}

}
