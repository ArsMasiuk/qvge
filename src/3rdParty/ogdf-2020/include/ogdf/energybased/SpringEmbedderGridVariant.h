/** \file
 * \brief Declaration of ogdf::SpringEmbedderGridVariant.
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

#include <ogdf/energybased/spring_embedder/SpringEmbedderBase.h>
#include <ogdf/basic/Array2D.h>
#include <ogdf/energybased/SpringForceModel.h>
#include <ogdf/basic/GraphAttributes.h>

namespace ogdf {

//! The spring-embedder layout algorithm with force approximation using hte grid variant approach.
/**
 * @ingroup gd-energy
 *
 * The implementation used in SpringEmbedderGridVariant is based on
 * the following publication:
 *
 * Thomas M. J. Fruchterman, Edward M. Reingold: <i>%Graph Drawing by Force-directed
 * Placement</i>. Software - Practice and Experience 21(11), pp. 1129-1164, 1991.
 *
 * <H3>Optional parameters</H3>
 * Fruchterman/Reingold layout provides the following optional parameters.
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>iterations</i><td>int<td>400
 *     <td>The number of iterations performed in the optimization.
 *   </tr><tr>
 *     <td><i>noise</i><td>bool<td>true
 *     <td>If set to true, (small) random perturbations are performed.
 *   </tr><tr>
 *     <td><i>minDistCC</i><td>double<td>20.0
 *     <td>The minimum distance between connected components.
 *   </tr><tr>
 *     <td><i>pageRatio</i><td>double<td>1.0
 *     <td>The page ratio.
 *   </tr><tr>
 *     <td><i>scaling</i><td> #Scaling <td> Scaling::scaleFunction
 *     <td>The scaling method for scaling the inital layout.
 *   </tr><tr>
 *     <td><i>scaleFunctionFactor</i><td>double<td>8.0
 *     <td>The scale function factor (used if scaling = scaleFunction).
 *   </tr><tr>
 *     <td><i>userBoundingBox</i><td>rectangle<td>(0.0,100.0,0.0,100.0)
 *     <td>The user bounding box for scaling (used if scaling = scUserBoundingBox).
 *   </tr>
 * </table>
 */
class OGDF_EXPORT SpringEmbedderGridVariant : public spring_embedder::SpringEmbedderBase
{
public:
	SpringEmbedderGridVariant() {
		m_forceLimitStep = .5;
	}

protected:
	void callMaster(const GraphCopy& copy, GraphAttributes& attr, DPoint& box) override;

private:
	struct NodeInfo
	{
		DPoint m_pos;

		int m_adjBegin;
		int m_adjStop;

		int m_gridX;
		int m_gridY;

		ListIterator<int> m_lit;
	};

	class ForceModelBase;
	class ForceModelFR;
	class ForceModelFRModAttr;
	class ForceModelFRModRep;
	class ForceModelEades;
	class ForceModelHachul;
	class ForceModelGronemann;

	class Master;
	class Worker;
};

}
