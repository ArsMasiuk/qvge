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

template<typename T>
struct ModuleData {
	//! a human-readable name/description of the module
	std::string name;
	//! the Steiner tree module to be tested
	std::unique_ptr<MinSteinerTreeModule<T>> alg;
	//! the approximation factor of this algorithm, needed for validating the results
	double ratio;
	//! the sizes (number of nodes) of the random graphs to test
	std::vector<int> sizes;
};

template<typename T>
using Modules = std::vector<ModuleData<T>>;

template<typename T>
static void addModule(Modules<T>& modules, const std::string& name, MinSteinerTreeModule<T>* alg, double ratio, std::vector<int> sizes = {35, 50}) {
	modules.emplace_back(ModuleData<T>{name, std::unique_ptr<MinSteinerTreeModule<T>>(alg), ratio, sizes});
}

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

	terminals.clear();

	int numberOfTerminals = max(3, randomNumber(n/4, n/2));
	int numberOfNonterminals = n - numberOfTerminals;
	int numberOfEdges = randomNumber(numberOfTerminals-1 + numberOfNonterminals*2, (n*(n-1))/2);

	randomTree(graph, numberOfTerminals);
	isTerminal.init(graph, false);
	for (node v : graph.nodes) {
		if (v->degree() == 1) {
			isTerminal[v] = true;
		}
	}
	for (edge e : graph.edges) {
		graph.setWeight(e, 1);
	}

	tree.init(graph);
	T result = tree.numberOfEdges();

	for(int i = numberOfTerminals-1; i < numberOfEdges;) {
		node v = graph.chooseNode();
		node u = graph.chooseNode([&](node w) { return w != v; });
		OGDF_ASSERT(u != nullptr);

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

	MinSteinerTreeModule<T>::getTerminals(terminals, graph, isTerminal);

	OGDF_ASSERT(terminals.size() <= numberOfTerminals);
	OGDF_ASSERT(graph.numberOfEdges() == numberOfEdges);
	OGDF_ASSERT(tree.numberOfNodes() == numberOfTerminals);
	OGDF_ASSERT(tree.numberOfEdges() == numberOfTerminals - 1);
	OGDF_ASSERT(tree.weight(tree.firstEdge()) == 1);
	OGDF_ASSERT(tree.weight(tree.lastEdge()) == 1);
	OGDF_ASSERT(graph.numberOfNodes() == n);
	OGDF_ASSERT(isSimpleUndirected(graph));
	OGDF_ASSERT(isConnected(graph));
	return result;
}

/**
 * Generates a random Steiner tree instance.
 *
 * \param n number of nodes
 * \param graph the resulting graph
 * \param terminals this list will hold all terminals
 * \param isTerminal stores which node is a terminal
 */
template<typename T>
void randomSteinerTreeInstance(
  int n,
  EdgeWeightedGraph<T> &graph,
  List<node> &terminals,
  NodeArray<bool> &isTerminal)
{
	OGDF_ASSERT(n >= 3);

	randomSimpleConnectedGraph(graph, n, randomNumber(2*n - 3, n*(n-1)/2));
	int numberOfTerminals = max(3, randomNumber(n/4, 2*n/3));

	for (edge e : graph.edges) {
		graph.setWeight(e, randomNumber(1, 100));
	}

	Array<node> nodes;
	graph.allNodes(nodes);
	nodes.permute();

	terminals.clear();
	isTerminal.init(graph, false);
	for (int i = 0; i < numberOfTerminals; ++i) {
		const node v = nodes[i];
		isTerminal[v] = true;
		terminals.pushBack(v);
	}
}

/**
 * Test if modules generates a valid/reasonable Steiner tree for a graph with given number of nodes
 */
