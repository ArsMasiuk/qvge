/** \file
 * \brief Implements class GraphReduction.
 *
 * \author Markus Chimani
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


#include <ogdf/graphalg/GraphReduction.h>

namespace ogdf {

GraphReduction::GraphReduction(const Graph &G)
	: m_pGraph(&G), m_vOrig(), m_eOrig(), m_vReduction(), m_eReduction()
{
	Graph::construct(*m_pGraph, m_vReduction, m_eReduction);

	// remove selfloops
	for (edge e1 : edges) {
		if (e1->isSelfLoop()) {
			m_eReduction[e1] = nullptr;
			this->delEdge(e1);
		}
	}

	m_vOrig.init(*this);
	m_eOrig.init(*this);
	for (node v : m_pGraph->nodes)
		m_vOrig[m_vReduction[v]] = v;

	for (edge e1 : m_pGraph->edges)
		m_eOrig[m_eReduction[e1]].pushBack(e1);

	List<node> next;
	for (node v : m_pGraph->nodes)
		next.pushBack(v);

	while (next.size()) {
		node ov = next.front();
		next.popFront();
		node v = m_vReduction[ov];
		int d;
		if (v && (d = v->degree()) < 3) {
			if (d == 2) {
				edge e1 = v->firstAdj()->theEdge();
				edge e2 = v->lastAdj()->theEdge();
				if (e1 == e2) {
					for (edge f : m_eOrig[e1]) {
						m_eReduction[f] = nullptr;
					}
				}
				else
				if (e1->source() == v) {
					if (e2->source() == v) m_eOrig[e2].reverse();
					this->moveSource(e1, e2->opposite(v));
					for (edge origEdge : reverse(m_eOrig[e2])) {
						m_eReduction[origEdge] = e1;
						m_eOrig[e1].pushFront(origEdge);
					}
				}
				else {
					if (e2->target() == v) m_eOrig[e2].reverse();
					this->moveTarget(e1, e2->opposite(v));
					for (ListConstIterator<edge> it = m_eOrig[e2].begin(); it.valid(); ++it) {
						m_eReduction[*it] = e1;
						m_eOrig[e1].pushBack(*it);
					}
				}
				m_eOrig[e2].clear();
				this->delEdge(e2);
			}
			else if (d == 1) {
				edge e1 = v->firstAdj()->theEdge();
				const List<edge>& el = m_eOrig[e1];
				node nv;
				if (el.size() == 1)
					nv = el.front()->opposite(ov);
				else {
					bool front_e1 = el.front()->source() == ov || el.front()->target() == ov;
					edge e2, e3;
					if (front_e1) {
						e2 = el.back();
						e3 = *(el.rbegin().succ());
					}
					else {
						e2 = el.front();
						e3 = *(el.begin().succ());
					}
					nv = (e2->source() == e3->source() || e2->source() == e3->target()) ? e2->target() : e2->source();
				}
				next.pushBack(nv);

				for (ListIterator<edge> it = m_eOrig[e1].begin(); it.valid(); ++it)
					m_eReduction[*it] = nullptr;
				this->delEdge(e1);
			}
			m_vReduction[ov] = nullptr;
			this->delNode(v);
		}
	}
}

}
