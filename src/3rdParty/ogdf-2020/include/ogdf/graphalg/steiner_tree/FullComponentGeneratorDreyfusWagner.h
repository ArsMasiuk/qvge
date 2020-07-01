/** \file
 * \brief Definition of the ogdf::steiner_tree::FullComponentGeneratorDreyfusWagner class template
 *
 * \author Stephan Beyer
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

#include <ogdf/basic/Hashing.h>
#include <ogdf/basic/Math.h>
#include <ogdf/basic/SubsetEnumerator.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>

namespace ogdf {
namespace steiner_tree {

/**
 * A generator for restricted full components (for Steiner tree approximations)
 * based on the Dreyfus-Wagner algorithm
 *
 * This generator can handle (and exploit) predecessor matrices that use \c nullptr
 * instead of resembling shortest paths over terminals. (See the terminal-preferring
 * shortest path algorithms in ogdf::MinSteinerTreeModule<T>.)
 */
template<typename T>
class FullComponentGeneratorDreyfusWagner
{
	const EdgeWeightedGraph<T> &m_G; //!< A reference to the graph instance
	const List<node> &m_terminals; //!< A reference to the index-sorted list of terminals
	const NodeArray<bool>& m_isTerminal; //!< A reference to the terminal incidence vector
	const NodeArray<NodeArray<T>> &m_distance; //!< A reference to the full distance matrix
	const NodeArray<NodeArray<edge>> &m_pred; //!< A reference to the full predecessor matrix
	SubsetEnumerator<node> m_terminalSubset; //!< Handling subsets of terminals

	using NodePairs = ArrayBuffer<NodePair>;

	//! Subgraphs (given by other subgraphs and additional node pairs) and their cost for a partial solution
	struct DWMData {
		T cost;
		NodePairs nodepairs;
		ArrayBuffer<const DWMData*> subgraphs;
		DWMData(T _cost, NodePairs _nodepairs) : cost(_cost), nodepairs(_nodepairs) {}
		explicit DWMData(T _cost = std::numeric_limits<T>::max()) : cost(_cost) {}

		//! Invalidates the data
		void invalidate() {
			nodepairs.clear();
			subgraphs.clear();
		}

		//! Returns true iff the data is valid
		bool valid() const {
			return cost == 0 || !(nodepairs.empty() && subgraphs.empty());
		}

		//! Adds \p other subgraph to ours
		void add(const DWMData* other) {
			if (valid()) {
				if (other->valid()) {
					subgraphs.push(other);
				} else {
					invalidate();
				}
			}
			cost += other->cost;
		}

		//! Remove all subgraphs and edges and set cost to zero
		void clear() {
			invalidate();
			cost = 0; // make it valid again
		}

		//! Adds a \p nodepair of cost \p c
		void add(NodePair nodepair, T c) {
			if (valid()) {
				nodepairs.push(nodepair);
			}
			cost += c;
		}
	};

	//! A collection of two subgraphs and their total cost
	struct DWMSplit {
		T cost = std::numeric_limits<T>::max();
		const DWMData* subgraph1 = nullptr;
		const DWMData* subgraph2 = nullptr;

		//! Sets #subgraph1 and #subgraph2 and computes #cost
		void set(const DWMData* s1, const DWMData* s2) {
			subgraph1 = s1;
			subgraph2 = s2;
			cost = s1->cost + s2->cost;
		}
	};

	//! Hash function to save partial solutions by terminal set (sorted node list)
	class SortedNodeListHashFunc;

	//! A hash array for keys of size > 2
	Hashing<List<node>, DWMData, SortedNodeListHashFunc> m_map;

	//! Returns a pointer to the relevant data of the partial solution given by \p key
	const DWMData* dataOf(const List<node> &key) const {
		OGDF_ASSERT(key.size() > 1);
		OGDF_ASSERT(m_map.member(key));
		return &m_map.lookup(key)->info();
	}

	//! Returns the cost of the partial solution given by \p key
	T costOf(const List<node> &key) const {
		OGDF_ASSERT(key.size() > 1);
		if (key.size() == 2) { // a shortcut to avoid using the hash table
#ifdef OGDF_FULL_COMPONENT_GENERATION_TERMINAL_SSSP_AWARE
			if (m_isTerminal[key.front()]) {
				return m_distance[key.front()][key.back()];
			} else {
				OGDF_ASSERT(m_isTerminal[key.back()]);
				return m_distance[key.back()][key.front()];
			}
#else
			return m_distance[key.front()][key.back()];
#endif
		}
		return dataOf(key)->cost;
	}

	//! Checks overflow-safe if \p summand1 plus \p summand2 is less than \p compareValue
	bool safeIfSumSmaller(const T summand1, const T summand2, const T compareValue) const
	{
#ifdef OGDF_FULL_COMPONENT_GENERATION_ALWAYS_SAFE
		return summand1 + summand2 < compareValue;
#else
		return summand1 < std::numeric_limits<T>::max()
		    && summand2 < std::numeric_limits<T>::max()
		    && summand1 + summand2 < compareValue;
#endif
	}

