/** \file
 * \brief Classes for solving the Steiner tree problem exactly
 * with a branch&cut algorithm. The used ILP formulation
 * is the directed cut formulation.
 *
 * The implementation follows the ideas from the literature,
 * cf., e.g.,
 * T. Polzin, S. V. Daneshmand:
 * "Improved algorithms for the Steiner problem in networks"
 * or
 * T. Koch, A. Martin:
 * "Solving Steiner tree problems in graphs to optimality"
 *
 * \author Bernd Zey
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

#pragma once

#include <ogdf/external/abacus.h>

#include <ogdf/basic/Graph.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraph.h>
#include <ogdf/graphalg/steiner_tree/EdgeWeightedGraphCopy.h>
#include <ogdf/basic/Logger.h>

#include <memory>
#include <ogdf/graphalg/MinSTCutMaxFlow.h>

#include <ogdf/lib/abacus/opensub.h>
// heuristics, approximation algorithms:
#include <ogdf/graphalg/MinSteinerTreeTakahashi.h>


// turn on/off logging for STP b&c algorithm
//#define OGDF_STP_EXACT_LOGGING

// TODO: Add module option for max flow algorithm

namespace ogdf {

//! This class implements the Directed Cut Integer Linear Program for the Steiner tree problem.
/**
 * @ingroup ga-steiner
 */
template<typename T>
class MinSteinerTreeDirectedCut : public MinSteinerTreeModule<T> {
protected:
	// all the settings
	const char *m_configFile;
	double m_eps;
#ifdef OGDF_STP_EXACT_LOGGING
	Logger::Level m_outputLevel;
#endif
	std::unique_ptr<MaxFlowModule<double>> m_maxFlowModuleOption;
	bool m_addDegreeConstraints;
	bool m_addIndegreeEdgeConstraints;
	bool m_addGSEC2Constraints;
	bool m_addFlowBalanceConstraints;
	int m_maxNrAddedCuttingPlanes;
	bool m_shuffleTerminals;
	bool m_backCutComputation;
	bool m_nestedCutComputation;
	int m_separationStrategy;
	int m_saturationStrategy;
	bool m_minCardinalityCuts;
	int m_callPrimalHeuristic;
	MinSteinerTreeModule<double> *m_primalHeuristic;
	int m_poolSizeInitFactor;

	// Abacus LP classes
	class Sub;
	class EdgeConstraint;
	class DegreeConstraint;
	class DegreeEdgeConstraint;
	class DirectedCutConstraint;
	class EdgeVariable;

public:
	class Master;

	// a lot of setter methods
	//! Set the epsilon for the LP
	void setEpsilon(double eps) {
		m_eps = eps;
	}
	//! Set a configuration file to use. The contents of the configuration file can override all other used options.
	void setConfigFile(const char *configfile) {
		m_configFile = configfile;
	}
#ifdef OGDF_STP_EXACT_LOGGING
	//! Set the output level, higher values result in less output
	void setOutputLevel(Logger::Level outputLevel)
	{
		m_outputLevel = outputLevel;
	}
#endif
	//! Set the maximum flow module to be used for separation
	void setMaxFlowModule(MaxFlowModule<double> *module)
	{
		m_maxFlowModuleOption.reset(module);
	}
	//! Switch usage of degree constraints (like indeg <= 1) on or off
	void useDegreeConstraints(bool b)
	{
		m_addDegreeConstraints = b;
	}
	//! Switch usage of indegree edge constraints (indeg(v) >= outgoing edge(v,x) for all x) on or off
	void useIndegreeEdgeConstraints(bool b)
	{
		m_addIndegreeEdgeConstraints = b;
	}
	//! Switch usage of constraints x_uv + x_vu <= 1 on or off
	void useGSEC2Constraints(bool b)
	{
		m_addGSEC2Constraints = b;
	}
	//! Switch usage of flow balance constraints on or off
	void useFlowBalanceConstraints(bool b)
	{
		m_addFlowBalanceConstraints = b;
	}
	//! Set maximum number of added cutting planes per iteration
	void setMaxNumberAddedCuttingPlanes(int b)
	{
		m_maxNrAddedCuttingPlanes = b;
	}
	//! Switch terminal shuffling before separation on or off
	void useTerminalShuffle(bool b)
	{
		m_shuffleTerminals = b;
	}
	//! Switch computation of back-cuts on or off
	void useBackCuts(bool b)
	{
		m_backCutComputation = b;
	}
	//! Switch computation of nested cuts on or off
	void useNestedCuts(bool b)
	{
		m_nestedCutComputation = b;
	}
	//! Set separation strategy for nested cuts
	void setSeparationStrategy(int b)
	{
		m_separationStrategy = b;
	}
	//! Set saturation strategy for nested cuts
	void setSaturationStrategy(int b)
	{
		m_saturationStrategy = b;
	}
	//! Switch usage of the cardinality heuristic (minimum-cardinality cuts) on or off
	void useMinCardinalityCuts(bool b)
	{
		m_minCardinalityCuts = b;
	}
	//! Set the module option for the primal heuristic (use MinSteinerTreeModule<double> types). Default: MinSteinerTreeModuleTakahashi
	void setPrimalHeuristic(MinSteinerTreeModule<double> *b) {
		m_primalHeuristic = b;
	}
	//! Set primal heuristic call strategy
	void setPrimalHeuristicCallStrategy(int b)
	{
		OGDF_ASSERT(b >= 0);
		OGDF_ASSERT(b <= 2);
		m_callPrimalHeuristic = b;
	}
	//! Set factor for the initial size of the cutting pool
	void setPoolSizeInitFactor(int b)
	{
		m_poolSizeInitFactor = b;
	}

	MinSteinerTreeDirectedCut()
	  : m_configFile(nullptr)
	  , m_eps(1e-6)
#ifdef OGDF_STP_EXACT_LOGGING
	  , m_outputLevel(Logger::Level::Default)
#endif
	  , m_maxFlowModuleOption(new MaxFlowGoldbergTarjan<double>())
	  , m_addDegreeConstraints(true)
	  , m_addIndegreeEdgeConstraints(true)
	  , m_addGSEC2Constraints(true)
	  , m_addFlowBalanceConstraints(true)
	  , m_maxNrAddedCuttingPlanes(500)
	  , m_shuffleTerminals(true)
	  , m_backCutComputation(true)
	  , m_nestedCutComputation(true)
	  , m_separationStrategy(1)
	  , m_saturationStrategy(1)
	  , m_minCardinalityCuts(true)
	  , m_callPrimalHeuristic(1)
	  , m_primalHeuristic(nullptr)
	  , m_poolSizeInitFactor(5)
	{
	}

protected:
	virtual T computeSteinerTree(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal,
			EdgeWeightedGraphCopy<T> *&finalSteinerTree) override;
};

//! %Master problem of Steiner tree branch&cut algorithm
template<typename T>
class MinSteinerTreeDirectedCut<T>::Master : public abacus::Master, public Logger
{
public:
	/**
	 * \brief Constructor of the master problem
	 * \param wG:         	the underlying undirected edge weighted graph.
	 *						Since we work on the bidirection we construct a new graph.
	 * \param terminals:  	list of terminals
	 * \param isTerminal: 	boolean array indicating whether a node is a terminal
	 * \param eps:         epsilon precision
	 * \param relaxed:    	true if the relaxed problem should be solved
	 */
	Master(const EdgeWeightedGraph<T> &wG,
		const List<node> &terminals,
		const NodeArray<bool> &isTerminal,
		double eps,
		bool relaxed = false);

	//! destructor
	virtual ~Master()
	{
		delete[] m_bestSolution;
		delete[] m_terminals;
		delete[] m_nodes;
		delete[] m_edges;
		delete m_pGraph;
		delete m_pWeightedGraphPH;
		delete m_pCutPool;
	}

	//! Set the config file to use that overrides all other settings
	void setConfigFile(const char *filename)
	{
		m_configfile = filename;
	}
#ifdef OGDF_STP_EXACT_LOGGING
	//! Set the output level
	void setOutputLevel(Level outputLevel)
	{
		this->globalLogLevel(Level::Default);
		this->localLogMode(LogMode::Log);
		if (outputLevel >= Level::Minor
		 && outputLevel <= Level::Force) {
			this->localLogLevel((Level)outputLevel);
			this->globalMinimumLogLevel((Level)outputLevel);
		}
	}
#endif
	//! Set the maximum flow module to be used for separation
	void setMaxFlowModule(MaxFlowModule<double> *module) {
		m_maxFlowModule = module;
	}

	//! Get the maximum flow module used by separation algorithm
	MaxFlowModule<double> *getMaxFlowModule()
	{
		return m_maxFlowModule;
	}

	//! Switch usage of degree constraints (like indeg <= 1) on or off
	void useDegreeConstraints(bool b)
	{
		m_addDegreeConstraints = b;
	}
	//! Switch usage of indegree edge constraints (indeg(v) >= outgoing edge(v,x) for all x) on or off
	void useIndegreeEdgeConstraints(bool b)
	{
		m_addIndegreeEdgeConstraints = b;
	}
	//! Switch usage of constraints x_uv + x_vu <= 1 on or off
	void useGSEC2Constraints(bool b)
	{
		m_addGSEC2Constraints = b;
	}
	//! Switch usage of flow balance constraints on or off
	void useFlowBalanceConstraints(bool b)
	{
		m_addFlowBalanceConstraints = b;
	}
	//! Set maximum number of added cutting planes per iteration
	void setMaxNumberAddedCuttingPlanes(int b)
	{
		m_maxNrAddedCuttingPlanes = b;
		this->maxConAdd(m_maxNrAddedCuttingPlanes);
		this->maxConBuffered(m_maxNrAddedCuttingPlanes);
	}
	//! Switch terminal shuffling before separation on or off
	void useTerminalShuffle(bool b)
	{
		m_shuffleTerminals = b;
	}
	//! Switch computation of back-cuts on or off
	void useBackCuts(bool b)
	{
		m_backCutComputation = b;
	}
	//! Switch computation of nested cuts on or off
	void useNestedCuts(bool b)
	{
		m_nestedCutComputation = b;
	}
	//! Set separation strategy for nested cuts
	void setSeparationStrategy(int b)
	{
		OGDF_ASSERT(b >= 1);
		OGDF_ASSERT(b <= 2);
		m_separationStrategy = b;
	}
	//! Set saturation strategy for nested cuts
	void setSaturationStrategy(int b)
	{
		OGDF_ASSERT(b >= 1);
		OGDF_ASSERT(b <= 2);
		m_saturationStrategy = b;
	}
	//! Switch usage of the cardinality heuristic (minimum-cardinality cuts) on or off
	void useMinCardinalityCuts(bool b)
	{
		m_minCardinalityCuts = b;
	}
	//! Set primal heuristic call strategy
	void setPrimalHeuristicCallStrategy(int b)
	{
		OGDF_ASSERT(b >= 0);
		OGDF_ASSERT(b <= 2);
		m_callPrimalHeuristic = b;
	}
	//! Set factor for the initial size of the cutting pool
	void setPoolSizeInitFactor(int b)
	{
		m_poolSizeInitFactor = b;
	}

