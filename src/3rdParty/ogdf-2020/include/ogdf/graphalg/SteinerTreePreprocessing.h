/*! \file
 * \brief Definition of the ogdf::SteinerTreePreprocessing class template.
 *
 * \author Mihai Popa
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

#include <forward_list>
#include <memory>
#include <set>
#include <unordered_map>

#include <ogdf/basic/BoundedQueue.h>
#include <ogdf/basic/SubsetEnumerator.h>
#include <ogdf/graphalg/MinSteinerTreeMehlhorn.h>
#include <ogdf/graphalg/MinSteinerTreeTakahashi.h>
#include <ogdf/graphalg/SteinerTreeLowerBoundDualAscent.h>
#include <ogdf/graphalg/steiner_tree/HeavyPathDecomposition.h>

namespace ogdf {

// Helpers:
namespace steiner_tree {
/** A class used by the unordered_maps inside the reductions.
 *  The operator() is defined as a hashing function for NodePair.
 *  The pair is unordered: (u, v) equals (v, u).
 */
class UnorderedNodePairHasher {
public:
	int operator() (NodePair const &v) const
	{
		return static_cast<int>((static_cast<long long>(
		 min(v.source->index(), v.target->index())+11) * (max(v.source->index(), v.target->index())+73)) % 700001);
	}
};

/** A class used by the unordered_maps inside the reductions.
 *  The operator() is defined as an equality function for NodePair.
 *  The pair is unordered: (u, v) equals (v, u).
 */
class UnorderedNodePairEquality {
public:
	bool operator() (NodePair const &pair1, NodePair const &pair2) const
	{
		return (pair1.source == pair2.source && pair1.target == pair2.target)
		    || (pair1.source == pair2.target && pair1.target == pair2.source);
	}
};

}

/** \brief This class implements preprocessing strategies for the Steiner tree problem.
 *
 * It contains a subset of strategies from [DV89] and [PV01].
 *
 * References:
 *  - [DV89] C. W. Duin, A. Volgenant: Reduction tests for the Steiner problem in graphs, Networks 19(5), pp. 549-567, 1989
 *  - [PV01] T. Polzin, S. V. Daneshmand: Improved algorithms for the Steiner problem in networks,
 *    Discrete Applied Mathematics 112, pp. 263-300, 2001
 *
 * @ingroup ga-steiner
 */
template<typename T>
class SteinerTreePreprocessing {
protected:
	const EdgeWeightedGraph<T> &m_origGraph; //!< Const reference to the original graph
	const List<node> &m_origTerminals; //!< Const reference to the original list of terminals
	const NodeArray<bool> &m_origIsTerminal; //!< Const reference to the original isTerminal
	const EpsilonTest m_eps;

	EdgeWeightedGraph<T> m_copyGraph; //!< Copy of the original graph; this copy will actually be reduced
	List<node> m_copyTerminals; //!< The reduced form of terminals
	NodeArray<bool> m_copyIsTerminal; //!< The reduced form of isTerminal

	T m_costAlreadyInserted; //!< The cost of the already inserted in solution edges

	NodeArray<int> m_nodeSonsListIndex; /*!< It contains for each node
	  * an index i. If i is non-negative, m_sonsList[i] is a list containing
	  * the indices of the node's sons. A son of the current node is a node
	  * or edge that must appear in the solution if the current node appears.
	  * If i is negative, it means that the current node is original (i.e., was
	  * not created by the applied reductions). The corresponding node is
	  * the (-i)-th node of m_origGraph */
	EdgeArray<int> m_edgeSonsListIndex; /*!< See m_nodeSonsListIndex but for edges. */
	std::vector<std::vector<int>> m_sonsList; //!< List with lists (corresponding to nodes and containing the indexes of their sons)

	std::unique_ptr<MinSteinerTreeModule<T>> m_costUpperBoundAlgorithm; //!< Algorithm used for computing the upper bound for the cost of a minimum Steiner tree.

public:
	/*!
	 * @param wg The initial graph that will be reduced.
	 * @param terminals The list of terminals of the initial graph.
	 * @param isTerminal True if a node is terminal in the initial graph, false otherwise.
	 */
	SteinerTreePreprocessing(const EdgeWeightedGraph<T> &wg, const List<node> &terminals, const NodeArray<bool> &isTerminal);

	/**
	 * \brief A shortcut to get the solution of a reduced instance.
	 * Note that you have to apply reductions first, e.g., reduceFast().
	 * @param mst A MinSteinerTreeModule<T> instance that is used to solve the problem
	 * @param finalSteinerTree A pointer to the final Steiner tree (has to be freed)
	 * @return The weight of the final Steiner tree
	 */
	T solve(MinSteinerTreeModule<T> &mst, EdgeWeightedGraphCopy<T> *&finalSteinerTree)
	{
		finalSteinerTree = new EdgeWeightedGraphCopy<T>;
		// reductions generate new nodes and edges, which may inflate the internal structure
		if (m_copyGraph.maxNodeIndex() <= m_copyGraph.numberOfNodes() + 5
		 && m_copyGraph.maxEdgeIndex() <= m_copyGraph.numberOfEdges() + 10) { // within inflate tolerance
			EdgeWeightedGraphCopy<T> *reducedSolution = nullptr;
			T cost = mst.call(m_copyGraph, m_copyTerminals, m_copyIsTerminal, reducedSolution);
			cost += costEdgesAlreadyInserted();
			computeOriginalSolution(*reducedSolution, *finalSteinerTree);
			delete reducedSolution;
			return cost;
		}

		// make a compact copy
		EdgeWeightedGraph<T> ccGraph;
		NodeArray<node> nCopy(m_copyGraph, nullptr);
		EdgeArray<edge> eCopy(m_copyGraph, nullptr);
		List<node> ccTerminals;
		NodeArray<bool> ccIsTerminal(ccGraph, false);
		for (node v : m_copyGraph.nodes) {
			nCopy[v] = ccGraph.newNode();
		}
		for (edge e : m_copyGraph.edges) {
			eCopy[e] = ccGraph.newEdge(nCopy[e->source()], nCopy[e->target()], m_copyGraph.weight(e));
		}
		for (node t : m_copyTerminals) {
			const node tC = nCopy[t];
			ccTerminals.pushBack(tC);
			ccIsTerminal[tC] = true;
		}

		// solve compact copy
		EdgeWeightedGraphCopy<T> *ccSolution = nullptr;
		T cost = mst.call(ccGraph, ccTerminals, ccIsTerminal, ccSolution);
		cost += costEdgesAlreadyInserted();

		// get reduced and original solution from compact copy solution
		EdgeWeightedGraphCopy<T> reducedSolution;
		reducedSolution.createEmpty(m_copyGraph);
		for (node v : m_copyGraph.nodes) {
			if (ccSolution->copy(nCopy[v]) != nullptr) { // is in solution
				reducedSolution.newNode(v);
			}
		}
		for (edge e : m_copyGraph.edges) {
			if (ccSolution->copy(eCopy[e]) != nullptr) { // is in solution
				reducedSolution.newEdge(e, m_copyGraph.weight(e));
			}
		}
		delete ccSolution;

		computeOriginalSolution(reducedSolution, *finalSteinerTree);
		return cost;
	}

	/**
	 * @name Methods on Steiner tree instances
	 * These methods reference to the reduced (preprocessed) instance.
	 */
	//@{
	//! Returns the reduced form of the graph.
	inline const EdgeWeightedGraph<T>& getReducedGraph() const {	return m_copyGraph; }

	//! Returns the list of the terminals corresponding to the reduced graph.
	inline const List<node>& getReducedTerminals() const {	return m_copyTerminals; }

	//! Shuffles the list of reduced terminals. This can have an effect on some tests.
	inline void shuffleReducedTerminals() { m_copyTerminals.permute(); }

	//! Returns the NodeArray<bool> isTerminal corresponding to the reduced graph.
	inline const NodeArray<bool>& getReducedIsTerminal() const { return m_copyIsTerminal; }
	//@}

	/**
	 * @name Methods for Steiner tree solutions
	 * These methods allow retrieval of solution information about the original instance using a solution of a preprocessed instance.
	 */
	//@{
	//! Returns the cost of the edges already inserted in solution during the reductions.
	inline T costEdgesAlreadyInserted() const { return m_costAlreadyInserted; }

	/*!
	 * \brief Computes the solution for the original graph, given a solution on the reduction.
	 * @param reducedGraphSolution The already computed solution on the reduced graph.
	 * @param correspondingOriginalSolution The solution on the original graph, corresponding to the reducedGraphSolution.
	 */
	void computeOriginalSolution(const EdgeWeightedGraphCopy<T> &reducedGraphSolution, EdgeWeightedGraphCopy<T> &correspondingOriginalSolution);
	//@}

	/**
	 * @name Combined reduction sets
	 * Each method applies several reductions iteratively.
	 */
	//@{

	//! \brief Apply trivial (hence amazingly fast) reductions iteratively, that is degree2Test(), makeSimple(), deleteLeaves().
	bool reduceTrivial()
	{
		return repeat([this]() {
			bool changed = false;
			changed |= degree2Test();
			changed |= makeSimple();
			changed |= deleteLeaves();
			return changed;
		});
	}

