/** \file
 * \brief Declaration of HierarchyLevels class.
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

#include <ogdf/basic/EdgeArray.h>
#include <ogdf/layered/Level.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/layered/Hierarchy.h>
#include <ogdf/layered/CrossingMinInterfaces.h>

namespace ogdf {

//! Representation of proper hierarchies used by Sugiyama-layout.
/**
 * \see Level, SugiyamaLayout
 */
class OGDF_EXPORT HierarchyLevels : public HierarchyLevelsBase {

public:

	friend class Level;
	friend class LayerBasedUPRLayout;

private:
	const Hierarchy &m_H;

	Array<Level *> m_pLevel; //!< The array of all levels.
	NodeArray<int> m_pos;    //!< The position of a node on its level.

	NodeArray<Array<node> > m_lowerAdjNodes; //!< (Sorted) adjacent nodes on lower level.
	NodeArray<Array<node> > m_upperAdjNodes; //!< (Sorted) adjacent nodes on upper level.

	NodeArray<int> m_nSet; //!< (Only used by buildAdjNodes().)

	TraversingDir m_direction; //!< The current direction of layer-by-layer sweep.

public:
	explicit HierarchyLevels(const Hierarchy &H);
	~HierarchyLevels();

	const Hierarchy &hierarchy() const override { return m_H; }

	//! Returns the current direction of layer-by-layer sweep.
	TraversingDir direction() const {
		return m_direction;
	}

	//! Sets the current direction of layer-by-layer sweep.
	void direction (TraversingDir dir) {
		m_direction = dir;
	}

	//! Returns the number of levels.
	int size() const override { return m_pLevel.size(); }

	//! Returns the maximal array index of a level (= size()-1).
	int high() const override { return m_pLevel.high(); }

	//! Returns the position of node \p v on its level.
	int pos(node v) const override { return m_pos[v]; }

	//! Returns the adjacent nodes of \p v (according to direction()).
	const Array<node> &adjNodes(node v) const {
		return (m_direction == TraversingDir::downward) ? m_lowerAdjNodes[v] :
		m_upperAdjNodes[v];
	}

	//! Returns the adjacent nodes of \p v.
	const Array<node> &adjNodes(node v, TraversingDir dir) const override {
		return (dir == TraversingDir::downward) ? m_lowerAdjNodes[v] :
		m_upperAdjNodes[v];
	}

	//! Returns the adjacent level of level \p i (according to direction()).
	const Level &adjLevel(int i) const {
		return (m_direction == TraversingDir::downward) ? *m_pLevel[i-1] : *m_pLevel[i+1];
	}

	//! Returns the <i>i</i>-th level.
	const Level &operator[](int i) const override { return *m_pLevel[i]; }

	//! Returns the <i>i</i>-th level.
	Level &operator[](int i) { return *m_pLevel[i]; }

	//! Computes the number of crossings between level \p i and \p i+1 (for simultaneous drawing).
	int calculateCrossingsSimDraw(int i, const EdgeArray<uint32_t> *edgeSubGraphs) const;
	//! Computes the total number of crossings (for simultaneous drawing).
	int calculateCrossingsSimDraw(const EdgeArray<uint32_t> *edgeSubGraphs) const;

	//! Stores the position of nodes in \p oldPos.
	void storePos (NodeArray<int> &oldPos) const;
	//! Restores the position of nodes from \p newPos.
	void restorePos (const NodeArray<int> &newPos);

	//! Permutes the order of nodes on each level.
	void permute();

	template<class RNG>
	void permute(RNG &rng);

	//! Adjusts node positions such that nodes are ordered according to components numbers.
	void separateCCs(int numCC, const NodeArray<int> &component);

	bool transpose(node v);

	void print(std::ostream &os) const;

	void buildAdjNodes(int i);
	void buildAdjNodes();

	void check() const;

private:
	int transposePart(const Array<node> &adjV, const Array<node> &adjW);

	OGDF_MALLOC_NEW_DELETE
};


template<class RNG>
void HierarchyLevels::permute(RNG &rng)
{
	for(int i = 0; i < m_pLevel.high(); ++i) {
		Level &level = *m_pLevel[i];
		level.m_nodes.permute(rng);
		for(int j = 0; j <= level.high(); ++j)
			m_pos[level[j]] = j;
	}

	buildAdjNodes();
}

}