	//! Set the module option for the primal heuristic
	void setPrimalHeuristic(MinSteinerTreeModule<double> *pMinSteinerTreeModule) {
		m_primalHeuristic.reset(pMinSteinerTreeModule);
	}
	//! the primal heuristic module
	std::unique_ptr<MinSteinerTreeModule<double>> &getPrimalHeuristic() {return m_primalHeuristic;}

	//! the non-duplicate cutpool for the separated Steiner cuts
	abacus::NonDuplPool<abacus::Constraint, abacus::Variable> *cutPool() {return m_pCutPool;}

	//! generates the first subproblem
	virtual abacus::Sub *firstSub() override
	{
		return new Sub(this);
	}

	//! returns true iff original edge is contained in optimum solution
	bool isSolutionEdge(edge e) const
	{
		return m_isSolutionEdge[m_mapToBidirectedGraph1[e]]
		    || m_isSolutionEdge[m_mapToBidirectedGraph2[e]];
	}

	//! the directed graph, i.e., the bidirection of the input graph
	const Graph &graph() const {return *m_pGraph;}

	//! number of nodes of the graph
	int nNodes() const {return m_pGraph->numberOfNodes();}
	//! returns the number of edges
	int nEdges() const {return m_pGraph->numberOfEdges();}
	//! returns number of undirected edges, i.e., nEdges()/2
	int nEdgesU() const {return m_nEdgesU;}
	//! the designated root node (special terminal)
	node rootNode() const {return m_root;}

	//! number of terminals
	int nTerminals() const {return m_nTerminals;}
	//! terminals in an array
	const node *terminals() const {return m_terminals;}
	//! get terminal with index i
	node terminal(int i) const {return m_terminals[i];}
	//! true if n is a terminal
	bool isTerminal(node n) const {return m_isTerminal[n];}
	//! boolean array of terminal status
	const NodeArray<bool> isTerminal() const {return m_isTerminal;}

	//! edge -> id of lp variable
	int edgeID(edge e) const {return m_edgeIDs[e];}
	//! npde -> id of lp variable
	int nodeID(node n) const {return m_nodeIDs[n];}

	//! nodes of the graph
	node *nodes() const {return m_nodes;}
	//! lp variable ids of nodes
	const NodeArray<int> &nodeIDs() const {return m_nodeIDs;}
	//! lp variable ids of edges
	const EdgeArray<int> &edgeIDs() const {return m_edgeIDs;}

	//! id -> edge
	edge getEdge(int i) const {return m_edges[i];}
	//! id -> node
	node getNode(int i) const {return m_nodes[i];}

	//! edge costs
	const EdgeArray<double> &capacities() const {return m_capacities;}
	//! costs for edge e
	double capacities(edge e) const {return m_capacities[e];}

	//! the twin edge, i.e. twin[(u,v)] = (v,u)
	edge twin(edge e) const {return m_twin[e];}
	// the twin edge array
	const EdgeArray<edge> &twins() const {return m_twin;}

	//! returns the variable assigned to edge e
	EdgeVariable *getVar(edge e) const {return m_edgeToVar[e];}
	//! returns the variable assigned to the twin of edge e
	EdgeVariable *getVarTwin(edge e) const {return m_edgeToVar[m_twin[e]];}

	//! solve relaxed LP or ILP
	bool relaxed() const {return m_relaxed;}

	//! solution value after solving the problem, i.e., returns final primal bound
	double solutionValue() const {return this->primalBound();}
	//! the best found solution
	double *bestSolution() const {return m_bestSolution;}
	//! updates best found solution
	void updateBestSolution(double *values);
	//! updates best found solution by list of edges
	void updateBestSolutionByEdges(const List<edge> &sol);

	//! solution value of the root
	void setRelaxedSolValue(double val) {m_relaxedSolValue = val;}
	//! nr of iterations in the root node
	void setNIterRoot(int val) {m_nIterRoot = val;}

	//! maximum nr of cutting planes
	int maxNrAddedCuttingPlanes() const {return m_maxNrAddedCuttingPlanes;}
	//! parameter: nested cut computation
	bool computeNestedCuts() const {return m_nestedCutComputation;}
	//! parameter: back cut computation
	bool computeBackCuts() const {return m_backCutComputation;}
	//! parameter: compute minimum cardinality cuts
	bool minCardinalityCuts() const {return m_minCardinalityCuts;}
	//! parameter: call primal heuristic yes/no
	bool callPrimalHeuristic() const {return m_callPrimalHeuristic > 0;}
	//! strategy for calling primal heuristic (PH)
	int callPrimalHeuristicStrategy() const {return m_callPrimalHeuristic;}
	//! strategy for separating directed Steiner cuts; Only relevant for nested cuts
	int separationStrategy() const {return m_separationStrategy;}
	//! strategy for saturating edges during separation; Only relevant for nested cuts
	int saturationStrategy() const {return m_saturationStrategy;}

	//! shuffle ordering of terminals before each separation routine
	bool shuffleTerminals() const {return m_shuffleTerminals;}

	//! the maximum pool size during the algorithm
	int maxPoolSize() const {return m_maxPoolSize;}
	//! checks if current pool size is maximum and sets it if necessary
	void checkSetMaxPoolSize()
	{
		if (m_poolSizeMax < m_pCutPool->size())
			m_poolSizeMax = m_pCutPool->size();
	}
	//! initial pool size
	int poolSizeInit() const {return m_poolSizeInit;}

	//! increases the number of separated directed cuts
	void incNrCutsTotal(int val) {m_nrCutsTotal += val;}
	//! increases the number of separated directed cuts by 1
	void incNrCutsTotal() {m_nrCutsTotal++;}
	//! total number of separated directed cuts
	int nrCutsTotal() const {return m_nrCutsTotal;}

	/*
	 * Methods for primal heuristics.
	 * Naming convention: suffix "PH"
	 */
	/*
	 * Edge weighted bidirected graph; used and modified for primal heuristics.
	 * Required for calling MinSteinerTreeModule algorihms
	 */
	EdgeWeightedGraph<double> &weightedGraphPH() {return *m_pWeightedGraphPH;}
	//! list of terminals (in m_pWeightedGraphPH)
	const List<node> &terminalListPH() const {return m_terminalListPH;}
	//! terminal yes/no (in m_pWeightedGraphPH)
	const NodeArray<bool> &isTerminalPH() const {return m_isTerminalPH;}
	//! root node (in m_pWeightedGraphPH)
	node rootNodePH() const {return m_rootPH;}
	//! edge mapping m_pGraph -> m_pWeightedGraphPH
	edge edgeGToWgPH(edge e) const {return m_edgesGToWgPH[e];}
	//! edge mapping m_pWeightedGraphPH -> m_pGraph
	edge edgeWgToGPH(edge e) const {return m_edgesWgToGPH[e];}

	/*
	 * some additional timers
	 */
	//! timer for separation
	StopwatchWallClock *separationTimer() {return &m_separationTimer;}
	//! timer for minimum st-cut computations. Measures updates + algorithm
	StopwatchWallClock *timerMinSTCut() {return &m_timerMinSTCut;}
	//! timer for primal heuristics
	StopwatchWallClock *primalHeuristicTimer() {return &m_primalHeuristicTimer;}

protected:
	//! insert variables and base constraints
	virtual void initializeOptimization() override;
	//! store solution in EdgeArray
	virtual void terminateOptimization() override;
	//! read/set parameters from file
	virtual void initializeParameters() override;

private:
	MaxFlowModule<double> *m_maxFlowModule;

	//! problem dependent config file
	const char *m_configfile;

	//! Algorithm used for the primal heuristic
	std::unique_ptr<MinSteinerTreeModule<double>> m_primalHeuristic;

	//! parameter: indicates whether we solve the relaxed problem (LP) or the ILP
	bool m_relaxed;

	//! statistics: solution value of the relaxed master problem
	double m_relaxedSolValue;

	//! statistics: nr of iterations in the root node of the b&b tree
	int m_nIterRoot;

	//! the original weighted graph
	const EdgeWeightedGraph<T> &m_wG;

	//! the bidirected graph
	Graph *m_pGraph;

	//! number of undirected edges
	int m_nEdgesU;
	//! id -> edge
	edge *m_edges;
	//! edge -> id
	EdgeArray<int> m_edgeIDs;
	//! the twin edges (u,v) <-> (v,u)
	EdgeArray<edge> m_twin;

	//! edge costs
	EdgeArray<double> m_capacities;
	//! edge -> lp variable
	EdgeArray<EdgeVariable*> m_edgeToVar;

	//! the undirected edge in the original graph for each arc in m_pGraph
	EdgeArray<edge> m_mapToOrigGraph;
	//! the first directed arc in m_pGraph for an original edge
	EdgeArray<edge> m_mapToBidirectedGraph1;
	//! the second directed arc in m_pGraph for an original edge
	EdgeArray<edge> m_mapToBidirectedGraph2;

	//! id -> node
	node *m_nodes;
	//! node -> id
	NodeArray<int> m_nodeIDs;
	//! node is terminal yes/no
	NodeArray<bool> m_isTerminal;
	//! nr of terminals
	int m_nTerminals;
	//! terminal index -> terminal node
	node *m_terminals;

	 //! the virtual root of our graph. This node is a terminal.
	node m_root;

	//! best found solution
	double *m_bestSolution;
	EdgeArray<bool> m_isSolutionEdge;

