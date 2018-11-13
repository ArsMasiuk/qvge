/** \file
 * \brief Bandit test suite for Steiner tree algorithms
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

#include <string>
#include <tuple>
#include <vector>
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/graphalg/MinSteinerTreeDirectedCut.h>
#include <ogdf/graphalg/MaxFlowEdmondsKarp.h>
#include <ogdf/graphalg/MinSteinerTreeKou.h>
#include <ogdf/graphalg/MinSteinerTreeMehlhorn.h>
#include <ogdf/graphalg/MinSteinerTreeRZLoss.h>
#include <ogdf/graphalg/MinSteinerTreeZelikovsky.h>
#include <ogdf/graphalg/MinSteinerTreeShore.h>
#include <ogdf/graphalg/MinSteinerTreePrimalDual.h>
#include <ogdf/graphalg/MinSteinerTreeDualAscent.h>
#include <ogdf/graphalg/MinSteinerTreeGoemans139.h>
#include <resources.h>

template<typename T> using ModuleTuple = std::tuple<std::string, MinSteinerTreeModule<T>*, double>;

/**
 * Generates a new graph with an optimal Steiner tree.
 * Only very basic graphs are generated
 * to guarantee the optimality of the resulting Steiner tree.
 *
 * \param n
 *	number of nodes
 * \param graph
 *	the resulting graph
 * \param terminals
 *	this list will hold all terminals
 * \param isTerminal
 *	stores which node is a terminal
 * \param tree
 *	an optimal Steiner tree for this graph.
 */
template<typename T>
T randomOptimalSteiner(
  int n,
  EdgeWeightedGraph<T> &graph,
  List<node> &terminals,
  NodeArray<bool> &isTerminal,
  EdgeWeightedGraphCopy<T> &tree
)
{
	OGDF_ASSERT(n >= 4);

	T result = 0;

	graph.clear();
	terminals.clear();
	tree.clear();
	tree.createEmpty(graph);
	isTerminal.init(graph, false);

	node source = graph.newNode();
	tree.newNode(source);
	isTerminal[source] = true;

	int numberOfTerminals = randomNumber(n/4, n/2);
	int numberOfNonterminals = n - numberOfTerminals;
	int numberOfEdges = randomNumber(numberOfTerminals-1 + numberOfNonterminals*2, (n*(n-1))/2);

	for(int i = 1; i < numberOfTerminals; i++) {
		node v = graph.chooseNode();
		node u = graph.newNode();
		tree.newNode(u);

		edge e = graph.newEdge(v, u, 1);
		result++;
		tree.newEdge(e, 1);
		if(isTerminal[v] && v != source) {
			isTerminal[v] = false;
		}

		isTerminal[u] = true;
	}

	for(int i = numberOfTerminals-1; i < numberOfEdges;) {
		node v = graph.chooseNode();
		node u = graph.chooseNode([&](node w) { return w != v; });

		if(numberOfNonterminals > 0) {
			node w = graph.newNode();
			graph.newEdge(v, w, n);
			graph.newEdge(w, u, n);
			numberOfNonterminals--;
			i += 2;
		}
		else {
			if (graph.searchEdge(v, u) == nullptr
			 && graph.searchEdge(u, v) == nullptr) {
				graph.newEdge(v, u, n);
				i++;
			}
		}
	}

	for (node v : graph.nodes) {
		if (isTerminal[v]) {
			terminals.pushBack(v);
		}
	}

	OGDF_ASSERT(terminals.size() <= numberOfTerminals);
	OGDF_ASSERT(graph.numberOfEdges() == numberOfEdges);
	OGDF_ASSERT(tree.numberOfNodes() == numberOfTerminals);
	OGDF_ASSERT(tree.numberOfEdges() == numberOfTerminals - 1);
	OGDF_ASSERT(graph.numberOfNodes() == n);
	OGDF_ASSERT(isSimpleUndirected(graph));
	OGDF_ASSERT(isConnected(graph));
	return result;
}

/**
 * Test if modules generates a valid Steiner tree for a graph with given number of nodes
 */
