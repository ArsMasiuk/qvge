/** \file
 * \brief Definition of ogdf::steiner_tree::goemans::BlowupGraph class template
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

#include <ogdf/basic/Math.h>
#include <ogdf/basic/HashArray.h>
#include <ogdf/graphalg/steiner_tree/FullComponentStore.h>
#include <ogdf/graphalg/steiner_tree/goemans/CoreEdgeModule.h>

namespace ogdf {
namespace steiner_tree {
namespace goemans {

/**
 * A special-purpose blowup graph for gammoid computation:
 * directed, with special source and target, with core edges (implemented as nodes)
 */
template<typename T>
class BlowupGraph {
private:
	Graph m_graph;
	const FullComponentWithExtraStore<T, double> &m_fullCompStore; //!< all enumerated full components, with solution
	const double m_eps; //!< epsilon for double operations

	List<node> m_terminals; //!< The terminals in the blowup graph
	NodeArray<bool> m_isTerminal; //!< Incidence vector for the blowup graph terminals

	//! Mapping of blowup graph nodes to original nodes.
	//! If a node in a blowup graph represents more than one (contracted),
	//! it maps just to one original node.
	//! If it maps to nullptr, there is no original node, i.e. we have
	//! a core edge, a source, pseudotarget or target.
	NodeArray<node> m_original;

	EdgeArray<T> m_cost;
	EdgeArray<int> m_capacity;

	int m_lcm;
	int m_y;
	node m_source;
	node m_pseudotarget;
	node m_target;

	const CoreEdgeModule<T> &m_ceModule;

	List<node> m_coreEdges; //!< the core edges as nodes

	/* witness set (for some component and specified K (core edge set))
	 *
	 *  W(e) = { e } if e is a core edge
	 *  W(e) =
	 *     take the path P of loss edges from u to e in the component
	 *     add every incident core edge of u
	 *  that is: if we root the loss connected component at the terminal,
	 *   W(e) = all core edges incident to the subtree below e
	 *
	 * Need fast lookups for:
	 *  (1) |W(f)|
	 *  (2) all loss edges f such that given core edge e is in W(f)
	 *  (3) all loss edges f such that W(f) is a subset of a given basis -> for each e \in B: do (2)
	 *
	 * Data Structures:
	 *  - witnessCard[e] = |W(e)|
	 *  - witness[v_e] = { f | e \in W(f) }
	 * (Note that core edges are given as nodes.)
	 * Also note that we can save it all much sparser.
	 */
	EdgeArray<int> m_witnessCard;
	NodeArray<ArrayBuffer<edge>> m_witness;

protected:
	//! Computes the least common multiple of the values assigned to the full components
	void computeLCM() {
		ArrayBuffer<int> denominators;

		for (int i = 0; i < m_fullCompStore.size(); ++i) {
			OGDF_ASSERT(m_fullCompStore.extra(i) <= 1.0 + m_eps);
			OGDF_ASSERT(m_fullCompStore.extra(i) >= m_eps);
			int num, denom;
			Math::getFraction(m_fullCompStore.extra(i), num, denom);
			OGDF_ASSERT(Math::gcd(num, denom) == 1);
			denominators.push(denom);
		}

		m_lcm = 1;
		for (int denom : denominators) {
			m_lcm = Math::lcm(m_lcm, denom);
		}
	}

	//! Inserts a terminal
	node initTerminal(node t) {
		const node v = m_graph.newNode();
		m_isTerminal[v] = true;
		m_terminals.pushBack(v);
		m_original[v] = t;
		return v;
	}

	node initNode(node v) {
		const node vCopy = m_graph.newNode();
		m_original[vCopy] = v;
		return vCopy;
	}

