/** \file
 * \brief Declaration of min-st-cut algorithms parameterized by max-flow alorithm.
 *
 * \author Mirko Wagner, Tilo Wiedera
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

#include <memory>
#include <ogdf/graphalg/MinSTCutModule.h>
#include <ogdf/graphalg/MaxFlowGoldbergTarjan.h>

namespace ogdf {

/**
 * Min-st-cut algorithm, that calculates the cut via maxflow.
 *
 * @tparam TCost The type in which the weight of the edges is given.
 */
template<typename TCost>
class MinSTCutMaxFlow: public MinSTCutModule<TCost> {
	using MinSTCutModule<TCost>::m_gc;
public:
	/**
	 * Constructor
	 *
	 * @param treatAsUndirected States whether or not
	 * @param mfModule The MaxFlowModule that is used to calculate a flow.
	 * The module will be deleted, when the lifetime of this object is over.
	 * @param primaryCut true if the algorithm should search for the mincut nearest to \a s,
	 * false if it should be near to \a t.
	 * @param calculateOtherCut if true, the other cut (primaryCut == false : front cut, primaryCut == true : back cut)
	 * should also be calculated.
	 * Setting this to false will speed up the algorithm a bit, but some functions
	 * might not work correctly or at all.
	 * @param epsilonTest The module used for epsilon tests.
	 * The module will be deleted, when the lifetime of this object is over.
	 */
	explicit MinSTCutMaxFlow(bool treatAsUndirected = true, MaxFlowModule<TCost> *mfModule = new MaxFlowGoldbergTarjan<TCost>(),
	                bool primaryCut = true, bool calculateOtherCut = true,
	                EpsilonTest *epsilonTest = new EpsilonTest()) :
		m_mfModule(mfModule), m_treatAsUndirected(treatAsUndirected), m_primaryCut(primaryCut),
		m_calculateOtherCut(calculateOtherCut), m_et(epsilonTest)
	{ }

	/**
	 * @copydoc ogdf::MinSTCutModule<TCost>::call(const Graph&,const EdgeArray<TCost>&,node,node,List<edge>&,edge)
	 */
	virtual bool call(const Graph &graph, const EdgeArray<TCost> &weight, node s, node t,
	                  List<edge> &edgeList, edge e_st = nullptr) override;

	/**
	 * @copydoc ogdf::MinSTCutModule<TCost>::call(const Graph&,node,node,List<edge>&,edge)
	 */
	virtual bool call(const Graph &graph, node s, node t, List<edge> &edgeList, edge e_st = nullptr) override {
		EdgeArray<TCost> weight(graph, 1);
		return call(graph, weight, s, t, edgeList, e_st);
	}

	/**
	 * Partitions the nodes to front- and backcut.
	 *
	 * @param graph The underlying graph
	 * @param weights The weights (aka capacity) of the edges
	 * @param flow The precomputed flow of each edge
	 * @param source The source of the min cut
	 * @param target the target (aka sink) of the minimum cut
	 */
	void call(const Graph &graph, const EdgeArray<TCost> &weights, const EdgeArray<TCost> &flow, const node source, const node target);

	/**
	 * The three types of cuts.
	 */
	enum class cutType {
		FRONT_CUT, //!< node is in front cut
		BACK_CUT, //!< node is in back cut
		NO_CUT //!< node is not part of any cut
	};

	/**
	 * Assigns a new epsilon test.
	 */
	void setEpsilonTest(EpsilonTest *et)
	{
		m_et.reset(et);
	}

	/**
	 * Returns whether the front cut is the complement of
	 * the backcut. i.e. there are no nodes not assigned to
	 * one of both cut types.
	 * @pre \c calculateOtherCut has to be set to true in the constructor
	 */
	bool frontCutIsComplementOfBackCut() const
	{
		OGDF_ASSERT(m_calculateOtherCut);
		return m_backCutCount + m_frontCutCount == m_totalCount;
	}

	/**
	 * Returns whether this edge is leaving the front cut.
	 */
	bool isFrontCutEdge(const edge e) const
	{
		OGDF_ASSERT(m_calculateOtherCut || m_primaryCut);
		return m_nodeSet[e->source()] == cutType::FRONT_CUT
			   && m_nodeSet[e->target()] != cutType::FRONT_CUT;
	}

