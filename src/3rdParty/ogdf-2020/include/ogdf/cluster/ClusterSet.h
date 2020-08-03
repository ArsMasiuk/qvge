/** \file
 * \brief Declaration and implementation of class ClusterSetSimple,
 * ClusterSetPure and ClusterSet
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

#include <ogdf/cluster/ClusterArray.h>
#include <ogdf/basic/List.h>



namespace ogdf {


//! Simple cluster sets.
/**
 * @ingroup graph-containers
 *
 * A cluster set maintains a subset \a S of the clusters contained in an associated
 * clustered graph. This kind of cluster set only provides efficient operation for testing
 * membership, insertion, and clearing the set.
 *
 * \sa ClusterSet, ClusterSetPure
 */
class OGDF_EXPORT ClusterSetSimple {
public:
	//! Creates an empty cluster set associated with clustered graph \p C.
	explicit ClusterSetSimple(const ClusterGraph &C) : m_isContained(C, false) { }

	// destructor
	~ClusterSetSimple() { }

	//! Inserts cluster \p c into \a S.
	/**
	 * This operation has constant runtime.
	 *
	 * \pre \p c is a cluster in the associated clustered graph.
	 */
	void insert(cluster c) {
		OGDF_ASSERT(c->graphOf() == m_isContained.graphOf());
		bool &isContained = m_isContained[c];
		if (!isContained) {
			isContained = true;
			m_clusters.pushFront(c);
		}
	}


	//! Removes all clusters from \a S.
	/**
	 * After this operation, \a S is empty and still associated with the same clustered graph.
	 * The runtime of this operations is O(k), where k is the number of clusters in \a S
	 * before this operation.
	 */
	void clear() {
		SListIterator<cluster> it;
		for(it = m_clusters.begin(); it.valid(); ++it) {
			m_isContained[*it] = false;
		}
		m_clusters.clear();
	}


	//! Returns true if cluster \p c is contained in \a S, false otherwise.
	/**
	 * This operation has constant runtime.
	 *
	 * \pre \p c is a cluster in the associated graph.
	 */
	bool isMember(cluster c) const {
		OGDF_ASSERT(c->graphOf() == m_isContained.graphOf());
		return m_isContained[c];
	}

	//! Returns a reference to the list of clusters contained in \a S.
	/**
	 * This list can be used for iterating over all clusters in \a S.
	 */
	const SListPure<cluster> &clusters() const {
		return m_clusters;
	}

private:
	//! m_isContained[\a c] is true iff \a c is contained in \a S
	ClusterArray<bool> m_isContained;

	//! The list of clusters contained in \a S
	SListPure<cluster> m_clusters;
};



//! Cluster sets.
/**
 * @ingroup graph-containers
 *
 * A cluster set maintains a subset \a S of the clusters contained in an associated
 * clustered graph. This kind of cluster set provides efficient operations for testing
 * membership, insertion and deletion of elements, and clearing the set.
 *
 * In contrast to ClusterSet, a ClusterSetPure does not provide efficient access
 * to the number of clusters stored in the set.
 *
 * \sa ClusterSet, ClusterSetSimple
 */
class OGDF_EXPORT ClusterSetPure {
public:
	//! Creates an empty cluster set associated with clustered graph \p C.
	explicit ClusterSetPure(const ClusterGraph &C) : m_it(C,ListIterator<cluster>()) { }

	// destructor
	~ClusterSetPure() { }

	//! Inserts cluster \p c into \a S.
	/**
	 * This operation has constant runtime.
	 *
	 * \pre \p c is a cluster in the associated clustered graph.
	 */
	void insert(cluster c) {
		OGDF_ASSERT(c->graphOf() == m_it.graphOf());
		ListIterator<cluster> &itV = m_it[c];
		if (!itV.valid())
			itV = m_clusters.pushBack(c);
	}

	//! Removes cluster \p c from \a S.
	/**
	 * This operation has constant runtime.
	 *
	 * \pre \p c is a cluster in the associated clustered graph.
	 */
	void remove(cluster c) {
		OGDF_ASSERT(c->graphOf() == m_it.graphOf());
		ListIterator<cluster> &itV = m_it[c];
		if (itV.valid()) {
			m_clusters.del(itV);
			itV = ListIterator<cluster>();
		}
	}