	//! Does a bfs of the component tree to add *directed* components with the first terminal as root
	//! @return the root of the component
	node initBlowupGraphComponent(const NodeArray<node> &copy, adjEntry start, int cap) {
		node v = m_fullCompStore.original(start->theNode());
		List<adjEntry> queueT;
		List<node> queueC;
		queueT.pushBack(start);
		queueC.pushBack(copy[v]);
		while (!queueT.empty()) {
			const adjEntry inAdj = queueT.popFrontRet();
			const node wT = inAdj->twinNode();
			const node vC = queueC.popFrontRet();

			const node wO = m_fullCompStore.original(wT);
			if (m_fullCompStore.isTerminal(wT)) {
				newEdge(vC, copy[wO], m_fullCompStore.graph().weight(inAdj->theEdge()), cap);
			} else { // not a terminal
				const node wC = initNode(wO);
				newEdge(vC, wC, m_fullCompStore.graph().weight(inAdj->theEdge()), cap);
				const adjEntry back = inAdj->twin();
				for (adjEntry adj = back->cyclicSucc(); adj != back; adj = adj->cyclicSucc()) {
					queueT.pushBack(adj);
					queueC.pushBack(wC);
				}
			}
		}
		return copy[v];
	}

	//! Connects source to component roots
	void initSource(ArrayBuffer<std::pair<node, int>> &roots) {
		OGDF_ASSERT(m_source == nullptr);
		m_source = m_graph.newNode();
		for (auto root : roots) {
			newEdge(getSource(), root.first, 0, root.second);
		}
	}

	//! Initializes all components in the blowup graph as well as core edges and witness sets
	void initBlowupGraphComponents(const EdgeWeightedGraph<T> &originalGraph, const List<node> &terminals) {
		ArrayBuffer<std::pair<node, int>> roots; // roots (in the blowupGraph) and their capacities

		NodeArray<node> copy(originalGraph, nullptr);
		for (node t : terminals) { // generate all terminals
			copy[t] = initTerminal(t);
		}
		for (int i = 0; i < m_fullCompStore.size(); ++i) {
			int cap = int(getLCM() * m_fullCompStore.extra(i) + m_eps);
			node root = initBlowupGraphComponent(copy, m_fullCompStore.start(i), cap);
			roots.push(std::make_pair(root, cap));
		}

		removeIsolatedTerminals(); // can exist by preprocessing

		// compute core edges (and replace these edges by nodes)
		// and witness sets
		initCoreWitness();

		initSource(roots);
	}

	//! Connects pseudotarget
	void initPseudotarget() {
		OGDF_ASSERT(m_pseudotarget == nullptr);
		m_pseudotarget = m_graph.newNode();

		for (node v : terminals()) {
			OGDF_ASSERT(v);
			int y_v = -getLCM();
			// compute y_v, the number of components containing v in the blow up graph - N
			// NOTE: for the non-blowup variant this is simply the sum of all x_C where C contains v ... - 1
			for (adjEntry adj : v->adjEntries) {
				if (adj->twinNode() != getSource()) {
					y_v += getCapacity(adj->theEdge());
				}
			}
			OGDF_ASSERT(y_v >= 0);

			if (y_v > 0) {
				newEdge(v, getPseudotarget(), 0, y_v);
				m_y += y_v;
			}
		}
	}

	//! Connects target
	void initTarget() {
		OGDF_ASSERT(m_target == nullptr);
		m_target = m_graph.newNode();

		newEdge(getPseudotarget(), getTarget(), 0, m_y);
	}

	//! Updates arc capacities s->v and v->t
	int updateSourceAndTargetArcCapacities(const node v) {
		int delta = 0;
		int capSource = 0;
		int capTarget = -getLCM();
		adjEntry adj2;
		for (adjEntry adj = v->firstAdj(); adj; adj = adj2) {
			adj2 = adj->succ();
			if (adj->twinNode() == getSource()) {
				// remove arcs from the source
				m_graph.delEdge(adj->theEdge());
			}
			else
			if (adj->twinNode() == getPseudotarget()) {
				// remove arcs to the pseudotarget
				delta -= getCapacity(adj->theEdge());
				m_graph.delEdge(adj->theEdge());
			}
			else {
				// compute y_v for the contraction node
				capTarget += getCapacity(adj->theEdge());
				// compute s->v capacity
				if (v != adj->theEdge()->target()) { // outgoing edge
					capSource += getCapacity(adj->theEdge());
				}
			}
		}
		OGDF_ASSERT(capTarget >= 0);
		if (capTarget > 0) {
			newEdge(v, getPseudotarget(), 0, capTarget);
		}
		if (capSource > 0) {
			newEdge(getSource(), v, 0, capSource);
		}

		return delta + capTarget;
	}