	/**
	 * Returns whether this edge is entering the back cut.
	 */
	bool isBackCutEdge(const edge e) const
	{
		OGDF_ASSERT(m_calculateOtherCut || !m_primaryCut);
		return m_nodeSet[e->target()] == cutType::BACK_CUT
			   && m_nodeSet[e->source()] != cutType::BACK_CUT;
	}

	/**
	 * Returns whether this node is part of the front cut.
	 * Meaning it is located in the same set as the source.
	 */
	bool isInFrontCut(const node v) const
	{
		OGDF_ASSERT(m_calculateOtherCut || m_primaryCut);
		return m_nodeSet[v] == cutType::FRONT_CUT;
	}

	/**
	 * Returns whether this node is part of the back cut.
	 * Meaning it is located in the same set as the target.
	 *
	 * @param v The node in question.
	 */
	bool isInBackCut(const node v) const
	{
		OGDF_ASSERT(m_calculateOtherCut || !m_primaryCut);
		return m_nodeSet[v] == cutType::BACK_CUT;
	}

	/**
	 * Return whether this node is of the specified type.
	 *
	 * \param v
	 * 	the node to be tested
	 * \param type
	 * 	the cut type to test for (see MinSTCut::CutType)
	 *
	 * \return
	 * 	true if the node is contained in the specified cut
	 */
	bool isOfType(const node v, cutType type) const
	{
		return m_nodeSet[v] == type;
	}

private:
	//! the module used for calculating the maxflow
	std::unique_ptr<MaxFlowModule<TCost>> m_mfModule;
	//! states whether or not edges are considered undirected while calculating the maxflow
	bool m_treatAsUndirected = false;
	//! true if the algorithm should search for the mincut nearest to s, false if it should be near to t.
	bool m_primaryCut = true;
	/**
	 * iff true, the other (near to s for primaryCut == false, near to t for primaryCut == true)
	 * cut should also be calculated. Setting this to false will speed up the algorithm a bit, but all functions
	 * but `call` might not work correctly or at all.
	 */
	bool m_calculateOtherCut = true;


	std::unique_ptr<EpsilonTest> m_et; //!<- the module used for epsilon tests
	NodeArray<cutType> m_nodeSet; //!<- holds the partition type for each node
	EdgeArray<TCost> m_flow;
	EdgeArray<TCost> m_weight;
	int m_frontCutCount; //!<- the number of nodes in the front cut
	int m_backCutCount; //!<- the number of nodes in the back cut
	int m_totalCount; //!<- the total number of nodes in the graph

	/**
	 * Mark the all nodes which are in the same cut partition as the \p startNode.
	 *
	 * @param startNode Only works if this is either \a s or \a t.
	 * @param frontCut Should be set to true, iff \p startNode is s.
	 * @param origNode A function which maps the internal nodes to the nodes of the input graph.
	 */
	void markCut(node startNode, bool frontCut, std::function<node(node)> origNode) {
		List<node> queue;
		queue.pushBack(startNode);
		m_nodeSet[origNode(startNode)] = (frontCut ? cutType::FRONT_CUT : cutType::BACK_CUT);
		frontCut ? m_frontCutCount++ : m_backCutCount++;

		while (!queue.empty()) {
			const node v = queue.popFrontRet();
			for (adjEntry adj : v->adjEntries) {
				const node w = adj->twinNode();
				const edge e = adj->theEdge();
				if (m_nodeSet[origNode(w)] == cutType::NO_CUT
				    && (((frontCut ? e->source() :e->target()) == v &&
						m_et->less(m_flow[e], m_weight[e]))
					|| ((frontCut ? e->target() : e->source()) == v &&
						m_et->greater(m_flow[e], TCost(0))))) {
					queue.pushBack(w);
					m_nodeSet[origNode(w)] = (frontCut ? cutType::FRONT_CUT : cutType::BACK_CUT);
					frontCut ? m_frontCutCount++ : m_backCutCount++;
				}
			}
		}
	}

