/** \file
 * \brief Implementations of force-models for Spring-Embedder algorithm
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

#include <ogdf/energybased/spring_embedder/SEGV_ForceModel.h>

namespace ogdf {

DPoint SpringEmbedderGridVariant::ForceModelBase::
computeRepulsiveForce(int j, double boxLength, int idealExponent, int normExponent) const
{
	const NodeInfo &vj = m_vInfo[j];
	int grid_x = vj.m_gridX;
	int grid_y = vj.m_gridY;

	// repulsive forces on node j: F_rep(d) = iel^2 / d^normExponent
	DPoint force(0, 0);
	for (int gi = -1; gi <= 1; ++gi) {
		for (int gj = -1; gj <= 1; ++gj) {
			for (int u : m_gridCell(grid_x + gi, grid_y + gj)) {
				if (u == j) {
					continue;
				}
				DPoint dist = vj.m_pos - m_vInfo[u].m_pos;
				double d = dist.norm();

				if(d < boxLength) {
					dist /= std::pow(d, normExponent+1) + eps();
					force += dist;
				}
			}
		}
	}

	force *= std::pow(m_idealEdgeLength, idealExponent);

	return force;
}

DPoint SpringEmbedderGridVariant::ForceModelBase::
computeMixedForcesDisplacement(int j, int boxLength,
                               std::function<DPoint(double, const DPoint &)> attractiveChange,
                               std::function<double()> attractiveFinal) const
{
	DPoint disp(computeRepulsiveForce(j, boxLength, 2));

	const NodeInfo &vj = m_vInfo[j];

	DPoint forceAttr(0, 0);
	DPoint forceRep(0, 0); // subtract rep. force on adjacent vertices
	for (int i = vj.m_adjBegin; i != vj.m_adjStop; ++i) {
		int u = m_adjLists[i];

		DPoint dist = vj.m_pos - m_vInfo[u].m_pos;
		double d = dist.norm();

		forceAttr -= attractiveChange(d, dist);
		if (d < boxLength) {
			double f = 1.0 / (d * d + eps());
			forceRep += f * dist;
		}
	}

	forceAttr *= attractiveFinal();
	forceRep *= m_idealEdgeLength * m_idealEdgeLength;

	disp += forceAttr - forceRep;
	return disp;
}

// Fruchterman / Reingold
DPoint SpringEmbedderGridVariant::ForceModelFR::computeDisplacement(int j, double boxLength) const
{
	return computeRepulsiveForce(j, boxLength, 2) + computeFruchtermanReingoldAttractiveForce(j, 1);
}

// Fruchterman / Reingold with modified attractive forces
DPoint SpringEmbedderGridVariant::ForceModelFRModAttr::computeDisplacement(int j, double boxLength) const
{
	return computeRepulsiveForce(j, boxLength, 3) + computeFruchtermanReingoldAttractiveForce(j, 1);
}

// Fruchterman / Reingold with modified repulsive
DPoint SpringEmbedderGridVariant::ForceModelFRModRep::computeDisplacement(int j, double boxLength) const
{
	return computeRepulsiveForce(j, boxLength, 2, 2) + computeFruchtermanReingoldAttractiveForce(j, 2);
}

// Eades
DPoint SpringEmbedderGridVariant::ForceModelEades::computeDisplacement(int j, double boxLength) const
{
	return computeMixedForcesDisplacement(j, boxLength, [=](double d, const DPoint &dist) {
		// attractive forces on j: F_attr(d) = -c iel log_2(d/iel)
		return std::log2(normByIdealEdgeLength(d)) * dist;
	}, [=] {
		return 0.1 * m_idealEdgeLength;
	});
}

// Hachul (new method)
DPoint SpringEmbedderGridVariant::ForceModelHachul::computeDisplacement(int j, double boxLength) const
{
	return computeMixedForcesDisplacement(j, boxLength, [=](double d, const DPoint &dist) {
		// attractive forces on j: F_attr(d) = -d^2 log_2(d/iel) / iel
		return d * std::log2(normByIdealEdgeLength(d)) * dist;
	}, [=] {
		return 1.0 / m_idealEdgeLength;
	});
}

// Gronemann
DPoint SpringEmbedderGridVariant::ForceModelGronemann::computeDisplacement(int j, double boxLength) const
{
	return computeMixedForcesDisplacement(j, boxLength, [=](double d, const DPoint &dist) {
		// attractive forces on j: F_attr(d) = c / deg(v) * ln(d/iel)
		return log(normByIdealEdgeLength(d)) * dist;
	}, [=] {
		return 0.5 * (m_vInfo[j].m_adjStop - m_vInfo[j].m_adjBegin);
	});
}

}
