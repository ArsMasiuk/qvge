/** \file
 * \brief Declaration of ClusterPlanarModule which implements a c-planarity
 *        test.
 *
 * \author Karsten Klein
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

#include <ogdf/basic/Module.h>
#include <ogdf/cluster/ClusterGraph.h>

namespace ogdf {

class OGDF_EXPORT ClusterPlanarModule : public Module {
public:
	ClusterPlanarModule() { }
	virtual ~ClusterPlanarModule() { }

	//! Returns true, if CG is c-planar, false otherwise.
	virtual bool isClusterPlanar(const ClusterGraph &CG) {
		return doTest(CG);
	}

protected:
	//! Performs a c-planarity test on CG.
	virtual bool doTest(const ClusterGraph &CG) = 0;

	OGDF_MALLOC_NEW_DELETE
};

}
