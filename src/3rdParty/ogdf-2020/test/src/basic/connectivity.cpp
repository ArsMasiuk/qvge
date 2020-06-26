/** \file
 * \brief Tests for ogdf::ConnectivityTester.
 *
 * \author Tilo Wiedera
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

#include <ogdf/graphalg/ConnectivityTester.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/graphalg/MaxFlowEdmondsKarp.h>
#include <graphs.h>
#include <sstream>

go_bandit([]() {
	describe("ConnectivityTester", []() {
		for(bool directed : {true, false}) {
			std::stringstream ss;
			if(!directed) {
				ss << "un";
			}
			ss << "directed";
			describe(ss.str().c_str(), [&]() {
				for(bool nodeConnectivity : {true, false}) {
					ss.str(nodeConnectivity ? "node" : "edge");
					ss << "-connectivity";
					describe(ss.str().c_str(), [&]() {
						forEachGraphItWorks({GraphProperty::simple}, [&](const Graph& G) {
							NodeArray<NodeArray<int>> matrix(G, NodeArray<int>(G));
							ConnectivityTester con(nodeConnectivity, directed);
							int value = con.computeConnectivity(G, matrix);
							bool valueFound = G.numberOfNodes() < 2;

							for(node v : G.nodes) {
								for(node w : G.nodes) {
									if(v != w) {
										if(!directed) {
											AssertThat(matrix[v][w], Equals(matrix[w][v]));
										}
										AssertThat(matrix[v][w], !IsLessThan(value));
										valueFound |= matrix[v][w] == value;
										AssertThat(matrix[v][w], Equals(con.computeConnectivity(G, v, w)));
									}
								}
							}

							AssertThat(valueFound, IsTrue());
						}, GraphSizes(5, 20, 5));
					});

				}
			});
		}
	});
});
