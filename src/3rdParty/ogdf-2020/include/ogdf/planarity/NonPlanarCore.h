/** \file
 * \brief Declaration of class NonPlanarCore which represents the
 *        non-planar core reduction for biconnected graphs.
 *
 * \author Carsten Gutwenger, Mirko Wagner
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

#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/graphalg/MinSTCutBFS.h>
#include <ogdf/graphalg/MinSTCutDijkstra.h>
#include <ogdf/basic/simple_graph_alg.h>


namespace ogdf {

//! Non-planar core reduction.
/**
 * @ingroup ga-planarity
 *
 * The class ogdf::NonPlanarCore implements a reduction method that reduces a graph to a
 * smaller core graph which behaves invariant with respect to non-planarity measures
 * like crossing number, skewness, coarseness, and thickness. The core reduction is
 * based on the decomposition of a graph into its triconnected components and can
 * be computed in linear time.
 *
 * The implementation is based on the following publication:
 *
 * Markus Chimani, Carsten Gutwenger: <i>Non-planar core reduction of graphs</i>.
 * Discrete Mathematics 309(7) (2009) 1838-1855
 *
 * If the core reduction is done for a weighted graph, the running time is no longer linear, because
 * the mincut of st-components can't be calculated via BFS anymore, but either via Dijkstra (default) on the dual
 * graph (O(n log n)) or via any other minSTCut routine.
 * In tests we found out that Dijkstra outperforms all other minSTCut routines for all instances.
 *
 */
template<typename TCost = int>
class NonPlanarCore {

	template<typename T>
	friend class GlueMap;

public:
	/**
	 * Struct to represent an edge that needs to be crossed in order to cross an st-component.
	 *
	 */
	struct CutEdge {
		const edge e; //!< the edge
		const bool dir; //!< true, iff the edge is directed from the \a s partition to the \a t partion

		CutEdge(edge paramEdge, bool directed) : e(paramEdge), dir(directed) {};
	};

	/**
	 * \brief The unweighted version of the Algorithm call and constructor
	 *
	 * \details This constructor computes the non-planar core of the graph \p G.
	 *
	 * @pre \p G has to be biconnected.
	 *
	 * \param G the graph of which the NPC is to be made
	 * \param nonPlanarityGuaranteed iff set to true the algorithm runs a bit faster for nonplanar graphs
	 */
	explicit NonPlanarCore(const Graph &G, bool nonPlanarityGuaranteed = false);

	/**
	 * \brief An slimmed version of the Algorithm call and constructor
	 * @copydetails ogdf::NonPlanarCore::NonPlanarCore(const Graph&,bool)
	 * \param weight if the graph is weighted, the weights otherwise a nullptr
	 */
	NonPlanarCore(const Graph &G, const EdgeArray<TCost> &weight, bool nonPlanarityGuaranteed = false);

	/**
	 * \brief Algorithm call and constructor
	 * @copydetails ogdf::NonPlanarCore::NonPlanarCore(const Graph&,bool)
	 * \param weight if the graph is weighted, the weights otherwise a nullptr
	 * \param minSTCutModule the MaxFlowModule that should be used for calculating the traversing path
	 * for weighted graphs.
	 */
	NonPlanarCore(const Graph &G, const EdgeArray<TCost> &weight, MinSTCutModule<TCost> *minSTCutModule,
	              bool nonPlanarityGuaranteed = false);

	//! Returns the non-planar core
	const Graph &core() const {
		return m_graph;
	}

	//! Returns the original graph
	const Graph &originalGraph() const {
		return *m_pOriginal;
	}

	//! Returns the node of the original graph, which is represented by \p v in the core
	node original(node v) const {
		return m_orig[v];
	}

	//! Returns the edges of the original graph, which are represented by \p e in the core
	List<edge> original(edge e) const {
		List<edge> res;
		if (isVirtual(e)) {
			EdgeArray<edge> origEdgesOfThisSTComponent(*mapE(e));
			for (edge eInCop: origEdgesOfThisSTComponent.graphOf()->edges) {
				if (origEdgesOfThisSTComponent[eInCop] != nullptr) {
					res.pushBack(origEdgesOfThisSTComponent[eInCop]);
				}
			}
		} else {
			res.pushBack(realEdge(e));
		}
		return res;
	}

	//! True iff the edge \p e in the core represents more than one orginal edge and therefore is virtual
	bool isVirtual(edge e) const {
		return m_real[e] == nullptr;
	}

	//! Returns the edge of the orginal graph, which is represented by \p e or nullptr iff \p e is virtual
	edge realEdge(edge e) const {
		return m_real[e];
	}

	/**
	 * Returns the costs of the edges in the core, which is the number of original edges crossed,
	 * if \a e is crossed, i.e. one if an edge is real and `|`mincut(edge)`|` if an edge is virtual
	 */
	const EdgeArray<TCost> &cost() const {
		return m_cost;
	}

	/**
	 * Returns the t node of the skeleton of the st-component represented by the core edge \p e = (s,t)
	 * Note that this node is not contained in the input graph, but an internal auxiliary graph.
	 */
	node tNode(edge e) const {
		return m_tNode[e];
	}

	/**
	 * Returns the s node of the skeleton of the st-component represented by the core edge \p e = (s,t)
	 * Note that this node is not contained in the input graph, but an internal auxiliary graph.
	 */
	node sNode(edge e) const {
		return m_sNode[e];
	}

	/**
	 * Returns a map from the edges of the st-component represented by the core edge e to the original graph
	 */
	EdgeArray<edge>* mapE(edge e) const {
		return m_mapE[e];
	}

	/**
	 * Returns the cost of \p e, which is the number of original edges crossed, if \p e is crossed,
	 * i.e. 1 if \p e is real and `|`mincut(e)`|` if e is virtual
	 */
	TCost cost(edge e) const {
		return m_cost[e];
	}

	//! Returns the mincut of the st-component represented by \p e
	const List<CutEdge> &mincut(edge e) const {
		return m_mincut[e];
	}

	// destructor
	virtual ~NonPlanarCore();

