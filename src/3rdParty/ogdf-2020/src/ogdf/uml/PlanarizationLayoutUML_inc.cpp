/** \file
 * \brief implementation of class UMLPlanarizationLayout.
 *
 * applies planarization approach for drawing UML diagrams
 * by calling a planar layouter for every planarized connected
 * component
 * Static and incremental calls available
 * Replaces cliques (if m_processCliques is set) to speed up
 * the computation, this does only work in non-UML mode
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

#include <ogdf/uml/PlanarizationLayoutUML.h>
#include <ogdf/planarity/TopologyModule.h>
#include <ogdf/planarity/PlanRepInc.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/planarity/SimpleIncNodeInserter.h>

namespace ogdf {

//incremental call: takes a fixed part of the input
//graph (indicated by fixedNodes/Edges==true), embeds it using
//the input layout, then inserts the remaining part into this embedding
//currently, only the subgraph induced by the fixed nodes is fixed
void PlanarizationLayoutUML::callIncremental(
	UMLGraph &umlGraph,
	NodeArray<bool> &fixedNodes,
	const EdgeArray<bool> & /* fixedEdges */)
{
	try {

		if(((const Graph &)umlGraph).empty())
			return;

		//check preconditions
		//(change edge types in umlGraph at non-tree hierarchies
		//or replace cliques by nodes)
		preProcess(umlGraph);

		m_nCrossings = 0; //number of inserted crossings

		//use the options set at the planar layouter
		int l_layoutOptions = m_planarLayouter->getOptions();
		bool l_align = ((l_layoutOptions & UMLOpt::OpAlign)>0);

		//check: only use alignment mode if there are generalizations
		bool l_gensExist = false; //set this for all CC's, start with first gen

		//first, we sort all edges around each node corresponding
		//to the given layout in umlGraph, then we insert the mergers
		//May be inserted in original or in copy
		bool umlMerge = false; //Standard: insertion in copy
#if 0
		if (umlMerge)
		{
			umlGraph.sortEdgesFromLayout();
			prepareIncrementalMergers(umlGraph);
		}
#endif
		//second, we embed the fixed part corresponding to the layout
		//given in umlgraph
		//TODO: check if we still need the global lists, do this
		//for all CCs
		//we create lists of the additional elements, maybe this can
		//later be done implicitly within some other loop

		// first phase: Compute planarized representation from input

		//now we derive a partial planar representation of
		//the input graph using its layout information
		PlanRepInc PG(umlGraph, fixedNodes);

		const int numCC = PG.numberOfCCs();

		//TODO: adjust for CC number before/after insertion
		// (width,height) of the layout of each connected component
		Array<DPoint> boundingBox(numCC);

		//now planarize CCs and apply drawing module
		int i;
		for(i = 0; i < numCC; ++i)
		{
			// we treat connected component i where PG is set
			// to a copy of this CC consisting only of fixed nodes
			node minActive = PG.initMinActiveCC(i);
			//if a single node was made active, we update its status
			if (minActive != nullptr)
			{
				fixedNodes[minActive] = true;
			}

#ifdef OGDF_DEBUG
			for(edge e : PG.edges)
			{
				edge eOrig = PG.original(e);
				if (eOrig)
				{
					OGDF_ASSERT(PG.chain(eOrig).size() <= 1);
				}
			}
#endif

			int nOrigVerticesPG = PG.numberOfNodes();
			//TODO: check if copying is really necessary
			//we want to sort the additional nodes and therefore
			//copy the list
			List<node> addNodes;
			for(int j = PG.startNode(); j < PG.stopNode(); ++j)
			{
				node vG = PG.v(j);
				if (!fixedNodes[vG])
					addNodes.pushBack(vG);
			}


			//now we insert the additional nodes and edges
			//sort the additional nodes
			//simple strategy: sort by their #connections to the fixed part
			//we can therefore only count if we have fixed nodes
			if ((addNodes.size() > 1) && (PG.numberOfNodesInCC() != addNodes.size()))
				sortIncrementalNodes(addNodes, fixedNodes);

			//TODO: guarantee that the CC is non-empty and connected
			//insert a first part otherwise
			//DONE: unconnected parts are connected in a chain

			//attention: the following list pop relies on the property of
			//the sorting algorithm, that the first node has a connection
			//to a fixed node or otherwise none of the addnodes has a
			//connection to fixednodes (its a CC on its own)
			//in the worst case, we have to introduce a tree, which equals
			//the Steiner tree problem, which is NP-hard. As we do not want
			//to preinsert nodes that do not have layout information (they
			//do have, but we do not consider it (e.g. for randomly
			//inserted nodes)), we avoid the problem by inserting artificial
			//edges connecting all CCs, building a tree of CCs

			//now we derive the embedding given in umlgraph for the fixed part
			//we work on a copy of umlgraph, because we have to add/delete nodes
			//and edges and create a PlanRep on an intermediate representation
			adjEntry adjExternal = nullptr;

			//here lies the main difference to the static call
			//we first set the embedding corresponding to the
			//input and then planarize the given layout
			TopologyModule TM;

			bool embedded = true;
			try { //TODO:should be catched within setEmbeddingFromGraph
				//do not yet compute external face, embed and planarize
				//with layout given in umlGraph
				embedded = TM.setEmbeddingFromGraph(PG, umlGraph,
					adjExternal, false, umlMerge);
			}
			catch(...)
			{
				embedded = false;
			}

			//returns true if connnectivity edges introduced
			//TODO: hierhin oder eins nach unten?
			PG.makeTreeConnected(adjExternal);

			//if embedding could not be set correctly, use standard embedding
			if (!embedded)
			{
				reembed(PG, i, l_align, l_gensExist);
			}

			//now we compute a combinatorial embedding on
			//the partial PlanRep that is used for node insertion
			//and the external face
			bool singleNode = (PG.numberOfNodes() == 1);
			if ((PG.numberOfEdges() > 0) || singleNode)
			{
				CombinatorialEmbedding E(PG);

				//if we have edges, but no external face, find one
				//we have also to select one later if there are no edges yet
				if((adjExternal == nullptr) && PG.numberOfEdges() > 0)
				{
					//face fExternal = E.maximalFace();
					face fExternal = findBestExternalFace(PG,E);
					adjExternal = fExternal->firstAdj();
#if 0
					while (PG.sinkConnect(adjExternal->theEdge()))
						adjExternal = adjExternal->faceCycleSucc();
#endif
				}
				if ( (adjExternal != nullptr) && (PG.numberOfEdges() > 0) )
					E.setExternalFace(E.rightFace(adjExternal));

				//we insert additional nodes into the given PlanRep
				SimpleIncNodeInserter inserter(PG);
				ListIterator<node> itAdd = addNodes.begin();
				while (itAdd.valid())
				{
					OGDF_ASSERT(PG.chain((*itAdd)->firstAdj()->theEdge()).size() <= 1);
					//we can check here if PG CC connected and speed
					//up insertion by not updating CC part information
					inserter.insertCopyNode((*itAdd), E, umlGraph.type((*itAdd)));

					//select an arbitrary external face if there was only one fixed node
					if (singleNode && (PG.numberOfEdges() > 0))
					{
						adjExternal = PG.firstEdge()->adjSource();
						E.setExternalFace(E.rightFace(adjExternal));
					}
					else
					{
						adjExternal = E.externalFace()->firstAdj();
						int eNum = max(10, PG.numberOfEdges()+1);
						int count = 0;
						while ((adjExternal->theNode() == adjExternal->twinNode()) &&
							(count < eNum))
						{
							adjExternal = adjExternal->faceCycleSucc();
							count++;
						}
						OGDF_ASSERT(count < eNum);
					}

					++itAdd;
				}

				if (!umlMerge)
					PG.setupIncremental(i, E);
#ifdef OGDF_DEBUG
				E.consistencyCheck();
#endif

					//we now have a complete representation of the
					//original CC

					m_nCrossings += PG.numberOfNodes() - nOrigVerticesPG;

				//copied from fixembed

				// third phase: Compute layout of planarized representation
				Layout drawing(PG);

				//distinguish between CC's with/without generalizations
				//this changes the input layout modules options!
				if (l_gensExist)
					m_planarLayouter->setOptions(l_layoutOptions);
				else m_planarLayouter->setOptions((l_layoutOptions & ~UMLOpt::OpAlign));


				//call the Layouter for the CC's UMLGraph
				m_planarLayouter->call(PG,adjExternal,drawing);

				// copy layout into umlGraph
				// Later, we move nodes and edges in each connected component, such
				// that no two overlap.

				//set position for original nodes and set bends for
				//all edges
				for(int j = PG.startNode(i); j < PG.stopNode(i); ++j)
				{
					node vG = PG.v(j);

					umlGraph.x(vG) = drawing.x(PG.copy(vG));
					umlGraph.y(vG) = drawing.y(PG.copy(vG));

					for(adjEntry adj : vG->adjEntries)
					{
						if ((adj->index() & 1) == 0) continue;
						edge eG = adj->theEdge();

						drawing.computePolylineClear(PG,eG,umlGraph.bends(eG));
					}
				}

				if (!umlMerge)
				{
					//insert bend point for incremental mergers
					const SList<node>& mergers = PG.incrementalMergers(i);
					SListConstIterator<node> itMerger = mergers.begin();
					while (itMerger.valid())
					{
						node vMerger = (*itMerger);
						//due to the fact that the merger may be expanded, we are
						//forced to run through the face
						adjEntry adjMerger = PG.expandAdj(vMerger);
						//first save a pointer to the bends of the generalization
						//leading to the target
						DPolyline dpUp;

						//check if there is an expansion face
						if (adjMerger != nullptr)
						{
							//if there are bends on the edge, copy them
							//TODO: check where to derive the bends from
							//if eUp is nil
							adjEntry adjUp = adjMerger->cyclicPred();
							OGDF_ASSERT(PG.isGeneralization(adjUp->theEdge()));
							edge eUp = PG.original(adjUp->theEdge());
							//There is never an original here in current incremental algo
							if (eUp) dpUp = umlGraph.bends(eUp);
							//we run through the expansion face to the connected edges
							//this edge is to dummy corner
							adjEntry runAdj = adjMerger->faceCycleSucc();
							while (runAdj != adjMerger)
							{
								node vConnect = runAdj->theNode();
								//because of the node collapse using the original
								//edges instead of the merger copy edges (should be
								//fixed for incremental mode) the  degree is 4
								//FIXED!?
								//if (vConnect->degree() != 4)
								//while?
								if (vConnect->degree() != 3)
								{
									runAdj = runAdj->faceCycleSucc();
									continue;
								}
								edge eCopy = runAdj->cyclicPred()->theEdge();
								OGDF_ASSERT(eCopy->target() == runAdj->theNode());
								OGDF_ASSERT(PG.isGeneralization(eCopy));
								OGDF_ASSERT(PG.original(eCopy));

								DPolyline &eBends = umlGraph.bends(PG.original(eCopy));

								eBends.pushBack(
									DPoint(drawing.x(vMerger), drawing.y(vMerger)));

								if (eUp)
								{
									ListConstIterator<DPoint> itDp;
									for(itDp = dpUp.begin(); itDp.valid(); ++itDp)
										eBends.pushBack(*itDp);
								}
								else
									eBends.pushBack(DPoint(drawing.x(adjUp->twinNode()),
									drawing.y(adjUp->twinNode())));

								runAdj = runAdj->faceCycleSucc();

							}

						}
						else //currently all nodes are expanded, but this is not guaranteed
						{
							//first save the bends
							adjEntry adjUp = nullptr;
							for(adjEntry adjVMerger : vMerger->adjEntries)
							{
								//upgoing edge
								if (adjVMerger->theEdge()->source() == vMerger)
								{
									adjUp = adjVMerger;
									OGDF_ASSERT(PG.isGeneralization(adjVMerger->theEdge()));
									edge eUp = PG.original(adjVMerger->theEdge());
									//check if this is
									//a) the merger up edge and not a connectivity edge
									//b) what to do if there is no original of outgoing edge
									if (eUp)
										dpUp = umlGraph.bends(eUp);
									break;
								}
							}

							for(adjEntry adjVMerger : vMerger->adjEntries)
							{
								if (adjVMerger->theEdge()->target() == vMerger)
								{
									edge eOrig = PG.original(adjVMerger->theEdge());
									if (eOrig)
									{
										//incoming merger edges always have an original here!
										umlGraph.bends(eOrig).pushBack(DPoint(drawing.x(vMerger),
											drawing.y(vMerger)));

										//was there an original edge?
										if (!dpUp.empty())
										{
											ListConstIterator<DPoint> itDp;
											for(itDp = dpUp.begin(); itDp.valid(); ++itDp)
												umlGraph.bends(eOrig).pushBack(*itDp);
										} else {
											if (adjUp)
												umlGraph.bends(eOrig).pushBack(DPoint(drawing.x(adjUp->twinNode()),
												drawing.y(adjUp->twinNode())));
										}
									}
								}
							}
						}
						++itMerger;
					}
				}

				// the width/height of the layout has been computed by the planar
				// layout algorithm; required as input to packing algorithm
				boundingBox[i] = m_planarLayouter->getBoundingBox();
			} else {
				//TODO: what if there are no edges but the insertion edges
				//DONE: we make the CC treeConnected
				//Nonetheless we have to compute a layout here
				OGDF_ASSERT(PG.numberOfNodes() < 2);
			}



			//TODO: set m_crossings here
#if 0
			m_nCrossings += PG.numberOfNodes() - numOrigNodes;
#endif
		}

		//TODO: check shifting to new place

		// Arrange layouts of connected components

		arrangeCCs(PG, umlGraph, boundingBox);

		if (umlMerge) umlGraph.undoGenMergers();

		umlGraph.removeUnnecessaryBendsHV();

		//new position after adding clique process, check if correct
		postProcess(umlGraph);
	}
	catch (...)
	{
		call(umlGraph);
		return;
	}
}

