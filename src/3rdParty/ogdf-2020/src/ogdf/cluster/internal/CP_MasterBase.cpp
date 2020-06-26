/** \file
 * \brief Implementation of the CP_MasterBase class for the Branch-Cut-Price algorithm
 * for c-planarity testing.
 *
 * This class is managing the optimization.
 * Variables and initial constraints are generated and pools are initialized.
 * Since variables correspond to the edges of a complete graph, node pairs
 * are used mostly instead of edges.
 *
 * \author Markus Chimani, Karsten Klein
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

#include <ogdf/basic/basic.h>

#include <ogdf/cluster/internal/CP_MasterBase.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/fileformats/GraphIO.h>

using namespace ogdf;
using namespace ogdf::cluster_planarity;
using namespace abacus;

#ifdef OGDF_DEBUG
void CP_MasterBase::printGraph(const Graph &G) {
	int i=0;
	Logger::slout() << "The Given Graph" << std::endl;
	for(edge e : G.edges) {
		Logger::slout() << "Edge " << i++ << ": (" << e->source()->index() << "," << e->target()->index() << ") " << std::endl;
	}
}
#endif


CP_MasterBase::CP_MasterBase(
	const ClusterGraph &C,
	int heuristicLevel,
	int heuristicRuns,
	double heuristicOEdgeBound,
	int heuristicNPermLists,
	int kuratowskiIterations,
	int subdivisions,
	int kSupportGraphs,
	double kHigh,
	double kLow,
	bool perturbation,
	double branchingGap,
	const char *time) :
	Master("CPlanarity", true, false, OptSense::Min), //no pricing so far
#ifdef OGDF_DEBUG
	m_solByHeuristic(false),
#endif
	m_solState(solutionState::Undefined),
	m_cutConnPool(nullptr),
	m_cutKuraPool(nullptr),
	m_useDefaultCutPool(true)
{

	// Reference to the given ClusterGraph and the underlying Graph.
	m_C = &C;
	m_G = &(C.constGraph());
	// Create a copy of the graph as we may need to modify it
	m_solutionGraph = new GraphCopy(*m_G);
	// Define the maximum number of variables needed.
	// The actual number needed may be much smaller, so there
	// is room for improvement...
	//ToDo: Just count how many vars are added

	//Max number of edges and extension edges, to be compared
	//with actual number of variables used.
	int nComplete = (m_G->numberOfNodes()*(m_G->numberOfNodes()-1)) / 2;
	m_nMaxVars = nComplete-m_G->numberOfEdges();

	// Initialize the node array to keep track of created variables
	m_varCreated.init(*m_G);
	for(node v : m_G->nodes)
	{
		m_varCreated[v].init(*m_G, false);
	}

	// Computing the main objective function coefficient for the connection edges.
	m_epsilon = (double)(0.2/(2*(m_G->numberOfNodes())));

	// Setting parameters
	m_nKuratowskiIterations = kuratowskiIterations;
	m_nSubdivisions = subdivisions;
	m_nKuratowskiSupportGraphs = kSupportGraphs;
	m_heuristicLevel = heuristicLevel;
	m_nHeuristicRuns = heuristicRuns;
	m_usePerturbation = perturbation;
	m_kuratowskiBoundHigh = kHigh;
	m_kuratowskiBoundLow = kLow;
	m_branchingGap = branchingGap;
	m_maxCpuTime = new string(time);
	m_heuristicFractionalBound = heuristicOEdgeBound;
	m_nHeuristicPermutationLists = heuristicNPermLists;
	m_mpHeuristic = true;

	// Further settings
	m_nCConsAdded = 0;
	m_nKConsAdded = 0;
	m_solvesLP = 0;
	m_varsInit = 0;
	m_varsAdded = 0;
	m_varsPotential = 0;
	m_varsMax = 0;
	m_varsCut = 0;
	m_varsKura = 0;
	m_varsPrice = 0;
	m_varsBranch = 0;
	m_activeRepairs = 0;
	m_repairStat.init(100);
}


CP_MasterBase::~CP_MasterBase() {
	delete m_maxCpuTime;
	delete m_solutionGraph;
}


#if 0
Sub *CP_MasterBase::firstSub() {
	return new CPlanaritySub(this);
}
#endif


// Replaces current m_solutionGraph by new GraphCopy based on \p connection list
void CP_MasterBase::updateBestSubGraph(List<NodePair> &connection) {

	// Creates a new GraphCopy #m_solutionGraph and deletes all edges
	// TODO: Extend GraphCopySimple to be usable here: Allow
	// edge deletion and add pure node initialization.
	// Is the solutiongraph used during computation anyhow?
	// Otherwise only store the lists
	delete m_solutionGraph;
	m_solutionGraph = new GraphCopy(*m_G);

	// Delete all edges that have been stored previously in edge lists
	m_connectionOneEdges.clear();

	for(const NodePair &np : connection)
	{
		// Add all new connection edges to #m_solutionGraph
		node cv = m_solutionGraph->copy(np.source);
		node cw = m_solutionGraph->copy(np.target);
		m_solutionGraph->newEdge(cv,cw);

		m_connectionOneEdges.pushBack(np);
	}

#ifdef OGDF_CPLANAR_DEBUG_OUTPUT
	GraphIO::write(*m_solutionGraph, "UpdateSolutionGraph.gml", GraphIO::writeGML);
#endif
}


void CP_MasterBase::getConnectionOptimalSolutionEdges(List<NodePair> &edges) const
{
	edges.clear();

	for (const NodePair &np : m_connectionOneEdges) {
		edges.pushBack(np);
	}
}


//todo: is called only once, but could be sped up the same way as the co-conn check
//Returns number of edges to be added to achieve cluster connectivity for \p c
double CP_MasterBase::clusterConnection(cluster c, GraphCopy &gc) {
	// For better performance, a node array is used to indicate which nodes are contained
	// in the currently considered cluster.
	NodeArray<bool> vInC(gc,false);
	double connectNum = 0.0; //Minimum number of connection edges
	// First check, if the current cluster \p c is a leaf cluster.
	// If so, compute the number of edges that have at least to be added
	// to make the cluster induced graph connected.
	if (c->cCount()==0) { 	//cluster \p c is a leaf cluster
		GraphCopy *inducedC = new GraphCopy((const Graph&)gc);
		List<node> clusterNodes;
		c->getClusterNodes(clusterNodes); // \a clusterNodes now contains all (original) nodes of cluster \p c.
		for (node v : clusterNodes) {
			vInC[gc.copy(v)] = true;
		}

		// Delete all nodes from \a inducedC that do not belong to the cluster,
		// in order to obtain the cluster induced graph.
		node v = inducedC->firstNode();
		while (v!=nullptr)  {
			node w = v->succ();
			if (!vInC[inducedC->original(v)]) inducedC->delNode(v);
			v = w;
		}

		// Determine number of connected components of cluster induced graph.
		//Todo: check could be skipped
		if (!isConnected(*inducedC)) {

			NodeArray<int> conC(*inducedC);
			//at least #connected components - 1 edges have to be added.
			connectNum = connectedComponents(*inducedC,conC) - 1;
		}
		delete inducedC;
	// Cluster \p c is an "inner" cluster. Process all child clusters first.
	} else {	//c->cCount is != 0, process all child clusters first
		for (cluster cc : c->children) {
			connectNum += clusterConnection(cc,gc);
		}

		// Create cluster induced graph.
		GraphCopy *inducedC = new GraphCopy((const Graph&)gc);
		List<node> clusterNodes;
		c->getClusterNodes(clusterNodes); //\a clusterNodes now contains all (original) nodes of cluster \p c.
		for (node v : clusterNodes) {
			vInC[gc.copy(v)] = true;
		}
		node v = inducedC->firstNode();
		while (v!=nullptr)  {
			node w = v->succ();
			if (!vInC[inducedC->original(v)]) inducedC->delNode(v);
			v = w;
		}

		// Now collapse each child cluster to one node and determine #connected components of \a inducedC.
		List<node> oChildClusterNodes;
		List<node> cChildClusterNodes;
		for (cluster cc : c->children) {
			cc->getClusterNodes(oChildClusterNodes);
			// Compute corresponding nodes of graph \a inducedC.
			for (node vi : oChildClusterNodes) {
				node copy = inducedC->copy(gc.copy(vi));
				cChildClusterNodes.pushBack(copy);
			}
			inducedC->collapse(cChildClusterNodes);
			oChildClusterNodes.clear();
			cChildClusterNodes.clear();
		}
		// Now, check \a inducedC for connectivity.
		if (!isConnected(*inducedC)) {

			NodeArray<int> conC(*inducedC);
			//at least #connected components - 1 edges have to added.
			connectNum += connectedComponents(*inducedC,conC) - 1;
		}
		delete inducedC;
	}
	return connectNum;
}

double CP_MasterBase::heuristicInitialLowerBound()
{
	//Heuristics?
	/*
		 * Heuristic can be improved by checking, how many additional C-edges have to be added at least.
		 * A first simple approach is the following:
		 * Since the Graph has to be completely connected in the end, all chunks have to be connected.
		 * Thus the numbers of chunks minus 1 summed up over all clusters is a trivial lower bound.

		* We perform a bottom-up search through the cluster-tree, each time checking the cluster
		 * induced Graph for connectivity. If the Graph is not connected, the number of chunks -1 is added to
		 * a counter. For "inner" clusters we have to collapse all child clusters to one node,
		 * in order to obtain a correct result.
		 */

	GraphCopy gcc(*m_G);
	cluster c = m_C->rootCluster();

	return clusterConnection(c, gcc);
}