	//! Inserts the crossings from a copy of the core into a copy of the original graph.
	/**
	 * This method expects \p planarCore to be planarly embedded without pseudo-crossings.
	 * \param planarCore a GraphCopy of the core, in which dummy nodes are inserted to represent crossings
	 * \param planarGraph a GraphCopy which is replaced by a GraphCopy of the original graph
	 * \param pCisPlanar Set this to true if a non-planar embedding of \p planarCore is given.
	 */
	void retransform(const GraphCopy &planarCore, GraphCopy &planarGraph, bool pCisPlanar = true);

protected:
	//! The private method behind the constructors.
	void call(const Graph &G, const EdgeArray<TCost> *weight, MinSTCutModule<TCost> *minSTCutModule,
              bool nonPlanarityGuaranteed);

	/**
	 * Checks for multiedges in the core.
	 * This method is a slightly modified version of ogdf::IsParallelFreeUndirected(),
	 * that adds the functionality that the found multiedges are stored in lists.
	 *
	 * \param winningEdges The list of edges that will survive the glue.
	 * \param losingEdges The list of edges that won't survive the glue.
	 */
	void getAllMultiedges(List<edge> &winningEdges, List<edge> &losingEdges);

	/**
	 * Computes the traversing path for a given edge and the unmarked tree rooted in the node of \p eS
	 * and saves the combinatorial embedding of the st-component which \p eS represents,
	 * i.e. a list of edges that are to be crossed, when the given edge is crossed in the core.
	 * This list is minimal.
	 *
	 * \param Sv the Skeleton of one of the marked nodes of #m_T.
	 * \param eS an edge in \p Sv.
	 * \param path a container to write the traversing path to
	 * `true` iff the source of the edge is on the s side of the cut.
	 * \param mapV a NodeArray of the original graph to map original nodes to nodes created in this method.
	 * \param coreEdge the edge in the core that represents the st-component of which the
	 * traversing path is computed.
	 * \param weight_src the weight of the edges of the original graph
	 * \param minSTCutModule same as in the constructor
	 */
	void traversingPath(const Skeleton &Sv, edge eS, List<CutEdge> &path, NodeArray<node> &mapV,
	                    edge coreEdge, const EdgeArray<TCost> *weight_src, MinSTCutModule<TCost> *minSTCutModule);

	/**
	 * To be able to insert crossings correctly, an end graph edge ought to be split into n-1 sections
	 * if n is the number of crossings on the edge.
	 * This method does exactly that.
	 *
	 * \param e The edge to be split
	 * \param splitdummies To delete the inserted dummynodes later, we store all of them in here
	 */
	void splitEdgeIntoSections(edge e, List<node> &splitdummies);

	/**
	 * After inserting the crossings, the end graph edges don't need to be partitioned anymore
	 * so the \p splitdummies get removed.
	 */
	void removeSplitdummies(List<node> &splitdummies);

	/**
	 * Every edge of \p coreEdge's cut that doesn't go the same direction as \p coreEdge gets reversed.
	 * This method is used to both normalize the cutedges and denormalize them
	 * after the retransformation.
	 *
	 * \param coreEdge the core edge
	 */
	void normalizeCutEdgeDirection(edge coreEdge);

	/**
	 * This method asserts that all parts of the end graph that are represented by edge \p e
	 * internally have the same embedding every time retransform is called,
	 * regardless of which planarization of the core is given.
	 * Only nodes that are present in the core can have a different embedding
	 * for a different planarization of the core. They are infact reassembling the planarization of the core.
	 */
	void importEmbedding(edge e);

	/**
	 * Marks all nodes of the underlying SPQRTree and prunes planar leaves until the marked nodes span a tree,
	 * which has only non-planar leaves, i.e. non-planar R-nodes.
	 *
	 * \param mark The array where the marking is done
	 */
	void markCore(NodeArray<bool> &mark);

	/**
	 * The crossing denoted by dummy node \p v from the planarized copy of the core
	 * get inserted into the end graph.
	 */
	void inflateCrossing(node v);

	/**
	 * Get the mincut of \p e with respect to its position in the chain of its original edge.
	 *
	 * \param e The edge that we want to know the position of in the
	 * graphcopy representing the planarized version of the original graph.
	 * \param cut A list to write the mincut to.
	 */
	void getMincut(edge e, List<edge> &cut);

	/**
	 * Glues together the skeletons of \p eWinner and \p eLoser for pruned P- and S-nodes.
	 * This is done, by inserting all nodes and edges of \p eLoser's skeleton into \p eWinner's skeleton,
	 * while preserving the embedding of both skeletons.
	 *
	 * \param eWinner the core edge that will represent the newly formed skeleton/embedding
	 * \param eLoser the core edge that is copied from
	 */
	void glue(edge eWinner, edge eLoser);

	/**
	 * Glues together the mincuts of the winner and the loser edge.
	 *
	 * \param eWinner the edge whose mincut gets augmented
	 * \param eLoser the edge whose mincut gets glued to the winner mincut
	 */
	void glueMincuts(edge eWinner, edge eLoser);

	//! The core
	Graph m_graph;

	//! The original graph.
	const Graph *m_pOriginal;

	/**
	 * A pointer to a copy of the core, in which crossings are replaced by dummy nodes.
	 * It's a `nullptr` unless NonPlanarCore::retransform was called.
	 */
	GraphCopy const *m_planarCore;

	/**
	 * A pointer to a copy of the original graph, in which crossings are replaced by dummy nodes.
	 * It's a `nullptr` unless NonPlanarCore::retransform was called.
	 */
	GraphCopy *m_endGraph;

	//! Corresp. original node
	NodeArray<node> m_orig;

	//! Corresp. original edge (0 if virtual)
	EdgeArray<edge> m_real;

	//! Traversing path for an edge in the core
	EdgeArray<List<CutEdge>> m_mincut;

	//! Costs to cross each edge of the core
	EdgeArray<TCost> m_cost;

	//! The SPQRTree that represents the original graph.
	StaticSPQRTree m_T;

	//! The mapping between the nodes of each embedding and their original
	EdgeArray<NodeArray<node> *> m_mapV;

	//! The mapping between the edges of each embedding and their original
	EdgeArray<EdgeArray<edge> *> m_mapE;

	//! The graph for the underlying skeleton of a virtual edge in the core
	EdgeArray<Graph *> m_underlyingGraphs;

	//! The s node of the st-component of a core edge
	EdgeArray<node> m_sNode;

	//! The t node of the st-component of a core edge
	EdgeArray<node> m_tNode;
};

/**
 * This is a helper class to make the glueing of two edges simpler.
 */
