/** \file
 * \brief The Sparse Table Algorithm for the Least Common Ancestor
 *  problem as proposed by Bender and Farach-Colton.
 *
 * \author Matthias Woste
 * Porting to OGDF Graph class based trees by Stephan Beyer
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
 * Implements the <O(\a n log \a n), O(1)>-time "sparse table" algorithm
 * by Bender and Farach-Colton to compute lowest common ancestors (LCAs)
 * in arborescences (*not* arbitrary directed acyclic graphs).
 *
 * This implementation is based on:
 *
 * (M. Bender, M. Farach-Colton, The %LCA problem revisited, LATIN '00, volume 1776 of LNCS,
 * pages 88-94, Springer, 2000)
 *
 * @ingroup ga-tree
 */
class OGDF_EXPORT LCA {
public:
	/**
	 * Builds the %LCA data structure for an arborescence
	 *
	 * If \p root is not provided, it is computed in additional O(\a n) time.
	 *
	 * @param G an arborescence
	 * @param root optional root of the arborescence
	 * @pre Each node in \p G is reachable from the root via a unique directed path, that is, \p G is an arborescence.
	 */
	explicit LCA(const Graph &G, node root = nullptr);

	/**
	 * Returns the %LCA of two nodes \p u and \p v.
	 *
	 * If \p u and \p v are the same node, \p u itself is defined
	 * to be the %LCA, not its father.
	 */
	node call(node u, node v) const;

	//! Returns the level of a node. The level of the root is 0.
	int level(node v) const
	{
		return m_n == 1 ? 0 : m_level[m_representative[v]];
	}

private:
	const node m_root; //!< the root of the tree
	const int m_n; //!< number of nodes in graph
	const int m_len; //!< length of the RMQ array (always 2 m_n - 1)
	const int m_rangeJ; //!< always floor(log(#m_len)) (size of a row in the table)
	Array<node> m_euler; //!< Euler[i] is i-th node visited in Euler Tour
	NodeArray<int> m_representative; //!< Euler[Representative[v]] = v
	Array<int> m_level; //!< L[i] is distance of node E[i] from root
	Array<int> m_table; //!< preprocessed M[i,j] array

	/**
	 * Performs an Euler tour (actually a DFS with virtual back-edges) through the underlying tree
	 * and fills Euler tour and %Level arrays.
	 */
	void dfs(const Graph &G, node root);

	/**
	 * Fills the O(\a n log \a n)-space matrix with auxiliary data
	 * the %LCA values can be computed from.
	 */
	void buildTable();

	/**
	 * Access the sparse table at [\p i, \p j]
	 *
	 * @pre 0 <= \p i < #m_len and 1 <= \p j <= #m_rangeJ
	 */
	//! @{
	const int &sparseTable(int i, int j) const
	{
		return m_table[i * m_rangeJ + j - 1];
	}
	int &sparseTable(int i, int j)
	{
		return m_table[i * m_rangeJ + j - 1];
	}
	//! @}

	/**
	 * Returns the internal index pointing to the %LCA between two nodes \p u and \p v
	 * @return Internal index pointing to %LCA
	 */
	int rmq(int u, int v) const;
};

}
