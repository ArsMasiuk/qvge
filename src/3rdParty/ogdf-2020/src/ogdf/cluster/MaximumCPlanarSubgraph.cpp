/** \file
 * \brief Implementation of class MaximumCPlanarSubgraph
 *
 * \author Karsten Klein
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

#include <ogdf/cluster/MaximumCPlanarSubgraph.h>
#include <ogdf/cluster/CconnectClusterPlanar.h>
#include <ogdf/basic/simple_graph_alg.h>

#ifdef OGDF_CPLANAR_DEBUG_OUTPUT
#include <ogdf/fileformats/GraphIO.h>
#endif
using namespace abacus;

namespace ogdf {

using namespace cluster_planarity;

Module::ReturnType MaximumCPlanarSubgraph::doCall(const ClusterGraph &G,
                                                  const EdgeArray<double> *pCost,
                                                  List<edge> &delEdges,
                                                  List<NodePair> &addedEdges)
{
#ifdef OGDF_DEBUG
	std::cout << "Creating new Masterproblem for clustergraph with " << G.constGraph().numberOfNodes() << " nodes\n";
#endif
	MaxCPlanarMaster* cplanMaster = new MaxCPlanarMaster(G, pCost,
		m_heuristicLevel,
		m_heuristicRuns,
		m_heuristicOEdgeBound,
		m_heuristicNPermLists,
		m_kuratowskiIterations,
		m_subdivisions,
		m_kSupportGraphs,
		m_kuratowskiHigh,
		m_kuratowskiLow,
		m_perturbation,
		m_branchingGap,
		m_time.c_str(),
		m_pricing,
		m_checkCPlanar,
		m_numAddVariables,
		m_strongConstraintViolation,
		m_strongVariableViolation);

	cplanMaster->setPortaFile(m_portaOutput);
	cplanMaster->useDefaultCutPool() = m_defaultCutPool;
#ifdef OGDF_DEBUG
	std::cout << "Starting Optimization\n";
#endif
	Master::STATUS status;
	try {
		status = cplanMaster->optimize();
	}
	catch (...)
	{
#ifdef OGDF_DEBUG
		std::cout << "ABACUS Optimization failed...\n";
#endif
	}
	m_totalTime    = getDoubleTime(*cplanMaster->totalTime());
	m_heurTime     = getDoubleTime(*cplanMaster->improveTime());
	m_sepTime      = getDoubleTime(*cplanMaster->separationTime());
	m_lpTime       = getDoubleTime(*cplanMaster->lpTime());
	m_lpSolverTime = getDoubleTime(*cplanMaster->lpSolverTime());
	m_totalWTime   = getDoubleTime(*cplanMaster->totalCowTime());
	m_numKCons     = cplanMaster->addedKConstraints();
	m_numCCons     = cplanMaster->addedCConstraints();
	m_numLPs       = cplanMaster->nLp();
	m_numBCs       = cplanMaster->nSub();
	m_numSubSelected = cplanMaster->nSubSelected();
	m_numVars      = cplanMaster->nMaxVars()-cplanMaster->getNumInactiveVars();
#ifdef OGDF_DEBUG
	m_solByHeuristic = cplanMaster->m_solByHeuristic;
#endif
#ifdef OGDF_DEBUG
	if(cplanMaster->pricing())
		Logger::slout() << "Pricing was ON\n";
	Logger::slout() << "ABACUS returned with status '" << Master::STATUS_[status] << "'" << std::endl;
#endif

	NodePairs allEdges;
	cplanMaster->getDeletedEdges(delEdges);
	cplanMaster->getConnectionOptimalSolutionEdges(addedEdges);
	cplanMaster->getAllOptimalSolutionEdges(allEdges);

#ifdef OGDF_DEBUG
	int delE = delEdges.size();
	int addE = addedEdges.size();

	std::cout << delE << " Number of deleted edges, " << addE << " Number of added edges " << allEdges.size() << " gesamt" << std::endl;
#endif

	if (m_portaOutput)
	{
		writeFeasible(getPortaFileName(), *cplanMaster, status);
	}

	delete cplanMaster;
	switch (status) {
	case Master::Optimal: return Module::ReturnType::Optimal; break;
	case Master::Error: return Module::ReturnType::Error; break;
	default: break;
	}

	return Module::ReturnType::Error;
}

//returns list of all clusters in subtree at c in bottom up order
void MaximumCPlanarSubgraph::getBottomUpClusterList(const cluster c, List< cluster > & theList)
{
	for(cluster cc : c->children) {
		getBottomUpClusterList(cc, theList);
	}
	theList.pushBack(c);
}

//outputs the set of feasible solutions
void MaximumCPlanarSubgraph::writeFeasible(const char *filename,
		MaxCPlanarMaster &master,
		MaxCPlanarMaster::STATUS &status)
{
	const ClusterGraph& CG = *(master.getClusterGraph());
	const Graph& G = CG.constGraph();
	//first compute the nodepairs that are potential candidates to connect
	//chunks in a cluster
	//potential connection edges
	NodeArray< NodeArray<bool> > potConn(G);
	for(node v : G.nodes)
	{
		potConn[v].init(G, false);
	}
	//we perform a bottom up cluster tree traversal
	List< cluster > clist;
	getBottomUpClusterList(CG.rootCluster(), clist);
	//could use postordertraversal instead

	NodePairs connPairs; //holds all connection node pairs
	//counts the number of potential connectivity edges
	//int potCount = 0; //equal to number of true values in potConn

	//we run through the clusters and check connected components
	//we consider all possible edges connecting CCs in a cluster,
	//even if they may be connected by edges in a child cluster
	//(to get the set of all feasible solutions)

	for (cluster c : clist)
	{
		//we compute the subgraph induced by vertices in c
		GraphCopy gcopy;
		gcopy.createEmpty(G);
		List<node> clusterNodes;
		//would be more efficient if we would just merge the childrens' vertices
		//and add c's
		c->getClusterNodes(clusterNodes);
		NodeArray<bool> activeNodes(G, false); //true for all cluster nodes
		EdgeArray<edge> copyEdge(G); //holds the edge copy
		for (node v : clusterNodes) {
			activeNodes[v] = true;
		}
		gcopy.initByActiveNodes(clusterNodes, activeNodes, copyEdge);
		//gcopy now represents the cluster induced subgraph

		//we compute the connected components and store all nodepairs
		//that connect two of them
		NodeArray<int> component(gcopy);
		connectedComponents(gcopy, component);
		//now we run over all vertices and compare the component
		//number of adjacent vertices. If they differ, we found a
		//potential connection edge. We do not care if we find them twice.
		for(node v : gcopy.nodes)
		{
			for(node w : gcopy.nodes)
			{
				if (component[v] != component[w])
				{
					std::cout << "Indizes: " << v->index() << ":" << w->index() << "\n";
					node vg = gcopy.original(v);
					node wg = gcopy.original(w);
					bool newConn = !((vg->index() < wg->index()) ? potConn[vg][wg] : potConn[wg][vg]);
					if (newConn)
					{
						NodePair np;
						np.source = vg;
						np.target = wg;
						connPairs.pushBack(np);
						if (vg->index() < wg->index())
							potConn[vg][wg] = true;
						else
							potConn[wg][vg] = true;
					}
				}
			}
		}
	}

	std::cout << "Potentielle Verbindungskanten: "<< connPairs.size()<<"\n";

	//we run through our candidates and save them in an array
	//that can be used for dynamic graph updates
	int i = 0;

	struct connStruct {
		bool connected;
		node v1, v2;
		edge e;
	};
	connStruct *cons = new connStruct[connPairs.size()];
	for(const NodePair &np : connPairs)
	{
		connStruct cs;
		cs.connected = false;
		cs.v1 = np.source;
		cs.v2 = np.target;
		cs.e  = nullptr;

		cons[i] = cs;
		i++;
	}

	// WARNING: this is extremely slow for graphs with a large number of cluster
	// chunks now we test all possible connection edge combinations for c-planarity
	Graph G2;

	NodeArray<node> origNodes(CG.constGraph());
	ClusterArray<cluster> origCluster(CG);
	EdgeArray<edge> origEdges(CG.constGraph());
	ClusterGraph testCopy(CG, G2, origCluster, origNodes, origEdges);

	std::ofstream os(filename);

	// Output dimension of the LP (number of variables)
	os << "DIM = " << connPairs.size() << "\n";
	os << "COMMENT\n";

	switch (status) {
		case Master::Optimal:
			os << "Optimal \n\n"; break;
		case Master::Error:
			os << "Error \n\n"; break;
		default:
			os << "unknown \n\n";
	}

	for (i = 0; i < connPairs.size(); i++)
	{
		os << "Var " << i << ": " << origNodes[cons[i].v1]->index() << "->" << origNodes[cons[i].v2] << "\n";
	}

	os << "CONV_SECTION\n";

#ifdef OGDF_CPLANAR_DEBUG_OUTPUT
	int writeCount = 0; //debug
#endif

	if (connPairs.size() > 0)
	while (true)
	{
		//we create the next test configuration by incrementing the edge selection array
		//we create the corresponding graph dynamically on the fly
		i = 0;
		while (i < connPairs.size() && cons[i].connected)
		{
			cons[i].connected = false;
			OGDF_ASSERT(cons[i].e != nullptr);
			G2.delEdge(cons[i].e);
			i++;
		}
		if (i >= connPairs.size()) break;
#if 0
		std::cout<<"v1graph: "<<&(*(cons[i].v1->graphOf()))<<"\n";
		std::cout<<"origNodesgraph: "<<&(*(origNodes.graphOf()))<<"\n";
#endif
		cons[i].connected = true; //i.e., (false) will never be a feasible solution
		cons[i].e = G2.newEdge(origNodes[cons[i].v1], origNodes[cons[i].v2]);


		//and test it for c-planarity
		CconnectClusterPlanar CCCP;
		bool cplanar = CCCP.call(testCopy);

		//c-planar graphs define a feasible solution
		if (cplanar)
		{
			std::cout << "Feasible solution found\n";
			for (int j = 0; j < connPairs.size(); j++)
			{
				char ch = (cons[j].connected ? '1' : '0');
				std::cout << ch;
				os << ch << " ";
			}
			std::cout << "\n";
			os << "\n";
#ifdef OGDF_CPLANAR_DEBUG_OUTPUT
			string fn = "cGraph" + to_string(writeCount++) + ".gml";
			std::ofstream out(fn);
			GraphIO::writeGML(testCopy, out);
#endif
		}
	}

	delete[] cons;

	os << "\nEND" <<"\n";
	os.close();

#if 0
	return;
#endif

	os.open(getIeqFileName());
	os << "DIM = " << m_numVars << "\n";
	// Output the status as a comment
	os << "COMMENT\n";

	switch (status) {
		case Master::Optimal:
			os << "Optimal \n\n"; break;
		case Master::Error:
			os << "Error \n\n"; break;
		default:
			os << "unknown \n\n";
	}

	// In case 0 is not a valid solution, some PORTA functions need
	//a valid solution in the ieq file
	os << "VALID\n";

	os << "\nLOWER_BOUNDS\n";

	for (i = 0; i < m_numVars; i++) os << "0 ";
	os << "\n";

	os << "\nHIGHER_BOUNDS\n";
	for (i = 0; i < m_numVars; i++) os << "1 ";
	os << "\n";

	os << "\nINEQUALITIES_SECTION\n";
	//we first read the standard constraint that are written
	//into a text file by the optimization master
	std::ifstream isf(master.getStdConstraintsFileName());
	if (!isf)
	{
		std::cerr << "Could not open optimization master's standard constraint file\n";
		os << "#No standard constraints read\n";
	}
	else
	{
		char* fileLine = new char[maxConLength()];
		while (isf.getline(fileLine, maxConLength()))
		{
			//skip commment lines
			if (fileLine[0] == '#') continue;
			int count = 1;
			std::istringstream iss(fileLine);
			char d;
			bool rhs = false;
			while (iss >> d)
			{
				if ( rhs || ( (d == '<') || (d == '>') || (d == '=') ) )
				{
					os << d;
					rhs = true;
				}
				else
				{
					if (d != '0')
					{
						os <<"+"<< d <<"x"<<count;
					}
					count++;
				}
			}
			os <<"\n";
		}
		delete[] fileLine;
	}
	//now we read the cut pools from the master
	if (master.useDefaultCutPool())
	{
		os << "#No cut constraints read from master\n";
#if 0
		StandardPool<Constraint, Variable> *connCon = master.cutPool();
#endif
	}
	else
	{
		StandardPool<Constraint, Variable> *connCon = master.getCutConnPool();
		StandardPool<Constraint, Variable> *kuraCon = master.getCutKuraPool();
		StandardPool<Variable, Constraint> *stdVar = master.varPool();
		OGDF_ASSERT(connCon != nullptr);
		OGDF_ASSERT(kuraCon != nullptr);
		std::cout << connCon->number() << " Constraints im MasterConnpool \n";
		std::cout << kuraCon->number() << " Constraints im MasterKurapool \n";
		std::cout << connCon->size() << " Größe ConnPool"<<"\n";
		outputCons(os, connCon, stdVar);
		outputCons(os, kuraCon, stdVar);
	}
	os << "\nEND" <<"\n";
	os.close();
	std::cout << "Cutting is set: "<<master.cutting()<<"\n";
#if 0
	std::cout <<"Bounds for the variables:\n";
	Sub &theSub = *(master.firstSub());
	for ( i = 0; i < theSub.nVar(); i++)
	{
		std::cout << i << ": " << theSub.lBound(i) << " - " << theSub.uBound(i) << "\n";
	}
#endif
#if 0
	// OLD CRAP
	std::cout << "Constraints: \n";
	StandardPool< Constraint, Variable > *spool = master.conPool();
	StandardPool< Constraint, Variable > *cpool = master.cutPool();

	std::cout << spool->size() << " Constraints im Masterpool \n";
	std::cout << cpool->size() << " Constraints im Mastercutpool \n";

	std::cout << "ConPool Constraints \n";
	for ( i = 0; i < spool->size(); i++)
	{
		PoolSlot< Constraint, Variable > * sloty = spool->slot(i);
		Constraint *mycon = sloty->conVar();
		switch (mycon->sense()->sense())
		{
			case CSense::Less: std::cout << "<" << "\n"; break;
			case CSense::Greater: std::cout << ">" << "\n"; break;
			case CSense::Equal: std::cout << "=" << "\n"; break;
			default: std::cout << "Inequality sense doesn't make any sense \n"; break;
		}
	}
	std::cout << "CutPool Constraints \n";
	for ( i = 0; i < cpool->size(); i++)
	{
		PoolSlot< Constraint, Variable > * sloty = cpool->slot(i);
		Constraint *mycon = sloty->conVar();
		switch (mycon->sense()->sense())
		{
			case CSense::Less: std::cout << "<" << "\n"; break;
			case CSense::Greater: std::cout << ">" << "\n"; break;
			case CSense::Equal: std::cout << "=" << "\n"; break;
			default: std::cout << "Inequality sense doesn't make any sense \n"; break;
		}
	}
	for ( i = 0; i < theSub.nCon(); i++)
	{
		Constraint &theCon = *(theSub.constraint(i));

		for ( i = 0; i < theSub.nVar(); i++)
		{
			double c = theCon.coeff(theSub.variable(i));
			if (c != 0)
				std::cout << c;
			else std::cout << "  ";
		}
		switch (theCon.sense()->sense())
		{
			case CSense::Less: std::cout << "<" << "\n"; break;
			case CSense::Greater: std::cout << ">" << "\n"; break;
			case CSense::Equal: std::cout << "=" << "\n"; break;
			default: std::cout << "doesn't make any sense \n"; break;
		}
		float fl;
		while(!(std::cin >> fl))
		{
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<streamsize>::max(),'\n');
		}
	}
#endif
}

void MaximumCPlanarSubgraph::outputCons(std::ofstream &os,
	StandardPool<Constraint, Variable> *connCon,
	StandardPool<Variable, Constraint> *stdVar)
{
	int i;
	for ( i = 0; i < connCon->number(); i++)
		{
			PoolSlot< Constraint, Variable > * sloty = connCon->slot(i);
			Constraint *mycon = sloty->conVar();
			OGDF_ASSERT(mycon != nullptr);
			int count;
			for (count = 0; count < stdVar->size(); count++)
			{
				PoolSlot< Variable, Constraint > * slotv = stdVar->slot(count);
				Variable *myvar = slotv->conVar();
				double d = mycon->coeff(myvar);
				if (d != 0.0) //precision!
				{
					os <<"+"<< d <<"x"<<count+1;
				}
			}
			switch (mycon->sense()->sense())
			{
				case CSense::Less: os << " <= "; break;
				case CSense::Greater: os << " >= "; break;
				case CSense::Equal: os << " = "; break;
				default:
					os << "Inequality sense doesn't make any sense \n";
					std::cerr << "Inequality sense unknown \n";
					break;
			}
			os << mycon->rhs();
			os << "\n";
		}
}

}