template<typename T>
void testModuleOnRandomGraph(MinSteinerTreeModule<T> &alg, int n, double factor = 0)
{
	it(string("generates a valid Steiner tree for a graph of " + to_string(n) + " nodes"), [&](){
		EdgeWeightedGraph<T> graph;
		EdgeWeightedGraphCopy<T> tree;
		NodeArray<bool> isTerminal(graph, false);
		List<node> terminals;

		T cost = randomOptimalSteiner<T>(n, graph, terminals, isTerminal, tree);
		std::cout << std::endl << "        graph has " << terminals.size() << " terminals" << std::endl;

		EdgeWeightedGraphCopy<T> *algTree;
		T algCost = alg.call(graph, terminals, isTerminal, algTree);

		AssertThat(MinSteinerTreeModule<T>::isSteinerTree(graph, terminals, isTerminal, *algTree), Equals(true));

		// only check optimum approximation
		// for algorithms with factor of 2 or better
		if(factor >= 1 && factor <= 2) {
			AssertThat(algCost, Equals(cost));
			AssertThat(algTree->numberOfNodes(), Equals(tree.numberOfNodes()));
			AssertThat(algTree->numberOfEdges(), Equals(tree.numberOfEdges()));

			List<node> nodes;
			tree.allNodes(nodes);
			for(node v : nodes) {
				AssertThat(algTree->copy(tree.original(v)), !IsNull());
			}

			List<edge> edges;
			tree.allEdges(edges);
			for(edge e : edges) {
				AssertThat(algTree->copy(tree.original(e)), !IsNull());
			}
		}
		delete algTree;
	});
}

/**
 * Tests one subclass of MinSteinerTreeModule for a specific type.
 *
 * \param moduleName
 *	a human readable name/description of the module
 * \alg
 *	the Steiner tree module to be tested
 * \factor
 *	the approximation factor of this algorithm, needed for validating the results
 */
template<typename T>
void testModule(const std::string &moduleName, MinSteinerTreeModule<T> &alg, double factor)
{
	describe(moduleName, [&]() {
		int sizes[] = { 35, 50 };
		for (int n : sizes) {
			testModuleOnRandomGraph(alg, n, factor);
		}

		for_each_file("steiner", [&](const string &filename){
			// optimal solution value is extracted from the filename
			string tmp = filename.substr(0, filename.length() - 4);
			tmp = tmp.substr(tmp.find_last_of('.') + 1);
			std::stringstream ss(tmp);
			T opt = -1;
			ss >> opt;

			it(string("yields correct results on " + filename + " (optimum is " + to_string(opt) + ")"), [&](){
				EdgeWeightedGraph<T> graph;
				List<node> terminals;
				NodeArray<bool> isTerminal(graph);

				std::ifstream is(filename);
				GraphIO::readSTP(graph, terminals, isTerminal, is);

				EdgeWeightedGraphCopy<T> *algTree;
				T algCost = alg.call(graph, terminals, isTerminal, algTree);

				AssertThat(MinSteinerTreeModule<T>::isSteinerTree(graph, terminals, isTerminal, *algTree), Equals(true));
				delete algTree;
				AssertThat(algCost, IsGreaterThan(opt) || Equals(opt));
				if(factor != 0) {
					AssertThat(algCost, IsLessThan(factor*opt) || Equals(factor*opt));
				}
			});
		});
	});
}

/**
 * Registers one instance of the MinSteinerTreeDirectedCut class for each of its variants
 */
template <typename T>
static void
registerDirectedCutVariants(std::vector<ModuleTuple<T>*> &modules)
{
	for(int i = 0; i <
	  2 * // maximum flow modules
	  2 * // back cuts on/off
	  2 * // min cardinality cuts on/off
	  2 * // nested cuts on/off
	  2;   // all non-dynamic constraints on/off
	  i++) {
		MinSteinerTreeDirectedCut<T> *alg = new MinSteinerTreeDirectedCut<T>();

		std::stringstream ss;
		ss << "DirectedCut";
		int n = 2;

		if(i % n) {
			alg->setMaxFlowModule(new MaxFlowEdmondsKarp<double>());
			ss << ", Edmonds-Karp";
		} else {
			alg->setMaxFlowModule(new MaxFlowGoldbergTarjan<double>());
			ss << ", Goldberg-Tarjan";
		}
		n *= 2;

		alg->useBackCuts(i % n);
		if(i % n) { ss << ", back cuts"; }
		n *= 2;

		alg->useMinCardinalityCuts(i % n);
		if(i % n) { ss << ", min cardinality cuts"; }
		n *= 2;

		alg->useNestedCuts(i % n);
		if(i % n) { ss << ", nested cuts"; }
		n *= 2;

		alg->useDegreeConstraints(i % n);
		alg->useFlowBalanceConstraints(i % n);
		alg->useGSEC2Constraints(i % n);
		alg->useIndegreeEdgeConstraints(i % n);
		ss << (i % n ? ", all constraints enabled" : ", static constraints disabled");

		modules.push_back(new ModuleTuple<T>(ss.str(), alg, 1));
	}
}

/**
 * Registers one instance of the MinSteinerTreeZelikovsky class for each of its variants
 */