	void setCapacity(edge e, int capacity) {
		m_capacity[e] = capacity;
	}

	//! @name Core edges and witness set management
	//! @{

	//! Adds a core edge
	//! \remark Note that core edges are implemented by nodes in the blowup graph
	void addCore(node e) {
		m_coreEdges.pushBack(e);
	}

	//! Adds \p e to W(\p f)
	void addWitness(node e, edge f) {
		++m_witnessCard[f];
		m_witness[e].push(f);
	}

	/**
	 * Finds a "random" set of core edges and "replace" found edges by nodes,
	 * also find the witness sets for the core edges
	 * \todo Derandomization: find best set of core edges using dynamic programming
	 */
	void initCoreWitness() {
		m_witnessCard.init(m_graph, 0);
		m_witness.init(m_graph);

		// compute set of core edges
		EdgeArray<bool> isLossEdge;
		m_ceModule.call(m_graph, terminals(), isLossEdge);

		// add nodes for core edges and be able to map them
		EdgeArray<node> splitMap(m_graph, nullptr);
		ArrayBuffer<edge> coreEdges;
		for (edge e : m_graph.edges) {
			if (!isLossEdge[e]) {
				splitMap[e] = m_graph.newNode();
				coreEdges.push(e);
			}
		}

		// traverse losses from all terminals to find witness sets
		NodeArray<adjEntry> pred(m_graph, nullptr);
		for (node t : terminals()) {
			ArrayBuffer<node> stack;
			stack.push(t);
			while (!stack.empty()) {
				// for each node v "below" an edge e in the traversal:
				//   add all incident core edges vw to W(e)
				const node v = stack.popRet();
				for (adjEntry adj : v->adjEntries) {
					const edge e = adj->theEdge();
					const node w = adj->twinNode();
					if (!pred[v] || w != pred[v]->theNode()) { // do not look at backward arcs in the tree
						if (isLossEdge[e]) {
							stack.push(w);
							pred[w] = adj;
						} else {
							for (node x = v; pred[x]; x = pred[x]->theNode()) {
								addWitness(splitMap[e], pred[x]->theEdge());
							}
						}
					}
				}
			}
		}

		// finally replace core edges by nodes
		for (edge e : coreEdges) {
			const T w = getCost(e);
			const node x = splitMap[e];
			const int cap = getCapacity(e);
			OGDF_ASSERT(x);
			newEdge(e->source(), x, w, cap);
			newEdge(x, e->target(), w, cap);
			// the cost of a core edge node is hence the weight of one incident edge; also keep capacity
			m_graph.delEdge(e);
			addCore(x);
		}
	}

	//! Copies witness sets and core edges for a given copy map
	void makeCWCopy(const HashArray<edge,edge> &edgeMap) {
		for (HashConstIterator<edge,edge> pair = edgeMap.begin(); pair.valid(); ++pair) {
			const edge eO = pair.key();
			const edge eC = pair.info();
			const node vO = eO->target();
			const node vC = eC->target();
			m_witnessCard[eC] = m_witnessCard[eO]; // copy witness cardinality
			if (vC == vO) { // target is a terminal, so it cannot be a core edge
				continue;
			}
			for (ListIterator<node> it = m_coreEdges.begin(); it.valid(); ++it) {
				if (*it == vO) { // vO is a core edge
					m_coreEdges.insertAfter(vC, it); // make vC a core edge

					// copy witness sets
					// XXX: do we need this or are witness sets computed after the loop again?!
					for (edge e : m_witness[vO]) {
						m_witness[vC].push(edgeMap[e]);
					}
					break;
				}
			}
		}
	}

