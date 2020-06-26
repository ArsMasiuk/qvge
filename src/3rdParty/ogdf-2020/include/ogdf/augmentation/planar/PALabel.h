/** \file
 * \brief Declares auxiliary structure of planar augmentation algorithms.
 *
 * \author Bernd Zey
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


namespace ogdf {

/**
 * \brief auxiliary class for the planar augmentation algorithm
 *
 *   A label contains several pendants, a parent- and a head- node.
 *   The head node is a cutvertex in the correspondign BC-Tree.
 *   The pendants can be connected by edges so planarity is maintained.
 */
class PALabel
{
	friend class PlanarAugmentation;
	friend class PlanarAugmentationFix;

public:
	enum class StopCause { Planarity, CDegree, BDegree, Root };

	PALabel(node parent, node cutvertex, StopCause sc = StopCause::BDegree) {
		m_parent = parent;
		m_head = cutvertex;
		m_stopCause = sc;
	}

	bool isBLabel() {
		return m_parent != nullptr;
	}

	bool isCLabel() {
		return m_parent == nullptr;
	}

	//! return pendant with number nr, starts counting at 0
	node getPendant(int nr) {
		return nr < m_pendants.size() ? *m_pendants.get(nr) : nullptr;
	}

	node getFirstPendant() {
		return (m_pendants.size() > 0) ? m_pendants.front() : nullptr;
	}

	node getLastPendant() {
		return (m_pendants.size() > 0) ? m_pendants.back() : nullptr;
	}

	//! return number of pendants
	int size() {
		return m_pendants.size();
	}

	void removePendant(node pendant);

	void removePendant(ListIterator<node> it){
		m_pendants.del(it);
	}

	void removeFirstPendant() {
		if (m_pendants.size() > 0){
			m_pendants.popFront();
		}
	}

	void addPendant(node pendant) {
		m_pendants.pushBack(pendant);
	}

	void deleteAllPendants() {
		m_pendants.clear();
	}

	//! return the parent node. If the label is a c-label it returns m_head
	node parent() {
		return (m_parent != nullptr) ? m_parent : m_head;
	}

	//! returns the head node
	node head() {
		return m_head;
	}

	void setParent(node newParent){
		m_parent = newParent;
	}

	void setHead(node newHead){
		m_head = newHead;
	}

	StopCause stopCause(){
		return m_stopCause;
	}

	void stopCause(StopCause sc){
		m_stopCause = sc;
	}

	OGDF_NEW_DELETE

private:

	/**
	 * \brief the "parent" of the pendants in the BC-Tree, m_parent is a b-vertex or a c-vertex
	 * if it is a b-vertex m_parent != 0
	 * otherwise m_parent == 0 and the parent is the head node
	 * m_head is always != 0
	 */
	node m_parent;

	node m_head; //!< the cutvertex and perhaps (see m_parent) the parent node

	List<node> m_pendants; //!< list with all pendants of the label

	StopCause m_stopCause;  //!< the stop cause that occurs when traversing from the pendants to the bc-tree-root computed in PlanarAugmentation::followPath()
};

using pa_label = PALabel*;

}