	/*
	 * Stuff for primal heuristics
	 */
	//! edge weighted bidirected graph; used and modified for primal heuristics
	EdgeWeightedGraph<double> *m_pWeightedGraphPH;
	//! list of terminal nodes (in m_pWeightedGraphPH)
	List<node> m_terminalListPH;
	//! is terminal yes/no (in m_pWeightedGraphPH)
	NodeArray<bool> m_isTerminalPH;
	//! node mapping m_pGraph -> m_pWeightedGraphPH
	NodeArray<node> m_nodesGToWgPH;
	//! edge mapping m_pGraph -> m_pWeightedGraphPH
	EdgeArray<edge> m_edgesGToWgPH;
	//! edge mapping m_pWeightedGraphPH -> m_pGraph
	EdgeArray<edge> m_edgesWgToGPH;
	//! root node in m_pWeightedGraphPH
	node m_rootPH;

	//! the non-duplicate cut pool for the directed Steiner cuts
	abacus::NonDuplPool<abacus::Constraint, abacus::Variable> *m_pCutPool;
	//! parameter: factor for the initial size of the cutting pool
	int m_poolSizeInitFactor;
	//! size of initial pool
	int m_poolSizeInit;
	//! maximal size of the pool
	int m_poolSizeMax;
	//! statistic number of cuts in pool
	int m_maxPoolSize;
	//! total number of separated directed cuts
	int m_nrCutsTotal;

	//! parameter: add GSEC2 constraints yes/no, i.e. x_uv + x_vu <= 1
	bool m_addGSEC2Constraints;
	//! parameter: add constraints concerning the indegree of a node, like: indeg <= 1 for all vertices
	bool m_addDegreeConstraints;
	//! add constraints concerning the indegree of a node w.r.t. one outgoing edge: indeg(v) >= outgoing edge(v,x) for all x
	bool m_addIndegreeEdgeConstraints;
	//! parameter: add flow balance constraints for Steiner nodes n: outdeg(n) >= indeg(n)
	bool m_addFlowBalanceConstraints;

	//! parameter: maximum nr of cutting planes per iteration
	int m_maxNrAddedCuttingPlanes;
	//! parameter: shuffle the list of terminals right before separation
	bool m_shuffleTerminals;
	//! parameter: compute back cuts yes/no i.e., outgoing edges of the root-set
	bool m_backCutComputation;
	//!parameter: compute nested cuts yes/no i.e., saturate all cut edges and recompute the mincut
	bool m_nestedCutComputation;
	//! parameter: separation strategy, only important if nested cuts are computed
	/*
	 * basic strategy:
	 *    Computes mincut between the root and a terminal. Saturates cutedges afterwards.
	 *    Continues with same terminal until no violated cut is found. Considers next terminal.
	 * 1: When switching to next terminal saturated edges remain saturated  (default)
	 * 2: When switching to next terminal saturated edges are reset to original capacity
	 */
	int m_separationStrategy;
	//! parameter: saturation strategy, only important if nested cuts are computed
	/*
	 *    for all cutedges e:
	 * 1: capacity[e]=1   (default)
	 * 2: capacity[e]=1/C with C=nr of cutedges
	 */
	int m_saturationStrategy;

	//! parameter: compute minimum cardinality cuts
	/*
	 * adds epsilon to each arc capacity before computing the minimum cut
	 */
	bool m_minCardinalityCuts;

	//! parameter: primal heuristic (PH) call strategy
	/*
	 * 0: no PH
	 * 1: call PH right before branchting
	 * 2: call PH every iteration
	 */
	int m_callPrimalHeuristic;

	//! timer for separation
	StopwatchWallClock m_separationTimer;
	//! timer for minimum st-cut computations. Measures updates + algorithm
	StopwatchWallClock m_timerMinSTCut;
	//! timer for primal heuristics
	StopwatchWallClock m_primalHeuristicTimer;

	Master(const Master &rhs);
	const Master &operator= (const Master &rhs);
};

//! Subproblem of Steiner tree algorithm
template<typename T>
class MinSteinerTreeDirectedCut<T>::Sub : public abacus::Sub
{
public:
	//! Constructor for the root problem of the b&b tree
	explicit Sub(abacus::Master *master);
	//! Constructor for non-root problems of the b&b tree
	Sub(abacus::Master *master, abacus::Sub *father, abacus::BranchRule *branchRule);
	//!
	virtual ~Sub()
	{
	}

#ifdef OGDF_STP_EXACT_LOGGING
	//! prints the current solution. Uses Loglevel Minor and Medium
	void printCurrentSolution(bool onlyNonZeros = true);
#endif

protected:
	//! checks if the current solution is feasible, i.e., calls mySeparate()
	virtual bool feasible() override;

	//! calls mySeparate() if mySeparate wasn't called in another procedure
	virtual int separate() override
	{
		if (m_alreadySeparated == -1) {
			m_alreadySeparated = mySeparate();
		}
		return m_alreadySeparated;
	}
	//! separation procedure
	int mySeparate();

	//! primal heuristic procedure
	void myImprove();

#ifdef OGDF_STP_EXACT_LOGGING
	//! prints/logs main infos at the beginning of feasible()
	void printMainInfosInFeasible(bool header = false) const;
#endif

private:
#ifdef OGDF_STP_EXACT_LOGGING
	//! prints the constaint
	void printConstraint(abacus::Constraint *constraint, Logger::Level level = Logger::Level::Default) const;
#endif

	//! the master problem of the b&c algorithm
	Master *m_pMaster;

	//! nr of already separated cuts, default is -1
	int m_alreadySeparated;

	//! maximum nr of cutting planes to be added
	int m_maxNrCuttingPlanes;
	//!
	bool m_computeNestedCuts;
	//!
	int m_separationStrategy;
	//!
	int m_saturationStrategy;
	//!
	bool m_computeBackCuts;
	//!
	bool m_shuffleTerminals;
	//!
	bool m_minCardinalityCuts;

	//! Strategy for primal heuristic (PH) calls
	/*
	 * 0: no PH
	 * 1: call PH right before branchting
	 * 2: call PH every iteration
	 */
	int m_callPrimalHeuristic;

	//! generates a b&b node
	virtual Sub *generateSon(abacus::BranchRule *rule) override
	{
		return new Sub(master_, this, rule);
	}
};

//! Variable for directed edges
template<typename T>
class MinSteinerTreeDirectedCut<T>::EdgeVariable : public abacus::Variable
{
public:
	EdgeVariable(abacus::Master *master,
#if 0
					const abacus::Sub *sub,
#endif
					int id,
					edge e,
					double coeff,
					double lb = 0.0,
					double ub = 1.0,
					abacus::VarType::TYPE vartype = abacus::VarType::Binary
					)
	  : abacus::Variable(master, nullptr /*, sub*/, false, false, coeff, lb, ub, vartype)
	  , m_edge(e)
	  , m_id(id)
	{
	}

	//! the associated edge
	edge theEdge() const {return m_edge;}
	//! id of the edge (variable)
	int id() const {return m_id;}
	//! objective function coefficient
	double coefficient() const {return this->obj();}
	//! source node
	node source() const {return m_edge->source();}
	//! target node
	node target() const {return m_edge->target();}

private:
	//! the edge
	edge m_edge;
	//! id of the edge
	int m_id;
};


//! Constraint for edges, e.g., subtour elimination constraints of size 2 ((G)SEC2)
template<typename T>
class MinSteinerTreeDirectedCut<T>::EdgeConstraint : public abacus::Constraint
{
public:
	EdgeConstraint(abacus::Master *master,
						edge e1,
						edge e2,
						int factor = 1.0,
						abacus::CSense::SENSE sense = abacus::CSense::Less,
						double rhs = 1.0)
	  : abacus::Constraint(master, nullptr, sense, rhs, false, false, false)
	  , m_e1(e1)
	  , m_e2(e2)
	  , m_factor(factor)
	{
	}

	//! coefficient of variable in constraint
	double coeff(const abacus::Variable *v) const override
	{
		EdgeVariable *edgeVar = (EdgeVariable*)v;
		edge e = edgeVar->theEdge();
		if (e != m_e1 && e != m_e2)
			return 0.0;
		return m_factor;
	}

private:
	//! base edge and twin of m_e2, i.e., if m_e1 = (u,v) it holds m_e2 = (v,u)
	edge m_e1;
	//! twin of m_e1, i.e., if m_e1 = (u,v) it holds m_e2 = (v,u)
	edge m_e2;
	//! factor for edge coefficients in the constraint
	int m_factor;
};


//! Constraint for nodes, e.g., in/outdegree stuff
template<typename T>
class MinSteinerTreeDirectedCut<T>::DegreeConstraint : public abacus::Constraint
{
public:
	DegreeConstraint(abacus::Master *master,
						node n,
						double coeffIn,
						double coeffOut,
						abacus::CSense::SENSE sense,
						double rhs)
	  : abacus::Constraint(master, nullptr, sense, rhs, false, false, false)
	  , m_node(n)
	  , m_coeffIn(coeffIn)
	  , m_coeffOut(coeffOut)
	{
	}

	//! coefficient of variable in constraint
	virtual double coeff(const abacus::Variable *v) const override
	{
		EdgeVariable *edgeVar = (EdgeVariable*)v;
		edge e = edgeVar->theEdge();
		if (e->target() == m_node) {
			return m_coeffIn;
		} else {
			if (e->source() == m_node) {
				return m_coeffOut;
			} else {
				return 0.0;
			}
		}
	}

	//! the associated node
	node theNode() const {return m_node;}

private:
	//! the associated node
	node m_node;
	//! coefficient of ingoing edges
	double m_coeffIn;
	//! coefficient of outgoing edges
	double m_coeffOut;
};


//! Constraint for relating the indegree and one outgoing edge of a node
template<typename T>
class MinSteinerTreeDirectedCut<T>::DegreeEdgeConstraint : public abacus::Constraint
{
public:
	DegreeEdgeConstraint(abacus::Master *master,
							edge e,
							double coeffIn,
							double coeffEdge,
							abacus::CSense::SENSE sense,
							double rhs)
	  : abacus::Constraint(master, nullptr, sense, rhs, false, false, false)
	  , m_edge(e)
	  , m_coeffIn(coeffIn)
	  , m_coeffEdge(coeffEdge)
	{
	}

	//! coefficient of variable in constraint
	double coeff(const abacus::Variable *v) const override
	{
		EdgeVariable *edgeVar = (EdgeVariable*)v;
		edge e = edgeVar->theEdge();
		// the edge
		if (e->isParallelDirected(m_edge))
			return m_coeffEdge;
		// all edges to vertices x != source
		if (e->target() != m_edge->source())
			return 0.0;
		// reverse edge
		if (e->source() == m_edge->target())
			return 0.0;
		// all ingoing edges of the source (except the reverse edge, of course)
		return m_coeffIn;
	}