//Insertion order of added nodes:
//sorting strategy: we count all adjacent nodes that are in
//fixedNodes and sort the nodes correspondingly
//In addition, we guarantee that no node is inserted before at least one
//of its neighbours is inserted

//compute how far away from the fixed part the added nodes lay
//uses BFS
void PlanarizationLayoutUML::getFixationDistance(node startNode,
	HashArray<int, int> &distance,
	const NodeArray<bool> &fixedNodes)
{
	//we only change the distance for nodes with distance <=0 because they are not
	//connected to the fixed part
	//we only care about nodes which are members of array distance
	HashArray<int, bool> indexMark(false);
	//the BFS queue
	QueuePure<node> nodeQ;

	nodeQ.append(startNode);
	indexMark[startNode->index()] = true;
	while (!nodeQ.empty())
	{
		node topNode = nodeQ.pop();
		//hier aufpassen: geht nur, wenn kein fixedNode eine Distance hat
		//alternativ: alle auf null
		//zur sicherheit: fixed uebergeben und vergleichen
		bool fixedBase = fixedNodes[topNode];

		for(adjEntry adjE : topNode->adjEntries)
		{
			node testNode = adjE->twinNode();
			int ind = testNode->index();

			if (!indexMark[ind])
			{
				indexMark[ind] = true;
				nodeQ.append(testNode);
			}

			//we have a distance value, i.e. ind is an additional node
			if (!fixedNodes[testNode])
			{
				if (distance[ind] <= 0)
				{
					//should never occur (check: nodes not in CC?)
					if (fixedBase)
					{
						distance[ind] = max(-1, distance[ind]);
						OGDF_ASSERT(false);
					} else {
						if (distance[ind] == 0) distance[ind] = min(-1, distance[topNode->index()] - 1);
						else distance[ind] = min(-1, max(distance[ind], distance[topNode->index()] - 1));
					}
				}
			}
		}
	}
}

