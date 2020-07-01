/** \file
 * \brief Declaration of the class EmbedPQTree.
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
#include <ogdf/planarity/booth_lueker/EmbedIndicator.h>

namespace ogdf {
namespace booth_lueker {

class EmbedPQTree: public PQTree<edge,IndInfo*,bool>
{
public:

	EmbedPQTree() : PQTree<edge,IndInfo*,bool>() { }

	virtual ~EmbedPQTree() { }

	virtual void emptyAllPertinentNodes() override;

	virtual void clientDefinedEmptyNode(PQNode<edge,IndInfo*,bool>* nodePtr) override;

	virtual int Initialize(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys);

	int Initialize(SListPure<PQLeafKey<edge,IndInfo*,bool>*> &leafKeys) override {
		return PQTree<edge,IndInfo*,bool>::Initialize(leafKeys);
	}

	void ReplaceRoot(
		SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys,
		SListPure<edge> &frontier,
		SListPure<node> &opposed,
		SListPure<node> &nonOpposed,
		node v);

	virtual bool Reduction(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys);

	bool Reduction(SListPure<PQLeafKey<edge,IndInfo*,bool>*> &leafKeys) override {
		return PQTree<edge,IndInfo*,bool>::Reduction(leafKeys);
	}

	PQNode<edge,IndInfo*,bool>* scanSibLeft(PQNode<edge,IndInfo*,bool> *nodePtr) const {
		return clientSibLeft(nodePtr);
	}

	PQNode<edge,IndInfo*,bool>* scanSibRight(PQNode<edge,IndInfo*,bool> *nodePtr) const {
		return clientSibRight(nodePtr);
	}

	PQNode<edge,IndInfo*,bool>* scanLeftEndmost(PQNode<edge,IndInfo*,bool> *nodePtr) const {
		return clientLeftEndmost(nodePtr);
	}

	PQNode<edge,IndInfo*,bool>* scanRightEndmost(PQNode<edge,IndInfo*,bool> *nodePtr) const {
		return clientRightEndmost(nodePtr);
	}

	PQNode<edge,IndInfo*,bool>* scanNextSib(
		PQNode<edge,IndInfo*,bool> *nodePtr,
		PQNode<edge,IndInfo*,bool> *other) {
			return clientNextSib(nodePtr,other);
	}

	virtual void getFront(
		PQNode<edge,IndInfo*,bool>* nodePtr,
		SListPure<PQBasicKey<edge,IndInfo*,bool>*> &leafKeys);

protected:

	virtual PQNode<edge,IndInfo*,bool>*
		clientSibLeft(PQNode<edge,IndInfo*,bool> *nodePtr) const override;

	virtual PQNode<edge,IndInfo*,bool>*
		clientSibRight(PQNode<edge,IndInfo*,bool> *nodePtr) const override;

	virtual PQNode<edge,IndInfo*,bool>*
		clientLeftEndmost(PQNode<edge,IndInfo*,bool> *nodePtr) const override;

	virtual PQNode<edge,IndInfo*,bool>*
		clientRightEndmost(PQNode<edge,IndInfo*,bool> *nodePtr) const override;

	virtual PQNode<edge,IndInfo*,bool>*
		clientNextSib(PQNode<edge,IndInfo*,bool> *nodePtr,
		PQNode<edge,IndInfo*,bool> *other) const override;
	virtual const char*
		clientPrintStatus(PQNode<edge,IndInfo*,bool> *nodePtr) override;

	virtual void front(
		PQNode<edge,IndInfo*,bool>* nodePtr,
		SListPure<PQBasicKey<edge,IndInfo*,bool>*> &leafKeys);

	void front(
		PQNode<edge,IndInfo*,bool>* nodePtr,
		SListPure<PQLeafKey<edge,IndInfo*,bool>*> &leafKeys) override
	{
		PQTree<edge,IndInfo*,bool>::front(nodePtr, leafKeys);
	}

private:

	void ReplaceFullRoot(
		SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys,
		SListPure<PQBasicKey<edge,IndInfo*,bool>*> &frontier,
		node v,
		bool addIndicator = false,
		PQNode<edge,IndInfo*,bool> *opposite = nullptr);

	void ReplacePartialRoot(
		SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys,
		SListPure<PQBasicKey<edge,IndInfo*,bool>*> &frontier,
		node v);
};

}
}
