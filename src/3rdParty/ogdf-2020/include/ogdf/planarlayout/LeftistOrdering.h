/** \file
 * \brief Declares the class LeftistOrdering...
 *
 * ... that computes a leftist canonical ordering as described by Badent et al. in More Canonical Ordering
 *
 * \author Martin Gronemann
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

#include <ogdf/planarlayout/ShellingOrderModule.h>
#include <ogdf/basic/AdjEntryArray.h>

namespace ogdf {

// context class for the leftist canonical ordering algorithm
class LeftistOrdering
{
protected:

	// struct for a candidate aka belt item
	struct Candidate
	{
		Candidate() : stopper(nullptr) {};

		// the edges in the belt item
		List<adjEntry> chain;

		// a possible stopper of the candidate
		node stopper;
	};

public:
	// computes the leftist canonical order. Requires that G is simple, triconnected and embedded.
	// adj_v1n is the adjEntry at v_1 looking towards v_n, the outerface is choosen such that v_2 is the cyclic pred
	// of v_n. the result is saved in result, a list of list of nodes, first set is v_1, v_2, last one is v_n.
	bool call(const Graph& G, adjEntry adj_v1n, List<List<node> >& result);

private:
	// the leftmost feasible candidate function from the paper
	bool leftmostFeasibleCandidate(List<node>& result);

	// this is used to check a candidate for a singleton copy
	bool isSingletonWith(const Candidate& c,node v) const;

	// update belt function from the paper
	void updateBelt();

	// belt extension function from the paper
	void beltExtension(List<Candidate>& extension);

	// returns true if v is forbidde
	bool forbidden(node v) const
	{
		// too many cut faces?
		return m_cutFaces[v] > m_cutEdges[v] + 1;
	}

	// returns true if v is singular
	bool singular(node v) const
	{
		// not more cutfaces then cut edges plus one ?
		return m_cutFaces[v] > 2 && m_cutFaces[v] == m_cutEdges[v] + 1;
	}

	// the belt
	List<Candidate> m_belt;

	// the curr candidate in the belt
	List<Candidate>::iterator m_currCandidateIt;

	// number of cutfaces incident to a vertex
	NodeArray<int> m_cutFaces;

	// number of cutedges incident to a vertex
	NodeArray<int> m_cutEdges;

	// flag for marking directed edges
	AdjEntryArray<bool> m_marked;

public:
	// this is a custom class to have a more convienent way to access a canonical ordering
	// used somewhere
	class Partitioning
	{
	public:
		Partitioning() { }
		Partitioning(const Graph& G, const List<List<node> >& lco) { buildFromResult(G, lco); }

		void buildFromResult(const Graph& G, const List<List<node> >& lco);

		// returns the adjEntry to the left node in G_k-1
		adjEntry left(int k) const
		{
			if (m_ears[k][0])
				return m_ears[k][0]->twin();
			else
				return nullptr;
		}

		// returns the adjEntry to the left node in G_k-1
		adjEntry right(int k) const
		{
			return m_ears[k][m_ears[k].size()-1];
		}

		// returns the edge from v_i to v_i+1 in the k-th partition
		adjEntry getChainAdj(int k, int i) const
		{
			return m_ears[k][i+1];
		}

		adjEntry getPathAdj(int k, int i) const
		{
			return m_ears[k][i];
		}

		node getNode(int k, int i) const
		{
			return m_ears[k][i+1]->theNode();
		}

		// returns the number of all partitions
		int numPartitions() const
		{
			return m_ears.size();
		}

		// returns the number of nodes in partition k
		int numNodes(int k) const
		{
			return m_ears[k].size() - 1;
		}

		int pathLength(int k) const
		{
			return m_ears[k].size();
		}

		bool isSingleton(int k) const
		{
			return numNodes(k)==1;
		}

	private:
		// keeps for every partition the path from left, v_1, ... v_k, right
		Array< Array<adjEntry> > m_ears;
	};

	bool call(const Graph& G, adjEntry adj_v1n, Partitioning& partition)
	{
		// the simple result
		List<List<node> > result;
		// compute it
		if (!call(G, adj_v1n, result))
			return false;

		// generate the comfortable partitioning
		partition.buildFromResult(G, result);

		// success hopefully..
		return true;
	}
};

}
