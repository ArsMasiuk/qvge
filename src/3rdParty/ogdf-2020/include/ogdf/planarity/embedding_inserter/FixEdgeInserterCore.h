/** \file
 * \brief Declaration of class FixEdgeInserterCore and FixEdgeInserterUMLCore,
 * which are the implementation classes for edge insertion with fixed embedding.
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

#pragma once

#include <ogdf/basic/Timeouter.h>
#include <ogdf/basic/Module.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/FaceSet.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/planarity/PlanRepLight.h>
#include <ogdf/planarity/RemoveReinsertType.h>

namespace ogdf {

class OGDF_EXPORT FixEdgeInserterCore : public Timeouter
{
public:
	FixEdgeInserterCore(
		PlanRepLight &pr,
		const EdgeArray<int>      *pCostOrig,
		const EdgeArray<bool>     *pForbiddenOrig,
		const EdgeArray<uint32_t> *pEdgeSubgraphs)
		: m_pr(pr), m_pCost(pCostOrig), m_pForbidden(pForbiddenOrig), m_pSubgraph(pEdgeSubgraphs) { }

	virtual ~FixEdgeInserterCore() { }

	Module::ReturnType call(
		const Array<edge> &origEdges,
		bool keepEmbedding,
		RemoveReinsertType rrPost,
		double percentMostCrossed);

	int runsPostprocessing() const { return m_runsPostprocessing; }

protected:
	int getCost(edge e, int stSubGraph) const;
	void findShortestPath(const CombinatorialEmbedding &E, edge eOrig, SList<adjEntry> &crossed);
	void findWeightedShortestPath(const CombinatorialEmbedding &E, edge eOrig, SList<adjEntry> &crossed);

	int costCrossed(edge eOrig) const;
	void insertEdge(CombinatorialEmbedding &E, edge eOrig, const SList<adjEntry> &crossed);
	void removeEdge(CombinatorialEmbedding &E, edge eOrig);

	virtual void storeTypeOfCurrentEdge(edge eOrig) { }
	virtual void init(CombinatorialEmbedding &E);
	virtual void cleanup();
	virtual void constructDual(const CombinatorialEmbedding &E);

	virtual void appendCandidates(QueuePure<edge> &queue, node v);
	virtual void appendCandidates(Array<SListPure<edge> > &nodesAtDist, EdgeArray<int> &costDual, int maxCost, node v, int currentDist);

	virtual void insertEdgesIntoDual(const CombinatorialEmbedding &E, adjEntry adjSrc);
	virtual void insertEdgesIntoDualAfterRemove(const CombinatorialEmbedding &E, face f);

	PlanRepLight	&m_pr;

	const EdgeArray<int>		*m_pCost;
	const EdgeArray<bool>		*m_pForbidden;
	const EdgeArray<uint32_t>	*m_pSubgraph;

	Graph m_dual;   //!< (Extended) dual graph, constructed/destructed during call.

	EdgeArray<adjEntry> m_primalAdj;   //!< Adjacency entry in primal graph corresponding to edge in dual.
	FaceArray<node>     m_nodeOf;      //!< The node in dual corresponding to face in primal.

	FaceSet<false> *m_delFaces;
	FaceSet<false> *m_newFaces;

	node m_vS; //!< The node in extended dual representing s.
	node m_vT; //!< The node in extended dual representing t.

	int m_runsPostprocessing; //!< Runs of remove-reinsert method.
};


class FixEdgeInserterUMLCore : public FixEdgeInserterCore
{
public:
	FixEdgeInserterUMLCore(
		PlanRepLight &pr,
		const EdgeArray<int>      *pCostOrig,
		const EdgeArray<uint32_t> *pEdgeSubgraph) : FixEdgeInserterCore(pr, pCostOrig, nullptr, pEdgeSubgraph) { }

protected:
	void storeTypeOfCurrentEdge(edge eOrig) override { m_typeOfCurrentEdge = m_pr.typeOrig(eOrig); }
	void init(CombinatorialEmbedding &E) override;
	void cleanup() override;
	void constructDual(const CombinatorialEmbedding &E) override;

	void appendCandidates(QueuePure<edge> &queue, node v) override;
	void appendCandidates(Array<SListPure<edge> > &nodesAtDist, EdgeArray<int> &costDual, int maxCost, node v, int currentDist) override;

	void insertEdgesIntoDual(const CombinatorialEmbedding &E, adjEntry adjSrc) override;
	void insertEdgesIntoDualAfterRemove(const CombinatorialEmbedding &E, face f) override;

	EdgeArray<bool> m_primalIsGen; //!< true iff corresponding primal edge is a generalization.
	Graph::EdgeType	m_typeOfCurrentEdge;
};

}