	//! Removes all clusters from \a S.
	/**
	 * After this operation, \a S is empty and still associated with the same clustered graph.
	 * The runtime of this operations is O(k), where k is the number of clusters in \a S
	 * before this operation.
	 */
	void clear() {
		ListIterator<cluster> it;
		for(it = m_clusters.begin(); it.valid(); ++it) {
			m_it[*it] = ListIterator<cluster>();
		}
		m_clusters.clear();
	}


	//! Returns true if cluster \p c is contained in \a S, false otherwise.
	/**
	 * This operation has constant runtime.
	 *
	 * \pre \p c is a cluster in the associated graph.
	 */
	bool isMember(cluster c) const {
		OGDF_ASSERT(c->graphOf() == m_it.graphOf());
		return m_it[c].valid();
	}

	//! Returns a reference to the list of clusters contained in \a S.
	/**
	 * This list can be used for iterating over all clusters in \a S.
	 */
	const ListPure<cluster> &clusters() const {
		return m_clusters;
	}

private:
	//! #m_it[\a c] contains the list iterator pointing to \a c if \a c is contained in \a S,
	//! an invalid list iterator otherwise.
	ClusterArray<ListIterator<cluster> > m_it;

	//! The list of clusters contained in \a S.
	ListPure<cluster> m_clusters;
};



//! Cluster sets.
/**
 * @ingroup graph-containers
 *
 * A cluster set maintains a subset \a S of the clusters contained in an associated
 * clustered graph. This kind of cluster set provides efficient operations for testing
 * membership, insertion and deletion of elements, and clearing the set.
 *
 * In contrast to ClusterSetPure, a ClusterSet provides efficient access
 * to the number of clusters stored in the set.
 *
 * \sa - ClusterSetPure, ClusterSetSimple
 */
class OGDF_EXPORT ClusterSet {
public:
	//! Creates an empty cluster set associated with clustered graph \p C.
	explicit ClusterSet(const ClusterGraph &C) : m_it(C, ListIterator<cluster>()) { }

	// destructor
	~ClusterSet() { }

	//! Inserts cluster \p c into \a S.
	/**
	 * This operation has constant runtime.
	 *
	 * \pre \p c is a cluster in the associated clustered graph.
	 */
	void insert(cluster c) {
		OGDF_ASSERT(c->graphOf() == m_it.graphOf());
		ListIterator<cluster> &itV = m_it[c];
		if (!itV.valid())
			itV = m_clusters.pushBack(c);
	}

	//! Removes cluster \p c from \a S.
	/**
	 * This operation has constant runtime.
	 *
	 * \pre \p c is a cluster in the associated clustered graph.
	 */
	void remove(cluster c) {
		OGDF_ASSERT(c->graphOf() == m_it.graphOf());
		ListIterator<cluster> &itV = m_it[c];
		if (itV.valid()) {
			m_clusters.del(itV);
			itV = ListIterator<cluster>();
		}
	}


	//! Removes all clusters from \a S.
	/**
	 * After this operation, \a S is empty and still associated with the same clustered graph.
	 * The runtime of this operations is O(k), where k is the number of clusters in \a S
	 * before this operation.
	 */
	void clear() {
		ListIterator<cluster> it;
		for(it = m_clusters.begin(); it.valid(); ++it) {
			m_it[*it] = ListIterator<cluster>();
		}
		m_clusters.clear();
	}


	//! Returns true if cluster \p c is contained in \a S, false otherwise.
	/**
	 * This operation has constant runtime.
	 *
	 * \pre \p c is a cluster in the associated graph.
	 */
	bool isMember(cluster c) const {
		OGDF_ASSERT(c->graphOf() == m_it.graphOf());
		return m_it[c].valid();
	}

	//! Returns the size of \a S.
	/**
	 * This operation has constant runtime.
	 */
	int size() const {
		return m_clusters.size();
	}

	//! Returns a reference to the list of clusters contained in \a S.
	/**
	 * This list can be used for iterating over all clusters in \a S.
	 */
	const List<cluster> &clusters() const {
		return m_clusters;
	}

private:
	//! #m_it[\a c] contains the list iterator pointing to \a c if \a c is contained in \a S,
	//! an invalid list iterator otherwise.
	ClusterArray<ListIterator<cluster> > m_it;

	//! The list of clusters contained in \a S.
	List<cluster> m_clusters;
};

}