	//! \brief Apply fast reductions iteratively (includes trivial reductions).
	bool reduceFast() {
		int k = 5; // for PTmTest
		// comment wrt. Apple Clang 10: making k const, results in compiler error while binding it in lambda
		// comment wrt. MSVC 19: not binding the const k would result in compiler error

		bool changed = deleteComponentsWithoutTerminals();
		bool triviallyChanged = false;
		changed |= repeat([this, &triviallyChanged, k]() {
			bool innerChanged = false;
			triviallyChanged = reduceTrivial();
			// graph guaranteed to be simple and connected

			if (nearestVertexTest()) {
				makeSimple();
				innerChanged = true;
			}

			// precond: connected
			if (terminalDistanceTest()) {
				// can occur: disconnected
				deleteComponentsWithoutTerminals();
				innerChanged = true;
			}

			// precond: simple, connected
			innerChanged |= NTDkTest(10, k);
			// can occur: parallel edges

			// precond: connected
			innerChanged |= shortLinksTest();
			// can occur: parallel edges, self-loops

			// precond: connected
			innerChanged |= lowerBoundBasedTest();

			// is not thaaat good but helps a little:
			innerChanged |= PTmTest(k);
			// can occur: parallel edges

			return innerChanged;
		});
		return changed | triviallyChanged;
	}

	//! Apply fast reductions and the dual-ascent-based tests iteratively.
	bool reduceFastAndDualAscent() {
		return repeat([this] {
			bool changed = reduceFast();
			changed |= dualAscentBasedTest();
			return changed;
		});
	}

	//@}

	/**
	 * @name Single reductions
	 * Each method applies a single reduction to the Steiner tree instance.
	 */
	//@{

	/*!
	 * \brief Deletes the leaves of the graph.
	 * The path to the terminal is included into the solution if the leaf is a terminal.
	 * - Time: O(n+m)
	 * - Memory: O(n)
	 * @return True iff the graph is changed
	 */
	bool deleteLeaves();

	/*!
	 * \brief Deletes parallel edges keeping only the minimum cost one, and deletes self-loops.
	 * - Time: O(n+m)
	 * - Memory: O(m)
	 * @return True iff the graph is changed
	 */
	bool makeSimple();

	/*!
	 * \brief Deletes connected components with no terminals.
	 * - Time: O(n+m)
	 * - Memory: O(n)
	 * @return True iff the graph is changed
	 */
	bool deleteComponentsWithoutTerminals();

	/*!
	 * \brief Performs a least cost test [DV89] computing the whole shortest path matrix.
	 * - Time: O(n^3)
	 * - Memory: O(n^2)
	 * @return True iff the graph is changed
	 */
	bool leastCostTest();

	/*!
	 * \brief deletes degree-2 nodes and replaces them with one edge with the adjacent edges' sum
	 * - Time: O(n)
	 * - Memory: O(1)
	 * @return True iff the graph is changed
	 */
	bool degree2Test();

	/*!
	 * \brief "Paths with many terminals" test, efficient on paths with many terminals. Heuristic approach, as suggested in [PV01].
	 * - Time: O(m + n log n)
	 * - Memory: O(n)
	 * @param k Consider the k nearest terminals to all non-terminals in the heuristic approach
	 * @return True iff the graph is changed
	 */
	bool PTmTest(const int k = 3);

	/*!
	 * \brief Simple terminal distance test [PV01].
	 * - Time: O(m + n log n)
	 * - Memory: O(n)
	 * @return True iff the graph is changed
	 *
	 * \pre Graph must be connected (use deleteComponentsWithoutTerminals())
	 */
	bool terminalDistanceTest();

	/*!
	 * \brief Long-Edges test from [DV89]
	 * @return True iff the graph is changed
	 *
	 * \pre Graph must be connected (use deleteComponentsWithoutTerminals())
	 */
	bool longEdgesTest();

	/*!
	 * \brief Non-terminals of degree k test [DV89, PV01]
	 * - Time: O(m + n log n)
	 * - Memory: O(n)
	 * @param maxTestedDegree The reduction test is applied to all nodes with degree >= 3 and <= this value
	 * @param k The parameter k for the internal PTmTest, applied to avoid adding useless edges
	 * @return True iff the graph is changed
	 *
	 * \pre Graph must be simple (use makeSimple()) and connected (use deleteComponentsWithoutTerminals())
	 */
	bool NTDkTest(const int maxTestedDegree = 5, const int k = 3);

	/*!
	 * \brief Nearest vertex test using Voronoi regions [DV89, PV01].
	 * - Time: O(n log n)
	 * - Memory: O(n)
	 * @return True iff the graph is changed
	 *
	 * \pre Graph must be loop-free (use makeSimple()) and connected (use deleteComponentsWithoutTerminals())
	 */
	bool nearestVertexTest();

	/*!
	 * \brief Short links test using Voronoi regions [PV01].
	 * - Time: O(n log n)
	 * - Memory: O(n)
	 * @return True iff the graph is changed
	 *
	 * \pre Graph must be connected (use deleteComponentsWithoutTerminals())
	 */
	bool shortLinksTest();

	/*!
	 * Computes for each non-terminal a lower bound of the cost of the minimum Steiner tree containing it.
	 * Deletes the nodes and edges whose corresponding lower bounds exceed the \p upperBound.
	 * See [PV01, Observations 3.5, 3.6, and 3.8].
	 * - Time: O(m log n)
	 * - Memory: O(n)
	 * @return True iff the graph is changed
	 *
	 * \pre Graph must be connected (use deleteComponentsWithoutTerminals())
	 */
	bool lowerBoundBasedTest(T upperBound);

	/*!
	 * Like #lowerBoundBasedTest(T upperBound) but the
	 * upper bound is computed by #computeMinSteinerTreeUpperBound.
	 */
	inline bool lowerBoundBasedTest() {
		return lowerBoundBasedTest(computeMinSteinerTreeUpperBound());
	}

	/**
	 * Like #lowerBoundBasedTest(T upperBound)
	 * but uses ogdf::SteinerTreeLowerBoundDualAscent to compute lower bounds.
	 *    See [PV01, Section 3.2.2 on DA].
	 * - Time: O(m min(m, r*n) + n log n)
	 * @return True iff the graph is changed
	 *
	 * \pre Graph must be connected (use deleteComponentsWithoutTerminals())
	 */
	bool dualAscentBasedTest(int repetitions, T upperBound);

	/*!
	 * Like #dualAscentBasedTest(int repetitions, T upperBound) but the
	 * upper bound is computed by #computeMinSteinerTreeUpperBound.
	 */
	inline bool dualAscentBasedTest(int repetitions = 1) {
		return dualAscentBasedTest(repetitions, computeMinSteinerTreeUpperBound());
	}

	/*!
	 * \brief Performs a reachability test [DV89].
	 * - Time: O(n * m * log n)
	 * - Memory: O(n)
	 * @param maxDegreeTest Ignores nodes with degree larger than this. Ignores no node if non-positive (default).
	 * @param k The parameter k for the internal PTmTest, applied to avoid adding useless edges
	 * @return True iff the graph is changed
	 *
	 * \pre Graph must be simple (use makeSimple()) and connected (use deleteComponentsWithoutTerminals())
	 */
	bool reachabilityTest(int maxDegreeTest = 0, const int k = 3);

	/*!
	 * \brief Performs a cut reachability test [DV89].
	 * - Time: O(n * m * log n)
	 * - Memory: O(n)
	 * @return True iff the graph is changed
	 *
	 * \pre Graph must not contain non-terminal leaves (use deleteLeaves()) and be connected (use deleteComponentsWithoutTerminals()).
	 */
	bool cutReachabilityTest();

	//@}

	/**
	 * @name Miscellaneous methods
	 * These methods allow retrieval of solution information about the original instance using a solution of a preprocessed instance.
	 */
	//@{
	//! Set the module option for the algorithm used for computing the MinSteinerTree cost upper bound.
	inline void setCostUpperBoundAlgorithm(MinSteinerTreeModule<T> *pMinSteinerTreeModule) {
		m_costUpperBoundAlgorithm.reset(pMinSteinerTreeModule);
	}

	//@}

	//! Auxiliary function: Repeats a function until it returns false (used for iteratively applying reductions)
	static bool repeat(std::function<bool()> f) {
		bool changed = false;
		while (f()) {
			changed = true;
		}
		return changed;
	}

protected:
	/*!
	 * \brief Update internal data structures to let a (new) node or edge represent replaced nodes and/or edges.
	 * @param x The node or edge that represents the replaced entities. Note that its existing information will be overwritten.
	 * @param replacedNodes The deleted nodes that have to appear in solution iff x appears.
	 * @param replacedEdges The deleted edges that have to appear in solution iff x appears.
	 * @param deleteReplacedElements True iff the replaced nodes and edges should be deleted.
	 * @param whatSonsListIndex An internal data structure (depending on whether you wish to use nodes or edges)
	 */
	template<typename TWhat, typename TWhatArray>
	void addNew(TWhat x, const std::vector<node> &replacedNodes, const std::vector<edge> &replacedEdges, bool deleteReplacedElements,
	            TWhatArray &whatSonsListIndex);

	//! \brief Calls addNew() for a node.
	inline void addNewNode(node v, const std::vector<node> &replacedNodes, const std::vector<edge> &replacedEdges, bool deleteReplacedElements)
	{
		addNew(v, replacedNodes, replacedEdges, deleteReplacedElements, m_nodeSonsListIndex);
	}

	//! \brief The function is called after a new edge is added to the copy graph during the reductions.
	inline void addNewEdge(edge e, const std::vector<node> &replacedNodes, const std::vector<edge> &replacedEdges, bool deleteReplacedElements)
	{
		addNew(e, replacedNodes, replacedEdges, deleteReplacedElements, m_edgeSonsListIndex);
	}

	/*!
	 * \brief The function adds a set of edges in the solution.
	 * It assumes that after the insertion of one edge the next ones can still be added.
	 * @param edgesToBeAddedInSolution The list containing the edges that are going to be added in solution.
	 * @return True iff the graph is changed
	 */
	bool addEdgesToSolution(const List<edge> &edgesToBeAddedInSolution);

	//! Used by reductions to recompute the m_copyTerminals list, according to m_copyIsTerminal; useful when "online" updates to m_copyTerminals are inefficient.
	void recomputeTerminalsList();

