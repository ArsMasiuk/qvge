/** \file
 * \brief Declaration of interfaces used in Sugiyama framework.
 *
 * \author Carsten Gutwenger, Paweł Schmidt
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
#include <ogdf/layered/Hierarchy.h>


namespace ogdf {

//! Representation of levels in hierarchies.
/**
 * \see Hierarchy, SugiyamaLayout
 */
class LevelBase
{
public:
	//  destruction
	virtual ~LevelBase() { }

	//! Returns the node at position \p i.
	virtual const node &operator[](int i) const = 0;

	//! Returns the node at position \p i.
	virtual node &operator[](int i) = 0;

	//! Returns the number of nodes on this level.
	virtual int size() const = 0;

	//! Returns the maximal array index (= size()-1).
	virtual int high() const = 0;
};



class OGDF_EXPORT HierarchyLevelsBase {

public:
	HierarchyLevelsBase() = default;

	HierarchyLevelsBase(const HierarchyLevelsBase&) = default;
	HierarchyLevelsBase& operator=(const HierarchyLevelsBase&) = default;

	// destruction
	virtual ~HierarchyLevelsBase() { }

	enum class TraversingDir { downward, upward };

	//! Returns the <i>i</i>-th level.
	virtual const LevelBase &operator[](int i) const = 0;

	//! Returns the position of node \p v on its level.
	virtual int pos(node v) const = 0;

	//! Returns the number of levels.
	virtual int size() const = 0;

	//! Returns the maximal array index of a level (= size()-1).
	virtual int high() const { return size() - 1; }

	virtual const Hierarchy &hierarchy() const = 0;

	//! Returns the adjacent nodes of \p v.
	virtual const Array<node> &adjNodes(node v, TraversingDir dir) const = 0;

	//! Computes the number of crossings between level \p i and \p i+1.
	int calculateCrossings(int i) const;

	//! Computes the total number of crossings.
	int calculateCrossings() const;
};

}
