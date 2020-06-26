/** \file
 * \brief Declaration and implementation of the class PQNodeRoot.
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

#include <ogdf/basic/basic.h>
#include <ogdf/basic/memory.h>

namespace ogdf {

/**
 * The class PQNodeRoot is used as a base class of the class
 * PQNode. Using the class PQNodeRoot, a user may
 * refer to a node without the class structure.
 */
class PQNodeRoot {
public:
	enum class PQNodeType { PNode = 1, QNode = 2, Leaf = 3, Undefined = 0 };

	enum class SibDirection { NoDir, Left, Right };

	// Status Definitions
	enum class PQNodeStatus {
		Empty       = 1,
		Partial     = 2,
		Full        = 3,
		Pertinent   = 4,
		ToBeDeleted = 5,

		//! Indicator for extra node status defines
		Indicator   = 6,
		//! Nodes removed during the template reduction are marked as
		//! as Eliminated. Their memory is not freed. They are kept
		//! for parent pointer update.
		Eliminated  = 6,
		//! Nodes that need to be removed in order to obtain a
		//! maximal pertinent sequence are marked WhaDelete.
		WhaDelete  = 7,
		//! The pertinent Root is marked PertRoot during the clean up
		//! after a reduction. Technical.
		PertRoot    = 8
	};

	// Mark Definitions for Bubble Phase
	enum class PQNodeMark { Unmarked = 0, Queued = 1, Blocked = 2, Unblocked = 3 };


	PQNodeRoot() { }
	virtual ~PQNodeRoot() { }

	OGDF_NEW_DELETE
};

}
