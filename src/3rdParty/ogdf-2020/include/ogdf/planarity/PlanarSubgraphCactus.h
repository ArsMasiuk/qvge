/** \file
 * \brief Declaration of class PlanarSubgraphCactus.
 *
 * \author Jöran Schierbaum
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

#include <ogdf/planarity/PlanarSubgraphTriangles.h>

namespace ogdf {

//! Maximum planar subgraph approximation algorithm by Calinescu et al.
/**
 * @ingroup ga-plansub
 *
 * The algorithm has an approximation factor of 7/18.
 * Setting preferred edges is not supported.
 * Weighted edges are heuristically respected but there is no approximation guarantee in the weighted case.
 */
template<typename TCost>
class PlanarSubgraphCactus : public PlanarSubgraphTriangles<TCost>
{
public:
	PlanarSubgraphCactus() : PlanarSubgraphTriangles<TCost>(true) { }

	virtual PlanarSubgraphCactus *clone() const override {
		return new PlanarSubgraphCactus();
	}
};

}
