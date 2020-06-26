/** \file
 * \brief Declaration of class SubgraphPlanarizerUML.
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

#include <ogdf/uml/UMLCrossingMinimizationModule.h>
#include <ogdf/planarity/PlanarSubgraphModule.h>
#include <ogdf/uml/UMLEdgeInsertionModule.h>
#include <memory>
#include <ogdf/basic/Logger.h>

#include <random>

namespace ogdf {

//! The planarization approach for UML crossing minimization.
/**
 * This crossing minimization module represents a customizable implementation
 * of the planarization approach. This approach consists of two phases.
 * In the first phase, a planar subgraph is computed, and in the second
 * phase, the remaining edges are re-inserted one-by-one, each time with
 * as few crossings as possible; the crossings are then replaced by dummy
 * nodes of degree four, resulting in a <i>planarized representation</i> of the
 * graph.
 *
 * Both steps, the computation of the planar subgraph and the re-insertion
 * of a single edge, are implemented using module options. Additionaly,
 * the second phase can be repeated several times, each time with a randomly
 * permuted order of the edges to be re-inserted, and taking the solution
 * with the least crossings. This can improve the quality of the solution
 * significantly. More details on the planarization approach can be found in
 *
 * C. Gutwenger, P. Mutzel: <i>An Experimental Study of Crossing
 * Minimization Heuristics</i>. 11th International Symposium on %Graph
 * Drawing 2003, Perugia (GD '03), LNCS 2912, pp. 13-24, 2004.
 *
 * <H3>Optional parameters</H3>
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>permutations</i><td>int<td>1
 *     <td>The number of permutations the (complete) edge insertion phase is repeated.
 *   </tr><tr>
 *     <td><i>setTimeout</i><td>bool<td>true
 *     <td>If set to true, the time limit is also passed to submodules; otherwise,
 *     a timeout might be checked late when a submodule requires a lot of runtime.
 *   </tr><tr>
 *     <td><i>maxThreads</i><td>int<td>System::numberOfProcessors()
 *     <td>This is the maximal number of threads that will be used for parallelizing the
 *     algorithm. At the moment, each permutation is parallelized, hence the there will
 *     never be used more threads than permutations. To achieve sequential behaviour, set
 *     maxThreads to 1.
 *   </tr>
 * </table>
 *
 * <H3>%Module options</H3>
 * The various phases of the algorithm can be exchanged by setting
 * module options allowing flexible customization. The algorithm provides
 * the following module options:
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>subgraph</i><td>PlanarSubgraphModule<td>FastPlanarSubgraph
 *     <td>The module for the computation of the planar subgraph.
 *   </tr><tr>
 *     <td><i>inserter</i><td>UMLEdgeInsertionModule<td>VariableEmbeddingInserterLight
 *     <td>The module used for edge insertion. The edges not contained in the planar
 *     subgraph are re-inserted one-by-one, each with as few crossings as possible.
 *   </tr>
 * </table>
*/
class OGDF_EXPORT SubgraphPlanarizerUML : public UMLCrossingMinimizationModule, public Logger
{
	class ThreadMaster;
	class Worker;

protected:
	//! Implements the algorithm call.
	virtual ReturnType doCall(
		PlanRepUML &pr,
		int cc,
		const EdgeArray<int>  *pCostOrig,
		int& crossingNumber) override;

public:
	//! Creates an instance of subgraph planarizer with default settings.
	SubgraphPlanarizerUML();

	//! Creates an instance of subgraph planarizer with the same settings as \p planarizer.
	SubgraphPlanarizerUML(const SubgraphPlanarizerUML &planarizer);

	//! Returns a new instance of subgraph planarizer with the same option settings.
	virtual UMLCrossingMinimizationModule *clone() const override;

	//! Assignment operator. Copies option settings only.
	SubgraphPlanarizerUML &operator=(const SubgraphPlanarizerUML &planarizer);


	//! Sets the module option for the computation of the planar subgraph.
	void setSubgraph(PlanarSubgraphModule<int> *pSubgraph) {
		m_subgraph.reset(pSubgraph);
	}

	//! Sets the module option for the edge insertion module.
	void setInserter(UMLEdgeInsertionModule *pInserter) {
		m_inserter.reset(pInserter);
	}

	//! Returns the number of permutations.
	int permutations() { return m_permutations; }

	//! Sets the number of permutations to \p p.
	void permutations(int p) { m_permutations = p; }

	//! Returns the current setting of options <i>setTimeout</i>.
	bool setTimeout() { return m_setTimeout; }

	//! Sets the option <i>setTimeout</i> to \p b.
	void setTimeout(bool b) { m_setTimeout = b; }

	//! Returns the maximal number of used threads.
	unsigned int maxThreads() const { return m_maxThreads; }

	//! Sets the maximal number of used threads to \p n.
	void maxThreads(unsigned int n) {
#ifndef OGDF_MEMORY_POOL_NTS
		m_maxThreads = n;
#endif
	}

private:
	static void doWorkHelper(ThreadMaster &master, UMLEdgeInsertionModule &inserter, std::minstd_rand &rng);

	static bool doSinglePermutation(
		PlanRepLight &prl,
		int cc,
		const EdgeArray<int>  *pCost,
		Array<edge> &deletedEdges,
		UMLEdgeInsertionModule &inserter,
		std::minstd_rand &rng,
		int &crossingNumber);

	std::unique_ptr<PlanarSubgraphModule<int>>   m_subgraph; //!< The planar subgraph algorithm.
	std::unique_ptr<UMLEdgeInsertionModule> m_inserter; //!< The edge insertion module.

	int m_permutations;	//!< The number of permutations.
	bool m_setTimeout;	//!< The option for setting timeouts in submodules.
	unsigned int m_maxThreads;	//!< The maximal number of used threads.
};

}