double CP_MasterBase::heuristicInitialUpperBound() {

	//Todo: Nice heuristic
	//Can we just use the number of edges needed
	//to make both the clusters and their complement connected?
	return m_nMaxVars;
}

void CP_MasterBase::nodeDistances(node u, NodeArray<NodeArray<int> > &dist) {

	// Computing the graphtheoretical distances of node u
	NodeArray<bool> visited(*m_G);
	List<node> queue;
	visited.fill(false);
	visited[u] = true;
	int nodesVisited = 1;
	for(adjEntry adj : u->adjEntries) {
		visited[adj->twinNode()] = true;
		nodesVisited++;
		dist[u][adj->twinNode()] += 1;
		queue.pushBack(adj->twinNode());
	}
	while (!queue.empty() || nodesVisited!=m_G->numberOfNodes()) {
		node v = queue.front();
		queue.popFront();
		for(adjEntry adj : v->adjEntries) {
			if (!visited[adj->twinNode()]) {
				visited[adj->twinNode()] = true;
				nodesVisited++;
				dist[u][adj->twinNode()] += (dist[u][v]+1);
				queue.pushBack(adj->twinNode());
			}
		}
	}
}

// Create variables for complete connectivity - any solution allowed
void CP_MasterBase::createCompConnVars(List<CPlanarEdgeVar*>& initVars)
{
#if 0
	initVars.clear(); // We don't care if there are already vars added
#endif
	//We create a copy of the clustergraph and insert connections to
	//make the clusters connected. Afterwards, we check if the complements
	//need to be made connected and add corresponding edges
	Graph G;
	ClusterArray<cluster> oriCluster(*m_C);
	NodeArray<node> copyNode(*m_G);
	ClusterGraph cg(*m_C, G, oriCluster, copyNode);

	NodeArray<node> oriNode(G);
	for(node w : m_G->nodes)
	{
		oriNode[copyNode[w]] = w;
	}

	// First, we add the edges to make the clusters connected
	List<edge> addedEdges;
	//Todo: Use post order traversal to do this without recursion
	//forall_postOrderClusters(c,C)
	makeCConnected(cg, G, addedEdges, true); //use simple cc method
	for (edge e : addedEdges)
	{
		node u = e->source();
		node v = e->target();
		initVars.pushBack(createVariable(oriNode[u], oriNode[v]));
#ifdef OGDF_DEBUG
		std::cout << "Added var " << oriNode[u]->index() << ":" << oriNode[v]->index() << "\n";
#endif
	}

	// Now complement connnectivity (TODO)
	// TODO: do this optionally, experimentally compare performance

}