	//! the associated edge
	edge theEdge() const {return m_edge;}
private:
	//! the associated edge
	edge m_edge;
	//! coefficient of ingoing edges
	double m_coeffIn;
	//! coefficient of the edge
	double m_coeffEdge;
};


//! Class for directed cuts (i.e., separated Steiner cuts)
template<typename T>
class MinSteinerTreeDirectedCut<T>::DirectedCutConstraint : public abacus::Constraint
{
public:
	DirectedCutConstraint(abacus::Master *master, const Graph &g, const MinSTCutMaxFlow<double> *minSTCut, MinSTCutMaxFlow<double>::cutType _cutType)
		: abacus::Constraint(master, nullptr, abacus::CSense::Greater, 1.0, false, false, false)
		, m_pGraph(&g)
		, m_name("")
	{
#ifdef OGDF_STP_EXACT_LOGGING
		std::cout << "Creating new DirectedCutConstraint: " << std::endl;
#endif
		m_hashKey = 0;
		m_marked.init(g);
		for (node n : g.nodes) {
			if(_cutType == MinSTCutMaxFlow<double>::cutType::FRONT_CUT) {
				m_marked[n] = minSTCut->isInFrontCut(n);
#ifdef OGDF_STP_EXACT_LOGGING
				if(m_marked[n]) {
					// TODO: Use lout instead
					std::cout << "  marked node " << n << std::endl;
				}
#endif
			} else {
				OGDF_ASSERT(_cutType == MinSTCutMaxFlow<double>::cutType::BACK_CUT);
				m_marked[n] = !minSTCut->isInBackCut(n);
			}
			if (m_marked[n]) {
				m_nMarkedNodes++;
				m_hashKey += n->index();
			}
		}
		m_hashKey += m_nMarkedNodes*g.numberOfNodes()*g.numberOfNodes();
#ifdef OGDF_STP_EXACT_LOGGING
		std::cout << "  front cut edges:" << std::endl;
		for (edge e : g.edges) {
			if(minSTCut->isFrontCutEdge(e)) {
				std::cout << "    " << e << std::endl;
			}
		}
		std::cout << "  back cut edges:" << std::endl;
		for (edge e : g.edges) {
			if(minSTCut->isBackCutEdge(e)) {
				std::cout << "    " << e << std::endl;
			}
		}
#endif
	}

	double coeff(const abacus::Variable *v) const override;

	//! returns true iff the node n is separated by this cut
	bool active(node n) const {
		return m_marked[n];
	}
	//! returns true iff the edge is contained in the cut
	bool cutedge(edge e) const {
		return m_marked[e->source()] && !m_marked[e->target()];
	}
	//! the number of marked nodes
	int nMarkedNodes() const { return m_nMarkedNodes; }
	//! returns status of node n
	bool marked(node n) const { return m_marked[n]; }
	//! retuns an hashkey for the cut; required method for nonduplpool
	unsigned hashKey() const override { return m_hashKey; };
	//! tests if cuts are equal; required method for nonduplpool
	bool equal(const ConVar *cv) const override;
	//! return the name of the cut; required method for nonduplpool
	const char *name() const override { return m_name; }

private:
	//! the graph
	const Graph *m_pGraph;

	//! defines if a vertex is on the left side of the cut (false) or not
	/*
	 * A vertex is marked iff it is separated by the cut
	 * i.e., it is contained in the same set as the target
	 */
	NodeArray<bool> m_marked;

	//! number of marked nodes
	int m_nMarkedNodes;

	//! hashkey of the cut; required method for nonduplpool
	unsigned int m_hashKey;
	//! name of the cut; required method for nonduplpool
	const char *m_name;
};


template<typename T>
MinSteinerTreeDirectedCut<T>::Master::Master(
					const EdgeWeightedGraph<T> &wG,
					const List<node> &terminals,
					const NodeArray<bool> &isTerminal,
					double eps,
					bool relaxed)
  : abacus::Master("MinSteinerTreeDirectedCut::Master", true, false, abacus::OptSense::Min, eps)
  , m_maxFlowModule(nullptr)
  , m_configfile(nullptr)
  , m_relaxed(relaxed)
  , m_relaxedSolValue(-1)
  , m_nIterRoot(-1)
  , m_wG(wG)
  , m_pCutPool(nullptr)
  , m_poolSizeInitFactor(5)
  , m_poolSizeMax(0)
  , m_maxPoolSize(-1)
  , m_nrCutsTotal(0)
  , m_addGSEC2Constraints(true)
  , m_addDegreeConstraints(true)
  , m_addIndegreeEdgeConstraints(true)
  , m_addFlowBalanceConstraints(true)
  , m_maxNrAddedCuttingPlanes(500)
  , m_shuffleTerminals(true)
  , m_backCutComputation(true)
  , m_nestedCutComputation(true)
  , m_separationStrategy(1)
  , m_saturationStrategy(1)
  , m_minCardinalityCuts(true)
  , m_callPrimalHeuristic(1)
{
#ifdef OGDF_STP_EXACT_LOGGING
	lout(Level::Default)
		<< "Master::Master(): default LP solver: "
		<< this->OSISOLVER_[this->defaultLpSolver()] << std::endl;
#endif
	m_pGraph = new Graph();

	edge e1, e2;
	int i = 0;
	int t = 0;

	m_nodes = new node[m_wG.numberOfNodes()];
	m_nodeIDs.init(*m_pGraph);
	m_isTerminal.init(*m_pGraph);
	m_nTerminals = terminals.size();
	m_root = nullptr;

#ifdef OGDF_STP_EXACT_LOGGING
	lout(Level::Default)
	  << "Master::Master(): nTerminals="
	  << m_nTerminals << std::flush;
	lout(Level::Minor) << " terminals: " << std::flush;
#endif
	NodeArray<node> nodeMapping(m_wG);
	m_terminals = new node[m_nTerminals];

	for (node nOrig : m_wG.nodes) {
		node n = m_pGraph->newNode();
		nodeMapping[nOrig] = n;
		m_nodes[i] = n;
		m_nodeIDs[n] = i;
		m_isTerminal[n] = isTerminal[nOrig];
		if (m_isTerminal[n]) {
#ifdef OGDF_STP_EXACT_LOGGING
			lout(Level::Minor) << n << "," << std::flush;
#endif
			m_terminals[t++] = n;
		}
		i++;
	}
#ifdef OGDF_STP_EXACT_LOGGING
	lout(Level::Minor) << std::endl << "Master::Master(): edges: ";
#endif

	m_nEdgesU = m_wG.numberOfEdges();
	m_capacities.init(*m_pGraph);
	m_twin.init(*m_pGraph);
	m_edgeIDs.init(*m_pGraph);
	m_edges = new edge[2*m_nEdgesU];

	m_mapToOrigGraph.init(*m_pGraph);
	m_mapToBidirectedGraph1.init(m_wG);
	m_mapToBidirectedGraph2.init(m_wG);

	i = 0;
	for (edge eOrig : m_wG.edges) {
		e1 = m_pGraph->newEdge(nodeMapping[eOrig->source()],
				nodeMapping[eOrig->target()]);
		e2 = m_pGraph->newEdge(nodeMapping[eOrig->target()], nodeMapping[eOrig->source()]);
		m_capacities[e1] = m_capacities[e2] = m_wG.weight(eOrig);
		m_twin[e1] = e2;
		m_twin[e2] = e1;
		m_edges[i] = e1;
		m_edgeIDs[e1] = i++;
		m_edges[i] = e2;
		m_edgeIDs[e2] = i++;
		m_mapToOrigGraph[e1] = eOrig;
		m_mapToOrigGraph[e2] = eOrig;
		m_mapToBidirectedGraph1[eOrig] = e1;
		m_mapToBidirectedGraph2[eOrig] = e2;
#ifdef OGDF_STP_EXACT_LOGGING
		lout(Level::Minor) << " " << eOrig << "[" << e1 << ", " << e2 << "]" << std::flush;
#endif
	}
#ifdef OGDF_STP_EXACT_LOGGING
	lout(Level::Default) << std::endl;
#endif
	for (node n : m_pGraph->nodes) {
		if (m_isTerminal[n]) {
			// possibility to set the root node
			// default: terminal with highest degree
			if (!m_root)
				m_root = n;
			else {
				if (m_root->degree() < n->degree())
					m_root = n;
			}
		}
	}

#ifdef OGDF_STP_EXACT_LOGGING
	lout(Level::Medium) << "Master::Master(): m_root=" << m_root << std::endl;
#endif

	m_isSolutionEdge.init(*m_pGraph, false);
	m_bestSolution = new double[m_pGraph->numberOfEdges()];
	for (i = 0; i < m_pGraph->numberOfEdges(); i++) {
		m_bestSolution[i] = 1.0;
	}

	// stuff for "primal heuristic"
	m_pWeightedGraphPH = new EdgeWeightedGraph<double>();
	m_nodesGToWgPH.init(*m_pGraph);
	m_edgesGToWgPH.init(*m_pGraph);
	m_isTerminalPH.init(*m_pWeightedGraphPH);
	m_edgesWgToGPH.init(*m_pWeightedGraphPH);

	for (node nOrig : m_pGraph->nodes) {
		node n = m_pWeightedGraphPH->newNode();
		m_nodesGToWgPH[nOrig] = n;
		m_isTerminalPH[n] = m_isTerminal[nOrig];
		if (m_isTerminalPH[n])
			m_terminalListPH.pushBack(n);
		if (m_root == nOrig)
			m_rootPH = n;
	}

	for (edge eOrig : m_pGraph->edges) {
		edge e = m_pWeightedGraphPH->newEdge(m_nodesGToWgPH[eOrig->source()], m_nodesGToWgPH[eOrig->target()], 0.0);
		m_edgesGToWgPH[eOrig] = e;
		m_edgesWgToGPH[e] = eOrig;
	}

	// set default primal heuristic module to takahashi algorithm
	m_primalHeuristic.reset(new MinSteinerTreeTakahashi<double>());
}

