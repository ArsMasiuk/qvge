/** \file
 * \brief Implementation of class WSPD (well-separated pair decomposition).
 *
 * \author Martin Gronemann
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

#include <ogdf/energybased/fast_multipole_embedder/WSPD.h>

namespace ogdf {
namespace fast_multipole_embedder {

WSPD::WSPD(uint32_t maxNumNodes) : m_maxNumNodes(maxNumNodes)
{
	m_maxNumPairs = maxNumNodes*2;
	m_numPairs = 0;
	allocate();
	clear();
}


WSPD::~WSPD(void)
{
	deallocate();
}


unsigned long WSPD::sizeInBytes() const
{
	return m_maxNumNodes*sizeof(NodeAdjInfo) +
		m_maxNumPairs*sizeof(EdgeAdjInfo);
}


void WSPD::allocate()
{
	m_nodeInfo = static_cast<NodeAdjInfo*>(OGDF_MALLOC_16(m_maxNumNodes*sizeof(NodeAdjInfo)));
	m_pairs = static_cast<EdgeAdjInfo*>(OGDF_MALLOC_16(m_maxNumPairs*sizeof(EdgeAdjInfo)));
}


void WSPD::deallocate()
{
	OGDF_FREE_16(m_nodeInfo);
	OGDF_FREE_16(m_pairs);
}


void WSPD::clear()
{
	for (uint32_t i = 0; i < m_maxNumNodes; i++)
	{
		m_nodeInfo[i].degree = 0;
	}
	m_numPairs = 0;
}

void WSPD::addWSP(NodeID a, NodeID b) {
	// get the index of a free element
	uint32_t e_index = m_numPairs++;

	pushBackEdge(
		a, b,
		[&](uint32_t i) -> EdgeAdjInfo& {return pairInfo(i);},
		[&](uint32_t i) -> NodeAdjInfo& {return nodeInfo(i);},
		e_index);
}

}
}