#if 0
//create the variables at start of optimization
void CP_MasterBase::createInitialVariables(List<CPlanarEdgeVar*>& initVars) {
	// In case of pricing, create an initial variable pool allowing
	// connectivity
	if (pricing())
		createCompConnVars(initVars);
}

//! Checks which of the inactive vars are needed to cover all chunk connection constraints.
//! Those then are added to the connectVars.
void CP_MasterBase::generateVariablesForFeasibility(
		const List<ChunkConnection*>& ccons,
		List<CPlanarEdgeVar*>& connectVars)
{
	List<ChunkConnection*> cpy(ccons);
#if 0
	for(ChunkConnection *cc : cpy) {
		cc->printMe();
	}
#endif

	//First we check which of the constraints are already covered by existing
	//connect vars and delete them.
	ListIterator<CPlanarEdgeVar*> itev = connectVars.begin();
	while (itev.valid())
	{
		nodePair np((*itev)->sourceNode(),(*itev)->targetNode());
		ListIterator<ChunkConnection*> ccit = cpy.begin();
		while(ccit.valid()) {
			if((*ccit)->coeff(np)) {
				ListIterator<ChunkConnection*> delme = ccit;
				++ccit;
				cpy.del(delme);
			} else
				++ccit;
		}
		itev++;
	}

	ArrayBuffer<ListIterator<nodePair> > creationBuffer(ccons.size());
	for(ListIterator<nodePair> npit = m_inactiveVariables.begin(); npit.valid(); ++npit) {
		bool select = false;

		ListIterator<ChunkConnection*> ccit = cpy.begin();
		while(ccit.valid()) {
			if((*ccit)->coeff(*npit)) {
				ListIterator<ChunkConnection*> delme = ccit;
				++ccit;
				cpy.del(delme);
				select = true;
			} else
				++ccit;
		}
		if(select) {
			creationBuffer.push(npit);
		}
		if(cpy.size()==0) break;
	}

	OGDF_ASSERT(cpy.size()==0);
	Logger::slout() << "Creating " << creationBuffer.size() << " Connect-Variables for feasibility\n";
	m_varsInit = creationBuffer.size();
	// realize creationList
	for(int i = creationBuffer.size(); i-- > 0;) {
		connectVars.pushBack( createVariable( creationBuffer[i] ) );
	}
}
#endif