template<typename T>
void testModuleOnRandomGraph(MinSteinerTreeModule<T> &alg, int n, double factor = 0)
{
	it("generates a valid Steiner tree for a random graph of " + to_string(n) + " nodes", [&]() {
		EdgeWeightedGraph<T> graph;
		NodeArray<bool> isTerminal(graph, false);
		List<node> terminals;

		randomSteinerTreeInstance(n, graph, terminals, isTerminal);
		std::cout << " (" << terminals.size() << " terminals, " << graph.numberOfEdges() << " edges)";

		EdgeWeightedGraphCopy<T>* make_solution;
		T returnedCost = alg.call(graph, terminals, isTerminal, make_solution);

		std::unique_ptr<EdgeWeightedGraphCopy<T>> solution(make_solution);

		for (node v : solution->nodes) {
			AssertThat(solution->original(v), !IsNull());
		}

		T actualCost(0);
		for (edge e : solution->edges) {
			AssertThat(solution->original(e), !IsNull());
			actualCost += solution->weight(e);
		}

		AssertThat(actualCost, Equals(returnedCost));
		AssertThat(MinSteinerTreeModule<T>::isSteinerTree(graph, terminals, isTerminal, *solution), Equals(true));
	});

	it("finds a reasonable Steiner tree for a graph of " + to_string(n) + " nodes", [&]() {
		EdgeWeightedGraph<T> graph;
		EdgeWeightedGraphCopy<T> tree;
		NodeArray<bool> isTerminal(graph, false);
		List<node> terminals;

		T cost = randomOptimalSteiner<T>(n, graph, terminals, isTerminal, tree);
		std::cout << " (" << terminals.size() << " terminals, " << graph.numberOfEdges() << " edges)";

		EdgeWeightedGraphCopy<T>* make_solution;
		T algCost = alg.call(graph, terminals, isTerminal, make_solution);
		std::unique_ptr<EdgeWeightedGraphCopy<T>> solution(make_solution);

		AssertThat(MinSteinerTreeModule<T>::isSteinerTree(graph, terminals, isTerminal, *solution), IsTrue());

		// only check optimum approximation
		// for algorithms with factor of 2 or better
		if(factor >= 1 && factor <= 2) {
			AssertThat(algCost, Equals(cost));
			AssertThat(solution->numberOfNodes(), Equals(tree.numberOfNodes()));
			AssertThat(solution->numberOfEdges(), Equals(tree.numberOfEdges()));

			List<node> nodes;
			tree.allNodes(nodes);
			for(node v : nodes) {
				AssertThat(solution->copy(tree.original(v)), !IsNull());
			}

			List<edge> edges;
			tree.allEdges(edges);
			for(edge e : edges) {
				AssertThat(solution->copy(tree.original(e)), !IsNull());
			}
		}
	});
}

/**
 * Tests one subclass of MinSteinerTreeModule for a specific type.
 */
template<typename T>
void testModule(const ModuleData<T>& module)
{
	describe(module.name, [&]() {
		for (int n : module.sizes) {
			testModuleOnRandomGraph(*module.alg, n, module.ratio);
		}

		for_each_file("steiner", [&](const ResourceFile* file){
			// optimal solution value is extracted from the filename
			string filename = file->name();
			string tmp = filename.substr(0, filename.length() - 4);
			T opt(0);
			auto pos = tmp.find_last_of('.');
			if (pos != tmp.npos) {
				tmp = tmp.substr(tmp.find_last_of('.') + 1);
				std::stringstream ss(tmp);
				ss >> opt;
			}

			it("yields correct results on " + file->fullPath() + " (optimum is " + (opt == 0 ? "unknown" : to_string(opt)) + ")", [&] {
				EdgeWeightedGraph<T> graph;
				List<node> terminals;
				NodeArray<bool> isTerminal;

				std::stringstream is{file->data()};
				GraphIO::readSTP(graph, terminals, isTerminal, is);

				EdgeWeightedGraphCopy<T>* make_solution;
				T algCost = module.alg->call(graph, terminals, isTerminal, make_solution);
				std::unique_ptr<EdgeWeightedGraphCopy<T>> solution(make_solution);

				AssertThat(MinSteinerTreeModule<T>::isSteinerTree(graph, terminals, isTerminal, *solution), IsTrue());
				if (opt > 0) {
					AssertThat(algCost, IsGreaterThan(opt) || Equals(opt));
					if (module.ratio != 0) {
						AssertThat(algCost, IsLessThan(module.ratio*opt) || Equals(module.ratio*opt));
					}
				}
			});
		});
	});
}

struct MaxFlowFactoryBase {
	virtual MaxFlowModule<double>* create() = 0;
	virtual ~MaxFlowFactoryBase() = default;
};
template<typename MaxFlowModuleType>
struct MaxFlowFactory : MaxFlowFactoryBase {
	MaxFlowModule<double>* create() override {
		return new MaxFlowModuleType();
	}
};

/**
 * Registers one instance of the MinSteinerTreeDirectedCut class for each of its variants
 */
