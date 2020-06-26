/** \file
 * \brief Declaration of planarization with grid layout.
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

#include <ogdf/planarlayout/GridLayoutModule.h>
#include <memory>
#include <ogdf/planarity/CrossingMinimizationModule.h>
#include <ogdf/packing/CCLayoutPackModule.h>

namespace ogdf {

/**
 * \brief The planarization grid layout algorithm.
 *
 * @ingroup gd-planlayout
 *
 * The class PlanarizationGridLayout represents a customizable implementation
 * of the planarization approach for drawing graphs. The class uses a
 * planar grid layout algorithm as a subroutine and allows to generate
 * a usual layout or a grid layout.
 *
 * If the planarization layout algorithm shall be used for simultaneous drawing,
 * you need to define the different subgraphs by setting the <i>subgraphs</i>
 * option.
 *
 * The implementation used in PlanarizationGridLayout is based on the following
 * publication:
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
 *     <td><i>pageRatio</i><td>double<td>1.0
 *     <td>Specifies the desired ration of width / height of the computed
 *     layout. It is currently only used when packing connected components.
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
 *     <td><i>crossMin</i><td>CrossingMinimizationModule<td>SubgraphPlanarizer
 *     <td>The module used for the crossing minimization step.
 *   </tr><tr>
 *     <td><i>planarLayouter</i><td>GridLayoutPlanRepModule<td>MixedModelLayout
 *     <td>The planar layout algorithm used to compute a planar layout
 *     of the planarized representation resulting from the crossing minimization step.
 *   </tr><tr>
 *     <td><i>packer</i><td>CCLayoutPackModule<td>TileToRowsCCPacker
 *     <td>The packer module used for arranging connected components.
 *   </tr>
 * </table>
 */
class OGDF_EXPORT PlanarizationGridLayout : public GridLayoutModule
{
public:
	//! Creates an instance of planarization layout and sets options to default values.
	PlanarizationGridLayout();

	~PlanarizationGridLayout() { }

	/**
	 *  @name Optional parameters
	 *  @{
	 */

	/**
	 * \brief Returns the current setting of option pageRatio.
	 *
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

	/** @}
	 *  @name Module options
	 *  @{
	 */

	//! Sets the module option for crossing minimization.
	void setCrossMin(CrossingMinimizationModule *pCrossMin) {
		m_crossMin.reset(pCrossMin);
	}


	/**
	 * \brief Sets the module option for the planar grid layout algorithm.
	 *
	 * The planar layout algorithm is used to compute a planar layout
	 * of the planarized representation resulting from the crossing
	 * minimization step. Planarized representation means that edge crossings
	 * are replaced by dummy nodes of degree four, so the actual layout
	 * algorithm obtains a planar graph as input. By default, the planar
	 * layout algorithm produces an orthogonal drawing.
	 */
	void setPlanarLayouter(GridLayoutPlanRepModule *pPlanarLayouter) {
		m_planarLayouter.reset(pPlanarLayouter);
	}

	/**
	 * \brief Sets the module option for the arrangement of connected components.
	 *
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

	//! Returns the number of crossings in computed layout.
	int numberOfCrossings() const {
		return m_nCrossings;
	}

	//! @}

protected:
	virtual void doCall(const Graph &G, GridLayout &gridLayout, IPoint &boundingBox) override;


private:
	//! The module for computing a planar subgraph.
	std::unique_ptr<CrossingMinimizationModule> m_crossMin;

	//! The module for computing a planar grid layout.
	std::unique_ptr<GridLayoutPlanRepModule> m_planarLayouter;

	//! The module for arranging connected components.
	std::unique_ptr<CCLayoutPackModule>      m_packer;

	double m_pageRatio; //!< The desired page ratio.

	int m_nCrossings; //!< The number of crossings in the computed layout.
};

}
