/** \file
 * \brief Declaration and implementation of ogdf::FaceSet.
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

#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/SList.h>

namespace ogdf {

//! Face sets.
/**
 * @ingroup graph-containers
 *
 * Maintains a subset of faces contained in an associated combinatorial embedding.
 * Provides efficient operations for testing membership,
 * iteration, insertion and deletion of elements, as well as clearing the set.
 *
 * \tparam SupportFastSizeQuery Whether this set supports querying it's #size in
 * constant instead of linear time (in the size).
 *
 * \sa NodeSet
 */
template<bool SupportFastSizeQuery = true>
class FaceSet {
public:
	using ListType = typename std::conditional<SupportFastSizeQuery, List<face>, ListPure<face>>::type;

	//! Creates an empty face set associated with combinatorial embedding \p E.
	explicit FaceSet(const CombinatorialEmbedding &E) : m_it(E) { }

	//! Inserts face \p f into this set.
	/**
	 * This operation has constant runtime.
	 * If the face is already contained in this set, nothing happens.
	 *
	 * \pre \p f is a face in the associated combinatorial embedding.
	 */
	void insert(face f) {
		OGDF_ASSERT(f->embeddingOf() == m_it.embeddingOf());
		ListIterator<face> &itF = m_it[f];

		if (!itF.valid()) {
			itF = m_faces.pushBack(f);
		}
	}

	//! Removes face \p f from this set.
	/**
	 * This operation has constant runtime.
	 * If the face is not contained in this set, nothing happens.
	 *
	 * \pre \p f is a face in the associated combinatorial embedding.
	 */
	void remove(face f) {
		OGDF_ASSERT(f->embeddingOf() == m_it.embeddingOf());
		ListIterator<face> &itF = m_it[f];

		if (itF.valid()) {
			m_faces.del(itF);
			itF = ListIterator<face>();
		}
	}

	//! Removes all faces from this set-
	/**
	 * After this operation, this set is empty and still associated with the same combinatorial embedding.
	 * The runtime of this operations is linear in the #size().
	 */
	void clear() {
		m_it.init(embeddingOf());
		m_faces.clear();
	}

	//! Returns \c true iff face \p f is contained in this set.
	/**
	 * This operation has constant runtime.
	 *
	 * \pre \p f is a face in the associated combinatorial embedding.
	 */
	bool isMember(face f) const {
		OGDF_ASSERT(f->embeddingOf() == m_it.embeddingOf());
		return m_it[f].valid();
	}

	//! Returns a reference to the list of faces contained in this set.
	const ListType &faces() const {
		return m_faces;
	}

	//! Returns the associated combinatorial embedding
	const ConstCombinatorialEmbedding& embeddingOf() const {
		return *m_it.embeddingOf();
	}

	//! Returns the number of faces in this set.
	/**
	 * This operation has either linear or constant runtime, depending on \a SupportFastSizeQuery.
	 */
	int size() const {
		return m_faces.size();
	}

	//! Copy constructor.
	template<bool OtherSupportsFastSizeQuery>
	FaceSet(const FaceSet<OtherSupportsFastSizeQuery>& other) : m_it(other.embeddingOf()) {
		this = other;
	}

	//! Assignment operator.
	template<bool OtherSupportsFastSizeQuery>
	FaceSet &operator=(const FaceSet<OtherSupportsFastSizeQuery> &other) {
		m_faces.clear();
		m_it.init(other.embeddingOf());
		for(face f : other.faces()) {
			insert(f);
		}
	}

private:
	//! #m_it[\a f] contains the list iterator pointing to \a f if \a f is contained in S,
	//! or an invalid list iterator otherwise.
	FaceArray<ListIterator<face>> m_it;

	//! The list of faces contained in this set.
	ListType m_faces;
};

}
