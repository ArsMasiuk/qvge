/** \file
 * \brief Implementation of FixedEmbeddingUpwardInserter class.
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

#include <ogdf/basic/Queue.h>
#include <ogdf/basic/PriorityQueue.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/upward/UpwardPlanRep.h>
#include <ogdf/upward/FixedEmbeddingUpwardEdgeInserter.h>


//debug only
#include <ogdf/upward/LayerBasedUPRLayout.h>


namespace ogdf {

Module::ReturnType FixedEmbeddingUpwardEdgeInserter::doCall(
	UpwardPlanRep &UPR,
	const List<edge> &origEdges,
	const EdgeArray<int>  *costOrig,
	const EdgeArray<bool> *forbiddenEdgeOrig)
{
	if (origEdges.empty())
		return Module::ReturnType::Feasible;

	List<edge> toInsert = origEdges;

	if (!UPR.augmented())
		UPR.augment();

	EdgeArray<int> cost;
	if (costOrig != nullptr)
		cost = *costOrig;
	else
		cost.init(UPR.original(), 1);

	if (forbiddenEdgeOrig != nullptr) {
		for(edge e : UPR.original().edges) {
			if ((*forbiddenEdgeOrig)[e])
				cost[e] = std::numeric_limits<int>::max();
		}
	}

#if 0
	std::cout << std::endl << std::endl << "edge to insert " << toInsert.size() << " : " << std::endl;
	for(edge e : toInsert) {
		std::cout << "edge : " << e << std::endl;
	}
	std::cout << std::endl << std::endl << "number of edges to insert: " << toInsert.size() << std::endl;
#endif

	return insertAll(UPR, toInsert, cost);
}



Module::ReturnType FixedEmbeddingUpwardEdgeInserter::insertAll(
	UpwardPlanRep &UPR,
	List<edge> &toInsert,
	EdgeArray<int>  &costOrig)
{
	if (toInsert.empty())
		return Module::ReturnType::Feasible;

	List<edge> list;
	int size_new = toInsert.size();
	int size_old = 0;
	while (size_old != size_new) {
		size_old = size_new;
		while (!toInsert.empty()) {
			edge e_orig = toInsert.popFrontRet();
			SList<adjEntry> path;

#if 0
			std::cout << std::endl;
			std::cout << "  insertion path for e_orig :" << e_orig << ";  e_UPR: (" << m_UPR.copy(e_orig->source()) << ","
				 << m_UPR.copy(e_orig->target()) << ")" << std::endl;
#endif

			minFIP(UPR, toInsert, costOrig, e_orig, path);

#if 0
			for(adjEntry adj : path) {
				std::cout << adj->theEdge() << ";  node: " << adj->theNode() << std::endl;
			}
#endif

			List<edge> lEdges = toInsert, lTmp = list;
			lEdges.conc(lTmp);
			bool ok = isConstraintFeasible(UPR, lEdges, e_orig, path);
			if (ok) {
				UPR.insertEdgePathEmbedded(e_orig, path, costOrig);

				OGDF_ASSERT(isUpwardPlanar(UPR));
				OGDF_ASSERT(isSimple(UPR));
				OGDF_ASSERT(isConnected(UPR));
				OGDF_ASSERT(hasSingleSource(UPR));

			}
			else
				list.pushBack(e_orig);

#if 0
#if 0
			m_UPR.outputFaces(m_UPR.getEmbedding());
			m_UPR.writeGML("c:/temp/bug5.gml");
#endif

			LayerBasedUPRLayout uprLayout;
			Graph GTmp( (const Graph &) m_UPR);
			CombinatorialEmbedding embTmp(GTmp);
			node tTmp = 0;
#if 0
			GTmp.writeGML("c:/temp/bug4.gml");
#endif
			hasSingleSink(GTmp, tTmp);
			OGDF_ASSERT(tTmp != 0);
			embTmp.setExternalFace(embTmp.rightFace(tTmp->firstAdj()));
#if 0
			adjEntry adjTmp = GCTmp.copy(m_UPR.extFaceHandle->theEdge())->adjTarget();
#endif
			UpwardPlanRep upr_bug(embTmp);
			adjEntry adj_bug = upr_bug.getAdjEntry(upr_bug.getEmbedding(), upr_bug.getSuperSource(), upr_bug.getEmbedding().externalFace());
			node s_upr_bug = upr_bug.newNode();
			upr_bug.getEmbedding().splitFace(s_upr_bug, adj_bug);
			upr_bug.m_isSourceArc.init(upr_bug, false);
			upr_bug.m_isSourceArc[s_upr_bug->firstAdj()->theEdge()] = true;
			upr_bug.s_hat = s_upr_bug;
			upr_bug.augment();

			GraphAttributes GA_UPR_tmp(GTmp, GraphAttributes::nodeGraphics |
					GraphAttributes::edgeGraphics |
					GraphAttributes::nodeStyle |
					GraphAttributes::edgeStyle |
					GraphAttributes::nodeLabel |
					GraphAttributes::edgeLabel
					);
			GA_UPR_tmp.setAllHeight(30.0);
			GA_UPR_tmp.setAllWidth(30.0);

			uprLayout.call(upr_bug, GA_UPR_tmp);

			// label the nodes with their index
			for(node z : GA_UPR_tmp.constGraph().nodes) {
				GA_UPR_tmp.label(z) = to_string(z->index());
				GA_UPR_tmp.y(z)=-GA_UPR_tmp.y(z);
				GA_UPR_tmp.x(z)=-GA_UPR_tmp.x(z);
			}
			for(edge eee : GA_UPR_tmp.constGraph().edges) {
				DPolyline &line = GA_UPR_tmp.bends(eee);
				ListIterator<DPoint> it;
				for(it = line.begin(); it.valid(); it++) {
					(*it).m_y = -(*it).m_y;
					(*it).m_x = -(*it).m_x;
				}
			}
			GA_UPR_tmp.writeGML("c:/temp/UPR_int.gml");
#if 0
			std::cout << "face of UPR_int :" << std::endl;
			upr_bug.outputFaces(upr_bug.getEmbedding());
#endif
#endif
		}
		size_new = list.size();
		toInsert = list;
		list.clear();
	}

	/*
	 * some edges cannot be inserted, so use heuristic insertion methods
	 */
	if (!toInsert.empty()) {

#if 0
		std::cout << std::endl << "\a\a\a\a\aheuristical call!! " << std::endl;
#endif

		edge e_orig = toInsert.popFrontRet();

#if 0
		std::cout << std::endl;
		std::cout << "heuristical insertion path for e_orig :" << e_orig << ";  e_UPR: (" << m_UPR.copy(e_orig->source()) << ","
			<< m_UPR.copy(e_orig->target()) << ")" <<  std::endl;
#endif


#if 0
#if 0
		m_UPR.outputFaces(m_UPR.getEmbedding());
		m_UPR.writeGML("c:/temp/bug5.gml");
#endif

		LayerBasedUPRLayout uprLayout;
		Graph GTmp( (const Graph &) m_UPR);
		CombinatorialEmbedding embTmp(GTmp);
		node tTmp = 0;
#if 0
		GTmp.writeGML("c:/temp/bug4.gml");
#endif
		hasSingleSink(GTmp, tTmp);
		OGDF_ASSERT(tTmp != 0);
		embTmp.setExternalFace(embTmp.rightFace(tTmp->firstAdj()));
#if 0
		adjEntry adjTmp = GCTmp.copy(m_UPR.extFaceHandle->theEdge())->adjTarget();
#endif
		UpwardPlanRep upr_bug(embTmp);
		adjEntry adj_bug = upr_bug.getAdjEntry(upr_bug.getEmbedding(), upr_bug.getSuperSource(), upr_bug.getEmbedding().externalFace());
		node s_upr_bug = upr_bug.newNode();
		upr_bug.getEmbedding().splitFace(s_upr_bug, adj_bug);
		upr_bug.m_isSourceArc.init(upr_bug, false);
		upr_bug.m_isSourceArc[s_upr_bug->firstAdj()->theEdge()] = true;
		upr_bug.s_hat = s_upr_bug;
		upr_bug.augment();

		GraphAttributes GA_UPR_tmp(GTmp, GraphAttributes::nodeGraphics |
				GraphAttributes::edgeGraphics |
				GraphAttributes::nodeStyle |
				GraphAttributes::edgeStyle |
				GraphAttributes::nodeLabel |
				GraphAttributes::edgeLabel
				);
		GA_UPR_tmp.setAllHeight(30.0);
		GA_UPR_tmp.setAllWidth(30.0);

		uprLayout.call(upr_bug, GA_UPR_tmp);

		// label the nodes with their index
		for(node z : GA_UPR_tmp.constGraph().nodes) {
			GA_UPR_tmp.label(z) = to_string(z->index());
			GA_UPR_tmp.y(z)=-GA_UPR_tmp.y(z);
			GA_UPR_tmp.x(z)=-GA_UPR_tmp.x(z);
		}
		for(edge eee : GA_UPR_tmp.constGraph().edges) {
			DPolyline &line = GA_UPR_tmp.bends(eee);
			ListIterator<DPoint> it;
			for(it = line.begin(); it.valid(); it++) {
				(*it).m_y = -(*it).m_y;
				(*it).m_x = -(*it).m_x;
			}
		}
		GA_UPR_tmp.writeGML("c:/temp/UPR_int.gml");
#if 0
		std::cout << "face of UPR_int :" << std::endl;
		upr_bug.outputFaces(upr_bug.getEmbedding());
#endif
#endif

		SList<adjEntry> path;
		constraintFIP(UPR, toInsert, costOrig, e_orig, path);

#if 0
		for(adjEntry adj : path) {
			std::cout << adj->theEdge() << ";  node: " << adj->theNode() << std::endl;
		}
#endif

		UPR.insertEdgePathEmbedded(e_orig, path, costOrig);

		OGDF_ASSERT(isUpwardPlanar(UPR));

		return insertAll(UPR, toInsert, costOrig);
	}
	return Module::ReturnType::Feasible;
}



