/** \file
 * \brief Declaration of interface for two-layer crossing
 *        minimization algorithms.
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

#include <ogdf/layered/Hierarchy.h>
#include <ogdf/layered/CrossingMinInterfaces.h>

namespace ogdf {

class SugiyamaLayout;

/**
 * \brief Interface of crossing minimization algorithms for layered graphs.
 *
 * The interface of a two-layer crossing minimization algorithm consists of
 * two methods:
 *   -# virtual const HierarchyLevelsBase *reduceCrossings(const SugiyamaLayout sugi,
 * Hierarchy &H) performs crossing minimization on layered graph H as a part
 * of Sugiyama algorithm instance sugi.
 *   -# cleanup() has to be called last and performs some final clean-up work.
 */


class OGDF_EXPORT LayeredCrossMinModule {
public:

	//! Creates empty module.
	LayeredCrossMinModule() { }

	//! Destruct.
	virtual ~LayeredCrossMinModule() { }

	//! Calls the actual crossing minimization algorithm.
	virtual const HierarchyLevelsBase *reduceCrossings(const SugiyamaLayout &sugi, Hierarchy &H, int &nCrossings) = 0;

	//! Performs clean-up.
	virtual void cleanup() { }

	OGDF_MALLOC_NEW_DELETE
};

}
