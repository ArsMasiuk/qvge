/** \file
 * \brief Implementation of pushBackEdge.
 *
 * \author Ivo Hedtke
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

#include <ogdf/energybased/fast_multipole_embedder/EdgeChain.h>

namespace ogdf {
namespace fast_multipole_embedder {

void pushBackEdge(uint32_t a, uint32_t b,
                  std::function<EdgeAdjInfo&(uint32_t)> edgeInform,
                  std::function<NodeAdjInfo&(uint32_t)> nodeInform,
                  int e_index)
{
	auto adjustLinks = [&](NodeAdjInfo& info, uint32_t index) {
		// if is part of at least one edge
		if (info.degree) {
			// adjust the links
			EdgeAdjInfo& a_e = edgeInform(info.lastEntry);
			// check which one is a
			if (index == a_e.a) { a_e.a_next = e_index; }
			else { a_e.b_next = e_index; }
		} else {
			// this edge is the first for a => set the firstEntry link
			info.firstEntry = e_index;
		}
		// and the lastEntry link
		info.lastEntry = e_index;
		// one more edge for each node
		info.degree++;
	};

	adjustLinks(nodeInform(a), a);
	adjustLinks(nodeInform(b), b);

	// get the pair entry
	EdgeAdjInfo& e = edgeInform(e_index);

	// (a,b) is the pair we are adding
	e.a = a;
	e.b = b;
}

}
}