void FixedEmbeddingUpwardEdgeInserter::staticLock(
	UpwardPlanRep &UPR,
	EdgeArray<bool> &locked,
	const List<edge> &origEdges,
	edge e_orig)
{
	// construct merge graph M
	GraphCopy M((const Graph &) UPR);

	// add deleted edges to M
	for(edge e : origEdges) {
		node u = M.copy(UPR.copy(e->source()));
		node v = M.copy(UPR.copy(e->target()));
		M.newEdge(u,v);
	}

	EdgeArray<bool> markedEdges(M, false);
	markUp(M, M.copy(UPR.copy(e_orig->target())), markedEdges);
	markDown(M, M.copy(UPR.copy(e_orig->source())), markedEdges);

	for(edge e : M.edges) {
		if (markedEdges[e] && M.original(e) != nullptr) {
			locked[M.original(e)] = true;
		}
	}
}



void FixedEmbeddingUpwardEdgeInserter::getPath(
	UpwardPlanRep &UPR,
	List<edge> &origEdges,
	EdgeArray<int> &cost,
	edge e_orig,
	SList<adjEntry> &path,
	bool heuristic)
{
	path.clear();
	node x_1 = UPR.copy(e_orig->source());
	node y_1 = UPR.copy(e_orig->target());
	const CombinatorialEmbedding &Gamma = UPR.getEmbedding();

	EdgeArray<bool> locked(UPR, false);
	staticLock(UPR, locked, origEdges, e_orig);

	//locked the adjacent edges of x_1 and y_1
	for(adjEntry adjTmp : x_1->adjEntries)
		locked[adjTmp->theEdge()] = true;
	for(adjEntry adjTmp : y_1->adjEntries)
		locked[adjTmp->theEdge()] = true;

	EdgeArray<adjEntry> predAdj(UPR, nullptr); // for path reconstruction
	EdgeArray<int> dist(UPR, std::numeric_limits<int>::max()); // current distance to an edge
	EdgeArray<adjEntry> toAdjEntry(UPR, nullptr);


	/*
	* compute the adjEntry of the adjacent edges of x_1
	* this is necessary for the initiliazation of priorQ
	*/
	List<edge> outEdges;
	List<adjEntry> adjOut;
	x_1->outEdges(outEdges);
	for(edge eOut : outEdges) {
		adjEntry adj = eOut->adjSource();
		adjOut.pushBack(adj);
		if (adj->cyclicPred()->theEdge()->target() == x_1)
			adjOut.pushBack(adj->cyclicPred()); // right face of the left in edge of x_1
	}

	List<adjEntry> initEdges;
	for(adjEntry adj : adjOut) {
		feasibleEdges(UPR, Gamma.rightFace(adj), adj, locked, initEdges, heuristic);
		for(adjEntry adjInit : initEdges) {
			edge ee = adjInit->theEdge();
			if (!locked[ee]) {
				if (UPR.isSinkArc(ee) || UPR.isSourceArc(ee))
					dist[ee] = 0;
				else
					dist[ee] = 1;
				predAdj[ee] = adj;
				// mappe ee to the "correct" adjEntry
				toAdjEntry[ee] = adjInit;
			}

			//check if ee contains the target source y_1
			if (adjInit->twin()->theNode() == y_1) {
				adjEntry adjTgt = UPR.getAdjEntry(UPR.getEmbedding(), y_1, UPR.getEmbedding().rightFace(adj));

				//for the case if there are two adjEntry of y_1 which right face is the external face
				if (Gamma.rightFace(adj) == Gamma.externalFace()) {
					//we have to compute the correct adjEntry
					adjEntry tgtLeft = nullptr, tgtRight = nullptr;
					for(adjEntry runAdj : y_1->adjEntries) {
						if (Gamma.rightFace(runAdj) == Gamma.externalFace()) {
							if (runAdj->theEdge()->target() == y_1)
								tgtLeft = runAdj;
							else
								tgtRight = runAdj;
						}
					}
					if (adj->theNode() == adj->theEdge()->source())
						adjTgt = tgtRight; // *it->theEdge() is on the right face side
					else
						adjTgt = tgtLeft; // *it->theEdge() is on the left face side
				}

				if (Gamma.rightFace(adj) != Gamma.rightFace(adjTgt))
					adjTgt = adjTgt->cyclicPred();
				path.pushFront(adj);
				path.pushBack(adjTgt);

				OGDF_ASSERT(Gamma.rightFace(adj) == Gamma.rightFace(adjTgt));

				break;
			}
		}
		if (path.size() == 2) // edge can be inserted without crossing
			break; //leave for(adjEntry adj : adjOut) loop

		initEdges.clear();
	}

	// if path.size == 2 then we can insert e_orig without crossing (the path is not necessary constrain feasible)
	if (path.size() != 2) {
		// init the priority queque
		PrioritizedMapQueue<edge, int> priorQ(UPR);
		for(edge e : UPR.edges) {
			if (!locked[e]) {
				int priority = dist[e];
				priorQ.push(e, priority);
			}
		}
		adjEntry adjLast = nullptr;
		while (!priorQ.empty()) {

			adjEntry adj_cur = toAdjEntry[priorQ.topElement()];
			priorQ.pop();

			OGDF_ASSERT(adj_cur != nullptr);

			face f = Gamma.rightFace(adj_cur);	//current face f
			List<adjEntry> nextAdjs;
			feasibleEdges(UPR, f, adj_cur, locked, nextAdjs, heuristic);

			bool reached = false;
			for(adjEntry adjNext : nextAdjs) {

				if (adjNext->theNode() == y_1) { 	// y_1 reached ?

					adjLast = UPR.getAdjEntry(UPR.getEmbedding(), y_1, f);
					reached = true;

					if (Gamma.rightFace(adj_cur) == Gamma.externalFace()) {
						//we have to compute the correct adjEntry
						adjEntry tgtLeft = nullptr, tgtRight = nullptr;
						for(adjEntry runAdj : y_1->adjEntries) {
							if (Gamma.rightFace(runAdj) == Gamma.externalFace()) {
								if (runAdj->theEdge()->target() == y_1)
									tgtLeft = runAdj;
								else
									tgtRight = runAdj;
							}
						}
						if (adj_cur->theNode() == adj_cur->theEdge()->source())
							adjLast = tgtRight; // adj_cur->theEdge() is on the right face side
						else
							adjLast = tgtLeft; // adj_cur->theEdge() is on the left face side
					}

					OGDF_ASSERT(adjLast != nullptr);

					predAdj[adjLast->theEdge()] = adj_cur;
					break; // leave forall-loop
				}

				bool ok = !locked[adjNext->theEdge()];

				//use heuristic to check curent path
				if (ok && heuristic)
					ok = isConstraintFeasible(UPR, origEdges, e_orig, adj_cur, adjNext, predAdj);

				//relax if ok
				if (ok) {
					int c = 0;
					if (UPR.original(adjNext->theEdge()) != nullptr)
						c = cost[UPR.original(adjNext->theEdge())];

					int new_dist = dist[adj_cur->theEdge()] + c;
					if (dist[adjNext->theEdge()] > new_dist) {
						priorQ.decrease(adjNext->theEdge(), new_dist);
						predAdj[adjNext->theEdge()] = adj_cur;
						dist[adjNext->theEdge()] = new_dist;
						toAdjEntry[adjNext->theEdge()] = adjNext;
					}
				}
			}

			if (reached)
				break; // leave while-loop if y_1 is found
		}

		//construct the path
		path.pushBack(adjLast);
		adjEntry run = predAdj[adjLast];
		while (run != nullptr) {
			path.pushFront(run);
			run = predAdj[run];
		}
	}

	OGDF_ASSERT(path.size() >= 2);
}



