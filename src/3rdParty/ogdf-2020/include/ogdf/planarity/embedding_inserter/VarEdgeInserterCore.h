/** \file
 * \brief Declaration of class VarEdgeInserterCore and VarEdgeInserterUMLCore,
 * which are the implementation classes for edge insertion with variable embedding.
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
#include <ogdf/planarity/PlanRepLight.h>
#include <ogdf/planarity/RemoveReinsertType.h>
#include <ogdf/decomposition/StaticSPQRTree.h>

namespace ogdf {

class OGDF_EXPORT VarEdgeInserterCore : public Timeouter
{
public:
	VarEdgeInserterCore(
		PlanRepLight &pr,
		const EdgeArray<int>      *pCostOrig,
		const EdgeArray<bool>     *pForbiddenOrig,
		const EdgeArray<uint32_t> *pEdgeSubgraphs)
		: m_pr(pr), m_pCost(pCostOrig), m_pForbidden(pForbiddenOrig), m_pSubgraph(pEdgeSubgraphs) { }

	virtual ~VarEdgeInserterCore() { }

	Module::ReturnType call(
		const Array<edge> &origEdges,
		RemoveReinsertType rrPost,
		double percentMostCrossed);

	Module::ReturnType callPostprocessing(
		const Array<edge> &origEdges,
		RemoveReinsertType rrPost,
		double percentMostCrossed);

	int runsPostprocessing() const { return m_runsPostprocessing; }

protected:
	class BiconnectedComponent;
	class ExpandedGraph;

	void insert(node s, node t, SList<adjEntry> &eip);
	int costCrossed(edge eOrig) const;

	bool dfsVertex(node v, int parent);
	node dfsComp(int i, node parent);

	void blockInsert(
		const BiconnectedComponent &G,
		node s,
		node t,
		List<adjEntry> &L);
	bool pathSearch(node v, edge parent, List<edge> &path);
	virtual void buildSubpath(node v,
		edge eIn,
		edge eOut,
		List<adjEntry> &L,
		ExpandedGraph &Exp,
		node s,
		node t);

	virtual void storeTypeOfCurrentEdge(edge eOrig) { }
	virtual BiconnectedComponent *createBlock();
	virtual ExpandedGraph *createExpandedGraph(const BiconnectedComponent &BC, const StaticSPQRTree &T);

	static const int c_bigM = 10000;
	PlanRepLight	&m_pr;

	const EdgeArray<int>		*m_pCost;
	const EdgeArray<bool>		*m_pForbidden;
	const EdgeArray<uint32_t>	*m_pSubgraph;

	node   m_s, m_t;
	edge   m_st;
	SList<adjEntry> *m_pEip;

	// representation of BC tree
	NodeArray<SList<int> > m_compV;
	Array<SList<node> >    m_nodeB;
	Array<SList<edge> >    m_edgeB;
	NodeArray<node>        m_GtoBC;

	node m_v1, m_v2;

	int m_runsPostprocessing; //!< Runs of remove-reinsert method.
};


class VarEdgeInserterUMLCore : public VarEdgeInserterCore
{
public:
	VarEdgeInserterUMLCore(
		PlanRepLight &pr,
		const EdgeArray<int>      *pCostOrig,
		const EdgeArray<uint32_t> *pEdgeSubgraph) : VarEdgeInserterCore(pr, pCostOrig, nullptr, pEdgeSubgraph) { }

protected:
	class BiconnectedComponentUML;
	class ExpandedGraphUML;

	void storeTypeOfCurrentEdge(edge eOrig) override { m_typeOfCurrentEdge = m_pr.typeOrig(eOrig); }
	BiconnectedComponent *createBlock() override;
	ExpandedGraph *createExpandedGraph(const BiconnectedComponent &BC, const StaticSPQRTree &T) override;
	virtual void buildSubpath(node v,
		edge eIn,
		edge eOut,
		List<adjEntry> &L,
		ExpandedGraph &Exp,
		node s,
		node t) override;

	Graph::EdgeType	m_typeOfCurrentEdge;
};

}
