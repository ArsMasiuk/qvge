/** \file
 * \brief Declaration of class EdgeWeightedGraph
 *
 * \author Matthias Woste
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

#include <ogdf/basic/GraphCopy.h>

namespace ogdf {

template<typename T>
class EdgeWeightedGraph: public Graph {
public:
	EdgeWeightedGraph()
	  : Graph()
	  , m_edgeWeight(*this, 0)
	{
	}

	explicit EdgeWeightedGraph(GraphCopy &gC)
	{
	}

	virtual ~EdgeWeightedGraph()
	{
	}

	edge newEdge(node v, node w, T weight)
	{
		edge e = Graph::newEdge(v, w);
		m_edgeWeight[e] = weight;
		return e;
	}

	node newNode()
	{
		node u = Graph::newNode();
		return u;
	}

	T weight(const edge e) const
	{
		return m_edgeWeight[e];
	}

	const EdgeArray<T> &edgeWeights() const
	{
		return m_edgeWeight;
	}

	void setWeight(const edge e, T weight)
	{
		m_edgeWeight[e] = weight;
	}

protected:
	EdgeArray<T> m_edgeWeight;
};

}
