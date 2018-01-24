/** \file
 * \brief Declaration of the class EmbedIndicator.
 *
 * Implements the direction Indicator. Used by class EmbedPQTree.
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

#include <ogdf/basic/pqtree/PQNode.h>
#include <ogdf/basic/pqtree/PQNodeKey.h>
#include <ogdf/basic/pqtree/PQInternalKey.h>
#include <ogdf/planarity/booth_lueker/IndInfo.h>

namespace ogdf {
namespace booth_lueker {

class EmbedIndicator : public PQNode<edge,IndInfo*,bool>
{
public:

	EmbedIndicator(int count, PQNodeKey<edge,IndInfo*,bool>* infoPtr)
		: PQNode<edge,IndInfo*,bool>(count,infoPtr) { }

	virtual ~EmbedIndicator() {
		delete getNodeInfo()->userStructInfo();
		delete getNodeInfo();
	}

	PQNodeType  type() const override { return PQNodeType::Leaf; }

	void type(PQNodeType) override { }

	PQNodeStatus  status() const override { return PQNodeRoot::PQNodeStatus::Indicator; }

	void status(PQNodeStatus) override { }

	PQNodeMark mark() const override { return PQNodeMark::Unmarked; }

	void mark(PQNodeMark) override { }

	PQLeafKey<edge,IndInfo*,bool>* getKey() const override { return nullptr; }

	bool setKey(PQLeafKey<edge,IndInfo*,bool>* pointerToKey) override {
		return pointerToKey == nullptr;
	}

	PQInternalKey<edge,IndInfo*,bool>* getInternal() const override { return nullptr; }

	bool setInternal(PQInternalKey<edge,IndInfo*,bool>* pointerToInternal) override {
		return pointerToInternal == nullptr;
	}
};

}
}
