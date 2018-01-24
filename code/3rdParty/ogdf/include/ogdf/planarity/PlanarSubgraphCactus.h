/** \file
 * \brief Declaration of class PlanarSubgraphCactus.
 *
 * \author Tilo Wiedera
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

#include <ogdf/module/PlanarSubgraphModule.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/DisjointSets.h>

namespace ogdf {

//! Maximum planar subgraph approximation algorithm by Calinescu et al.
/**
 * @ingroup ga-plansub
 *
 * The algorithm has an approximation factor of 7/18.
 * Setting preferred edges is not supported.
 * Weighted edges are heuristically respected but there is no approximation guarantee in the weighted case.
 */
template<typename TCost>
class PlanarSubgraphCactus : public PlanarSubgraphModule<TCost>
{
public:
	virtual PlanarSubgraphCactus *clone() const override {
		return new PlanarSubgraphCactus();
	}

protected:
	virtual Module::ReturnType doCall(
		const Graph &graph,
		const List<edge> &,
		List<edge> &delEdges,
		const EdgeArray<TCost>  *pCost,
		bool preferedImplyPlanar) override
	{
		OGDF_ASSERT(isConnected(graph));
		OGDF_ASSERT(isSimpleUndirected(graph));

		delEdges.clear();
		GraphCopy copy(graph);
		DisjointSets<> components(copy.numberOfNodes());
		NodeArray<int> id(copy);
		for (node v : copy.nodes) {
			id[v] = components.makeSet();
		}

		EdgeArray<bool> includeEdge(copy, false);

		List<edge> edges;
		copy.allEdges(edges);

		// sort weighted edges
		if(pCost != nullptr) {
			EdgeComparer edgeCmp(copy, *pCost);
			AdjEntryComparer adjCmp(edgeCmp);
			edges.quicksort(edgeCmp);

			for(node v : copy.nodes) {
				List<adjEntry> newOrder;
				v->allAdjEntries(newOrder);
				newOrder.quicksort(adjCmp);
				copy.sort(v, newOrder);
			}
		}

		// select triangles
		for (edge e : edges) {
			node s = e->source();
			node t = e->target();
			int setS = components.find(id[s]);
			int setT = components.find(id[t]);

			if(setS != setT) {
				for(adjEntry adj : s->adjEntries) {
					edge f = adj->theEdge();
					node v = f->opposite(s);
					int setV = components.find(id[v]);

					if(setV != setS && setV != setT) {
						edge g = copy.searchEdge(v,  t);

						if(g != nullptr) {
							includeEdge[e] = includeEdge[f] = includeEdge[g] = true;
							components.link(components.link(setS, setT), setV);
							break;
						}
					}
				}
			}
		}

		// connect the subgraph and store the result
		for(edge e : edges) {
			int setS = components.find(id[e->source()]);
			int setT = components.find(id[e->target()]);

			if(setS != setT) {
				includeEdge[e] = true;
				components.link(setS, setT);
			}

			if(!includeEdge[e]) {
				delEdges.pushBack(copy.original(e));
			}
		}

		return Module::ReturnType::Feasible;
	}

private:
	class EdgeComparer {
	private:
		const GraphCopy &m_copy;
		const EdgeArray<TCost> &m_weight;
	public:
		EdgeComparer(const GraphCopy &copy, const EdgeArray<TCost> &weight) : m_copy(copy), m_weight(weight) { }

		bool less(const edge &e, const edge &f) const {
			return m_weight[m_copy.original(e)] < m_weight[m_copy.original(f)];
		}
	};

	class AdjEntryComparer {
	private:
		const EdgeComparer &m_edgeComparer;
	public:
		explicit AdjEntryComparer(const EdgeComparer &edgeComparer) : m_edgeComparer(edgeComparer) { }

		bool less(adjEntry adjA, adjEntry adjB) const {
			return m_edgeComparer.less(adjA->theEdge(), adjB->theEdge());
		}

	};
};

}
