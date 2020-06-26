/** \file
 * \brief Declaration and implementation of ogdf::NodeSet.
 *
 * \author Carsten Gutwenger, Tilo Wiedera
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

#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/List.h>

namespace ogdf {

//! Node sets.
/**
 * @ingroup graph-containers
 *
 * Maintains a subset of nodes contained in an associated graph.
 *
 * Provides efficient operations for testing membership,
 * iteration, insertion, and deletion of elements, as well as clearing the set.
 *
 * \tparam SupportFastSizeQuery Whether this set supports querying it's #size in
 * constant instead of linear time (in the size).
 *
 * \sa FaceSet
 */
template<bool SupportFastSizeQuery = true>
class NodeSet {
public:
	using ListType = typename std::conditional<SupportFastSizeQuery, List<node>, ListPure<node>>::type;

	//! Creates an empty node set associated with graph \p G.
	explicit NodeSet(const Graph &G) : m_it(G) { }

	//! Inserts node \p v into this set.
	/**
	 * This operation has constant runtime.
	 * If the node is already contained in this set, nothing happens.
	 *
	 * \pre \p v is a node in the associated graph.
	 */
	void insert(node v) {
		OGDF_ASSERT(v->graphOf() == m_it.graphOf());
		ListIterator<node> &itV = m_it[v];

		if (!itV.valid()) {
			itV = m_nodes.pushBack(v);
		}
	}

	//! Removes node \p v from this set.
	/**
	 * This operation has constant runtime.
	 *
	 * \pre \p v is a node in the associated graph.
	 * If the node is not contained in this set, nothing happens.
	 */
	void remove(node v) {
		OGDF_ASSERT(v->graphOf() == m_it.graphOf());
		ListIterator<node> &itV = m_it[v];

		if (itV.valid()) {
			m_nodes.del(itV);
			itV = ListIterator<node>();
		}
	}

	//! Removes all nodes from this set.
	/**
	 * After this operation, this set is empty and still associated with the same graph.
	 * The runtime of this operations is linear in the #size().
	 */
	void clear() {
		m_it.init(graphOf());
		m_nodes.clear();
	}

	//! Returns \c true iff node \p v is contained in this set.
	/**
	 * This operation has constant runtime.
	 *
	 * \pre \p v is a node in the associated graph.
	 */
	bool isMember(node v) const {
		OGDF_ASSERT(v->graphOf() == m_it.graphOf());
		return m_it[v].valid();
	}

	//! Returns a reference to the list of nodes contained in this set.
	const ListType &nodes() const {
		return m_nodes;
	}

	//! Returns the associated graph
	const Graph& graphOf() const {
		return *m_it.graphOf();
	}

	//! Returns the number of nodes in this set.
	/**
	 * This operation has either linear or constant runtime, depending on \a SupportFastSizeQuery.
	 */
	int size() const {
		return m_nodes.size();
	}

	//! Copy constructor.
	template<bool OtherSupportsFastSizeQuery>
	NodeSet(const NodeSet<OtherSupportsFastSizeQuery>& other) : m_it(other.graphOf()) {
		this = other;
	}

	//! Assignment operator.
	template<bool OtherSupportsFastSizeQuery>
	NodeSet &operator=(const NodeSet<OtherSupportsFastSizeQuery> &other) {
		m_nodes.clear();
		m_it.init(other.graphOf());
		for(node v : other.nodes()) {
			insert(v);
		}
	}

private:
	//! #m_it[\a v] contains the list iterator pointing to \a v if \a v is contained in this set,
	//! or an invalid list iterator otherwise.
	NodeArray<ListIterator<node>> m_it;

	//! The list of nodes contained in this set.
	ListType m_nodes;
};

}
