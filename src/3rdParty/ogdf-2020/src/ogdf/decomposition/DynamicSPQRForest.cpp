/** \file
 * \brief Implementation of class DynamicSPQRForest
 *
 * \author Jan Papenfuß
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


#include <ogdf/decomposition/DynamicSPQRForest.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/graphalg/Triconnectivity.h>


namespace ogdf {


void DynamicSPQRForest::init()
{
	m_bNode_SPQR.init(m_B, nullptr);
	m_bNode_numS.init(m_B, 0);
	m_bNode_numP.init(m_B, 0);
	m_bNode_numR.init(m_B, 0);
	m_tNode_type.init(m_T, TNodeType::SComp);
	m_tNode_owner.init(m_T);
	m_tNode_hRefEdge.init(m_T);
	m_tNode_hEdges.init(m_T);
	m_tNode_isMarked.init(m_T, false);
	m_hEdge_position.init(m_H);
	m_hEdge_tNode.init(m_H);
	m_hEdge_twinEdge.init(m_H, nullptr);
	m_htogc.init(m_H);
}

void DynamicSPQRForest::createSPQR(node vB) const
{
	Graph GC;
	NodeArray<node> origNode(GC, nullptr);
	EdgeArray<edge> origEdge(GC, nullptr);

	for (edge eH : m_bNode_hEdges[vB]) {
		m_htogc[eH->source()] = m_htogc[eH->target()] = nullptr;
	}

	for (edge eH : m_bNode_hEdges[vB]) {
		node sH = eH->source();
		node tH = eH->target();
		node& sGC = m_htogc[sH];
		node& tGC = m_htogc[tH];
		if (sGC == nullptr) { sGC = GC.newNode(); origNode[sGC] = sH; }
		if (tGC == nullptr) { tGC = GC.newNode(); origNode[tGC] = tH; }
		origEdge[GC.newEdge(sGC, tGC)] = eH;
	}

	Triconnectivity tricComp(GC);

	const GraphCopySimple& GCC = *tricComp.m_pGC;

	EdgeArray<node> partnerNode(GCC, nullptr);
	EdgeArray<edge> partnerEdge(GCC, nullptr);

	for (int i = 0; i < tricComp.m_numComp; ++i) {
		const Triconnectivity::CompStruct &C = tricComp.m_component[i];

		if (C.m_edges.empty()) {
			continue;
		}

		node vT;
		switch (C.m_type) {
			case Triconnectivity::CompType::bond:
				vT = newSPQRNode(vB, TNodeType::PComp);
				break;
			case Triconnectivity::CompType::polygon:
				vT = newSPQRNode(vB, TNodeType::SComp);
				break;
			case Triconnectivity::CompType::triconnected:
				vT = newSPQRNode(vB, TNodeType::RComp);
				break;
		}

		for (edge eGCC : C.m_edges) {
			edge eH = GCC.original(eGCC);
			if (eH == nullptr) {
				node uH = origNode[GCC.original(eGCC->source())];
				node vH = origNode[GCC.original(eGCC->target())];
				eH = m_H.newEdge(uH, vH);

				if (partnerNode[eGCC] == nullptr) {
					partnerNode[eGCC] = vT;
					partnerEdge[eGCC] = eH;
				} else {
					m_T.newEdge(partnerNode[eGCC], vT);
					m_hEdge_twinEdge[eH] = partnerEdge[eGCC];
					m_hEdge_twinEdge[partnerEdge[eGCC]] = eH;
				}
			} else {
				eH = origEdge[eH];
			}
			addHEdge(eH, vT);
		}
	}

	m_bNode_SPQR[vB] = m_hEdge_tNode[origEdge[GC.firstEdge()]];
	m_tNode_hRefEdge[m_bNode_SPQR[vB]] = nullptr;

	SList<node> lT;
	lT.pushBack(m_bNode_SPQR[vB]);
	lT.pushBack(nullptr);
	while (!lT.empty()) {
		node vT = lT.popFrontRet();
		node wT = lT.popFrontRet();
		for (edge eH : *m_tNode_hEdges[vT]) {
			edge fH = m_hEdge_twinEdge[eH];
			if (fH != nullptr) {
				node uT = m_hEdge_tNode[fH];
				if (uT == wT) {
					m_tNode_hRefEdge[vT] = eH;
				} else {
					lT.pushBack(uT);
					lT.pushBack(vT);
				}
			}
		}
	}
}


node DynamicSPQRForest::uniteSPQR(node vB, node sT, node tT)
{
	if (m_tNode_type[tT] == TNodeType::SComp) {
		m_bNode_numS[vB]--;
	} else if (m_tNode_type[tT] == TNodeType::PComp) {
		m_bNode_numP[vB]--;
	} else if (m_tNode_type[tT] == TNodeType::RComp) {
		m_bNode_numR[vB]--;
	}

	if (sT == nullptr) {
		m_bNode_numR[vB]++;
		sT = tT;
	} else {
		if (m_tNode_hEdges[sT]->size() < m_tNode_hEdges[tT]->size()) {
			std::swap(sT, tT);
		}
		m_tNode_owner[tT] = sT;
		m_tNode_hEdges[sT]->conc(*m_tNode_hEdges[tT]);
	}
	m_tNode_type[sT] = TNodeType::RComp;
	return sT;
}


node DynamicSPQRForest::findSPQR(node vT) const
{
	if (vT == nullptr || m_tNode_owner[vT] == vT) {
		return vT;
	} else {
		return m_tNode_owner[vT] = findSPQR(m_tNode_owner[vT]);
	}
}


node DynamicSPQRForest::findNCASPQR(node sT, node tT) const
{
	if (m_tNode_isMarked[sT]) {
		return sT;
	}
	m_tNode_isMarked[sT] = true;
	node uT = m_tNode_hRefEdge[sT] ? spqrproper(m_hEdge_twinEdge[m_tNode_hRefEdge[sT]]) : nullptr;
	if (uT == nullptr) {
		for (uT = tT; !m_tNode_isMarked[uT]; uT = spqrproper(m_hEdge_twinEdge[m_tNode_hRefEdge[uT]])) {};
	} else {
		uT = findNCASPQR(tT, uT);
	}
	m_tNode_isMarked[sT] = false;
	return uT;
}


SList<node>& DynamicSPQRForest::findPathSPQR(node sH, node tH, node& rT) const
{
	SList<node>& pathT = *new SList<node>;
	node sT = spqrproper(sH->firstAdj()->theEdge());
	node tT = spqrproper(tH->firstAdj()->theEdge());
	node nT = findNCASPQR(sT, tT);
	while (sT != nT) {
		edge eH = m_tNode_hRefEdge[sT];
		node uH = eH->source();
		node vH = eH->target();
		if (uH != sH && vH != sH) { pathT.pushBack(sT); }
		if (uH == tH || vH == tH) { rT = sT; return pathT; }
		sT = spqrproper(m_hEdge_twinEdge[eH]);
	}
	SListIterator<node> iT = pathT.backIterator();
	while (tT != nT) {
		edge eH = m_tNode_hRefEdge[tT];
		node uH = eH->source();
		node vH = eH->target();
		if (uH != tH && vH != tH) {
			if (iT.valid()) {
				pathT.insertAfter(tT, iT);
			} else {
				pathT.pushFront(tT);
			}
		}
		if (uH == sH || vH == sH) { rT = tT; return pathT; }
		tT = spqrproper(m_hEdge_twinEdge[eH]);
	}
	if (iT.valid()) {
		pathT.insertAfter(nT, iT);
	} else {
		pathT.pushFront(nT);
	}
	rT = nT; return pathT;
}


SList<node>& DynamicSPQRForest::findPathSPQR(node sH, node tH) const
{
	node vB = bComponent(m_hNode_gNode[sH], m_hNode_gNode[tH]);
	if (vB == nullptr) {
		return *new SList<node>;
	}
	if (m_bNode_SPQR[vB] == nullptr) {
		if (m_bNode_hEdges[vB].size() < 3) {
			return *new SList<node>;
		}
		createSPQR(vB);
	}
	node rT;
	SList<node>& pathT = findPathSPQR(sH, tH, rT);
	if (pathT.empty() && rT != nullptr) {
		pathT.pushBack(rT);
	}
	return pathT;
}


edge DynamicSPQRForest::virtualEdge(node vT, node wT) const
{
	edge eH = m_tNode_hRefEdge[vT];
	if (eH != nullptr) {
		eH = m_hEdge_twinEdge[eH];
		if (spqrproper(eH) == wT) {
			return eH;
		}
	}

	eH = m_tNode_hRefEdge[wT];
	if (eH != nullptr && spqrproper(m_hEdge_twinEdge[eH]) == vT) {
		return eH;
	}
	return nullptr;
}


edge DynamicSPQRForest::updateInsertedEdgeSPQR(node vB, edge eG)
{
	node sH = repVertex(eG->source(), vB);
	node tH = repVertex(eG->target(), vB);
	edge eH = m_H.newEdge(sH, tH);
	m_gEdge_hEdge[eG] = eH;
	m_hEdge_gEdge[eH] = eG;

	for (adjEntry adj : sH->adjEntries) {
		edge fH = adj->theEdge();
		if (fH == eH || fH->opposite(sH) != tH) {
			continue;
		}
		node vT = spqrproper(fH);
		if (m_tNode_type[vT] == TNodeType::PComp) {
			addHEdge(eH, vT);
			return eG;
		}
		edge gH = m_hEdge_twinEdge[fH];
		if (gH == nullptr) {
			node nT = newSPQRNode(vB, TNodeType::PComp);
			edge v1 = m_H.newEdge(sH, tH);
			edge v2 = newTwinEdge(v1, nT);
			m_hEdge_position[v1] = m_tNode_hEdges[vT]->insertAfter(v1, m_hEdge_position[fH]);
			m_tNode_hEdges[vT]->del(m_hEdge_position[fH]);
			m_hEdge_tNode[v1] = vT;
			addHEdge(fH, nT);
			addHEdge(eH, nT);
			m_tNode_hRefEdge[nT] = v2;
			return eG;
		}
		node wT = spqrproper(gH);
		if (m_tNode_type[wT] == TNodeType::PComp) {
			addHEdge(eH, vT);
		} else {
			node nT = newSPQRNode(vB, TNodeType::PComp);
			edge v1 = m_tNode_hRefEdge[vT];
			if (v1 == nullptr || spqrproper(m_hEdge_twinEdge[v1]) != wT) {
				v1 = m_tNode_hRefEdge[wT];
			}
			edge v4 = m_hEdge_twinEdge[v1];
			newTwinEdge(v1, nT);
			edge v3 = newTwinEdge(v4, nT);
			addHEdge(eH, nT);
			m_tNode_hRefEdge[nT] = v3;
		}
		return eG;
	}

	node rT = nullptr;
	SList<node>& pathT = findPathSPQR(sH, tH, rT);
	OGDF_ASSERT(rT != nullptr);
	if (pathT.size() < 2) {
		if (m_tNode_type[rT] == TNodeType::RComp) {
			addHEdge(eH, rT);
		} else {
			List<edge>* aH = m_tNode_hEdges[rT];
			SList<edge> pathH;
			bool aIsParent = true;
			ListIterator<edge> iH = aH->begin();
			node uH = sH;
			while (uH != tH) {
				while (!(*iH)->isIncident(uH)) {
					iH = aH->cyclicSucc(iH);
				}
				uH = (*iH)->opposite(uH);
				if (*iH == m_tNode_hRefEdge[rT]) {
					aIsParent = false;
				}
				pathH.pushBack(*iH);
				ListIterator<edge> jH = iH;
				iH = aH->cyclicSucc(iH);
				aH->del(jH);
			}
			node sT = newSPQRNode(vB, TNodeType::SComp);
			node newPT = newSPQRNode(vB, TNodeType::PComp);
			edge v1 = m_H.newEdge(sH, tH);
			edge v2 = newTwinEdge(v1, newPT);
			edge v3 = m_H.newEdge(sH, tH);
			edge v4 = newTwinEdge(v3, rT);
			addHEdge(v1, sT);
			addHEdge(eH, newPT);
			addHEdge(v3, newPT);
			for (edge ePathH : pathH) {
				addHEdge(ePathH, sT);
			}
			if (aIsParent) {
				m_tNode_hRefEdge[sT] = v1;
				m_tNode_hRefEdge[newPT] = v3;
			} else {
				m_tNode_hRefEdge[sT] = m_tNode_hRefEdge[rT];
				m_tNode_hRefEdge[newPT] = v2;
				m_tNode_hRefEdge[rT] = v4;
				if (!m_tNode_hRefEdge[sT]) {
					m_bNode_SPQR[vB] = sT;
				}
			}
		}
	} else {
		node xT = nullptr;
		SList<edge> absorbedEdges;
		SList<edge> virtualEdgesInPath;
		SList<edge> newVirtualEdges;

		edge rH = m_tNode_hRefEdge[rT];

		SListIterator<node> iT = pathT.begin();
		SListIterator<node> jT = iT;

		virtualEdgesInPath.pushBack(nullptr);
		for (++jT; jT.valid(); ++iT, ++jT) {
			edge gH;
			edge fH = m_tNode_hRefEdge[*iT];
			if (fH == nullptr || spqrproper(m_hEdge_twinEdge[fH]) != *jT) {
				gH = m_tNode_hRefEdge[*jT];
				fH = m_hEdge_twinEdge[gH];
			} else {
				gH = m_hEdge_twinEdge[fH];
			}
			virtualEdgesInPath.pushBack(fH);
			virtualEdgesInPath.pushBack(gH);
		}
		virtualEdgesInPath.pushBack(nullptr);

		for (node vT : pathT) {
			edge fH = virtualEdgesInPath.popFrontRet();
			edge gH = virtualEdgesInPath.popFrontRet();
			if (m_tNode_type[vT] == TNodeType::SComp) {
				List<edge>* aH = m_tNode_hEdges[vT];
				node uH = nullptr;
				bool bothVirtEdgesExist = false;
				if (fH == nullptr) {
					fH = gH;
					uH = sH;
				} else if (gH == nullptr) {
					uH = tH;
				} else {
					bothVirtEdgesExist = true;
				}

				node vH = fH->source();
				node wH = fH->target();
				node xH = bothVirtEdgesExist ? gH->source() : uH;
				node yH = bothVirtEdgesExist ? gH->target() : uH;
				bool incidentVirtEdges = bothVirtEdgesExist &&
				     (vH == xH || vH == yH || wH == xH || wH == yH);

				if (bothVirtEdgesExist && aH->size() == 3) {
					delHEdge(fH, vT);
					delHEdge(gH, vT);
					xT = uniteSPQR(vB, xT, vT);
				} else if (incidentVirtEdges) {
					edge nH = nullptr;
					if (vH == xH) { nH = m_H.newEdge(wH, yH); }
					else if (vH == yH) { nH = m_H.newEdge(wH, xH); }
					else if (wH == xH) { nH = m_H.newEdge(vH, yH); }
					else if (wH == yH) { nH = m_H.newEdge(vH, xH); }
					m_hEdge_position[nH] = aH->insertAfter(nH, m_hEdge_position[gH]);
					m_hEdge_tNode[nH] = vT;
					if (vT == rT) {
						rH = nH;
					} else {
						m_tNode_hRefEdge[vT] = nH;
					}
					delHEdge(fH, vT);
					delHEdge(gH, vT);
					newVirtualEdges.pushBack(nH);
				} else {
					ListIterator<edge> iH = m_hEdge_position[fH];
					ListIterator<edge> jH = m_hEdge_position[fH];
					SList<edge> pathH;
					node zH = nullptr;

					while (zH == nullptr) {
						iH = aH->cyclicSucc(iH);
						zH = fH->commonNode(*iH);
					}
					wH = fH->opposite(zH);
					vH = (*iH)->opposite(zH);
					delHEdge(*jH, vT);
					jH = iH;
					iH = aH->cyclicSucc(iH);
					pathH.pushBack(*jH);
					aH->del(jH);

					while (vH != xH && vH != yH) {
						while (!(*iH)->isIncident(vH)) {
							iH = aH->cyclicSucc(iH);
						}
						vH = (*iH)->opposite(vH);
						jH = iH;
						iH = aH->cyclicSucc(iH);
						pathH.pushBack(*jH);
						aH->del(jH);
					}
					if (bothVirtEdgesExist) {
						delHEdge(gH, vT);
					}
					if (pathH.size() == 1) {
						edge bhFront = pathH.front();
						if (bhFront == rH) { rT = nullptr; }
						absorbedEdges.pushBack(bhFront);
					} else {
						node nT = newSPQRNode(vB, TNodeType::SComp);
						while (!pathH.empty()) {
							edge bhFront = pathH.popFrontRet();
							addHEdge(bhFront, nT);
							if (bhFront == rH) { rT = nT; }
						}
						edge newEdgeH = m_H.newEdge(vH, zH);
						addHEdge(newEdgeH, nT);
						if (nT == rT) {
							m_tNode_hRefEdge[nT] = rH;
							if (rH == nullptr) { m_bNode_SPQR[vB] = nT; }
							rH = newEdgeH;
						} else {
							m_tNode_hRefEdge[nT] = newEdgeH;
						}
						newVirtualEdges.pushBack(newEdgeH);
					}
					if (m_tNode_hEdges[vT]->size() == 1) {
						xT = uniteSPQR(vB, xT, vT);
					}  else {
						edge newEdgeH = m_H.newEdge(wH, bothVirtEdgesExist ?
						                            (vH == yH ? xH : yH) : vH);
						addHEdge(newEdgeH, vT);
						if (vT == rT) {
							rH = newEdgeH;
						} else {
							m_tNode_hRefEdge[vT] = newEdgeH;
						}
						newVirtualEdges.pushBack(newEdgeH);
					}
				}
			} else {
				if (fH != nullptr) {
					delHEdge(fH, vT);
				}
				if (gH != nullptr) {
					delHEdge(gH, vT);
				}
				if (m_tNode_type[vT] == TNodeType::PComp &&
				    m_tNode_hEdges[vT]->size() > 1) {
					edge nH = m_tNode_hEdges[vT]->front();
					nH = m_H.newEdge(nH->source(), nH->target());
					addHEdge(nH, vT);
					if (vT == rT) {
						rH = nH;
					} else {
						m_tNode_hRefEdge[vT] = nH;
					}
					newVirtualEdges.pushBack(nH);
				} else {
					xT = uniteSPQR(vB, xT, vT);
				}
			}
		}
		if (xT == nullptr) {
			xT = newSPQRNode(vB, TNodeType::RComp);
		}
		while (!newVirtualEdges.empty()) {
			newTwinEdge(newVirtualEdges.popFrontRet(), xT);
		}
		while (!absorbedEdges.empty()) {
			addHEdge(absorbedEdges.popFrontRet(), xT);
		}
		addHEdge(eH, xT);
		if (rT != nullptr && findSPQR(rT) != xT) {
			m_tNode_hRefEdge[xT] = m_hEdge_twinEdge[rH];
		} else {
			m_tNode_hRefEdge[xT] = rH;
			if (rT != nullptr && rH == nullptr) {
				m_bNode_SPQR[vB] = xT;
			}
		}
	}
	delete &pathT;
	return eG;
}


node DynamicSPQRForest::updateInsertedNodeSPQR(node vB, edge eG, edge fG)
{
	node vG = fG->source();
	node wG = fG->target();
	node vH = m_H.newNode();
	node wH = repVertex(wG, vB);
	m_gNode_hNode[vG] = vH;
	m_hNode_gNode[vH] = vG;
	edge fH = m_H.newEdge(vH, wH);
	m_gEdge_hEdge[fG] = fH;
	m_hEdge_gEdge[fH] = fG;
	edge eH = m_gEdge_hEdge[eG];
	m_H.moveTarget(eH, vH);
	node vT = spqrproper(eH);
	if (m_tNode_type[vT] == TNodeType::SComp) {
		m_hEdge_position[fH] = m_tNode_hEdges[vT]->insertAfter(fH, m_hEdge_position[eH]);
		m_hEdge_tNode[fH] = vT;
	} else {
		node nT = newSPQRNode(vB, TNodeType::SComp);
		edge v1 = m_H.newEdge(eH->source(), fH->target());
		edge v2 = newTwinEdge(v1, nT);
		m_hEdge_position[v1] = m_tNode_hEdges[vT]->insertAfter(v1, m_hEdge_position[eH]);
		m_tNode_hEdges[vT]->del(m_hEdge_position[eH]);
		addHEdge(eH, nT);
		addHEdge(fH, nT);
		m_hEdge_tNode[v1] = vT;
		m_tNode_hRefEdge[nT] = v2;
	}
	return vG;
}


edge DynamicSPQRForest::updateInsertedEdge(edge eG)
{
	node sG = eG->source();
	node tG = eG->target();
	node vB = bComponent(sG, tG);
	if (vB == nullptr) {
		node nT = nullptr;
		int numS, numP, numR;
		SList<node>& pathB = findPath(sG, tG);
		SListIterator<node> jB = pathB.begin();
		SListIterator<node> iB = jB;
		while (iB.valid() && m_bNode_SPQR[*iB] == nullptr) { iB++; }
		if (iB.valid()) {
			nT = m_T.newNode();
			m_tNode_type[nT] = TNodeType::SComp;
			m_tNode_owner[nT] = nT;
			m_tNode_hRefEdge[nT] = nullptr;
			m_tNode_hEdges[nT] = new List<edge>;
			numS = 1;
			numP = 0;
			numR = 0;
			node sH = repVertex(sG, *jB);
			for (iB = jB; iB.valid(); ++iB) {
				node tH = (++jB).valid() ? cutVertex(*jB, *iB) : repVertex(tG, *iB);
				node mT;
				edge mH, nH;
				switch (numberOfEdges(*iB)) {
					case 0:
						break;
					case 1:
						addHEdge(m_bNode_hEdges[*iB].front(), nT);
						break;
					case 2:
						mT = m_T.newNode();
						m_tNode_type[mT] = TNodeType::PComp;
						m_tNode_owner[mT] = mT;
						m_tNode_hEdges[mT] = new List<edge>;
						addHEdge(m_bNode_hEdges[*iB].front(), mT);
						addHEdge(m_bNode_hEdges[*iB].back(), mT);
						mH = m_H.newEdge(sH, tH);
						addHEdge(mH, mT);
						newTwinEdge(mH, mT);
						m_tNode_hRefEdge[mT] = mH;
						numP++;
						break;
					default:
						if (m_bNode_SPQR[*iB] == nullptr) {
							createSPQR(*iB);
						}
						edge mG = m_G.newEdge(m_hNode_gNode[sH], m_hNode_gNode[tH]);
						updateInsertedEdgeSPQR(*iB, mG);
						mH = m_gEdge_hEdge[mG];
						mT = spqrproper(mH);
						m_G.delEdge(mG);
						m_hEdge_gEdge[mH] = nullptr;
						newTwinEdge(mH, nT);
						nH = m_tNode_hRefEdge[mT];
						m_tNode_hRefEdge[mT] = mH;
						while (nH != nullptr) {
							mH = m_hEdge_twinEdge[nH];
							mT = spqrproper(mH);
							nH = m_tNode_hRefEdge[mT];
							m_tNode_hRefEdge[mT] = mH;
						}
						numS += m_bNode_numS[*iB];
						numP += m_bNode_numP[*iB];
						numR += m_bNode_numR[*iB];
				}
				if (jB.valid()) {
					sH = cutVertex(*iB, *jB);
				}
			}
		}
		delete &pathB;
		DynamicBCTree::updateInsertedEdge(eG);
		if (nT != nullptr) {
			addHEdge(m_gEdge_hEdge[eG], nT);
			node eB = bcproper(eG);
			m_bNode_SPQR[eB] = nT;
			m_bNode_numS[eB] = numS;
			m_bNode_numP[eB] = numP;
			m_bNode_numR[eB] = numR;
		}
	} else {
		if (m_bNode_SPQR[vB] == nullptr) {
			DynamicBCTree::updateInsertedEdge(eG);
		} else {
			edge eH = m_gEdge_hEdge[updateInsertedEdgeSPQR(vB, eG)];
			m_bNode_hEdges[vB].pushBack(eH);
			m_hEdge_bNode[eH] = vB;
		}
	}
	return eG;
}


node DynamicSPQRForest::updateInsertedNode(edge eG, edge fG)
{
	node vB = bcproper(eG);
	if (m_bNode_SPQR[vB]) {
		node uG = updateInsertedNodeSPQR(vB, eG, fG);
		m_gNode_isMarked[uG] = false;
		edge fH = m_gEdge_hEdge[fG];
		m_bNode_hEdges[vB].pushBack(fH);
		m_hEdge_bNode[fH] = vB;
		m_hNode_bNode[fH->source()] = vB;
		m_bNode_numNodes[vB]++;
		return uG;
	}
	return DynamicBCTree::updateInsertedNode(eG, fG);
}


}