template<typename T>
void
MinSteinerTreeDirectedCut<T>::Master::initializeParameters()
{
	if (m_configfile) {
		bool objectiveInteger = false;
		try {
			this->readParameters(m_configfile);
		}
		catch (AlgorithmFailureException) {
#ifdef OGDF_STP_EXACT_LOGGING
			lout(Level::Alarm)
				<< "Master::initializeParameters(): Error reading parameters."
				<< "Using default values." << std::endl;
#endif
		}
#ifdef OGDF_STP_EXACT_LOGGING
		int outputLevel;
		getParameter("OutputLevel", outputLevel);
		setOutputLevel(static_cast<Level>(outputLevel));
#endif
		int solver = (OSISOLVER) findParameter("DefaultLpSolver", 12, OSISOLVER_);
		this->defaultLpSolver((OSISOLVER)solver);
		getParameter("AddGSEC2Constraints",        m_addGSEC2Constraints);
		getParameter("AddDegreeConstraints",       m_addDegreeConstraints);
		getParameter("AddIndegreeEdgeConstraints", m_addIndegreeEdgeConstraints);
		getParameter("AddFlowBalanceConstraints",  m_addFlowBalanceConstraints);
		getParameter("MaxNrCuttingPlanes",         m_maxNrAddedCuttingPlanes);
		getParameter("ShuffleTerminals",           m_shuffleTerminals);
		getParameter("BackCutComputation",         m_backCutComputation);
		getParameter("NestedCutComputation",       m_nestedCutComputation);
		getParameter("SeparationStrategy",         m_separationStrategy);
		getParameter("SaturationStrategy",         m_saturationStrategy);
		getParameter("MinCardinalityCuts",         m_minCardinalityCuts);
		getParameter("PrimalHeuristic",            m_callPrimalHeuristic);
		getParameter("PoolSizeInitFactor",         m_poolSizeInitFactor);
		getParameter("ObjInteger",                 objectiveInteger);
		this->objInteger(objectiveInteger);
	}

#ifdef OGDF_STP_EXACT_LOGGING
	lout(Level::High)
		<< "Master::initializeParameters(): parameters:" << std::endl
		<< "\tLP-Solver                  " << OSISOLVER_[this->defaultLpSolver()] << std::endl
		<< "\tOutputLevel                " << this->localLogLevel() << std::endl
		<< "\tAddDegreeConstraints       " << m_addDegreeConstraints << std::endl
		<< "\tAddIndegreeEdgeConstraints " << m_addIndegreeEdgeConstraints << std::endl
		<< "\tAddGSEC2Constraints        " << m_addGSEC2Constraints << std::endl
		<< "\tAddFlowBalanceConstraints  " << m_addFlowBalanceConstraints << std::endl
		<< "\tMaxNrCuttingPlanes         " << m_maxNrAddedCuttingPlanes << std::endl
		<< "\tShuffleTerminals           " << m_shuffleTerminals << std::endl
		<< "\tBackCutComputation         " << m_backCutComputation << std::endl
		<< "\tMinCardinalityCuts         " << m_minCardinalityCuts << std::endl
		<< "\tNestedCutComputation       " << m_nestedCutComputation << std::endl;
	if (m_nestedCutComputation) {
		lout(Level::High)
			<< "\t   SeparationStrategy      " << m_separationStrategy << std::endl
			<< "\t   SaturationStrategy      " << m_saturationStrategy << std::endl;
	}
	lout(Level::High)
		<< "\tPrimalHeuristic            " << m_callPrimalHeuristic << std::endl
		<< "\tPoolSizeInitFactor         " << m_poolSizeInitFactor << std::endl
		<< "\tObjective integer          " << this->objInteger() << std::endl
		<< std::endl;
#endif
	setMaxNumberAddedCuttingPlanes(m_maxNrAddedCuttingPlanes);
}

template<typename T>
void
MinSteinerTreeDirectedCut<T>::Master::initializeOptimization()
{
#ifdef OGDF_STP_EXACT_LOGGING
	lout(Level::High)
		<< "Master::initializeOptimization(): Instance properties:" << std::endl
		<< "\t(nNodes,nEdges)     : (" << m_pGraph->numberOfNodes()
									   << ", " << m_nEdgesU << ")" << std::endl
		<< "\tNumber of terminals : " << m_nTerminals << std::endl
		<< "\tRoot node           : " << m_root << std::endl
		<< std::endl;
#endif
	int i;

	// buffer for variables; one for each directed edge
	ArrayBuffer<abacus::Variable*> variables(m_pGraph->numberOfEdges());

	// add (edge) variables
	EdgeVariable *eVar;
	m_edgeToVar.init(*m_pGraph, nullptr);

	abacus::VarType::TYPE vartype = abacus::VarType::Binary;
	if (m_relaxed) {
		vartype = abacus::VarType::Continuous;
	}

	for (i = 0; i < m_pGraph->numberOfEdges(); i++) {
		edge e = m_edges[i];
		if (e->target() != m_root // not going into root
		 && !e->isSelfLoop()) {
			eVar = new EdgeVariable(this, i, e, m_capacities[e], 0.0, 1.0, vartype);
#ifdef OGDF_STP_EXACT_LOGGING
			lout(Level::Minor) << "\tadding variable x_" << i << ", edge " << e << std::endl;
#endif
		}
		else {
			// ub = lb = 0 is not nice, but makes life easier -> ids
			OGDF_ASSERT(m_capacities[e] >= 0);
			eVar = new EdgeVariable(this, i, e, m_capacities[e], 0.0, 0.0, vartype);
#ifdef OGDF_STP_EXACT_LOGGING
			lout(Level::Minor) << "\tmuting variable x_" << i << ", edge " << e << std::endl;
#endif
		}
		variables.push(eVar);
		m_edgeToVar[e] = eVar;
	}

	// add constraints
	int nCons = 0;
	if (m_addGSEC2Constraints)
		nCons += m_nEdgesU;
	if (m_addDegreeConstraints)
		nCons += m_pGraph->numberOfNodes();
	if (m_addIndegreeEdgeConstraints)
		nCons += m_pGraph->numberOfEdges();
	if (m_addFlowBalanceConstraints)
		nCons += m_pGraph->numberOfNodes()-1;

	ArrayBuffer<abacus::Constraint*> basicConstraints(nCons);

	i = 0;
	if (m_addGSEC2Constraints) {
		EdgeArray<bool> marked(*m_pGraph, false);
		for (edge e : m_pGraph->edges) {
			if (!marked[e]
			 && !e->isSelfLoop()) { // we have to ignore self-loops here
				EdgeConstraint *newCon =
						new EdgeConstraint(this, e, m_twin[e], 1, abacus::CSense::Less, 1.0);
				basicConstraints.push(newCon);
				marked[e] = true;
				marked[m_twin[e]] = true;

#ifdef OGDF_STP_EXACT_LOGGING
				lout(Level::Minor) << "\tadding constraint " << i++ << " GSEC2: edge " << e << std::endl;
#endif
			}
		}
	}

	// "degree" constraints:
	// (1) forall terminals t != m_root: x(delta-(t)) == 1
	// (2) forall non-temrinals n      : x(delta-(n)) <= 1
	// (3) for the root                : x(delta+(m_root)) >= 1
	if (m_addDegreeConstraints)
	{
		for (node n : m_pGraph->nodes) {
			DegreeConstraint *newCon;
			if (n == m_root) {
				// (3)
				newCon = new DegreeConstraint(this, n, 0.0, 1.0, abacus::CSense::Greater, 1.0);
			}
			else {
				if (m_isTerminal[n]) {
					// (1)
					newCon = new DegreeConstraint(this, n, 1.0, 0.0, abacus::CSense::Equal, 1.0);
				}
				else {
					// (2)
					newCon = new DegreeConstraint(this, n, 1.0, 0.0, abacus::CSense::Less, 1.0);
				}
			}
			basicConstraints.push(newCon);

#ifdef OGDF_STP_EXACT_LOGGING
			lout(Level::Minor) << "\tadding constraint " << i++ << " Degree: node " << n << std::endl;
#endif
		}
	}

	if (m_addIndegreeEdgeConstraints) {
		for (edge e : m_pGraph->edges) {
			if (e->source() != m_root) {
				DegreeEdgeConstraint *newCon =
						new DegreeEdgeConstraint(this, e, 1.0, -1.0, abacus::CSense::Greater, 0.0);
				basicConstraints.push(newCon);

#ifdef OGDF_STP_EXACT_LOGGING
				lout(Level::Minor) << "\tadding constraint " << i++ << " Indegree: edge " << e << std::endl;
#endif
			}
		}
	}

	if (m_addFlowBalanceConstraints) {
		for (node n : m_pGraph->nodes) {
			if (!m_isTerminal[n]) {
				DegreeConstraint *newCon =
						new DegreeConstraint(this, n, -1.0, 1.0, abacus::CSense::Greater, 0.0);
				basicConstraints.push(newCon);

#ifdef OGDF_STP_EXACT_LOGGING
				lout(Level::Minor) << "\tadding constraint " << i++ << " Flow-Balance: node " << n << std::endl;
#endif
			}
		}
	}

	m_poolSizeInit = m_poolSizeInitFactor * m_pGraph->numberOfEdges();
	m_poolSizeMax = m_poolSizeInit;
	// initialize the non-duplicate cut pool
	m_pCutPool = new abacus::NonDuplPool<abacus::Constraint, abacus::Variable>(this, m_poolSizeInit, true);

	this->initializePools(
			basicConstraints,
			variables,
			0,
			nCons,
			true);

#ifdef OGDF_STP_EXACT_LOGGING
	lout(Level::Minor) << "Master::initializeOptimization() done." << std::endl;
#endif
}


