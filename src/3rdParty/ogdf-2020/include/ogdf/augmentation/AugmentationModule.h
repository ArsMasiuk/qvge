/** \file
 * \brief Declaration of interface for graph augmentation algorithms
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

#include <ogdf/basic/Graph.h>

namespace ogdf {

/**
 * \brief The base class for graph augmentation algorithms.
 *
 * The class AugmentationModule is the base class for augmentation modules.
 * An augmentation module transforms an input graph \a G into an output
 * graph \a G' by adding edges, such that \a G' has a certain
 * property, e.g., biconnected.
 *
 * <H3>Implementation of Augmentation Algorithms</H3>
 * An implementation of an augmentation module must override
 * the protected method doCall(G,L), which gets as
 * input a graph reference \a G. It then adds the augmented edges
 * to \a G and returns the list of added edges in \a L.
 */

class OGDF_EXPORT AugmentationModule {
public:
	//! Initializes an augmentation module.
	AugmentationModule() : m_nAddedEdges(0) { }
	// destruction
	virtual ~AugmentationModule() { }

	//! Calls the augmentation module for graph \p G.
	void call(Graph& G) {
		List<edge> L;
		call(G,L);
	}

	//! Calls the augmentation module for graph \p G.
	void operator()(Graph& G) { call(G); }

	/**
	 * \brief Calls the augmentation module for graph \p G.
	 *
	 * Returns the list of added edges in \p L.
	 */
	void call(Graph& G, List<edge> &L) {
		doCall(G,L);
		m_nAddedEdges = L.size();
	}

	/**
	 * \brief Calls the augmentation module for graph \p G.
	 *
	 * Returns the list of added edges in \p L.
	 */
	void operator()(Graph& G, List<edge> &L) { call(G,L); }

	//! Returns the number of added edges.
	int numberOfAddedEdges() const {
		return m_nAddedEdges;
	}

protected:
	/**
	 * \brief Implements the augmentation algorithm for graph \p G.
	 *
	 * Returns the list of added edges in \p L.
	 */
	virtual void doCall(Graph& G, List<edge> &L) = 0;

private:
	int m_nAddedEdges;

	OGDF_MALLOC_NEW_DELETE
};

}
