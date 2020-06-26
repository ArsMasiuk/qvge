/** \file
 * \brief Declaration of class GeedyCycleRemoval.
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

#include <ogdf/layered/AcyclicSubgraphModule.h>
#include <ogdf/basic/NodeArray.h>

namespace ogdf {

//! Greedy algorithm for computing a maximal acyclic subgraph.
/**
 * The algorithm applies a greedy heuristic to compute a maximal
 * acyclic subgraph and works in linear-time.
 */
class OGDF_EXPORT GreedyCycleRemoval : public AcyclicSubgraphModule {
public:
	GreedyCycleRemoval() { }

	//! Computes the set of edges \p arcSet, which have to be deleted in the acyclic subgraph.
	virtual void call (const Graph &G, List<edge> &arcSet) override;

private:
	void dfs (node v, const Graph &G);

	int m_min, m_max, m_counter;

	NodeArray<int> m_in, m_out, m_index;
	Array<ListPure<node> > m_B;
	NodeArray<ListIterator<node> > m_item;
	NodeArray<bool> m_visited;
};

}