template<typename T>
void
MinSteinerTreeDirectedCut<T>::Master::terminateOptimization()
{
	int nOnesInSol = 0;
	for (int i = 0; i < m_pGraph->numberOfEdges(); i++) {
		if (m_bestSolution[i] > 0.5) {
			m_isSolutionEdge[m_edges[i]] = true;
			nOnesInSol++;
		}
	}

#ifdef OGDF_STP_EXACT_LOGGING
	lout(Level::High) << std::endl;
	if (is_lout(Level::Medium)) {
		lout(Level::Medium) << "\toptimum solution edges:" << std::endl;
		for (edge e : m_pGraph->edges) {
			if (m_isSolutionEdge[e])
				lout(Level::Medium) << "\t" << e << std::endl;
		}
	}
	lout(Level::Medium) << std::endl;

	lout(Level::High)
		<< "Finished optimization. Statistics:" << std::endl
		<< "Solution               " << std::endl
		<< "   value               " << this->primalBound() << std::endl
		<< "   rounded sol. value  " << ((int)this->primalBound()) << std::endl
		<< "   nr edges            " << nOnesInSol << std::endl
		<< "Status                 " << this->STATUS_[this->status()] << std::endl
		<< "Primal/dual bound      " << this->primalBound() << "/" << this->dualBound() << std::endl
		<< "Relaxed solution value " << this->m_relaxedSolValue << std::endl
		<< "Nr subproblems         " << this->nSub() << std::endl
		<< "Nr solved LPs          " << this->nLp() << std::endl
		<< "nr solved LPs in root  " << m_nIterRoot << std::endl
		<< std::endl

		<< "LP Solver              " << this->OSISOLVER_[this->defaultLpSolver()] << std::endl
		<< "Enumeration strategy   " << this->ENUMSTRAT_[this->enumerationStrategy()] << std::endl
		<< "Branching strategy     " << this->BRANCHINGSTRAT_[this->branchingStrategy()] << std::endl
		<< "Objective integer      " << (this->objInteger()? "true":"false") << std::endl
		<< std::endl

		<< "Total time (centi sec) " << this->totalTime()->centiSeconds() << std::endl
		<< "Total time             " << *this->totalTime() << std::endl
		<< "Total cow-time         " << *this->totalCowTime() << std::endl
		<< "LP time                " << *this->lpTime() << std::endl
		<< "LP solver time         " << *this->lpSolverTime() << std::endl
		<< "Separation time        " << this->m_separationTimer << std::endl
		<< "Minimum Cut time       " << this->m_timerMinSTCut << std::endl
		<< "Primal heuristic time  " << this->m_primalHeuristicTimer << std::endl
		<< std::endl

		<< "Initial cutpool size   " << this->m_poolSizeInit << std::endl
		<< "Maximum cutpool size   " << this->m_poolSizeMax << std::endl
		<< "Nr separated cuts      " << m_nrCutsTotal << std::endl;

	int nDuplicates, nCollisions;
	m_pCutPool->statistics(nDuplicates, nCollisions);
	lout(Level::High)
		<< "Cutpool duplications   " << nDuplicates << std::endl
		<< "Cutpool collisions     " << nCollisions << std::endl
		<< std::endl;
#endif
}


template<typename T>
void
MinSteinerTreeDirectedCut<T>::Master::updateBestSolution(double *values)
{
	double eps = this->eps();
	double oneMinusEps = 1.0 - eps;
	for (int i = 0; i < m_pGraph->numberOfEdges(); i++) {
		if (values[i] > oneMinusEps)
			m_bestSolution[i] = 1.0;
		else
			if (values[i] < eps)
				m_bestSolution[i] = 0.0;
			else
				m_bestSolution[i] = values[i];
	}
}

template<typename T>
void
MinSteinerTreeDirectedCut<T>::Master::updateBestSolutionByEdges(const List<edge> &sol)
{
	for (int i = 0; i < m_pGraph->numberOfEdges(); i++) {
		m_bestSolution[i] = 0.0;
	}
	for (ListConstIterator<edge> it = sol.begin(); it.valid(); it++) {
		m_bestSolution[m_edgeIDs[*it]] = 1.0;
	}
}


template<typename T>
MinSteinerTreeDirectedCut<T>::Sub::Sub(abacus::Master *master)
  : abacus::Sub(master,0,0,0)
  , m_alreadySeparated(-1)
{
	m_pMaster = (Master*) master;
	m_computeNestedCuts  = m_pMaster->computeNestedCuts();
	m_computeBackCuts 	 = m_pMaster->computeBackCuts();
	m_shuffleTerminals 	 = m_pMaster->shuffleTerminals();
	m_minCardinalityCuts = m_pMaster->minCardinalityCuts();
	m_maxNrCuttingPlanes = m_pMaster->maxNrAddedCuttingPlanes();
	m_callPrimalHeuristic= m_pMaster->callPrimalHeuristicStrategy();
	m_separationStrategy = m_pMaster->separationStrategy();
	m_saturationStrategy = m_pMaster->saturationStrategy();
}


template<typename T>
MinSteinerTreeDirectedCut<T>::Sub::Sub(abacus::Master *master, abacus::Sub *father, abacus::BranchRule *branchRule)
  : abacus::Sub(master, father, branchRule)
  , m_alreadySeparated(-1)
{
	m_pMaster = (Master*) master;
#ifdef OGDF_STP_EXACT_LOGGING
	m_pMaster->lout(Logger::Level::High)
		<< std::setw(7)  << this->id()
		<< std::setw(7)  << this->nIter_
		<< " new subproblem, parent=" << father->id() << std::endl;
#endif
	m_computeNestedCuts  = m_pMaster->computeNestedCuts();
	m_computeBackCuts 	 = m_pMaster->computeBackCuts();
	m_shuffleTerminals 	 = m_pMaster->shuffleTerminals();
	m_minCardinalityCuts = m_pMaster->minCardinalityCuts();
	m_maxNrCuttingPlanes = m_pMaster->maxNrAddedCuttingPlanes();
	m_callPrimalHeuristic= m_pMaster->callPrimalHeuristicStrategy();
	m_separationStrategy = m_pMaster->separationStrategy();
	m_saturationStrategy = m_pMaster->saturationStrategy();
}

#ifdef OGDF_STP_EXACT_LOGGING
template<typename T>
void
MinSteinerTreeDirectedCut<T>::Sub::printMainInfosInFeasible(bool header) const
{
	if (header) {
		// print header
		m_pMaster->lout(Logger::Level::High)
			<< std::endl
			<< std::setw(7)  << "id"
			<< std::setw(7)  << "iter"
			<< std::setw(10) << "lp value"
			<< std::setw(10) << "gl. LB"
			<< std::setw(10) << "gl. UB"
			<< std::setw(10) << "nSub"
			<< std::setw(10) << "nOpenSub"
			<< std::endl;
	}
	else {
		m_pMaster->lout(Logger::Level::High)
			<< std::setw(7)  << this->id()
			<< std::setw(7)  << this->nIter_
			<< std::setw(10) << lp_->value();
		if (this->id() == 1)
			m_pMaster->lout(Logger::Level::High) << std::setw(10) << "--";
		else
			m_pMaster->lout(Logger::Level::High) << std::setw(10) << master_->lowerBound();
		if (master_->feasibleFound())
			m_pMaster->lout(Logger::Level::High) << std::setw(10) << master_->upperBound();
		else
			m_pMaster->lout(Logger::Level::High) << std::setw(10) << "--";
		m_pMaster->lout(Logger::Level::High)
			<< std::setw(10) << master_->nSub()
			<< std::setw(10) << master_->openSub()->number()
			<< std::endl;
		m_pMaster->lout(Logger::Level::Minor)
			<< "\tcurrent LP:" << std::endl;
		m_pMaster->lout(Logger::Level::Minor) << *lp_;
		m_pMaster->lout(Logger::Level::Minor) << std::endl;
	}
}
#endif

template<typename T>
bool
MinSteinerTreeDirectedCut<T>::Sub::feasible()
{
	double eps = master_->eps();
	double oneMinusEps = 1.0 - eps;

#ifdef OGDF_STP_EXACT_LOGGING
	if (this->nIter_ == 1) {
		this->printMainInfosInFeasible(true);
	}
	else
		this->printMainInfosInFeasible(false);
#endif

	// separate directed cuts
	m_alreadySeparated = mySeparate();

	if (m_alreadySeparated > 0) {
		if (m_callPrimalHeuristic == 2 && !m_pMaster->relaxed()) {
			myImprove();
		}
		return false;
	}

	if (this->id() == 1) {
		// set some statistics if we are in the root node
		m_pMaster->setRelaxedSolValue(lp_->value());
		m_pMaster->setNIterRoot(nIter_);
	}

	if (!m_pMaster->relaxed()) {
		// check integrality
		for (int i = 0; i < m_pMaster->nEdges(); i++) {
			if (xVal_[i] > eps && xVal_[i] < oneMinusEps) {
				// found non-integral solution but no violated directed cuts
				// i.e., we have to branch.
				// But first, we call the primal heuristic
				if (m_callPrimalHeuristic > 0) {
					myImprove();
				}

#ifdef OGDF_STP_EXACT_LOGGING
				m_pMaster->lout(Logger::Level::Default)
					<< "\tsolution is fractional -> Branching." << std::endl;
#endif
				return false;
			}
		}
	}

	if (m_pMaster->betterPrimal(lp_->value())) {
#ifdef OGDF_STP_EXACT_LOGGING
		m_pMaster->lout(Logger::Level::High)
			<< std::setw(7)  << this->id()
			<< std::setw(7)  << this->nIter_
			<< " found better integer solution with value " << lp_->value();
		if (m_pMaster->is_lout(Logger::Level::Medium)) {
			m_pMaster->lout(Logger::Level::Medium) << ", variables > 0:" << std::endl;
			printCurrentSolution();
		}
		else
			m_pMaster->lout(Logger::Level::High) << std::endl;
#endif
		m_pMaster->primalBound(lp_->value());
		m_pMaster->updateBestSolution(xVal_);
	}

	return true;
}

#ifdef OGDF_STP_EXACT_LOGGING
template<typename T>
void
MinSteinerTreeDirectedCut<T>::Sub::printCurrentSolution(bool onlyNonZeros)
{
	int nOnesInSol = 0;
	double eps = master_->eps();
	double oneMinusEps = 1.0 - eps;
	for (int i = 0; i < nVar(); i++) {
		if (xVal_[i] > -eps && xVal_[i] < eps) {
			if (!onlyNonZeros) {
				m_pMaster->lout(Logger::Level::Minor) << "\tx" << i << "=0" << std::flush;
				m_pMaster->lout(Logger::Level::Minor) << " [edge " << ((EdgeVariable*)variable(i))->theEdge() << "]" << std::endl;
			}
		}
		else if (xVal_[i] > oneMinusEps && xVal_[i] < 1+eps) {
			m_pMaster->lout(Logger::Level::Minor) << "\tx" << i << "=1" << std::flush;
			m_pMaster->lout(Logger::Level::Minor) << " [edge " << ((EdgeVariable*)variable(i))->theEdge() << "]" << std::endl;
			nOnesInSol++;
		}
		else {
			m_pMaster->lout(Logger::Level::Minor) << "\tx" << i << "=" << xVal_[i] << std::flush;
			m_pMaster->lout(Logger::Level::Minor) << " [edge " << ((EdgeVariable*)variable(i))->theEdge() << "]" << std::endl;
		}
	}
	m_pMaster->lout(Logger::Level::Medium) << "\tnEdges=" << nOnesInSol << std::endl << std::flush;
}
#endif