	//! Computes the shortest path matrix corresponding to the m_copyGraph.
	void computeShortestPathMatrix(NodeArray<NodeArray<T>> &shortestPath) const;

	//! Applies the Floyd-Warshall algorithm on the m_copyGraph. The shortestPath matrix has to be already initialized.
	void floydWarshall(NodeArray<NodeArray<T>> &shortestPath) const;

	inline T computeMinSteinerTreeUpperBound(EdgeWeightedGraphCopy<T> *&finalSteinerTree) const
	{
		finalSteinerTree = nullptr;
		return m_costUpperBoundAlgorithm->call(m_copyGraph, m_copyTerminals, m_copyIsTerminal, finalSteinerTree);
	}

	// for convenience:
	inline T computeMinSteinerTreeUpperBound() const
	{
		EdgeWeightedGraphCopy<T> *finalSteinerTree;
		T treeCostUpperBound = computeMinSteinerTreeUpperBound(finalSteinerTree);
		delete finalSteinerTree;
		return treeCostUpperBound;
	}

	//! Helper method for computeOriginalSolution.
	void addToSolution(const int listIndex, Array<bool, int> &isInSolution) const;

	//! Deletes the nodes whose lowerBoundWithNode exceeds upperBound.
	bool deleteNodesAboveUpperBound(const NodeArray<T> &lowerBoundWithNode, const T upperBound);
	//! Deletes the edges whose lowerBoundWithEdge exceeds upperBound.
	bool deleteEdgesAboveUpperBound(const EdgeArray<T> &lowerBoundWithEdge, const T upperBound);

	//! Deletes a node that is known to have degree 2 in at least one minimum Steiner tree.
	void deleteSteinerDegreeTwoNode(node v,
			const EdgeWeightedGraphCopy<T>& tprime,
			const steiner_tree::HeavyPathDecomposition<T>& tprimeHPD,
			const NodeArray<List<std::pair<node,T>>>& closestTerminals);

	/**
	 * \brief Heuristic approach to computing the closest non-terminals for one node, such that there is no terminal on the path
	 * from it to a "close non-terminal" and a maximum constant number of expandedEdges are expanded during the computation
	 * @param source The source node.
	 * @param reachedNodes A list where the reached nodes are saved.
	 * @param distance A NodeArray where the minimum found distance is saved, infinity where no path is found.
	 * @param maxDistance Constant such that: any distance > maxDistance is not of interest.
	 * @param expandedEdges The maximum number of edges expanded during the computation.
	 */
	void findClosestNonTerminals(node source, List<node> &reachedNodes, NodeArray<T> &distance, T maxDistance, int expandedEdges) const;

	/**
	 * \brief Heuristic computation [PV01] of the bottleneck Steiner distance between two nodes in a graph.
	 * - Time: O(log n)
	 */
	T computeBottleneckDistance(node x, node y,
			const EdgeWeightedGraphCopy<T>& tprime,
			const steiner_tree::HeavyPathDecomposition<T>& tprimeHPD,
			const NodeArray<List<std::pair<node,T>>>& closestTerminals) const;

	/**
	 * \brief Computes for every non-terminal the closest k terminals such that there is no other terminal on the path
	 * @param k The maximum number of close terminals to be computed for every non-terminal node.
	 * @param closestTerminals A reference to a NodeArray of lists where the solution will be saved.
	 */
	void computeClosestKTerminals(const int k, NodeArray<List<std::pair<node,T>>> &closestTerminals) const;

	//! Compute radius of terminals
	void computeRadiusOfTerminals(NodeArray<T> &terminalRadius) const;

	//! Compute the sum of all radii except the two largest
	T computeRadiusSum() const;

	//! Compute first and second best terminals according to function \p dist
	template<typename LAMBDA>
	void computeOptimalTerminals(node v, LAMBDA dist, node &optimalTerminal1, node &optimalTerminal2, NodeArray<T> &distance) const;

	//! Mark successors of \p currentNode in its shortest-path tree in \p voronoiRegions
	void markSuccessors(node currentNode, const Voronoi<T> &voronoiRegions, NodeArray<bool> &isSuccessorOfMinCostEdge) const;

	//! Finds the \p first and \p second smallest edges incident to \p v
	void findTwoMinimumCostEdges(node v, edge& first, edge& second) const;
};

template<typename T>
SteinerTreePreprocessing<T>::SteinerTreePreprocessing(const EdgeWeightedGraph<T> &wg, const List<node> &terminals, const NodeArray<bool> &isTerminal)
  : m_origGraph(wg)
  , m_origTerminals(terminals)
  , m_origIsTerminal(isTerminal)
  , m_eps(1e-6)
{
	OGDF_ASSERT(!m_origGraph.empty());

	// make the initial graph copy
	m_copyGraph.clear();
	NodeArray<node> nCopy(m_origGraph);
	EdgeArray<edge> eCopy(m_origGraph);

	for (node v : m_origGraph.nodes) {
		nCopy[v] = m_copyGraph.newNode();
	}
	for (edge e : m_origGraph.edges) {
		eCopy[e] = m_copyGraph.newEdge(nCopy[e->source()], nCopy[e->target()], m_origGraph.weight(e));
	}

	// create the terminals and isTerminal arrays for the copyGraph
	m_copyTerminals.clear();
	m_copyIsTerminal.init(m_copyGraph, false);
	for (node v : m_origGraph.nodes) {
		if (m_origIsTerminal(v)) {
			const node vC = nCopy[v];
			m_copyTerminals.pushBack(vC);
			m_copyIsTerminal[vC] = true;
		}
	}
	m_costAlreadyInserted = 0;

	// map every node and edge to a negative number for m_(node/edge)_m_sonsListIndex
	m_nodeSonsListIndex.init(m_copyGraph);
	m_edgeSonsListIndex.init(m_copyGraph);
	int nextIndex = 1;
	for (node v : m_origGraph.nodes) {
		m_nodeSonsListIndex[nCopy[v]] = -(nextIndex++);
	}
	for (edge e : m_origGraph.edges) {
		m_edgeSonsListIndex[eCopy[e]] = -(nextIndex++);
	}

	// set the default approximation algorithm for computing the upper bound cost of the solution
	m_costUpperBoundAlgorithm.reset(new MinSteinerTreeTakahashi<T>);
}

template<typename T>
template<typename TWhat, typename TWhatArray>
void SteinerTreePreprocessing<T>::addNew(TWhat x,
                                         const std::vector<node> &replacedNodes,
                                         const std::vector<edge> &replacedEdges,
                                         bool deleteReplacedElements,
                                         TWhatArray &whatSonsListIndex)
{
	std::vector<int> sonsIndexList;
	for (node replacedNode : replacedNodes) {
		sonsIndexList.push_back(m_nodeSonsListIndex[replacedNode]);
	}
	for (edge replacedEdge : replacedEdges) {
		sonsIndexList.push_back(m_edgeSonsListIndex[replacedEdge]);
	}

	m_sonsList.emplace_back(sonsIndexList);
	whatSonsListIndex[x] = (int)m_sonsList.size() - 1;

	if (deleteReplacedElements) {
		for (edge e : replacedEdges) {
			m_copyGraph.delEdge(e);
		}
		for (node v :replacedNodes) {
			m_copyGraph.delNode(v);
		}
	}
}

template<typename T>
void SteinerTreePreprocessing<T>::addToSolution(const int listIndex, Array<bool, int>& isInSolution) const
{
	if (listIndex < 0) { // is starting node or edge
		isInSolution[listIndex] = true;
		return;
	}
	// is further added node or edge
	for (int son : m_sonsList[listIndex]) {
		addToSolution(son, isInSolution);
	}
}

template<typename T>
void SteinerTreePreprocessing<T>::computeOriginalSolution(const EdgeWeightedGraphCopy<T> &reducedGraphSolution, EdgeWeightedGraphCopy<T> &correspondingOriginalSolution)
{
	correspondingOriginalSolution.createEmpty(m_origGraph); // note that it is not cleared!

	Array<bool, int> isInSolution(-(m_origGraph.numberOfNodes() + m_origGraph.numberOfEdges()), -1, false);

	// make the indices of original nodes/edge true in isInSolution
	for (node v : reducedGraphSolution.nodes) {
		addToSolution(m_nodeSonsListIndex[reducedGraphSolution.original(v)], isInSolution);
	}
	for (edge e : reducedGraphSolution.edges) {
		addToSolution(m_edgeSonsListIndex[reducedGraphSolution.original(e)], isInSolution);
	}

	// insert nodes and edges
	int nextIndex = 1;
	for (node v : m_origGraph.nodes) {
		if (isInSolution[-(nextIndex++)]) {
			correspondingOriginalSolution.newNode(v);
		}
	}
	for (edge e : m_origGraph.edges) {
		if (isInSolution[-(nextIndex++)]) {
			correspondingOriginalSolution.newEdge(e, m_origGraph.weight(e));
		}
	}
}

