/** \file
 * \brief Definition of the ogdf::steiner_tree::FullComponentGeneratorDreyfusWagnerWithoutMatrix class template
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
#include <ogdf/graphalg/MinSteinerTreeModule.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>

namespace ogdf {
namespace steiner_tree {

/**
 * A generator for restricted full components (for Steiner tree approximations)
 * based on the Dreyfus-Wagner algorithm
 * that does not need a precomputed all-pair-shortest-paths matrix
 * because single-source-shortest-path are used within.
 *
 * See also
 * R. E. Erickson, C. L. Monma, A. F. Veinott:
 * Send-and-split method for minimum-concave-cost network flows.
 * Math. Oper. Res. 12 (1987) 634-664.
 */
template<typename T>
class FullComponentGeneratorDreyfusWagnerWithoutMatrix {
	const EdgeWeightedGraph<T>& m_G; //!< A reference to the graph instance
	const List<node>& m_terminals; //!< A reference to the index-sorted list of terminals
	const NodeArray<bool>& m_isTerminal; //!< A reference to the terminal incidence vector

	//! Necessary because ogdf::EdgeWeightedGraphCopy<T> is rubbish
	class AuxiliaryGraph {
		const EdgeWeightedGraph<T>& m_original; //!< A reference to the original graph
		EdgeWeightedGraph<T> m_copy; //!< The auxiliary copy
		NodeArray<node> m_copyOfNode; //!< A mapping from original nodes to copied nodes
		NodeArray<node> m_origOfNode; //!< A mapping from copied nodes to original nodes
		EdgeArray<edge> m_origOfEdge; //!< A mapping from copied edges to original edges
		NodeArray<bool> m_isTerminal; //!< True for terminals in the auxiliary graph

		node m_source; //!< The source node

	public:
		//! Constructs a copy of the original graph with an added source node having edges to all other nodes
		AuxiliaryGraph(const EdgeWeightedGraph<T>& orig, const List<node>& terminals)
		  : m_original(orig), m_copyOfNode(m_original),
		    m_origOfNode(m_copy, nullptr), m_origOfEdge(m_copy, nullptr),
		    m_isTerminal(m_copy, false) {
			// copy nodes and edges
			for (node v : m_original.nodes) {
				node vCopy = m_copy.newNode();
				m_copyOfNode[v] = vCopy;
				m_origOfNode[vCopy] = v;
			}
			for (edge e : m_original.edges) {
				edge eCopy = m_copy.newEdge(copy(e->source()), copy(e->target()), m_original.weight(e));
				m_origOfEdge[eCopy] = e;
			}

			// set terminals
			for (node v : terminals) {
				m_isTerminal[copy(v)] = true;
			}

			// initialize source node and its edges to every other node
			m_source = m_copy.newNode();
			for (node w : m_original.nodes) {
				m_copy.newEdge(m_source, copy(w), std::numeric_limits<T>::max());
			}
		}

		//! Returns the copied node of the original node \p v
		node copy(node v) const {
			OGDF_ASSERT(v->graphOf() == &m_original);
			return m_copyOfNode[v];
		}

		//! Returns the original node for the copied node \p v
		node original(node v) const {
			OGDF_ASSERT(v->graphOf() == &m_copy);
			return m_origOfNode[v];
		}

		//! Returns the original edge for the copied edge \p e
		edge original(edge e) const {
			OGDF_ASSERT(e->graphOf() == &m_copy);
			return m_origOfEdge[e];
		}

		//! Returns the source node
		node source() const {
			return m_source;
		}

		//! Returns a const reference to the graph
		const EdgeWeightedGraph<T>& graph() const {
			return m_copy;
		}

		//! Returns a const reference to #m_isTerminal
		const NodeArray<bool>& terminalArray() const {
			return m_isTerminal;
		}

		//! Returns the weight of a copied edge
		T weight(edge e) const {
			OGDF_ASSERT(e->graphOf() == &m_copy);
			return m_copy.weight(e);
		}