//Attention: changing this behavior makes it necessary to check
//the call procedure for the case where one of the addnodes is
//made fix to allow insertion of an edge
void PlanarizationLayoutUML::sortIncrementalNodes(List<node> &addNodes,
	const NodeArray<bool> &fixedNodes)
{
	//if there are no fixed nodes, we can not sort
	//todo: do some other sorting

	//we count all adjacent fixed nodes
	//store degree by node index
	HashArray<int, int> indexToDegree(0);
	ListIterator<node> it = addNodes.begin();
	adjEntry adjE;
	node someFixedNode = nullptr;

	while (it.valid())
	{
		if ((*it)->degree() < 1)
		{
			indexToDegree[(*it)->index()] = 0;
			++it;
			continue;
		}
		int vDegree = 0;
		adjE = (*it)->firstAdj();
		do {
			if (fixedNodes[adjE->twinNode()])
			{
				vDegree++;
				someFixedNode = adjE->twinNode();
			}
			adjE = adjE->cyclicSucc();
		} while (adjE != (*it)->firstAdj());

		indexToDegree[(*it)->index()] = vDegree;

		++it;
	}
	//for all nodes that are not connected to the fixed part we have to guarantee
	//that they are not inserted before one of their neighbours is inserted
	//therefore we set negative values for all nodes corresponding to their distance
	//to the fixed part
	OGDF_ASSERT(someFixedNode != nullptr);
	if (someFixedNode == nullptr) throw AlgorithmFailureException();
	//we start the BFS at some fixed node
	getFixationDistance(someFixedNode,indexToDegree, fixedNodes);

	// we sort the nodes in decreasing vDegree value order
	struct AddNodeComparer : public GenericComparer<node, int> {
		AddNodeComparer(HashArray<int, int> &ha) : GenericComparer([&](node v) { return -ha[v->index()]; }) {}
	};
	addNodes.quicksort(AddNodeComparer(indexToDegree));
}