template<typename T>
void SteinerTreePreprocessing<T>::deleteSteinerDegreeTwoNode(node v,
                                                             const EdgeWeightedGraphCopy<T> &tprime,
                                                             const steiner_tree::HeavyPathDecomposition<T> &tprimeHPD,
                                                             const NodeArray<List<std::pair<node,T>>> &closestTerminals)
{
	struct NewEdgeData {
		edge e1; // new edge replaces this...
		edge e2; // ...and this
		edge already; // but is it already there?
		T weight;
	};
	std::forward_list<NewEdgeData> newEdges;
	for (adjEntry adj1 : v->adjEntries) {
		const edge e1 = adj1->theEdge();
		const node adjacentNode1 = adj1->twinNode();

		for (adjEntry adj2 = adj1->succ(); adj2 ; adj2 = adj2->succ()) {
			const edge e2 = adj2->theEdge();
			const node adjacentNode2 = adj2->twinNode();

			T edgeWeight = m_copyGraph.weight(e1) + m_copyGraph.weight(e2);

			const edge f = m_copyGraph.searchEdge(adjacentNode1, adjacentNode2);
			if (f && m_copyGraph.weight(f) <= edgeWeight) {
				continue; // check whether there is already a lower cost edge connecting the two adjacent nodes
			}

			T bottleneckDistance = computeBottleneckDistance(adjacentNode1, adjacentNode2, tprime, tprimeHPD, closestTerminals);
			if (m_eps.greater(edgeWeight, bottleneckDistance)) {
				continue; // the PTm test
			}

			newEdges.emplace_front(NewEdgeData{e1, e2, f, edgeWeight});
		}
	}

	for (const auto &newEdge : newEdges) {
		// delete the old edge (do not generate parallel edges)
		edge newEdgeInGraph;
		if (newEdge.already) {
			OGDF_ASSERT(m_copyGraph.weight(newEdge.already) > newEdge.weight);
			m_copyGraph.setWeight(newEdge.already, newEdge.weight);
			newEdgeInGraph = newEdge.already;
		} else {
			newEdgeInGraph = m_copyGraph.newEdge(newEdge.e1->opposite(v), newEdge.e2->opposite(v), newEdge.weight);
		}
		addNewEdge(newEdgeInGraph, {v}, {newEdge.e1, newEdge.e2}, false);
	}

	m_copyGraph.delNode(v);
}

template<typename T>
void SteinerTreePreprocessing<T>::recomputeTerminalsList()
{
	m_copyTerminals.clear();
	for (node v : m_copyGraph.nodes) {
		if (m_copyIsTerminal[v]) {
			m_copyTerminals.pushBack(v);
		}
	}
}

template<typename T>
T SteinerTreePreprocessing<T>::computeBottleneckDistance(node x, node y,
                                                         const EdgeWeightedGraphCopy<T> &tprime,
                                                         const steiner_tree::HeavyPathDecomposition<T> &tprimeHPD,
                                                         const NodeArray<List<std::pair<node, T>>> &closestTerminals) const
{
	T bottleneckDistance = std::numeric_limits<T>::max();

	for (auto &xCloseTerminalDistancePair : closestTerminals[x]) {
		for (auto &yCloseTerminalDistancePair : closestTerminals[y]) {
			T possibleBottleneckDistance = xCloseTerminalDistancePair.second + yCloseTerminalDistancePair.second;

			node xTprimeCopy = tprime.copy(xCloseTerminalDistancePair.first);
			node yTprimeCopy = tprime.copy(yCloseTerminalDistancePair.first);
			possibleBottleneckDistance += tprimeHPD.getBottleneckSteinerDistance(xTprimeCopy, yTprimeCopy);

			Math::updateMin(bottleneckDistance, possibleBottleneckDistance);
		}
	}

	return bottleneckDistance;
}

template<typename T>
bool SteinerTreePreprocessing<T>::deleteLeaves()
{
	// exceptional case: only one terminal
	auto deleteAll = [this]() {
		if (m_copyGraph.numberOfNodes() > 1) {
			const node w = m_copyTerminals.front();
			// just remove all other nodes
			for (node v = m_copyGraph.firstNode(), nextV; v; v = nextV) {
				nextV = v->succ();
				if (v != w) {
					m_copyGraph.delNode(v);
				}
			}
			return true;
		}
		return false;
	};
	if (m_copyTerminals.size() == 1) {
		return deleteAll();
	}
	// general case: at least 2 terminals
	BoundedQueue<node> eraseQueue(m_copyGraph.numberOfNodes());

	for (node v : m_copyGraph.nodes) {
		if (v->degree() == 1) {
			eraseQueue.append(v);
		}
	}

	if (eraseQueue.empty()) {
		return false;
	}

	for (; !eraseQueue.empty(); eraseQueue.pop()) {
		node v = eraseQueue.top();
		if (v->degree() == 0) {
			continue;
		}
		OGDF_ASSERT(v->degree() == 1);
		const node w = v->firstAdj()->twinNode();
		if (m_copyIsTerminal[v]) { // v is a terminal
			// add edge to solution and contract v into w
			edge e = v->firstAdj()->theEdge();
			if (!m_copyIsTerminal[w]) {
				m_copyIsTerminal[w] = true;
				m_copyTerminals.pushBack(w);
			}
			m_copyTerminals.del(m_copyTerminals.search(v));
			m_costAlreadyInserted += m_copyGraph.weight(e);
			int cur = m_nodeSonsListIndex[w];
			if (cur < 0) { // w is an original (copied) node
				m_sonsList.emplace_back(std::vector<int>{cur, m_nodeSonsListIndex[v], m_edgeSonsListIndex[e]});
				m_nodeSonsListIndex[w] = (int)m_sonsList.size() - 1;
			} else { // w contains already sons
				m_sonsList[cur].push_back(m_nodeSonsListIndex[v]);
				m_sonsList[cur].push_back(m_edgeSonsListIndex[e]);
			}
		} else { // v is not a terminal
			if (w->degree() == 2) {
				eraseQueue.append(w);
			}
		}
		m_copyGraph.delNode(v);
		if (m_copyTerminals.size() == 1) {
			deleteAll();
			return true;
		}
	}
	return true;
}

template<typename T>
bool SteinerTreePreprocessing<T>::makeSimple()
{
	bool changed = false;
	NodeArray<edge> minCostEdge(m_copyGraph, nullptr);
	for (node v : m_copyGraph.nodes) {
		for (adjEntry adj = v->firstAdj(), nextAdj; adj; adj = nextAdj) {
			nextAdj = adj->succ();

			edge e = adj->theEdge();
			node adjNode = adj->twinNode();
			if (adjNode == v) { // found a self-loop
				if (nextAdj == adj->twin()) {
					nextAdj = nextAdj->succ();
				}
				m_copyGraph.delEdge(e);
				changed = true;
			} else {
				if (minCostEdge[adjNode] == nullptr
				 || m_copyGraph.weight(minCostEdge[adjNode]) > m_copyGraph.weight(e)) {
					if (minCostEdge[adjNode] != nullptr) {
						m_copyGraph.delEdge(minCostEdge[adjNode]);
						changed = true;
					}
					minCostEdge[adjNode] = e;
				} else {
					m_copyGraph.delEdge(e);
					changed = true;
				}
			}
		}

		for(adjEntry adj : v->adjEntries) {
			minCostEdge[adj->twinNode()] = nullptr;
		}
	}
	return changed;
}

template<typename T>
void SteinerTreePreprocessing<T>::floydWarshall(NodeArray<NodeArray<T>> &shortestPath) const
{
	for (node v1 : m_copyGraph.nodes) {
		for(adjEntry adj : v1->adjEntries) {
			node v2 = adj->twinNode();
			shortestPath[v1][v2] = shortestPath[v2][v1] = min(shortestPath[v1][v2], m_copyGraph.weight(adj->theEdge()));
		}
	}

	for (node pivot : m_copyGraph.nodes) {
		for (node v1 : m_copyGraph.nodes) {
			for (node v2 : m_copyGraph.nodes) {
				if (shortestPath[v1][pivot] == std::numeric_limits<T>::max()
				 || shortestPath[pivot][v2] == std::numeric_limits<T>::max()) {
					continue;
				}

				Math::updateMin(shortestPath[v1][v2], shortestPath[v1][pivot] + shortestPath[pivot][v2]);
			}
		}
	}
}

template<typename T>
inline void SteinerTreePreprocessing<T>::computeShortestPathMatrix(NodeArray<NodeArray<T>> &shortestPath) const
{
	shortestPath.init(m_copyGraph);
	for (node v : m_copyGraph.nodes) {
		shortestPath[v].init(m_copyGraph, std::numeric_limits<T>::max());
	}

	floydWarshall(shortestPath);
}

template<typename T>
bool SteinerTreePreprocessing<T>::leastCostTest() {
	OGDF_ASSERT(!m_copyGraph.empty());
	bool changed = false;
	NodeArray<NodeArray<T>> shortestPath;
	computeShortestPathMatrix(shortestPath);

	for (node v : m_copyGraph.nodes) {
		for (adjEntry adj = v->firstAdj(), nextAdj; adj; adj = nextAdj) {
			nextAdj = adj->succ();

			edge e = adj->theEdge();
			node adjNode = adj->twinNode();
			if (adjNode != v
			 && shortestPath[v][adjNode] < m_copyGraph.weight(e)) {
				m_copyGraph.delEdge(e);
				changed = true;
			}
		}
	}
	return changed;
}

template<typename T>
bool SteinerTreePreprocessing<T>::degree2Test() {
	OGDF_ASSERT(!m_copyGraph.empty());
	bool changed = false;
	for (node v = m_copyGraph.firstNode(), nextV; v; v = nextV) {
		nextV = v->succ();

		if (m_copyIsTerminal[v]
		 || v->degree() != 2 ) {
			continue;
		}

		// get left and right adjacent nodes
		edge eLeft = v->firstAdj()->theEdge();
		edge eRight = v->lastAdj()->theEdge();

		node vLeft = v->firstAdj()->twinNode();
		node vRight = v->lastAdj()->twinNode();
		if (vLeft != vRight) {
			T weight = m_copyGraph.weight(eLeft) + m_copyGraph.weight(eRight);
			edge newEdge = m_copyGraph.newEdge(vLeft, vRight, weight);
			addNewEdge(newEdge, {v}, {eLeft, eRight}, true);
			changed = true;
		} else { // v is leaf with parallel edges or self-loop component
			m_copyGraph.delNode(v);
		}
	}
	return changed;
}

