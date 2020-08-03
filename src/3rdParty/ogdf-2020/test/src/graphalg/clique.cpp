/** \file
 * \brief Tests for clique finding algorithms
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

//#define OGDF_TEST_CLIQUE_PRINT_DRAWINGS

#include <ogdf/clique/CliqueFinderHeuristic.h>
#include <ogdf/clique/CliqueFinderSPQR.h>

#ifdef OGDF_TEST_CLIQUE_PRINT_DRAWINGS
# include <ogdf/energybased/SpringEmbedderExact.h>
# include <regex>
#endif

#include <graphs.h>
#include <testing.h>

static void assertCliqueCorrectness(const Graph &G,
		const NodeArray<int> &cliqueNumber,
		const List<List<node>*> &cliqueList,
		int minSize,
		double density = 1.0)
{
	NodeArray<bool> used(G, false);
	for (List<node> *clique : cliqueList) {
		// All cliques are greater than minSize.
		AssertThat(clique->size(), IsGreaterThanOrEqualTo(minSize));

		int curCliqueNumber = cliqueNumber[clique->front()];
		for (node v : *clique) {
			// Clique number and clique list are congruent.
			AssertThat(cliqueNumber[v], Equals(curCliqueNumber));

			// All cliques are disjoint.
			AssertThat(used[v], IsFalse());
			used[v] = true;
		}

		// The nodes form a clique.
		AssertThat(CliqueFinderModule::cliqueOK(G, clique, density), IsTrue());
	}

	// More clique list/number congruency, in particular:
	// All unused nodes have a negative clique number.
	for (node v : G.nodes) {
		AssertThat(cliqueNumber[v] >= 0, Equals(used[v]));
	}
}

template<typename Algorithm>
static void describeCliqueFinder(Algorithm &cf, double density = 1.0) {
	forEachGraphDescribe({}, [&](const Graph& G,
				const std::string &graphName,
				const std::set<GraphProperty>&) {
		List<List<node>*> cliqueList;

		after_each([&cliqueList] {
			// Free memory.
			for (List<node> *clique : cliqueList) {
				delete clique;
			}
			cliqueList.clear();
		});

		// Find a reasonable maxMinSize and stepSize.
		// Cliques bigger than maxMinSize will probably not exist.
		int n = G.numberOfNodes();
		double graphDensity = min(1.0, G.numberOfEdges() / (n*(n-1)/2.0));
		int maxMinSize = max(2, int(n * graphDensity));
		int stepSize = maxMinSize <= 9 ? 1 : 3;

		for (int minSize = 1; minSize < maxMinSize + 1; minSize += stepSize) {
			it("works with minSize " + std::to_string(minSize), [&](){
				NodeArray<int> cliqueNumber(G);
				cf.setMinSize(minSize);
				cf.call(G, cliqueNumber);
				cf.call(G, cliqueList);

#ifdef OGDF_TEST_CLIQUE_PRINT_DRAWINGS
				GraphAttributes GA(G);
				CliqueFinderModule::cliqueGraphAttributes(G, cliqueNumber, GA);
				SpringEmbedderExact layout;
				layout.call(GA);
				GA.scale(3, false);
				std::string filename =
					std::regex_replace(graphName, std::regex("\\W+"), "_") +
					"_density=" + std::to_string(density) +
					"_minSize=" + std::to_string(minSize) + ".svg";
				GraphIO::write(GA, filename, GraphIO::drawSVG);
#endif

				assertCliqueCorrectness(G, cliqueNumber, cliqueList, minSize, density);
			});
		}
	});
}


go_bandit([] {
	describe("Clique finding algorithms", [] {
		describe("CliqueFinderHeuristic", [] {
			CliqueFinderHeuristic cf;

			for (bool postprocess : {false, true}) {
				string withOut = postprocess ? "with" : "without";
				describe(withOut + " postprocessing", [&] {
					cf.setPostProcessing(postprocess);

					for (double density : {1.0, 0.75}) {
						describe("with density = " + to_string(density), [&] {
							cf.setDensity(density);
							describeCliqueFinder<CliqueFinderHeuristic>(cf, density);
						});
					}
				});
			}
		});

		describe("CliqueFinderSPQR with CliqueFinderHeuristic", [] {
			CliqueFinderHeuristic heurCf;
			CliqueFinderSPQR cf(heurCf);
			describeCliqueFinder<CliqueFinderSPQR>(cf);
		});
	});
});
