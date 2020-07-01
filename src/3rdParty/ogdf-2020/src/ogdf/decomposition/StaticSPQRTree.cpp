/** \file
 * \brief Implements classes StaticSkeleton and StaticSPQRTree
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


#include <ogdf/decomposition/StaticSPQRTree.h>


namespace ogdf {

StaticSkeleton::StaticSkeleton(const StaticSPQRTree *T, node vT) : Skeleton(vT), m_owner(T)
{
	m_orig.init(m_M,nullptr);
	m_real.init(m_M,nullptr);
	m_treeEdge.init(m_M,nullptr);
}


const SPQRTree &StaticSkeleton::owner() const
{
	return *m_owner;
}


edge StaticSkeleton::twinEdge (edge e) const
{
	edge et = m_treeEdge[e];
	if (et == nullptr)
		return nullptr;

	return (et->source() == m_treeNode) ?
		m_owner->m_skEdgeTgt[et] : m_owner->m_skEdgeSrc[et];
}


node StaticSkeleton::twinTreeNode (edge e) const
{
	edge et = m_treeEdge[e];
	if (et == nullptr)
		return nullptr;
	return et->opposite(m_treeNode);
}


//
// initialization: builds tree, skeleton graphs and cross references
//
void StaticSPQRTree::init(edge eRef)
{
	Triconnectivity tricComp(*m_pGraph);
	init(eRef,tricComp);
}

void StaticSPQRTree::init(edge eRef, Triconnectivity &tricComp)
{
	m_cpV = nullptr;
	const GraphCopySimple &GC = *tricComp.m_pGC;

	m_type.init(m_tree, NodeType::SNode);
	m_sk.init(m_tree,nullptr);

	m_skEdgeSrc.init(m_tree,nullptr);
	m_skEdgeTgt.init(m_tree,nullptr);

	NodeArray<node> mapV(GC,nullptr);
	ArrayBuffer<node> inMapV(GC.numberOfNodes());

	EdgeArray<node> partnerNode(GC,nullptr);
	EdgeArray<edge> partnerEdge(GC,nullptr);

	m_numS = m_numP = m_numR = 0;

	for (int i = 0; i < tricComp.m_numComp; i++) {
		const Triconnectivity::CompStruct &C = tricComp.m_component[i];

		if (C.m_edges.empty()) continue;

		node vT = m_tree.newNode();

		switch(C.m_type) {
		case Triconnectivity::CompType::bond:
			m_type[vT] = NodeType::PNode;
			m_numP++; break;

		case Triconnectivity::CompType::polygon:
			m_type[vT] = NodeType::SNode;
			m_numS++; break;

		case Triconnectivity::CompType::triconnected:
			m_type[vT] = NodeType::RNode;
			m_numR++; break;
		}

		m_sk[vT] = new StaticSkeleton(this,vT);
		StaticSkeleton &S = *m_sk[vT];

		for(edge e : C.m_edges)
		{
			edge eG  = GC.original(e);

			node uGC = e->source(), vGC = e->target();
			node uM = mapV[uGC], vM = mapV[vGC];

			if (uM == nullptr) {
				uM = mapV[uGC] = S.m_M.newNode();
				inMapV.push(uGC);
				S.m_orig[uM] = GC.original(uGC);
			}
			if (vM == nullptr) {
				vM = mapV[vGC] = S.m_M.newNode();
				inMapV.push(vGC);
				S.m_orig[vM] = GC.original(vGC);
			}

			// normalize direction of virtual edges
			if(eG == nullptr && GC.original(vGC) < GC.original(uGC))
				std::swap(uM, vM);

			edge eM  = S.m_M.newEdge(uM,vM);

			if (eG == nullptr) {
				if (partnerNode[e] == nullptr) {
					partnerNode[e] = vT;
					partnerEdge[e] = eM;

				} else {
					edge eT = m_tree.newEdge(partnerNode[e],vT);
					StaticSkeleton &pS = *m_sk[partnerNode[e]];
					pS.m_treeEdge[partnerEdge[e]] = S.m_treeEdge[eM] = eT;
					m_skEdgeSrc[eT] = partnerEdge[e];
					m_skEdgeTgt[eT] = eM;
				}

			} else {
				S.m_real[eM] = eG;
				m_copyOf[eG] = eM;
				if (eG->source() != S.original(eM->source()))
					S.m_M.reverseEdge(eM);
				m_skOf  [eG] = &S;
			}
		}

		while(!inMapV.empty())
			mapV[inMapV.popRet()] = nullptr;
	}

	rootTreeAt(eRef);
}


//
// destructor: deletes skeleton graphs
//
StaticSPQRTree::~StaticSPQRTree()
{
	for (node vT : m_tree.nodes)
		delete m_sk[vT];

	delete m_cpV;
}


List<node> StaticSPQRTree::nodesOfType(NodeType t) const
{
	List<node> L;
	for (node v : m_tree.nodes)
		if (m_type[v] == t) L.pushBack(v);

	return L;
}


//
// rooting of tree at edge e
node StaticSPQRTree::rootTreeAt(edge e)
{
	m_rootEdge = e;
	m_rootNode = m_skOf[e]->treeNode();

	m_sk[m_rootNode]->m_referenceEdge = m_copyOf[e];
	rootRec(m_rootNode,nullptr);

	return m_rootNode;
}


node StaticSPQRTree::rootTreeAt(node v)
{
	m_rootEdge = nullptr;
	m_rootNode = v;

	m_sk[m_rootNode]->m_referenceEdge = nullptr;
	rootRec(m_rootNode,nullptr);

	return m_rootNode;
}


void StaticSPQRTree::rootRec(node v, edge eFather)
{
	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();

		if (e == eFather) continue;

		node w = e->target();
		if (w == v) {
			m_tree.reverseEdge(e);
			std::swap(m_skEdgeSrc[e], m_skEdgeTgt[e]);
			w = e->target();
		}

		m_sk[w]->m_referenceEdge = m_skEdgeTgt[e];
		rootRec(w,e);
	}
}

}
