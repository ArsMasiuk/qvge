/** \file
 * \brief Implementation of EdgeComparerSimple.
 *
 * \author Bernd Zey
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


#include <ogdf/basic/EdgeComparerSimple.h>

namespace ogdf {

int EdgeComparerSimple::compare(const adjEntry &e1, const adjEntry &e2) const
{
	auto findPoints = [&](const adjEntry a, DPoint& p) {
		DPolyline poly = m_AG->bends(a->theEdge());
		if (m_useBends && poly.size() > 2) {
			ListIterator<DPoint> it;
			if (a->theEdge()->source() == m_basis) {
				it = poly.begin();
				++it;
			} else {
				it = poly.rbegin();
				--it;
			}
			p = *it;
		} else {
			p.m_x = m_AG->x(a->twinNode());
			p.m_y = m_AG->y(a->twinNode());
		}
	};

	DPoint pE1, pE2;
	findPoints(e1, pE1);
	findPoints(e2, pE2);

	double xP1 = -m_AG->x(m_basis) + pE1.m_x;
	double yP1 = -m_AG->y(m_basis) + pE1.m_y;
	double xP2 = -m_AG->x(m_basis) + pE2.m_x;
	double yP2 = -m_AG->y(m_basis) + pE2.m_y;

	auto bothYSameSide = [](double x1, double x2, double y1, double y2) -> int {
		if (x1 >= 0 && x2 < 0) { return -1; }
		if (x1 < 0 && x2 >= 0) { return 1; }
		x1 /= sqrt(x1*x1 + y1*y1);
		x2 /= sqrt(x2*x2 + y2*y2);
		if (x1 > x2) { return -1; }
		else { return 1; }
	};

	if (yP1 >= 0 && yP2 < 0) { return 1; }
	if (yP1 < 0 && yP2 >= 0) { return -1; }
	if (yP1 >= 0 && yP2 >= 0) { return bothYSameSide(xP1, xP2, yP1, yP2); }
	if (yP1 < 0 && yP2 < 0) { return -bothYSameSide(xP1, xP2, yP1, yP2); }

	return 0;
}

}