template <typename T>
static void
registerDirectedCutVariants(Modules<T> &modules)
{
	using AlgPair = std::pair<MaxFlowFactoryBase*, std::string>;
	std::unique_ptr<MaxFlowFactoryBase> flowEK(new MaxFlowFactory<MaxFlowEdmondsKarp<double>>());
	std::unique_ptr<MaxFlowFactoryBase> flowGT(new MaxFlowFactory<MaxFlowEdmondsKarp<double>>());

	using BoolPair = std::pair<bool, std::string>;
	struct VerboseTrueFalse : public std::vector<BoolPair> {
		VerboseTrueFalse(std::string&& trueString, std::string&& falseString = "") : std::vector<BoolPair>({{true, trueString}, {false, falseString}}) {}
	};

	for (auto maxFlow : {AlgPair{flowEK.get(), "Edmonds-Karp"}, AlgPair{flowGT.get(), "Goldberg-Tarjan"}}) {
		for (auto useBackCuts : VerboseTrueFalse{", back cuts"}) {
			for (auto useMinCardinalityCuts : VerboseTrueFalse{", min cardinality cuts"}) {
				for (auto useNestedCuts : VerboseTrueFalse{", nested cuts"}) {
					for (auto useExtraConstraints : VerboseTrueFalse{"all extra constraints", "only necessary constraints"}) {
						MinSteinerTreeDirectedCut<T> *alg = new MinSteinerTreeDirectedCut<T>();

						std::stringstream ss;
						ss << "DirectedCut";

						alg->setMaxFlowModule(maxFlow.first->create());
						ss << ", " << maxFlow.second;

						alg->useBackCuts(useBackCuts.first);
						ss << useBackCuts.second;

						alg->useMinCardinalityCuts(useMinCardinalityCuts.first);
						ss << useMinCardinalityCuts.second;

						alg->useNestedCuts(useNestedCuts.first);
						ss << useNestedCuts.second;

						alg->useDegreeConstraints(useExtraConstraints.first);
						alg->useFlowBalanceConstraints(useExtraConstraints.first);
						alg->useGSEC2Constraints(useExtraConstraints.first);
						alg->useIndegreeEdgeConstraints(useExtraConstraints.first);
						ss << ", " << useExtraConstraints.second;

						addModule(modules, ss.str(), alg, 1, {12, 30});
					}
				}
			}
		}
	}
}

/**
 * Registers one instance of the MinSteinerTreeZelikovsky class for each of its variants
 */
template <typename T>
static void
registerZelikovskyVariants(Modules<T> &modules)
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
			addModule(modules, desc, module, 11/6.0);
		};
	} while (nextChoice());
}

/**
 * Registers one instance of the MinSteinerTreeRZLoss class for each of its variants
 */
template <typename T>
static void
registerRZLossVariants(Modules<T> &modules)
{
	// RZLoss for different maximum component sizes
	for(int maxCompSize = 3; maxCompSize < 6; ++maxCompSize) {
		MinSteinerTreeRZLoss<T> *alg = new MinSteinerTreeRZLoss<T>();
		// APSP is only being used for maximum component size of 3
		alg->setMaxComponentSize(maxCompSize);
		addModule(modules, "RZLoss with maximum component size of " + to_string(maxCompSize), alg, 2, {14, 25});
	}
}

/**
 * Registers one instance of the MinSteinerTreeGoemans139 class for each of its variants
 */
template <typename T>
static void
registerGoemans139Variants(Modules<T> &modules)
{
	// Goemans139 for different maximum component sizes
	for(int maxCompSize = 3; maxCompSize < 6; ++maxCompSize) {
		// and for standard and stronger LP relaxation
		for (int strongerLP = 0; strongerLP < 2; ++strongerLP) {
			for (int use2approx = 0; use2approx < 2; ++use2approx) {
				MinSteinerTreeGoemans139<T> *alg = new MinSteinerTreeGoemans139<T>();
				std::string info = "Goemans139 with maximum component size ";
				info += to_string(maxCompSize);
				alg->setMaxComponentSize(maxCompSize);
				if (strongerLP) {
					alg->separateCycles();
					info += " using stronger LP";
				}
				if (use2approx) {
					alg->use2Approximation();
					info += " with upper bound";
				}
				addModule(modules, info, alg, 2, {14, 25});
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
	describe("for graphs with " + typeName + "-typed costs:", [] {
		Modules<T> modules;
		addModule(modules, "DirectedCut default", new MinSteinerTreeDirectedCut<T>(), 1);
		addModule(modules, "Kou", new MinSteinerTreeKou<T>(), 2);
		addModule(modules, "Mehlhorn", new MinSteinerTreeMehlhorn<T>(), 2);
		addModule(modules, "RZLoss default", new MinSteinerTreeRZLoss<T>(), 2);
		addModule(modules, "Goemans139 default", new MinSteinerTreeGoemans139<T>(), 2);
		addModule(modules, "Takahashi", new MinSteinerTreeTakahashi<T>(), 2);
		addModule(modules, "Shore", new MinSteinerTreeShore<T>(), 1, {10, 20});
		addModule(modules, "Primal-Dual", new MinSteinerTreePrimalDual<T>(), 2);
		addModule(modules, "DualAscent", new MinSteinerTreeDualAscent<T>(), 0);
		addModule(modules, "Zelikovsky default", new MinSteinerTreeZelikovsky<T>(), 11/6.0);

		registerDirectedCutVariants<T>(modules);
		registerZelikovskyVariants<T>(modules);
		registerRZLossVariants<T>(modules);
		registerGoemans139Variants<T>(modules);

		// register suites
		for (auto& module : modules) {
			testModule<T>(module);
			module.alg.reset();
		}
	});
}

go_bandit([](){
	describe("Steiner tree algorithms", []() {
		registerSuite<int>("int");
		registerSuite<double>("double");
	});
});
