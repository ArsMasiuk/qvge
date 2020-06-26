/** \file
 * \brief Declaration of base class for master of Branch&Cut based algorithms
 * for c-planarity testing via an extension to complete connectivity.
 *
 *
 * \author Karsten Klein, Markus Chimani
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

#include <ogdf/cluster/internal/basics.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/cluster/internal/CPlanarEdgeVar.h>
#include <ogdf/basic/Logger.h>
#include <ogdf/basic/ArrayBuffer.h>
#include <ogdf/lib/abacus/master.h>

namespace ogdf {
namespace cluster_planarity {

class CP_MasterBase : public abacus::Master {
public:

	enum class solutionState {Undefined, CPlanar, NonCPlanar};

	//! Construction and default values.
	explicit CP_MasterBase(const ClusterGraph &C,
			//Check what we really still need here
			int heuristicLevel=1,
			int heuristicRuns=2,
			double heuristicOEdgeBound=0.3,
			int heuristicNPermLists=5,
			int kuratowskiIterations=3,
			int subdivisions=10,
			int kSupportGraphs=3,
			double kuratowskiHigh=0.7,
			double kuratowskiLow=0.3,
			bool perturbation=false,
			double branchingGap=0.4,
			const char *time="00:05:00" /* maximum computation time */);

	//! Destruction
	virtual ~CP_MasterBase();

#if 0
	// Initialization of the first Subproblem
	virtual Sub *firstSub();
#endif

	//! Returns the objective function coefficient of \a C-edges.
	double epsilon() const {return m_epsilon;}

	//! Returns the number of variables.
	int nMaxVars() const {return m_nMaxVars;}

	//! Returns a pointer to the underlying Graph.
	const Graph *getGraph() const {return m_G;}

	//! Returns a pointer to the given Clustergraph.
	const ClusterGraph *getClusterGraph() const {return m_C;}

	//! Updates the "best" Subgraph #m_solutionGraph found so far and fills \p connection with
	// corresponding edges (::nodePair).
	virtual void updateBestSubGraph(List<NodePair> &connection);

	//! Returns the optimal solution induced Clustergraph.
	virtual Graph *solutionInducedGraph() {return static_cast<Graph*>(m_solutionGraph);}

	//! Returns nodePairs of connecting optimal solution edges in list \p edges.
	virtual void getConnectionOptimalSolutionEdges(List<NodePair> &edges) const;

	void setTimeLimit(const char* s) {delete m_maxCpuTime; m_maxCpuTime = new string(s);}

	// Get parameters
	int getKIterations() const {return m_nKuratowskiIterations;}
	int getNSubdivisions() const {return m_nSubdivisions;}
	int getNKuratowskiSupportGraphs() const {return m_nKuratowskiSupportGraphs;}
	int getHeuristicLevel() const {return m_heuristicLevel;}
	int getHeuristicRuns() const {return m_nHeuristicRuns;}
	double getKBoundHigh() const {return m_kuratowskiBoundHigh;}
	double getKBoundLow() const {return m_kuratowskiBoundLow;}
	bool perturbation() const {return m_usePerturbation;}
#if 0
	double branchingOEdgeSelectGap() const {return m_branchingGap;}
#endif
	double getHeuristicFractionalBound() const {return m_heuristicFractionalBound;}
	int numberOfHeuristicPermutationLists() const {return m_nHeuristicPermutationLists;}
	bool getMPHeuristic() const {return m_mpHeuristic;}
	int getNumAddVariables() const {return m_numAddVariables;}
	double getStrongConstraintViolation() const {return m_strongConstraintViolation;}
	double getStrongVariableViolation() const {return m_strongVariableViolation;}

	// Read global constraint counter, i.e. the number of added constraints of specific type.
	int addedKConstraints() const {return m_nKConsAdded;}
	int addedCConstraints() const {return m_nCConsAdded;}


	// Set parameters
	void setKIterations(int n) {m_nKuratowskiIterations = n;}
	void setNSubdivisions(int n) {m_nSubdivisions = n;}
	void setNKuratowskiSupportGraphs(int n) {m_nKuratowskiSupportGraphs = n;}
	void setNHeuristicRuns(int n) {m_nHeuristicRuns = n;}
	void setKBoundHigh(double n) {m_kuratowskiBoundHigh = ((n>0.0 && n<1.0) ? n : 0.8);}
	void setKBoundLow(double n) {m_kuratowskiBoundLow = ((n>0.0 && n<1.0) ? n : 0.2);}
	void heuristicLevel(int level) {m_heuristicLevel = level;}
	void setHeuristicRuns(int n) {m_nHeuristicRuns = n;}
	void setPertubation(bool b) {m_usePerturbation = b;}
	void setHeuristicFractionalBound(double b) {m_heuristicFractionalBound = b;}
	void setHeuristicPermutationLists(int n) {m_nHeuristicPermutationLists = n;}
	//! Switches use of lower bound heuristic
	void setMPHeuristic(bool b) {m_mpHeuristic = b;}
	void setNumAddVariables(int i) {m_numAddVariables=i;}
	void setStrongConstraintViolation(double d) { m_strongConstraintViolation=d;}
	void setStrongVariableViolation(double d) { m_strongVariableViolation=d;}

	//! If set to true, PORTA output is written in a file
	void setPortaFile(bool b) {m_porta = b;}

	// Updating global constraint counter
	void updateAddedCCons(int n) {m_nCConsAdded += n;}
	void updateAddedKCons(int n) {m_nKConsAdded += n;}

	// Returns global primal and dual bounds.
	double getPrimalBound() {return globalPrimalBound;}
	double getDualBound() {return globalDualBound;}

	// Cut pools for connectivity and planarity
	//! Returns cut pool for connectivity
	abacus::StandardPool<abacus::Constraint, abacus::Variable> *getCutConnPool() {return m_cutConnPool;}
	//! Returns cut pool for planarity
	abacus::StandardPool<abacus::Constraint, abacus::Variable> *getCutKuraPool() {return m_cutKuraPool;}

	//! Returns true if default cut pool is used. Otherwise, separate
	//! connectivity and Kuratowski pools are generated and used.
	bool &useDefaultCutPool() { return m_useDefaultCutPool;}

	//! Returns a value that allows to distinguish result values
	//! when connection edges (tiny negative cost) are added.
	double intGap() {return 0.79;}