	/**
	 * Partitions the nodes to front and back cut.
	 *
	 * @param graph The input graph
	 * @param origEdge Maps internal edges to edges of the the input graph.
	 * @param origNode Maps internal nodes to nodes from the input graph.
	 * @param source The source node.
	 * @param target The target node.
	 * @param edgeList A list in which the edges of the cut are stored in.
	 */
	void computeCut(const Graph &graph, std::function<edge(edge)> origEdge, std::function<node(node)> origNode,
	                const node source, const node target, List<edge> &edgeList) {
		m_frontCutCount = 0;
		m_backCutCount = 0;
		m_totalCount = graph.numberOfNodes();


		List<node> queue;
		m_nodeSet.init(graph, cutType::NO_CUT);

		if(m_primaryCut || m_calculateOtherCut) {
			// front cut
			markCut(source, true, origNode);
		}

		if(!m_primaryCut || m_calculateOtherCut) {
			// back cut
			markCut(target, false, origNode);
		}

		ArrayBuffer<edge> stack;
		EdgeArray<bool> visited(graph, false);
		node startNode = (m_primaryCut ? source : target);
		adjEntry startAdj = startNode->firstAdj();
		if (startAdj == nullptr) {
			return;
		}
		if (startAdj->theEdge()->adjTarget() != startAdj) {
			stack.push(startAdj->theEdge());
		}
		for (adjEntry adj = (m_primaryCut ? startAdj->cyclicSucc() : startAdj->cyclicPred());
		     adj != startAdj;
		     adj = (m_primaryCut ? adj->cyclicSucc() : adj->cyclicPred()))
		{
			if (adj->theEdge()->adjTarget() != adj) {
				stack.push(adj->theEdge());
			}
		}
		while (!stack.empty()) {
			edge e = stack.popRet();
			if (visited[origEdge(e)]) {
				continue;
			}
			visited[origEdge(e)] = true;
			if (m_nodeSet[origNode(e->source())] == cutType::FRONT_CUT &&
			    m_nodeSet[origNode(e->target())] != cutType::FRONT_CUT) {
				edgeList.pushBack(origEdge(e));

				if(m_gc->numberOfEdges() != 0) {
					MinSTCutModule<TCost>::m_direction[origEdge(e)] = m_gc->copy(origEdge(e)) == e;
				}
			} else {
				startAdj = e->adjTarget();
				for (adjEntry adj = (m_primaryCut ? startAdj->cyclicSucc() : startAdj->cyclicPred());
				     adj != startAdj;
				     adj = (m_primaryCut ? adj->cyclicSucc() : adj->cyclicPred()))
				{
					if (adj->theEdge()->adjTarget() != adj) {
						stack.push(adj->theEdge());
					}
				}
			}
		}
	}
};

template<typename TCost>
bool MinSTCutMaxFlow<TCost>::call(const Graph &graph, const EdgeArray<TCost> &weight,
                                  node source, node target, List<edge> &edgeList, edge e_st) {
	MinSTCutModule<TCost>::m_direction.init(graph, -1);
	delete m_gc;
	m_gc = new GraphCopy(graph);

	if (e_st != nullptr) {
		m_gc->delEdge(m_gc->copy(e_st));
	}
	node s = m_gc->copy(source);
	node t = m_gc->copy(target);
	List<edge> edges;
	m_gc->allEdges(edges);
	EdgeArray<edge> originalEdge(*m_gc, nullptr);
	for (edge e : edges) {
		if (m_treatAsUndirected) {
			// a reversed edge is made and placed directly next to e
			edge revEdge = m_gc->newEdge(e->target(), e->source());
			m_gc->move(revEdge, e->adjTarget(), ogdf::Direction::before, e->adjSource(), ogdf::Direction::after);
			originalEdge[revEdge] = m_gc->original(e);
		}
		originalEdge[e] = m_gc->original(e);
	}

	m_flow.init(*m_gc, -1);
	m_weight.init(*m_gc, 1);
	for (edge e : m_gc->edges) {
		edge origEdge = originalEdge[e];
		m_weight[e] = weight[origEdge];
		OGDF_ASSERT(m_weight[e] >= 0);
	}

	m_mfModule->init(*m_gc);
	m_mfModule->computeFlow(m_weight, s, t, m_flow);

	computeCut(graph,
	           [&](edge e) -> edge {return originalEdge[e];},
	           [&](node v) -> node {return m_gc->original(v);},
	           s, t, edgeList);

	return true;
}

template<typename TCost>
void MinSTCutMaxFlow<TCost>::call(const Graph &graph,
                                  const EdgeArray<TCost> &weights,
                                  const EdgeArray<TCost> &flow,
                                  const node source, const node target) {
	delete m_gc;
	m_gc = new GraphCopy();
	m_flow = flow;
	m_weight = weights;
	List<edge> edgeList;
	computeCut(graph,
	           [&](edge e) -> edge {return e;},
	           [&](node v) -> node {return v;},
	           source, target, edgeList);
}
}
