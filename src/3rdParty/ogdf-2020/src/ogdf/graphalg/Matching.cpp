/** \file
 * \brief Implements (non-templated) simple matching functions
 *
 * \author Stephan Beyer
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

#include <ogdf/graphalg/Matching.h>

namespace ogdf {
namespace Matching {

void findMaximalMatching(const Graph& graph, ArrayBuffer<edge>& matching) {
	EdgeArray<bool> covered{graph, false};

	for (edge e : graph.edges) {
		if (!covered[e]) {
			matching.push(e);
			for (node v : e->nodes()) {
				for (adjEntry adj : v->adjEntries) {
					covered[adj->theEdge()] = true;
				}
			}
		}
	}
}

}
}
