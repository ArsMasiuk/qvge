/** \file
 * \brief Implementation of class BCTree
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


#include <ogdf/decomposition/BCTree.h>


namespace ogdf {

void BCTree::initBasic(node vG)
{
	m_numB = 0;
	m_numC = 0;

	m_gNode_isMarked.init(m_G, false);
	m_gNode_hNode.init(m_G, nullptr);
	m_gEdge_hEdge.init(m_G);

	m_bNode_type.init(m_B);
	m_bNode_isMarked.init(m_B, false);
	m_bNode_hRefNode.init(m_B);
	m_bNode_hParNode.init(m_B);
	m_bNode_hEdges.init(m_B);
	m_bNode_numNodes.init(m_B);

	m_hNode_bNode.init(m_H);
	m_hEdge_bNode.init(m_H);
	m_hNode_gNode.init(m_H);
	m_hEdge_gEdge.init(m_H);

	m_count = 0;
	m_number.init(m_G, 0);
	m_lowpt.init(m_G);
	m_gtoh.init(m_G);

	biComp(nullptr, vG);
}

void BCTree::initEdges()
{
	// first clear temporaries
	m_number.init();
	m_lowpt.init();
	m_eStack.clear();
	m_gtoh.init();

	// now add edges
	for (node uB : m_B.nodes) {
		node vB = parent(uB);
		if (vB) {
			m_B.newEdge(uB, vB);
		}
	}
}

void BCTree::init (node vG)
{
	initBasic(vG);
	initEdges();
}


void BCTree::initNotConnected (node vG)
{
	initBasic(vG);

	// call biComp for all nodes that are not in the
	//  first connected component
	for (node v : m_G.nodes) {
		if (m_number[v] == 0) {
			m_eStack.clear();
			biComp(nullptr, v);
		}
	}

	initEdges();
}


void BCTree::biComp (adjEntry adjuG, node vG)
{
	m_lowpt[vG] = m_number[vG] = ++m_count;

	for(adjEntry adj : vG->adjEntries) {
#if 0
		edge eG = adj->theEdge();
#endif
		node wG = adj->twinNode();
		if ((adjuG != nullptr) && (adj == adjuG->twin())) continue;
		if (m_number[wG]==0) {
			m_eStack.push(adj);
			biComp(adj,wG);
			if (m_lowpt[wG]<m_lowpt[vG]) m_lowpt[vG] = m_lowpt[wG];
			if (m_lowpt[wG]>=m_number[vG]) {
				node bB = m_B.newNode();
				m_bNode_type[bB] = BNodeType::BComp;
				m_bNode_isMarked[bB] = false;
				m_bNode_hRefNode[bB] = nullptr;
				m_bNode_hParNode[bB] = nullptr;
				m_bNode_numNodes[bB] = 0;
				m_numB++;
				adjEntry adjfG;
				do {
					adjfG = m_eStack.popRet();
					edge fG = adjfG->theEdge();
					for (int i=0; i<=1; ++i) {
						node xG = i ? fG->target() : fG->source();
						if (m_gNode_isMarked[xG]) continue;
						m_gNode_isMarked[xG] = true;
						m_nodes.pushBack(xG);
						m_bNode_numNodes[bB]++;
						node zH = m_H.newNode();
						m_hNode_bNode[zH] = bB;
						m_hNode_gNode[zH] = xG;
						m_gtoh[xG] = zH;
						node xH = m_gNode_hNode[xG];
						if (!xH) m_gNode_hNode[xG] = zH;
						else {
							node xB = m_hNode_bNode[xH];
							if (!m_bNode_hRefNode[xB]) {
								node cB = m_B.newNode();
								node yH = m_H.newNode();
								m_hNode_bNode[yH] = cB;
								m_hNode_gNode[yH] = xG;
								m_gNode_hNode[xG] = yH;
								m_bNode_type[cB] = BNodeType::CComp;
								m_bNode_isMarked[cB] = false;
								m_bNode_hRefNode[xB] = xH;
								m_bNode_hParNode[xB] = yH;
								m_bNode_hRefNode[cB] = yH;
								m_bNode_hParNode[cB] = zH;
								m_bNode_numNodes[cB] = 1;
								m_numC++;
							}
							else {
								node yH = m_bNode_hParNode[xB];
								node yB = m_hNode_bNode[yH];
								m_bNode_hParNode[yB] = xH;
								m_bNode_hRefNode[yB] = yH;
								m_bNode_hParNode[xB] = zH;
							}
						}
					}
					edge fH = m_H.newEdge(m_gtoh[fG->source()],m_gtoh[fG->target()]);
					m_bNode_hEdges[bB].pushBack(fH);
					m_hEdge_bNode[fH] = bB;
					m_hEdge_gEdge[fH] = fG;
					m_gEdge_hEdge[fG] = fH;
				} while (adj!=adjfG);
				while (!m_nodes.empty()) m_gNode_isMarked[m_nodes.popFrontRet()] = false;
			}
		}
		else if (m_number[wG]<m_number[vG]) {
			m_eStack.push(adj);
			if (m_number[wG]<m_lowpt[vG]) m_lowpt[vG] = m_number[wG];
		}
	}
}


node BCTree::parent (node vB) const
{
	if (!vB) return nullptr;
	node vH = m_bNode_hParNode[vB];
	if (!vH) return nullptr;
	return m_hNode_bNode[vH];
}


node BCTree::bComponent (node uG, node vG) const
{
	node uB = bcproper(uG);
	node vB = bcproper(vG);
	if (uB==vB) return uB;
	if (typeOfBNode(uB)==BNodeType::BComp) {
		if (typeOfBNode(vB)==BNodeType::BComp) return nullptr;
		if (parent(uB)==vB) return uB;
		if (parent(vB)==uB) return uB;
		return nullptr;
	}
	if (typeOfBNode(vB)==BNodeType::BComp) {
		if (parent(uB)==vB) return vB;
		if (parent(vB)==uB) return vB;
		return nullptr;
	}
	node pB = parent(uB);
	node qB = parent(vB);
	if (pB==qB) return pB;
	if (parent(pB)==vB) return pB;
	if (parent(qB)==uB) return qB;
	return nullptr;
}


node BCTree::findNCA (node uB, node vB) const
{
	if (m_bNode_isMarked[uB]) return uB;
	m_bNode_isMarked[uB] = true;
	node wB = parent(uB);
	if (wB) wB = findNCA(vB,wB);
	else for (wB=vB; !m_bNode_isMarked[wB]; wB=parent(wB));
	m_bNode_isMarked[uB] = false;
	return wB;
}


SList<node>& BCTree::findPath (node sG, node tG) const
{
	SList<node>& pB = *new SList<node>;
	node sB = bcproper(sG);
	node tB = bcproper(tG);
	node nB = findNCA(sB,tB);
	for (pB.pushBack(sB); sB!=nB; pB.pushBack(sB)) sB = parent(sB);
	for (SListIterator<node> iB=pB.backIterator(); tB!=nB; tB=parent(tB)) pB.insertAfter(tB,iB);
	return pB;
}


SList<node>* BCTree::findPathBCTree (node sB, node tB) const
{
	SList<node> *pB = new SList<node>;
	node nB = findNCA(sB,tB);
	for (pB->pushBack(sB); sB!=nB; pB->pushBack(sB)) sB = parent(sB);
	for (SListIterator<node> iB=pB->backIterator(); tB!=nB; tB=parent(tB)) pB->insertAfter(tB,iB);
	return pB;
}


node BCTree::repVertex (node uG, node vB) const
{
	node uB = bcproper(uG);
	if (uB==vB) return m_gNode_hNode[uG];
	if (typeOfBNode(uB)==BNodeType::BComp) return nullptr;
	if (parent(uB)==vB) return m_bNode_hParNode[uB];
	if (uB==parent(vB)) return m_bNode_hRefNode[vB];
	return nullptr;
}

node BCTree::cutVertex (node uB, node vB) const
{
	if (uB==vB) return typeOfBNode(uB)==BNodeType::CComp ? m_bNode_hRefNode[vB] : nullptr;
	if (parent(uB)==vB) return m_bNode_hParNode[uB];
	if (uB==parent(vB)) return m_bNode_hRefNode[vB];
	return nullptr;
}


}