// returns coefficients of all variables in connect in constraint con
// as list coeffs
void CP_MasterBase::getCoefficients(
	Constraint* con,
	const List<CPlanarEdgeVar* > &connect,
	List<double> &coeffs)
{
	coeffs.clear();
	for(CPlanarEdgeVar *cv : connect) {
		coeffs.pushBack(con->coeff(cv));
	}
}


//output statistics
void CP_MasterBase::terminateOptimization() {
	Logger::slout() << "=================================================\n";
	Logger::slout() << "Terminate Optimization:\n";
	Logger::slout() << "(primal Bound: " << primalBound() << ")\n";
	Logger::slout() << "(dual Bound: " << dualBound() << ")\n";
#if 0
	if(m_checkCPlanar2) {
#endif
	Logger::slout() << "*** " << (isCP() ? "" : "NON ") << "C-PLANAR ***\n";
#if 0
	} else {
		Logger::slout() << "*** " << (feasibleFound() ? "" : "NON ") << "C-PLANAR ***\n";
	}
#endif
	Logger::slout() << "=================================================\n";

	Logger::ssout() << "\n";

	Logger::ssout() << "C-Planar: " << isCP() << "\n";
	Logger::ssout() << "Time: "<< getDoubleTime(totalTime()) << "\n";
	Logger::ssout() << "LP-Time: " << getDoubleTime(lpSolverTime()) << "\n";
	Logger::ssout() << "\n";
	Logger::ssout() << "#BB-nodes: " << nSub() << "\n";
	Logger::ssout() << "#LP-relax: " << m_solvesLP << "\n";

	Logger::ssout() << "#Cut Constraints: " << m_nCConsAdded << "\n";
	Logger::ssout() << "#Kura Constraints: " << m_nKConsAdded << "\n";
	Logger::ssout() << "#Vars-init: " << m_varsInit << "\n";
	Logger::ssout() << "#Vars-used: " << m_varsAdded << "\n";
	Logger::ssout() << "#Vars-potential: " << m_varsPotential << "\n";
	Logger::ssout() << "#Vars-max: " << m_varsMax << "\n";
	Logger::ssout() << "#Vars-cut: " << m_varsCut << "\n";
	Logger::ssout() << "#Vars-kurarepair: " << m_varsKura << "\n";
	Logger::ssout() << "#Vars-price: " << m_varsPrice << "\n";
	Logger::ssout() << "#Vars-branch: " << m_varsBranch << "\n";
	Logger::ssout() << "#Vars-unused: " << m_inactiveVariables.size() << "\n";
	Logger::ssout() << "KuraRepair-Stat: <";
	for(auto &elem : m_repairStat) {
		Logger::ssout() << elem << ",";
	}
	Logger::ssout() << ">\n";

	for(node n : m_G->nodes) {
		for(node m : m_G->nodes) {
			if(m->index()<=n->index()) continue;
			for(adjEntry adj : n->adjEntries) {
				if(adj->twinNode()==m) {
					Logger::slout() << "ORIG: " << n << "-" << m << "\n";
					continue;
				}
			}
		}
	}
	for (node n : m_G->nodes) {
		for (node m : m_G->nodes) {
			if (m->index() <= n->index()) continue;
			for(adjEntry adj : n->adjEntries) {
				if (adj->twinNode() == m) {
					goto wup;
				}
			}
			for (const NodePair &p : m_inactiveVariables) {
				if ((p.source == n && p.target == m) || (p.target == n && p.source == m)) {
					goto wup;
				}
			}
			Logger::slout() << "CONN: " << n << "-" << m << "\n";
		wup:;
		}
	}

	globalPrimalBound = primalBound();
	globalDualBound = dualBound();
}
