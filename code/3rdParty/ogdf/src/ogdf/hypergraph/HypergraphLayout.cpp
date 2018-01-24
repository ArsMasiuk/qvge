/** \file
 * \brief Implementation of classes HypergraphLayoutES.
 *
 * \author Ondrej Moris
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

#include <ogdf/hypergraph/Hypergraph.h>
#include <ogdf/hypergraph/HypergraphLayout.h>

#include <ogdf/packing/TileToRowsCCPacker.h>

#include <ogdf/planarity/SubgraphPlanarizer.h>
#include <ogdf/planarity/PlanarSubgraphFast.h>
#include <ogdf/planarity/SimpleEmbedder.h>
#include <ogdf/planarity/FixedEmbeddingInserter.h>

#include <ogdf/orthogonal/OrthoLayout.h>

namespace ogdf {

HypergraphLayoutES::HypergraphLayoutES()
{
	m_profile = HypergraphLayoutES::Profile::Normal;
	m_crossings = 0;
	m_ratio = 1.0;
	m_constraintIO = false;
	m_constraintPorts = false;
	SubgraphPlanarizer *crossMin = new SubgraphPlanarizer;
	crossMin->setSubgraph(new PlanarSubgraphFast<int>);
	crossMin->setInserter(new FixedEmbeddingInserter);
	m_crossingMinimizationModule.reset(crossMin);
	m_planarLayoutModule.reset(new OrthoLayout);
	m_embeddingModule.reset(new SimpleEmbedder);
}


void HypergraphLayoutES::call(HypergraphAttributes &pHA)
{
	if (pHA.constHypergraph().empty())
		return;

	HypergraphAttributesES &HA = dynamic_cast<HypergraphAttributesES &>(pHA);

	GraphCopySimple gc(HA.repGraph());
	GraphAttributes ga(gc,
	  GraphAttributes::nodeGraphics | GraphAttributes::nodeType |
	  GraphAttributes::edgeType);

	for(node v : gc.nodes) {
		node vOrig = gc.original(v);
		ga.width(v)  = HA.repGA().width(vOrig);
		ga.height(v) = HA.repGA().width(vOrig);
	}

	List<edge> fixedShell;
	if (m_constraintIO) {
		List<node> src;
		List<node> tgt;
		for(node v : gc.nodes) {
			if (HA.type(gc.original(v)) == HypernodeElement::Type::INPUT) {
				src.pushBack(v);
			} else if (HA.type(gc.original(v)) ==
				HypernodeElement::Type::OUTPUT) {
					tgt.pushBack(v);
			}
		}
		insertShell(gc, src, tgt, fixedShell);
	}

	EdgeArray<bool> forbid(gc,false);
	ListConstIterator<edge> it;
	for(it = fixedShell.begin(); it.valid(); ++it)
		forbid[*it] = true;


	if (m_constraintPorts) {
		// TODO
	}

	PlanRep planarRep(ga);

	int connectedComponentsCount = planarRep.numberOfCCs();

	Array<DPoint> bounding(connectedComponentsCount);

	// Now we planarize each connected component of planarRep separately.
	for (int i = 0; i < connectedComponentsCount; i++)
	{
		// Planarize.
		int cr;
		m_crossingMinimizationModule->call(planarRep, i, cr, nullptr, &forbid);
		m_crossings += cr;
		//planarizeCC(planarRep, fixedShell);

		// Embed.
		adjEntry adjExternal = nullptr;
		m_embeddingModule->call(planarRep, adjExternal);

		// Draw.
		Layout ccPlaneRep(planarRep);
		applyProfile(HA);
		m_planarLayoutModule->call(planarRep, adjExternal, ccPlaneRep);

		// Copy drawing of this CC into the planar representation.
		for(int j = planarRep.startNode(); j < planarRep.stopNode(); ++j) {
			node vGC = planarRep.v(j);
			node vG = gc.original(vGC);

			HA.setX(vG, ccPlaneRep.x(planarRep.copy(vGC)));
			HA.setY(vG, ccPlaneRep.y(planarRep.copy(vGC)));

			for (adjEntry adj : vG->adjEntries) {
				if ((adj->index() & 1) != 0) {
					ccPlaneRep.computePolylineClear(planarRep,
					  gc.copy(adj->theEdge()),
					  HA.bends(adj->theEdge()));
				}
			}
		}

		// Store current bounding box and the number of crossings.
		bounding[i] = m_planarLayoutModule->getBoundingBox();

	}

	// Pack all components together.
	packAllCC(planarRep, gc, HA, bounding);
}

#if 0
void HypergraphLayoutES::planarizeCC(PlanRep &ccPlanarRep,
	List<edge> &fixedShell)
{
	//int ccPlanarRepSize = ccPlanarRep.numberOfNodes();

	EdgeArray<int> costs(ccPlanarRep.original(), 1);

	List<edge> crossingEdges;
	m_planarSubgraphModule->callAndDelete
		(ccPlanarRep, fixedShell, crossingEdges);

	m_crossingMinimizationModule->call(ccPlanarRep, costs, crossingEdges);
}
#endif

void HypergraphLayoutES::packAllCC(const PlanRep &planarRep,
                                   const GraphCopySimple &gc,
                                   HypergraphAttributesES &pHA,
                                   Array<DPoint> &bounding)
{
	int componentsCount = planarRep.numberOfCCs();

	// There is only one packing module implemented in OGDF now.
	TileToRowsCCPacker packer;

	// Positions of components represent "translation" offsets.
	Array<DPoint> position(componentsCount);

	// Pack them all! (ie. compute position offsets)
	packer.call(bounding, position, m_ratio);

	// All nodes, edges or bends must be positioned according to the offset.
	for (int i = 0; i < componentsCount; i++) {
		for (int j = planarRep.startNode(i); j < planarRep.stopNode(i); ++j) {
			node vGC = planarRep.v(j);
			node vG = gc.original(vGC);

			pHA.setX(vG, pHA.x(vG) + position[i].m_x);
			pHA.setY(vG, pHA.y(vG) + position[i].m_y);

			for (adjEntry entry : vG->adjEntries) {
				for (auto &dp : pHA.bends(entry->theEdge())) {
					dp.m_x += position[i].m_x;
					dp.m_y += position[i].m_y;
				}
			}
		}
	}
}


void HypergraphLayoutES::insertShell
	(GraphCopySimple &G, List<node> &src, List<node> &tgt, List<edge> &fixedShell)
{
	OGDF_ASSERT(src.size() > 0);
	OGDF_ASSERT(tgt.size() > 0);

	node s = G.newNode();
	for (ListIterator<node> it = src.begin(); it.valid(); ++it)
		fixedShell.pushBack(G.newEdge(s, *it));

	node t = G.newNode();
	for (ListIterator<node> it = tgt.begin(); it.valid(); ++it)
		fixedShell.pushBack(G.newEdge(*it, t));

	G.newEdge(s, t);
}


void HypergraphLayoutES::removeShell(PlanRep &G, NodePair &st)
{
	G.delNode(st.source);
	G.delNode(st.target);
}


void HypergraphLayoutES::applyProfile(HypergraphAttributesES &HA)
{
	switch (m_profile) {

		case HypergraphLayoutES::Profile::Normal:
		for(node v_g : HA.repGraph().nodes) {
			HA.setWidth(v_g, 5);
			HA.setHeight(v_g, 5);
		}
		hypernode v_h;
		forall_hypernodes(v_h, HA.constHypergraph()) {
			HA.setWidth(v_h, 20);
			HA.setHeight(v_h, 20);
		}
		break;

		case HypergraphLayoutES::Profile::ElectricCircuit:

		// TODO:
		// a) all gates should be depicted

		break;
	}
}

}