bool FixedEmbeddingUpwardEdgeInserter::isConstraintFeasible(
	UpwardPlanRep &UPR,
	const List<edge> &orig_edges,
	edge e_orig,
	adjEntry adjCurrent,
	adjEntry adjNext,
	EdgeArray<adjEntry> &predAdj)
{
	//construct path to adj->theEdge()
	SList<adjEntry> path;
	path.pushBack(adjNext);
	path.pushFront(adjCurrent);
	adjEntry run = predAdj[adjCurrent];
	while (run != nullptr) {
		path.pushFront(run);
		run = predAdj[run->theEdge()];
	}

#if 0
	std::cout << std::endl << std::endl << "current insertion path: " << std::endl;
	for(adjEntry adj : path) {
		std::cout << adj->theEdge() << ";  node: " << adj->theNode() << std::endl;
	}
#endif

	GraphCopy M( (const Graph &) UPR); // merge graph

	//convert adjEntry of path to adjEntry of M
	SList<adjEntry> path_M;
	for(adjEntry a : path) {
		edge e_M = M.copy(a->theEdge());
		node v = M.copy(a->theNode());
		if (e_M->source() == v)
			path_M.pushBack(e_M->adjSource());
		else
			path_M.pushBack(e_M->adjTarget());
	}

	// construct a partial path from src to adjNext
	path_M.popFrontRet();
	node src = M.copy(UPR.copy(e_orig->source()));
	node tgt = M.copy(UPR.copy(e_orig->target()));
	while (!path_M.empty()) {
		edge eM = path_M.popFrontRet()->theEdge();
		node d = M.split(eM)->source();
		M.newEdge(src, d);
		src = d;
	}

	M.newEdge(src, tgt);
	//add the deleted edges
	for(edge eOrig : orig_edges) {
		node a = M.copy(UPR.copy(eOrig->source()));
		node b = M.copy(UPR.copy(eOrig->target()));
		M.newEdge(a, b);
	}

	return isAcyclic(M);
}


