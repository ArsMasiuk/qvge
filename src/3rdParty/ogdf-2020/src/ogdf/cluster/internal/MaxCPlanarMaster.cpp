/** \file
 * \brief Implementation of the master class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem
 *
 * This class is managing the optimization.
 * Variables and initial constraints are generated and pools are initialized.
 * Since variables correspond to the edges of a complete graph, node pairs
 * are used mostly instead of edges.
 *
 * \author Markus Chimani, Mathias Jansen, Karsten Klein
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
#include <ogdf/cluster/internal/MaxCPlanarMaster.h>
#include <ogdf/cluster/internal/MaxCPlanarSub.h>
#include <ogdf/cluster/internal/ChunkConnection.h>
#include <ogdf/cluster/internal/MaxPlanarEdgesConstraint.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
//heuristics in case only max planar subgraph is computed
#include <ogdf/planarity/PlanarSubgraphFast.h>
#include <ogdf/planarity/MaximalPlanarSubgraphSimple.h>
#include <ogdf/fileformats/GraphIO.h>

using namespace ogdf;
using namespace ogdf::cluster_planarity;
using namespace abacus;

#ifdef OGDF_DEBUG
void MaxCPlanarMaster::printGraph(const Graph &G) {
	int i=0;
	Logger::slout() << "The Given Graph" << std::endl;
	for(edge e : G.edges) {
		Logger::slout() << "Edge " << i++ << ": (" << e->source()->index() << "," << e->target()->index() << ") " << std::endl;
	}
}
#endif

MaxCPlanarMaster::MaxCPlanarMaster(
	const ClusterGraph &C,
	const EdgeArray<double> *pCost,
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
	const char *time,
	bool dopricing,
	bool checkCPlanar,
	int numAddVariables,
	double strongConstraintViolation,
	double strongVariableViolation) : 	Master("MaxCPlanar", true, dopricing, OptSense::Max),
	m_pCost(pCost),
#ifdef OGDF_DEBUG
	m_solByHeuristic(false),
#endif
	m_numAddVariables(numAddVariables),
	m_strongConstraintViolation(strongConstraintViolation),
	m_strongVariableViolation(strongVariableViolation),
	m_fastHeuristicRuns(25),
	m_cutConnPool(nullptr),
	m_cutKuraPool(nullptr),
	m_useDefaultCutPool(true),
	m_checkCPlanar(checkCPlanar),
	m_porta(false)
{
	// Reference to the given ClusterGraph and the underlying Graph.
	m_C = &C;
	m_G = &(C.constGraph());

#ifdef OGDF_DEBUG
	if(m_pCost != nullptr) {
		OGDF_ASSERT(m_G == m_pCost->graphOf());
	}
#endif

	// Create a copy of the graph as we need to modify it
	m_solutionGraph = new GraphCopy(*m_G);
	// Define the maximum number of variables needed.
	// The actual number needed may be much smaller, so there
	// is room for improvement...
	//ToDo: Just count how many vars are added

	//Max number of edges
	//KK: Check this change, added div 2
	int nComplete = (m_G->numberOfNodes()*(m_G->numberOfNodes()-1)) / 2;
	m_nMaxVars = nComplete;
	//to use less variables in case we have only the root cluster,
	//we temporarily set m_nMaxVars to the number of edges
	if ( (m_C->numberOfClusters() == 1) && (isConnected(*m_G)) )
		m_nMaxVars = m_G->numberOfEdges();

	// Computing the main objective function coefficient for the connection edges.
#if 0
	int nConnectionEdges = nComplete - m_G->numberOfEdges();
#endif
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


MaxCPlanarMaster::~MaxCPlanarMaster() {
	delete m_maxCpuTime;
	delete m_solutionGraph;
}


Sub *MaxCPlanarMaster::firstSub() {
	return new MaxCPlanarSub(this);
}


// Replaces current m_solutionGraph by new GraphCopy based on \p connection list
void MaxCPlanarMaster::updateBestSubGraph(List<NodePair> &original, List<NodePair> &connection, List<edge> &deleted) {

	// Creates a new GraphCopy #m_solutionGraph and deletes all edges
	// TODO: Extend GraphCopySimple to be usable here: Allow
	// edge deletion and add pure node initialization
	// Is the solutiongraph used during computation anyhow?
	// Otherwise only store the lists
	delete m_solutionGraph;
	m_solutionGraph = new GraphCopy(*m_G);
	edge e = m_solutionGraph->firstEdge();
	edge succ;
	while (e!=nullptr) {
		succ = e->succ();
		m_solutionGraph->delEdge(e);
		e = succ;
	}

	// Delete all edges that have been stored previously in edge lists
	m_allOneEdges.clear();
	m_originalOneEdges.clear();
	m_connectionOneEdges.clear();
	m_deletedOriginalEdges.clear();

	// Update the edge lists according to new solution
	for(const NodePair &np : original) {

		// Add all original edges to #m_solutionGraph
		node cv = m_solutionGraph->copy(np.source);
		node cw = m_solutionGraph->copy(np.target);
		m_solutionGraph->newEdge(cv,cw);

		m_allOneEdges.pushBack(np);
		m_originalOneEdges.pushBack(np);
	}

	for(const NodePair &np : connection)
	{
		// Add all new connection edges to #m_solutionGraph
		node cv = m_solutionGraph->copy(np.source);
		node cw = m_solutionGraph->copy(np.target);
		m_solutionGraph->newEdge(cv,cw);

		m_allOneEdges.pushBack(np);
		m_connectionOneEdges.pushBack(np);
	}

	for(edge ed : deleted) {
		m_deletedOriginalEdges.pushBack(ed);
	}

#ifdef OGDF_CPLANAR_DEBUG_OUTPUT
	GraphIO::write(*m_solutionGraph, "UpdateSolutionGraph.gml", GraphIO::writeGML);
	//Just for special debugging purposes:
	ClusterArray<cluster> ca(*m_C);
	Graph GG;
	NodeArray<node> na(*m_G);
	ClusterGraph CG(*m_C, GG, ca, na);
	List<edge> le;
	for(const NodePair &np : connection)
	{
		// Add all new connection edges to #m_solutionGraph
		node cv = na[np.source];
		node cw = na[np.target];
		le.pushBack(GG.newEdge(cv,cw));
	}
	ClusterGraphAttributes CGA(CG, GraphAttributes::edgeType | GraphAttributes::nodeType |
			GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics | GraphAttributes::edgeStyle);
	for(edge ei : le)
	{
#ifdef OGDF_DEBUG
		std::cout << ei->graphOf() << "\n";
		std::cout << &GG << "\n";
#endif
		CGA.strokeColor(ei) = Color::Name::Red;
	}
	GraphIO::write(CGA, "PlanarExtensionMCPSP.gml", GraphIO::writeGML);
#endif
}


void MaxCPlanarMaster::getAllOptimalSolutionEdges(List<NodePair> &edges) const
{
	edges.clear();
	for (const NodePair &np : m_allOneEdges) {
		edges.pushBack(np);
	}
}


void MaxCPlanarMaster::getOriginalOptimalSolutionEdges(List<NodePair> &edges) const
{
	edges.clear();
	for (const NodePair &np : m_originalOneEdges) {
		edges.pushBack(np);
	}
}


void MaxCPlanarMaster::getConnectionOptimalSolutionEdges(List<NodePair> &edges) const
{
	edges.clear();
	for (const NodePair &np : m_connectionOneEdges) {
		edges.pushBack(np);
	}
}


void MaxCPlanarMaster::getDeletedEdges(List<edge> &edges) const
{
	edges.clear();
	for (edge e : m_deletedOriginalEdges) {
		edges.pushBack(e);
	}
}

//todo: is called only once, but could be sped up the same way as the co-conn check
void MaxCPlanarMaster::clusterConnection(cluster c, GraphCopy &gc, double &upperBoundC) {
	// For better performance, a node array is used to indicate which nodes are contained
	// in the currently considered cluster.
	NodeArray<bool> vInC(gc,false);
	// First check, if the current cluster \p c is a leaf cluster.
	// If so, compute the number of edges that have at least to be added
	// to make the cluster induced graph connected.
	if (c->cCount()==0) { 	//cluster \p c is a leaf cluster
		GraphCopy inducedC((const Graph&)gc);
		List<node> clusterNodes;
		c->getClusterNodes(clusterNodes); // \a clusterNodes now contains all (original) nodes of cluster \p c.
		for (node w : clusterNodes) {
			vInC[gc.copy(w)] = true;
		}

		// Delete all nodes from \a inducedC that do not belong to the cluster,
		// in order to obtain the cluster induced graph.
		node v = inducedC.firstNode();
		while (v!=nullptr)  {
			node w = v->succ();
			if (!vInC[inducedC.original(v)]) inducedC.delNode(v);
			v = w;
		}

		// Determine number of connected components of cluster induced graph.
		//Todo: check could be skipped
		if (!isConnected(inducedC)) {

			NodeArray<int> conC(inducedC);
			int nCC = connectedComponents(inducedC, conC);
			//at least #connected components - 1 edges have to be added.
			upperBoundC -= (nCC-1)*m_largestConnectionCoeff;
		}
	// Cluster \a c is an "inner" cluster. Process all child clusters first.
	} else {	//c->cCount is != 0, process all child clusters first

		for (cluster ci : c->children) {
			clusterConnection(ci, gc, upperBoundC);
		}

		// Create cluster induced graph.
		GraphCopy inducedC((const Graph&)gc);
		List<node> clusterNodes;
		c->getClusterNodes(clusterNodes); //\a clusterNodes now contains all (original) nodes of cluster \p c.
		for (node w : clusterNodes) {
			vInC[gc.copy(w)] = true;
		}
		node v = inducedC.firstNode();
		while (v!=nullptr)  {
			node w = v->succ();
			if (!vInC[inducedC.original(v)]) inducedC.delNode(v);
			v = w;
		}

		// Now collapse each child cluster to one node and determine #connected components of \a inducedC.
		List<node> oChildClusterNodes;
		List<node> cChildClusterNodes;
		for (cluster ci : c->children) {
			ci->getClusterNodes(oChildClusterNodes);
			// Compute corresponding nodes of graph \a inducedC.
			for (node u : oChildClusterNodes) {
				node copy = inducedC.copy(gc.copy(u));
				cChildClusterNodes.pushBack(copy);
			}
			inducedC.collapse(cChildClusterNodes);
			oChildClusterNodes.clear();
			cChildClusterNodes.clear();
		}
		// Now, check \a inducedC for connectivity.
		if (!isConnected(inducedC)) {

			NodeArray<int> conC(inducedC);
			int nCC = connectedComponents(inducedC, conC);
			//at least #connected components - 1 edges have to added.
			upperBoundC -= (nCC-1)*m_largestConnectionCoeff;
		}
	}
}

double MaxCPlanarMaster::heuristicInitialLowerBound()
{
	double lbound = 0.0;
	//In case we only have a single (root) cluster, we can
	//use the result of a fast Max Planar Subgraph heuristic
	//to initialize the lower bound
	if ( (m_C->numberOfClusters() == 1) && (m_mpHeuristic) )
	{
		//we run both heuristics that currently exist in OGDF
		//MaxSimple
		MaximalPlanarSubgraphSimple<double> simpleHeur;
		List<edge> delEdgesList;
		PlanarSubgraphFast<double> fastHeur;
		fastHeur.runs(m_fastHeuristicRuns);
		List<edge> delEdgesListFast;
		List<edge> *delEdges;

		if(m_pCost == nullptr) {
			simpleHeur.call(*m_G, delEdgesList);
			fastHeur.call(*m_G, delEdgesListFast);

			delEdges = delEdgesList.size() < delEdgesListFast.size() ? &delEdgesList : &delEdgesListFast;
			lbound = m_G->numberOfEdges()-min(delEdgesList.size(), delEdgesListFast.size());
		} else {
			simpleHeur.call(*m_G, *m_pCost, delEdgesList);
			fastHeur.call(*m_G, *m_pCost, delEdgesListFast);

			int total = 0;
			for(edge e : m_G->edges) {
				total += (*m_pCost)[e];
			}

			int del = 0;
			for(edge e : delEdgesList) {
				del += (*m_pCost)[e];
			}

			int delFast = 0;
			for(edge e : delEdgesListFast) {
				delFast += (*m_pCost)[e];
			}

			delEdges = del < delFast ? &delEdgesList : &delEdgesListFast;
			lbound = total - min(del, delFast);
		}

		m_deletedOriginalEdges.clear();
		for(edge e : *delEdges) {
			m_deletedOriginalEdges.pushBack(e);
		}

		if (!isConnected(*m_G)) lbound = lbound-1.0; //#edges*epsilon
	}
	return lbound;
}

double MaxCPlanarMaster::heuristicInitialUpperBound() {

	double upperBoundO( m_G->numberOfEdges() );
	double upperBoundC = 0.0;

	if(m_pCost != nullptr) {
		upperBoundO = 0;
		for(edge e : m_G->edges) {
			upperBoundO += (*m_pCost)[e];
		}
	}

	// Checking graph for planarity
	// If #m_G is planar \a upperBound is simply set to the number of edges of #m_G.
	GraphCopy gc(*m_G);
	BoyerMyrvold bm;
	if (!bm.isPlanarDestructive(gc)) {

		// Extract all possible Kuratowski subdivisions.
		// Compare extracted subdivisions and try to obtain the
		// maximum number of independent subdivisions, i.e. a maximum
		// independent set in the overlap graph.
		// Due to the complexity of this task, we only check if
		// a subdivision (sd) does overlap with a subdivision for which
		// we already decreased the upper bound. In this case,
		// upperBound stays the same.

		GraphCopy gCopy(*m_G);
		SList<KuratowskiWrapper> subDivs;

		bm.planarEmbedDestructive(gCopy, subDivs,-1);
		//we store a representative and its status for each edge
		//note that there might be an overlap, in that case
		//we keep a representative with status false if existing
		//to check if we can safely reduce the upper bound (ub)
		EdgeArray<edge> subRep(gCopy, nullptr); //store representing edge for sd
		EdgeArray<bool> coverStatus(gCopy, false); //false means not covered by ub decrease yet

		//runtime for the check: we run over all edges in all subdivisions
		if (subDivs.size() > 0) { // At least one edge has to be deleted to obtain planarity.

			// We run over all subdivisions and check for overlaps
			for(const KuratowskiWrapper &kw : subDivs)
			{
				bool covered = false; //may the sd already be covered by a decrease in ub
				//for each edge we set the representative to be the first edge of sd
				edge sdRep = kw.edgeList.front(); //sd is always non-empty
				//we check if any of the edges in sd were already visited and if
				//the representative has status false, in this case, we are not
				//allowed to decrease the ub
				for (edge e : kw.edgeList)
				{
					//we already encountered this edge
					if (subRep[e] != nullptr)
					{
						//and decreased ub for an enclosing sd
						//(could we just break in the if case?)
						if (coverStatus[subRep[e]])
							covered = true;
						else
							subRep[e] = sdRep; //might need an update
					}
					else
						subRep[e] = sdRep;
				}
				if (!covered)
				{
					coverStatus[sdRep] = true;
					upperBoundO--;
				}
			}
		}
	}

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
	clusterConnection(c, gcc, upperBoundC);

	// Return-value results from the max. number of O-edges that might be contained
	// in an optimal solution minus \a epsilon times the number of C-edges that have
	// to be added at least in any optimal solution. (\a upperBoundC is non-positive)
	return upperBoundO + upperBoundC;
}

void MaxCPlanarMaster::nodeDistances(node u, NodeArray<NodeArray<int> > &dist) {

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

bool MaxCPlanarMaster::goodVar(node a, node b) {
	return true; //add all variables even if they are bad (paper submission)
	Logger::slout() << "Good Var? " << a << "->" << b << ": ";
	GraphCopy GC(*m_G);
	GC.newEdge(GC.copy(a),GC.copy(b));
	BoyerMyrvold bm;
	bool ret =  bm.isPlanarDestructive(GC);
	Logger::slout() << ret << "\n";
	return ret;
}

void MaxCPlanarMaster::initializeOptimization() {

	//we don't try heuristic improvement (edge addition) in MaxCPlanarSub
	//when only checking c-planarity
	if (m_checkCPlanar) {
		heuristicLevel(0);
		//TODO Shortcut: lowerbound number original edges (-1) if not pricing
#if 0
		enumerationStrategy(BreadthFirst);
#endif
	}

	if (pricing())
		varElimMode(NoVarElim);
	else
		varElimMode(ReducedCost);
	conElimMode(Basic);
	if(pricing())
		pricingFreq(1);

	// Creation of Variables

	// Lists for original and connection edges
	List<EdgeVar*> origVars;    // MCh: ArrayBuffer would speed this up
	List<EdgeVar*> connectVars; // MCh: ArrayBuffer would speed this up

	//cluster connectivity only necessary if there are clusters (not for max planar subgraph)
	bool toBeConnected = (!( (m_C->numberOfClusters() == 1) && (isConnected(*m_G)) ) );

	int nComplete = (m_G->numberOfNodes()*(m_G->numberOfNodes()-1))/2;
	int nConnectionEdges = nComplete - m_G->numberOfEdges();

	double perturbMe = (m_usePerturbation)? 0.2*m_epsilon : 0;
	m_deltaCount = nConnectionEdges;
	m_delta  = (m_deltaCount > 0) ? perturbMe/m_deltaCount : 0;
#if 0
	double coeff;
#endif

	// In order not to place the initial upper bound too low,
	// we use the largest connection edge coefficient for each C-edge
	// to lower the upper bound (since these coeffs are negative,
	// this corresponds to the coeff that is closest to 0).
	m_largestConnectionCoeff = 0.8*m_epsilon;
	m_varsMax = 0;
	for(node u : m_G->nodes) {
		node v = u->succ();
		while (v!=nullptr) {//todo could skip searchedge if toBeConnected
			edge e = m_G->searchEdge(u,v);
			if(e != nullptr)
				origVars.pushBack(new EdgeVar(this, (m_pCost == nullptr ? 1 : (*m_pCost)[e]) + rand()*perturbMe,EdgeVar::EdgeType::Original,u,v));
			else if (toBeConnected) {
				if( (!m_checkCPlanar) || goodVar(u,v)) {
					if(pricing())
						m_inactiveVariables.pushBack(NodePair(u,v));
					else
						connectVars.pushBack( new EdgeVar(this, nextConnectCoeff(), EdgeVar::EdgeType::Connect, u,v) );
				}
				++m_varsMax;
			}
			v = v->succ();
		}
	}
	m_varsPotential = m_inactiveVariables.size();

	// Creation of ChunkConnection-Constraints

	int nChunks = 0;

	List<ChunkConnection*> constraintsCC;

	// The Graph, in which each cluster-induced Graph is temporarily stored
	Graph subGraph;

	// Since the function inducedSubGraph(..) creates a new Graph #subGraph, the original
	// nodes have to be mapped to the copies. This mapping is stored in #orig2new.
	NodeArray<node> orig2new;

	// Iterate over all clusters of the Graph
	for(cluster c : m_C->clusters) {

		List<node> clusterNodes;
		c->getClusterNodes(clusterNodes);

		// Compute the cluster-induced Subgraph
		ListConstIterator<node> it = clusterNodes.begin();
		inducedSubGraph(*m_G, it, subGraph, orig2new);

		// Compute the number of connected components
		NodeArray<int> components(subGraph);
		int nCC = connectedComponents(subGraph,components);
		nChunks+=nCC;
		// If the cluster consists of more than one connected component,
		// ChunkConnection constraints can be created.
		if (nCC > 1) {

			// Determine each connected component (chunk) of the current cluster-induced Graph
			for (int i=0; i<nCC; ++i) {

				ArrayBuffer<node> cC(subGraph.numberOfNodes());
				ArrayBuffer<node> cCComplement(subGraph.numberOfNodes());
				for(node v : m_G->nodes/*subGraph*/) {
					node n = orig2new[v];
					if(n) {
						if (components[n] == i) cC.push(v);
						else cCComplement.push(v);
					}
				}
				// Creating corresponding constraint
				constraintsCC.pushBack(new ChunkConnection(this, cC, cCComplement));
				// Avoiding duplicates if cluster consists of 2 chunks
				if (nCC == 2) break;

			}
		}
	}
	if(pricing())
		generateVariablesForFeasibility(constraintsCC, connectVars);

	// Creation of MinimalClusterConnection-Constraints
