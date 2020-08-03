/** \file
 * \brief Declaration of class PlanarSubgraphPQTree.
 *
 * Datastructure used by the planarization module FastPlanarSubgraph.
 * Derived class of MaxSequencePQTree. Implements an Interface
 * of MaxSequencePQTree for the planarization module FastPlanarSubgraph.
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
#include <ogdf/planarity/planar_subgraph_fast/MaxSequencePQTree.h>

namespace ogdf {

class OGDF_EXPORT PlanarSubgraphPQTree: public MaxSequencePQTree<edge,bool> {
public:
	using PlanarLeafKey = booth_lueker::PlanarLeafKey<whaInfo*>;

	PlanarSubgraphPQTree() : MaxSequencePQTree<edge,bool>() { }

	virtual ~PlanarSubgraphPQTree() { }

	//! Initializes a new PQ-tree with a set of leaves.
	virtual int Initialize(SListPure<PlanarLeafKey*> &leafKeys);

	int Initialize(SListPure<PQLeafKey<edge,whaInfo*,bool>*> &leafKeys) override {
		return MaxSequencePQTree<edge,bool>::Initialize(leafKeys);
	}

	//! Replaces the pertinent subtree by a set of new leaves.
	void ReplaceRoot(SListPure<PlanarLeafKey*> &leafKeys);

	//! Reduces a set of leaves.
	virtual bool Reduction(
		SListPure<PlanarLeafKey*> &leafKeys,
		SList<PQLeafKey<edge,whaInfo*,bool>*> &eliminatedKeys);

	bool Reduction(SListPure<PQLeafKey<edge,whaInfo*,bool>*> &leafKeys) override {
		return MaxSequencePQTree<edge,bool>::Reduction(leafKeys);
	}

private:
	//! Replaces a pertinet subtree by a set of new leaves if the root is full.
	void ReplaceFullRoot(SListPure<PlanarLeafKey*> &leafKeys);

	//! Replaces a pertinet subtree by a set of new leaves if the root is partial.
	void ReplacePartialRoot(SListPure<PlanarLeafKey*> &leafKeys);

	/**
	 * Removes the leaves that have been marked for elimination from the PQ-tree.
	 *
	 * This function handles the difficult task of cleaning up after every reduction.
	 *
	 * After a reduction is complete, different kind of garbage has to be handled.
	 *   - Pertinent leaves that are not in the maximal pertinent sequence.
	 *     from the $PQ$-tree in order to get it reducable have to be deleted.
	 *   - The memory of some pertinent nodes, that have only pertinent leaves not beeing
	 *     in the maximal pertinent sequence in their frontier has to be freed.
	 *   - Pertinent nodes that have only one child left after the removal
	 *     of pertinent leaves not beeing in the maximal pertinent sequence
	 *     have to be deleted.
	 *   - The memory of all full nodes has to be freed, since the complete
	 *     pertinent subtree is replaced by a $P$-node after the reduction.
	 *   - Nodes, that have been removed during the call of the function [[Reduce]]
	 *     of the base class template [[PQTree]] from the $PQ$-tree have to be
	 *     kept but marked as nonexisting.
	 */
	void removeEliminatedLeaves(SList<PQLeafKey<edge,whaInfo*,bool>*> &eliminatedKeys);
};

}
