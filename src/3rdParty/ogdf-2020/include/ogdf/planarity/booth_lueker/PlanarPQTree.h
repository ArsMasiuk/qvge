/** \file
 * \brief Declaration of class PlanarPQTree.
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
#include <ogdf/basic/SList.h>
#include <ogdf/basic/PQTree.h>
#include <ogdf/planarity/booth_lueker/PlanarLeafKey.h>
#include <ogdf/planarity/booth_lueker/IndInfo.h>

namespace ogdf {
namespace booth_lueker {

class PlanarPQTree: public PQTree<edge,IndInfo*,bool> {
public:
	PlanarPQTree() : PQTree<edge,IndInfo*,bool>() { }

	virtual ~PlanarPQTree() { }

	//! Does a clean up after a reduction.
	virtual void emptyAllPertinentNodes() override;

	//! Initializes a new PQ-tree with a set of leaves.
	virtual int Initialize(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys);

	int Initialize(SListPure<PQLeafKey<edge,IndInfo*,bool>*> &leafKeys) override {
		return PQTree<edge,IndInfo*,bool>::Initialize(leafKeys);
	}

	//! Replaces the pertinent subtree by a set of new leaves.
	void ReplaceRoot(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys);

	//! Reduces a set of leaves.
	virtual bool Reduction(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys);

	bool Reduction(SListPure<PQLeafKey<edge,IndInfo*,bool>*> &leafKeys) override {
		return PQTree<edge,IndInfo*,bool>::Reduction(leafKeys);
	}

private:

	//! Replaces a pertinet subtree by a set of new leaves if the root is full.
	void ReplaceFullRoot(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys);

	//! Replaces a pertinet subtree by a set of new leaves if the root is partial.
	void ReplacePartialRoot(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys);
};

}
}
