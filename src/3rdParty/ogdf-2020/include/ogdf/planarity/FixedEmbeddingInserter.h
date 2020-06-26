/** \file
 * \brief Declaration of class FixedEmbeddingInserter.
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

#include <ogdf/planarity/EdgeInsertionModule.h>
#include <ogdf/planarity/RemoveReinsertType.h>

namespace ogdf {

/**
 * Inserts edges optimally into an embedding.
 *
 * Iteratively inserts edges (optimally for each insertion step) into a possibly fixed embedding.
 * Note that by default the embedding is not fixed and will be changed regardless of the input.
 * If the input can be guaranteed to be a planar embedding you may flip #keepEmbedding to preserve it.
 *
 * @ingroup ga-insert
 */
class OGDF_EXPORT FixedEmbeddingInserter : public EdgeInsertionModule
{
public:
	//! Creates an instance of fixed embedding edge inserter with default settings.
	FixedEmbeddingInserter();

	//! Creates an instance of fixed embedding edge inserter with the same settings as \p inserter.
	FixedEmbeddingInserter(const FixedEmbeddingInserter &inserter);

	//! Destructor.
	~FixedEmbeddingInserter() { }

	//! Returns a new instance of the fixed embedding inserter with the same option settings.
	virtual EdgeInsertionModule *clone() const override;

	//! Assignment operator. Copies option settings only.
	FixedEmbeddingInserter &operator=(const FixedEmbeddingInserter &inserter);


	/**
	 *  @name Optional parameters
	 *  @{
	 */

	/**
	 * Sets the remove-reinsert postprocessing method.
	 *
	 * This might alter the input embedding.
	 */
	void removeReinsert(RemoveReinsertType rrOption) {
		m_rrOption = rrOption;
	}

	//! Returns the current setting of the remove-reinsert postprocessing method.
	RemoveReinsertType removeReinsert() const {
		return m_rrOption;
	}


	//! Sets the option <i>percentMostCrossed</i> to \p percent.
	/**
	 * This option determines the portion of most crossed edges used if the remove-reinsert
	 * method is set to RemoveReinsertType::MostCrossed. This portion is number of edges * percentMostCrossed() / 100.
	 */
	void percentMostCrossed(double percent) {
		m_percentMostCrossed = percent;
	}

	//! Returns the current setting of option percentMostCrossed.
	double percentMostCrossed() const {
		return m_percentMostCrossed;
	}

	//! Sets the option <i>keepEmbedding</i> to \p keep.
	/**
	 * This option determines if the planar embedding of the planarized representation \a PG passed to the call-method
	 * is preserved, or if always a new embedding is computed. If <i>keepEmbedding</i> is set to true,
	 * \a PG must always be planarly embedded.
	 */
	void keepEmbedding(bool keep) {
		m_keepEmbedding = keep;
	}

	//! Returns the current setting of option <i>keepEmbedding</i>.
	bool keepEmbeding() const {
		return m_keepEmbedding;
	}

	/** @}
	 *  @name Further information
	 *  @{
	 */

	//! Returns the number of runs performed by the remove-reinsert method after the algorithm has been called.
	int runsPostprocessing() const {
		return m_runsPostprocessing;
	}

	//! @}

private:
	//! Implements the algorithm call.
	virtual ReturnType doCall(
		PlanRepLight              &pr,
		const Array<edge>         &origEdges,
		const EdgeArray<int>      *costOrig,
		const EdgeArray<bool>     *pForbiddenOrig,
		const EdgeArray<uint32_t> *pEdgeSubGraphs) override;

	RemoveReinsertType m_rrOption; //!< The remove-reinsert method.
	double m_percentMostCrossed;   //!< The portion of most crossed edges considered.
	bool m_keepEmbedding;

	int m_runsPostprocessing; //!< Runs of remove-reinsert method.
};

}