template<typename T>
bool SteinerTreePreprocessing<T>::deleteComponentsWithoutTerminals()
{
	NodeArray<int> hisConnectedComponent(m_copyGraph, -1);
	bool changed = connectedComponents(m_copyGraph, hisConnectedComponent) > 1;
	if (changed) {
		int componentWithTerminals = -1;
		for (node v : m_copyTerminals) {
			if (componentWithTerminals != -1
			 && hisConnectedComponent[v] != componentWithTerminals) {
				std::cerr << "terminals in different connected components!\n";
				OGDF_ASSERT(false);
			}
			componentWithTerminals = hisConnectedComponent[v];
		}

		for (node v = m_copyGraph.firstNode(), nextV; v; v = nextV) {
			nextV = v->succ();
			if (hisConnectedComponent[v] != componentWithTerminals) {
				m_copyGraph.delNode(v);
			}
		}
	}
	return changed;
}

template<typename T>
bool SteinerTreePreprocessing<T>::terminalDistanceTest() {
	OGDF_ASSERT(!m_copyGraph.empty());
	OGDF_ASSERT(isConnected(m_copyGraph));
	bool changed = false;

	EdgeWeightedGraphCopy<T> tprime;
	steiner_tree::constructTerminalSpanningTreeUsingVoronoiRegions(tprime, m_copyGraph, m_copyTerminals);
	T maxBottleneck(0);
	for (edge e : tprime.edges) {
		Math::updateMax(maxBottleneck, tprime.weight(e));
	}

	for (edge e = m_copyGraph.firstEdge(), nextE; e; e = nextE) {
		nextE = e->succ();

		if (m_eps.greater(m_copyGraph.weight(e), maxBottleneck)) {
			m_copyGraph.delEdge(e);
			changed = true;
		}
	}

	return changed;
}

template<typename T>
void SteinerTreePreprocessing<T>::findClosestNonTerminals(node source, List<node> &reachedNodes, NodeArray<T> &distance, T maxDistance, int expandedEdges) const
{
	PrioritizedMapQueue<node, T> queue(m_copyGraph);

	// initialization
	distance[source] = 0;
	queue.push(source, distance[source]);

	while (!queue.empty()) {
		node currentNode = queue.topElement();
		queue.pop();

		reachedNodes.pushBack(currentNode);

		for(adjEntry adj : currentNode->adjEntries) {
			edge e = adj->theEdge();
			if (expandedEdges <= 0) {
				break;
			}
			--expandedEdges;

			node adjacentNode = e->opposite(currentNode);
			T possibleDistance = distance[currentNode] + m_copyGraph.weight(e);

			if (m_eps.geq(possibleDistance, maxDistance)
			 || m_copyIsTerminal[adjacentNode]) {
				continue;
			}

			if (possibleDistance < distance[adjacentNode]) {
				distance[adjacentNode] = possibleDistance;
				if (queue.contains(adjacentNode)) { // is already in the queue
					queue.decrease(adjacentNode, possibleDistance);
				} else {
					queue.push(adjacentNode, distance[adjacentNode]);
				}
			}
		}
	}
}

template<typename T>
bool SteinerTreePreprocessing<T>::longEdgesTest() {
	OGDF_ASSERT(!m_copyGraph.empty());
	bool changed = false;
	NodeArray<T> xDistance(m_copyGraph, std::numeric_limits<T>::max()),
	             yDistance(m_copyGraph, std::numeric_limits<T>::max());

	for (edge e = m_copyGraph.firstEdge(), nextE; e; e = nextE) {
		nextE = e->succ();
		List<node> xReachedNodes, yReachedNodes;

		findClosestNonTerminals(e->source(), xReachedNodes, xDistance, m_copyGraph.weight(e), 200);
		findClosestNonTerminals(e->target(), yReachedNodes, yDistance, m_copyGraph.weight(e), 200);

		for (node commonNode : xReachedNodes) {
			if (yDistance[commonNode] == std::numeric_limits<T>::max()) { // is not common
				continue;
			}
			if (m_eps.less(xDistance[commonNode] + yDistance[commonNode], m_copyGraph.weight(e))) {
				m_copyGraph.delEdge(e);
				changed = true;
				break;
			}
		}

		for (node reachedNode : xReachedNodes) {
			xDistance[reachedNode] = std::numeric_limits<T>::max();
		}
		for (node reachedNode : yReachedNodes) {
			yDistance[reachedNode] = std::numeric_limits<T>::max();
		}
	}
	return changed;
}

template<typename T>
void SteinerTreePreprocessing<T>::computeClosestKTerminals(const int k, NodeArray<List<std::pair<node,T>>> &closestTerminals) const
{
	using NodePairQueue = PrioritizedQueue<NodePair, T>;

	closestTerminals.init(m_copyGraph);
	NodePairQueue queue;
	std::unordered_map<NodePair, typename NodePairQueue::Handle, steiner_tree::UnorderedNodePairHasher, steiner_tree::UnorderedNodePairEquality> qpos;

	// initialization
	for (node v : m_copyTerminals) {
		closestTerminals[v].pushBack(std::make_pair(v, 0));
		qpos[NodePair(v, v)] = queue.push(NodePair(v, v), closestTerminals[v].front().second);
	}

	auto getCurrentDist = [&closestTerminals](const node currentNode, const node sourceTerminal) {
		for (std::pair<node,T> currentPair : closestTerminals[currentNode]) {
			if (currentPair.first == sourceTerminal) {
				return currentPair.second;
			}
		}
		return static_cast<T>(-1);
	};

	auto setNewDist = [&closestTerminals, k](const node currentNode, const node sourceTerminal, const T newDist) {
		List<std::pair<node, T>> &currentList = closestTerminals[currentNode];

		// delete the old distance
		for (ListIterator<std::pair<node, T>> it = currentList.begin(); it.valid(); ++it) {
			if ((*it).first == sourceTerminal) {
				currentList.del(it);
				break;
			}
		}

		if (currentList.size() == k) { // the list is full
			currentList.popBack(); // delete the largest cost element
		}

		// add the new distance such that the list remains sorted
		if (currentList.empty()
		 || (*currentList.begin()).second >= newDist) { // if the list is empty
			currentList.pushFront(std::make_pair(sourceTerminal, newDist));
			return;
		}

		ListIterator<std::pair<node, T>> it = closestTerminals[currentNode].begin();
		while (it.succ() != closestTerminals[currentNode].end()
		   && (*it.succ()).second < newDist) {
			++it;
		}
		closestTerminals[currentNode].insertAfter(std::make_pair(sourceTerminal, newDist), it);
	};

	while (!queue.empty()) {
		NodePair minDistPair = queue.topElement();
		queue.pop();
		node currentNode = minDistPair.source;
		node sourceTerminal = minDistPair.target;

		T currentDist = getCurrentDist(currentNode, sourceTerminal);
		if (currentDist == -1) { // source terminal not found
			continue; // check if the current path needs to be expanded
		}

		for(adjEntry adj : currentNode->adjEntries) {
			edge e = adj->theEdge();
			node adjacentNode = e->opposite(currentNode);

			if (m_copyIsTerminal[adjacentNode]) {
				continue;
			}

			T possibleNewDistance = currentDist + m_copyGraph.weight(e);

			// try to insert the new path from sourceTerminal to adjacentNode
			T currentDistToAdjacentNode = getCurrentDist(adjacentNode, sourceTerminal);
			if (currentDistToAdjacentNode != -1) { // there is already one path from sourceTerminal to adjNode
				if (possibleNewDistance < currentDistToAdjacentNode) {
					queue.decrease(qpos[NodePair(adjacentNode, sourceTerminal)], possibleNewDistance);
					setNewDist(adjacentNode, sourceTerminal, possibleNewDistance);
				}
			} else {
				if (closestTerminals[adjacentNode].size() < k
				 || closestTerminals[adjacentNode].back().second > possibleNewDistance) {
					qpos[NodePair(adjacentNode, sourceTerminal)] = queue.push(NodePair(adjacentNode, sourceTerminal), possibleNewDistance);
					setNewDist(adjacentNode, sourceTerminal, possibleNewDistance);
				}
			}
		}
	}
}

template<typename T>
bool SteinerTreePreprocessing<T>::PTmTest(const int k) {
	OGDF_ASSERT(!m_copyGraph.empty());
	OGDF_ASSERT(isConnected(m_copyGraph));
	bool changed = false;

	EdgeWeightedGraphCopy<T> tprime;
	steiner_tree::constructTerminalSpanningTreeUsingVoronoiRegions(tprime, m_copyGraph, m_copyTerminals);

	NodeArray<List<std::pair<node,T>>> closestTerminals;
	computeClosestKTerminals(k, closestTerminals);

	steiner_tree::HeavyPathDecomposition<T> tprimeHPD(tprime);

	for (edge e = m_copyGraph.firstEdge(), nextE; e; e = nextE) {
		nextE = e->succ();

		T bottleneckDistance = computeBottleneckDistance(e->source(), e->target(), tprime, tprimeHPD, closestTerminals);

		if (m_eps.greater(m_copyGraph.weight(e), bottleneckDistance)) {
			m_copyGraph.delEdge(e);
			changed = true;
		}
	}

	return changed;
}

