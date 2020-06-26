/** \file
 * \brief Declaration of class PlanarizationLayout.
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

#include <ogdf/basic/LayoutModule.h>
#include <ogdf/planarity/CrossingMinimizationModule.h>
#include <ogdf/planarity/EmbedderModule.h>
#include <ogdf/planarity/LayoutPlanRepModule.h>
#include <ogdf/packing/CCLayoutPackModule.h>
#include <memory>
#include <ogdf/planarity/planarization_layout/CliqueReplacer.h>

namespace ogdf {

//! The planarization approach for drawing graphs.
/**
 * @ingroup gd-planlayout
 */
class OGDF_EXPORT PlanarizationLayout : public LayoutModule
{
public:
	//! Creates an instance of planarization layout and sets options to default values.
	PlanarizationLayout();

	//! Destructor.
	~PlanarizationLayout() { }

	//! Calls planarization layout for GraphAttributes \p ga.
	/**
	 * \pre The graph has no self-loops.
	 * @param ga is the input graph and will also be assigned the layout information.
	 */
	void call(GraphAttributes &ga) override;

	//! Calls planarization layout with clique handling for GraphAttributes \p ga with associated graph \p g.
	/**
	 * \pre \p g is the graph associated with graph attributes \p ga.
	 *
	 * This call perfoms a special handling for cliques, which are temporarily replaced by a star graph.
	 * In the final drawing, the clique edges are drawn straight-line.
	 */
	void call(GraphAttributes &ga, Graph &g);

	void callSimDraw(GraphAttributes &ga);

	/** @}
	 *  @name Optional parameters
	 *  @{
	 */

	//! Returns the current setting of option pageRatio.
	/**
	 * This option specifies the desired ration width / height of the computed
	 * layout. It is currently only used for packing connected components.
	 */
	double pageRatio() const {
		return m_pageRatio;
	}

	//! Sets the option pageRatio to \p ratio.
	void pageRatio(double ratio) {
		m_pageRatio = ratio;
	}

	//! Returns the current setting of option minCliqueSize.
	/**
	 * If preprocessing of cliques is considered, this option determines the
	 * minimal size of cliques to search for.
	 */
	int minCliqueSize() const {
		return m_cliqueSize;
	}

	//! Set the option minCliqueSize to \p i.
	void minCliqueSize(int i) {
		m_cliqueSize = max(i, 3);
	}


	/** @}
	 *  @name Module options
	 *  @{
	 */

	//! Sets the module option for crossing minimization.
	void setCrossMin(CrossingMinimizationModule *pCrossMin) {
		m_crossMin.reset(pCrossMin);
	}

	//! Sets the module option for the graph embedding algorithm.
	/**
	 * The result of the crossing minimization step is a planar graph,
	 * in which crossings are replaced by dummy nodes. The embedding
	 * module then computes a planar embedding of this planar graph.
	 */
	void setEmbedder(EmbedderModule *pEmbedder) {
		m_embedder.reset(pEmbedder);
	}

	//! Sets the module option for the planar layout algorithm.
	/**
	 * The planar layout algorithm is used to compute a planar layout
	 * of the planarized representation resulting from the crossing
	 * minimization step. Planarized representation means that edge crossings
	 * are replaced by dummy nodes of degree four, so the actual layout
	 * algorithm obtains a planar graph as input. By default, the planar
	 * layout algorithm produces an orthogonal drawing.
	 */
	void setPlanarLayouter(LayoutPlanRepModule *pPlanarLayouter) {
		m_planarLayouter.reset(pPlanarLayouter);
	}

	//! Sets the module option for the arrangement of connected components.
	/**
	 * The planarization layout algorithm draws each connected component of
	 * the input graph seperately, and then arranges the resulting drawings
	 * using a packing algorithm.
	 */
	void setPacker(CCLayoutPackModule *pPacker) {
		m_packer.reset(pPacker);
	}

	/** @}
	 *  @name Further information
	 *  @{
	 */

	//! Returns the number of crossings in the computed layout.
	int numberOfCrossings() const {
		return m_nCrossings;
	}

	//! @}

private:
	using CliqueReplacer = planarization_layout::CliqueReplacer;

	void arrangeCCs(PlanRep &PG, GraphAttributes &GA, Array<DPoint> &boundingBox) const;
	void preprocessCliques(Graph &G, CliqueReplacer &cliqueReplacer);
	void fillAdjNodes(List<node>& adjNodes,
		PlanRep& PG,
		node centerNode,
		NodeArray<bool>& isClique,
		Layout& drawing);

	//! The module for computing a planar subgraph.
	std::unique_ptr<CrossingMinimizationModule> m_crossMin;

	//! The module for planar embedding.
	std::unique_ptr<EmbedderModule> m_embedder;

	//! The module for computing a planar layout.
	std::unique_ptr<LayoutPlanRepModule> m_planarLayouter;

	//! The module for arranging connected components.
	std::unique_ptr<CCLayoutPackModule> m_packer;

	double m_pageRatio;    //!< The desired page ratio.
	int m_nCrossings;      //!< The number of crossings in the computed layout.

	int m_cliqueSize;      //!< The minimum size of cliques to search for.
};

}