template<typename Cost>
class GlueMap {
public:
	/**
	 * A GlueMap is created from an NonPlanarCore and two core edges that ought to be glued together.
	 * It holds many mappings, mostly to the original graph of the core.
	 *
	 * \param eWinner This edge gets extended.
	 * \param eLoser This edge gets deleted in the end and everything it represents is transferred
	 * to \p eWinner.
	 * \param npc The NonPlanarCore all of this exists in.
	 */
	GlueMap(edge eWinner, edge eLoser, NonPlanarCore<Cost> &npc);

	/**
	 * A mapping from the \p eInLoser graph to a new edge in the winner graph is created.
	 */
	void mapLoserToNewWinnerEdge(edge eInLoser);

	/**
	 * A mapping from the \p vInLoser to the \p vInWinner is created.
	 */
	void mapLoserToWinnerNode(node vInLoser, node vInWinner);

	/**
	 * A mapping from the \p vInLoser to a new node in the winner graph is created.
	 */
	void mapLoserToNewWinnerNode(node vInLoser);

	/**
	 * This method reorders the adjacency order of \p vLoser's counterpart in the winner graph
	 * according to the AdjOrder of \p vLoser in the loser graph.
	 *
	 * \param vLoser the node in question
	 * \param sameDirection false iff this method is called while handling a P Node,
	 * for which the edges are not in the same direction.
	 * \param isTNodeOfPNode true iff \p vLoser is the target node of the loser graph and the glueing
	 * process is done on a P Node.
	 */
	void reorder(node vLoser, bool sameDirection, bool isTNodeOfPNode);

	/**
	 * Getter for #m_mapV_l2w
	 * @param v the loser node
	 * @return the winner node
	 */
	node getWinnerNodeOfLoserNode(node v) const {
		return m_mapV_l2w[v];
	}