template<typename T>
bool SteinerTreePreprocessing<T>::NTDkTest(const int maxTestedDegree, const int k) {
	OGDF_ASSERT(!m_copyGraph.empty());
	if (m_copyTerminals.size() <= 2) {
		return false;
	}

	bool changed = false;
	OGDF_ASSERT(isSimpleUndirected(m_copyGraph));
	OGDF_ASSERT(isConnected(m_copyGraph));

	EdgeWeightedGraphCopy<T> tprime;
	steiner_tree::constructTerminalSpanningTreeUsingVoronoiRegions(tprime, m_copyGraph, m_copyTerminals);

	NodeArray<List<std::pair<node,T>>> closestTerminals;
	computeClosestKTerminals(k, closestTerminals);

	steiner_tree::HeavyPathDecomposition<T> tprimeHPD(tprime);

	for (node v = m_copyGraph.firstNode(), nextV; v; v = nextV) {
		nextV = v->succ();

		if (m_copyIsTerminal[v]) {
			continue;
		}
		if (v->degree() <= 2
		 || v->degree() > maxTestedDegree) {
			continue;
		}

		// collect neighbors
		List<adjEntry> outgoingAdjs;
		for (adjEntry adj : v->adjEntries) {
			outgoingAdjs.pushBack(adj);
		}
		SubsetEnumerator<adjEntry> neighborSubset(outgoingAdjs);

		bool deleteNode = true;
		for (neighborSubset.begin(3, v->degree()); neighborSubset.valid() && deleteNode; neighborSubset.next()) {
			Graph auxGraph;
			std::unordered_map<node, node> initNodeToAuxGraph;
			std::unordered_map<node, node> auxGraphToInitNode;

			T sumToSelectedAdjacentNodes = 0;

			for (int i = 0; i < neighborSubset.size(); ++i) {
				const adjEntry adj = neighborSubset[i];
				const node adjacentNode = adj->twinNode();
				sumToSelectedAdjacentNodes += m_copyGraph.weight(adj->theEdge());

				OGDF_ASSERT(initNodeToAuxGraph.find(adjacentNode) == initNodeToAuxGraph.end());

				node newAuxGraphNode = auxGraph.newNode();
				initNodeToAuxGraph[adjacentNode] = newAuxGraphNode;
				auxGraphToInitNode[newAuxGraphNode] = adjacentNode;
			}

			EdgeArray<T> weight(auxGraph, 0);
			for (node auxGraphNode1 : auxGraph.nodes) {
				for (node auxGraphNode2 = auxGraphNode1->succ(); auxGraphNode2; auxGraphNode2 = auxGraphNode2->succ()) {
					edge e = auxGraph.newEdge(auxGraphNode1, auxGraphNode2);
					weight[e] = computeBottleneckDistance(
					  auxGraphToInitNode[auxGraphNode1],
					  auxGraphToInitNode[auxGraphNode2],
					  tprime, tprimeHPD, closestTerminals);
				}
			}

			// the auxGraph is now created; run MST on it
			EdgeArray<bool> isInTree(auxGraph, false);
			T mstCost = computeMinST(auxGraph, weight, isInTree);

			if (sumToSelectedAdjacentNodes < mstCost) {
				deleteNode = false;
			}
		}

		if (deleteNode) {
			deleteSteinerDegreeTwoNode(v, tprime, tprimeHPD, closestTerminals);
			changed = true;
		}
	}

	return changed;
}

template<typename T>
void SteinerTreePreprocessing<T>::markSuccessors(node currentNode, const Voronoi<T> &voronoiRegions, NodeArray<bool> &isSuccessorOfMinCostEdge) const
{
	isSuccessorOfMinCostEdge[currentNode] = true;

	OGDF_ASSERT(voronoiRegions.seed(currentNode) != currentNode);

	// recurse for every successor
	for (adjEntry adj : currentNode->adjEntries) {
		edge e = adj->theEdge();
		node adjacentNode = e->opposite(currentNode);

		if (voronoiRegions.predecessor(adjacentNode) == currentNode) {
			markSuccessors(adjacentNode, voronoiRegions, isSuccessorOfMinCostEdge);
		}
	}
}

template<typename T>
void SteinerTreePreprocessing<T>::findTwoMinimumCostEdges(node v, edge& first, edge& second) const {
	/* This is a simple problem, however, there is a weird case that can occur
	 * in nearestVertexTest(): Imagine a triangle of three terminals where the three edges
	 * have all equal costs. It can happen that each terminal chooses its first edge to
	 * be unique, so in the end we add all three edges of the triangle except only two of it.
	 * We tackle this problem at *this* stage by choosing the edge with smallest index
	 * in case of a tie in the costs. It is sufficient to do that for the first only
	 * because this is the edge that will be put into the solution. */
	for (adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		if (first == nullptr // not set or
		 || m_eps.less(m_copyGraph.weight(e), m_copyGraph.weight(first)) // smaller cost or
		 || (m_eps.equal(m_copyGraph.weight(e), m_copyGraph.weight(first)) // cost tie but...
		  && e->index() < first->index())) { // cost tie but smaller index
			second = first;
			first = e;
		} else {
			if (second == nullptr
			 || m_eps.less(m_copyGraph.weight(e), m_copyGraph.weight(second))) {
				second = e;
			}
		}
	}
	OGDF_ASSERT(first != second);
}

template<typename T>
bool SteinerTreePreprocessing<T>::addEdgesToSolution(const List<edge> &edgesToBeAddedInSolution)
{
	if (edgesToBeAddedInSolution.empty()) {
		return false;
	}
	for (edge e : edgesToBeAddedInSolution) {
		node x = e->source(), y = e->target();
		m_sonsList.emplace_back(std::vector<int>{m_nodeSonsListIndex[x], m_nodeSonsListIndex[y], m_edgeSonsListIndex[e]});
		m_costAlreadyInserted += m_copyGraph.weight(e);
		node newNode = m_copyGraph.contract(e);
		m_nodeSonsListIndex[newNode] = (int)m_sonsList.size() - 1;
		m_copyIsTerminal[newNode] = true;
	}

	recomputeTerminalsList();
	return true;
}

template<typename T>
bool SteinerTreePreprocessing<T>::nearestVertexTest() {
	OGDF_ASSERT(!m_copyGraph.empty());
	OGDF_ASSERT(isLoopFree(m_copyGraph));
	OGDF_ASSERT(isConnected(m_copyGraph));

	Voronoi<T> voronoiRegions(m_copyGraph, m_copyGraph.edgeWeights(), m_copyTerminals);

	NodeArray<edge> minCostIncidentEdge1(m_copyGraph, nullptr);
	NodeArray<edge> minCostIncidentEdge2(m_copyGraph, nullptr);

	// find the first and second minimum-cost edges incident to terminals
	for (node terminal : m_copyTerminals) {
		if (terminal->degree() >= 2) {
			findTwoMinimumCostEdges(terminal, minCostIncidentEdge1[terminal], minCostIncidentEdge2[terminal]);
		}
	}

	// for each Voronoi region, mark successors (recursively) of the minimum-cost edge
	NodeArray<bool> isSuccessorOfMinCostEdge(m_copyGraph, false);
	for (node terminal : m_copyTerminals) {
		if (terminal->degree() >= 2) {
			node closestNode = minCostIncidentEdge1[terminal]->opposite(terminal);

			if (voronoiRegions.seed(closestNode) == terminal) {
				markSuccessors(closestNode, voronoiRegions, isSuccessorOfMinCostEdge);
			}
		}
	}

	// compute for every terminal the distance to the closest terminal
	NodeArray<T> distanceToClosestTerminal(m_copyGraph, std::numeric_limits<T>::max());
	for (edge e : m_copyGraph.edges) {
		node x = e->source(), y = e->target();
		node seedX = voronoiRegions.seed(x), seedY = voronoiRegions.seed(y);
		if (seedX != seedY) {
			// update distanceToClosestTerminal for seed(x)
			T distanceThroughE = voronoiRegions.distance(x) + m_copyGraph.weight(e) + voronoiRegions.distance(y);

			if (isSuccessorOfMinCostEdge[x]) {
				Math::updateMin(distanceToClosestTerminal[seedX], distanceThroughE);
			}
			if (isSuccessorOfMinCostEdge[y]) {
				Math::updateMin(distanceToClosestTerminal[seedY], distanceThroughE);
			}
		}
	}

	// see what edges can be added in solution (Observation 3.2)
	List<edge> edgesToBeAddedInSolution;
	EdgeArray<bool> willBeAddedInSolution(m_copyGraph, false);
	for (node terminal : m_copyTerminals) {
		if (terminal->degree() >= 2) {
			const edge e = minCostIncidentEdge1[terminal];
			const node closestAdjacentNode = e->opposite(terminal);
			T distance{voronoiRegions.seed(closestAdjacentNode) == terminal
			  ? distanceToClosestTerminal[terminal]
			  : m_copyGraph.weight(e) + voronoiRegions.distance(closestAdjacentNode)};

			if (m_eps.geq(m_copyGraph.weight(minCostIncidentEdge2[terminal]), distance)
			 && !willBeAddedInSolution[e]) {
				edgesToBeAddedInSolution.pushBack(e);
				willBeAddedInSolution[e] = true;
			}
		}
	}

	return addEdgesToSolution(edgesToBeAddedInSolution);
}

