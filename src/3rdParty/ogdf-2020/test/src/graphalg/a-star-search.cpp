/** \file
 * \brief Tests for the A* informed search algorithm.
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

#include <iomanip>
#include <chrono>

#include <ogdf/basic/graph_generators.h>
#include <ogdf/graphalg/AStarSearch.h>
#include <ogdf/graphalg/Dijkstra.h>

#include <testing.h>

template<typename T>
void validatePath(
		const node source,
		const node target,
		const Graph &graph,
		const EdgeArray<T> &cost,
		const NodeArray<edge> &pred,
		const T expectedCost) {

	T actualCost = 0;

	NodeArray<bool> visited(graph, false);

	for(node v = target; v != source; v = pred[v]->opposite(v)) {
		AssertThat(visited[v], IsFalse());
		actualCost += cost[pred[v]];
		visited[v] = true;
	}

	AssertThat(actualCost, Equals(expectedCost));
}

template<typename T>
void performSingleTest(
		const Graph &graph,
		const node source,
		const node target,
		const EdgeArray<T> cost,
		const double maxGap,
		const bool directed,
		Dijkstra<T> &dijkstra,
		AStarSearch<T> &astar,
		long &ticksDijkstra,
		long &ticksUninformedAStar,
		long &ticksAStarHeuristic)
{
	NodeArray<T> distance(graph, -1);
	NodeArray<edge> pred(graph);

	auto start = std::chrono::system_clock::now();
	dijkstra.call(graph, cost, source, pred, distance, directed);
	ticksDijkstra += (std::chrono::system_clock::now() - start).count();
	bool foundPath = pred[target] != nullptr;
	T opt = distance[target];

	if(foundPath) {
		validatePath(source, target, graph, cost, pred, distance[target]);

		distance.init(graph, -1);
		pred.init(graph, nullptr);

		start = std::chrono::system_clock::now();
		T result = astar.call(graph, cost, source, target, pred, [&](node v) {
			// utilize distances as returned by Dijkstra for a perfect heuristic
			return distance[v];
		});
		ticksAStarHeuristic += (std::chrono::system_clock::now() - start).count();

		validatePath(source, target, graph, cost, pred, result);
		AssertThat(pred[target] != nullptr, IsTrue());
		AssertThat(distance[target], IsLessThan(opt * maxGap + 1));
	}

	start = std::chrono::system_clock::now();
	T result = astar.call(graph, cost, source, target, pred);
	ticksUninformedAStar += (std::chrono::system_clock::now() - start).count();

	AssertThat(pred[target] != nullptr, Equals(foundPath));
	if(foundPath) {
		validatePath(source, target, graph, cost, pred, result);
		AssertThat(distance[target], IsLessThan(opt * maxGap + 1));
	}
}

template<typename T>
void performTests(const bool directed, const double maxGap, const bool pathLike) {
	const int NUMBER_OF_GRAPHS = 10;
	const int MIN_NODES = 100;
	const int MAX_NODES = 200;

	AStarSearch<T> astar(directed, maxGap);
	Dijkstra<T> dijkstra;

	long ticksDijkstra = 0;
	long ticksUninformedAStar = 0;
	long ticksAStarHeuristic = 0;

	for(int i = 0; i < NUMBER_OF_GRAPHS; i++) {
		Graph graph;
		EdgeArray<T> cost(graph);
		node source = nullptr;
		node target = nullptr;
		int n = randomNumber(MIN_NODES, MAX_NODES);

		if(pathLike) {
			completeGraph(graph, n);
			cost.init(graph, n);

			source = graph.chooseNode();
			node v = source;

			for(int k = 0; k < n/2 || v == source; k++) {
				adjEntry adj = v->firstAdj();

				for(int j = randomNumber(0, v->degree()-1); j > 0; j--) {
					adj = adj->succ();
				}

				edge e = adj->theEdge();
				cost[e] = randomNumber(1, 10);
				v = e->opposite(v);
			}

			target = v;
		} else {
			randomBiconnectedGraph(graph, n, randomNumber(n, (n*(n-1) / 2)));

			for(edge e : graph.edges) {
				cost[e] = randomNumber(1, graph.numberOfEdges());
			}

			source = graph.chooseNode();
			target = graph.chooseNode([&](node v) { return v != source; });
		}

		performSingleTest(graph, source, target, cost, maxGap, directed, dijkstra, astar,
				ticksDijkstra, ticksUninformedAStar, ticksAStarHeuristic);
	}

	std::cout << std::endl;
	std::cout << std::left << "    Dijkstra              : " << std::right << std::setw(16) << ticksDijkstra << std::endl;
	std::cout << std::left << "    A* uninformed         : " << std::right << std::setw(16) << ticksUninformedAStar << std::endl;
	std::cout << std::left << "    A* perfect heuristic  : " << std::right << std::setw(16) << ticksAStarHeuristic << std::endl;
	std::cout << std::left;
}

template<typename T>
void registerTests(string typeName) {
	EpsilonTest et;
	for(int i = 0; i < 16; i++) {
		bool pathLike = i % 2;
		bool directed = (i / 2) % 2;
		double maxGap =  1 + (i / 4) / (double) 2;

		string title = "yields the same result as Dijkstra";
		if(!et.equal(maxGap, 1.0)) {
			title = "approximates the optimal solution with a maxmimum gap of " + to_string(maxGap);
		}

		title = "[" + typeName + "] " + title;

		title += " on ";
		title += (directed ? "directed " : "");
		title += (pathLike ? "path-like" : "biconnected");
		title += " graphs";

		it(title, [&](){
			performTests<T>(directed, maxGap, pathLike);
		});
	}
}

go_bandit([](){
	describe("A* Informed Search Algorithm", [](){
		registerTests<int>("int");
		registerTests<double>("double");
	});
});