#ifdef OGDF_DEBUG
	bool m_solByHeuristic; //solution computed by heuristic or ILP
		// Simple output function to print the given graph to the console.
	// Used for debugging only.
	void printGraph(const Graph &G);
#endif

	//! The name of the file that contains the standard, i.e., non-cut,
	//! constraints (may be deleted by ABACUS and shouldn't be stored twice)
	const char* getStdConstraintsFileName()
	{
		return "StdConstraints.txt";
	}

	int getNumInactiveVars() { return m_inactiveVariables.size();}

	solutionState m_solState; //! stores optimization success state

protected:

	List<NodePair> m_connectionOneEdges;  //<! Contains connection nodePairs whose variable is set to 1.0

	//! Pointers to the given Clustergraph and underlying Graph are stored.
	const ClusterGraph *m_C;
	const Graph *m_G;

	// Each time the primal bound is improved, the integer solution induced Graph is built.
	// #m_solutionGraph is a pointer to the currently best solution induced Graph.
	// #m_solutionGraph is deleted in the destructor.
	GraphCopy *m_solutionGraph;

	//! Cut pools for connectivity and Kuratowski constraints
	abacus::StandardPool< abacus::Constraint, abacus::Variable > *m_cutConnPool; //!< Connectivity Cuts
	abacus::StandardPool< abacus::Constraint, abacus::Variable > *m_cutKuraPool; //!< Kuratowski Cuts

	string *m_maxCpuTime; //!< Time threshold for optimization

	// Initializes constraints and variables and an initial dual bound.
	virtual void initializeOptimization() override = 0;

	//! Function that is invoked at the end of the optimization.
	//! Does nothing but output in CP_MasterBase.
	virtual void terminateOptimization() override;

	virtual double heuristicInitialLowerBound();

	//! All variables that have to be present at start of optimization
	//! are created here.
	virtual void createInitialVariables(List<CPlanarEdgeVar*>& initVars) = 0;

	// Computes a dual bound for the optimal solution.
	// Tries to find as many edge-disjoint Kuratowski subdivisions as possible.
	// If k edge-disjoint groups of subdivisions are found, the upper bound can be
	// initialized with number of edges in underlying graph minus k.
	virtual double heuristicInitialUpperBound();

	//! Derives and returns c-planarity property either directly or indirectly
	//! from computation results
	virtual bool isCP() = 0;

	// Node pair is potential candidate for new edge variable
	virtual bool goodVar(node a, node b) { return true;}

	List<NodePair> m_inactiveVariables; //! Keeps track of variables that are currently inactive during optimization
#if 0
	//used in initialization
	void generateVariablesForFeasibility(const List<ChunkConnection*>& ccons, List<CPlanarEdgeVar*>& connectVars);