bool FixedEmbeddingUpwardEdgeInserter::isConstraintFeasible(
	UpwardPlanRep &UPR,
	List<edge> &origEdges,
	edge e_orig,
	SList<adjEntry> &path)
{
	GraphCopy GC( (const Graph &) UPR);
	GraphCopy M((const Graph &)GC); // merge graph

	//convert adjEntry of path to adjEntry of M
	SList<adjEntry> path_M;
	for(adjEntry a : path) {
		edge e_M = M.copy( GC.copy(a->theEdge()) );
		node v = M.copy(GC.copy(a->theNode()));
		if (e_M->source() == v)
			path_M.pushBack(e_M->adjSource());
		else
			path_M.pushBack(e_M->adjTarget());
	}

#if 0
	std::cout << " insertion path for " << e_orig << std::endl;
	for(adjEntry adj : path_M) {
		std::cout << adj->theEdge() << ";  node: " << adj->theNode() << std::endl;
	}
#endif

	edge e = GC.newEdge(GC.copy(UPR.copy(e_orig->source())), GC.copy(UPR.copy(e_orig->target())));

	CombinatorialEmbedding Gamma(M);
	M.insertEdgePathEmbedded(e, Gamma, path_M);

	OGDF_ASSERT(isAcyclic(M));

	//add the deleted edges
	for(edge eOrig : origEdges) {
		node a = M.copy(GC.copy(UPR.copy(eOrig->source())));
		node b = M.copy(GC.copy(UPR.copy(eOrig->target())));
		M.newEdge(a, b);
	}

	return isAcyclic(M);
}


