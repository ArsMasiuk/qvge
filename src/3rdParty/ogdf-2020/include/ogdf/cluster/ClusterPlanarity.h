/** \file
 * \brief Declaration of a c-planarity testing algorithm,
 * based on a completely connected graph extension.
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

#include <ogdf/basic/Module.h>
#include <ogdf/basic/Timeouter.h>

#include <ogdf/cluster/ClusterPlanarModule.h>
#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/internal/CPlanarityMaster.h>

#include <ogdf/external/abacus.h>

namespace ogdf {

//! C-planarity testing via completely connected graph extension.
/**
 * @ingroup ga-cplanarity
 */
class OGDF_EXPORT ClusterPlanarity : public ClusterPlanarModule
{
public:
	using NodePairs = List<NodePair>;
	using CP_MasterBase = cluster_planarity::CP_MasterBase;

	//! Solution method, fallback to old version (allowing all extension edges,
	//! based on c-planar subgraph computation) or new direct version (allowing
	//! only a reduced set of extension edges for complete connectivity).
	enum class solmeth { Fallback, New };

	//! Construction
	ClusterPlanarity()
	: m_heuristicLevel(1)
	, m_heuristicRuns(1)
	, m_heuristicOEdgeBound(0.4)
	, m_heuristicNPermLists(5)
	, m_kuratowskiIterations(10)
	, m_subdivisions(10)
	, m_kSupportGraphs(10)
	, m_kuratowskiHigh(0.8)
	, m_kuratowskiLow(0.8)
	, m_perturbation(false)
	, m_branchingGap(0.4)
	, m_time("00:20:00")
	, m_pricing(false)
	, m_numAddVariables(15)
	, m_strongConstraintViolation(0.3)
	, m_strongVariableViolation(0.3)
	, m_solmeth(solmeth::New)
	, m_optStatus(abacus::Master::STATUS::Unprocessed)
	, m_totalTime(-1.0)
	, m_heurTime(-1.0)
	, m_lpTime(-1.0)
	, m_lpSolverTime(-1.0)
	, m_sepTime(-1.0)
	, m_totalWTime(-1.0)
	, m_numCCons(-1)
	, m_numKCons(-1)
	, m_numLPs(-1)
	, m_numBCs(-1)
	, m_numSubSelected(-1)
	, m_numVars(-1)
	, m_portaOutput(false)
	, m_defaultCutPool(true)
#ifdef OGDF_DEBUG
	, m_solByHeuristic(false)
#endif
	{ }

	//destruction
	~ClusterPlanarity() { }

	//! Computes a set of edges that augments the subgraph to be
	//! completely connected and returns c-planarity status and edge set.
	bool isClusterPlanar(const ClusterGraph &CG, NodePairs &addedEdges);

	//! Returns c-planarity status of \p CG.
	virtual bool isClusterPlanar(const ClusterGraph &CG) override;

	//setter methods for the  module parameters
	void setHeuristicLevel(int i) {m_heuristicLevel = i;}
	void setHeuristicRuns(int i) {m_heuristicRuns = i;}
	void setHeuristicBound(double d) {m_heuristicOEdgeBound = d;}
	void setNumberOfPermutations(int i) {m_heuristicNPermLists = i;}
	void setNumberOfKuraIterations(int i) {m_kuratowskiIterations = i;}
	void setNumberOfSubDivisions(int i) {m_subdivisions = i;}
	void setNumberOfSupportGraphs(int i) {m_kSupportGraphs = i;}
	void setUpperRounding(double d) {m_kuratowskiHigh = d;}
	void setLowerRounding(double d) {m_kuratowskiLow = d;}
	void setPerturbation(bool b) {m_perturbation = b;}
	void setBranchingGap(double d) {m_branchingGap = d;}
	void setTimeLimit(string s) {m_time = s.c_str();}
	void setPortaOutput(bool b) {m_portaOutput = b;}
	void setPricing(bool b) { m_pricing = b;}
	void setNumAddVariables(int n) { m_numAddVariables = n;}
	void setStrongConstraintViolation(double d) { m_strongConstraintViolation=d;}
	void setStrongVariableViolation(double d) { m_strongVariableViolation=d;}
	//! Use default abacus master cut pool or dedicated connectivity and
	//! kuratowski cut pools
	bool & useDefaultCutPool() { return m_defaultCutPool; }