template<typename T>
int
MinSteinerTreeDirectedCut<T>::Sub::mySeparate()
{
#ifdef OGDF_STP_EXACT_LOGGING
	m_pMaster->lout(Logger::Level::Medium)
		<< "Sub::mySeparate(): id="
		<< this->id() << ", iter=" << this->nIter_ << std::endl;
#endif
	m_pMaster->separationTimer()->start();
	double eps = master_->eps();
	double cardEps = eps / 100;
	double oneMinusEps = 1 - eps;
	node r = m_pMaster->rootNode();
	const Graph &g = m_pMaster->graph();
	int nEdgesU = m_pMaster->nEdgesU();

	int nTerminals = m_pMaster->nTerminals();
	const node *masterTerminals = m_pMaster->terminals();
	Array<node> terminal(nTerminals);

	for (int i = 0; i < nTerminals; i++) {
		terminal[i] = masterTerminals[i];
	}

	if (m_shuffleTerminals) {
		// shuffle the ordering of the terminals
		int j;
		node h = nullptr;
		for (int i = 0; i < nTerminals-1; i++) {
			j = randomNumber(i, nTerminals-1);
			h = terminal[i];
			terminal[i] = terminal[j];
			terminal[j] = h;
		}
	}

#ifdef OGDF_STP_EXACT_LOGGING
	if (m_pMaster->is_lout(Logger::Level::Medium)) {
		m_pMaster->lout(Logger::Level::Medium)
			<< "Sub::mySeparate(): considered terminal ordering: ";
		for (int i = 0; i < nTerminals; i++)
			m_pMaster->lout(Logger::Level::Medium) << terminal[i] << " ";
		m_pMaster->lout(Logger::Level::Medium) << std::endl;
	}
#endif

	EdgeArray<double> capacities;
	capacities.init(g, 0);
	for (edge e : g.edges) {
		// some LP solvers might return a negative epsilon instead of 0 due to numerical reasons
		capacities[e] = max(xVal_[m_pMaster->edgeID(e)], 0.);
		if (m_minCardinalityCuts)
			capacities[e] += cardEps;
	}

#ifdef OGDF_STP_EXACT_LOGGING
	m_pMaster->lout(Logger::Level::Minor)
			<< "Sub::mySeparate(): current capacities (>0) for mincut computation:" << std::endl;
	for (edge e : g.edges) {
		if (capacities[e] >= 2*cardEps) {
			m_pMaster->lout(Logger::Level::Minor) << "\tcapacity[" << e << "]=" << capacities[e] << std::endl;
		}
	}
#endif
	// Minimum s-t-cut object
	MinSTCutMaxFlow<double> minSTCut;
	// current terminal
	node t;
	// index of current terminal
	int ti = 0;
	// value of current cut
	double cutValue = 2.0;
	// value of current back cut
	double cutValueBack = 0.0;
	// for backcut computation
	int nOtherNodes = 0;
	// upper bound for mincut computation
	double uBound = 1 + nEdgesU*cardEps;
	// nr of violated cuts found
	int cutsFound = 0;

	// buffer for new constraints
	ArrayBuffer<abacus::Constraint*> newConstraints(m_maxNrCuttingPlanes);

	// size of cut and backcut
	int cardinalityCut, cardinalityBackcut;
	cardinalityCut = cardinalityBackcut = 0;

	// Only relevant if nested cuts are computed:
	// this list contains the modified edges i.e., the saturated edges.
	// The capacity of these edges can is reset in SeparationStrategy 2
	List<edge> modified;

	// main while loop for the computation of the cutting planes
	while (cutsFound < m_maxNrCuttingPlanes && ti < nTerminals) {
		t = terminal[ti];
		if (t != r) {
#ifdef OGDF_STP_EXACT_LOGGING
			m_pMaster->lout(Logger::Level::Medium)
				<< "Sub::mySeparate(): computing minimum cut between root " << r << " and " << t << std::flush;
#endif
			// /compute the minimum r-t-cut
			/*
			 * the cut itself is stored in the integer array with values
			 * in {0,1,2}. If a node has the value 1, it is contained in the
			 * subset of the root, 2 indicates the set of the separated node, and
			 * 0 depicts the set in between
			*/
			m_pMaster->timerMinSTCut()->start();

			EdgeArray<double> flow;
			MaxFlowModule<double> *mf = m_pMaster->getMaxFlowModule();
			OGDF_ASSERT(mf != nullptr);
			mf->init(g);
			mf->useEpsilonTest(cardEps / 100);
			cutValue = mf->computeFlow(capacities, r, t, flow);
#ifdef OGDF_STP_EXACT_LOGGING
			m_pMaster->lout(Logger::Level::Medium) << "  Calculated flow:" << std::endl;
			for (edge flowEdge : g.edges) {
				m_pMaster->lout(Logger::Level::Medium)
				  << "    " << flowEdge << " : "
				  << flow[flowEdge] << " / " << capacities[flowEdge] << std::endl;
			}
#endif

			// used epsilon should be smaller than the eps used for cardinality cuts heuristic
			minSTCut.setEpsilonTest(new EpsilonTest(cardEps / 100));
			minSTCut.call(g, capacities, flow, r, t);

			m_pMaster->timerMinSTCut()->stop();
			cutValueBack = 0;

#ifdef OGDF_STP_EXACT_LOGGING
			m_pMaster->lout(Logger::Level::Medium) << ", cutvalue=" << cutValue << std::flush;
#endif

			// min cardinality
			if (m_minCardinalityCuts && cutValue < uBound) {
				for (edge e : g.edges) {
					if (minSTCut.isFrontCutEdge(e)) {
						cutValue -= cardEps;
					}
					if (m_computeBackCuts && minSTCut.isBackCutEdge(e)) {
						cutValueBack += capacities[e] - cardEps;
					}
				}
			} else if (m_computeBackCuts) {
				for (edge e : g.edges) {
					if (minSTCut.isBackCutEdge(e)) {
						cutValueBack += capacities[e];
					}
				}
			}

			if (m_saturationStrategy == 2) {
				cardinalityCut = cardinalityBackcut = 0;
				for (edge e : g.edges) {
					if (minSTCut.isFrontCutEdge(e))
						cardinalityCut++;
					if (m_computeBackCuts && minSTCut.isBackCutEdge(e))
						cardinalityBackcut++;
				}
			}

#ifdef OGDF_STP_EXACT_LOGGING
			m_pMaster->lout(Logger::Level::Medium)
				<< ", actual cutvalue=" << cutValue;
			if (m_computeBackCuts)
				m_pMaster->lout(Logger::Level::Medium)
					<< ", actual cutValueBack=" << cutValueBack;
			m_pMaster->lout(Logger::Level::Medium) << std::endl;
#endif
			nOtherNodes = 0;

			if (cutValue < oneMinusEps) {
				// found violated cut
				cutsFound++;
				// generate new constraint
				DirectedCutConstraint *newCut = new DirectedCutConstraint(master_, g, &minSTCut, MinSTCutMaxFlow<double>::cutType::FRONT_CUT);
				// push cut to the set of new constraints
				newConstraints.push(newCut);

#ifdef OGDF_STP_EXACT_LOGGING
				m_pMaster->lout(Logger::Level::Medium)
					<< "Sub::mySeparate(): found violated cut:" << std::endl;
				printConstraint(newCut, Logger::Level::Medium);
#endif
				if (m_computeBackCuts
				&& !minSTCut.frontCutIsComplementOfBackCut()
				&& cutsFound < m_maxNrCuttingPlanes
				&& cutValueBack <= oneMinusEps) {
					cutsFound++;
					// generate new constraint
					DirectedCutConstraint *newBackCut = new DirectedCutConstraint(master_, g, &minSTCut, MinSTCutMaxFlow<double>::cutType::BACK_CUT);
					// push cut to the set of new constraints
					newConstraints.push(newBackCut);

#ifdef OGDF_STP_EXACT_LOGGING
					m_pMaster->lout(Logger::Level::Medium)
						<< "Sub::mySeparate(): found violated cut (backcut):" << std::endl;
					printConstraint(newBackCut, Logger::Level::Medium);
#endif
				}

				// saturate cut edges in case of nested cut computation
				if (m_computeNestedCuts)
				{
					for (edge e : g.edges) {
						if (minSTCut.isFrontCutEdge(e)) {
							if (m_saturationStrategy == 2)
								capacities[e] = 1.0/(double)cardinalityCut + eps;
							else
								capacities[e] = 1.0 + eps;
							// for resetting the saturation after each iteration
							if (m_separationStrategy == 2)
								modified.pushBack(e);
						}
						else {
							if (m_computeBackCuts
							&& nOtherNodes > 0
							&& cutValueBack <= oneMinusEps
							&& minSTCut.isBackCutEdge(e)) {
								if (m_saturationStrategy == 2)
									capacities[e] = 1.0/(double)cardinalityBackcut + eps;
								else
									capacities[e] = 1.0 + eps;
								// for resetting the saturation after each iteration
								if (m_separationStrategy == 2)
									modified.pushBack(e);
							}
						}
					}
				}
			}
		}

		if (!m_computeNestedCuts) {
			ti++;
		} else {
			if (cutValue > oneMinusEps || r == t) {
				ti++;
				if (m_separationStrategy == 2) {
					while (!modified.empty()) {
						edge e = modified.popFrontRet();
						capacities[e] = xVal_[m_pMaster->edgeID(e)];
						if (m_minCardinalityCuts)
							capacities[e] += cardEps;
					}
				}
			}
		}
	}

	m_alreadySeparated = cutsFound;

	if (cutsFound > 0) {
		// add separated directed cuts
		int nAdded = addCons(newConstraints, m_pMaster->cutPool());
		// update statistics
		m_pMaster->incNrCutsTotal(nAdded);
		m_pMaster->checkSetMaxPoolSize();
		if (nAdded != cutsFound) {
			// ToDo: is this a problem?!
		}
	}

#ifdef OGDF_STP_EXACT_LOGGING
	m_pMaster->lout(Logger::Level::Medium)
		<< "Sub::mySeparate(): id="
		<< this->id() << ", iter=" << this->nIter_
		<< " separated " << cutsFound << " directed cuts" << std::endl;
#endif
	m_pMaster->separationTimer()->stop();

	return cutsFound;
}

