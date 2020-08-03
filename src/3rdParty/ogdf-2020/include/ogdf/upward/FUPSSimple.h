/** \file
 * \brief Declaration of the FUPSSimple.
 *
 * \author Hoi-Ming Wong
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

#include <ogdf/upward/FUPSModule.h>

namespace ogdf {


class OGDF_EXPORT FUPSSimple : public FUPSModule{

public:
	//! Creates an instance of feasible subgraph algorithm.
	FUPSSimple() : m_nRuns(0) { }

	// destructor
	~FUPSSimple() { }


	// options

	//! Sets the number of randomized runs to \p nRuns.
	void runs (int nRuns) {
		m_nRuns = nRuns;
	}

	//! Returns the current number of randomized runs.
	int runs() const {
		return m_nRuns;
	}


	//! return a adjEntry of node v which right face is f. Be Carefully! The adjEntry is not always unique.
	adjEntry getAdjEntry(const CombinatorialEmbedding &Gamma, node v, face f)
	{
		adjEntry adjFound = nullptr;
		for(adjEntry adj : v->adjEntries) {
			if (Gamma.rightFace(adj) == f) {
				adjFound = adj;
				break;
			}
		}

		OGDF_ASSERT(Gamma.rightFace(adjFound) == f);

		return adjFound;
	}

protected:

	/**
	 * \brief Computes a feasible upward planar subgraph of the input graph.
	 *
	 * @param UPR represents the feasible upward planar subgraph after the call. \p UPR has to be initialzed as a
	 *        UpwardPlanRep of the input connected graph G and is modified to obtain the upward planar subgraph.
	 *		  The subgraph is represented as an upward planar representation.
	 * @param delEdges is the deleted edges in order to obtain the subgraph. The edges are edges of the original graph G.
	 * \return the status of the result.
	 */
	virtual Module::ReturnType doCall(UpwardPlanRep &UPR,
		List<edge> &delEdges) override;


private:

	int m_nRuns;  //!< The number of runs for randomization.

	void computeFUPS(UpwardPlanRep &UPR,
					List<edge> &delEdges);

	//! Compute a (random) span tree of the input sT-Graph.
	/*
	 * @param GC The Copy of the input graph G.
	 * @param &delEdges The deleted edges (edges of G).
	 * @param random compute a random span tree
	 * @multisource true, if the original graph got multisources. In this case, the incident edges of
	 *  the source are allways included in the span tree
	 */
	void getSpanTree(GraphCopy &GC, List<edge> &delEdges, bool random);

	/*
	 * Function use by geSpannTree to compute the spannig tree.
	 */
	void dfs_visit(const Graph &G, edge e, NodeArray<bool> &visited, EdgeArray<bool> &treeEdges, bool random);

	// construct a merge graph with repsect to gamma and its test acyclicity
	bool constructMergeGraph(GraphCopy &M, // copy of the original graph, muss be embedded
							adjEntry adj_orig, // the adjEntry of the original graph, which right face is the ext. Face and adj->theNode() is the source
							const List<edge> &del_orig); // deleted edges
};

}
