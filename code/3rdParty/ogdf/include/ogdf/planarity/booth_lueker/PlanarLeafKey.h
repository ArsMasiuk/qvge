/** \file
 * \brief Declaration of class PlanarLeafKey.
 *
 * \author Sebastian Leipert
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

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/pqtree/PQLeafKey.h>

namespace ogdf {
namespace booth_lueker {

template<class X>
class PlanarLeafKey : public PQLeafKey<edge,X,bool>
{
public:
	explicit PlanarLeafKey(edge e) : PQLeafKey<edge,X,bool>(e) { }

	virtual ~PlanarLeafKey() { }

	std::ostream &print(std::ostream &os)
	{
		int sId = this->m_userStructKey->source()->index();
		int tId = this->m_userStructKey->target()->index();

		os << " (" << sId << "," << tId << ")";

		return os;
	}
};

}
}