#endif
	NodeArray< NodeArray<bool> > m_varCreated; //! Keeps track of created variables


	// Parameters
	int m_nKuratowskiSupportGraphs; // Maximal number of times the Kuratowski support graph is computed
	int m_nKuratowskiIterations; // Maximal number of times BoyerMyrvold is invoked
	int m_nSubdivisions; // Maximal number of extracted Kuratowski subdivisions
	int m_nMaxVars; // Max Number of variables
	int m_heuristicLevel; // Indicates if primal heuristic shall be used or not
	int m_nHeuristicRuns; // Counts how often the primal heuristic has been called

	bool m_usePerturbation; // Indicates whether C-variables should be perturbated or not
	double m_branchingGap; // Modifies the branching behaviour
	double m_heuristicFractionalBound;
	int m_nHeuristicPermutationLists; // The number of permutation lists used in the primal heuristic
	bool m_mpHeuristic; //!< Indicates if simple max planar subgraph heuristic should be used to derive lower bound if only root cluster exists

	double m_kuratowskiBoundHigh; // Upper bound for deterministic edge addition in computation of the Supportgraph
	double m_kuratowskiBoundLow; // Lower bound for deterministic edge deletion in computation of the Supportgraph

	int m_numAddVariables; // how many variables should i add maximally per pricing round?
	double m_strongConstraintViolation; // when do i consider a constraint strongly violated -> separate in first stage
	double m_strongVariableViolation;   // when do i consider a variable strongly violated (red.cost) -> separate in first stage

	// Counters for the number of added constraints
	int m_nCConsAdded;
	int m_nKConsAdded;
	int m_solvesLP;
	int m_varsInit;
	int m_varsAdded;
	int m_varsPotential;
	int m_varsMax;
	int m_varsCut;
	int m_varsKura;
	int m_varsPrice;
	int m_varsBranch;
	int m_activeRepairs;
	ArrayBuffer<int> m_repairStat;
	inline void clearActiveRepairs() {
		if(m_activeRepairs) {
			m_repairStat.push(m_activeRepairs);
			m_activeRepairs = 0;
		}
	}

	double globalPrimalBound;
	double globalDualBound;

	inline double getDoubleTime(const Stopwatch* act) {
		int64_t tempo = act->centiSeconds()+100*act->seconds()+6000*act->minutes()+360000*act->hours();
		return  ((double) tempo)/ 100.0;
	}

#if 0
	//number of calls of the fast max planar subgraph heuristic
	const int m_fastHeuristicRuns;
#endif

private:
	// Is invoked by heuristicInitialLowerBound()
	virtual double clusterConnection(cluster c, GraphCopy &GC);

	//! Creates variables for complete connectivity
	virtual void createCompConnVars(List<CPlanarEdgeVar*>& initVars);

	// Computes the (here: graphtheoretical) distances of edges incident to node \p u.
	virtual void nodeDistances(node u, NodeArray<NodeArray<int> > &dist);

	// The basic objective function coefficient for connection edges.
	double m_epsilon;
#if 0
	// If perturbation is used, this variable stores the largest occuring coeff,
	// i.e. the one closest to 0. Otherwise it corresponds to #m_epsilon
	double m_largestConnectionCoeff;
#endif

	//! Defines if the ABACUS default cut pool or the separate Connectivity
	//! and Kuratowski constraint pools are used
	bool m_useDefaultCutPool;

	//!
#if 0
	double m_delta;
	double m_deltaCount;
#endif
	//Switch to minimization of additional edges, no delta necessary
#if 1
	virtual double nextConnectCoeff() {return 1.0;}
#else
	virtual double nextConnectCoeff() { return  -1  + m_deltaCount--*m_delta; }
#endif
	//! Variable creation for nodePair
	virtual CPlanarEdgeVar* createVariable(ListIterator<NodePair>& it) {
		++m_varsAdded;
		CPlanarEdgeVar* v = new CPlanarEdgeVar(this, nextConnectCoeff(), (*it).source, (*it).target);
		v->printMe(Logger::slout());
		m_inactiveVariables.del(it);
		//we don't need to check symmetry
		m_varCreated[(*it).source][(*it).target] = true;
		return v;
	}
	//! Variable creation for pair of nodes which is not stored in m_inactiveVariables.
	virtual CPlanarEdgeVar* createVariable(node a, node b) {
		OGDF_ASSERT(!(m_varCreated[a][b] || m_varCreated[b][a]));
		++m_varsAdded;
		CPlanarEdgeVar* v = new CPlanarEdgeVar(this, nextConnectCoeff(), a, b);
		v->printMe(Logger::slout());
		// we don't need to check symmetry
		m_varCreated[a][b] = true;
		return v;
	}
#if 0
	List<nodePair> m_inactiveVariables;

	//used in initialization
	void generateVariablesForFeasibility(const List<ChunkConnection*>& ccons, List<CPlanarEdgeVar*>& connectVars);

	//! Keeps track of created variables
	NodeArray< NodeArray<bool> > m_varCreated;
#endif

	//! If set to true, PORTA output is written in a file
	bool m_porta;
	//! writes coefficients of all orig and connect variables in constraint con into
	//! emptied list coeffs
	virtual void getCoefficients(abacus::Constraint* con, const List<CPlanarEdgeVar* > & connect,
			List<double> & coeffs);
};

}
}