	/**
	 * Getter for #m_gLoser
	 * @return the graph that loses this glueing
	 */
	const Graph &getLoserGraph() const {
		return *m_gLoser;
	}

protected:
	//! The NonPlanarCore on which this instance operates
	NonPlanarCore<Cost> &m_npc;
	//! The core edge that will survive
	const edge m_eWinner;
	//! The core edge that will be deleted
	const edge m_eLoser;
	//! The graph that eWinner represents.
	Graph *m_gWinner;
	//! The graph that eLoser represents.
	const Graph *m_gLoser;
	//! A map from the edges of the winner graph to the original graph, to denote the original of each edge.
	EdgeArray<edge> *m_mapEwinner;
	//! A map from the edges of the loser graph to the original graph, to denote the original of each node.
	const EdgeArray<edge> *m_mapEloser;
	//! A map from the nodes of the winner graph to the original graph, to denote the original of each edge.
	NodeArray<node> *m_mapVwinner;
	//! A map from the nodes of the loser graph to the original graph, to denote the original of each node.
	const NodeArray<node> *m_mapVloser;
	//! A map from the nodes of the loser graph to their new home in the winner graph.
	NodeArray<node> m_mapV_l2w;
	//! A map from the edges of the loser graph to their new home in the winner graph.
	EdgeArray<edge> m_mapE_l2w;
};

	template<typename Cost>
	NonPlanarCore<Cost>::NonPlanarCore(const Graph &G, bool nonPlanarityGuaranteed) :
			m_pOriginal(&G), m_orig(m_graph), m_real(m_graph, nullptr), m_mincut(m_graph),
			m_cost(m_graph), m_T(G), m_mapV(m_graph, nullptr), m_mapE(m_graph, nullptr), m_underlyingGraphs(m_graph, nullptr),
			m_sNode(m_graph), m_tNode(m_graph) {
		MinSTCutBFS<Cost> minSTCutBFS;
		call(G, nullptr, &minSTCutBFS, nonPlanarityGuaranteed);
	}

	template<typename Cost>
	NonPlanarCore<Cost>::NonPlanarCore(const Graph &G, const EdgeArray<Cost> &weight, MinSTCutModule<Cost> *minSTCutModule,
									bool nonPlanarityGuaranteed) :
			m_pOriginal(&G), m_orig(m_graph), m_real(m_graph, nullptr), m_mincut(m_graph),
			m_cost(m_graph), m_T(G), m_mapV(m_graph, nullptr), m_mapE(m_graph, nullptr), m_underlyingGraphs(m_graph, nullptr),
			m_sNode(m_graph), m_tNode(m_graph) {
		call(G, &weight, minSTCutModule, nonPlanarityGuaranteed);
	}

	template<typename Cost>
	NonPlanarCore<Cost>::NonPlanarCore(const Graph &G, const EdgeArray<Cost> &weight, bool nonPlanarityGuaranteed) :
			m_pOriginal(&G), m_orig(m_graph), m_real(m_graph, nullptr), m_mincut(m_graph),
			m_cost(m_graph), m_T(G), m_mapV(m_graph, nullptr), m_mapE(m_graph, nullptr), m_underlyingGraphs(m_graph, nullptr),
			m_sNode(m_graph), m_tNode(m_graph) {
		MinSTCutDijkstra<Cost> minSTCutDijkstra;
		call(G, &weight, &minSTCutDijkstra, nonPlanarityGuaranteed);
	}


	template<typename Cost>
	NonPlanarCore<Cost>::~NonPlanarCore() {
		for (auto pointer : m_mapE) {
			delete pointer;
		}
		for (auto pointer : m_mapV) {
			delete pointer;
		}
		for (auto pointer : m_underlyingGraphs) {
			delete pointer;
		}
	}

	template<typename Cost>
	void NonPlanarCore<Cost>::call(const Graph &G, const EdgeArray<Cost> *weight, MinSTCutModule<Cost> *minSTCutModule,
								bool nonPlanarityGuaranteed) {
		if (!nonPlanarityGuaranteed && isPlanar(G)) {
			return;
		}
		OGDF_ASSERT(!isPlanar(G));
		OGDF_ASSERT(isBiconnected(G));

		// mark tree nodes in the core
		NodeArray<bool> mark;
		markCore(mark);

		NodeArray<node> map(G, nullptr);
		NodeArray<node> mapAux(G, nullptr);
		const Graph &tree = m_T.tree();

		for (node v : tree.nodes) {
			if (!mark[v]) {
				continue;
			}

			Skeleton &S = m_T.skeleton(v);

			for (edge e : S.getGraph().edges) {
				node src = S.original(e->source());
				node tgt = S.original(e->target());

				if (tgt == src) {
					continue;
				}

				if (map[src] == nullptr) {
					m_orig[map[src] = m_graph.newNode()] = S.original(e->source());
				}

				if (map[tgt] == nullptr) {
					m_orig[map[tgt] = m_graph.newNode()] = S.original(e->target());
				}

				if (S.isVirtual(e)) {
					node w = S.twinTreeNode(e);

					if (!mark[w]) {
						// new virtual edge in core graph
						edge lastCreatedEdge = m_graph.newEdge(map[src], map[tgt]);
						m_real[lastCreatedEdge] = nullptr;
						traversingPath(S, e, m_mincut[lastCreatedEdge], mapAux,
						               lastCreatedEdge, weight, minSTCutModule);
					}
				} else {
					// new real edge in core graph
					edge lastCreatedEdge = m_graph.newEdge(map[src], map[tgt]);
					m_real[lastCreatedEdge] = S.realEdge(e);
					traversingPath(S, e, m_mincut[lastCreatedEdge], mapAux,
					               lastCreatedEdge, weight, minSTCutModule);
				}
			}
		}

		if (weight != nullptr) {
			for (edge e : m_graph.edges) {
				Cost cost(0);
				for (auto cutEdge : m_mincut[e]) {
					cost += (*weight)[cutEdge.e];
				}
				m_cost[e] = cost;
			}
		} else {
			for (edge e : m_graph.edges) {
				m_cost[e] = m_mincut[e].size();
			}
		}

		List<node> allNodes;
		m_graph.allNodes(allNodes);

		// The while loop is used to eliminate multiedges from the core. We're pruning P-Nodes.
		List<edge> winningMultiEdges;
		List<edge> losingMultiEdges;
		getAllMultiedges(winningMultiEdges, losingMultiEdges);
		while (!winningMultiEdges.empty() && !losingMultiEdges.empty()) {
			edge winningMultiEdge = winningMultiEdges.popFrontRet();
			edge losingMultiEdge = losingMultiEdges.popFrontRet();
#ifdef OGDF_DEBUG
			int sizeOfWinCut = m_mincut[winningMultiEdge].size();
			int sizeOfLosCut = m_mincut[losingMultiEdge].size();
#endif

			glue(winningMultiEdge, losingMultiEdge);
			glueMincuts(winningMultiEdge, losingMultiEdge);

			OGDF_ASSERT(m_mincut[winningMultiEdge].size() == sizeOfWinCut + sizeOfLosCut);
			delete m_underlyingGraphs[losingMultiEdge];
			delete m_mapV[losingMultiEdge];
			delete m_mapE[losingMultiEdge];
			m_real[winningMultiEdge] = nullptr;
			m_real[losingMultiEdge] = nullptr;
			m_graph.delEdge(losingMultiEdge);
		}
		// The for loop is used to eliminate deg 2 nodes from the core. We're pruning S-Nodes.
		for (node v : allNodes) {
			if (v->degree() != 2) {
				continue;
			}
			edge outEdge = v->firstAdj()->theEdge();
			edge inEdge = v->lastAdj()->theEdge();

			if (m_cost[inEdge] > m_cost[outEdge]) {
				std::swap(inEdge, outEdge);
			}
			// We join the embeddings of the underlying embeddings/graphs of both edges
			// so that outEdge gets integrated into inEdge
			glue(inEdge, outEdge);

			m_real[inEdge] = nullptr;
			m_real[outEdge] = nullptr;

			adjEntry adjSource = inEdge->adjSource()->cyclicSucc();
			adjEntry adjTarget = (outEdge->target() == v ? outEdge->adjSource()->cyclicSucc()
			                                             : outEdge->adjTarget()->cyclicSucc());
			if (inEdge->target() != v) {
				adjSource = adjTarget;
				adjTarget = inEdge->adjTarget()->cyclicSucc();
			}
			m_graph.move(inEdge, adjSource, ogdf::Direction::before, adjTarget, ogdf::Direction::before);
			delete m_underlyingGraphs[outEdge];
			delete m_mapV[outEdge];
			delete m_mapE[outEdge];
			m_graph.delNode(v);
		}


		if (nonPlanarityGuaranteed) {
			OGDF_ASSERT(!isPlanar(m_graph));
		}
	}

	template<typename Cost>
	void NonPlanarCore<Cost>::markCore(NodeArray<bool> &mark) {
		const Graph &tree = m_T.tree();

		// We mark every tree node that belongs to the core
		mark.init(tree, true); // start with all nodes and unmark planar leaves
		NodeArray<int> degree(tree);

		Queue<node> Q;

		for (node v : tree.nodes) {
			degree[v] = v->degree();
			if (degree[v] <= 1) { // also append deg-0 node (T has only one node)
				Q.append(v);
			}
		}

		while (!Q.empty()) {
			node v = Q.pop();

			// if v has a planar skeleton
			if (m_T.typeOf(v) != SPQRTree::NodeType::RNode || isPlanar(m_T.skeleton(v).getGraph())) {
				mark[v] = false; // unmark this leaf

				node w = nullptr;
				for (adjEntry adj : v->adjEntries) {
					node x = adj->twinNode();
					if (mark[x]) {
						w = x;
						break;
					}
				}

				if (w != nullptr) {
					--degree[w];
					if (degree[w] == 1) {
						Q.append(w);
					}
				}
			}
		}
	}

	struct QueueEntry {
		QueueEntry(node p, node v) : m_parent(p), m_current(v) { }

		node m_parent;
		node m_current;
	};

	template<typename Cost>
	void NonPlanarCore<Cost>::traversingPath(const Skeleton &Sv, edge eS, List<CutEdge> &path,
	                                         NodeArray<node> &mapV, edge coreEdge,
	                                         const EdgeArray<Cost> *weight_src,
	                                         MinSTCutModule<Cost> *minSTCutModule)
	{
		List<CutEdge> &currPath = path;

		// Build the graph representing the planar st-component
		Graph *h_pointer = new Graph();
		Graph &H = *h_pointer;

		EdgeArray<edge> *mapE_src_pointer = new EdgeArray<edge>(H, nullptr);
		EdgeArray<edge> &mapE_src = *mapE_src_pointer;
		NodeArray<node> *mapV_src_pointer = new NodeArray<node>(H, nullptr);
		NodeArray<node> &mapV_src = *mapV_src_pointer;
		SListPure<node> nodes;
		SListPure<edge> multedges;

		if (Sv.isVirtual(eS)) {
			Queue<QueueEntry> Q;
			Q.append(QueueEntry(Sv.treeNode(), Sv.twinTreeNode(eS)));

			while (!Q.empty()) {
				QueueEntry x = Q.pop();
				node parent = x.m_parent;
				node current = x.m_current;

				const Skeleton &S = m_T.skeleton(current);
				for (edge e : S.getGraph().edges) {
					if (S.isVirtual(e)) {
						continue;
					}

					node src = S.original(e->source());
					node tgt = S.original(e->target());

					if (mapV[src] == nullptr) {
						nodes.pushBack(src);
						mapV[src] = H.newNode();
						mapV_src[mapV[src]] = src;
					}
					if (mapV[tgt] == nullptr) {
						nodes.pushBack(tgt);
						mapV[tgt] = H.newNode();
						mapV_src[mapV[tgt]] = tgt;
					}

					edge e_new = H.newEdge(mapV[src], mapV[tgt]);
					mapE_src[e_new] = S.realEdge(e);
					OGDF_ASSERT(mapE_src[e_new]->source() != nullptr);
				}

				for (adjEntry adj : current->adjEntries) {
					node w = adj->twinNode();
					if (w != parent) {
						Q.append(QueueEntry(current, w));
					}
				}
			}
		} else {
			nodes.pushBack(Sv.original(eS->source()));
			nodes.pushBack(Sv.original(eS->target()));
			mapV[Sv.original(eS->source())] = H.newNode();
			mapV_src[mapV[Sv.original(eS->source())]] = Sv.original(eS->source());
			mapV[Sv.original(eS->target())] = H.newNode();
			mapV_src[mapV[Sv.original(eS->target())]] = Sv.original(eS->target());
			mapE_src[H.newEdge(mapV[Sv.original(eS->source())], mapV[Sv.original(eS->target())])] = Sv.realEdge(eS);
		}
		// add st-edge
		edge e_st = H.newEdge(mapV[Sv.original(eS->source())], mapV[Sv.original(eS->target())]);
		m_sNode[coreEdge] = mapV[Sv.original(eS->source())];
		m_tNode[coreEdge] = mapV[Sv.original(eS->target())];

		// Compute planar embedding of H
#ifdef OGDF_DEBUG
		bool ok =
#endif
		planarEmbed(H);
		OGDF_ASSERT(ok);
		CombinatorialEmbedding E(H);

		// we rearange the adj Lists of s and t, so that adj(e_st) is the first adj
		List<adjEntry> adjListFront;
		List<adjEntry> adjListBack;
		e_st->source()->allAdjEntries(adjListFront);
		if (adjListFront.front() != e_st->adjSource()) {
			adjListFront.split(adjListFront.search(e_st->adjSource()), adjListFront, adjListBack);
			adjListFront.concFront(adjListBack);
			H.sort(e_st->source(), adjListFront);
		}

		e_st->target()->allAdjEntries(adjListFront);
		if (adjListFront.front() != e_st->adjTarget()) {
			adjListFront.split(adjListFront.search(e_st->adjTarget()), adjListFront, adjListBack, ogdf::Direction::before);
			adjListFront.concFront(adjListBack);
			H.sort(e_st->target(), adjListFront);
		}

		if (Sv.isVirtual(eS)) {
			List<edge> edgeList;
			if (weight_src != nullptr) {
				EdgeArray<Cost> weight(H);
				for (edge e : H.edges) {
					if (e != e_st) {
						weight[e] = (*weight_src)[mapE_src[e]];
					}
				}
				minSTCutModule->call(H, weight, e_st->source(), e_st->target(), edgeList, e_st);
			} else {
				minSTCutModule->call(H, e_st->source(), e_st->target(), edgeList, e_st);
			}
			auto it = edgeList.begin();
			for (;it != edgeList.end(); it++) {
				currPath.pushBack(CutEdge(mapE_src[*it], minSTCutModule->direction(*it)));
			}
		}
		else {
			OGDF_ASSERT(Sv.realEdge(eS) != nullptr);
			currPath.pushFront(CutEdge(Sv.realEdge(eS), true));
		}
		H.delEdge(e_st);

		OGDF_ASSERT(m_underlyingGraphs[coreEdge] == nullptr);
		m_underlyingGraphs[coreEdge] = h_pointer;
		OGDF_ASSERT(m_mapE[coreEdge] == nullptr);
		m_mapE[coreEdge] = mapE_src_pointer;
		OGDF_ASSERT(m_mapV[coreEdge] == nullptr);
		m_mapV[coreEdge] = mapV_src_pointer;
#ifdef OGDF_DEBUG
		for (node v : H.nodes) {
			OGDF_ASSERT(mapV_src[v] != nullptr);
		}
		for (edge e : H.edges) {
			OGDF_ASSERT(mapE_src[e] != nullptr); }
#endif

		for (node v : nodes) {
			mapV[v] = nullptr;
		}
	}

	template<typename Cost>
	void NonPlanarCore<Cost>::getAllMultiedges(List<edge> &winningEdges, List<edge> &losingEdges) {
		winningEdges.clear();
		losingEdges.clear();
		SListPure<edge> edges;
		EdgeArray<int> minIndex(m_graph), maxIndex(m_graph);
		parallelFreeSortUndirected(m_graph, edges, minIndex, maxIndex);

		SListConstIterator<edge> it = edges.begin();
		edge ePrev = *it, e;
		for (it = ++it; it.valid(); ++it, ePrev = e) {
			e = *it;
			if (minIndex[ePrev] == minIndex[e] && maxIndex[ePrev] == maxIndex[e]) {
				winningEdges.pushFront(ePrev);
				losingEdges.pushFront(e);
			}
		}
	}

	template<typename Cost>
	void NonPlanarCore<Cost>::glue(edge eWinner, edge eLoser) {
		GlueMap<Cost> map(eWinner, eLoser, *this);

		// true iff this glueing is about a PNode (so a glueing at two common nodes)
		bool thisIsAboutAPNode = false;
		if (eLoser->isParallelUndirected(eWinner)) {
			thisIsAboutAPNode = true;
		}

		// find the s- and t-nodes in their skeleton for both of the edges
		node sWinner = m_sNode[eWinner];
		node tWinner = m_tNode[eWinner];
		node sLoser = m_sNode[eLoser];
		node tLoser = m_tNode[eLoser];

		bool sameDirection{!eWinner->isInvertedDirected(eLoser)};

		// we get a list of all nodes of the loser graph
		List<node> allNodesButSt;
		map.getLoserGraph().allNodes(allNodesButSt);

#ifdef OGDF_DEBUG
		bool ok = true;
#endif

		// for both s and t of eLoser we check if it's either the s or the t node of eWinner
		// if one of it is, we delete it from the list of nodes, that are to be added
		// otherwise it stays in 'allNodesButSt' to be added later
		if (eLoser->source() == eWinner->source() || eLoser->source() == eWinner->target()) {
#ifdef OGDF_DEBUG
			ok =
#endif
			allNodesButSt.removeFirst(sLoser);
			OGDF_ASSERT(ok);
			if (eLoser->source() == eWinner->source()) {
				map.mapLoserToWinnerNode(sLoser, sWinner);
			} else {
				map.mapLoserToWinnerNode(sLoser, tWinner);
			}
			OGDF_ASSERT(!allNodesButSt.search(sLoser).valid());
		}
		if (eLoser->target() == eWinner->source() || eLoser->target() == eWinner->target()) {
#ifdef OGDF_DEBUG
			ok =
#endif
			allNodesButSt.removeFirst(tLoser);
			OGDF_ASSERT(ok);
			if (eLoser->target() == eWinner->source()) {
				map.mapLoserToWinnerNode(tLoser, sWinner);
			} else {
				map.mapLoserToWinnerNode(tLoser, tWinner);
			}
			OGDF_ASSERT(!allNodesButSt.search(tLoser).valid());
		}


		// insert the remaining nodes of the loser graph into the winner graph
		for (node v : allNodesButSt) {
			map.mapLoserToNewWinnerNode(v);
		}

		// insert all edges of the loser graph into the the winner graph
		for (edge e : map.getLoserGraph().edges) {
			map.mapLoserToNewWinnerEdge(e);
		}

		// reorder the adjEntries of every node of the loser graph in the winner graph,
		// to match the embedding in the loser graph
		List<node> allNodes = allNodesButSt;
		allNodes.pushBack(sLoser);
		allNodes.pushBack(tLoser);
		for (node v : allNodes) {
			map.reorder(v, sameDirection, (tLoser == v && thisIsAboutAPNode));
		}
		if (!thisIsAboutAPNode) {
			if (eWinner->source() == eLoser->source()) {
				m_sNode[eWinner] = map.getWinnerNodeOfLoserNode(tLoser);
			}
			if (eWinner->target() == eLoser->source()) {
				m_tNode[eWinner] = map.getWinnerNodeOfLoserNode(tLoser);
			}
			if (eWinner->source() == eLoser->target()) {
				m_sNode[eWinner] = map.getWinnerNodeOfLoserNode(sLoser);
			}
			if (eWinner->target() == eLoser->target()) {
				m_tNode[eWinner] = map.getWinnerNodeOfLoserNode(sLoser);
			}
		}
	}

	template<typename Cost>
	void NonPlanarCore<Cost>::retransform(const GraphCopy &planarCore, GraphCopy &planarGraph, bool pCisPlanar) {
#ifdef OGDF_DEBUG
		GraphCopy copyCore(planarCore);
		copyCore.removePseudoCrossings();
		OGDF_ASSERT(copyCore.numberOfNodes() == planarCore.numberOfNodes());
#endif
		m_endGraph = &planarGraph;
		m_planarCore = &planarCore;
		OGDF_ASSERT(!pCisPlanar || m_planarCore->genus() == 0);
		m_endGraph->clear();
		m_endGraph->createEmpty(*m_pOriginal);
		List<node> allNodes;
		m_pOriginal->allNodes(allNodes);
		EdgeArray<edge> eCopy(*m_pOriginal, nullptr);
		m_endGraph->initByNodes(allNodes, eCopy);

#ifdef OGDF_DEBUG
		for (node v : m_endGraph->nodes) {
			List<adjEntry> adjEntries;
			v->allAdjEntries(adjEntries);
			OGDF_ASSERT(v->degree() == adjEntries.size());
		}
#endif

		// For every node of the core we rearrange the adjacency order of the corresponding endGraph node
		// according to the planarized core.
		for (node v : m_planarCore->nodes) {
			if (m_planarCore->isDummy(v)) {
				continue;
			}
			List<adjEntry> pcOrder;
			v->allAdjEntries(pcOrder);
			List<adjEntry> newOrder;
			node coreNode = m_planarCore->original(v);
			OGDF_ASSERT(coreNode != nullptr);
			for (adjEntry adjPC : v->adjEntries) {
				edge coreEdge = m_planarCore->original(adjPC->theEdge());
				EdgeArray<edge> &mapE = *m_mapE[coreEdge];
				NodeArray<node> &mapV = *m_mapV[coreEdge];
				node stNode = (mapV[m_sNode[coreEdge]] == original(coreNode) ? m_sNode[coreEdge] : m_tNode[coreEdge]);
				// find the node of emb which represents the same node v represents
				for (adjEntry adjEmb : stNode->adjEntries) {
					if (adjEmb->theEdge()->source() == adjEmb->theNode()) {
						newOrder.pushBack(m_endGraph->copy(mapE[adjEmb->theEdge()])->adjSource());
					} else {
						newOrder.pushBack(m_endGraph->copy(mapE[adjEmb->theEdge()])->adjTarget());
					}
				}
			}
			m_endGraph->sort(m_endGraph->copy(original(coreNode)), newOrder);
		}
		if (!pCisPlanar) {
			for (edge e: m_graph.edges) {
				importEmbedding(e);
			}
			return;
		} else {
			List<node> splitdummies;
			for (edge e : m_graph.edges) {
				// for every edge from the core we ensure, that the embedding of the subgraph it describes
				// is the same in both the original and the end graph
				importEmbedding(e);
				// reverse all cut edges, which are not the same direction as e
				normalizeCutEdgeDirection(e);
				// to ensure the right order of the inserted crossings, we insert dummy nodes
				// to split the edge in sections, each of which only has one crossing
				splitEdgeIntoSections(e, splitdummies);
			}

			// now we can start and insert the crossings of the planar core into the end graph.
			// a node represents a crossing if it's a dummy
			for (node v : m_planarCore->nodes) {
				if (m_planarCore->isDummy(v)) {
					inflateCrossing(v);
				}
			}
			OGDF_ASSERT(m_endGraph->genus() == 0);

			removeSplitdummies(splitdummies);
			for (edge e : m_graph.edges) {
				normalizeCutEdgeDirection(e);
			}
		}
	}

	template<typename Cost>
	void NonPlanarCore<Cost>::normalizeCutEdgeDirection(edge coreEdge) {
		for (auto cutE : m_mincut[coreEdge]) {
			if (!cutE.dir) {
				for (edge e : m_endGraph->chain(cutE.e)) {
					m_endGraph->reverseEdge(e);
				}
			}
		}
	}

	template<typename Cost>
	void NonPlanarCore<Cost>::removeSplitdummies(List<node> &splitdummies) {
		for (node v : splitdummies) {
			edge eIn = v->firstAdj()->theEdge();
			edge eOut = v->lastAdj()->theEdge();
			if (eIn->target() == v) {
				m_endGraph->unsplit(eIn, eOut);
			} else {
				m_endGraph->unsplit(eOut, eIn);
			}
		}
	}

	template<typename Cost>
	void NonPlanarCore<Cost>::splitEdgeIntoSections(edge e, List<node> &splitdummies) {
		List<edge> chain = m_planarCore->chain(e);
		int chainSize = chain.size();
		while (chainSize > 2) {
			for (CutEdge cutEdge : mincut(e)) {
				splitdummies.pushBack(m_endGraph->split(m_endGraph->copy(cutEdge.e))->source());
			}
			chainSize--;
		}
#ifdef OGDF_DEBUG
		for (CutEdge cutEdge : mincut(e)) {
			if (chain.size() < 3) {
				OGDF_ASSERT(m_endGraph->chain(cutEdge.e).size() == 1);
			} else {
				OGDF_ASSERT(m_endGraph->chain(cutEdge.e).size() == chain.size() - 1);
			}
			OGDF_ASSERT(m_endGraph->original(m_endGraph->chain(cutEdge.e).front()) == cutEdge.e);
		}
#endif
	}

	template<typename Cost>
	void NonPlanarCore<Cost>::importEmbedding(edge e) {
		const Graph &embG = *m_underlyingGraphs[e];
		// a map from the nodes of the emb to those in the end graph
		const EdgeArray<edge> &mapE_toOrig = *m_mapE[e];
		// bc the edges of the end graph are split for the crossing insertion,
		// a map of the emb might have more than one edge in the endgraph, we just
		// map the AdjEntries of both source and target of each edge of emb
		// to AdjEntries in the end graph
		const NodeArray<node> &mapV_toOrig = *m_mapV[e];
		AdjEntryArray<adjEntry> mapA_toFinal(embG, nullptr);
		for (auto it = mapE_toOrig.begin(); it != mapE_toOrig.end(); it++) {
			OGDF_ASSERT(it.key() != nullptr);
			OGDF_ASSERT((*it) != nullptr);
			OGDF_ASSERT((*it)->graphOf() == m_pOriginal);
			mapA_toFinal[it.key()->adjSource()] = m_endGraph->chain(*it).front()->adjSource();
			mapA_toFinal[it.key()->adjTarget()] = m_endGraph->chain(*it).back()->adjTarget();
		}
		node s(m_sNode[e]), t(m_tNode[e]);
		List<node> nodesOfEmb;
		embG.allNodes(nodesOfEmb);
		// for every node of emb we order the adjEntries of the corresponding node
		// in the end graph, so that both match
		for (node v : nodesOfEmb) {
			if (v == s || v == t) {
				continue;
			}
			List<adjEntry> rightAdjOrder;
			for (adjEntry adj = v->firstAdj(); adj; adj = adj->succ()) {
				rightAdjOrder.pushBack(mapA_toFinal[adj]);
			}
			m_endGraph->sort(m_endGraph->copy(mapV_toOrig[v]), rightAdjOrder);
		}
	}

	template<typename Cost>
	void NonPlanarCore<Cost>::inflateCrossing(node v) {
		// we want e1 and e2 to be these two edges
		//      ^
		//      |
		// e2-->v--->
		//      ^
		//      |
		//      e1
		edge e1 = v->firstAdj()->theEdge();
		while (e1->target() != v) {
			e1 = e1->adjSource()->succ()->theEdge();
		}
		edge e2 = e1->adjTarget()->succ()->theEdge();
		while (e2->target() != v) {
			e2 = e2->adjSource()->cyclicSucc()->theEdge();
		}
		if (e1 == e2->adjTarget()->cyclicSucc()->theEdge()) {
			edge help = e1;
			e1 = e2;
			e2 = help;
		}
		OGDF_ASSERT(e2 == e1->adjTarget()->cyclicSucc()->theEdge());
		List<edge> e1cut;
		getMincut(e1, e1cut);
		List<edge> e2cut;
		getMincut(e2, e2cut);
		OGDF_ASSERT(e1 != e2);
		OGDF_ASSERT(e1cut.size() > 0);
		OGDF_ASSERT(e2cut.size() > 0);
		// the actual crossing insertion
		// for (auto it1 = e1cut.begin(); it1.valid(); it1++)
		for (int i = 0; i < e1cut.size(); i++) {
			auto it1 = e1cut.get(i);
			edge crossingEdge = *it1;
			for (int j = 0; j < e2cut.size(); j++) {
				auto it2 = e2cut.get(j);
				edge crossedEdge = *it2;
				m_endGraph->insertCrossing(*it1, crossedEdge, true);
				OGDF_ASSERT(crossedEdge == *it2);
				e2cut.insertAfter(crossedEdge, it2);
				e2cut.del(it2);
			}
			OGDF_ASSERT(crossingEdge != *it1);
			e1cut.insertAfter(crossingEdge, it1);
			e1cut.del(it1);
		}
	}

	template<typename Cost>
	void NonPlanarCore<Cost>::getMincut(edge e, List<edge> &cut) {
		OGDF_ASSERT(e->graphOf() == m_planarCore);

		cut.clear();
		// chain is a list of the edges of the planar core, that represent e
		List<edge> chain = m_planarCore->chain(m_planarCore->original(e));
		// this is the main part of this function:
		// as we know, the cut edges are split by splitdummies to partition the edge,
		// such that every crossing on the edge has its own section to be inserted into (denoted by pos)
		// cut_pre stores the first section for every cut edge
		for (CutEdge eCut : mincut(m_planarCore->original(e))) {
			OGDF_ASSERT(m_endGraph->chain(eCut.e).size() + 1 >= chain.size());
			// while iterating we have to differentiate between already inserted crossings and splitdummies
			// we can do that, by only counting the deg 2 nodes we pass while iterating through the chain of the cut edge
			auto it = m_endGraph->chain(eCut.e).begin();
			for (int i = 0; i < chain.pos(chain.search(e)); i++) {
				it++;
				while ((*it)->source()->degree() == 4) {
					it++;
					OGDF_ASSERT(it.valid());
				}
			}
			cut.pushBack(*(it));
		}
		// cut is the result of this function
	}

	template<typename Cost>
	void NonPlanarCore<Cost>::glueMincuts(edge eWinner, edge eLoser) {
#ifdef OGDF_DEBUG
		if (eWinner->adjSource()->theNode() == eLoser->adjSource()->theNode()) {
			OGDF_ASSERT(eWinner->adjSource()->theNode() == eLoser->adjSource()->theNode());
			OGDF_ASSERT(eWinner->adjTarget()->theNode() == eLoser->adjTarget()->theNode());
		}
		else {
			OGDF_ASSERT(eWinner->adjSource()->theNode() == eLoser->adjTarget()->theNode());
			OGDF_ASSERT(eWinner->adjTarget()->theNode() == eLoser->adjSource()->theNode());
		}
#endif
		List<CutEdge> wincut = m_mincut[eWinner];

		List<CutEdge> losecut = m_mincut[eLoser];

		if (eWinner->source() == eLoser->target()) {
			List<CutEdge> newLosecut;
			for (auto cutEit = losecut.begin(); cutEit != losecut.end(); cutEit++) {
				newLosecut.pushBack(CutEdge((*cutEit).e, !(*cutEit).dir));
			}
			losecut = newLosecut;
		}

		wincut.conc(losecut);
		m_mincut[eWinner] = wincut;
		m_cost[eWinner] += m_cost[eLoser];
	}

	template<typename Cost>
	GlueMap<Cost>::GlueMap(edge eWinner, edge eLoser, NonPlanarCore<Cost> &npc)
	: m_npc(npc), m_eWinner(eWinner), m_eLoser(eLoser)
	{
		OGDF_ASSERT(m_eWinner != m_eLoser);

		OGDF_ASSERT(m_npc.m_underlyingGraphs[m_eLoser] != nullptr);
		OGDF_ASSERT(m_npc.m_underlyingGraphs[m_eWinner] != nullptr);

		m_gLoser = m_npc.m_underlyingGraphs[m_eLoser];
		m_gWinner = m_npc.m_underlyingGraphs[m_eWinner];

		OGDF_ASSERT(m_gWinner != m_gLoser);

		OGDF_ASSERT(m_npc.m_mapV[m_eWinner] != nullptr);
		OGDF_ASSERT(m_npc.m_mapV[m_eLoser] != nullptr);

		m_mapVwinner = m_npc.m_mapV[m_eWinner];
		m_mapVloser = m_npc.m_mapV[m_eLoser];

		OGDF_ASSERT(m_npc.m_mapE[m_eWinner] != nullptr);
		OGDF_ASSERT(m_npc.m_mapE[m_eLoser] != nullptr);

		m_mapEwinner = m_npc.m_mapE[m_eWinner];
		m_mapEloser = m_npc.m_mapE[m_eLoser];

		OGDF_ASSERT(m_mapEloser->graphOf() == m_gLoser);
		OGDF_ASSERT(m_mapVloser->graphOf() == m_gLoser);

		OGDF_ASSERT(m_mapEwinner->graphOf() == m_gWinner);
		OGDF_ASSERT(m_mapVwinner->graphOf() == m_gWinner);

		m_mapE_l2w = EdgeArray<edge>(*m_gLoser, nullptr);
		m_mapV_l2w = NodeArray<node>(*m_gLoser, nullptr);
	}

	template<typename Cost>
	void GlueMap<Cost>::mapLoserToNewWinnerEdge(edge loser) {
		edge newEdge = m_gWinner->newEdge(m_mapV_l2w[loser->source()], m_mapV_l2w[loser->target()]);
		m_mapE_l2w[loser] = newEdge;
		(*m_mapEwinner)[newEdge] = (*m_mapEloser)[loser];
	}

	template<typename Cost>
	void GlueMap<Cost>::mapLoserToWinnerNode(node loser, node winner) {
		m_mapV_l2w[loser] = winner;
		(*m_mapVwinner)[winner] = (*m_mapVloser)[loser];
	}

	template<typename Cost>
	void GlueMap<Cost>::mapLoserToNewWinnerNode(node loser) {
		node newNode = m_gWinner->newNode();
		m_mapV_l2w[loser] = newNode;
		(*m_mapVwinner)[newNode] = (*m_mapVloser)[loser];
	}

	template<typename Cost>
	void GlueMap<Cost>::reorder(node vLoser, bool sameDirection, bool isTNodeOfPNode) {
		node vWinner = m_mapV_l2w[vLoser];
		List<adjEntry> rightAdjOrder;
		List<adjEntry> wrongAdjOrder;
		vWinner->allAdjEntries(wrongAdjOrder);
		OGDF_ASSERT(wrongAdjOrder.size() == vWinner->degree());

		OGDF_ASSERT(vLoser->degree() <= vWinner->degree());
		// for every adjEntry of v in the "right" graph (the embedding which we want to get into the "wrong" graph)
		// we search for the corresponding adjEntry in the list of adjEntries of the "wrong" v
		for (adjEntry adj : vLoser->adjEntries) {
			OGDF_ASSERT(m_mapE_l2w[adj->theEdge()] != nullptr);
			edge edgeInWinner = m_mapE_l2w[adj->theEdge()];
			adjEntry adj_in = (adj->theEdge()->adjSource() == adj ? edgeInWinner->adjSource() : edgeInWinner->adjTarget());
			rightAdjOrder.pushBack(adj_in);
		}
		List<adjEntry> adjOrder;
		vWinner->allAdjEntries(adjOrder);
		OGDF_ASSERT(vLoser->degree() <= adjOrder.size());
		if (!sameDirection) {
			rightAdjOrder.reverse();
		}
		if (adjOrder.size() == rightAdjOrder.size()) {
			adjOrder = rightAdjOrder;
		} else {
			List<adjEntry> helpList;
			adjOrder.split(adjOrder.get(adjOrder.size() - rightAdjOrder.size()), adjOrder, helpList);
			if (isTNodeOfPNode) {
				adjOrder.concFront(rightAdjOrder);
			} else {
				adjOrder.conc(rightAdjOrder);
			}
		}
		m_gWinner->sort(vWinner, adjOrder);
	}
}