	//! @}

public:
	//! Initializes a blowup graph including core edges and witness sets
	BlowupGraph(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const FullComponentWithExtraStore<T, double> &fullCompStore, const CoreEdgeModule<T> &ceModule, double eps = 1e-8)
	  : m_fullCompStore(fullCompStore)
	  , m_eps(eps)
	  , m_terminals()
	  , m_isTerminal(m_graph, false)
	  , m_original(m_graph, nullptr)
	  , m_cost(m_graph)
	  , m_capacity(m_graph)
	  , m_y(0)
	  , m_source(nullptr)
	  , m_pseudotarget(nullptr)
	  , m_target(nullptr)
	  , m_ceModule(ceModule)
	{
		computeLCM();

		initBlowupGraphComponents(G, terminals);
		initPseudotarget();
		initTarget();
	}

	//! @name Getters for the blow-up graph (without core edges and witness sets)
	//! @{

	const Graph &getGraph() const {
		return m_graph;
	}

	//! Returns the source node
	node getSource() const {
		return m_source;
	}

	//! Returns the pseudotarget node
	node getPseudotarget() const {
		return m_pseudotarget;
	}

	//! Returns the target node
	node getTarget() const {
		return m_target;
	}

	//! Returns the capacity of \p e
	int getCapacity(edge e) const {
		return m_capacity[e];
	}

	//! Returns a reference to the edge array containing all capacities
	const EdgeArray<int> &capacities() const {
		return m_capacity;
	}

	//! Returns the cost of \p e
	T getCost(edge e) const {
		return m_cost[e];
	}

	//! Returns the original node of \p v
	node getOriginal(node v) const {
		return m_original[v];
	}

	//! Returns the least common multiple of all denominators
	int getLCM() const {
		return m_lcm;
	}

	//! Returns the y-value of all terminals
	int getY() const {
		return m_y;
	}

	//! Returns a reference to the list containing all terminals in the blowup graph
	const List<node> &terminals() const {
		return m_terminals;
	}

	//! Returns true if and only if \p v is a terminal
	bool isTerminal(node v) const {
		return m_isTerminal[v];
	}

	//! @}

	//! Updates capacities from source to terminals and terminals to pseudotarget
	void updateSpecialCapacities() {
#if 0
		m_y += updateSourceAndTargetArcCapacities(v);
#endif
		for (node t : terminals()) {
			m_y += updateSourceAndTargetArcCapacities(t);
		}
		// XXX: doing it for v and all terminals we have met during cleanup would be sufficient
		OGDF_ASSERT(getTarget()->degree() == 1);
		setCapacity(getTarget()->firstAdj()->theEdge(), m_y);
	}

	//! Adds and returns a new edge between \p v and \p w of specified \p cost and \p capacity
	edge newEdge(node v, node w, T cost, int capacity) {
		edge e = m_graph.newEdge(v, w);
		m_cost[e] = cost;
		m_capacity[e] = capacity;
		return e;
	}

	//! Removes edges in \p edges
	void delEdges(ArrayBuffer<edge> edges) {
		for (edge e : edges) {
			m_graph.delEdge(e);
		}
	}

	//! Contracts node \p v and terminal \p t into \p v
	void contract(node &v, node t) {
		if (v->degree() == 0) {
			std::swap(v, t);
		}

		OGDF_ASSERT(m_isTerminal[t]);
		m_terminals.removeFirst(t);
		m_isTerminal[t] = false;

		if (t->degree() > 0) {
			v = m_graph.contract(m_graph.newEdge(v, t));
			// the contract method ensures that capacities, weights, and everything is kept
		} else {
			m_graph.delNode(t);
		}
	}

