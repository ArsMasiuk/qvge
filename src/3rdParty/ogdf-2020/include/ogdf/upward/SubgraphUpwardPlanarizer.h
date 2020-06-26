 /** \file
 * \brief Declaration of class SubgraphUpwardPlanarizer.
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

#include <memory>
#include <ogdf/layered/AcyclicSubgraphModule.h>
#include <ogdf/upward/FUPSModule.h>
#include <ogdf/upward/UpwardEdgeInserterModule.h>
#include <ogdf/upward/UpwardPlanarizerModule.h>
#include <ogdf/upward/UpwardPlanRep.h>
#include <ogdf/upward/FUPSSimple.h>
#include <ogdf/upward/FixedEmbeddingUpwardEdgeInserter.h>
#include <ogdf/decomposition/BCTree.h>
#include <ogdf/layered/GreedyCycleRemoval.h>

namespace ogdf {

/**
 * Takes an acyclic connected non-upward-planar graph and planarizes it, i.e., we obtain an upward-planar graph
 * where crossings are represented via dummy vertices. The code corresponds to the following paper by Hoi-Ming Wong:
 *
 *   M. Chimani, C. Gutwenger, P. Mutzel, H.-M. Wong.
 *   Layer-Free Upward Crossing Minimization.
 *   ACM Journal of Experimental Algorithmics, Vol. 15, Art.No. 2.2, 27 pages, ACM, 2010.
 *
 * \note To call the planarizer, an UpwardPlanRep is required. It needs to have the input graph as its orginal,
 * but is initially empty! After the algorithm, it will hold the representation.
 *
 * Example for Input "Graph G":
 * \code
 *  UpwardPlanRep U;
 *  U.createEmpty(G);
 *  SubgraphUpwardPlanarizer sup;
 *  sup.call(U, 0, 0);
 * \endcode
 */
class OGDF_EXPORT SubgraphUpwardPlanarizer : public UpwardPlanarizerModule
{

public:
	//! Creates an instance of subgraph planarizer.
	SubgraphUpwardPlanarizer()
	{
		m_runs = 1;
		//set default module
		m_subgraph.reset(new FUPSSimple());
		m_inserter.reset(new FixedEmbeddingUpwardEdgeInserter());
		m_acyclicMod.reset(new GreedyCycleRemoval());
	}

	//! Sets the module option for the computation of the feasible upward planar subgraph.
	void setSubgraph(FUPSModule *FUPS) {
		m_subgraph.reset(FUPS);
	}

	//! Sets the module option for the edge insertion module.
	void setInserter(UpwardEdgeInserterModule *pInserter) {
		m_inserter.reset(pInserter);
	}

	//! Sets the module option for acyclic subgraph module.
	void setAcyclicSubgraphModule(AcyclicSubgraphModule *acyclicMod) {
		m_acyclicMod.reset(acyclicMod);
	}

	int runs() {return m_runs;}
	void runs(int n) {m_runs = n;}

protected:

	virtual ReturnType doCall(UpwardPlanRep &UPR,
		const EdgeArray<int>  &cost,
		const EdgeArray<bool> &forbid) override;

	std::unique_ptr<FUPSModule> m_subgraph; //!< The upward planar subgraph algorithm.
	std::unique_ptr<UpwardEdgeInserterModule> m_inserter; //!< The edge insertion module.
	std::unique_ptr<AcyclicSubgraphModule> m_acyclicMod; //!<The acyclic subgraph module.
	int m_runs;

private:

	void constructComponentGraphs(BCTree &BC, NodeArray<GraphCopy> &biComps);

	//! traversion the BTree and merge the component to a common graph
	void dfsMerge(const GraphCopy &GC,
	              BCTree &BC,
	              NodeArray<GraphCopy> &biComps,
	              NodeArray<UpwardPlanRep> &uprs,
	              UpwardPlanRep &UPR_res,
	              node parent_BC,
	              node current_BC,
	              NodeArray<bool> &nodesDone);


	//! add UPR to UPR_res.
	void merge(const GraphCopy &GC,
	           UpwardPlanRep &UPR_res,
	           const GraphCopy &block,
	           UpwardPlanRep &UPR);
};

}