#if 0
	List<MinimalClusterConnection*> constraintsMCC;
	cluster succ;
	ClusterArray<bool> connected(*m_C);
	// For each cluster run through all cluster vertices and check if
	// they have an outgoing edge to a different cluster.
	// In this case, mark that cluster in connected as true.
	for(cluster c : m_C->clusters) {

		succ = c->succ();
		connected.fill(false);
		List<node> clusterNodes;
		c->getClusterNodes(clusterNodes);
		ListConstIterator<node> it;
		for (it=clusterNodes.begin(); it.valid(); ++it) {
			for(adjEntry adj : (*it).adjEntries) {
				if(m_C->clusterOf(adj->twinNode()) != c) {
					connected[m_C->clusterOf(adj->twinNode())] = true;
				}
			}
		}

		//checking if there is an entry in \a connected of value false
		//if so, cluster \p c is not connected to this one and a constraint is created
		List<NodePair> mcc_edges;
		while(succ!=0) {
			if (!connected[succ]) {
				ListConstIterator<node> it;
				//determine all nodePairs between \p c and \a succ and add them to list \a mcc_edges
				for (it=clusterNodes.begin(); it.valid(); ++it) {
					NodePair np;
					for(adjEntry adj : (*it).adjEntries) {
						if (m_C->clusterOf(adj->twinNode()) == succ) {
							np.v1 = (*it);
							np.v2 = adj->twinNode();
							mcc_edges.pushBack(np);
						}
					}
				}
				Logger::slout() << "mcc_edges: " << mcc_edges.size() << std::endl;
				//create new constraint and put it into list \a constraintsMCC
				constraintsMCC.pushBack(new MinimalClusterConnection(this,mcc_edges));
				mcc_edges.clear();
			}
			succ = succ->succ();
		}
	}