void FixedEmbeddingUpwardEdgeInserter::feasibleEdges(
	UpwardPlanRep &UPR,
	face f,
	adjEntry adj,
	EdgeArray<bool> &locked,
	List<adjEntry> &feasible,
	bool heuristic)
{
	const CombinatorialEmbedding &Gamma = UPR.getEmbedding();

	OGDF_ASSERT(Gamma.rightFace(adj) == f);

	adjEntry start = adj, run = adj;
	if (f == Gamma.externalFace()) {
		bool stop = false;

		if (adj->theNode() == adj->theEdge()->source()) {
			/*
			 *adj->theEdge() is on the right path of f, so walk ccw
			 *all edges between adj->theEdge() and the super sink t_hat on the right path are feasible.
			 */
			do {
				if (run->theEdge()->target() == UPR.getSuperSink())
					stop = true;
				if (run != start)
					feasible.pushBack(run->twin());
				run = run->faceCycleSucc();
			} while (!stop);

			//dynamic lock; all edges between super source and adj->theEdge() of the corredponding right path muss be locked
			if (!heuristic) {
				run = start;
				stop = false;
				do {
					if (run->theEdge()->source() == UPR.getSuperSource())
						stop = true;
					locked[run->theEdge()] = true;
					run = run->faceCyclePred();
				} while (!stop);
			}
		}

		else {
			/*
			*currentEdge is on the left path of f, so walk cw
			*All edges between adj->theEdge() and the super sink t_hat on the left path are feasible.
			*/
			do {
				if (run->theEdge()->target()== UPR.getSuperSink())
					stop = true;
				if (run != start)
					feasible.pushBack(run->twin());
				run = run->faceCyclePred();
			} while (!stop);

			//dynamic lock; all edges between super source and adj->theEdge() of the corredponding left path muss be locked
			if (!heuristic) {
				run = start;
				stop = false;
				do {
					if (run->theEdge()->source() == UPR.getSuperSource())
						stop = true;
					locked[run->theEdge()] = true;
					run = run->faceCycleSucc();
				} while (!stop);
			}
		}
	}

	// internal face
	else {
		bool stop = false;

		if (adj->theNode() == adj->theEdge()->source()) {
		/*
		 *adj->theEdge() is on the left path of f; walk cw to the source-witch of f.
		 *All edges traversing by this walk are feasible.
		 */
			do {
				if (run->theEdge()->source() == run->faceCycleSucc()->theEdge()->source()) //reach source-switch of f
					stop = true;
				if (run != start)
					feasible.pushBack(run->twin());
				run = run->faceCycleSucc();
			} while (!stop);

			//dynamic lock; all edges between source-switch and adj->theEdge() of the corredponding left path muss be locked
			if (!heuristic) {
				run = start;
				stop = false;
				do {
					locked[run->theEdge()] = true;
					if (run->theEdge()->source() == run->faceCyclePred()->theEdge()->source()) //reach source-switch of f
						stop = true;
					run = run->faceCyclePred();
				} while (!stop);
			}
		}

		else {
			/*
			*adj->theEdge() is on the right path of f; walk ccw to the source-witch of f.
			*All edges traversing by this walk are feasible.
			*/
			do {
				if (run->theEdge()->source() == run->faceCyclePred()->theEdge()->source()) //reach source-switch of f
					stop = true;
				if (run != start)
					feasible.pushBack(run->twin());
				run = run->faceCyclePred();
			} while (!stop);

			//dynamic lock; all edges between source-switch and adj->theEdge() of the corredponding right path must be locked
			if (!heuristic) {
				run = start;
				stop = false;
				do {
					locked[run->theEdge()] = true;
					if (run->theEdge()->source() == run->faceCycleSucc()->theEdge()->source()) //reach source-switch of f
						stop = true;
					run = run->faceCycleSucc();
				} while (!stop);
			}
		}
	}
}


