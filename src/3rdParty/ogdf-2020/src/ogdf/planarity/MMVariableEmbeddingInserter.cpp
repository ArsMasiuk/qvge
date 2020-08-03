/** \file
 * \brief implementation of MMVariableEmbeddingInserter class
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
#include <ogdf/planarity/MMVariableEmbeddingInserter.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/decomposition/StaticPlanarSPQRTree.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/fileformats/GraphIO.h>

//#define MMC_OUTPUT

namespace ogdf {

static int globalCounter = 0;

// sets default values for options
MMVariableEmbeddingInserter::MMVariableEmbeddingInserter()
{
	m_rrOption = RemoveReinsertType::None;
	m_percentMostCrossed = 25;
}

void outputPG(const PlanRepExpansion &PG, int i)
{
	GraphAttributes AG(PG, GraphAttributes::nodeLabel);

	for(node v : PG.nodes) {
		AG.label(v) = to_string(v->index());
	}

	string str = string("PG_") + to_string(i) + ".gml";
	GraphIO::write(AG, str, GraphIO::writeGML);
}

void MMVariableEmbeddingInserter::writeEip(const List<Crossing> &eip)
{
	for (const auto &cr : eip) {
		if(cr.m_adj == nullptr) {
			std::cout << "nil {";
			std::cout << cr.m_partitionLeft;
			std::cout << "} {";
			std::cout << cr.m_partitionRight;
			std::cout << "}";

		} else
			std::cout << cr.m_adj;
		std::cout << std::endl;
	}
}

// actual call (called by all variations of call)
//   crossing of generalizations is forbidden if forbidCrossingGens = true
//   edge costs are obeyed if costOrig != 0
Module::ReturnType MMVariableEmbeddingInserter::doCall(
	PlanRepExpansion &PG,
	const List<edge> &origEdges,
	const EdgeArray<bool> *forbiddenEdgeOrig)
{
	ReturnType retValue = ReturnType::Feasible;

	if (origEdges.size() == 0)
		return ReturnType::Optimal;  // nothing to do

	m_pPG = &PG;
	m_forbiddenEdgeOrig = forbiddenEdgeOrig;

	SListPure<edge> rrEdges;
	if(removeReinsert() != RemoveReinsertType::None) {
		for(edge e : PG.edges)
			rrEdges.pushBack(PG.originalEdge(e));

		for(edge eOrig : origEdges)
			rrEdges.pushBack(eOrig);
	}

	m_pSources = new NodeSet<>(PG);
	m_pTargets = new NodeSet<>(PG);

#ifdef MMC_OUTPUT
	outputPG(PG,0);
	int ii = 0;
#endif

	// insertion of edges
	for (edge eOrig : origEdges) {
		node srcOrig = eOrig->source();
		node tgtOrig = eOrig->target();

#ifdef MMC_OUTPUT
		std::cout << "insert " << srcOrig << " -> " << tgtOrig << std::endl;
#endif

		node oldSrc = (PG.splittableOrig(srcOrig) && PG.expansion(srcOrig).size() == 1) ?
			PG.expansion(srcOrig).front() : nullptr;
		node oldTgt = (PG.splittableOrig(tgtOrig) && PG.expansion(tgtOrig).size() == 1) ?
			PG.expansion(tgtOrig).front() : nullptr;

		anchorNodes(eOrig->source(), *m_pSources);
		anchorNodes(eOrig->target(), *m_pTargets);

		node vDummy = commonDummy(*m_pSources, *m_pTargets);
		node src, tgt;
		edge eExtraSrc = nullptr, eExtraTgt = nullptr;
		List<Crossing> eip;

		if(vDummy == nullptr) {
			AnchorNodeInfo vStart, vEnd;
			insert(eip, vStart, vEnd);

#ifdef MMC_OUTPUT
			writeEip(eip);
#endif
			preprocessInsertionPath(vStart,vEnd,srcOrig,tgtOrig,src,tgt,eExtraSrc,eExtraTgt);

#ifdef MMC_OUTPUT
			std::cout << "from: " << src << "  to: " << tgt << std::endl;
#endif

		} else {
			insertWithCommonDummy(eOrig, vDummy, src, tgt);
#ifdef MMC_OUTPUT
			std::cout << "(direct) from: " << src << "  to: " << tgt << std::endl;
#endif
		}

		PG.insertEdgePath(eOrig, nullptr, src, tgt, eip, eExtraSrc, eExtraTgt);

#ifdef MMC_OUTPUT
		++ii;
		std::cout << std::endl;
		outputPG(PG,ii);
#endif

		m_pSources->clear();
		m_pTargets->clear();

		if(oldSrc != nullptr && PG.expansion(srcOrig).size() > 1)
			contractSplitIfReq(oldSrc);
		if(oldTgt != nullptr && PG.expansion(tgtOrig).size() > 1)
			contractSplitIfReq(oldTgt);

#if 0
		PG.consistencyCheck();
		OGDF_ASSERT(isPlanar(PG));
#endif
	}

	if(removeReinsert() != RemoveReinsertType::None) {
		// postprocessing (remove-reinsert heuristc)
#if 0
		SListPure<edge> rrEdges;

		switch(removeReinsert())
		{
		case RemoveReinsertType::All:
		case RemoveReinsertType::MostCrossed:
			{
				const List<node> &origInCC = PG.nodesInCC();

				for (node vG : origInCC) {
					for(adjEntry adj : vG->adjEntries) {
						if ((adj->index() & 1) == 0) continue;
						edge eG = adj->theEdge();
						rrEdges.pushBack(eG);
					}
				}
			}
			break;

		case RemoveReinsertType::Inserted:
			{
				for (edge eOrig : origEdges) {
					rrEdges.pushBack(eOrig);
				}
			}
			break;
		}

		// marks the end of the interval of rrEdges over which we iterate
		// initially set to invalid iterator which means all edges
		SListConstIterator<edge> itStop;
#endif

		bool improved;
		do {
			improved = false;

#if 0
			if(removeReinsert() == RemoveReinsertType::MostCrossed)
			{
				embedding_inserter::CrossingsBucket<PlanRepExpansion> bucket(&PG);
				rrEdges.bucketSort(bucket);

				const int num = int(0.01 * percentMostCrossed() * G.numberOfEdges());
				itStop = rrEdges.get(num);
			}
#endif

			for(edge eOrig: rrEdges) {
				globalCounter++;
				node srcOrig = eOrig->source();
				node tgtOrig = eOrig->target();

				int oldCrossings = PG.chain(eOrig).size() - 1;
				if (oldCrossings == 0) continue; // cannot improve

				node oldSrc = nullptr, oldTgt = nullptr;
				PG.removeEdgePath(eOrig,nullptr,oldSrc,oldTgt);
				// PG.consistencyCheck();

				// try to find a better insertion path
				anchorNodes(eOrig->source(), *m_pSources);
				anchorNodes(eOrig->target(), *m_pTargets);

				node vDummy = commonDummy(*m_pSources, *m_pTargets);
				node src, tgt;
				edge eExtraSrc = nullptr, eExtraTgt = nullptr;

				List<Crossing> eip;
				if(vDummy == nullptr) {
					AnchorNodeInfo vStart, vEnd;
					insert(eip, vStart, vEnd);
					preprocessInsertionPath(vStart,vEnd,srcOrig,tgtOrig,src,tgt,eExtraSrc,eExtraTgt);

				} else {
					insertWithCommonDummy(eOrig, vDummy, src, tgt);
				}

				int newCrossings = eip.size();

				OGDF_ASSERT(isPlanar(PG));
				PG.insertEdgePath(eOrig, nullptr, src, tgt, eip, eExtraSrc, eExtraTgt);
#ifdef OGDF_DEBUG
				PG.consistencyCheck();
#endif
				OGDF_ASSERT(isPlanar(PG));

				m_pSources->clear();
				m_pTargets->clear();

				if(PG.splittable(oldSrc))
					contractSplitIfReq(oldSrc);
				if(PG.splittable(oldTgt))
					contractSplitIfReq(oldTgt);

#ifdef OGDF_DEBUG
				PG.consistencyCheck();
#endif
				OGDF_ASSERT(isPlanar(PG));

				//int newPathLength = PG.chain(eOrig).size() - 1;
				int saved = oldCrossings - newCrossings;
				OGDF_ASSERT(saved >= 0);

				if(saved > 0)
					improved = true;
			}

			if(removeReinsert() == RemoveReinsertType::All)
			{
				// process all node splits
				int nsCount = PG.nodeSplits().size();
				ListIterator<PlanRepExpansion::NodeSplit> itS, itSNext;
				for(itS = PG.nodeSplits().begin(); itS.valid() && nsCount > 0; itS = itSNext, --nsCount)
				{
					globalCounter++;
					PlanRepExpansion::NodeSplit *ns = &(*itS);

					int oldCrossings = ns->m_path.size() - 1;
					if (oldCrossings == 0) {
						itSNext = itS.succ();
						PG.contractSplit(ns);
						continue; // cannot improve
					}

					node vOrig = PG.original(ns->source());

					node oldSrc = nullptr, oldTgt = nullptr;
					PG.removeEdgePath(nullptr,ns,oldSrc,oldTgt);
#ifdef OGDF_DEBUG
					PG.consistencyCheck();
#endif

					// try to find a better insertion path
					findSourcesAndTargets(oldSrc,oldTgt,*m_pSources,*m_pTargets);

					node vCommon = commonDummy(*m_pSources, *m_pTargets);

					if(vCommon == nullptr) {
						node src, tgt;
						edge eExtraSrc = nullptr, eExtraTgt = nullptr;

						List<Crossing> eip;
						AnchorNodeInfo vStart, vEnd;
						insert(eip, vStart, vEnd);
						preprocessInsertionPath(vStart,vEnd,vOrig,vOrig,src,tgt,eExtraSrc,eExtraTgt);
						PG.insertEdgePath(nullptr, ns, src, tgt, eip, eExtraSrc, eExtraTgt);

#ifdef OGDF_DEBUG
						PG.consistencyCheck();
#endif

						m_pSources->clear();
						m_pTargets->clear();

						if(PG.splittable(oldSrc))
							contractSplitIfReq(oldSrc);
						if(PG.splittable(oldTgt))
							contractSplitIfReq(oldTgt);

#ifdef OGDF_DEBUG
						PG.consistencyCheck();
#endif
						OGDF_ASSERT(isPlanar(PG));

						//int newCrossings = ns->m_path.size() - 1;
						int newCrossings = eip.size();
						int saved = oldCrossings - newCrossings;
						OGDF_ASSERT(saved >= 0);

						if(saved > 0)
							improved = true;

						itSNext = itS.succ();
						if(ns->m_path.size() == 1)
							PG.contractSplit(ns);

					} else {
						m_pSources->clear();
						m_pTargets->clear();

						improved = true;
						itSNext = itS.succ();

						convertDummy(vCommon,vOrig,ns);
#ifdef OGDF_DEBUG
						PG.consistencyCheck();
#endif
						OGDF_ASSERT(isPlanar(PG));
					}
				}
			}

		} while(improved);
	}

	delete m_pSources;
	delete m_pTargets;

#ifdef OGDF_DEBUG
	bool isPlanar =
#endif
		PG.embed();
	OGDF_ASSERT(isPlanar);
	OGDF_ASSERT(PG.representsCombEmbedding());

	return retValue;
}


void MMVariableEmbeddingInserter::insert(
	List<Crossing> &eip,
	AnchorNodeInfo &vStart,
	AnchorNodeInfo &vEnd)
{
	PlanRepExpansion &PG = *m_pPG;
	eip.clear();

	// compute biconnected components of PG
	EdgeArray<int> compnum(PG);
	int c = biconnectedComponents(PG,compnum);

	m_compV.init(PG);
	m_nodeB.init(c);

	// edgeB[i] = list of edges in component i
	m_edgeB.init(c);
	for(edge e : PG.edges)
		m_edgeB[compnum[e]].pushBack(e);

	// construct arrays compV and nodeB such that
	// m_compV[v] = list of components containing v
	// m_nodeB[i] = list of vertices in component i
	NodeArray<bool> mark(PG,false);

	int i;
	for(i = 0; i < c; ++i) {
		for (edge e : m_edgeB[i]) {
			if (!mark[e->source()]) {
				mark[e->source()] = true;
				m_nodeB[i].pushBack(e->source());
			}
			if (!mark[e->target()]) {
				mark[e->target()] = true;
				m_nodeB[i].pushBack(e->target());
			}
		}

		for (node v : m_nodeB[i]) {
			m_compV[v].pushBack(i);
			mark[v] = false;
		}
	}

	mark.init();
	m_GtoBC.init(PG,nullptr);

	m_conFinished = false;
	dfsVertex(m_pTargets->nodes().front(), -1, eip, vStart, vEnd);

	// deallocate resources used by insert()
	m_GtoBC.init();
	m_edgeB.init();
	m_nodeB.init();
	m_compV.init();
}


node MMVariableEmbeddingInserter::prepareAnchorNode(
	const AnchorNodeInfo &anchor,
	node vOrig,
	bool isSrc,
	edge &eExtra)
{
	PlanRepExpansion &PG = *m_pPG;

	adjEntry adj = nullptr;
	node vStraight = nullptr;

	edge eStraight;
	PlanRepExpansion::NodeSplit *nsStraight;
	if(anchor.m_adj_2 != nullptr) {
		adj = anchor.m_adj_1;
#if 0
		if(PG.originalEdge(adj->theEdge()) == PG.originalEdge(anchor.m_adj_2->theEdge()) &&
			PG.nodeSplitOf(adj->theEdge()) == PG.nodeSplitOf(anchor.m_adj_2->theEdge()))
		{
			node u = adj->theNode();
			for(adjEntry adj : u->adjEntries) {
				List<edge> *pathStraight = &PG.setOrigs(adj->theEdge(), eStraight, nsStraight);
				vStraight = pathStraight->front()->source();
				if(PG.original(vStraight) == vOrig)
					break;
				vStraight = pathStraight->back()->target();
				if(PG.original(vStraight)== vOrig)
					break;
			}

			PG.removeCrossingReuseDummy(adj, vStraight);

			return u;
		}
#endif

		List<edge> *pathStraight = &PG.setOrigs(adj->theEdge(), eStraight, nsStraight);

		vStraight = pathStraight->front()->source();
		if(PG.original(vStraight) != vOrig) {
			vStraight = pathStraight->back()->target();
			if(PG.original(vStraight) != vOrig) {
				adj = anchor.m_adj_2;
				pathStraight = &PG.setOrigs(adj->theEdge(), eStraight, nsStraight);
				vStraight = pathStraight->front()->source();
				if(PG.original(vStraight) != vOrig) {
					vStraight = pathStraight->back()->target();
				}
			}
		}

		if(PG.original(vStraight) != vOrig) {
			node u = adj->theNode();
			adjEntry adjA[2];
			int i = 0;
			for(adjEntry adj_x : u->adjEntries) {
				if(adj_x != anchor.m_adj_1 && adj_x != anchor.m_adj_2)
					adjA[i++] = adj_x;
			}

			List<edge> *pathStraightAdjA = &PG.setOrigs(adjA[0]->theEdge(), eStraight, nsStraight);
			vStraight = pathStraightAdjA->front()->source();
			if(PG.original(vStraight) != vOrig)
				vStraight = pathStraightAdjA->back()->target();
			OGDF_ASSERT(PG.original(vStraight) == vOrig);

			eExtra = PG.separateDummy(adjA[0], adjA[1], vStraight, isSrc);
			return u;
		}

	} else {
		// Should be the correct adj... (case S-node, dummy)
		adj = anchor.m_adj_1;
		List<edge> *pathStraight = &PG.setOrigs(adj->theEdge(), eStraight, nsStraight);

		if((eStraight && eStraight->source() != vOrig && eStraight->target() != vOrig) ||
			(nsStraight && PG.original(nsStraight->source()) != vOrig))
		{
			node vDummy = adj->theNode();
			edge eStraightOld = eStraight;
			PlanRepExpansion::NodeSplit *nsStraightOld = nsStraight;

			for(adjEntry adjRun : vDummy->adjEntries) {
				pathStraight = &PG.setOrigs(adjRun->theEdge(), eStraight, nsStraight);
				if((eStraightOld && eStraight != eStraightOld) || (nsStraightOld && nsStraight != nsStraightOld))
					break;
			}
		}

		vStraight = pathStraight->front()->source();
		if(PG.original(vStraight) != vOrig)
			vStraight = pathStraight->back()->target();
	}

	OGDF_ASSERT(PG.original(vStraight) == vOrig);
	eExtra = nullptr;

	if(PG.original(adj->twinNode()) == vOrig) {
		// No need for a split; can just directly go to a split node
		return adj->twinNode();

	} else {
		edge e = adj->theEdge();
		if(nsStraight == nullptr) {
			// We split a chain of an original edge
			PG.enlargeSplit(vStraight, e);
			return e->target();

		} else {
			// We split a node split
			PG.splitNodeSplit(e);
			return e->target();
		}
	}
}

void MMVariableEmbeddingInserter::preprocessInsertionPath(
	const AnchorNodeInfo &srcInfo,
	const AnchorNodeInfo &tgtInfo,
	node srcOrig,
	node tgtOrig,
	node &src,
	node &tgt,
	edge &eSrc,
	edge &eTgt)
{
	PlanRepExpansion &PG = *m_pPG;

	src = srcInfo.m_adj_1->theNode();
	if(PG.original(src) == nullptr)
		src = prepareAnchorNode(srcInfo,srcOrig,true,eSrc);

	tgt = tgtInfo.m_adj_1->theNode();
	if(PG.original(tgt) == nullptr)
		tgt = prepareAnchorNode(tgtInfo,tgtOrig,false,eTgt);
}


node MMVariableEmbeddingInserter::preparePath(
	node vAnchor,
	adjEntry adjPath,
	bool bOrigEdge,
	node vOrig)
{
	PlanRepExpansion &PG = *m_pPG;

	if(PG.original(adjPath->twinNode()) == vOrig) {
		// No need for a split; can just directly go to a split node
		return adjPath->twinNode();

	} else {
		edge e = adjPath->theEdge();
		if(bOrigEdge) {
			// We split a chain of an original edge
			PG.enlargeSplit(vAnchor, e);
		} else {
			// We split a node split
			PG.splitNodeSplit(e);
		}
		return e->target();
	}
}


void MMVariableEmbeddingInserter::findPseudos(
	node vDummy,
	adjEntry adjSrc,
	AnchorNodeInfo &infoSrc,
	SListPure<node> &pseudos)
{
	PlanRepExpansion &PG = *m_pPG;

	ListConstIterator<edge> it = PG.position(adjSrc->theEdge());
	node w;
	if((*it)->source() == vDummy) {
		edge e = *it;
		while(PG.isPseudoCrossing(w = e->target())) {
			pseudos.pushBack(w);
			++it; e = *it;
		}

		infoSrc.m_adj_1 = e->adjTarget();
		infoSrc.m_adj_2 = (adjSrc->cyclicSucc() == (*PG.position(adjSrc->theEdge()).pred())->adjTarget()) ?
			infoSrc.m_adj_1->cyclicSucc() : infoSrc.m_adj_1->cyclicPred();

	} else {
		edge e = *it;
		while(PG.isPseudoCrossing(w = e->source())) {
			pseudos.pushBack(w);
			--it; e = *it;
		}

		infoSrc.m_adj_1 = e->adjSource();
		infoSrc.m_adj_2 = (adjSrc->cyclicPred() == (*PG.position(adjSrc->theEdge()).succ())->adjSource()) ?
			infoSrc.m_adj_1->cyclicPred() : infoSrc.m_adj_1->cyclicSucc();
	}

}


void MMVariableEmbeddingInserter::insertWithCommonDummy(
	edge eOrig,
	node vDummy,
	node &src,
	node &tgt)
{
	PlanRepExpansion &PG = *m_pPG;
#ifdef OGDF_DEBUG
	bool isPlanar =
#endif
		PG.embed();
	OGDF_ASSERT(isPlanar);

	node vSrc = nullptr, vTgt = nullptr;
	adjEntry adjSrc = nullptr, adjTgt = nullptr;
	bool bOrigEdgeSrc = true, bOrigEdgeTgt = true;

	for(adjEntry adj : vDummy->adjEntries) {
		edge e = adj->theEdge();
		edge eStraight;
		PlanRepExpansion::NodeSplit *nsStraight;
		List<edge> &pathStraight = PG.setOrigs(e, eStraight, nsStraight);

		node vAnchor;
		if(vDummy == e->source())
			vAnchor = pathStraight.back()->target();
		else
			vAnchor = pathStraight.front()->source();

		node vOrig = PG.original(vAnchor);
		if(vOrig == eOrig->source()) {
			vSrc = vAnchor;
			adjSrc = adj;
			bOrigEdgeSrc = (eStraight != nullptr);
		} else if(vOrig == eOrig->target()) {
			vTgt = vAnchor;
			adjTgt = adj;
			bOrigEdgeTgt = (eStraight != nullptr);
		}
	}

	OGDF_ASSERT(vSrc != nullptr);
	OGDF_ASSERT(vTgt != nullptr);
	OGDF_ASSERT(adjSrc != nullptr);
	OGDF_ASSERT(adjTgt != nullptr);

	if(adjSrc == adjTgt->cyclicPred() || adjSrc == adjTgt->cyclicSucc())
	{
		src = preparePath(vSrc, adjSrc, bOrigEdgeSrc, eOrig->source());
		tgt = preparePath(vTgt, adjTgt, bOrigEdgeTgt, eOrig->target());

	} else {
		SListPure<node> pseudos;
		AnchorNodeInfo infoSrc;
		AnchorNodeInfo infoTgt;

		findPseudos(vDummy,adjSrc,infoSrc,pseudos);
		findPseudos(vDummy,adjTgt,infoTgt,pseudos);

		for (auto pseudo : pseudos) {
			PG.resolvePseudoCrossing(pseudo);
		}

		edge eExtra = nullptr;
		src = infoSrc.m_adj_1->theNode();
		if(PG.original(src) == nullptr)
			src = prepareAnchorNode(infoSrc,eOrig->source(),true,eExtra);
		OGDF_ASSERT(eExtra == nullptr);

		tgt = infoTgt.m_adj_1->theNode();
		if(PG.original(tgt) == nullptr)
			tgt = prepareAnchorNode(infoTgt,eOrig->target(),false,eExtra);
		OGDF_ASSERT(eExtra == nullptr);


#if 0
		adjEntry adjTgt_2 = adjTgt->cyclicSucc()->cyclicSucc();
		OGDF_ASSERT(PG.nodeSplitOf(adjTgt->theEdge()) == PG.nodeSplitOf(adjTgt_2->theEdge()));
		OGDF_ASSERT(PG.originalEdge(adjTgt->theEdge()) == PG.originalEdge(adjTgt_2->theEdge()));

		edge e = PG.insertBySepDummy(eOrig, vSrc, vTgt, adjSrc, adjTgt, adjTgt_2);
		//separateDummy(adjTgt, adjTgt_2, vTgt, false);
		PG.assignOrig(e,eOrig);
#endif
	}
}

// Block represents a block of the original graph
class MMVariableEmbeddingInserter::Block : public Graph
{
public:
	// constructor
	explicit Block(PlanRepExpansion &PG) : m_PG(PG)
	{
		m_adjBCtoG    .init(*this,nullptr);
		m_forbidden   .init(*this,false);
		m_vBCtoG      .init(*this,nullptr);
		m_isSource    .init(*this,false);
		m_isTarget    .init(*this,false);
		m_isSplittable.init(*this,false);
		m_pT = nullptr;
	}

	~Block() { delete m_pT; }

	// avoid automatic generation of assignment operator
	Block &operator=(const Block &);

	node containsSource(node v) const;
	node containsTarget(node v) const;

	adjEntry containsSourceAdj(node v) const;
	adjEntry containsTargetAdj(node v) const;

	PlanRepExpansion &m_PG;
	StaticPlanarSPQRTree *m_pT;

	AdjEntryArray<adjEntry> m_adjBCtoG;
	EdgeArray<bool>         m_forbidden;
	NodeArray<node>         m_vBCtoG;

	NodeArray<bool> m_isSource;
	NodeArray<bool> m_isTarget;
	NodeArray<bool> m_isSplittable;
};


node MMVariableEmbeddingInserter::Block::containsSource(node v) const
{
	const Skeleton &S = m_pT->skeleton(v);
	const Graph &M = S.getGraph();

	for(node w : M.nodes) {
		node wOrig = S.original(w);
		if(m_isSource[wOrig])
			return wOrig;
	}

	return nullptr;
}

node MMVariableEmbeddingInserter::Block::containsTarget(node v) const
{
	const Skeleton &S = m_pT->skeleton(v);
	const Graph &M = S.getGraph();

	for(node w : M.nodes) {
		node wOrig = S.original(w);
		if(m_isTarget[wOrig])
			return wOrig;
	}

	return nullptr;
}

adjEntry MMVariableEmbeddingInserter::Block::containsSourceAdj(node v) const
{
	const Skeleton &S = m_pT->skeleton(v);
	const Graph &M = S.getGraph();

	node wOrig = nullptr;
	for (node w : M.nodes) {
		wOrig = S.original(w);
		if(m_isSource[wOrig])
			break;
	}

	if(wOrig == nullptr) return nullptr;

	for(adjEntry adj : wOrig->adjEntries) {
		if(m_pT->skeletonOfReal(adj->theEdge()).treeNode() == v)
			return adj;
	}

	return wOrig->firstAdj();
}

adjEntry MMVariableEmbeddingInserter::Block::containsTargetAdj(node v) const
{
	const Skeleton &S = m_pT->skeleton(v);
	const Graph &M = S.getGraph();

	node wOrig = nullptr;
	for(node w : M.nodes) {
		wOrig = S.original(w);
		if(m_isTarget[wOrig])
			break;
	}

	if(wOrig == nullptr) return nullptr;

	for(adjEntry adj : wOrig->adjEntries) {
		if(m_pT->skeletonOfReal(adj->theEdge()).treeNode() == v)
			return adj;
	}

	return wOrig->firstAdj();
}


// ExpandedSkeleton represents the (partially) expanded skeleton with
// its dual graph (search network)
class MMVariableEmbeddingInserter::ExpandedSkeleton
{
	Block &m_BC;

	NodeArray<node> m_GtoExp;
	List<node>      m_nodesG;
	Graph           m_exp;   // expanded graph
	AdjEntryArray<adjEntry> m_expToG;
	edge            m_eS, m_eT; // (virtual) edges in exp representing s and t (if any)

	ConstCombinatorialEmbedding m_E;  //!< Embedding of expanded graph.

	Graph               m_dual;       //!< Search network of expanded skeleton.
	NodeArray<node>     m_primalNode; //!< The node in PG corresponding to dual node (0 if face).
	EdgeArray<adjEntry> m_primalAdj;  //!< The adjacency entry in primal graph corresponding to edge in dual.
	EdgeArray<int>      m_dualCost;   //!< The cost of an edge in the seach network.

	node m_startEdge;
	node m_startSource;
	node m_startTarget;
	node m_endEdge;
	node m_endSource;
	node m_endTarget;


public:
	explicit ExpandedSkeleton(Block &BC) :
		m_BC(BC),
		m_GtoExp(BC.m_pT->originalGraph(),nullptr),
		m_expToG(m_exp,nullptr),
		m_primalNode(m_dual,nullptr),
		m_primalAdj(m_dual,nullptr),
		m_dualCost(m_dual,0) { }

	void expand(node v, edge eIn, edge eOut);

	void constructDual(bool bPathToEdge, bool bPathToSrc, bool bPathToTgt);

	void findShortestPath(bool &bPathToEdge, bool &bPathToSrc, bool &bPathToTgt, Paths &paths);

	// avoid automatic generation of assignment operator
	ExpandedSkeleton &operator=(const ExpandedSkeleton &);

private:
	edge insertEdge(node vG, node wG, edge eG);
	void expandSkeleton(node v, edge e1, edge e2);

	static void addOutgoingEdges(node v, SListPure<edge> &edges);
	PathType reconstructInsertionPath(
		node v,
		AnchorNodeInfo &m_srcInfo,
		AnchorNodeInfo &m_tgtInfo,
		List<Crossing> &crossed,
		SList<adjEntry> &addLeft,
		SList<adjEntry> &addRight,
		NodeArray<edge> &spPred);
};


// build expanded graph (by expanding skeleton(v), edges eIn and eOut
// are the adjacent tree edges on the path from v1 to v2
void MMVariableEmbeddingInserter::ExpandedSkeleton::expand(
	node v, edge eIn, edge eOut)
{
	m_exp.clear();

	while (!m_nodesG.empty())
		m_GtoExp[m_nodesG.popBackRet()] = nullptr;

	const StaticSPQRTree &T = *m_BC.m_pT;
	const Skeleton &S = T.skeleton(v);

	m_eS = nullptr;
	if (eIn != nullptr) {
		edge eInS = (v != eIn->source()) ? T.skeletonEdgeTgt(eIn) :
			T.skeletonEdgeSrc(eIn);
		node x = S.original(eInS->source()), y = S.original(eInS->target());
		m_eS = insertEdge(x,y,nullptr);
	}

	m_eT = nullptr;
	if (eOut != nullptr) {
		edge eOutS = (v != eOut->source()) ? T.skeletonEdgeTgt(eOut) :
			T.skeletonEdgeSrc(eOut);
		node x = S.original(eOutS->source()), y = S.original(eOutS->target());
		m_eT = insertEdge(x,y,nullptr);
	}

	expandSkeleton(v, eIn, eOut);

	planarEmbed(m_exp);
	m_E.init(m_exp);
}


// insert edge in exp (from a node corresponding to vG in G to a node
// corresponding to wG)
edge MMVariableEmbeddingInserter::ExpandedSkeleton::insertEdge(
	node vG, node wG, edge eG)
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


// expand one skeleton (recursive construction)
void MMVariableEmbeddingInserter::ExpandedSkeleton::expandSkeleton(
	node v, edge e1, edge e2)
{
	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_BC.m_pT->skeleton(v));
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
				expandSkeleton((v == eT->source()) ? eT->target() : eT->source(),
					eT,nullptr);
			}
		}
	}
}


// construct augmented dual graph (search network)
void MMVariableEmbeddingInserter::ExpandedSkeleton::constructDual(
	bool bPathToEdge, bool bPathToSrc, bool bPathToTgt)
{
	m_dual.clear();

	// insert a node in the dual graph for each face in E
	FaceArray<node> dualOfFace(m_E);

	for(face f : m_E.faces)
		dualOfFace[f] = m_dual.newNode();

	SListPure<node> sources, targets;

	// insert a node in the dual graph for each splittable node in PG
	NodeArray<node> dualOfNode(m_exp,nullptr);

	for(node vBC : m_nodesG) {
		node v = m_GtoExp[vBC];

		bool addDualNode = m_BC.m_isSplittable[vBC];

		if(m_BC.m_isSource[vBC]) {
			sources.pushBack(v);
			addDualNode = true;
		}
		if(m_BC.m_isTarget[vBC]) {
			targets.pushBack(v);
			addDualNode = true;
		}

		if(addDualNode)
			m_primalNode[dualOfNode[v] = m_dual.newNode()] = v;
	}

	// Insert an edge into the dual graph for each adjacency entry in E.
	// The edges are directed from the left face to the right face.
	for(node v : m_exp.nodes)
	{
		node vDual = dualOfNode[v];

		for(adjEntry adj : v->adjEntries)
		{
			// cannot cross virtual edge representing sources / targets
			adjEntry adjBC = m_expToG[adj];
			if(adjBC == nullptr)
				continue;

			node vLeft  = dualOfFace[m_E.leftFace (adj)];
			node vRight = dualOfFace[m_E.rightFace(adj)];

			if(!m_BC.m_forbidden[adjBC->theEdge()]) {
				edge e = m_dual.newEdge(vLeft,vRight);
				m_primalAdj[e] = adj;
				m_dualCost [e] = 1;
			}

			if(vDual) {
#if 0
				if(m_eT == 0 || (v != m_eT->source() && v != m_eT->target()))
#endif
				{
					edge eOut = m_dual.newEdge(vDual,vLeft);
					m_primalAdj[eOut] = adj;
					m_dualCost [eOut]  = 0;
				}

				if(m_eS == nullptr ||
					((!bPathToSrc || v != m_eS->source()) && (!bPathToTgt || v != m_eS->target())))
				{
					edge eIn = m_dual.newEdge(vLeft,vDual);
					m_primalAdj[eIn] = adj;
					m_dualCost [eIn]  = 1;
				}
			}
		}
	}

	m_startEdge = (bPathToEdge) ? m_dual.newNode() : nullptr;
	if(m_eS != nullptr) {
		if(m_startEdge) {
			m_dual.newEdge(m_startEdge, dualOfFace[m_E.rightFace(m_eS->adjSource())]);
			m_dual.newEdge(m_startEdge, dualOfFace[m_E.rightFace(m_eS->adjTarget())]);
		}

		m_startSource = (bPathToSrc) ? dualOfNode[m_eS->source()] : nullptr;
		m_startTarget = (bPathToTgt) ? dualOfNode[m_eS->target()] : nullptr;

	} else {
		m_startSource = m_startTarget = nullptr;
		for (node source : sources) {
			m_dual.newEdge(m_startEdge, dualOfNode[source]);
		}
	}

	m_endEdge = m_dual.newNode();
	if(m_eT != nullptr) {
		m_dual.newEdge(dualOfFace[m_E.rightFace(m_eT->adjSource())], m_endEdge);
		m_dual.newEdge(dualOfFace[m_E.rightFace(m_eT->adjTarget())], m_endEdge);

		m_endSource = dualOfNode[m_eT->source()];
		m_endTarget = dualOfNode[m_eT->target()];

	} else {
		m_endSource = m_endTarget = nullptr;
		for (node target : targets) {
			m_dual.newEdge(dualOfNode[target], m_endEdge);
		}
	}
}


// finds shortest paths in the search network
void MMVariableEmbeddingInserter::ExpandedSkeleton::addOutgoingEdges(
	node v, SListPure<edge> &edges)
{
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if(e->target() != v)
			edges.pushBack(e);
	}
}

MMVariableEmbeddingInserter::PathType
	MMVariableEmbeddingInserter::ExpandedSkeleton::reconstructInsertionPath(
		node v,
		AnchorNodeInfo &srcInfo,
		AnchorNodeInfo &tgtInfo,
		List<Crossing> &crossed,
		SList<adjEntry> &addLeft,
		SList<adjEntry> &addRight,
		NodeArray<edge> &spPred)
{
	// handle last node on path split first
	if(v == m_endEdge) {
		edge eDual = spPred[v];
		v = eDual->source();
		if(m_primalNode[v] != nullptr) {
			eDual = spPred[v];
			// hier Ziel speichern: m_expToG[m_primalAdj[eDual]]->theNode()
			//tgt = m_expToG[m_primalAdj[eDual]]->theNode();
			tgtInfo.m_adj_1 = m_expToG[m_primalAdj[eDual]];
			tgtInfo.m_adj_2 = m_expToG[m_primalAdj[eDual]->cyclicPred()];

			OGDF_ASSERT(tgtInfo.m_adj_1 != nullptr);

			v = eDual->source();
		}

	} else {
		edge eDual = spPred[v];

		if(eDual != nullptr) {
			adjEntry adj_1 = m_primalAdj[eDual];
			Crossing &cr = *crossed.pushFront(Crossing());

			adjEntry adj;
			for(adj = adj_1; adj->theEdge() != m_eT; adj = adj->cyclicSucc()) {
				adjEntry adjBC = m_expToG[adj];
				if(adjBC != nullptr)
					cr.m_partitionLeft.pushBack(adjBC);
				//OGDF_ASSERT(m_expToG[adj] != 0);
			}
			for(adj = adj->cyclicSucc(); adj != adj_1; adj = adj->cyclicSucc()) {
				adjEntry adjBC = m_expToG[adj];
				if(adjBC != nullptr)
					cr.m_partitionRight.pushBack(adjBC);
				//OGDF_ASSERT(m_expToG[adj] != 0);
			}

			v = eDual->source();

		} else {
			node vExp = m_primalNode[v];
			OGDF_ASSERT(m_eS->source() == vExp || m_eS->target() == vExp);
			OGDF_ASSERT(m_eT->source() == vExp || m_eT->target() == vExp);
			adjEntry adjS = (m_eS->source() == vExp) ? m_eS->adjSource() : m_eS->adjTarget();
			adjEntry adjT = (m_eT->source() == vExp) ? m_eT->adjSource() : m_eT->adjTarget();
			OGDF_ASSERT(adjS->theNode() == vExp);
			OGDF_ASSERT(adjT->theNode() == vExp);

			adjEntry adj;
			for(adj = adjS->cyclicSucc(); adj != adjT; adj = adj->cyclicSucc()) {
				addLeft.pushBack(m_expToG[adj]);
				OGDF_ASSERT(m_expToG[adj] != nullptr);
			}
			for(adj = adj->cyclicSucc(); adj != adjS; adj = adj->cyclicSucc()) {
				addRight.pushBack(m_expToG[adj]);
				OGDF_ASSERT(m_expToG[adj] != nullptr);
			}
		}
	}

	while(v != m_startEdge && v != m_startSource && v != m_startTarget) {
		OGDF_ASSERT(m_primalNode[v] == nullptr);

		edge eDual = spPred[v];
		node w = eDual->source();

		if(m_primalNode[w] == nullptr) {
			// w is a face node
			adjEntry adjExp = m_primalAdj[spPred[v]];
			if(adjExp != nullptr)
				crossed.pushFront(Crossing(m_expToG[adjExp]));

		} else {
			edge eDual2 = spPred[w];

			if(eDual2 != nullptr) {
				adjEntry adj_1 = m_primalAdj[eDual2];
				adjEntry adj_2 = m_primalAdj[eDual];

				w = eDual2->source();
				if(adj_1 != nullptr) {
					Crossing &cr = *crossed.pushFront(Crossing());

					adjEntry adj;
					for(adj = adj_1; adj != adj_2; adj = adj->cyclicSucc()) {
						adjEntry adjBC = m_expToG[adj];
						if(adjBC != nullptr)
							cr.m_partitionLeft.pushBack(adjBC);
						//OGDF_ASSERT(m_expToG[adj] != 0);
					}
					for(; adj != adj_1; adj = adj->cyclicSucc()) {
						adjEntry adjBC = m_expToG[adj];
						if(adjBC != nullptr)
							cr.m_partitionRight.pushBack(adjBC);
						//OGDF_ASSERT(m_expToG[adj] != 0);
					}

				} else {
					// speichere Startkonoten hier:
					//src = m_expToG[m_primalAdj[eDual]]->theNode();
					srcInfo.m_adj_1 = m_expToG[m_primalAdj[eDual]];
					srcInfo.m_adj_2 = m_expToG[m_primalAdj[eDual]->cyclicPred()];

					OGDF_ASSERT(srcInfo.m_adj_1 != nullptr);
				}

			} else {
				adjEntry adj_2 = m_primalAdj[eDual];

				adjEntry adj;
				for(adj = adj_2; adj->theEdge() != m_eS; adj = adj->cyclicSucc()) {
					adjEntry adjBC = m_expToG[adj];
					if(adjBC != nullptr)
						addLeft.pushBack(adjBC);
					//OGDF_ASSERT(m_expToG[adj] != 0);
				}
				for(adj = adj->cyclicSucc(); adj != adj_2; adj = adj->cyclicSucc()) {
					adjEntry adjBC = m_expToG[adj];
					if(adjBC != nullptr)
						addRight.pushBack(adjBC);
					//OGDF_ASSERT(m_expToG[adj] != 0);
				}
			}
		}

		v = w;
	}

	if(v == m_startEdge)
		return PathType::pathToEdge;
	else if (v == m_startSource)
		return PathType::pathToSource;
	else
		return PathType::pathToTarget;
}

void MMVariableEmbeddingInserter::ExpandedSkeleton::findShortestPath(
	bool &bPathToEdge,
	bool &bPathToSrc,
	bool &bPathToTgt,
	Paths &paths)
{
	const int maxCost = 2;

	Array<SListPure<edge> > nodesAtDist(maxCost);
	NodeArray<edge> spPred(m_dual,nullptr);

	// start edges
	if(m_startEdge)
		addOutgoingEdges(m_startEdge, nodesAtDist[0]);
	if(m_startSource)
		addOutgoingEdges(m_startSource, nodesAtDist[0]);
	if(m_startTarget)
		addOutgoingEdges(m_startTarget, nodesAtDist[0]);

	bool vEdgeReached = false;
	bool vSourceReached = (m_endSource == nullptr || m_endSource == m_startSource || m_endSource == m_startTarget);
	bool vTargetReached = (m_endTarget == nullptr || m_endTarget == m_startSource || m_endTarget == m_startTarget);

	// actual search (using extended bfs on directed dual)
	int currentDist = 0;

	for( ; ; )
	{
		// next candidate edge
		while(nodesAtDist[currentDist % maxCost].empty())
			++currentDist;

		edge eCand = nodesAtDist[currentDist % maxCost].popFrontRet();
		node v = eCand->target();

		// leads to an unvisited node?
		if (spPred[v] == nullptr)
		{
			// yes, then we set v's predecessor in search tree
			spPred[v] = eCand;
			if(v == m_endEdge  ) vEdgeReached   = true;
			if(v == m_endSource) vSourceReached = true;
			if(v == m_endTarget) vTargetReached = true;

			auto updatePred = [&](PathType pt, node n) {
				int index = static_cast<int>(pt);
				paths.m_pred[index] = reconstructInsertionPath(n,
				                                               paths.m_src[index],
				                                               paths.m_tgt[index],
				                                               paths.m_paths[index],
				                                               paths.m_addPartLeft[index],
				                                               paths.m_addPartRight[index],
				                                               spPred);
			};

			// all targets reached?
			if (vEdgeReached && vSourceReached && vTargetReached)
			{
				updatePred(PathType::pathToEdge, m_endEdge);
				if(m_endSource != nullptr) {
					updatePred(PathType::pathToSource, m_endSource);
				}
				if(m_endTarget != nullptr) {
					updatePred(PathType::pathToSource, m_endTarget);
				}
				break;
			}

			// append next candidate edges to queue
			// (all edges leaving v)
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();
				if (v != e->source()) continue;

				int listPos = (currentDist + m_dualCost[e]) % maxCost;
				nodesAtDist[listPos].pushBack(e);
			}
		}
	}

	int lenEdge = paths.m_paths[static_cast<int>(PathType::pathToEdge)].size();
	int lenSrc  = paths.m_paths[static_cast<int>(PathType::pathToSource)].size();
	int lenTgt  = paths.m_paths[static_cast<int>(PathType::pathToTarget)].size();
	OGDF_ASSERT(m_endSource == nullptr || lenSrc >= lenEdge);
	OGDF_ASSERT(m_endTarget == nullptr || lenTgt >= lenEdge);

	//bPathToEdge = ((m_endSource == 0 || lenEdge <= lenSrc) && (m_endTarget == 0 || lenEdge <= lenTgt));
	bPathToEdge = true;
	bPathToSrc  = (m_endSource != nullptr && lenSrc == lenEdge);
	bPathToTgt  = (m_endTarget != nullptr && lenTgt == lenEdge);
}


bool MMVariableEmbeddingInserter::dfsVertex(
	node v,
	int parent,
	List<Crossing> &eip,
	AnchorNodeInfo &vStart,
	AnchorNodeInfo &vEnd)
{
	// forall biconnected components containing v (except predecessor parent)
	for (int i : m_compV[v]) {
		if (i == parent) continue;

		node repS; // representative of s in B(i)
		if (dfsBlock(i, v, repS, eip, vStart, vEnd)) { // path found?
			if(m_conFinished) return true;

			// build graph BC of biconnected component B(i)
			SList<node> nodesG;
			Block BC(*m_pPG);

			for (edge e : m_edgeB[i]) {
				node vSrc = e->source();
				if (m_GtoBC[vSrc] == nullptr) {
					BC.m_vBCtoG[m_GtoBC[vSrc] = BC.newNode()] = vSrc;
					nodesG.pushBack(vSrc);
					if(m_pSources->isMember(vSrc) || vSrc == repS)
						BC.m_isSource[m_GtoBC[vSrc]] = true;
					if(m_pTargets->isMember(vSrc) || vSrc == v)
						BC.m_isTarget[m_GtoBC[vSrc]] = true;
					BC.m_isSplittable[m_GtoBC[vSrc]] = m_pPG->splittable(vSrc);
				}

				node vTgt = e->target();
				if (m_GtoBC[vTgt] == nullptr) {
					BC.m_vBCtoG[m_GtoBC[vTgt] = BC.newNode()] = vTgt;
					nodesG.pushBack(vTgt);
					if(m_pSources->isMember(vTgt) || vTgt == repS)
						BC.m_isSource[m_GtoBC[vTgt]] = true;
					if(m_pTargets->isMember(vTgt) || vTgt == v)
						BC.m_isTarget[m_GtoBC[vTgt]] = true;
					BC.m_isSplittable[m_GtoBC[vTgt]] = m_pPG->splittable(vTgt);
				}

				edge eBC = BC.newEdge(m_GtoBC[vSrc],m_GtoBC[vTgt]);
				BC.m_adjBCtoG[eBC->adjSource()] = e->adjSource();
				BC.m_adjBCtoG[eBC->adjTarget()] = e->adjTarget();

				if(m_forbiddenEdgeOrig != nullptr) {
					edge eOrig = m_pPG->originalEdge(e);
					if(eOrig != nullptr)
						BC.m_forbidden[eBC] = (*m_forbiddenEdgeOrig)[eOrig];
				}
			}

			AnchorNodeInfo srcInfo = repS->firstAdj();
			AnchorNodeInfo tgtInfo = v->firstAdj();

			// less than 3 nodes requires no crossings (cannot build SPQR-tree
			// for a graph with less than 3 nodes!)
			if (nodesG.size() >= 3) {
				// call biconnected case
				List<Crossing> L;
				blockInsert(BC, L, srcInfo, tgtInfo);

				srcInfo.m_adj_1 = BC.m_adjBCtoG[srcInfo.m_adj_1];
				if(srcInfo.m_adj_2 != nullptr)
					srcInfo.m_adj_2 = BC.m_adjBCtoG[srcInfo.m_adj_2];
				tgtInfo.m_adj_1 = BC.m_adjBCtoG[tgtInfo.m_adj_1];
				if(tgtInfo.m_adj_2 != nullptr)
					tgtInfo.m_adj_2 = BC.m_adjBCtoG[tgtInfo.m_adj_2];

				// transform crossed edges to edges in G
				for (Crossing &cr : L) {
					if (cr.m_adj != nullptr) {
						cr.m_adj = BC.m_adjBCtoG[cr.m_adj];
					}
					for (adjEntry &adj : cr.m_partitionLeft) {
						adj = BC.m_adjBCtoG[adj];
					}
					for (adjEntry &adj : cr.m_partitionRight) {
						adj = BC.m_adjBCtoG[adj];
					}

#if 0
					adjEntry adj1 = BC.m_BCtoG[cr.m_adj1];
					adjEntry adj2 = (cr.m_adj1 == 0) ? 0 : BC.m_BCtoG[cr.m_adj1];
					eip.pushBack(Crossing(adj1, adj2));
#endif
				}
				eip.conc(L);
			}

			if(m_pSources->isMember(srcInfo.m_adj_1->theNode()))
				vStart = srcInfo;

			if(m_pTargets->isMember(tgtInfo.m_adj_1->theNode())) {
				vEnd = tgtInfo;
				m_conFinished = true;
			}

			// set entries of GtoBC back to nil (GtoBC allocated only once
			// in insert()!)
			for (node w : nodesG) {
				m_GtoBC[w] = nullptr;
			}

			return true; // path found
		}
	}

	return false; // path not found
}


bool MMVariableEmbeddingInserter::dfsBlock(int i,
	node parent,
	node &repS,
	List<Crossing> &eip,
	AnchorNodeInfo &vStart,
	AnchorNodeInfo &vEnd)
{
	// forall nodes in biconected component B(i) (except predecessor parent)
	for (node rep : m_nodeB[i]) {
		repS = rep;
		if (repS == parent) continue;
		if (m_pSources->isMember(repS)) { // s found?
			return true;
		}

		if (dfsVertex(repS, i, eip, vStart, vEnd)) {
			return true; // path found
		}
	}

	return false; // path not found
}


// find optimal edge insertion path for biconnected graph G
bool MMVariableEmbeddingInserter::pathSearch(
	node v,
	edge parent,
	const Block &BC,
	List<edge> &path)
{
	if(BC.containsTarget(v) != nullptr)
		return true;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if(e == parent) continue;
		if(pathSearch(e->opposite(v), e, BC, path)) {
			path.pushFront(e);
			return true;
		}
	}

	return false;
}

void MMVariableEmbeddingInserter::blockInsert(
	Block &BC,
	List<Crossing> &L,
	AnchorNodeInfo &srcInfo,
	AnchorNodeInfo &tgtInfo)
{
	L.clear();
	srcInfo = tgtInfo = AnchorNodeInfo();

	// construct SPQR-tree
	BC.m_pT = new StaticPlanarSPQRTree(BC);
	const StaticSPQRTree &T = *BC.m_pT;
	const Graph &tree = T.tree();

	node v1 = nullptr, vx = nullptr;
	for(node v : tree.nodes) {
		if(BC.containsSource(v) != nullptr) {
			if(v1 == nullptr)
				v1 = v;
			if(T.typeOf(v) == SPQRTree::NodeType::RNode && BC.containsTarget(v) != nullptr) {
				vx = v; break;
			}
		}
	}

	List<edge> path;
	if(vx == nullptr) {
		// find path in tree connecting a nodes whose skeleton contains and
		// source and a target, resp.
		pathSearch(v1,nullptr,BC,path);

		// remove unnecessary allocation nodes of sources from start of path
		node v;
		while(!path.empty() && BC.containsSource(v = path.front()->opposite(v1)) != nullptr)
		{
			v1 = v;
			path.popFront();
		}
	} else
		v1 = vx;

	// call build_subpath for every R-node building the list of crossed edges/nodes
	ExpandedSkeleton exp(BC);

	bool bPathToEdge = true;
	bool bPathToSrc  = false;
	bool bPathToTgt  = false;
	Array<Paths> pathsInSks(path.size()+1);
	int i = 0;

	switch(T.typeOf(v1)) {
		case SPQRTree::NodeType::RNode:
			buildSubpath(v1, nullptr,
				(path.empty()) ? nullptr : path.front(),
				pathsInSks[i++],
				bPathToEdge,
				bPathToSrc,
				bPathToTgt,
				exp);
			break;

		case SPQRTree::NodeType::PNode:
			break;

		case SPQRTree::NodeType::SNode:
			srcInfo.m_adj_1 = BC.containsSourceAdj(v1);
			break;
	}

	node v = v1;
	for (ListConstIterator<edge> it = path.begin(); it.valid(); ++it) {
		edge e = *it;
		v = e->opposite(v);

		switch(T.typeOf(v)) {
			case SPQRTree::NodeType::RNode:
				buildSubpath(v, e, it.succ().valid() ? *it.succ() : nullptr, pathsInSks[i++], bPathToEdge, bPathToSrc, bPathToTgt, exp);
				break;

			case SPQRTree::NodeType::PNode:
				break;

			case SPQRTree::NodeType::SNode:
				if(it.succ().valid()) {
					edge eNext = *it.succ();

					edge e_1 = (v == e    ->target()) ? T.skeletonEdgeTgt(e    ) : T.skeletonEdgeSrc(e    );
					edge e_2 = (v == eNext->target()) ? T.skeletonEdgeTgt(eNext) : T.skeletonEdgeSrc(eNext);

					bool bPathToSrcOld = bPathToSrc;
					bool bPathToTgtOld = bPathToTgt;

					Paths &p = pathsInSks[i];
					if(e_2->source() == e_1->source() && bPathToSrcOld) {
						p.m_pred[static_cast<int>(PathType::pathToSource)] = PathType::pathToSource;
						bPathToSrc = true;
					} else if (e_2->source() == e_1->target() && bPathToTgtOld) {
						p.m_pred[static_cast<int>(PathType::pathToSource)] = PathType::pathToTarget;
						bPathToSrc = true;
					} else
						bPathToSrc = false;

					if(e_2->target() == e_1->target() && bPathToTgtOld) {
						p.m_pred[static_cast<int>(PathType::pathToTarget)] = PathType::pathToTarget;
						bPathToTgt = true;
					} else if (e_2->target() == e_1->source() && bPathToSrcOld) {
						p.m_pred[static_cast<int>(PathType::pathToTarget)] = PathType::pathToSource;
						bPathToTgt = true;
					} else
						bPathToTgt = false;

					i++;
				}
				break;
		}
	}

	if(T.typeOf(v) == SPQRTree::NodeType::SNode)
		tgtInfo.m_adj_1 = BC.containsTargetAdj(v);
		//tgtInfo.m_adj_1 = BC.containsTarget(v)->firstAdj();

	if(i == 0) return;

	// construct list of crossings L
	int currentPath = static_cast<int>(PathType::pathToEdge);
	AnchorNodeInfo &x = pathsInSks[i-1].m_tgt[currentPath];
	if(x.m_adj_1 != nullptr)
		tgtInfo = x;
	SList<adjEntry> *pAddLeft  = nullptr;
	SList<adjEntry> *pAddRight = nullptr;

	while(--i >= 0) {
		List<Crossing> &eip = pathsInSks[i].m_paths[currentPath];

		if(currentPath > 0) {
			if(eip.empty()) {
				pathsInSks[i].m_addPartLeft [currentPath].conc(*pAddLeft );
				pathsInSks[i].m_addPartRight[currentPath].conc(*pAddRight);

			} else {
				Crossing &cr = *eip.rbegin();
				cr.m_partitionLeft .conc(*pAddLeft );
				cr.m_partitionRight.conc(*pAddRight);
			}
		}

		L.concFront(eip);

		pAddLeft  = &pathsInSks[i].m_addPartLeft [currentPath];
		pAddRight = &pathsInSks[i].m_addPartRight[currentPath];
		if(i == 0) {
			AnchorNodeInfo &ani = pathsInSks[0].m_src[currentPath];
			if(ani.m_adj_1 != nullptr)
				srcInfo = ani;
			OGDF_ASSERT(srcInfo.m_adj_1 != nullptr);
		}

		currentPath = static_cast<int>(pathsInSks[i].m_pred[currentPath]);
	}
}


// find the shortest path from represent. of s to represent. of t in
// the dual of the (partially) expanded skeleton of v
void MMVariableEmbeddingInserter::buildSubpath(
	node v,
	edge eIn,
	edge eOut,
	Paths &paths,
	bool &bPathToEdge,
	bool &bPathToSrc,
	bool &bPathToTgt,
	ExpandedSkeleton &Exp)
{
	// build expanded graph Exp
	Exp.expand(v,eIn,eOut);

	// construct augmented dual of expanded graph
	Exp.constructDual(bPathToEdge,bPathToSrc,bPathToTgt);

	// find shortest path in augmented dual
	Exp.findShortestPath(bPathToEdge, bPathToSrc, bPathToTgt, paths);
}


void MMVariableEmbeddingInserter::contractSplitIfReq(node u)
{
	if(u->degree() == 2) {
		edge eContract = u->firstAdj()->theEdge();
		edge eExpand   = u->lastAdj ()->theEdge();
		if(m_pPG->nodeSplitOf(eContract) == nullptr)
			std::swap(eContract, eExpand);

		if(m_pPG->nodeSplitOf(eContract) != nullptr) {
			edge e = m_pPG->unsplitExpandNode(u,eContract,eExpand);

			if(e->isSelfLoop())
				m_pPG->removeSelfLoop(e);
		}
	}
}


void MMVariableEmbeddingInserter::convertDummy(
	node u,
	node vOrig,
	PlanRepExpansion::nodeSplit ns_0)
{
	PlanRepExpansion::nodeSplit ns_1 = m_pPG->convertDummy(u,vOrig,ns_0);

	if(ns_0->m_path.size() == 1)
		m_pPG->contractSplit(ns_0);
	if(ns_1->m_path.size() == 1)
		m_pPG->contractSplit(ns_1);
}


void MMVariableEmbeddingInserter::collectAnchorNodes(
	node v,
	NodeSet<> &nodes,
	const PlanRepExpansion::NodeSplit *nsParent) const
{
	if(m_pPG->original(v) != nullptr)
		nodes.insert(v);

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		const PlanRepExpansion::NodeSplit *ns = m_pPG->nodeSplitOf(e);
		if(ns == nullptr) {
			// add dummy nodes of non-node-split edge
			ListConstIterator<edge> it = m_pPG->chain(m_pPG->originalEdge(e)).begin();
			for(++it; it.valid(); ++it)
				nodes.insert((*it)->source());

		} else if(ns != nsParent) {
			// add dummy nodes of node-split edge
			ListConstIterator<edge> it = ns->m_path.begin();
			for(++it; it.valid(); ++it)
				nodes.insert((*it)->source());

			node w = (v == e->source()) ? ns->target() : ns->source();
			collectAnchorNodes(w, nodes, ns);
		}
	}
}


void MMVariableEmbeddingInserter::findSourcesAndTargets(
	node src, node tgt,
	NodeSet<> &sources,
	NodeSet<> &targets) const
{
	collectAnchorNodes(src, sources, nullptr);
	collectAnchorNodes(tgt, targets, nullptr);
}


void MMVariableEmbeddingInserter::anchorNodes(
	node vOrig,
	NodeSet<> &nodes) const
{
	node vFirst = m_pPG->expansion(vOrig).front();
	if(m_pPG->splittableOrig(vOrig))
		collectAnchorNodes(vFirst, nodes, nullptr);
	else
		nodes.insert(vFirst);
}


node MMVariableEmbeddingInserter::commonDummy(
	NodeSet<> &sources,
	NodeSet<> &targets)
{
	for (node v : sources.nodes()) {
		if (targets.isMember(v)) {
			return v;
		}
	}

	return nullptr;
}

}
