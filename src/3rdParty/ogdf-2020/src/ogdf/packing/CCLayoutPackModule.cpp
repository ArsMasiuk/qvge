/** \file
 * \brief implementation of class CCLayoutPackModule.
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

#include <ogdf/packing/CCLayoutPackModule.h>

namespace ogdf {

template<class POINT>
bool CCLayoutPackModule::checkOffsetsTP(
	const Array<POINT> &box,
	const Array<POINT> &offset)
{
	OGDF_ASSERT(box.size() == offset.size());
	const int n = box.size();

	for (int i = 0; i < n; ++i)
	{
		auto xl = offset[i].m_x;
		auto xr = xl + box[i].m_x;
		auto yb = offset[i].m_y;
		auto yt = yb + box[i].m_y;

		OGDF_ASSERT(xl <= xr);
		OGDF_ASSERT(yb <= yt);

		for (int j = i+1; j < n; ++j)
		{
			auto xl2 = offset[j].m_x;
			auto xr2 = xl2 + box[j].m_x;
			auto yb2 = offset[j].m_y;
			auto yt2 = yb2 + box[j].m_y;

			if (xr2 > xl && xl2 < xr && yt2 > yb && yb2 < yt)
				return false;
		}
	}

	return true;
}

bool CCLayoutPackModule::checkOffsets(const Array<DPoint> &box,
	const Array<DPoint> &offset)
{
	return checkOffsetsTP(box,offset);
}

bool CCLayoutPackModule::checkOffsets(const Array<IPoint> &box,
	const Array<IPoint> &offset)
{
	return checkOffsetsTP(box,offset);
}

}
