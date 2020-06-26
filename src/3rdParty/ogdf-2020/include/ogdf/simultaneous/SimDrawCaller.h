/** \file
 * \brief Offers variety of possible algorithm calls for simultaneous
 * drawing.
 *
 * \author Michael Schulz
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

#include <ogdf/simultaneous/SimDrawManipulatorModule.h>

namespace ogdf
{

//! Calls modified algorithms for simdraw instances
/**
 *  Runs special algorithms suitable for simultaneous drawing
 *  on current SimDraw instance. The algorithms take
 *  care of all necessary GraphAttributes activations and
 *  take over calculated coordinates and dummy nodes.
 *
 *  A typical use of SimDrawCaller involves a predefined SimDraw
 *  instance on which SimDrawCaller works.
 *  \code
 *  SimDraw SD;
 *  ...
 *  SimDrawCaller SDC(SD);
 *  SDC.callSubgraphPlanarizer();
 *  \endcode
 */
class OGDF_EXPORT SimDrawCaller : public SimDrawManipulatorModule
{

private:
	EdgeArray<uint32_t> *m_esg;     //!< saves edgeSubGraphs data

	//! updates m_esg
	/**
	*  Should be called whenever graph changed and current
	*  basic graph membership is needed.
	*/
	void updateESG();

public:
	//! constructor
	explicit SimDrawCaller(SimDraw &SD);

	//! runs SugiyamaLayout with modified SplitHeuristic
	/**
	*  Runs special call of SugiyamaLayout using
	*  SugiyamaLayout::setSubgraphs().
	*  Saves node coordinates and dummy node bends in current
	*  simdraw instance.
	*
	*  Uses TwoLayerCrossMinSimDraw object to perform crossing
	*  minimization. The default is SplitHeuristic.
	*
	*  Automatically activates GraphAttributes::nodeGraphics.\n
	*  Automatically activates GraphAttributes::edgeGraphics.
	*/
	void callSugiyamaLayout();

	//! runs UMLPlanarizationLayout with modified inserter
	/**
	*  Runs UMLPlanarizationLayout with callSimDraw and retransfers
	*  node coordinates and dummy node bend to current simdraw
	*  instance.
	*
	*  Automatically activates GraphAttributes::nodeGraphics.\n
	*  Automatically activates GraphAttributes::edgeGraphics.
	*/
	void callPlanarizationLayout();

	//! runs SubgraphPlanarizer with modified inserter
	/**
	*  Runs SubgraphPlanarizer on connected component \p cc with simdraw
	*  call. Integer edge costs of GraphAttributes are used
	*  (1 for each edge if not available).
	*
	*  Modifies graph by inserting dummy nodes for each crossing.
	*  All dummy nodes are marked as dummy.
	*  (Method SimDrawColorizer::addColorNodeVersion is recommended
	*  for visualizing dummy nodes.)
	*
	*  No layout is calculated. The result is a planar graph.
	*/
	int callSubgraphPlanarizer(int cc = 0, int numberOfPermutations = 1);
};

}
