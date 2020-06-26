/** \file
 * \brief Declaration and implementation of the optimal third
 *        phase of the Sugiyama algorithm.
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

#include <ogdf/layered/HierarchyLayoutModule.h>

namespace ogdf {


//! The LP-based hierarchy layout algorithm.
/**
 * @ingroup gd-hlm
 *
 * OptimalHierarchyLayout implements a hierarchy layout algorithm that is based
 * on an LP-formulation. It is only available if OGDF is compiled with LP-solver
 * support (e.g., Coin).
 *
 * The used model avoids Spaghetti-effect like routing of edges by using
 * long vertical segments as in FastHierarchyLayout. An additional balancing
 * can be used which balances the successors below a node.
 *
 * <H3>Optional parameters</H3>
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>nodeDistance</i><td>double<td>3.0
 *     <td>The minimal allowed x-distance between nodes on a layer.
 *   </tr><tr>
 *     <td><i>layerDistance</i><td>double<td>3.0
 *     <td>The minimal allowed y-distance between layers.
 *   </tr><tr>
 *     <td><i>fixedLayerDistance</i><td>bool<td>false
 *     <td>If set to true, the distance between neighboured layers is always
 *     layerDistance; otherwise the distance is adjusted (increased) to improve readability.
 *   </tr><tr>
 *     <td><i>weightSegments</i><td>double<td>2.0
 *     <td>The weight of edge segments connecting to vertical segments.
 *   </tr><tr>
 *     <td><i>weightBalancing</i><td>double<td>0.1
 *     <td>The weight for balancing successors below a node; 0.0 means no balancing.
 *   </tr>
 * </table>
 */
class OGDF_EXPORT OptimalHierarchyLayout : public HierarchyLayoutModule
{
public:
	//! Creates an instance of optimal hierarchy layout.
	OptimalHierarchyLayout();

	//! Copy constructor.
	OptimalHierarchyLayout(const OptimalHierarchyLayout &);

	// destructor
	~OptimalHierarchyLayout() { }


	//! Assignment operator.
	OptimalHierarchyLayout &operator=(const OptimalHierarchyLayout &);


	/**
	 *  @name Optional parameters
	 *  @{
	 */

	//! Returns the minimal allowed x-distance between nodes on a layer.
	double nodeDistance() const {
		return m_nodeDistance;
	}

	//! Sets the minimal allowed x-distance between nodes on a layer to \p x.
	void nodeDistance(double x) {
		if(x >= 0)
			m_nodeDistance = x;
	}

	//! Returns the minimal allowed y-distance between layers.
	double layerDistance() const {
		return m_layerDistance;
	}

	//! Sets the minimal allowed y-distance between layers to \p x.
	void layerDistance(double x) {
		if(x >= 0)
			m_layerDistance = x;
	}

	//! Returns the current setting of option <i>fixedLayerDistance</i>.
	/**
	 * If set to true, the distance is always layerDistance; otherwise
	 * the distance is adjusted (increased) to improve readability.
	 */
	bool fixedLayerDistance() const {
		return m_fixedLayerDistance;
	}

	//! Sets the option <i>fixedLayerDistance</i> to \p b.
	void fixedLayerDistance(bool b) {
		m_fixedLayerDistance = b;
	}

	//! Returns the weight of edge segments connecting to vertical segments.
	double weightSegments() const {
		return m_weightSegments;
	}

	//! Sets the weight of edge segments connecting to vertical segments to \p w.
	void weightSegments(double w) {
		if(w > 0.0 && w <= 100.0)
			m_weightSegments = w;
	}

	//! Returns the weight for balancing successors below a node; 0.0 means no balancing.
	double weightBalancing() const {
		return m_weightBalancing;
	}

	//! Sets the weight for balancing successors below a node to \p w; 0.0 means no balancing.
	void weightBalancing(double w) {
		if(w >= 0.0 && w <= 100.0)
			m_weightBalancing = w;
	}

	//! @}

protected:
	//! Implements the algorithm call.
	virtual void doCall(const HierarchyLevelsBase &levels,GraphAttributes &AGC) override;

private:
	void computeXCoordinates(
		const HierarchyLevelsBase &levels,
		GraphAttributes &AGC);
	void computeYCoordinates(
		const HierarchyLevelsBase &levels,
		GraphAttributes &AGC);

	// options
	double m_nodeDistance;  //!< The minimal distance between nodes.
	double m_layerDistance; //!< The minimal distance between layers.
	bool   m_fixedLayerDistance; //!< Use fixed layer distances?

	double m_weightSegments;  //!< The weight of edge segments.
	double m_weightBalancing; //!< The weight for balancing.
};

}