#endif

	// Creation of MaxPlanarEdges-Constraints

	List<MaxPlanarEdgesConstraint*> constraintsMPE;

	int nMaxPlanarEdges = 3*m_G->numberOfNodes() - 6;
	constraintsMPE.pushBack(new MaxPlanarEdgesConstraint(this,nMaxPlanarEdges));

	List<node> clusterNodes;
	List<NodePair> clusterEdges;
	for(cluster c : m_C->clusters) {

		if (c == m_C->rootCluster()) continue;
		clusterNodes.clear(); clusterEdges.clear();
		c->getClusterNodes(clusterNodes);
		if (clusterNodes.size() >= 4) {
			NodePair np;
			ListConstIterator<node> it;
			ListConstIterator<node> it_succ;
			for (it=clusterNodes.begin(); it.valid(); ++it) {
				it_succ = it.succ();
				while (it_succ.valid()) {
					np.source = (*it); np.target = (*it_succ);
					clusterEdges.pushBack(np);
					++it_succ;
				}
			}
			int maxPlanarEdges = 3*clusterNodes.size() - 6;
			constraintsMPE.pushBack(new MaxPlanarEdgesConstraint(this,maxPlanarEdges,clusterEdges));
		}
	}


	// Adding Constraints to the Pool

	// Adding constraints to the standardpool
	ArrayBuffer<Constraint *> initConstraints(constraintsCC.size()/*+constraintsMCC.size()*/+constraintsMPE.size(),false);

	updateAddedCCons(constraintsCC.size());
	for (ChunkConnection *cc : constraintsCC) {
		initConstraints.push(cc);
	}
