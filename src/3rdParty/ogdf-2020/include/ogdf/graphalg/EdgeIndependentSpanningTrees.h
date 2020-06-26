/** \file
 * \brief Declaration of ogdf::EdgeIndependentSpanningTrees
 *
 * \author Manuel Fiedler
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

#include <vector>
#include <set>
#include <ogdf/basic/List.h>
#include <ogdf/basic/EdgeArray.h>

namespace ogdf {

/**
 * Calculates k edge-independent spanning trees of a graph.
 *
 * Given a root note, a set of k edge-independent spanning trees of
 * a graph G is a set of k spanning trees, for which for each two
 * trees the paths from any node to the root are edge-disjoint.
 */
class OGDF_EXPORT EdgeIndependentSpanningTrees {
public:
	/**
	 * The solution type
	 *
	 * Since each edge of the associated graph can be contained in up to
	 * two different spanning trees (traversed in opposite directions),
	 * the first and second members stored for each edge in the solution
	 * indicates the IDs of spanning trees containing them (0 meaning none).
	 */
	using Solution = EdgeArray<std::pair<unsigned int, unsigned int>>;

	//! Creates an instance of edge-independent spanning tree withou associated graph and root
	EdgeIndependentSpanningTrees() : m_G{nullptr}, m_root{nullptr} {}

	//! Creates an instance of edge-independent spanning tree and sets the graph
	EdgeIndependentSpanningTrees(const Graph &G) : EdgeIndependentSpanningTrees{G, G.firstNode()} {}

	//! Creates an instance of edge-independent spanning tree and sets the graph and root node
	EdgeIndependentSpanningTrees(const Graph &G, node root) : m_G{&G}, m_root{root} {}

	//! Destructor.
	~EdgeIndependentSpanningTrees() = default;

	//! Finds k edge-independent spanning trees in graph #m_G rooted at #m_root.
	bool findOne(unsigned int k, Solution &f) const;

	//! Finds all k edge-independent spanning trees in graph #m_G rooted at #m_root.
	List<Solution> findAll(unsigned int k) const;

	//! Finds all k edge-independent spanning trees in graph #m_G rooted at #m_root, including permutations.
	List<Solution> findAllPerm(unsigned int k) const;

	//! Returns a pointer to the associated graph.
	const Graph *getGraph() const {
		return m_G;
	}

	//! Sets the associated graph.
	void setGraph(const Graph &G) {
		OGDF_ASSERT(!G.empty());
		m_G = &G;
	}

	//! Returns the associated root node.
	node getRoot() const {
		return m_root;
	}

	//! Sets the associated root node.
	void setRoot(node root) {
		m_root = root;
	}

protected:
	//! Finds k edge-independent spanning trees and invokes \p func for each one,
	//! The search is stopped if \p func returns false.
	void findDo(unsigned int k, std::function<bool(Solution&)> func) const;

private:
	const Graph *m_G; //!< The associated graph.
	node m_root; //!< The associated root node.

	//! Takes two edge-independent spanning trees, permutes one and checks for them being unequal
	bool checkOnePermUnequal(const Solution &f1, const Solution &f2, const std::vector<unsigned int> &perm) const;

	//! Takes two edge-independent spanning trees and checks for them being unequal under all permutations
	bool checkNewTree(const Solution &f1, const Solution &f2, unsigned int k) const;

	//! Creates a parent relation, which for each node in the associated graph contains the adjEntry pointing towards the root node.
	bool createParentRel(const Solution &f, unsigned int j, NodeArray<adjEntry> &parent) const;

	//! Checks k spanning trees for independence
	bool checkIndependence(const std::vector<NodeArray<adjEntry>> &parents, unsigned int k) const;

	//! Checks two paths for independence
	bool checkTwoPathIndependence(const std::vector<NodeArray<adjEntry>> &parents, node v, unsigned int p1, unsigned int p2) const;

	//! Iterates over all subgraphs
	bool iterate(Solution &f, unsigned int j, unsigned int k) const;

	//! Checks whether iteration is finished
	bool isFinished(const Solution &f, unsigned int k) const;

	//! Creates the values needed for nextSpanningTree from \p f
	unsigned int createVals(const Solution &f, unsigned int k, std::vector<edge> &tree) const;

	//! Clears the j-th Tree from \p f
	void clearTree(Solution &f, unsigned int j) const;

	//! Calculates the next spanning tree after tree using backtracking
	bool nextSpanningTree(unsigned int &t, std::vector<edge> &tree) const;

	//! Checks whether \p v1 and \p v2 are connected in the subgraph
	bool pathExists(const std::vector<edge> &tree, node v1, node v2, unsigned int t) const;

	//! Checks whether \p e is in the subgraph
	bool isInSubGraph(const std::vector<edge> &sub, const edge &e, unsigned int t) const;

	//! Creates first k spanning trees
	bool createInitialSpanningTrees(Solution &f, unsigned int k) const;

	bool insertNewTree(Solution &f, unsigned int t, unsigned int j, std::vector<edge> &tree) const;

	//! Iterates using #nextSpanningTree until #insertNewTree succeeds
	bool findAndInsertNextTree(Solution &f, unsigned int &t, unsigned int j, std::vector<edge> &tree) const;
};

}
