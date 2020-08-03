/** \file
 * \brief Common implementations of force models for SpringEmbedder algorithms
 *
 * \author Carsten Gutwenger, Stephan Beyer
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

#include <ogdf/basic/basic.h>
#include <ogdf/basic/geometry.h>

namespace ogdf {
namespace spring_embedder {

template<typename NodeInfo>
class CommonForceModelBase
{
public:
	CommonForceModelBase(const Array<NodeInfo> &vInfo, const Array<int> &adjLists, double idealEdgeLength) :
	  m_vInfo(vInfo),
	  m_adjLists(adjLists),
	  m_idealEdgeLength(idealEdgeLength)
	{
	}

	double eps() const { return 0.01*m_idealEdgeLength; }

protected:
	const Array<NodeInfo> &m_vInfo;
	const Array<int>      &m_adjLists;

	double m_idealEdgeLength;

	double normByIdealEdgeLength(double norm) const
	{
		return (norm + eps()) / (m_idealEdgeLength + eps());
	}

	DPoint computeFruchtermanReingoldAttractiveForce(int j, int idealExponent) const
	{
		const NodeInfo &vj = m_vInfo[j];

		// attractive forces on j: F_attr(d) = -d^2 / iel
		DPoint force(0, 0);
		for (int i = vj.m_adjBegin; i != vj.m_adjStop; ++i) {
			int u = m_adjLists[i];

			DPoint dist = vj.m_pos - m_vInfo[u].m_pos;
			double d = dist.norm();

			dist *= d;
			force -= dist;
		}

		force /= std::pow(m_idealEdgeLength, idealExponent);

		return force;
	}
};

}
}