#if 0
	ListConstIterator<MinimalClusterConnection*> mccIt;
	for (mccIt = constraintsMCC.begin(); mccIt.valid(); ++mccIt) {
		initConstraints.push(*mccIt);
	}
#endif
	for (MaxPlanarEdgesConstraint *mpe : constraintsMPE) {
		initConstraints.push(mpe);
	}
	//output these constraints in a file that can be read by the module
	if (m_porta)
	{

		std::ofstream ofs(getStdConstraintsFileName());
		if (!ofs) std::cerr << "Could not open output stream for PORTA constraints file\n";
		else
		{
			ofs << "# Chunkconnection constraints\n";
			//holds the coefficients in a single constraint for all vars defined
			//so far
			List<double> theCoeffs;
			for(ChunkConnection *cs : constraintsCC) {
				getCoefficients(cs, origVars, connectVars, theCoeffs);
				for(double d : theCoeffs) {
					ofs << d << " ";
				}
				//check csense here
				ofs << ">= " << cs->rhs();
				ofs << "\n";
			}
#if 0
			ofs << "# MinimalClusterconnection constraints\n";
			ListConstIterator<MinimalClusterConnection*> mccIt;
			for (mccIt = constraintsMCC.begin(); mccIt.valid(); ++mccIt) {
				getCoefficients((*mccIt), origVars, connectVars, theCoeffs);
				ListConstIterator<double> dIt = theCoeffs.begin();
				while (dIt.valid())
				{
					ofs << (*dIt) << " ";
					dIt++;
				}
				ofs << "<= " << (*mccIt)->rhs();
				ofs << "\n";
			}
#endif
			ofs << "# MaxPlanarEdges constraints\n";
			for (MaxPlanarEdgesConstraint *mpe : constraintsMPE) {
				getCoefficients(mpe, origVars, connectVars, theCoeffs);
				for(double d : theCoeffs) {
					ofs << d << " ";
				}
				ofs << "<= " << mpe->rhs();
				ofs << "\n";
			}
			ofs.close();
		}
	}

	// Adding Variables to the Pool

	// Adding variables to the standardpool
	ArrayBuffer<Variable *> edgeVariables(origVars.size()+connectVars.size(),false);
	for (EdgeVar *ev : origVars) {
		edgeVariables.push(ev);
	}
	for (EdgeVar *ev : connectVars) {
		edgeVariables.push(ev);
	}


	// Initializing the Pools

	int poolsize = (getGraph()->numberOfNodes() * getGraph()->numberOfNodes());
	if (m_useDefaultCutPool)
		initializePools(initConstraints, edgeVariables, m_nMaxVars, poolsize, true);
	else
	{
		initializePools(initConstraints, edgeVariables, m_nMaxVars, 0, false);
		//TODO: How many of them?
		m_cutConnPool = new StandardPool<Constraint, Variable>(this, poolsize, true);
		m_cutKuraPool = new StandardPool<Constraint, Variable>(this, poolsize, true);
	}


	// Initialize Upper Bound

	//if we check only for c-planarity, we cannot set bounds
	if (!m_checkCPlanar)
	{
		double upperBound = heuristicInitialUpperBound(); // TODO-TESTING
		dualBound(upperBound); // TODO-TESTING

	// Initialize Lower Bound

		primalBound(heuristicInitialLowerBound()); // TODO-TESTING
#if 0
	} else {
		primalBound(-m_G->numberOfNodes()*3);
#endif
	}

	// Setting Parameters

