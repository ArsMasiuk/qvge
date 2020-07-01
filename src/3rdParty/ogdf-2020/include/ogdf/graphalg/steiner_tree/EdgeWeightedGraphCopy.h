/** \file
 * \brief Extends the GraphCopy concept to weighted graphs
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
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraph.h>

namespace ogdf {

template<typename T>
class EdgeWeightedGraphCopy: public GraphCopy {
public:
	EdgeWeightedGraphCopy() : GraphCopy() {}
	explicit EdgeWeightedGraphCopy(const EdgeWeightedGraph<T> &wC);
	EdgeWeightedGraphCopy(const EdgeWeightedGraphCopy &wGC);
	EdgeWeightedGraphCopy &operator=(const EdgeWeightedGraphCopy &wGC);
	virtual ~EdgeWeightedGraphCopy() {}
	void createEmpty(const Graph &wG);
	void init(const EdgeWeightedGraph<T>& wG);
	edge newEdge(node u, node v, T weight);
	edge newEdge(edge eOrig, T weight);
	T weight(const edge e) const {
		return m_edgeWeight[e];
	}
	void setWeight(const edge e, T v) {
		m_edgeWeight[e] = v;
	}
	const EdgeArray<T> &edgeWeights() const {
		return m_edgeWeight;
	}

protected:
	EdgeArray<T> m_edgeWeight;

private:
	void initWGC(const EdgeWeightedGraphCopy &wGC, NodeArray<node> &vCopy, EdgeArray<edge> &eCopy);
};

}

// Implementation

namespace ogdf {

template<typename T>
void EdgeWeightedGraphCopy<T>::initWGC(const EdgeWeightedGraphCopy &wGC, NodeArray<node> &vCopy, EdgeArray<edge> &eCopy)
{
	m_pGraph = wGC.m_pGraph;

	m_vOrig.init(*this, 0);
	m_eOrig.init(*this, 0);
	m_vCopy.init(*m_pGraph, 0);
	m_eCopy.init(*m_pGraph);
	m_eIterator.init(*this, 0);

	for(node v : wGC.nodes) {
		m_vOrig[vCopy[v]] = wGC.original(v);
	}

	for(edge e : wGC.edges) {
		m_eOrig[eCopy[e]] = wGC.original(e);
	}

	for(node v : nodes) {
		node w = m_vOrig[v];
		if (w != nullptr) {
			m_vCopy[w] = v;
		}
	}

	for(edge e : m_pGraph->edges) {
		ListConstIterator<edge> it;
		for (it = wGC.m_eCopy[e].begin(); it.valid(); ++it)
			m_eIterator[eCopy[*it]] = m_eCopy[e].pushBack(eCopy[*it]);
	}

	m_edgeWeight.init(*this);

	for(edge e : wGC.edges) {
		m_edgeWeight[eCopy[e]] = wGC.weight(e);
	}
}

template<typename T>
EdgeWeightedGraphCopy<T> &EdgeWeightedGraphCopy<T>::operator=(const EdgeWeightedGraphCopy<T> &wGC)
{
	GraphCopy::operator =(wGC);

	m_edgeWeight.init(*this);

	for(edge e : wGC.edges) {
		const edge f = wGC.original(e);
		m_edgeWeight[copy(f)] = wGC.weight(e);
	}

	return *this;
}

template<typename T>
EdgeWeightedGraphCopy<T>::EdgeWeightedGraphCopy(const EdgeWeightedGraphCopy<T> &wGC)
	: GraphCopy(wGC), m_edgeWeight(*this)
{
	for(edge e : wGC.edges) {
		const edge f = wGC.original(e);
		m_edgeWeight[copy(f)] = wGC.weight(e);
	}
}

template<typename T>
EdgeWeightedGraphCopy<T>::EdgeWeightedGraphCopy(const EdgeWeightedGraph<T> &wG)
	: GraphCopy(wG), m_edgeWeight(*this)
{
	for(edge e : edges) {
		m_edgeWeight[e] = wG.weight(original(e));
	}
}

template<typename T>
void EdgeWeightedGraphCopy<T>::init(const EdgeWeightedGraph<T>& wG)
{
	GraphCopy::init(wG);

	m_edgeWeight.init(*this);
	for (edge e : edges) {
		m_edgeWeight[e] = wG.weight(original(e));
	}
}

template<typename T>
void EdgeWeightedGraphCopy<T>::createEmpty(const Graph &G) {
	GraphCopy::createEmpty(G);
	m_pGraph = &G;
	m_edgeWeight.init(*this);
}

template<typename T>
edge EdgeWeightedGraphCopy<T>::newEdge(node u, node v, T weight) {
	edge e = GraphCopy::newEdge(u, v);
	m_edgeWeight[e] = weight;
	return e;
}

template<typename T>
edge EdgeWeightedGraphCopy<T>::newEdge(edge eOrig, T weight) {
	edge e = GraphCopy::newEdge(eOrig);
	m_edgeWeight[e] = weight;
	return e;
}

}