template <typename T>
static void
registerZelikovskyVariants(std::vector<ModuleTuple<T>*> &modules)
{
	using WCalc = std::tuple<std::string, typename MinSteinerTreeZelikovsky<T>::WinCalculation>;
	using TGen = std::tuple<std::string, typename MinSteinerTreeZelikovsky<T>::TripleGeneration>;
	using TRed = std::tuple<std::string, typename MinSteinerTreeZelikovsky<T>::TripleReduction>;
	using SCalc = std::tuple<std::string, typename MinSteinerTreeZelikovsky<T>::SaveCalculation>;
	using Pass = std::tuple<std::string, typename MinSteinerTreeZelikovsky<T>::Pass>;
	using APSPStrategy = std::tuple<std::string, bool>;

	std::vector<WCalc> winCalculations = {
		WCalc("absolute win function", MinSteinerTreeZelikovsky<T>::WinCalculation::absolute),
		WCalc("relative win function", MinSteinerTreeZelikovsky<T>::WinCalculation::relative)
	};
	std::vector<TGen> tripleGenStrategies = {
		TGen("exhaustive triple generation", MinSteinerTreeZelikovsky<T>::TripleGeneration::exhaustive),
		TGen("Voronoi triple generation", MinSteinerTreeZelikovsky<T>::TripleGeneration::voronoi),
		TGen("direct triple generation", MinSteinerTreeZelikovsky<T>::TripleGeneration::ondemand)
	};
	std::vector<TRed> tripleReductStrategies = {
		TRed("enabled reduction", MinSteinerTreeZelikovsky<T>::TripleReduction::on),
		TRed("disabled reduction", MinSteinerTreeZelikovsky<T>::TripleReduction::off),
	};
	std::vector<SCalc> saveCalculations = {
		SCalc("static enumeration save calculation", MinSteinerTreeZelikovsky<T>::SaveCalculation::staticEnum),
		SCalc("static LCATree save calculation", MinSteinerTreeZelikovsky<T>::SaveCalculation::staticLCATree),
		SCalc("dynamic LCATree save calculation", MinSteinerTreeZelikovsky<T>::SaveCalculation::dynamicLCATree),
		SCalc("hybrid save calculation", MinSteinerTreeZelikovsky<T>::SaveCalculation::hybrid)
	};
	std::vector<Pass> passes = {
		Pass("one-pass", MinSteinerTreeZelikovsky<T>::Pass::one),
		Pass("multi-pass", MinSteinerTreeZelikovsky<T>::Pass::multi)
	};
	std::vector<APSPStrategy> apspStrategies = {
		APSPStrategy("forced APSP", true),
		APSPStrategy("SSSP", false),
	};

	std::vector<typename decltype(winCalculations)::size_type>
	  choice = { 0, 0, 0, 0, 0, 0 },
	  maxchoice = {
	    winCalculations.size(),
	    tripleGenStrategies.size(),
	    tripleReductStrategies.size(),
	    saveCalculations.size(),
	    passes.size(),
	    apspStrategies.size(),
	  };
	enum indices {
		winIdx = 0,
		tgenIdx,
		tredIdx,
		saveIdx,
		passIdx,
		apspIdx,
	};

	auto nextChoice = [&]() {
		bool overflow;
		unsigned int i = 0;
		do {
			overflow = false;
			++choice[i];
			choice[i] %= maxchoice[i];
			if (choice[i] == 0) {
				++i;
				overflow = true;
			}
		} while (overflow && i < choice.size());
		return !overflow;
	};

	do {
		string desc = "Zelikovsky: ";

		MinSteinerTreeZelikovsky<T> *module = new MinSteinerTreeZelikovsky<T>();

		Pass pass = passes[choice[passIdx]];
		desc += std::get<0>(pass);
		module->pass(std::get<1>(pass));

		SCalc saveCalc = saveCalculations[choice[saveIdx]];
		desc += ", " + std::get<0>(saveCalc);
		module->saveCalculation(std::get<1>(saveCalc));

		TGen tripleGen = tripleGenStrategies[choice[tgenIdx]];
		desc += ", " + std::get<0>(tripleGen);
		module->tripleGeneration(std::get<1>(tripleGen));

		TRed tripleRed = tripleReductStrategies[choice[tredIdx]];
		desc += ", " + std::get<0>(tripleRed);
		module->tripleReduction(std::get<1>(tripleRed));

		WCalc winCalc = winCalculations[choice[winIdx]];
		desc += ", " + std::get<0>(winCalc);
		module->winCalculation(std::get<1>(winCalc));

		APSPStrategy apspStrategy = apspStrategies[choice[apspIdx]];
		desc += ", " + std::get<0>(apspStrategy);
		module->forceAPSP(std::get<1>(apspStrategy));

		// check for invalid configurations
		if (module->tripleGeneration() == MinSteinerTreeZelikovsky<T>::TripleGeneration::ondemand
		 && ((module->winCalculation() != MinSteinerTreeZelikovsky<T>::WinCalculation::absolute)
		  || (module->saveCalculation() == MinSteinerTreeZelikovsky<T>::SaveCalculation::hybrid)
		  || (module->tripleReduction() == MinSteinerTreeZelikovsky<T>::TripleReduction::off)
		  || (module->pass() == MinSteinerTreeZelikovsky<T>::Pass::one))) {
			delete module;
		} else {
			modules.push_back(new ModuleTuple<T>(desc, module, 11/6.0));
		};
	} while (nextChoice());
}

