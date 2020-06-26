/** \file
 * \brief Implements the class EdgeStandardRep.
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
#include <ogdf/hypergraph/EdgeStandardRep.h>

namespace ogdf {

EdgeStandardRep::EdgeStandardRep()
	: m_type(EdgeStandardType::star), m_hypergraph(nullptr)
{
}

EdgeStandardRep::EdgeStandardRep(const Hypergraph &pH, EdgeStandardType pType = EdgeStandardType::star)
	: HypergraphObserver(&pH)
{
	m_type = pType;
	m_hypergraph = &pH;

	m_hypernodeMap = NodeArray<hypernode>(m_graphRep, nullptr);
	m_hyperedgeMap = EdgeArray<hyperedge>(m_graphRep, nullptr);

	m_nodeMap = HypernodeArray<node>(pH, nullptr);
	m_edgeMap = HyperedgeArray<List<edge> > (pH, List<edge>());

	if (m_type == EdgeStandardType::clique)
		constructCliqueRep();
	else if (m_type == EdgeStandardType::star)
		constructStarRep();
	else if (m_type == EdgeStandardType::tree)
		constructTreeRep();
}

EdgeStandardRep::~EdgeStandardRep()
{
	m_hypergraph = nullptr;
}

void EdgeStandardRep::constructCliqueRep()
{
	OGDF_ASSERT(m_hypergraph != nullptr);
	OGDF_ASSERT(m_type == EdgeStandardType::clique);

	cloneHypernodes();

	hyperedge e;
	forall_hyperedges(e, *m_hypergraph)
		hyperedgeToClique(e);
}

void EdgeStandardRep::constructStarRep()
{
	OGDF_ASSERT(m_hypergraph != nullptr);
	OGDF_ASSERT(m_type == EdgeStandardType::star);

	cloneHypernodes();
	hyperedge e;
	forall_hyperedges(e, *m_hypergraph)
		hyperedgeToTree(e, e->cardinality());
}

void EdgeStandardRep::constructTreeRep()
{
	OGDF_ASSERT(m_hypergraph != nullptr);
	OGDF_ASSERT(m_type == EdgeStandardType::tree);

	cloneHypernodes();

	hyperedge e;
	forall_hyperedges(e, *m_hypergraph)
		hyperedgeToTree(e, 3);
}

void EdgeStandardRep::cloneHypernodes()
{
	hypernode v;
	forall_hypernodes(v, *m_hypergraph) {
		node vRep = m_graphRep.newNode(v->index());
		m_hypernodeMap[vRep] = v;
		m_nodeMap[v] = vRep;
	}
}

void EdgeStandardRep::hypernodeDeleted(hypernode v)
{
	OGDF_ASSERT(v != nullptr);

	m_graphRep.delNode(m_nodeMap[v]);
}

void EdgeStandardRep::hypernodeAdded(hypernode v)
{
	OGDF_ASSERT(v != nullptr);

	node vRep = m_graphRep.newNode(v->index());
	m_hypernodeMap[vRep] = v;
	m_nodeMap[v] = vRep;
}

void EdgeStandardRep::hyperedgeDeleted(hyperedge e)
{
	OGDF_ASSERT(e != nullptr);

	for (ListIterator<edge> it = m_edgeMap[e].begin(); it.valid(); ++it) {
		m_graphRep.delEdge(*it);
		m_edgeMap[e].del(it);
	}

	for (ListIterator<node> it = m_dummyNodes.begin(); it.valid(); ++it) {
		if ((*it)->degree() == 0) {
			m_graphRep.delNode(*it);
			m_dummyNodes.del(it);
		}
	}
}

void EdgeStandardRep::hyperedgeAdded(hyperedge e)
{
	OGDF_ASSERT(e != nullptr);

	if (m_type == EdgeStandardType::clique)
		hyperedgeToClique(e);
	else if (m_type == EdgeStandardType::star)
		hyperedgeToTree(e, e->cardinality());
	else if (m_type == EdgeStandardType::tree)
		hyperedgeToTree(e, 3);
}

void EdgeStandardRep::cleared()
{
	m_graphRep.clear();
}

void EdgeStandardRep::hyperedgeToClique(hyperedge e)
{
	OGDF_ASSERT(e != nullptr);

	edge eRep;
	for (adjHypergraphEntry adjSrc = e->firstAdj(); adjSrc->succ(); adjSrc = adjSrc->succ()) {
		for (adjHypergraphEntry adjTgt = adjSrc->succ(); adjTgt; adjTgt = adjTgt->succ()) {

			eRep = m_graphRep.newEdge(m_nodeMap[reinterpret_cast<hypernode>(adjSrc->element())],
					m_nodeMap[reinterpret_cast<hypernode>(adjTgt->element())]);

			m_hyperedgeMap[eRep] = e;
			m_edgeMap[e].pushBack(eRep);
		}
	}
}

void EdgeStandardRep::hyperedgeToTree(hyperedge e, int degree)
{
	OGDF_ASSERT(e != nullptr);
	OGDF_ASSERT(degree >= 2);

	List<node> orphans;
	for (adjHypergraphEntry adj = e->firstAdj(); adj; adj = adj->succ()) {
		OGDF_ASSERT(m_nodeMap[reinterpret_cast<hypernode>(adj->element())] != nullptr);
		orphans.pushBack(m_nodeMap[reinterpret_cast<hypernode>(adj->element())]);
	}

	edge eRep;
	node parentDummy;
	while (orphans.size() > degree) {
		parentDummy = m_graphRep.newNode();
		m_hypernodeMap[parentDummy] = nullptr;
		m_dummyNodes.pushBack(parentDummy);

		for (int i = 0; i < degree - 1; i++) {

			eRep = m_graphRep.newEdge(*(orphans.begin()), parentDummy);
			m_hyperedgeMap[eRep] = e;
			m_edgeMap[e].pushBack(eRep);

			orphans.del(orphans.begin());
		}

		orphans.pushBack(parentDummy);
	}
	OGDF_ASSERT(orphans.size() <= degree);
	if (orphans.size() == 2) {
		eRep = m_graphRep.newEdge(*(++(orphans.begin())), *(orphans.begin()));
		m_hyperedgeMap[eRep] = e;
		m_edgeMap[e].pushBack(eRep);
	} else {
		parentDummy = m_graphRep.newNode();
		m_dummyNodes.pushBack(parentDummy);
		m_hypernodeMap[parentDummy] = nullptr;
		for (ListIterator<node> it = orphans.begin(); it.valid(); ++it) {
			eRep = m_graphRep.newEdge(parentDummy, *it);
			m_hyperedgeMap[eRep] = e;
			m_edgeMap[e].pushBack(eRep);
		}
	}
}

}