	//getter methods for results
	abacus::Master::STATUS getOptStatus() { return m_optStatus; }
	double getTotalTime() {return m_totalTime;}
	double getHeurTime() {return m_heurTime;}
	double getLPTime() {return m_lpTime;}
	double getLPSolverTime() {return m_lpSolverTime;}
	double getSeparationTime() {return m_sepTime;}
	double getTotalWTime() {return m_totalWTime;}
	//! Returns number of connectivity constraints added during computation
	int    getNumCCons() {return m_numCCons;}
	//! Returns number of Kuratowski constraints added during computation
	int    getNumKCons() {return m_numKCons;}
	//! Returns number of optimized LPs (only LP-relaxations)
	int    getNumLPs()   {return m_numLPs;}
	//! Returns number of generated LPs
	int    getNumBCs()   {return m_numBCs;}
	//! Returns number of subproblems selected by ABACUS
	int    getNumSubSelected() {return m_numSubSelected;}
	//! Returns number of global variables. Todo: Real number from ABACUS
	int    getNumVars() {return m_numVars;}

	//! Writes feasible solutions as a file in PORTA format
	void writeFeasible(const char *filename, CP_MasterBase &master,
			abacus::Master::STATUS &status);
#ifdef OGDF_DEBUG
	bool getSolByHeuristic(){return m_solByHeuristic;}
#endif

	solmeth& solutionMethod() { return m_solmeth; }

protected:

	// Performs a c-planarity test on CG.
	virtual bool doTest(const ClusterGraph &CG) override;

	//as above, on success also returns the set of edges that were
	//added to construct a completely connected planar graph.
	virtual bool doTest(const ClusterGraph &G, NodePairs &addedEdges);

	// Performs a c-planarity test using only a reduced set
	// of allowed extension edges, returns c-planarity status
#if 0
	virtual bool doFastTest(const ClusterGraph &G);
#endif

	double getDoubleTime(const Stopwatch &act)
	{
		int64_t tempo = act.centiSeconds()+100*act.seconds()+6000*act.minutes()+360000*act.hours();
		return  ((double) tempo)/ 100.0;
	}

	//! Stores clusters in subtree at c in bottom up order in theList
	void getBottomUpClusterList(const cluster c, List< cluster > & theList);

private:
	//Abacus Master class settings in order of appearance
	int m_heuristicLevel, m_heuristicRuns;
	double m_heuristicOEdgeBound;
	int m_heuristicNPermLists, m_kuratowskiIterations;
	int m_subdivisions, m_kSupportGraphs;
	double m_kuratowskiHigh, m_kuratowskiLow;
	bool m_perturbation;
	double m_branchingGap;
	string m_time;
	bool m_pricing;//<! Decides if pricing is used
	int m_numAddVariables;
	double m_strongConstraintViolation;
	double m_strongVariableViolation;

	solmeth m_solmeth; //!< Solution method, see description of solmeth

	const char* getPortaFileName()
	{
		return "porta.poi";
	}
	const char* getIeqFileName()
	{
		return "porta.ieq";
	}
	int maxConLength()
	{
		return 1024;
	}
	void outputCons(std::ofstream &os,
			abacus::StandardPool<abacus::Constraint, abacus::Variable> *connCon,
			abacus::StandardPool<abacus::Variable, abacus::Constraint> *stdVar);

	//results
	abacus::Master::STATUS m_optStatus; //<! Status of optimization, returned from abacus
	double m_totalTime;  //<! Total computation time, returned from abacus
	double m_heurTime;   //<! Time spent in heuristic, returned from abacus
	double m_lpTime;     //<! Cpu time spent in members of the LP-interface, returned from abacus
	double m_lpSolverTime; //<! Cpu time required by the LP solver, returned from abacus
	double m_sepTime;  //<! Cpu time spent in the separation of cutting planes, returned from abacus
	double m_totalWTime; //<! Total wall clock time, returned from abacus
	int    m_numCCons;   //<! Number of added connectivity constraints
	int    m_numKCons;   //<! Number of added kuratowski constraints
	int    m_numLPs;     //<! The number of optimized linear programs (LP-relax.).
	int    m_numBCs;     //<! The number of generated subproblems.
	int    m_numSubSelected; //<! The number of subproblems selected by ABACUS.
	int    m_numVars;    //<! The number of global variables.
	bool   m_portaOutput; //<! If set to true, output for PORTA in ieq format is generated.
	bool   m_defaultCutPool; //<! Passed through to master
#ifdef OGDF_DEBUG
	bool m_solByHeuristic;
#endif

};

}
