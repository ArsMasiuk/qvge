/** \file
 * \brief Declaration of the PlanarSubgraphFast.
 *
 * \author Sebastian Leipert
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

#include <ogdf/planarity/PlanarSubgraphModule.h>
#include <ogdf/basic/STNumbering.h>
#include <ogdf/planarity/booth_lueker/PlanarLeafKey.h>
#include <ogdf/planarity/planar_subgraph_fast/PlanarSubgraphPQTree.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Thread.h>
#include <atomic>

namespace ogdf {

/**
 * \brief Computation of a planar subgraph using PQ-trees.
 *
 * @ingroup ga-plansub
 *
 * Literature: Jayakumar, Thulasiraman, Swamy 1989
 *
 * <h3>Optional Parameters</h3>
 *
 * <table>
 *   <tr>
 *     <th>Option</th><th>Type</th><th>Default</th><th>Description</th>
 *   </tr><tr>
 *     <td><i>runs</i></td><td>int</td><td>0</td>
 *     <td>the number of randomized runs performed by the algorithm; the best
 *         solution is picked among all the runs. If runs is 0, one
 *         deterministic run is performed.</td>
 *   </tr>
 * </table>
 *
 * Observe that this algorithm by theory does not compute a maximal
 * planar subgraph. It is however the fastest known good heuristic.
 */
template<typename TCost>
class PlanarSubgraphFast : public PlanarSubgraphModule<TCost> {

	using BlockType = std::pair<Graph*,EdgeArray<edge>*>;

	class ThreadMaster {
		Array<TCost>            m_bestSolution;  //!< value of best solution for block
		Array<List<edge>*>      m_bestDelEdges;  //!< best solution for block
		int                     m_nBlocks;  //!< number of blocks
		const Array<BlockType>& m_block;  //!< the blocks (graph and edge mapping)
		const EdgeArray<TCost>* m_pCost;  //!< edge cost (may be 0)
		std::atomic<int>        m_runs;
		std::mutex              m_mutex; //!< thread synchronization

	public:
		ThreadMaster(const Array<BlockType> &block, const EdgeArray<TCost> *pCost, int runs)
		: m_bestSolution(block.size())
		, m_bestDelEdges(block.size())
		, m_nBlocks(block.size())
		, m_block(block)
		, m_pCost(pCost)
		, m_runs(runs)
		{
			for(int i = 0; i < m_nBlocks; ++i) {
				m_bestDelEdges[i] = nullptr;
				m_bestSolution[i] = (m_block[i].first != nullptr) ? std::numeric_limits<int>::max() : 0;
			}
		}

		int numBlocks() const {
			return m_nBlocks;
		}

		const Graph &block(int i) const {
			return *m_block[i].first;
		}

		bool considerBlock(int i) const {
			return m_bestSolution[i] > 1;
		}

		List<edge> *postNewResult(int i, List<edge> *pNewDelEdges) {
			TCost newSolution = pNewDelEdges->size();
			if(m_pCost != nullptr) {
				const EdgeArray<edge> &origEdge = *m_block[i].second;
				newSolution = TCost(0);
				for(edge e : *pNewDelEdges)
					newSolution += (*m_pCost)[origEdge[e]];
			}

			// m_mutex is automatically released when guard goes out of scope
			std::lock_guard<std::mutex> guard(m_mutex);

			if(newSolution < m_bestSolution[i]) {
				std::swap(pNewDelEdges, m_bestDelEdges[i]);
				m_bestSolution[i] = newSolution;
			}

			return pNewDelEdges;
		}

		// creates a solution for the original graph from best solutions per block
		// also deletes (intermediate) solution lists
		void buildSolution(List<edge> &delEdges) {
			for(int i = 0; i < m_nBlocks; ++i) {
				if(m_bestDelEdges[i] != nullptr) {
					const EdgeArray<edge> &origEdge = *m_block[i].second;
					for(edge e : *m_bestDelEdges[i])
						delEdges.pushBack(origEdge[e]);
					delete m_bestDelEdges[i];
				}
			}
		}

		bool getNextRun() {
			return --m_runs >= 0;
		}
	};

	class Worker {
		ThreadMaster *m_pMaster;  // associated master

	public:
		Worker(ThreadMaster *pMaster) : m_pMaster(pMaster) { }
		~Worker() = default;

		void operator()() {
			doWorkHelper(*m_pMaster);
		}

