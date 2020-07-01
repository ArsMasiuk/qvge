/** \file
 * \brief Reinsertion of deleted edges in embedded subgraph with
 * modeled cluster boundaries.
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


#include <ogdf/cluster/CPlanarEdgeInserter.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/fileformats/GraphIO.h>

namespace ogdf {


//Note that edge insertions in cluster (sub)graphs are always performed
//on already embedded graphs with modeled cluster boundaries
void CPlanarEdgeInserter::call(
	ClusterPlanRep& CPR,
	CombinatorialEmbedding& E,
	const Graph& G,
	const List<edge>& origEdges)
{
	OGDF_ASSERT(&E.getGraph() == &CPR);

	m_originalGraph = &G;
	FaceArray<node> nodeOfFace(E, nullptr);
#if 0
	NodeArray<face>&, faceOfNode(m_dualGraph, 0);
#endif
	EdgeArray<edge> arcRightToLeft(CPR, nullptr);//arc from srcadj face to tgtadj face
	EdgeArray<edge> arcLeftToRight(CPR, nullptr);//vice versa
	EdgeArray<edge> arcTwin(m_dualGraph, nullptr);
	m_arcOrig.init(m_dualGraph, nullptr);

	constructDualGraph(CPR, E, arcRightToLeft, arcLeftToRight, nodeOfFace, arcTwin);
	//the dual graph has a node for each face of E
	//and two arcs for every edge of CPR

	m_eStatus.init(m_dualGraph, 0); //1 = usable

	const ClusterGraph& CG = CPR.getClusterGraph();

	//every face is completely inside a cluster (at least root)
	//facenodes are associated with clusters
	NodeArray<cluster> clusterOfFaceNode(m_dualGraph, nullptr);
	deriveFaceCluster(CPR, E, CG, nodeOfFace, clusterOfFaceNode);

	//nodes representing the edge endpoints
	node uDummy = m_dualGraph.newNode();
	node vDummy = m_dualGraph.newNode();

	//for each edge (u,v) to be inserted, we need the path in the
	//cluster hierarchy to orient the dual arcs (set the status)
	for(edge e : origEdges)
	{
		//m_eStatus.fill(0); do this manually
		//first, we temporarily insert connections from node dummies
		//to the faces adjacent to start- and endpoint of the edge
		node oSource = e->source();
		node oTarget = e->target();
		node u = CPR.copy(oSource);
		node v = CPR.copy(oTarget);

		List<cluster> cList;

		//we compute the cluster tree path between oS and oT
		CG.commonClusterPath(oSource, oTarget, cList);

		//orient the edges according to cluster structure
		//save which clusters are on path from u to v
		//(do this by setting their edge status)
		EdgeArray<bool> done(m_dualGraph, false);
		for(edge eArc : m_dualGraph.edges)
		{
			if (done[eArc]) continue; //twin already processed
			if (arcTwin[eArc] == nullptr) { // dummies
				done[eArc] = true;
				continue;
			}

			cluster c1 = clusterOfFaceNode[eArc->source()];
			cluster c2 = clusterOfFaceNode[eArc->target()];

			ListIterator<cluster> itC = cList.begin();

			OGDF_ASSERT(itC.valid());
			//run over path and search for c1, c2
			int ind = 1, ind1 = 0, ind2 = 0;
			while (itC.valid())
			{
				cluster cCheck = (*itC);

				if (cCheck == c1)
				{
					ind1 = ind;
				}
				if (cCheck == c2)
				{
					ind2 = ind;
				}

				++itC;
				++ind;

				//stop search, both clusters found
				if ((ind1 > 0) && (ind2 > 0))
					itC = cList.end();
			}
			//set status
			if ((ind1 > 0 ) && (ind2 > 0))
			{
				if (ind1 == ind2) //bidirectional
				{
					m_eStatus[eArc] = 1;
					m_eStatus[arcTwin[eArc]] = 1;
				} else
					if (ind1 < ind2)
					{
						m_eStatus[eArc] = 1;
						m_eStatus[arcTwin[eArc]] = 0;
					}
					else
					{
						m_eStatus[eArc] = 0;
						m_eStatus[arcTwin[eArc]] = 1;
					}

			}
			else
			{
				//remove edge
				m_eStatus[eArc] = 0;
				m_eStatus[arcTwin[eArc]] = 0;
			}

			done[arcTwin[eArc]] = true;
			done[eArc] = true;
		}

		// we compute the shortest path
		SList<adjEntry> crossed;
		findShortestPath(E, u, v, uDummy, vDummy, crossed, nodeOfFace);

		// we insert the edge
		insertEdge(CPR, E, e, nodeOfFace, arcRightToLeft, arcLeftToRight, arcTwin, clusterOfFaceNode, crossed);

		// we updated the dual graph and are ready to insert the next edge
	}

	//delete artificial endpoint representations
	m_dualGraph.delNode(vDummy);
	m_dualGraph.delNode(uDummy);
}

// protected member functions

void CPlanarEdgeInserter::constructDualGraph(ClusterPlanRep& CPR,
                                             CombinatorialEmbedding& E,
                                             EdgeArray<edge>& arcRightToLeft,
                                             EdgeArray<edge>& arcLeftToRight,
                                             FaceArray<node>& nodeOfFace,
                                             //NodeArray<face>&, faceOfNode,
                                             EdgeArray<edge>& arcTwin)
{
	//dual graph gets two arcs for each edge (in both directions)
	//these arcs get their status (usable for path) depending on
	//the edge to be reinserted

	m_dualGraph.clear();
#if 0
	faceOfNode.init(m_dualGraph, 0);
#endif

	//construct nodes
	//corresponding to the graphs faces
	face f;
	for (f = E.firstFace(); f; f = f->succ())
	{
		node v = m_dualGraph.newNode();
		nodeOfFace[f] = v;
#if 0
		faceOfNode[v] = f;
#endif
	}

	for(edge e : CPR.edges)
	{
		edge arc1 = m_dualGraph.newEdge( nodeOfFace[E.rightFace(e->adjTarget())],
			nodeOfFace[E.rightFace(e->adjSource())] );
		arcLeftToRight[e] = arc1;
		edge arc2 = m_dualGraph.newEdge( nodeOfFace[E.rightFace(e->adjSource())],
			nodeOfFace[E.rightFace(e->adjTarget())] );
		arcRightToLeft[e] = arc2;
		arcTwin[arc1] = arc2;
		arcTwin[arc2] = arc1;
		m_arcOrig[arc1] = e->adjSource();//e->adjTarget();
		m_arcOrig[arc2] = e->adjTarget();//e->adjSource();
	}
}

//private functions
void CPlanarEdgeInserter::deriveFaceCluster(ClusterPlanRep& CPR,
											CombinatorialEmbedding& E,
											const ClusterGraph& CG,
											FaceArray<node>& nodeOfFace,
											NodeArray<cluster>& clusterOfFaceNode)
{
	//we need to map indices to clusters
	//cluster numbers don't need to be consecutive
	HashArray<int, cluster> ClusterOfIndex;
	for(cluster ci : CG.clusters)
	{
		ClusterOfIndex[ci->index()] = ci; //numbers are unique
	}

	face f;
	for (f = E.firstFace(); f; f = f->succ())
	{
		//we examine all face nodes
		//nodes v with original define the cluster in which the face lies
		//- it's cluster(original(v))
		//dummy nodes can sit on unbounded many different cluster boundaries
		//either one is the parent of another (=> is the searched face)
		//or all lie in the same parent face
		cluster c1 = nullptr;
		cluster cResult = nullptr;
		for(adjEntry adjE : f->entries)
		{
			node v = adjE->theNode();
			if (CPR.original(v))
			{
				cResult = CG.clusterOf(CPR.original(v));
				break;
			} else {
				//a dummy node on a cluster boundary
				cluster c = ClusterOfIndex[CPR.ClusterID(v)];
				if (!c1) c1 = c;
				else
				{
					if (c != c1)
					{
						//either they lie in the same parent or one is the parent
						//of the other one
						OGDF_ASSERT( (c->parent() == c1->parent()) || (c1 == c->parent()) || (c == c1->parent()) );
						if (c1 == c->parent())
						{
							cResult = c1;
							break;
						}
						if (c == c1->parent())
						{
							cResult = c;
							break;
						}
						if (c->parent() == c1->parent())
						{
							cResult = c->parent();
							break;
						}
					}
				}
			}
		}

		OGDF_ASSERT(cResult);
		clusterOfFaceNode[nodeOfFace[f]] = cResult;
	}
}

// finds a shortest path in the dual graph augmented by s and t (represented
// by sDummy and tDummy); returns list of crossed adjacency entries (corresponding
// to used edges in the dual) in crossed.
//
void CPlanarEdgeInserter::findShortestPath(
	const CombinatorialEmbedding &E,
	node s, //edge startpoint
	node t,	//edge endpoint
	node sDummy, //representing s in network
	node tDummy, //representing t in network
	//Graph::EdgeType eType,
	SList<adjEntry> &crossed,
	FaceArray<node>& nodeOfFace)
{
	OGDF_ASSERT(s != t);

	OGDF_ASSERT(sDummy->graphOf() == tDummy->graphOf());

	OGDF_ASSERT(s->graphOf() == t->graphOf());

	NodeArray<edge> spPred(m_dualGraph,nullptr);
	QueuePure<edge> queue;
	int oldIdCount = m_dualGraph.maxEdgeIndex();

	//list of current best path
	SList<adjEntry> bestCrossed;
	SList<adjEntry> currentCrossed;
	//int bestCost = 4*m_dualGraph.numberOfEdges(); //just an upper bound

	//insert connections to adjacent faces
	//be careful with selfloops and bridges (later)
	for(adjEntry adjE : s->adjEntries)
	{
		edge eNew = m_dualGraph.newEdge(sDummy, nodeOfFace[E.rightFace(adjE)]);
		m_arcOrig[eNew] = adjE;
		m_eStatus[eNew] = 1;
	}
	for(adjEntry adjE : t->adjEntries)
	{
		edge eNew = m_dualGraph.newEdge(nodeOfFace[E.rightFace(adjE)], tDummy);
		m_arcOrig[eNew] = adjE;
		m_eStatus[eNew] = 1;
	}

	// Start with outgoing edges
	for(adjEntry adj : sDummy->adjEntries) {
		// starting edges of bfs-search are all edges leaving s
#if 0
		edge eDual = m_dual.newEdge(m_vS, m_nodeOf[E.rightFace(adj)]);
		m_primalAdj[eDual] = adj;
#endif
		queue.append(adj->theEdge());
	}
	OGDF_ASSERT(!queue.empty());
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
			if (v == tDummy)
			{
				// ... then search is done.
				// We should not stop here but calculate the cost and save
				// the path if it is an improvement

				// constructed list of used edges (translated to crossed
				// adjacency entries in PG) from t back to s (including first
				// and last!)

				do {
					edge eDual = spPred[v];
					if (m_arcOrig[eDual] != nullptr)
						currentCrossed.pushFront(m_arcOrig[eDual]);
					v = eDual->source();
				} while(v != sDummy);

				//now check if the current solution is cheaper than bestCrossed
				//min cross is 1 at this point
				bool betterSol = false;
				if (bestCrossed.empty()) betterSol = true;
				if (!betterSol)
				{
					//derive actual cost
					for(adjEntry adj : currentCrossed)
					{
						//here we can check different edge costs

						//only temporary: just fill in
						bestCrossed.pushBack(adj);
					}
				}
				//cop current into best
				if (betterSol)
				{
					for(adjEntry adj : currentCrossed) {
						bestCrossed.pushBack(adj);
					}
				}

				break;//only temporary, rebuild path later
			}

			// append next candidate edges to queue
			// (all edges leaving v)
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();

				if ((v == e->source()) &&
					(m_eStatus[e] == 1) )
				{
					queue.append(e);
				}
			}
		}
	}

	//set result in list parameter
	for(adjEntry adj : bestCrossed) {
		crossed.pushBack(adj);
	}

	bestCrossed.clear();
	currentCrossed.clear();

	//delete dummies
	//connections and update graph
	List<edge> delMe;
	for(adjEntry adjE : sDummy->adjEntries)
	{
		delMe.pushBack(adjE->theEdge());
	}
	while (!delMe.empty())
		m_dualGraph.delEdge(delMe.popFrontRet());

	for(adjEntry adjE : tDummy->adjEntries)
	{
		delMe.pushBack(adjE->theEdge());
	}
	while (!delMe.empty())
		m_dualGraph.delEdge(delMe.popFrontRet());
#if 0
	// remove augmented edges again
	while ((adj = sDummy->firstAdj()) != 0)
		m_dualGraph.delEdge(adj->theEdge());

	while ((adj = tDummy->firstAdj()) != 0)
		m_dualGraph.delEdge(adj->theEdge());
#endif
	m_dualGraph.resetEdgeIdCount(oldIdCount);
}

// inserts edge e according to insertion path crossed.
// updates embeding and dual graph
//
void CPlanarEdgeInserter::insertEdge(
	ClusterPlanRep &CPR,
	CombinatorialEmbedding &E,
	edge insertMe,
	FaceArray<node>& nodeOfFace,
	EdgeArray<edge>& arcRightToLeft,
	EdgeArray<edge>& arcLeftToRight,
	EdgeArray<edge>& arcTwin,
	NodeArray<cluster>& clusterOfFaceNode,
	const SList<adjEntry> &crossed)
{
	// remove dual nodes on insertion path

	List<cluster> faceCluster; //clusters of deleted faces

	//first node double, what about last?
	ArrayBuffer<node> delS;
	for(adjEntry adj : crossed)
	{
#if 0
		m_dualGraph.delNode(nodeOfFace[E.rightFace(*it)]);
#endif
		if (!delS.empty())
		{
			if (delS.top() != nodeOfFace[E.rightFace(adj)])
			{
				delS.push(nodeOfFace[E.rightFace(adj)]);
				faceCluster.pushBack(clusterOfFaceNode[nodeOfFace[E.rightFace(adj)]]);
			}
		}
		else
		{
			delS.push(nodeOfFace[E.rightFace(adj)]);
			faceCluster.pushBack(clusterOfFaceNode[nodeOfFace[E.rightFace(adj)]]);
		}
	}

	while (!delS.empty())
	{
		m_dualGraph.delNode(delS.popRet());
	}
#if 0
	for(it = crossed.begin(); it.valid(); it++)
	{
		//only dummy edge
		if (!( (CPR.copy(np.m_src) == (*it)->theNode())
			 || (CPR.copy(np.m_tgt) == (*it)->theNode())))
			m_dualGraph.delNode(nodeOfFace[E.rightFace(*it)]);
	}
#endif

	// update primal
	CPR.insertEdgePathEmbedded(insertMe, E, crossed);


	// insert new face nodes into dual
	const List<edge> &path = CPR.chain(insertMe);

	OGDF_ASSERT(faceCluster.size() == path.size());
	ListConstIterator<cluster> itC = faceCluster.begin();

	for(edge ei : path)
	{
		adjEntry adj = ei->adjSource();
		nodeOfFace[E.leftFace (adj)] = m_dualGraph.newNode();
		nodeOfFace[E.rightFace(adj)] = m_dualGraph.newNode();
		clusterOfFaceNode[nodeOfFace[E.leftFace (adj)]] = (*itC);
		clusterOfFaceNode[nodeOfFace[E.rightFace (adj)]] = (*itC);
		++itC;

	}

	//update network for both faces

	// insert new edges into dual
	for(edge ei : path)
	{
		adjEntry adjSrc = ei->adjSource();
		face f = E.rightFace(adjSrc);  // face to the right of adj in loop
		node vRight = nodeOfFace[f];

		adjEntry adj1 = f->firstAdj(), adj = adj1;
		do {
			node vLeft = nodeOfFace[E.leftFace(adj)];

			edge eLR = m_dualGraph.newEdge(vLeft,vRight);
			m_arcOrig[eLR] = adj;

			edge eRL = m_dualGraph.newEdge(vRight,vLeft);
			m_arcOrig[eRL] = adj->twin();

			arcTwin[eLR] = eRL;
			arcTwin[eRL] = eLR;

			//now check if edge can be used
			setArcStatus(eLR, insertMe->source(), insertMe->target(), CPR.getClusterGraph(),
			             clusterOfFaceNode, arcTwin);

			if (adj == adj->theEdge()->adjSource())
			{
				arcLeftToRight[adj->theEdge()] = eLR;
				arcRightToLeft[adj->theEdge()] = eRL;
#if 0
				m_arcOrig[eLR] = e->adjTarget();
				m_arcOrig[eRL] = e->adjSource();
#endif
			}
			else
			{
				arcLeftToRight[adj->theEdge()] = eRL;
				arcRightToLeft[adj->theEdge()] = eLR;
#if 0
				m_arcOrig[eRL] = e->adjTarget();
				m_arcOrig[eLR] = e->adjSource();
#endif
			}

		}
		while((adj = adj->faceCycleSucc()) != adj1);

		// the other face adjacent to *itEdge ...
		f = E.rightFace(adjSrc->twin());
		vRight = nodeOfFace[f];

		adj1 = f->firstAdj();
		adj = adj1;
		do {
			node vLeft = nodeOfFace[E.leftFace(adj)];

			edge eLR = m_dualGraph.newEdge(vLeft,vRight);
			m_arcOrig[eLR] = adj;

			edge eRL = m_dualGraph.newEdge(vRight,vLeft);
			m_arcOrig[eRL] = adj->twin();

			arcTwin[eLR] = eRL;
			arcTwin[eRL] = eLR;

			if (adj == adj->theEdge()->adjSource())
			{
				arcLeftToRight[adj->theEdge()] = eLR;
				arcRightToLeft[adj->theEdge()] = eRL;
#if 0
				m_arcOrig[eLR] = e->adjTarget();
				m_arcOrig[eRL] = e->adjSource();
#endif
			}
			else
			{
				arcLeftToRight[adj->theEdge()] = eRL;
				arcRightToLeft[adj->theEdge()] = eLR;
#if 0
				m_arcOrig[eRL] = e->adjTarget();
				m_arcOrig[eLR] = e->adjSource();
#endif
			}

		}
		while((adj = adj->faceCycleSucc()) != adj1);
	}
}

//sets status for new arc and twin
//uses dual arc, original nodes of edge to be inserted, ClusterGraph
void CPlanarEdgeInserter::setArcStatus(
	edge eArc,
	node oSrc,
	node oTgt,
	const ClusterGraph& CG,
	NodeArray<cluster>& clusterOfFaceNode,
	EdgeArray<edge>& arcTwin)
{
	cluster c1 = clusterOfFaceNode[eArc->source()];
	cluster c2 = clusterOfFaceNode[eArc->target()];
#if 0
	std::cout<< "Searching for clusters " << c1 << " and " << c2 << "\n";
#endif
	List<cluster> cList;

	//we compute the cluster tree path between oS and oT
	CG.commonClusterPath(oSrc, oTgt, cList);
	ListIterator<cluster> itC = cList.begin();
	OGDF_ASSERT(itC.valid());
	//run over path and search for c1, c2
	int ind = 0, ind1 = 0, ind2 = 0;
	while (itC.valid())
	{
		cluster cCheck = (*itC);
#if 0
		std::cout << "Checking " << cCheck << "\n" << std::flush;
#endif

		if (cCheck == c1)
		{
			ind1 = ind;
#if 0
			std::cout << "Found c1 " << cCheck << " at number "<< ind << "\n" << std::flush;
#endif
		}
		if (cCheck == c2)
		{
			ind2 = ind;
#if 0
			std::cout << "Found c2 " << cCheck << " at number "<< ind << "\n" << std::flush;
#endif
		}

		++itC;
		++ind;

		//stop search, both clusters found
		if ((ind1 > 0) && (ind2 > 0))
			itC = cList.end();
	}

	//set status
	OGDF_ASSERT(arcTwin[eArc]);
	if ((ind1 > 0 ) && (ind2 > 0))
	{
		if (ind1 == ind2) //bidirectional
		{
			m_eStatus[eArc] = 1;
			m_eStatus[arcTwin[eArc]] = 1;
		} else {
			if (ind1 < ind2)
			{
				m_eStatus[eArc] = 1;
				m_eStatus[arcTwin[eArc]] = 0;
			} else {
				m_eStatus[eArc] = 0;
				m_eStatus[arcTwin[eArc]] = 1;
			}
		}
	} else {
		//remove edge
		m_eStatus[eArc] = 0;
		m_eStatus[arcTwin[eArc]] = 0;
	}
}

//improve the insertion result by heuristics
//TODO
void CPlanarEdgeInserter::postProcess()
{
	switch (m_ppType)
	{
		case PostProcessType::RemoveReinsert:

			break;
		default:
			break;
	}
}

//file output

void CPlanarEdgeInserter::writeDual(const char *fileName)
{
	Layout drawing(m_dualGraph);
	std::ofstream os(fileName);
	writeGML(os,drawing);
}


void CPlanarEdgeInserter::writeGML(std::ostream &os, const Layout &drawing)
{
	const Graph &G = m_dualGraph;
	GraphAttributes GA(G,
	  GraphAttributes::nodeLabel |
	  GraphAttributes::nodeGraphics |
	  GraphAttributes::nodeStyle |
	  GraphAttributes::edgeGraphics |
	  GraphAttributes::edgeStyle);

	GA.directed() = true;

	for (node v : G.nodes) {
		GA.label(v) = to_string(v->index());
		GA.x(v) = drawing.x(v);
		GA.y(v) = drawing.y(v);
		GA.width(v) = GA.height(v) = 10.0;
		GA.shape(v) = Shape::Rect;
		// "width 1.0"?
		// or GA.shape(v) = Shape::Ellipse?
		GA.fillColor(v) = "00FF00";
	}

	for (edge e : G.edges) {
		GA.strokeColor(e) = m_eStatus[e] > 0 ? "FF0000" : "0000FF";
		GA.strokeWidth(e) = 3.0;
	}

	GraphIO::writeGML(GA, os);
}

}
