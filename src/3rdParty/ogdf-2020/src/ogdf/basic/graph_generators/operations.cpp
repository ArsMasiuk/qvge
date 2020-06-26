/** \file
 * \brief Implementation of graph operations
 *
 * \author Max Ilsen
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

#include <ogdf/basic/graph_generators/operations.h>
#include <ogdf/basic/simple_graph_alg.h>


namespace ogdf {

void graphUnion(Graph &G1, const Graph &G2,
	NodeArray<node> &map2to1,
	bool parallelfree,
	bool directed)
{
	for (node v2 : G2.nodes) {
		if (map2to1[v2] == nullptr) {
			map2to1[v2] = G1.newNode();
		}
	}

	for (edge e2 : G2.edges) {
		G1.newEdge(map2to1[e2->source()], map2to1[e2->target()]);
	}

	if (parallelfree) {
		if (directed) {
			makeParallelFree(G1);
		} else {
			makeParallelFreeUndirected(G1);
		}
	}
}


void graphProduct(const Graph &G1, const Graph &G2, Graph &product, NodeMap &nodeInProduct,
	const std::function<void(node, node)> &addEdges)
{
	nodeInProduct.init(G1);

	// Clear product.
	product.clear();

	// Add nodes to product.
	for (node v1 : G1.nodes) {
		nodeInProduct[v1].init(G2);
		for (node v2 : G2.nodes) {
			nodeInProduct[v1][v2] = product.newNode();
		}
	}

	// Add edges to product.
	for (node v1 : G1.nodes) {
		for (node v2 : G2.nodes) {
			addEdges(v1, v2);
		}
	}
}


void cartesianProduct(const Graph &G1, const Graph &G2, Graph &product, NodeMap &nodeInProduct)
{
	graphProduct(G1, G2, product, nodeInProduct, [&](node v1, node v2) {
		node srcInProduct = nodeInProduct[v1][v2];

		// Add G2-edges between copies of G1.
		for (adjEntry adj2 : v2->adjEntries) {
			if (adj2->isSource()) {
				product.newEdge(srcInProduct,
					nodeInProduct[v1][adj2->twinNode()]
				);
			}
		}

		// Add G1-edges between copies of G2.
		for (adjEntry adj1 : v1->adjEntries) {
			if (adj1->isSource()) {
				product.newEdge(srcInProduct,
					nodeInProduct[adj1->twinNode()][v2]
				);
			}
		}
	});
}


void tensorProduct(const Graph &G1, const Graph &G2, Graph &product, NodeMap &nodeInProduct)
{
	graphProduct(G1, G2, product, nodeInProduct, [&](node v1, node v2) {
		// Add edges between adjacent node pairs.
		for (adjEntry adj1 : v1->adjEntries) {
			for (adjEntry adj2 : v2->adjEntries) {
				if (adj2->isSource()) {
					product.newEdge(
						nodeInProduct[v1][v2],
						nodeInProduct[adj1->twinNode()][adj2->twinNode()]
					);
				}
			}
		}
	});
}


void lexicographicalProduct(const Graph &G1, const Graph &G2, Graph &product, NodeMap &nodeInProduct)
{
	graphProduct(G1, G2, product, nodeInProduct, [&](node v1, node v2) {
		node srcInProduct = nodeInProduct[v1][v2];

		// Add G1-edges between copies of G2, linking all pairs of G2-nodes.
		for (node v2Tgt : G2.nodes) {
			for (adjEntry adj1 : v1->adjEntries) {
				if (adj1->isSource()) {
					product.newEdge(srcInProduct,
						nodeInProduct[adj1->twinNode()][v2Tgt]
					);
				}
			}
		}

		// Add G2-edges between copies of G1.
		for (adjEntry adj2 : v2->adjEntries) {
			if (adj2->isSource()) {
				product.newEdge(srcInProduct,
					nodeInProduct[v1][adj2->twinNode()]
				);
			}
		}
	});
}


void strongProduct(const Graph &G1, const Graph &G2, Graph &product, NodeMap &nodeInProduct)
{
	graphProduct(G1, G2, product, nodeInProduct, [&](node v1, node v2) {
		node srcInProduct = nodeInProduct[v1][v2];

		// Add G2-edges between copies of G1.
		for (adjEntry adj2 : v2->adjEntries) {
			if (adj2->isSource()) {
				product.newEdge(srcInProduct,
					nodeInProduct[v1][adj2->twinNode()]);
			}
		}

		// Add G1-edges between copies of G2.
		for (adjEntry adj1 : v1->adjEntries) {
			if (adj1->isSource()) {
				product.newEdge(srcInProduct,
					nodeInProduct[adj1->twinNode()][v2]
				);
			}
		}

		// Add edges between adjacent node pairs.
		for (adjEntry adj1 : v1->adjEntries) {
			for (adjEntry adj2 : v2->adjEntries) {
				if (adj2->isSource()) {
					product.newEdge(srcInProduct,
						nodeInProduct[adj1->twinNode()][adj2->twinNode()]
					);
				}
			}
		}
	});
}


void coNormalProduct(const Graph &G1, const Graph &G2, Graph &product, NodeMap &nodeInProduct)
{
	graphProduct(G1, G2, product, nodeInProduct, [&](node v1, node v2) {
		node srcInProduct = nodeInProduct[v1][v2];

		// Add G1-edges between copies of G2, linking all a pairs of G2-nodes.
		for (node v2Tgt : G2.nodes) {
			for (adjEntry adj1 : v1->adjEntries) {
				if (adj1->isSource()) {
					product.newEdge(srcInProduct,
						nodeInProduct[adj1->twinNode()][v2Tgt]
					);
				}
			}
		}

		// Add G2-edges between copies of G1, linking all a pairs of G1-nodes.
		for (node v1Tgt : G1.nodes) {
			for (adjEntry adj2 : v2->adjEntries) {
				if (adj2->isSource()) {
					product.newEdge(srcInProduct,
						nodeInProduct[v1Tgt][adj2->twinNode()]
					);
				}
			}
		}
	});
}


void modularProduct(const Graph &G1, const Graph &G2, Graph &product, NodeMap &nodeInProduct)
{
	graphProduct(G1, G2, product, nodeInProduct, [&](node v1, node v2) {
		node srcInProduct = nodeInProduct[v1][v2];
		NodeArray<bool> adjacentToV1(G1, false);
		NodeArray<bool> adjacentToV2(G2, false);

		// Add edges between adjacent node pairs, remember v1-adjacencies.
		for (adjEntry adj1 : v1->adjEntries) {
			adjacentToV1[adj1->twinNode()] = true;
			for (adjEntry adj2 : v2->adjEntries) {
				if (adj2->isSource()) {
					product.newEdge(srcInProduct,
						nodeInProduct[adj1->twinNode()][adj2->twinNode()]
					);
				}
			}
		}

		// Remember v2-adjacencies
		// (extra loop in case we do not iterate through the loop above).
		for (adjEntry adj2 : v2->adjEntries) {
			adjacentToV2[adj2->twinNode()] = true;
		}

		// Add edges between non-adjacent node pairs,
		for (node neighbour1 : G1.nodes) {
			if (neighbour1 != v1 && !adjacentToV1[neighbour1]) {
				// Only to nodes "after" v2 so edges are not inserted twice.
				for (node neighbour2 = v2->succ(); neighbour2; neighbour2 = neighbour2->succ()) {
					if (!adjacentToV2[neighbour2]) {
						product.newEdge(srcInProduct,
							nodeInProduct[neighbour1][neighbour2]
						);
					}
				}
			}
		}
	});
}


void rootedProduct(const Graph &G1, const Graph &G2, Graph &product,
	NodeMap &nodeInProduct, node rootInG2)
{
	graphProduct(G1, G2, product, nodeInProduct, [&](node v1, node v2) {
		node srcInProduct = nodeInProduct[v1][v2];

		// Add G2-edges between copies of G1.
		for (adjEntry adj2 : v2->adjEntries) {
			if (adj2->isSource()) {
				product.newEdge(srcInProduct,
					nodeInProduct[v1][adj2->twinNode()]
				);
			}
		}

		// Add G1-edges for copy of G1 that represents rootInG2.
		if (v2 == rootInG2) {
			for (adjEntry adj1 : v1->adjEntries) {
				if (adj1->isSource()) {
					product.newEdge(srcInProduct,
						nodeInProduct[adj1->twinNode()][v2]
					);
				}
			}
		}
	});
}

}