	/**
	 * Removes a basis and cleans up
	 *
	 * @param v a core edge node of the basis (to determine the basis)
	 * @param newRootFunction determines what to do with newly emerging component roots
	 */
	void removeBasis(node v, std::function<void(edge)> newRootFunction) {
		ArrayBuffer<node> cleanup;
		cleanup.push(v->firstAdj()->twinNode());
		cleanup.push(v->lastAdj()->twinNode());
		OGDF_ASSERT(v->degree() == 2);
		OGDF_ASSERT(v->firstAdj()->twinNode() != v->lastAdj()->twinNode());
		m_graph.delNode(v);

		while (!cleanup.empty()) {
			v = cleanup.popRet();
			if (!isTerminal(v)) {
				OGDF_ASSERT(v->degree() >= 1);
				if (v->degree() == 1) { // v is a pendant node, delete
					cleanup.push(v->firstAdj()->twinNode());
					m_graph.delNode(v);
				} else
				if (v->indeg() == 0) { // v has no incoming edge, fix
					const node w = v->firstAdj()->twinNode();
					const edge e = v->firstAdj()->theEdge();
					m_graph.reverseEdge(e);
					OGDF_ASSERT(e->source() == w);
					if (!isTerminal(w)) {
						cleanup.push(w);
						// when w is cleaned up, it must not go back to v
						if (w->firstAdj()->theEdge() == e) {
							// move first adjacency entries of w away (w->v is not first anymore)
							m_graph.moveAdjAfter(w->firstAdj(), w->lastAdj());
						}
					} else {
						// a terminal means we have a new root
						newRootFunction(e);
					}
				}
			}
		}
	}

	//! Removes isolated terminals
	void removeIsolatedTerminals() {
		for (ListIterator<node> it = m_terminals.begin(), itNext; it.valid(); it = itNext) {
			itNext = it.succ();
			if ((*it)->degree() == 0) {
				m_graph.delNode(*it);
				m_terminals.del(it);
			}
		}
	}

	//! Copy a component in the blowup graph and set original capacity to \a origCap and capacity of copy to \a copyCap
	void copyComponent(const edge origEdge, const int origCap, const int copyCap) {
		if (copyCap == 0) {
			return;
		}
		List<edge> todo;
		List<node> origin;
		HashArray<edge,edge> edgeMap;
		todo.pushBack(origEdge);
		origin.pushBack(origEdge->source());
		while (!todo.empty()) {
			edge eO = todo.popFrontRet();
			node vC = origin.popFrontRet();
			node wO = eO->target();
			node wC = wO;
			if (!isTerminal(wO)) {
				wC = initNode(getOriginal(wO));
			}
			edge eC = newEdge(vC, wC, getCost(eO), copyCap);
			setCapacity(eO, origCap);
			edgeMap[eO] = eC;
			if (!isTerminal(wO)) {
				for (adjEntry adj = eO->adjTarget()->cyclicSucc(); adj != eO->adjTarget(); adj = adj->cyclicSucc()) {
					OGDF_ASSERT(adj->theEdge()->target() != eO->target()); // outgoing edges
					origin.pushBack(wC);
					todo.pushBack(adj->theEdge());
				}
			}
		}
		makeCWCopy(edgeMap);
	}

	//! @name Core edges and witness set management
	//! @{

	//! Return list of core edges (implemented by nodes)
	const List<node> &core() const {
		return m_coreEdges;
	}

	//! Remove a core edge
	//! \note the blowup graph is not affected
	void delCore(node e) {
		/* What happens when we remove a core edge?
		 *  - loss edges are not affected
		 *  - we have to remove a core edge e from W(f) for all f, which means:
		 *    for all elements f of witness[v_e], decrease witnessCard[f], then remove witness[v_e]
		 */
		for (edge f : m_witness[e]) {
			--m_witnessCard[f];
		}
		// witness[e] is removed by removing the node from the graph
		m_coreEdges.removeFirst(e);
	}

	//! Return the number of witnesses of an edge
	int numberOfWitnesses(edge e) const {
		return m_witnessCard[e];
	}

	//! Return list of loss edges f in W(e)
	const ArrayBuffer<edge> &witnessList(node e) const {
		return m_witness[e];
	}

	//! @}
};

}
}
}