	/**
	 * Is being used as a callback to ogdf::SubsetEnumerator's forEach* methods
	 * to get the subset plus a correctly inserted \p newNode (ie, sorted by index)
	 * into \p list. This takes linear time in comparison to copy, insert, sort
	 * which takes average case O(n log n) time.
	 *
	 * @param w Node argument for the callback
	 * @param list Resulting list
	 * @param inserted Whether \p newNode was inserted; must be initialized to \c false
	 * @param newNode New node to be inserted into the list
	 */
	static void sortedInserter(node w, List<node> &list, bool &inserted, node newNode)
	{
		if (!inserted && w->index() > newNode->index()) {
			list.pushBack(newNode);
			inserted = true;
		}
		list.pushBack(w);
	}

	//! Makes a list from the current terminal subset including an correctly inserted node \p v
	void makeKey(List<node>& newSubset, node v) const
	{
		bool inserted = false;
		m_terminalSubset.forEachMember([&](node w) { sortedInserter(w, newSubset, inserted, v); });
		if (!inserted) {
			newSubset.pushBack(v);
		}
	}

	//! Makes a list from \p subset and its complement, each including an correctly inserted node \p v
	void makeKey(List<node>& newSubset, List<node>& newComplement, const SubsetEnumerator<node>& subset, node v) const
	{
		bool insertedIntoSubset = false;
		bool insertedIntoComplement = false;
		// Interestingly std::bind is much slower than using lambdas (at least on g++ 6.3)
		subset.forEachMemberAndNonmember(
		    [&](node w) { sortedInserter(w, newSubset, insertedIntoSubset, v); },
		    [&](node w) { sortedInserter(w, newComplement, insertedIntoComplement, v); });
		if (!insertedIntoSubset) {
			newSubset.pushBack(v);
		}
		if (!insertedIntoComplement) {
			newComplement.pushBack(v);
		}
	}

	/**
	 * Populates \p split that contains a partial solution for an included nonterminal \p v in #m_G
	 *
	 * Note that it is not guaranteed that any resulting collection of node pairs represents a tree.
	 */
	void computeSplit(NodeArray<DWMSplit> &split, node v, SubsetEnumerator<node> &subset) const {
		if (split[v].subgraph1 != nullptr) { // already computed
			return;
		}

		DWMSplit& best = split[v];
		for (subset.begin(1, subset.numberOfMembersAndNonmembers() / 2); subset.valid(); subset.next()) {
			List<node> newSubset, newComplement;
			makeKey(newSubset, newComplement, subset, v);

			if (safeIfSumSmaller(costOf(newSubset), costOf(newComplement), best.cost)) {
				best.set(dataOf(newSubset), dataOf(newComplement));
			}
		}
	}

	//! Computes a partial solution for given \p terminals and node \p v
	void computePartialSolution(NodeArray<DWMSplit> &split,
			node v,
			SubsetEnumerator<node> &subset,
			const List<node> &terminals)
	{
		List<node> newSubset;
		makeKey(newSubset, v);

		if (!m_map.member(newSubset)) { // not already defined
			T oldCost = costOf(terminals);
			DWMData best;
			auto addPair = [&](node v1, node v2, T dist) {
				best.add(NodePair(v1, v2), dist);
				if (m_pred[v1][v2] == nullptr) {
					best.invalidate();
				}
			};
			for (node w : m_G.nodes) {
				T dist = m_distance[v][w];
				if (m_terminalSubset.hasMember(w)) {
					// we attach edge vw to tree containing terminal w
					if (safeIfSumSmaller(oldCost, dist, best.cost)) {
						best.clear();
						best.add(dataOf(terminals));
						addPair(v, w, dist);
					}
				} else {
					// we attach edge vw to tree split[w]
					OGDF_ASSERT(!m_terminalSubset.hasMember(v));
					computeSplit(split, w, subset);
					if (safeIfSumSmaller(split[w].cost, dist, best.cost)) {
						best.clear();
						best.add(split[w].subgraph1);
						best.add(split[w].subgraph2);
						if (v != w) {
							addPair(v, w, dist);
						}
					}
				}
			}
			m_map.fastInsert(newSubset, best);
		}
	}

	//! Computes all partial solutions for given #m_terminalSubset
	template<typename CONTAINER>
	void computePartialSolutions(const CONTAINER& nodeContainer) {
		List<node> terminals;
		m_terminalSubset.list(terminals);
		SubsetEnumerator<node> subset(terminals); // done here because of linear running time
		NodeArray<DWMSplit> split(m_G);
		for (node v : nodeContainer) {
			if (!m_terminalSubset.hasMember(v)) {
				computePartialSolution(split, v, subset, terminals);
			}
		}
	}