/**
 * Registers one instance of the MinSteinerTreeRZLoss class for each of its variants
 */
template <typename T>
static void
registerRZLossVariants(std::vector<ModuleTuple<T>*> &modules)
{
	// RZLoss for different maximum component sizes
	for(int i = 2; i < 6; i++) {
		MinSteinerTreeRZLoss<T> *alg = new MinSteinerTreeRZLoss<T>();
		int maxCompSize = i;
		std::string info = "";
		// APSP is only being used for maximum component size of 3
		if(i == 2) {
			alg->forceAPSP(true);
			info = " and forced APSP";
			maxCompSize = 3;
		}
		alg->setMaxComponentSize(maxCompSize);
		modules.push_back(new ModuleTuple<T>("RZLoss with maximum component size of " + to_string(maxCompSize) + info, alg, 2));
	}
}

/**
 * Registers one instance of the MinSteinerTreeGoemans139 class for each of its variants
 */
template <typename T>
static void
registerGoemans139Variants(std::vector<ModuleTuple<T>*> &modules)
{
	// Goemans139 for different maximum component sizes
	for(int i = 2; i < 6; i++) {
		// and for standard and stronger LP relaxation
		for (int strongerLP = 0; strongerLP < 1; ++strongerLP) { // XXX: strongerLP = 1 is temporarily disabled
			for (int use2approx = 0; use2approx < 2; ++use2approx) {
				MinSteinerTreeGoemans139<T> *alg = new MinSteinerTreeGoemans139<T>();
				int maxCompSize = i;
				std::string info = "Goemans139 with maximum component size ";
				if(i == 2) {
					alg->forceAPSP();
					maxCompSize = 3;
					info += "3 (enforced APSP)";
				} else {
					info += to_string(maxCompSize);
				}
				alg->setMaxComponentSize(maxCompSize);
				if (strongerLP) {
					alg->separateCycles();
					info += " using stronger LP";
				}
				if (use2approx) {
					alg->use2Approximation();
					info += " with upper bound";
				}
				modules.push_back(new ModuleTuple<T>(info, alg, 2));
			}
		}
	}
}

/**
 * Registers a complete Steiner test suite for a given
 * template parameter, like int or double.
 */
template<typename T>
void registerSuite(const std::string typeName)
{
	auto typeString = [&typeName](std::string str) {
		return str + string(" [") + typeName + string("]");
	};

	std::vector<ModuleTuple<T>*> modules = {
		new ModuleTuple<T>("DirectedCut default", new MinSteinerTreeDirectedCut<T>(), 1),
		new ModuleTuple<T>("Kou", new MinSteinerTreeKou<T>(), 2),
		new ModuleTuple<T>("Mehlhorn", new MinSteinerTreeMehlhorn<T>(), 2),
		new ModuleTuple<T>("RZLoss default", new MinSteinerTreeRZLoss<T>(), 2),
		new ModuleTuple<T>("Goemans139 default", new MinSteinerTreeGoemans139<T>(), 2),
		new ModuleTuple<T>("Takahashi", new MinSteinerTreeTakahashi<T>(), 2),
		new ModuleTuple<T>("Shore", new MinSteinerTreeShore<T>(), 1),
		new ModuleTuple<T>("Primal-Dual", new MinSteinerTreePrimalDual<T>(), 2),
		new ModuleTuple<T>("DualAscent", new MinSteinerTreeDualAscent<T>(), 0),
		new ModuleTuple<T>("Zelikovsky default", new MinSteinerTreeZelikovsky<T>(), 11/6.0),
	};

	registerDirectedCutVariants<T>(modules);
	registerZelikovskyVariants<T>(modules);
	registerRZLossVariants<T>(modules);
	registerGoemans139Variants<T>(modules);

	// register suites
	for(auto module : modules) {
		testModule<T>(typeString(std::get<0>(*module)), *std::get<1>(*module), std::get<2>(*module));
		delete std::get<1>(*module);
		delete module;
	}
}

go_bandit([](){
	describe("MinSteinerTreeModule", []() {
		registerSuite<int>("int");
		registerSuite<double>("double");
	});
});