template<typename T>
void
MinSteinerTreeDirectedCut<T>::Sub::myImprove()
{
	m_pMaster->primalHeuristicTimer()->start();

#ifdef OGDF_STP_EXACT_LOGGING
	m_pMaster->lout(Logger::Level::Minor)
		<< "Sub::myImprove(): id="
		<< this->id() << ", iter=" << this->nIter_ << std::endl;
#endif
	double eps = master_->eps();
	const Graph &g = m_pMaster->graph();
	EdgeWeightedGraph<double> &ewg = m_pMaster->weightedGraphPH();

#ifdef OGDF_STP_EXACT_LOGGING
	if (m_pMaster->ilout(Logger::Level::Minor)) {
		m_pMaster->lout(Logger::Level::Minor)
			<< "Sub::myImprove(): current solution:" << std::endl;
		for(edge e : g.edges) {
			m_pMaster->lout(Logger::Level::Minor)
				<< "\tx" << m_pMaster->edgeID(e) << "=" << xVal_[m_pMaster->edgeID(e)]
				<< ", edge " << e << std::endl;
		}
	}
#endif

	// set edge weights to eps + (1-x_e)*c_e, forall edges e
	// thereby, use minimum of e and twin(e), respectively
	double currWeight, twinWeight;
	for (edge e : g.edges) {
		edge e2 = m_pMaster->twin(e);
		currWeight = 1.0-xVal_[m_pMaster->edgeID(e)];
		twinWeight = 1.0-xVal_[m_pMaster->edgeID(e2)];
		if (twinWeight < currWeight)
			currWeight = twinWeight;
		if (currWeight < 0)
			currWeight = 0;
		ewg.setWeight(m_pMaster->edgeGToWgPH(e), eps + currWeight * variable(m_pMaster->edgeID(e))->obj());
	}

#ifdef OGDF_STP_EXACT_LOGGING
	if (m_pMaster->ilout(Logger::Level::Minor)) {
		m_pMaster->lout(Logger::Level::Minor)
			<< "Sub::myImprove(): edge weights:" << std::endl;
		for (edge e : g.edges) {
			m_pMaster->lout(Logger::Level::Minor)
				<< "\tweight[" << e << "]=" << ewg.weight(m_pMaster->edgeGToWgPH(e)) << std::endl;
		}
	}
#endif

	// get primal heuristic algorithm
	auto &primalHeuristic = m_pMaster->getPrimalHeuristic();

	// the computed heuristic solution
	EdgeWeightedGraphCopy<double> *heuristicSolutionWg = nullptr;

#ifdef OGDF_STP_EXACT_LOGGING
	// heuristic solution value for the modified edge weights
	double tmpHeuristicSolutionValue =
#endif
	// call primal heuristic
	primalHeuristic->call(
					m_pMaster->weightedGraphPH(),
					m_pMaster->terminalListPH(),
					m_pMaster->isTerminalPH(),
					heuristicSolutionWg);

	// verify that solution is a Steiner tree
	bool isSteinerTree = primalHeuristic->isSteinerTree(
								m_pMaster->weightedGraphPH(),
								m_pMaster->terminalListPH(),
								m_pMaster->isTerminalPH(),
								*heuristicSolutionWg);

#ifdef OGDF_STP_EXACT_LOGGING
	m_pMaster->lout(Logger::Level::Default)
		<< "Sub::myImprove(): primal heuristic algorithm returned solution with "
		<< "value " << tmpHeuristicSolutionValue << ", isSteinerTree=" << isSteinerTree << std::endl;
#endif

	if (isSteinerTree) {
		// actual heuristic solution value, i.e., by using real edge weights
		double heuristicSolutionValue = 0.0;
		// list of edges in the heuristic solution
		List<edge> solutionEdges;

		for (edge e : heuristicSolutionWg->edges) {
			edge e2 = m_pMaster->edgeWgToGPH(heuristicSolutionWg->original(e));
			solutionEdges.pushBack(e2);
			heuristicSolutionValue +=  m_pMaster->capacities(e2);
#ifdef OGDF_STP_EXACT_LOGGING
			m_pMaster->lout(Logger::Level::Minor)
				<< "\t" << e << " -> " << e2 << " c=" << m_pMaster->capacities(e2) << std::endl;
#endif
		}

#ifdef OGDF_STP_EXACT_LOGGING
		m_pMaster->lout(Logger::Level::Default)
			<< "Sub::myImprove(): found integer solution with value "
			<< heuristicSolutionValue << std::endl;
#endif
		if (m_pMaster->betterPrimal(heuristicSolutionValue)) {
#ifdef OGDF_STP_EXACT_LOGGING
			m_pMaster->lout(Logger::Level::High)
				<< std::setw(7)  << this->id()
				<< std::setw(7)  << this->nIter_
				<< " primal heuristic founds better integer solution with value " << heuristicSolutionValue << std::endl;
#endif
			m_pMaster->primalBound(heuristicSolutionValue);
			m_pMaster->updateBestSolutionByEdges(solutionEdges);
		}
	}
#ifdef OGDF_STP_EXACT_LOGGING
	else {
		m_pMaster->lout(Logger::Level::High)
			<< "Sub::myImprove(): id="
			<< this->id() << ", iter=" << this->nIter_
			<< ": computed solution is no Steiner tree!" << std::endl;
	}
#endif

	delete heuristicSolutionWg;
	m_pMaster->primalHeuristicTimer()->stop();
}

#ifdef OGDF_STP_EXACT_LOGGING
template<typename T>
void
MinSteinerTreeDirectedCut<T>::Sub::printConstraint(abacus::Constraint *constraint, Logger::Level level) const
{
	double eps = master_->eps();
	double val = 0;
	abacus::Variable *var;
	bool first = true;
	for (int i = 0; i < nVar(); i++) {
		var = variable(i);
		val = constraint->coeff(var);
		if (val > eps || val < -eps) {
			if (val > 0) {
				if (val > 1-eps && val < 1+eps) {
					if (!first)
						m_pMaster->lout(level) << " + ";
				}
				else {
					if (!first)
						m_pMaster->lout(level) << " + " << val;
				}
			}
			else {
				if (val < -1+eps && val > -1-eps) {
					if (!first)
						m_pMaster->lout(level) << " - ";
					else
						m_pMaster->lout(level) << " -";
				}
				else {
					if (!first)
						m_pMaster->lout(level) << " - " << (-1)*val;
					else
						m_pMaster->lout(level) << val;
				}
			}
			m_pMaster->lout(level)  << "x" << i;
			first = false;
		}
	}
	m_pMaster->lout(level)
		<< " " << *(constraint->sense()) << " "
		<< constraint->rhs() << std::endl;
}
#endif

template<typename T>
bool
MinSteinerTreeDirectedCut<T>::DirectedCutConstraint::equal(const ConVar *cv) const
{
	if (m_hashKey != cv->hashKey()) {
		return false;
	}
	DirectedCutConstraint *dirCut = (DirectedCutConstraint*)cv;
	if (m_nMarkedNodes != dirCut->nMarkedNodes())
		return false;
	for (node n : m_pGraph->nodes) {
		if (m_marked[n] != dirCut->marked(n)) {
			return false;
		}
	}
	return true;
}


template<typename T>
double
MinSteinerTreeDirectedCut<T>::DirectedCutConstraint::coeff(const abacus::Variable *v) const
{
	EdgeVariable *edgeVar = (EdgeVariable*)v;
	if (this->cutedge(edgeVar->theEdge())) {
		return 1.0;
	}
	return 0.0;
}


template<typename T>
T MinSteinerTreeDirectedCut<T>::computeSteinerTree(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal, EdgeWeightedGraphCopy<T> *&finalSteinerTree)
{
	Master stpMaster(G, terminals, isTerminal, m_eps);
	if (m_configFile) {
		stpMaster.setConfigFile(m_configFile);
	}
#ifdef OGDF_STP_EXACT_LOGGING
	stpMaster.setOutputLevel(m_outputLevel);
#endif
	stpMaster.useDegreeConstraints(m_addDegreeConstraints);
	stpMaster.useIndegreeEdgeConstraints(m_addIndegreeEdgeConstraints);
	stpMaster.useGSEC2Constraints(m_addGSEC2Constraints);
	stpMaster.useFlowBalanceConstraints(m_addFlowBalanceConstraints);
	stpMaster.setMaxNumberAddedCuttingPlanes(m_maxNrAddedCuttingPlanes);
	stpMaster.useTerminalShuffle(m_shuffleTerminals);
	stpMaster.useBackCuts(m_backCutComputation);
	stpMaster.useNestedCuts(m_nestedCutComputation);
	stpMaster.setSeparationStrategy(m_separationStrategy);
	stpMaster.setSaturationStrategy(m_saturationStrategy);
	stpMaster.useMinCardinalityCuts(m_minCardinalityCuts);
	stpMaster.setMaxFlowModule(m_maxFlowModuleOption.get());
	if (m_primalHeuristic) {
		stpMaster.setPrimalHeuristic(m_primalHeuristic);
	}
	stpMaster.setPrimalHeuristicCallStrategy(m_callPrimalHeuristic);
	stpMaster.setPoolSizeInitFactor(m_poolSizeInitFactor);
	// XXX: should we set stpMaster.objInteger true/false according to weights automatically?

	// now solve LP
	stpMaster.optimize();

	// collect solution edges to build Steiner tree
	finalSteinerTree = new EdgeWeightedGraphCopy<T>();
	finalSteinerTree->createEmpty(G);
	T weight(0);
	for (edge e = G.firstEdge(); e; e = e->succ()) {
		if (stpMaster.isSolutionEdge(e)) {
			const node vO = e->source();
			node vC = finalSteinerTree->copy(vO);
			if (!vC) {
				vC = finalSteinerTree->newNode(vO);
			}
			const node wO = e->target();
			node wC = finalSteinerTree->copy(wO);
			if (!wC) {
				wC = finalSteinerTree->newNode(wO);
			}
			T edgeCost = G.weight(e);
			finalSteinerTree->newEdge(e, edgeCost);
			weight += edgeCost;
		}
	}

	return weight;
}

}