#if 0
	conElimMode(Master::NonBinding);
#endif
	maxCpuTime(*m_maxCpuTime);

	Logger::ssout() << "#Nodes: " << m_G->numberOfNodes() << "\n";
	Logger::ssout() << "#Edges: " << m_G->numberOfEdges() << "\n";
	Logger::ssout() << "#Clusters: " << m_C->numberOfClusters() << "\n";
	Logger::ssout() << "#Chunks: " << nChunks << "\n";


}

// returns coefficients of all variables in orig and connect in constraint con
// as list coeffs
void MaxCPlanarMaster::getCoefficients(Constraint* con,  const List<EdgeVar* > & orig,
	const List<EdgeVar* > & connect, List<double> & coeffs)
{
	coeffs.clear();
	for (EdgeVar *ev : orig) {
		coeffs.pushBack(con->coeff(ev));
	}

	for (EdgeVar *ev : connect) {
		coeffs.pushBack(con->coeff(ev));
	}

}


//output statistics
//and change the list of deleted edges in case only c-planarity is tested
//(to guarantee that the list is non-empty if input is not c-planar)
void MaxCPlanarMaster::terminateOptimization() {
	Logger::slout() << "=================================================\n";
	Logger::slout() << "Terminate Optimization:\n";
	Logger::slout() << "(primal Bound: " << primalBound() << ")\n";
	Logger::slout() << "(dual Bound: " << dualBound() << ")\n";
	Logger::slout() << "*** " << (m_deletedOriginalEdges.size() == 0 ? "" : "NON ") << "C-PLANAR ***\n";
	Logger::slout() << "*** " << (feasibleFound() ? "" : "NO ") << "feasible solution found ***\n";
	Logger::slout() << "=================================================\n";

	Logger::ssout() << "\n";

	Logger::ssout() << "C-Planar: " << (feasibleFound() && (m_deletedOriginalEdges.size() == 0)) << "\n";
	Logger::ssout() << "FeasibleFound: " << feasibleFound() << "\n";
	Logger::ssout() << "Time: "<< getDoubleTime(totalTime()) << "\n";
	Logger::ssout() << "LP-Time: " << getDoubleTime(lpSolverTime()) << "\n";
	Logger::ssout() << "\n";
	Logger::ssout() << "#BB-nodes: " << nSub() << "\n";
	Logger::ssout() << "#LP-relax: " << m_solvesLP << "\n";
	Logger::ssout() << "Added Edges: " <<m_connectionOneEdges.size()<<"\n";

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

void MaxCPlanarMaster::generateVariablesForFeasibility(const List<ChunkConnection*>& ccons, List<EdgeVar*>& connectVars) {
	List<ChunkConnection*> cpy(ccons);
#if 0
	for(ChunkConnection *cc : cpy) {
		cc->printMe();
	}
#endif

	ArrayBuffer<ListIterator<NodePair> > creationBuffer(ccons.size());
	for (ListIterator<NodePair> npit = m_inactiveVariables.begin(); npit.valid(); ++npit) {
		bool select = false;
#if 0
		(*npit).printMe();
#endif
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
#if 0
			Logger::slout() << "<--CREATE";
#endif
			creationBuffer.push(npit);
		}
		if(cpy.size()==0) break;
	}
#if 0
	for(ChunkConnection *cc : cpy) {
		cc->printMe();
	}
#endif
	OGDF_ASSERT(cpy.size()==0);
	Logger::slout() << "Creating " << creationBuffer.size() << " Connect-Variables for feasibility\n";
	m_varsInit = creationBuffer.size();
	// realize creationList
	for(int i = creationBuffer.size(); i-- > 0;) {
		connectVars.pushBack( createVariable( creationBuffer[i] ) );
	}
}
