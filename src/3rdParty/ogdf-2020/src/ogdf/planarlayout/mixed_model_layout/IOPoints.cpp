/** \file
 * \brief Implementation in-/out-points management.
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


#include <ogdf/planarlayout/mixed_model_layout/IOPoints.h>


namespace ogdf {


ListConstIterator<InOutPoint> IOPoints::searchRealForward(
	ListConstIterator<InOutPoint> it) const
{
	while (it.valid() && marked((*it).m_adj))
		++it;

	return it;
}

ListConstIterator<InOutPoint> IOPoints::searchRealBackward(
	ListConstIterator<InOutPoint> it) const
{
	while (it.valid() && marked((*it).m_adj))
		--it;

	return it;
}


void IOPoints::restoreDeg1Nodes(PlanRep &PG, ArrayBuffer<PlanRep::Deg1RestoreInfo> &S)
{
	List<node> deg1s;

	PG.restoreDeg1Nodes(S,deg1s);

	for(node v : deg1s) {
		adjEntry adj = v->firstAdj();
		m_mark[adj] = m_mark[adj->twin()] = true;
	}
}


adjEntry IOPoints::switchBeginIn(node v)
{
	List<InOutPoint> &Lin  = m_in [v];
	List<InOutPoint> &Lout = m_out[v];

	ListConstIterator<InOutPoint> it;
	adjEntry adj;

	while ((it = Lin.begin()).valid() && marked(adj = (*it).m_adj))
		m_pointOf[adj] = &(*Lout.pushFront(Lin.popFrontRet()));

	return it.valid() ? adj : nullptr;
}


adjEntry IOPoints::switchEndIn(node v)
{
	List<InOutPoint> &Lin  = m_in [v];
	List<InOutPoint> &Lout = m_out[v];

	ListConstReverseIterator<InOutPoint> it;
	adjEntry adj;

	while ((it = Lin.rbegin()).valid() && marked(adj = (*it).m_adj))
		m_pointOf[adj] = &(*Lout.pushBack(Lin.popBackRet()));

	return it.valid() ? adj : nullptr;
}


void IOPoints::switchBeginOut(node v)
{
	List<InOutPoint> &Lin  = m_in [v];
	List<InOutPoint> &Lout = m_out[v];

	adjEntry adj = (*Lout.begin()).m_adj;
	m_pointOf[adj] = &(*Lin.pushFront(Lout.popFrontRet()));
}


void IOPoints::switchEndOut(node v)
{
	List<InOutPoint> &Lin  = m_in [v];
	List<InOutPoint> &Lout = m_out[v];

	adjEntry adj = (*Lout.rbegin()).m_adj;
	m_pointOf[adj] = &(*Lin.pushBack(Lout.popBackRet()));
}


void IOPoints::numDeg1(node v, int &xl, int &xr,
	bool doubleCount) const
{
	const List<InOutPoint> &L = m_out[v];
	ListConstIterator<InOutPoint> it;

	xl = xr = 0;
	for (it = L.begin(); it.valid() && marked((*it).m_adj); ++it)
		++xl;

	if (doubleCount || it.valid()) // avoid double counting if all are marked
		for (ListConstReverseIterator<InOutPoint> revIt = L.rbegin(); revIt.valid() && marked((*revIt).m_adj); ++revIt)
			++xr;
}

InOutPoint IOPoints::middleNeighbor(node z1) const
{
	const List<InOutPoint> &L = m_in[z1];

	ListConstIterator<InOutPoint> it, itFound;
	int i,  pos = (L.size()-1)/2;

	for (it = L.begin().succ(), i = 1; i <= pos || !itFound.valid(); ++it, ++i)
		if (!marked((*it).m_adj))
			itFound = it;

	return *itFound;
}

}
