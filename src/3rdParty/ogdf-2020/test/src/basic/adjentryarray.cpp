/** \file
 * \brief Tests for ogdf::AdjEntryArray
 *
 * \author Mirko Wagner, Tilo Wiedera
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
#include "array_helper.h"
#include <ogdf/basic/AdjEntryArray.h>

using namespace ogdf;
using namespace bandit;

go_bandit([]() {
	auto chooseAdjEntry = [](const Graph &graph) {
		edge e = graph.chooseEdge();
		return randomNumber(0, 1) ? e->adjSource() : e->adjTarget();
	};

	auto allAdjEntries = [](const Graph &graph, List<adjEntry> &list) {
		list.clear();

		for(edge e : graph.edges) {
			list.pushBack(e->adjSource());
			list.pushBack(e->adjTarget());
		}
	};

	auto createAdjEntry = [](Graph &graph) {
		edge e = graph.newEdge(graph.chooseNode(), graph.chooseNode());
		return randomNumber(0, 1) ? e->adjSource() : e->adjTarget();
	};

	describeArray<AdjEntryArray, adjEntry, int>("AdjEntryArray filled with ints", 42, 43, chooseAdjEntry, allAdjEntries, createAdjEntry);
	describeArray<AdjEntryArray, adjEntry, List<int>>("AdjEntryArray filled with lists of ints", {1, 2, 3}, {42}, chooseAdjEntry, allAdjEntries, createAdjEntry);
});