	private:
		Worker(const Worker &other); // = delete
		Worker &operator=(const Worker &other); // = delete
	};

public:
	//! Creates an instance of the fast planar subgraph algorithm with default settings (\a runs = 10).
	PlanarSubgraphFast() {
		m_nRuns = 10;
	}

	//! Returns a new instance of fast planar subgraph with the same option settings.
	virtual PlanarSubgraphFast *clone() const override {
		auto* res = new PlanarSubgraphFast<TCost>(*this);
		res->m_nRuns = m_nRuns;
		return res;
	}

	//! Sets the number of randomized runs to \p nRuns.
	void runs (int nRuns) {
		m_nRuns = nRuns;
	}

	//! Returns the current number of randomized runs.
	int runs() const {
		return m_nRuns;
	}


protected:
	//! Returns true, if G is planar, false otherwise.
	/**
	 * \todo Add timeout support (limit number of runs when timeout is reached).
	 */
	virtual Module::ReturnType doCall(
		const Graph &G,
		const List<edge> &preferedEdges,
		List<edge> &delEdges,
		const EdgeArray<TCost>  *pCost,
		bool preferedImplyPlanar) override
	{
		delEdges.clear();

		if (G.numberOfEdges() < 9)
			return Module::ReturnType::Optimal;

		// Determine Biconnected Components
		EdgeArray<int> componentID(G);
		int nBlocks = biconnectedComponents(G,componentID);

		// Determine edges per biconnected component
		Array<SList<edge>> blockEdges(0,nBlocks-1);
		for(edge e : G.edges) {
			if (!e->isSelfLoop())
				blockEdges[componentID[e]].pushFront(e);
		}

		// Build non-trivial blocks
		Array<BlockType> block(nBlocks);
		NodeArray<node> copyV(G,nullptr);

		for(int i = 0; i < nBlocks; i++) {
			if(blockEdges[i].size() < 9) {
				block[i] = BlockType((Graph*)nullptr, (EdgeArray<edge>*)nullptr); // explicit casts required for VS2010
				continue;
			}

			Graph *bc = new Graph;
			EdgeArray<edge> *origE = new EdgeArray<edge>(*bc,nullptr);
			block[i] = BlockType(bc,origE);

			SList<node> marked;
			for (edge e : blockEdges[i]) {
				if (copyV[e->source()] == nullptr) {
					copyV[e->source()] = bc->newNode();
					marked.pushBack(e->source());
				}
				if (copyV[e->target()] == nullptr) {
					copyV[e->target()] = bc->newNode();
					marked.pushBack(e->target());
				}

				(*origE)[bc->newEdge(copyV[e->source()],copyV[e->target()])] = e;
			}

			for (node v : marked)
				copyV[v] = nullptr;
		}
		copyV.init();

		int nRuns = max(1, m_nRuns);
		unsigned int nThreads = min(this->maxThreads(), (unsigned int)nRuns);

		if(nThreads == 1)
			seqCall(block, pCost, nRuns, (m_nRuns == 0), delEdges);
		else
			parCall(block, pCost, nRuns, nThreads, delEdges);

		// clean-up
		for(int i = 0; i < nBlocks; i++) {
			delete block[i].first;
			delete block[i].second;
		}

		return Module::ReturnType::Feasible;
	}


private:
	int m_nRuns;  //!< The number of runs for randomization.

	//! Realizes the sequential implementation.
	void seqCall(const Array<BlockType> &block,
			const EdgeArray<TCost> *pCost,
			int nRuns,
			bool randomize,
			List<edge> &delEdges
	) {
		const int nBlocks = block.size();

		Array<TCost>       bestSolution(nBlocks);
		Array<List<edge>*> bestDelEdges(nBlocks);

		for(int i = 0; i < nBlocks; ++i) {
			bestDelEdges[i] = nullptr;
			bestSolution[i] = (block[i].first != nullptr) ? std::numeric_limits<TCost>::max() : 0;
		}

		for(int run = 0; run < nRuns; ++run) {
			for(int i = 0; i < nBlocks; ++i) {
				if(bestSolution[i] > 1) {
					const Graph           &B        = *block[i].first;
					const EdgeArray<edge> &origEdge = *block[i].second;

					// compute (randomized) st-numbering
					NodeArray<int> numbering(B, 0);
					computeSTNumbering(B, numbering, nullptr, nullptr, randomize);

					List<edge> *pCurrentDelEdges = new List<edge>;
					planarize(B, numbering, *pCurrentDelEdges);

					TCost currentSolution;
					if(pCost == nullptr) {
						currentSolution = pCurrentDelEdges->size();
					} else {
						currentSolution = 0;
						for(edge e : *pCurrentDelEdges)
							currentSolution += (*pCost)[origEdge[e]];
					}

					if(currentSolution < bestSolution[i]) {
						delete bestDelEdges[i];
						bestDelEdges[i] = pCurrentDelEdges;
						bestSolution[i] = currentSolution;
					} else
						delete pCurrentDelEdges;
				}
			}
		}

		// build final solution from block solutions
		for(int i = 0; i < nBlocks; ++i) {
			if(bestDelEdges[i] != nullptr) {
				const EdgeArray<edge> &origEdge = *block[i].second;
				for(edge e : *bestDelEdges[i])
					delEdges.pushBack(origEdge[e]);
				delete bestDelEdges[i];
			}
		}
	}

