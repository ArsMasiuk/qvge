/** \file
 * \brief Tests for ogdf::AdjacencyOracle
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

#include <ogdf/basic/AdjacencyOracle.h>
#include <graphs.h>

static void describeAdjacencyOracleWithDegreeThreshold(string title, int degreeThreshold) {
	describe(title + " (degree threshold = " + std::to_string(degreeThreshold) + ")", [&] {
		forEachGraphItWorks({}, [&] (const Graph& graph) {
			AdjacencyOracle oracle{graph, degreeThreshold};

			for (node u : graph.nodes) {
				for (node v : graph.nodes) {
					bool isContained{graph.searchEdge(u, v) != nullptr};
					AssertThat(oracle.adjacent(u, v), Equals(isContained));
				}
			}
		});
	});
}

go_bandit([] {
	describe("AdjacencyOracle", [] {
		describeAdjacencyOracleWithDegreeThreshold("using lookups only", 0);
		describeAdjacencyOracleWithDegreeThreshold("using partly lookups and partly search", 4);
		describeAdjacencyOracleWithDegreeThreshold("using search only", 1000);
	});
});