template<typename T>
bool SteinerTreePreprocessing<T>::shortLinksTest() {
	OGDF_ASSERT(!m_copyGraph.empty());
	OGDF_ASSERT(isConnected(m_copyGraph));

	Voronoi<T> voronoiRegions(m_copyGraph, m_copyGraph.edgeWeights(), m_copyTerminals);

	NodeArray<edge> minCostLeavingRegionEdge1(m_copyGraph, nullptr);
	NodeArray<edge> minCostLeavingRegionEdge2(m_copyGraph, nullptr);

	// populate minCostLeavingRegions{1,2}
	for (edge e : m_copyGraph.edges) {
		auto updateMinCostLeavingRegionsFor = [&](node seed) {
			if (minCostLeavingRegionEdge1[seed] == nullptr
			 || m_copyGraph.weight(minCostLeavingRegionEdge1[seed]) > m_copyGraph.weight(e)) {
				minCostLeavingRegionEdge2[seed] = minCostLeavingRegionEdge1[seed];
				minCostLeavingRegionEdge1[seed] = e;
			} else
			if (minCostLeavingRegionEdge2[seed] == nullptr
			  || m_copyGraph.weight(minCostLeavingRegionEdge2[seed]) > m_copyGraph.weight(e)) {
				minCostLeavingRegionEdge2[seed] = e;
			}
		};
		const node x = e->source(), y = e->target();
		const node seedX = voronoiRegions.seed(x), seedY = voronoiRegions.seed(y);
		if (seedX != seedY) { // e is a link between Voronoi regions
			// update minCostLeavingRegions for both x and y
			updateMinCostLeavingRegionsFor(seedX);
			updateMinCostLeavingRegionsFor(seedY);
		}
	}

	List<edge> edgesToBeAddedInSolution;
	EdgeArray<bool> willBeAddedInSolution(m_copyGraph, false);
	for (node terminal : m_copyTerminals) {
		if (minCostLeavingRegionEdge2[terminal] == nullptr) {
			continue;
		}
		// XXX: hm, is that smart? If there is no second shortest edge, then the first shortest edge belongs to the solution, doesn't it?

		const edge e1 = minCostLeavingRegionEdge1[terminal];
		const node x = e1->source(), y = e1->target();
		if (m_eps.geq(m_copyGraph.weight(minCostLeavingRegionEdge2[terminal]),
		              voronoiRegions.distance(x) + m_copyGraph.weight(e1) + voronoiRegions.distance(y))
		 && !willBeAddedInSolution[e1]) {
			edgesToBeAddedInSolution.pushBack(e1);
			willBeAddedInSolution[e1] = true;
		}
	}

	return addEdgesToSolution(edgesToBeAddedInSolution);
}

template<typename T>
void SteinerTreePreprocessing<T>::computeRadiusOfTerminals(NodeArray<T> &terminalRadius) const
{
	OGDF_ASSERT(m_copyTerminals.size() > 1);

	// compute the Voronoi regions
	Voronoi<T> voronoiRegions(m_copyGraph, m_copyGraph.edgeWeights(), m_copyTerminals);

	// compute the radius for each terminal
	terminalRadius.init(m_copyGraph, std::numeric_limits<T>::max());
	for (node v : m_copyGraph.nodes) {
		node seedV = voronoiRegions.seed(v);
		T distanceToSeedV = voronoiRegions.distance(v);

		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node adjacentNode = e->opposite(v);

			if (voronoiRegions.seed(adjacentNode) != seedV) {
				Math::updateMin(terminalRadius[seedV], distanceToSeedV + m_copyGraph.weight(e));
			}
		}
	}
}

template<typename T>
bool SteinerTreePreprocessing<T>::deleteNodesAboveUpperBound(const NodeArray<T> &lowerBoundWithNode, const T upperBound)
{
	bool changed = false;
	for (node v = m_copyGraph.firstNode(), nextV; v; v = nextV) {
		nextV = v->succ();
		if (m_eps.greater(lowerBoundWithNode[v], upperBound)) {
			OGDF_ASSERT(!m_copyIsTerminal[v]);
			m_copyGraph.delNode(v);
			changed = true;
		}
	}
	return changed;
}

template<typename T>
bool SteinerTreePreprocessing<T>::lowerBoundBasedTest(T upperBound) {
	OGDF_ASSERT(!m_copyGraph.empty());
	if (m_copyTerminals.size() <= 1) {
		return false;
	}

	OGDF_ASSERT(isConnected(m_copyGraph));

	NodeArray<T> lowerBoundWithNode(m_copyGraph, std::numeric_limits<T>::lowest());

	NodeArray<List<std::pair<node, T>>> closestTerminals;
	computeClosestKTerminals(3, closestTerminals);

	// Update the lowerbound of the cost of a Steiner tree containing one particular node
	// as explained in [PV01, page 278, Observation 3.5]
	const T radiusSum = computeRadiusSum();
	for (node v : m_copyGraph.nodes) {
		if (m_copyIsTerminal[v]) {
			continue;
		}

		if (closestTerminals[v].size() < 2) {
			lowerBoundWithNode[v] = std::numeric_limits<T>::max();
			continue;
		}

		std::pair<node, T> closestTerminalPair1 = *(closestTerminals[v].get(0)), closestTerminalPair2 = *(closestTerminals[v].get(1));
		T distanceToClosestTerminal1 = closestTerminalPair1.second, distanceToClosestTerminal2 = closestTerminalPair2.second;

		Math::updateMax(lowerBoundWithNode[v], distanceToClosestTerminal1 + distanceToClosestTerminal2 + radiusSum);
	}

	// Compute lower bound for edges, see [PV01, page 278, Observation 3.5]
	EdgeArray<T> lowerBoundWithEdge(m_copyGraph, 0);
	for (edge e : m_copyGraph.edges) {
		node x = e->source(), y = e->target();

		T distanceToClosestTerminalX = (*closestTerminals[x].begin()).second;
		T distanceToClosestTerminalY = (*closestTerminals[y].begin()).second;

		Math::updateMax(lowerBoundWithEdge[e], m_copyGraph.weight(e) + distanceToClosestTerminalX + distanceToClosestTerminalY + radiusSum);
	}

	// Update the lowerbound of the cost of a Steiner tree containing one particular node
	// as explained in [PV01, pages 279-280, Observation 3.8]
	Graph auxiliaryGraph;
	NodeArray<node> terminalInAuxiliaryGraph(m_copyGraph, nullptr);
	for (node terminal : m_copyTerminals) {
		node newAuxiliaryNode = auxiliaryGraph.newNode();
		terminalInAuxiliaryGraph[terminal] = newAuxiliaryNode;
	}

	EdgeArray<T> initialEdgeWeight(m_copyGraph);
	for (edge e : m_copyGraph.edges) {
		initialEdgeWeight[e] = m_copyGraph.weight(e);
	}
	Voronoi<T> voronoiRegions(m_copyGraph, initialEdgeWeight, m_copyTerminals);

	std::unordered_map<NodePair, edge, steiner_tree::UnorderedNodePairHasher, steiner_tree::UnorderedNodePairEquality> edgeBetweenNodes;
	EdgeArray<T> edgeWeight(auxiliaryGraph, std::numeric_limits<T>::max());
	for (edge e : m_copyGraph.edges) {
		node x = e->source(), y = e->target();
		node seedX = voronoiRegions.seed(x), seedY = voronoiRegions.seed(y);
		if (seedX == seedY) {
			continue;
		}

		auto pair = NodePair(terminalInAuxiliaryGraph[seedX], terminalInAuxiliaryGraph[seedY]);
		if (edgeBetweenNodes.find(pair) == edgeBetweenNodes.end()) {
			edgeBetweenNodes[pair] = auxiliaryGraph.newEdge(terminalInAuxiliaryGraph[seedX], terminalInAuxiliaryGraph[seedY]);
		}
		edge auxiliaryEdge = edgeBetweenNodes[pair];
		Math::updateMin(edgeWeight[auxiliaryEdge], min(voronoiRegions.distance(x), voronoiRegions.distance(y)) + m_copyGraph.weight(e));
	}

	EdgeArray<bool> isInTree(auxiliaryGraph, false);
	T minimumSpanningTreeCost = computeMinST(auxiliaryGraph, edgeWeight, isInTree);
	T longestEdgeCost = std::numeric_limits<T>::lowest();
	for (edge e : auxiliaryGraph.edges) {
		if (isInTree[e]) {
			Math::updateMax(longestEdgeCost, edgeWeight[e]);
		}
	}

	for (node v : m_copyGraph.nodes) {
		if (m_copyIsTerminal[v] || closestTerminals[v].size() < 2) {
			continue;
		}

		std::pair<node, T> closestTerminalPair1 = *(closestTerminals[v].get(0)), closestTerminalPair2 = *(closestTerminals[v].get(1));
		T distanceToClosestTerminal1 = closestTerminalPair1.second, distanceToClosestTerminal2 = closestTerminalPair2.second;

		Math::updateMax(lowerBoundWithNode[v], minimumSpanningTreeCost - longestEdgeCost + distanceToClosestTerminal1 + distanceToClosestTerminal2);
	}

	return deleteNodesAboveUpperBound(lowerBoundWithNode, upperBound)
	    || deleteEdgesAboveUpperBound(lowerBoundWithEdge, upperBound);
}

template<typename T>
bool SteinerTreePreprocessing<T>::dualAscentBasedTest(int repetitions, T upperBound) {
	OGDF_ASSERT(!m_copyGraph.empty());
	bool changed = false;
	if (m_copyTerminals.size() > 1) {
		NodeArray<T> lowerBoundNodes;
		EdgeArray<T> lowerBoundEdges;
		SteinerTreeLowerBoundDualAscent<T> alg;
		alg.setRepetitions(repetitions);
		alg.call(m_copyGraph, m_copyTerminals, lowerBoundNodes, lowerBoundEdges);

		changed |= deleteNodesAboveUpperBound(lowerBoundNodes, upperBound);
		changed |= deleteEdgesAboveUpperBound(lowerBoundEdges, upperBound);
	}
	return changed;
}

template<typename T>
bool SteinerTreePreprocessing<T>::deleteEdgesAboveUpperBound(const EdgeArray<T> &lowerBoundWithEdge, const T upperBound)
{
	bool changed = false;
	for (edge e = m_copyGraph.firstEdge(), nextE; e; e = nextE) {
		nextE = e->succ();
		if (m_eps.greater(lowerBoundWithEdge[e], upperBound)) {
			m_copyGraph.delEdge(e);
			changed = true;
		}
	}
	return changed;
}

