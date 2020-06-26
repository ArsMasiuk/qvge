/** \file
 * \brief Implements ogdf::CliqueFinderSPQR class.
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

#include <ogdf/clique/CliqueFinderSPQR.h>
#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Math.h>

namespace ogdf {

void CliqueFinderSPQR::doCall()
{
	// Precondition for SPQR-trees: n >= 3 or (n == 2 and m >= 3).
	// Special cases are handled by CliqueFinderModule::handleTrivialCases().

	// The GraphCopy has to be made biconnected to use the SPQR tree.
	// This keeps the triconnected components.
	makeBiconnected(*m_pCopy);
	OGDF_ASSERT(isSimple(*m_pCopy));
	StaticSPQRTree spqrTree(*m_pCopy);

	// Go through (bigger) R nodes first since they contain bigger cliques.
	List<node> spqrNodes;
	spqrTree.tree().allNodes(spqrNodes);
	spqrNodes.quicksort(
		GenericComparer<node, int, false>([&spqrTree](node v) {
			if (spqrTree.typeOf(v) == SPQRTree::NodeType::RNode) {
				return spqrTree.skeleton(v).getGraph().numberOfNodes();
			} else {
				return -1;
			}
		})
	);

	// Search for cliques in SPQR-nodes:
	int cliqueNumber = 0;
	for (node v : spqrNodes) {
		// Retrieve the skeleton.
		Skeleton &s = spqrTree.skeleton(v);
		Graph &skeletonG = s.getGraph();

		// Remove all nodes which are already in other cliques.
		safeForEach(skeletonG.nodes, [&](node vSkel) {
			if (m_copyCliqueNumber[s.original(vSkel)] >= 0) {
				skeletonG.delNode(vSkel);
			}
		});

		// Remove all edges which are dummies created by makeBiconnected or
		// virtual in the SPQR tree.
		safeForEach(skeletonG.edges, [&](edge eSkel) {
			if (s.isVirtual(eSkel) || m_pCopy->isDummy(s.realEdge(eSkel))) {
				skeletonG.delEdge(eSkel);
			}
		});

		if (spqrTree.typeOf(v) == SPQRTree::NodeType::RNode) {
			// Get cliques in the triconnected skeleton.
			NodeArray<int> skelCliqueNumber(skeletonG);
			m_cliqueFinder.setMinSize(m_minDegree + 1);
			m_cliqueFinder.call(skeletonG, skelCliqueNumber);

			// Set clique numbers in original GraphCopy.
			int maxCliqueNumber = cliqueNumber - 1;
			for (node vSkel : skeletonG.nodes) {
				if (skelCliqueNumber[vSkel] >= 0) {
					int newCliqueNum = skelCliqueNumber[vSkel] + cliqueNumber;
					m_copyCliqueNumber[s.original(vSkel)] = newCliqueNum;
					Math::updateMax(maxCliqueNumber, newCliqueNum);
				}
			}
			cliqueNumber = maxCliqueNumber + 1;
		} else {
			// The SPQR node is either an S node or a P node.
			// Each triangle (S node) is a clique for min degree 2.
			if (skeletonG.numberOfNodes() == 3 &&
			    skeletonG.numberOfEdges() == 3 && m_minDegree <= 2) {
				for (node vSkel : skeletonG.nodes) {
					m_copyCliqueNumber[s.original(vSkel)] = cliqueNumber;
				}
				cliqueNumber++;
			} else if (m_minDegree <= 1) {
				// Each single edge in an S/P node is a clique for min degree 1,
				for (node vSkel : skeletonG.nodes) {
					node vOrig = s.original(vSkel);
					if (m_copyCliqueNumber[vOrig] < 0) {
						for (adjEntry adj : vSkel->adjEntries) {
							node wOrig = s.original(adj->twinNode());
							if (m_copyCliqueNumber[wOrig] < 0) {
								m_copyCliqueNumber[vOrig] = cliqueNumber;
								m_copyCliqueNumber[wOrig] = cliqueNumber;
								cliqueNumber++;
								break;
							}
						}
					}
				}
			}
		}
	}
}

}