void PlanarizationLayoutUML::reembed(PlanRepUML &pr, int ccNumber, bool l_align,
	bool /* l_gensExist */)
{
	//TODO: update by reinitialization?
#if 0
	PG.initActiveCC(i);
#endif
	//first we remove all inserted crossings
	List<node> crossings;
	for(node v : pr.nodes)
	{
		if (pr.isCrossingType(v))
		{
			crossings.pushBack(v);
		}
	}
	ListIterator<node> it = crossings.begin();
	while (it.valid())
	{
		pr.removeCrossing((*it));
		++it;
	}

	// first phase: Compute a planar subgraph

	// The planar subgraph should contain as many generalizations
	// as possible, hence we put all generalizations into the list
	// preferedEdges.
	//List<edge> preferedEdges;
	EdgeArray<int> costOrig(pr.original(), 1);
	for(edge e : pr.edges)
	{
		if (pr.typeOf(e) == Graph::EdgeType::generalization)
		{
#if 0
			if (l_align) l_gensExist = true;
			preferedEdges.pushBack(e);
#endif
			edge ori = pr.original(e);
			//high cost to allow alignment without crossings
			if ( (l_align) &&
				((ori && (pr.typeOf(e->target()) == Graph::NodeType::generalizationMerger))
				|| (pr.alignUpward(e->adjSource()))
				)
				)
				costOrig[ori] = 10;
		}
	}

	int cr;
	m_crossMin->call(pr, ccNumber, cr, &costOrig);

#if 0
	List<edge> deletedEdges;
	m_subgraph->callAndDelete(PG, preferedEdges, deletedEdges);
#endif


	//// second phase: Re-insert deleted edges
#if 0
	m_inserter->callForbidCrossingGens(PG, costOrig, deletedEdges);
#endif

	OGDF_ASSERT(isPlanar(pr));


	//
	// determine embedding of PG
	//

	// We currently compute any embedding and choose the maximal face
	// as external face

	// if we use FixedEmbeddingInserterOld, we have to re-use the computed
	// embedding, otherwise crossing nodes can turn into "touching points"
	// of edges (alternatively, we could compute a new embedding and
	// finally "remove" such unnecessary crossings).
	if(!pr.representsCombEmbedding())
		planarEmbed(pr);


#if 0
	// CG: This code does not do anything...
	adjEntry adjExternal = 0;

	if(PG.numberOfEdges() > 0)
	{
		CombinatorialEmbedding E(PG);
		//face fExternal = E.maximalFace();
		face fExternal = findBestExternalFace(PG,E);
		adjExternal = fExternal->firstAdj();
#if 0
		while (PG.sinkConnect(adjExternal->theEdge()))
			adjExternal = adjExternal->faceCycleSucc();
#endif
	}
#endif
}

}