template<typename T>
T SteinerTreePreprocessing<T>::computeRadiusSum() const
{
	NodeArray<T> terminalRadius;
	computeRadiusOfTerminals(terminalRadius);

	// instead of sorting, we can simply ignore the two largest radii
	T radiusSum = T();
	T largestRadius1 = std::numeric_limits<T>::lowest(), largestRadius2 = std::numeric_limits<T>::lowest();
	for (node terminal : m_copyTerminals) {
		radiusSum += terminalRadius[terminal];

		if (terminalRadius[terminal] > largestRadius1) {
			largestRadius2 = largestRadius1;
			largestRadius1 = terminalRadius[terminal];
		} else {
			if (terminalRadius[terminal] > largestRadius2) {
				largestRadius2 = terminalRadius[terminal];
			}
		}
	}
	radiusSum -= largestRadius1 + largestRadius2;
	return radiusSum;
}

template<typename T>
bool SteinerTreePreprocessing<T>::reachabilityTest(int maxDegreeTest, const int k) {
	OGDF_ASSERT(!m_copyGraph.empty());
	OGDF_ASSERT(isSimpleUndirected(m_copyGraph));
	OGDF_ASSERT(isConnected(m_copyGraph));
	bool changed = false;
	if (maxDegreeTest <= 0) {
		maxDegreeTest = m_copyGraph.numberOfNodes();
	}

	EdgeWeightedGraphCopy<T> *approximatedSteinerTree;
	T upperBoundCost = computeMinSteinerTreeUpperBound(approximatedSteinerTree);

	NodeArray<bool> isInUpperBoundTree(m_copyGraph, false);
	for (node v : approximatedSteinerTree->nodes) {
		isInUpperBoundTree[approximatedSteinerTree->original(v)] = true;
	}
	delete approximatedSteinerTree;

	// Initialize tprime and its hpd decomposition used for partially not adding useless edges during nodes' deletion
	EdgeWeightedGraphCopy<T> tprime;
	steiner_tree::constructTerminalSpanningTreeUsingVoronoiRegions(tprime, m_copyGraph, m_copyTerminals);
	OGDF_ASSERT(!tprime.empty());

	NodeArray<List<std::pair<node,T>>> closestTerminals;
	computeClosestKTerminals(k, closestTerminals);

	steiner_tree::HeavyPathDecomposition<T> tprimeHPD(tprime);

	// check which nodes can be deleted
	for (node v = m_copyGraph.firstNode(), nextV; v; v = nextV) {
		nextV = v->succ();

		if (isInUpperBoundTree[v]
		 || v->degree() > maxDegreeTest) {
			continue;
		}

		// compute v's farthest and closest terminals
		Dijkstra<T> dijkstra;

		NodeArray<T> distance(m_copyGraph);
		NodeArray<edge> predecessor(m_copyGraph, nullptr);
		dijkstra.call(m_copyGraph, m_copyGraph.edgeWeights(), v, predecessor, distance);

		// compute first, second nearest terminals and farthest terminal
		node farthestTerminal = nullptr;
		T distanceToFarthestTerminal(0);
		T distanceToClosestTerminal1 = std::numeric_limits<T>::max(),
		  distanceToClosestTerminal2 = std::numeric_limits<T>::max();
		for (node terminal : m_copyTerminals) {
			if (distanceToFarthestTerminal < distance[terminal]) {
				farthestTerminal = terminal;
				distanceToFarthestTerminal = distance[terminal];
			}

			if (distanceToClosestTerminal1 > distance[terminal]) {
				distanceToClosestTerminal2 = distanceToClosestTerminal1;
				distanceToClosestTerminal1 = distance[terminal];
			} else {
				if (distanceToClosestTerminal2 > distance[terminal]) {
					distanceToClosestTerminal2 = distance[terminal];
				}
			}
		}

		if (predecessor[farthestTerminal] == nullptr // is not in the same component with the terminals
		 || distanceToClosestTerminal2 == std::numeric_limits<T>::max() // cannot reach at least 2 terminals, must be deleted
		 || m_eps.geq(distanceToFarthestTerminal + distanceToClosestTerminal1 + distanceToClosestTerminal2, upperBoundCost)) {
			changed = true;
			// delete the node
			if (predecessor[farthestTerminal] != nullptr
			 && distanceToClosestTerminal2 != std::numeric_limits<T>::max()
			 && m_eps.less(distanceToFarthestTerminal + distanceToClosestTerminal1, upperBoundCost)) {
				// the deleted node has degree 2 -> replace it with edges
				deleteSteinerDegreeTwoNode(v, tprime, tprimeHPD, closestTerminals);
			} else {
				// just delete the node
				m_copyGraph.delNode(v);
			}
		}
	}

	return changed;
}

template<typename T>
template<typename LAMBDA>
void SteinerTreePreprocessing<T>::computeOptimalTerminals(node v, LAMBDA dist, node &optimalTerminal1, node &optimalTerminal2, NodeArray<T> &distance) const
{
	// run Dijkstra starting from v
	Dijkstra<T> dijkstra;

	distance.init(m_copyGraph);
	NodeArray<edge> predecessor(m_copyGraph, nullptr);
	dijkstra.call(m_copyGraph, m_copyGraph.edgeWeights(), v, predecessor, distance);

	for (node terminal : m_copyTerminals) {
		if (predecessor[terminal] == nullptr) {
			continue;
		}

		if (optimalTerminal1 == nullptr
		 || dist(optimalTerminal1, distance) > dist(terminal, distance)) {
			optimalTerminal2 = optimalTerminal1;
			optimalTerminal1 = terminal;
		} else {
			if (optimalTerminal2 == nullptr
			 || dist(optimalTerminal2, distance) > dist(terminal, distance)) {
				optimalTerminal2 = terminal;
			}
		}
	}
	OGDF_ASSERT(optimalTerminal1 != nullptr);
	OGDF_ASSERT(optimalTerminal2 != nullptr);
	OGDF_ASSERT(optimalTerminal1 != optimalTerminal2);
}

template<typename T>
bool SteinerTreePreprocessing<T>::cutReachabilityTest() {
	OGDF_ASSERT(!m_copyGraph.empty());
	if (m_copyTerminals.size() <= 2) {
		return false;
	}

	// get the upper bound
	EdgeWeightedGraphCopy<T> *approximatedSteinerTree;
	const T upperBoundCost = computeMinSteinerTreeUpperBound(approximatedSteinerTree);

	NodeArray<bool> isInUpperBoundTree(m_copyGraph, false);
	for (node v : approximatedSteinerTree->nodes) {
		isInUpperBoundTree[approximatedSteinerTree->original(v)] = true;
	}
	delete approximatedSteinerTree;

	T cK = T();
	NodeArray<T> minCostOfAdjacentEdge(m_copyGraph, std::numeric_limits<T>::max());
	for (node terminal : m_copyTerminals) {
		for (adjEntry adj : terminal->adjEntries) {
			Math::updateMin(minCostOfAdjacentEdge[terminal], m_copyGraph.weight(adj->theEdge()));
		}
		cK += minCostOfAdjacentEdge[terminal];
	}
	auto dist = [&minCostOfAdjacentEdge](node terminal, const NodeArray<T> &distance) {
		return distance[terminal] - minCostOfAdjacentEdge[terminal];
	};

	List<node> delNodes;
	std::set<edge> delEdges;
	NodeArray<T> vDistance;
	NodeArray<T> wDistance;
	for (node v = m_copyGraph.firstNode(), nextV; v; v = nextV) {
		nextV = v->succ();

		if (isInUpperBoundTree[v]) {
			continue;
		}

		// compute its optimal terminals
		node vOptimalTerminal1 = nullptr, vOptimalTerminal2 = nullptr;
		computeOptimalTerminals(v, dist, vOptimalTerminal1, vOptimalTerminal2, vDistance);

		// check whether it can be deleted
		if (m_eps.geq(cK + dist(vOptimalTerminal1, vDistance) + dist(vOptimalTerminal2, vDistance), upperBoundCost)) {
			delNodes.pushBack(v);
		} else { // it is not deleted, perform the edge test
			for (adjEntry adj = v->firstAdj(), adjNext; adj; adj = adjNext) {
				adjNext = adj->succ();
				node w = adj->twinNode();
				if (m_copyIsTerminal[w]) {
					continue;
				}
				node wOptimalTerminal1 = nullptr, wOptimalTerminal2 = nullptr;
				computeOptimalTerminals(w, dist, wOptimalTerminal1, wOptimalTerminal2, wDistance);

				node vOptimalTerminal = vOptimalTerminal1;
				node wOptimalTerminal = wOptimalTerminal1;
				if (vOptimalTerminal == wOptimalTerminal) {
					// the nearest terminals to v and w are the same, but they have to be
					// different. Obtain the minimum choice such that they are different.
					if (m_eps.leq(
					     dist(vOptimalTerminal1, vDistance) + dist(wOptimalTerminal2, wDistance),
					     dist(vOptimalTerminal2, vDistance) + dist(wOptimalTerminal1, wDistance))) {
						wOptimalTerminal = wOptimalTerminal2;
					} else {
						vOptimalTerminal = vOptimalTerminal2;
					}
				}
				OGDF_ASSERT(vOptimalTerminal != wOptimalTerminal);
				if (m_eps.geq(cK + dist(vOptimalTerminal, vDistance) + dist(wOptimalTerminal, wDistance) + m_copyGraph.weight(adj->theEdge()), upperBoundCost)) {
					delEdges.insert(adj->theEdge());
				}
			}
		}
	}

	bool changed = false;
	for (edge e : delEdges) {
		m_copyGraph.delEdge(e);
		changed = true;
	}
	for (node v : delNodes) {
		m_copyGraph.delNode(v);
		changed = true;
	}
	return changed;
}

}