		//! Sets the weight of a copied edge
		void setWeight(edge e, T value) {
			OGDF_ASSERT(e->graphOf() == &m_copy);
			m_copy.setWeight(e, value);
		}
	};

	AuxiliaryGraph m_auxG; //!< An auxiliary graph to compute partial solutions
	SubsetEnumerator<node> m_terminalSubset; //!< Handling subsets of terminals

	//! Subgraphs (given by other subgraphs and additional edges) and their cost for a partial solution
	struct DWMData {
		T cost;
		ArrayBuffer<edge> edges;
		ArrayBuffer<const DWMData*> subgraphs;
		DWMData(T _cost, ArrayBuffer<edge> _edges) : cost(_cost), edges(_edges) {}
		explicit DWMData(T _cost = std::numeric_limits<T>::max()) : cost(_cost) {}

		//! Invalidates the data
		void invalidate() {
			edges.clear();
			subgraphs.clear();
		}

		//! Returns true iff the data is valid
		bool valid() const {
			return cost == 0 || !(edges.empty() && subgraphs.empty());
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

		//! Adds an edge \p e of cost \p c
		void add(edge e, T c) {
			if (valid()) {
				edges.push(e);
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
	const DWMData* dataOf(const List<node>& key) const {
		OGDF_ASSERT(key.size() > 1);
		OGDF_ASSERT(m_map.member(key));
		return &m_map.lookup(key)->info();
	}

	//! Returns the cost of the partial solution given by \p key
	T costOf(const List<node>& key) const {
		OGDF_ASSERT(key.size() > 1);
		return dataOf(key)->cost;
	}

	//! Checks overflow-safe if \p summand1 plus \p summand2 is less than \p compareValue
	bool safeIfSumSmaller(const T summand1, const T summand2, const T compareValue) const {
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
	static void sortedInserter(node w, List<node>& list, bool& inserted, node newNode) {
		if (!inserted && w->index() > newNode->index()) {
			list.pushBack(newNode);
			inserted = true;
		}
		list.pushBack(w);
	}

	//! Makes a list from the current terminal subset including an correctly inserted node \p v
	void makeKey(List<node>& newSubset, node v) const {
		bool inserted = false;
		m_terminalSubset.forEachMember([&](node w) { sortedInserter(w, newSubset, inserted, v); });
		if (!inserted) {
			newSubset.pushBack(v);
		}
	}

	//! Makes a list from \p subset and its complement, each including an correctly inserted node \p v
	void makeKey(List<node>& newSubset, List<node>& newComplement, const SubsetEnumerator<node>& subset, node v) const {
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
	 * Note that it is not guaranteed that any resulting collection of edges represents a tree.
	 */
	void computeSplit(NodeArray<DWMSplit>& split, node v, SubsetEnumerator<node>& subset) const {
		OGDF_ASSERT(v->graphOf() == &m_G);
		OGDF_ASSERT(split[v].subgraph1 == nullptr);
		OGDF_ASSERT(split[v].subgraph2 == nullptr);

		DWMSplit& best = split[v];
		for (subset.begin(1, subset.numberOfMembersAndNonmembers() / 2); subset.valid(); subset.next()) {
			List<node> newSubset, newComplement;
			makeKey(newSubset, newComplement, subset, v);

			if (safeIfSumSmaller(costOf(newSubset), costOf(newComplement), best.cost)) {
				best.set(dataOf(newSubset), dataOf(newComplement));
			}
		}
	}

	//! Computes all partial solutions for given #m_terminalSubset
	template<typename CONTAINER>
	void computePartialSolutions(const CONTAINER& targets) {
		OGDF_ASSERT(m_terminalSubset.size() >= 2);

		List<node> terminals;
		m_terminalSubset.list(terminals);
		SubsetEnumerator<node> subset(terminals); // done here because of linear running time
		NodeArray<DWMSplit> split(m_G);

		// update auxiliary graph
		updateAuxGraph(split, subset, costOf(terminals));

		// compute shortest-paths tree on graph
		NodeArray<T> distance;
		NodeArray<edge> pred;
		MinSteinerTreeModule<T>::singleSourceShortestPaths(m_auxG.graph(), m_auxG.source(), m_auxG.terminalArray(), distance, pred);

		// insert best subtrees
		insertBestSubtrees(targets, split, pred, distance, terminals);
	}

	//! Initializes the hash array with all node-terminal-pairs
	void initializeMap() {
		for (node t : m_terminals) {
			NodeArray<T> distance;
			NodeArray<edge> pred;
			MinSteinerTreeModule<T>::singleSourceShortestPaths(m_G, t, m_isTerminal, distance, pred);

			for (node v : m_G.nodes) {
				// make key
				List<node> key;
				key.pushBack(t);
				if (v->index() < t->index()) {
					key.pushFront(v);
				} else {
					key.pushBack(v);
				}

				// insert if not already defined
				if (!m_map.member(key)) {
					T dist = distance[v];
					ArrayBuffer<edge> edges;
					for (node curr = v; pred[curr] != nullptr; curr = pred[curr]->opposite(curr)) {
						edges.push(pred[curr]);
					}
					m_map.fastInsert(key, DWMData(dist, edges));
				}
			}
		}
	}

	//! Updates the auxiliary graph
	void updateAuxGraph(NodeArray<DWMSplit>& split, SubsetEnumerator<node>& subset, T oldCost) {
		for (adjEntry adj : m_auxG.source()->adjEntries) {
			node w = m_auxG.original(adj->twinNode());
			T cost = oldCost;
			if (!m_terminalSubset.hasMember(w)) {
				computeSplit(split, w, subset);
				cost = split[w].cost;
			}
			m_auxG.setWeight(adj->theEdge(), cost);
		}
	}

	//! Adds the shortest path from the source to \p curr into \p result
	//! @returns the edge of that path that is incident to the source
	edge addNewPath(DWMData& result, node curr, const NodeArray<edge>& pred) const {
		edge e = nullptr;
		while (curr != m_auxG.source()) {
			e = pred[curr];
			OGDF_ASSERT(e != nullptr);
			node prev = e->opposite(curr);
			OGDF_ASSERT(prev != curr);
			if (prev != m_auxG.source()) {
				edge eO = m_auxG.original(e);
				OGDF_ASSERT(eO != nullptr);
				result.add(eO, m_auxG.weight(e));
			}
			curr = prev;
		}
		OGDF_ASSERT(e != nullptr);
		OGDF_ASSERT(e->source() == curr);
		OGDF_ASSERT(curr == m_auxG.source());

		return e;
	}

	//! Inserts the invalid best subtree (based on the SSSP computation) into the hash map
	void insertInvalidBestSubtree(node v, const NodeArray<T>& distance, const List<node>& newSubset) {
		OGDF_ASSERT(v->graphOf() == &m_auxG.graph());
		OGDF_ASSERT(v != m_auxG.source());
		DWMData best(distance[v]);
		best.invalidate();
		m_map.fastInsert(newSubset, best);
	}

	//! Inserts the valid best subtree (based on the SSSP computation) into the hash map
	void insertValidBestSubtree(node v, const NodeArray<DWMSplit>& split, const NodeArray<edge>& pred, const List<node>& newSubset, const List<node>& terminals) {
		OGDF_ASSERT(v->graphOf() == &m_auxG.graph());
		OGDF_ASSERT(v != m_auxG.source());
		DWMData best(0);

		edge e = addNewPath(best, v, pred);
		// add best solution so far
		node tO = m_auxG.original(e->target());
		if (m_terminalSubset.hasMember(tO)) {
			best.add(dataOf(terminals));
		} else {
			best.add(split[tO].subgraph1);
			best.add(split[tO].subgraph2);
		}

		m_map.fastInsert(newSubset, best);
	}

	//! Inserts the best subtrees into the hash map
	template<typename CONTAINER>
	void insertBestSubtrees(const CONTAINER& targets, const NodeArray<DWMSplit>& split, const NodeArray<edge>& pred, const NodeArray<T>& distance, const List<node>& terminals) {
		for (node v : targets) {
			if (!m_terminalSubset.hasMember(v)) {
				List<node> newSubset;
				makeKey(newSubset, v);

				if (!m_map.member(newSubset)) { // not already defined
					node vCopy = m_auxG.copy(v);
					if (pred[m_auxG.copy(v)] == nullptr) { // path over terminal
						insertInvalidBestSubtree(vCopy, distance, newSubset);
					} else {
						insertValidBestSubtree(vCopy, split, pred, newSubset, terminals);
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
			for (edge e : data.edges) {
				node uO = e->source();
				node vO = e->target();
				OGDF_ASSERT(uO != nullptr);
				OGDF_ASSERT(vO != nullptr);
				node uC = tree.copy(uO);
				node vC = tree.copy(vO);
				if (uC == nullptr) {
					uC = tree.newNode(uO);
				}
				if (vC == nullptr) {
					vC = tree.newNode(vO);
				}
				T dist = m_G.weight(e);
				tree.newEdge(e, dist);
				cost += dist;
			}

			// recurse
			for (const DWMData* subgraph : data.subgraphs) {
				cost += getSteinerTreeFor(*subgraph, tree);
			}

			OGDF_ASSERT(isAcyclicUndirected(tree));
		}

		return cost;
	}

public:
	/**
	 * The constructor
	 * \pre The list of terminals has to be sorted by index (use MinSteinerTreeModule<T>::sortTerminals)
	 */
	FullComponentGeneratorDreyfusWagnerWithoutMatrix(const EdgeWeightedGraph<T>& G, const List<node>& terminals, const NodeArray<bool>& isTerminal)
	  : m_G(G),
	    m_terminals(terminals),
	    m_isTerminal(isTerminal),
	    m_auxG(m_G, m_terminals),
	    m_terminalSubset(m_terminals),
	    m_map(1 << 22) { // we initially allocate 4MB*sizeof(DWMData) for hashing
	}

	void call(int restricted) {
		OGDF_ASSERT(restricted >= 2);
		Math::updateMin(restricted, m_terminals.size());
		initializeMap();
		for (m_terminalSubset.begin(2, restricted - 2); m_terminalSubset.valid(); m_terminalSubset.next()) {
			computePartialSolutions(m_G.nodes);
		}
		// save time by only adding terminals instead of all nodes
		for (m_terminalSubset.begin(restricted - 1); m_terminalSubset.valid(); m_terminalSubset.next()) {
			computePartialSolutions(m_terminals);
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

	//! Checks if a given \p tree is a valid full component
	bool isValidComponent(const EdgeWeightedGraphCopy<T>& tree) const {
		if (tree.empty()) {
			return false;
		}
		for (node v : tree.nodes) {
			OGDF_ASSERT(v->degree() > 1 || m_isTerminal[tree.original(v)]);
			if (m_isTerminal[tree.original(v)] // is a terminal
			 && v->degree() > 1) { // but not a leaf
				return false;
			}
		}
		return true;
	}
};

template<typename T>
class FullComponentGeneratorDreyfusWagnerWithoutMatrix<T>::SortedNodeListHashFunc {
	static const unsigned int c_prime = 0x7fffffff; // mersenne prime 2**31 - 1
	// would be nicer: 0x1fffffffffffffff; // mersenne prime 2**61 - 1
	const int m_random;

public:
	//! Initializes the random number
	SortedNodeListHashFunc() : m_random(randomNumber(2, c_prime - 1)) {}

	//! The actual hash function
	unsigned int hash(const List<node>& key) const {
		unsigned int hash = 0;
		for (node v : key) {
			hash = (hash * m_random + v->index()) % c_prime;
		}
		return hash;
	}
};

}
}
