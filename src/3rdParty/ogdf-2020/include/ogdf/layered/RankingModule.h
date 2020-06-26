/** \file
 * \brief Declaration of interface for ranking algorithms.
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
 * \brief Interface of algorithms for computing a node ranking.
 *
 * \see SugiyamaLayout
 */
class OGDF_EXPORT RankingModule {
public:
	//! Initializes a ranking module.
	RankingModule() { }

	virtual ~RankingModule() { }

	/**
	 * \brief Computes a node ranking of the digraph \p G in \p rank.
	 *
	 * This method is the actual algorithm call and must be implemented by
	 * derived classes.
	 *
	 * @param G is the input digraph.
	 * @param rank is assigned the node ranking.
	 */
	virtual void call(const Graph &G, NodeArray<int> &rank) = 0;

	virtual void call(const Graph &G, const EdgeArray<int> & /* length */, const EdgeArray<int> & /* cost */, NodeArray<int> &rank)
	{
		call(G, rank);
	}

	/**
	 * \brief Computes a node ranking of the digraph \p G in \p rank.
	 *
	 * @param G is the input digraph.
	 * @param rank is assigned the node ranking.
	 */
	void operator()(const Graph &G, NodeArray<int> &rank) {
		call(G,rank);
	}

	OGDF_MALLOC_NEW_DELETE
};

}