void FixedEmbeddingUpwardEdgeInserter::markUp(
	const Graph &G,
	node v,
	//NodeArray<bool> &markedNodes,
	EdgeArray<bool> &markedEdges )
{
	// traversing G from v
	Queue<node> nodesToDo;
	nodesToDo.append(v);
	NodeArray<bool> inQueue(G, false);
	while(!nodesToDo.empty()) {
		node w = nodesToDo.pop();
		List<edge> outEdges;
		w->outEdges(outEdges);
		ListIterator <edge> it;
		for (it = outEdges.begin(); it.valid(); ++it) {
			edge e = *it;
			if (!inQueue[e->target()]) { // put the next node in queue if it is not already in queue
				nodesToDo.append( e->target() );
				inQueue[e->target()] = true;
			}
			markedEdges[e] = true;	// mark edge
		}
	}
}


void FixedEmbeddingUpwardEdgeInserter::markDown(
	const Graph &G,
	node v,
#if 0
	NodeArray<bool> &markedNodes,
#endif
	EdgeArray<bool> &markedEdges)
{
	// mark the subgraph, i.e all nodes and edges, which dominate node v
	Queue<node> nodesToDo;
	nodesToDo.append(v);
	NodeArray<bool> inQueue(G, false);
	while(!nodesToDo.empty()) {
		node w = nodesToDo.pop();
		List<edge> inEdges;
		w->inEdges(inEdges);
		ListIterator <edge> it;
		for (it = inEdges.begin(); it.valid(); ++it) {
			edge e = *it;
			if (!inQueue[e->source()]) {
				nodesToDo.append(e->source() ); // put the next node in queue if it is not already in queue
				inQueue[e->source()] = true;
			}
			markedEdges[e] = true;	// mark the edge
		}
	}
}

}