	//! Realizes the parallel implementation.
	void parCall(const Array<BlockType> &block,
			const EdgeArray<TCost> *pCost,
			int nRuns,
			unsigned int nThreads,
			List<edge> &delEdges
	) {
		ThreadMaster master(block, pCost, nRuns-nThreads);

		Array<Worker*> worker(nThreads-1);
		Array<Thread>  thread(nThreads-1);
		for(unsigned int i = 0; i < nThreads-1; ++i) {
			worker[i] = new Worker(&master);
			thread[i] = Thread(*worker[i]);
		}

		doWorkHelper(master);

		for(unsigned int i = 0; i < nThreads-1; ++i) {
			thread[i].join();
			delete worker[i];
		}

		master.buildSolution(delEdges);
	}

	//! Performs a planarization on a biconnected component of \p G.
	/** The numbering contains an st-numbering of the component.
	 */
	static void planarize(const Graph &G, NodeArray<int> &numbering, List<edge> &delEdges) {
		using PlanarLeafKey = booth_lueker::PlanarLeafKey<whaInfo*>;

		NodeArray<SListPure<PlanarLeafKey*>> inLeaves(G);
		NodeArray<SListPure<PlanarLeafKey*>> outLeaves(G);
		Array<node> table(G.numberOfNodes()+1);

		for(node v : G.nodes) {
			for(adjEntry adj : v->adjEntries) {
				edge e = adj->theEdge();
				if (numbering[e->opposite(v)] > numbering[v]) { // sideeffect: ignores selfloops
					PlanarLeafKey* L = new PlanarLeafKey(e);
					inLeaves[v].pushFront(L);
				}
			}
			table[numbering[v]] = v;
		}

		for(node v : G.nodes) {
			for (PlanarLeafKey *L : inLeaves[v]) {
				outLeaves[L->userStructKey()->opposite(v)].pushFront(L);
			}
		}

		SList<PQLeafKey<edge,whaInfo*,bool>*> totalEliminatedKeys;

		PlanarSubgraphPQTree T;
		T.Initialize(inLeaves[table[1]]);
		for (int i = 2; i < G.numberOfNodes(); i++) {
			SList<PQLeafKey<edge,whaInfo*,bool>*> eliminatedKeys;
			T.Reduction(outLeaves[table[i]], eliminatedKeys);
			totalEliminatedKeys.conc(eliminatedKeys);
			T.ReplaceRoot(inLeaves[table[i]]);
			T.emptyAllPertinentNodes();
		}

		for (PQLeafKey<edge, whaInfo*, bool> *key : totalEliminatedKeys) {
			edge e = key->userStructKey();
			delEdges.pushBack(e);
		}

		//cleanup
		for(node v : G.nodes) {
			while (!inLeaves[v].empty()) {
				PlanarLeafKey *L = inLeaves[v].popFrontRet();
				delete L;
			}
		}

		T.Cleanup();	// Explicit call for destructor necessary. This allows to call virtual
		// function CleanNode for freeing node information class.
	}

	static void doWorkHelper(ThreadMaster &master) {
		const int nBlocks = master.numBlocks();

		do {
			for(int i = 0; i < nBlocks; ++i) {
				if(master.considerBlock(i)) {
					const Graph &B = master.block(i);

					NodeArray<int> numbering(B,0); // compute (randomized) st-numbering
					computeSTNumbering(B, numbering, nullptr, nullptr, true);

					List<edge> *pCurrentDelEdges = new List<edge>;
					planarize(B, numbering, *pCurrentDelEdges);

					pCurrentDelEdges = master.postNewResult(i, pCurrentDelEdges);
					delete pCurrentDelEdges;
				}
			}

		} while(master.getNextRun());
	}
};

}
