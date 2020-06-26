/** \file
 * \brief Declaration of coffman graham ranking algorithm for Sugiyama
 *        algorithm.
 *
 * \author Till Schäfer
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

#include <ogdf/layered/RankingModule.h>
#include <ogdf/layered/AcyclicSubgraphModule.h>
#include <ogdf/basic/NodeArray.h>
#include <memory>
#include <ogdf/basic/tuples.h>

namespace ogdf {

//! The coffman graham ranking algorithm.
/**
 * @ingroup gd-ranking
 *
 * The class CoffmanGrahamRanking implements a node ranking algorithmn based on
 * the coffman graham scheduling algorithm, which can be used as first phase
 * in SugiyamaLayout. The aim of the algorithm is to ensure that the height of
 * the ranking (the number of layers) is kept small.
 */
class OGDF_EXPORT CoffmanGrahamRanking : public RankingModule {

public:
	//! Creates an instance of coffman graham ranking.
	CoffmanGrahamRanking();


	/**
	 *  @name Algorithm call
	 *  @{
	 */

	//! Computes a node ranking of \p G in \p rank.
	virtual void call(const Graph &G, NodeArray<int> &rank) override;


	/** @}
	 *  @name Module options
	 *  @{
	 */

	//! Sets the module for the computation of the acyclic subgraph.
	void setSubgraph(AcyclicSubgraphModule *pSubgraph) {
		m_subgraph.reset(pSubgraph);
	}

	//! @}

	//! Get for the with
	int width() const {
		return m_w;
	}

	//! Set for the with
	void width (int w) {
		m_w = w;
	}


private:
	// CoffmanGraham data structures
	class _int_set {
		int* m_array;
		int m_length;
		int m_index;
	public:
		_int_set() : m_array(nullptr), m_length(0), m_index(0) { }
		explicit _int_set(int len) : m_array(nullptr), m_length(len), m_index(len) {
			if (len > 0)
				m_array = new int[m_length];
		}
		~_int_set() { delete[] m_array; }

		void init(int len) {
			delete m_array;
			if ((m_length = len) == 0)
				m_array = nullptr;
			else
				m_array = new int[m_length];
			m_index = len;
		}

		int length() const {
			return m_length;
		}

		int operator[](int i) const {
			return m_array[i];
		}

		void insert(int x) {
			m_array[--m_index] = x;
		}

		bool ready() const {
			return m_index == 0;
		}
	};

	// CoffmanGraham members
	std::unique_ptr<AcyclicSubgraphModule> m_subgraph;
	int m_w;
	NodeArray<_int_set> m_s;

	// dfs members
	NodeArray<int> m_mark;

	// CoffmanGraham funktions
	void insert (node u, List<Tuple2<node,int> > &ready_nodes);
	void insert (node u, List<node> &ready, const NodeArray<int> &pi);

	// dfs funktions
	void removeTransitiveEdges (Graph& G);
	void dfs(node v);
};

}
