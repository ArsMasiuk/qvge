/** \file
 * \brief Declares ogdf::CliqueFinderHeuristic class.
 *
 * \author Karsten Klein, Max Ilsen
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

#include <ogdf/clique/CliqueFinderModule.h>
#include <ogdf/basic/AdjacencyOracle.h>

namespace ogdf{

//! Finds cliques and dense subgraphs using a heuristic.
/**
 * @ingroup ga-induced
 *
 * The class CliqueFinderHeuristic can be called on a graph to retrieve
 * (disjoint) cliques or dense subgraphs respectively by using a greedy
 * heuristic.
 */
class OGDF_EXPORT CliqueFinderHeuristic : public CliqueFinderModule {

public:
	//! Creates a CliqueFinderHeuristic.
	explicit CliqueFinderHeuristic();

	/**
	 * Sets whether postprocessing should be activated.
	 *
	 * @param postProcess Whether postprocessing should be activated.
	 */
	inline void setPostProcessing(bool postProcess) {
		m_postProcess = postProcess;
	}

	/**
	 * Sets the density needed for subgraphs to be detected.
	 *
	 * For a subgraph of size k to recognized as dense, it has to contain at
	 * least (\p density * (k*(k-1))/2) edges.
	 * This setting does not have an effect for graphs with less than 3 nodes.
	 *
	 * @param density value in [0.0 ... 1.0]
	 */
	void setDensity(double density) {
		if (density < 0.0) {
			m_density = 0.0;
		} else if (density > 1.0) {
			m_density = 1.0;
		} else {
			m_density = density;
		}
	}

protected:
	//! @copydoc CliqueFinderModule::doCall
	void doCall() override;

	/**
	 * Deletes all nodes from #m_pCopy with degree < #m_density * #m_minDegree
	 * in O(n+m).
	 */
	void preProcess();

	/**
	 * If postprocessing is activated, use the result of the first phase and
	 * revisit cliques that are too small. These cliques are rearranged to
	 * potentially find new, bigger cliques.
	 *
	 * @param cliqueList The cliques to postprocess.
	 */
	void postProcessCliques(List<List<node>*> &cliqueList);

	/**
	 * Checks whether \p v is adjacent to (at least #m_density times) all nodes
	 * in \p vList.
	 *
	 * @warning #m_pGraph must be parallel free!
	 *
	 * @param v The node to check.
	 * @param vList The nodes that potentially form a dense subgraph with \p v.
	 * @return whether node \p v is adjacent to (at least #m_density times) all
	 * nodes in \p vList.
	 */
	bool allAdjacent(node v, List<node>* vList) const;

	/**
	 * Evaluates \p v in #m_pCopy heuristically concerning its qualification as
	 * a clique start node. The higher this value, the better the node as a
	 * clique start node.
	 *
	 * @return The number of 3-circles starting at \p v.
	 */
	int evaluate(node v);

	/**
	 * Searches for a clique/dense subgraph around node \p v in list \p neighbours.
	 * \p neighbours + \p v will form a clique/dense subgraph afterwards.
	 *
	 * @param v The node around which to search for a clique/dense subgraph.
	 * @param neighbours The nodes which potentially form a clique together with
	 * \p v. Is assigned a list of nodes that actually forms a clique/dense
	 * subgraph together with \p v.
	 */
	void findClique(node v, List<node> &neighbours);

private:
	double m_density; //!< Value in [0,1] defining how dense subgraphs need to be.

	bool m_postProcess; //!< Whether postprocessing should be activated.

	AdjacencyOracle *m_adjOracle; //!< Adjacency oracle for #m_pCopy.

	NodeArray<bool> m_usedNode; //!< Whether the node is already assigned to a clique.
};

}