	//! Initializes the hash array with all node-terminal-pairs
	void initializeMap() {
		for (node v : m_G.nodes) {
			for (node t : m_terminals) {
				if (t != v) {
					List<node> key;
					key.pushBack(t);
					if (v->index() < t->index()) {
						key.pushFront(v);
					} else {
						key.pushBack(v);
					}

					if (!m_map.member(key)) { // not already defined
						if (m_pred[t][v] == nullptr) {
							m_map.fastInsert(key, DWMData(m_distance[t][v]));
							OGDF_ASSERT(!dataOf(key)->valid() || m_distance[t][v] == 0);
						} else {
							NodePairs nodepairs;
							nodepairs.push(NodePair(key.front(), key.back()));
							m_map.fastInsert(key, DWMData(m_distance[t][v], nodepairs));
							OGDF_ASSERT(dataOf(key)->valid());
						}
					}
				}
			}
		}
	}

	//! Adds edges to construct a Steiner \p tree from the given \p data (recursively) if the \p data is valid
	T getSteinerTreeFor(const DWMData& data, EdgeWeightedGraphCopy<T>& tree) const {
		T cost(0);
		if (data.valid()) {
			// add edges
			for (auto nodepair : data.nodepairs) {
				node uO = nodepair.source;
				node vO = nodepair.target;
				node uC = tree.copy(uO);
				node vC = tree.copy(vO);
				if (uC == nullptr) {
					uC = tree.newNode(uO);
				}
				if (vC == nullptr) {
					vC = tree.newNode(vO);
				}
#ifdef OGDF_FULL_COMPONENT_GENERATION_TERMINAL_SSSP_AWARE
				const T dist = m_isTerminal[uO] ? m_distance[uO][vO] : m_distance[vO][uO];
#else
				const T dist = m_distance[uO][vO];
#endif
				tree.newEdge(uC, vC, dist);
				cost += dist;
			}

			// recurse
			for (const DWMData* subgraph : data.subgraphs) {
				cost += getSteinerTreeFor(*subgraph, tree);
			}
		}

		OGDF_ASSERT(isAcyclicUndirected(tree));
		return cost;
	}

public:
	/** The constructor
	 * \pre The list of terminals has to be sorted by index (use MinSteinerTreeModule<T>::sortTerminals)
	 */
	FullComponentGeneratorDreyfusWagner(const EdgeWeightedGraph<T>& G,
			const List<node>& terminals, const NodeArray<bool>& isTerminal,
			const NodeArray<NodeArray<T>>& distance, const NodeArray<NodeArray<edge>>& pred)
	  : m_G(G)
	  , m_terminals(terminals)
	  , m_isTerminal(isTerminal)
	  , m_distance(distance)
	  , m_pred(pred)
	  , m_terminalSubset(m_terminals)
	  , m_map(1 << 22) // we initially allocate 4MB*sizeof(DWMData) for hashing
	{
		initializeMap();
	}

	void call(int restricted)
	{
		OGDF_ASSERT(restricted >= 2);
		Math::updateMin(restricted, m_terminals.size());
		for (m_terminalSubset.begin(2, restricted-1); m_terminalSubset.valid(); m_terminalSubset.next()) {
			if (m_terminalSubset.size() != restricted - 1) {
				computePartialSolutions(m_G.nodes);
			} else { // maximal terminal subset
				// save time by only adding terminals instead of all nodes
				computePartialSolutions(m_terminals);
			}
		}
	}

	//! Constructs a Steiner tree for the given set of terminals if it is valid,
	//! otherwise an empty tree is returned
	T getSteinerTreeFor(const List<node>& terminals, EdgeWeightedGraphCopy<T>& tree) const {
		tree.createEmpty(m_G);
		T cost(getSteinerTreeFor(*dataOf(terminals), tree));
		OGDF_ASSERT(isTree(tree));
		return cost;
	}

	//! Checks if a given \p graph is a valid full component
	bool isValidComponent(const EdgeWeightedGraphCopy<T>& graph) const {
		if (graph.empty()) {
			return false;
		}
		for (node v : graph.nodes) {
			if (m_isTerminal[graph.original(v)] // is a terminal
			 && v->degree() > 1) { // but not a leaf
				return false;
			}
		}
		return true;
	}
};

template<typename T>
class FullComponentGeneratorDreyfusWagner<T>::SortedNodeListHashFunc
{
	static const unsigned int c_prime = 0x7fffffff; // mersenne prime 2**31 - 1
	// would be nicer: 0x1fffffffffffffff; // mersenne prime 2**61 - 1
	const int m_random;

public:
	//! Initializes the random number
	SortedNodeListHashFunc()
	  : m_random(randomNumber(2, c_prime - 1))
	{
	}

	//! The actual hash function
	unsigned int hash(const List<node> &key) const
	{
		unsigned int hash = 0;
		for (node v : key) {
			hash = (hash * m_random + v->index()) % c_prime;
		}
		return hash;
	}
};

}
}
