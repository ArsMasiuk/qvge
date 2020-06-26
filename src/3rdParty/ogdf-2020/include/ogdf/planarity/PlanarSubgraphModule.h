/** \file
 * \brief Declaration of interface for planar subgraph algorithms.
 *
 * \author Carsten Gutwenger
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

#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/Module.h>
#include <ogdf/basic/Logger.h>
#include <ogdf/basic/Timeouter.h>
#include <ogdf/basic/Thread.h>


namespace ogdf {

/**
 * \brief Interface for planar subgraph algorithms.
 *
 * \see PlanarizationLayout, PlanarizationGridLayout
 */
template<typename TCost>
class PlanarSubgraphModule : public Module, public Timeouter {

	unsigned int m_maxThreads;	//!< The maximal number of used threads.

public:
	//! Initializes a planar subgraph module (default constructor).
	PlanarSubgraphModule() {
#ifdef OGDF_MEMORY_POOL_NTS
		m_maxThreads = 1;
#else
		m_maxThreads = max(1u, Thread::hardware_concurrency());
#endif
	}


	/**
	 * \brief Returns the set of edges \p delEdges which have to be deleted to obtain the planar subgraph.
	 * @param G is the input graph.
	 * @param cost are the costs of edges.
	 * @param preferredEdges are edges that should be contained in the planar subgraph.
	 * @param delEdges is the set of edges that need to be deleted to obtain the planar subgraph.
	 * @param preferredImplyPlanar indicates that the edges \p preferredEdges induce a planar graph.
	 */
	ReturnType call(const Graph &G,
		const EdgeArray<TCost> &cost,
		const List<edge> &preferredEdges,
		List<edge> &delEdges,
		bool preferredImplyPlanar = false)
	{
		return doCall(G,preferredEdges,delEdges,&cost,preferredImplyPlanar);
	}


	/**
	 * \brief Returns the set of edges \p delEdges which have to be deleted to obtain the planar subgraph.
	 * @param G is the input graph.
	 * @param preferredEdges are edges that should be contained in the planar subgraph.
	 * @param delEdges is the set of edges that need to be deleted to obtain the planar subgraph.
	 * @param preferredImplyPlanar indicates that the edges \p preferredEdges induce a planar graph.
	 */
	ReturnType call(const Graph &G,
		const List<edge> &preferredEdges,
		List<edge> &delEdges,
		bool preferredImplyPlanar = false)
	{
		return doCall(G,preferredEdges,delEdges,nullptr,preferredImplyPlanar);
	}


	/**
	 * \brief Returns the set of edges \p delEdges which have to be deleted to obtain the planar subgraph.
	 * @param G is the input graph.
	 * @param cost are the costs of edges.
	 * @param delEdges is the set of edges that need to be deleted to obtain the planar subgraph.
	 */
	ReturnType call(const Graph &G, const EdgeArray<TCost> &cost, List<edge> &delEdges) {
		List<edge> preferredEdges;
		return doCall(G,preferredEdges,delEdges, &cost);
	}

	/**
	 * \brief Returns the set of edges \p delEdges which have to be deleted to obtain the planar subgraph.
	 * @param G is the input graph.
	 * @param delEdges is the set of edges that need to be deleted to obtain the planar subgraph.
	 */
	ReturnType call(const Graph &G, List<edge> &delEdges) {
		List<edge> preferredEdges;
		return doCall(G,preferredEdges,delEdges);
	}


	//! Returns the set of edges \p delEdges which have to be deleted to obtain the planar subgraph.
	ReturnType operator()(const Graph &G,
		const List<edge> &preferredEdges,
		List<edge> &delEdges,
		bool preferredImplyPlanar = false)
	{
		return call(G,preferredEdges,delEdges,preferredImplyPlanar);
	}

	//! Returns the set of edges \p delEdges which have to be deleted to obtain the planar subgraph.
	ReturnType operator()(const Graph &G, List<edge> &delEdges) {
		return call(G,delEdges);
	}


	/**
	 * \brief Makes \p GC planar by deleting edges.
	 * @param GC is a copy of the input graph.
	 * @param preferredEdges are edges in \p GC that should be contained in the planar subgraph.
	 * @param delOrigEdges is the set of original edges whose copy has been deleted in \p GC.
	 * @param preferredImplyPlanar indicates that the edges \p preferredEdges induce a planar graph.
	 */
	ReturnType callAndDelete(GraphCopy &GC,
		const List<edge> &preferredEdges,
		List<edge> &delOrigEdges,
		bool preferredImplyPlanar = false)
	{
		List<edge> delEdges;
		ReturnType retValue = call(GC, preferredEdges, delEdges, preferredImplyPlanar);
		if(isSolution(retValue)) {
			for (edge eCopy : delEdges) {
				delOrigEdges.pushBack(GC.original(eCopy));
				GC.delEdge(eCopy);
			}
		}
		return retValue;
	}

	/**
	 * \brief Makes \p G planar by deleting edges.
	 * @param GC is a copy of the input graph.
	 * @param delOrigEdges is the set of original edges whose copy has been deleted in \p GC.
	 */
	ReturnType callAndDelete(GraphCopy &GC, List<edge> &delOrigEdges) {
		List<edge> preferredEdges;
		return callAndDelete(GC,preferredEdges,delOrigEdges);
	}

	//! Returns a new instance of the planar subgraph module with the same option settings.
	virtual PlanarSubgraphModule *clone() const = 0;

	//! Returns the maximal number of used threads.
	unsigned int maxThreads() const { return m_maxThreads; }

	//! Sets the maximal number of used threads to \p n.
	void maxThreads(unsigned int n) {
#ifndef OGDF_MEMORY_POOL_NTS
		m_maxThreads = n;
#endif
	}

protected:
	// computes set of edges delEdges, which have to be deleted
	// in order to get a planar subgraph; edges in preferredEdges
	// should be contained in planar subgraph
	// must be implemented by derived classes!
	/**
	 * \brief Computes the set of edges \p delEdges which have to be deleted to obtain the planar subgraph.
	 *
	 * This is the actual algorithm call and needs to be implemented
	 * by derived classes.
	 * @param G is the input graph.
	 * @param preferredEdges are edges that should be contained in the planar subgraph.
	 * @param delEdges is the set of edges that need to be deleted to obtain the planar subgraph.
	 * @param pCost is apointer to an edge array containing the edge costs; this pointer
	 *        can be 0 if no costs are given (all edges have cost 1).
	 * @param preferredImplyPlanar indicates that the edges \p preferredEdges induce a planar graph.
	 */
	virtual ReturnType doCall(const Graph &G,
		const List<edge> &preferredEdges,
		List<edge> &delEdges,
		const EdgeArray<TCost>  *pCost = nullptr,
		bool preferredImplyPlanar = false) = 0;

	OGDF_MALLOC_NEW_DELETE
};

}
