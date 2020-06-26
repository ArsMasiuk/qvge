/** \file
 * \brief Declaration of Hierarchy class.
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

#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/GraphCopy.h>


namespace ogdf {

//! Representation of proper hierarchies used by Sugiyama-layout.
/**
 * \see Level, SugiyamaLayout
 */
class OGDF_EXPORT Hierarchy {

	friend class LayerBasedUPRLayout;

	GraphCopy m_GC; //!< The graph copy representing the topology of the proper hierarchy.
	NodeArray<int> m_rank; //!< The rank (level) of a node.
	Array<int> m_size;

public:
	//! Creates an empty hierarchy.
	Hierarchy() { }
	//! Creates an hierarchy of graph \p G with node ranks \p rank.
	Hierarchy(const Graph &G, const NodeArray<int> &rank);

	// destruction
	~Hierarchy() { }

	void createEmpty(const Graph &G);
	void initByNodes(const List<node> &nodes,
		EdgeArray<edge> &eCopy,
		const NodeArray<int> &rank);

	//! Conversion to const GraphCopy reference.
	operator const GraphCopy &() const { return m_GC; }

	//! Returns the rank (level) of node \p v.
	int rank(node v) const { return m_rank[v]; }

	int maxRank() const { return m_size.high(); }

	int size(int i) const { return m_size[i]; }

	bool isLongEdgeDummy(node v) const {
		return m_GC.isDummy(v) && v->outdeg() == 1;
	}

private:
	void doInit(const NodeArray<int> &rank);
};

}
