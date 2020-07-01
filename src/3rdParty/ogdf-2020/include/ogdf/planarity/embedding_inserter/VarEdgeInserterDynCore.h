/** \file
 * \brief Declaration of class VarEdgeInserterCore and VarEdgeInserterUMLCore,
 * which are the implementation classes for edge insertion with variable embedding.
 *
 * \author Carsten Gutwenger, Jan Papenfuß
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
#include <ogdf/planarity/PlanRepLight.h>
#include <ogdf/planarity/RemoveReinsertType.h>


namespace ogdf {


class OGDF_EXPORT VarEdgeInserterDynCore : public Timeouter
{
public:
	VarEdgeInserterDynCore(
		PlanRepLight &pr,
		const EdgeArray<int>      *pCostOrig,
		const EdgeArray<bool>     *pForbiddenOrig,
		const EdgeArray<uint32_t> *pEdgeSubgraphs)
		: m_pr(pr), m_pCost(pCostOrig), m_pForbidden(pForbiddenOrig), m_pSubgraph(pEdgeSubgraphs) { }

	virtual ~VarEdgeInserterDynCore() { }

	Module::ReturnType call(
		const Array<edge> &origEdges,
		RemoveReinsertType rrPost,
		double percentMostCrossed);

	int runsPostprocessing() const { return m_runsPostprocessing; }

protected:
	class BCandSPQRtrees;
	class ExpandedGraph;

	int costCrossed(edge eOrig) const;

	void insert(edge eOrig, SList<adjEntry> &eip);
	void blockInsert(node s, node t, List<adjEntry> &L);

	virtual void storeTypeOfCurrentEdge(edge eOrig) { }
	virtual BCandSPQRtrees *createBCandSPQRtrees();
	virtual ExpandedGraph *createExpandedGraph(BCandSPQRtrees &BC);

	virtual void buildSubpath(node v,
		node vPred,
		node vSucc,
		List<adjEntry> &L,
		ExpandedGraph &Exp,
		node s,
		node t);

	static const int c_bigM = 10000;
	PlanRepLight	&m_pr;

	const EdgeArray<int>		*m_pCost;
	const EdgeArray<bool>		*m_pForbidden;
	const EdgeArray<uint32_t>	*m_pSubgraph;

	BCandSPQRtrees *m_pBC;

	int m_runsPostprocessing; //!< Runs of remove-reinsert method.
};


class VarEdgeInserterDynUMLCore : public VarEdgeInserterDynCore
{
public:
	VarEdgeInserterDynUMLCore(
		PlanRepLight &pr,
		const EdgeArray<int>      *pCostOrig,
		const EdgeArray<uint32_t> *pEdgeSubgraph) : VarEdgeInserterDynCore(pr, pCostOrig, nullptr, pEdgeSubgraph) { }

protected:
	class BCandSPQRtreesUML;
	class ExpandedGraphUML;

	void storeTypeOfCurrentEdge(edge eOrig) override { m_typeOfCurrentEdge = m_pr.typeOrig(eOrig); }
	virtual BCandSPQRtrees *createBCandSPQRtrees() override;
	virtual ExpandedGraph *createExpandedGraph(BCandSPQRtrees &BC) override;
	virtual void buildSubpath(node v,
		node vPred,
		node vSucc,
		List<adjEntry> &L,
		ExpandedGraph &Exp,
		node s,
		node t) override;

	Graph::EdgeType	m_typeOfCurrentEdge;
};


}
