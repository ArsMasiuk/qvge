/** \file
 * \brief Implementation of the subproblem class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem
 * Contains separation algorithms as well as primal heuristics.
 *
 * \authors Markus Chimani, Mathias Jansen, Karsten Klein
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

#include <ogdf/cluster/internal/MaxCPlanarSub.h>
#include <ogdf/cluster/internal/ClusterKuratowskiConstraint.h>
#include <ogdf/cluster/internal/CutConstraint.h>
#include <ogdf/cluster/internal/ChunkConnection.h>
#include <ogdf/cluster/internal/MaxPlanarEdgesConstraint.h>
#include <ogdf/graphalg/MinimumCut.h>
#include <ogdf/basic/PriorityQueue.h>
#include <ogdf/cluster/CconnectClusterPlanar.h>
#include <ogdf/basic/extended_graph_alg.h>

#include <ogdf/lib/abacus/setbranchrule.h>

#ifdef OGDF_CPLANAR_DEBUG_OUTPUT
#include <ogdf/fileformats/GraphIO.h>
#endif

using namespace ogdf;
using namespace ogdf::cluster_planarity;
using namespace abacus;

MaxCPlanarSub::MaxCPlanarSub(Master *master)
  : Sub(master, 500, static_cast<MaxCPlanarMaster*>(master)->m_inactiveVariables.size(), 2000, false)
  , detectedInfeasibility(false)
  , inOrigSolveLp(false)
  , bufferedForCreation(10) {
	m_constraintsFound = false;
	m_sepFirst = false;
#if 0
	for(int k=0; k<nVar(); ++k) {
		EdgeVar* ev = dynamic_cast<EdgeVar*>(variable(k));
		if(ev->theEdgeType()==EdgeVar::EdgeType::Original)
			fsVarStat(k)->status()->status(FSVarStat::SetToUpperBound);
	}
#endif

	// only output below...
#ifdef OGDF_DEBUG
	Logger::slout() << "Construct 1st Sub\n";
	Logger::slout() << nVar() << " " << nCon() << "\n";
	int i;
	for (i = 0; i < nVar(); ++i) {
		(dynamic_cast<EdgeVar*>(variable(i)))->printMe(Logger::slout()); Logger::slout() << "\n";
	}
	for (i = 0; i < nCon(); ++i) {
		Constraint* c = constraint(i);
		ChunkConnection* ccon = dynamic_cast<ChunkConnection*>(c);
		MaxPlanarEdgesConstraint* cmax = dynamic_cast<MaxPlanarEdgesConstraint*>(c);
		if (ccon) {
			Logger::slout() << "ChunkConstraint: Chunk=";
			for(node v : ccon->m_chunk) {
				Logger::slout() << v << ",";
			}
			Logger::slout() << " Co-Chunk=";
			for(node v : ccon->m_cochunk) {
				Logger::slout() << v << ",";
			}
			Logger::slout() << "\n";
		}
		else if (cmax) {
			Logger::slout() << "MaxPlanarEdgesConstraint: rhs=" << cmax->rhs() << ", graphCons=" << cmax->m_graphCons << ", nodePairs=";
			for (const NodePair &p : cmax->m_edges) {
				Logger::slout() << p;
			}
			Logger::slout() << "\n";
		}
		else {
			Logger::slout() << "** Unexpected Constraint\n";
		}
	}
#endif //OGDF_DEBUG
}


MaxCPlanarSub::MaxCPlanarSub(Master *master,Sub *father,BranchRule *rule,List<Constraint*>& criticalConstraints):
Sub(master,father,rule),detectedInfeasibility(false),inOrigSolveLp(false), bufferedForCreation(10) {
	m_constraintsFound = false;
	m_sepFirst = false;
	criticalSinceBranching.swap(criticalConstraints); // fast load
	Logger::slout() << "Construct Child Sub " << id() << "\n";
}


MaxCPlanarSub::~MaxCPlanarSub() {}


Sub *MaxCPlanarSub::generateSon(BranchRule *rule) {
#if 0
	dualBound_ = realDualBound;
#endif

	const double minViolation = 0.001; // value fixed from abacus...
#ifdef OGDF_CPLANAR_DEBUG_OUTPUT
	if (father() == 0)
	{
		std::ofstream output("Intermediate.txt", std::ios::app);
		if (!output) std::cerr<<"Could not open file for intermediate results!\n";

		output << "Intermediate Results at branching in Root Node\n";
		output << "Number of  graph nodes: " << ((MaxCPlanarMaster*)master_)->getGraph()->numberOfNodes() <<"\n";
		output << "Number of  graph edges: " << ((MaxCPlanarMaster*)master_)->getGraph()->numberOfEdges() <<"\n";

		output << "Current number of active variables: " << nVar() << "\n";
		output << "Current number of active constraints: " << nCon() << "\n";

		output << "Current lower bound: " << lowerBound()<<"\n";
		output << "Current upper bound: " << upperBound()<<"\n";

		output << "Global primal bound: " << ((MaxCPlanarMaster*)master_)->getPrimalBound()<<"\n";
		output << "Global dual bound: " << ((MaxCPlanarMaster*)master_)->getDualBound()<<"\n";

		output << "Added K cons: " << ((MaxCPlanarMaster*)master_)->addedKConstraints()<<"\n";
		output << "Added C cons: " << ((MaxCPlanarMaster*)master_)->addedCConstraints()<<"\n";
		output.close();

		GraphAttributes GA(*(((MaxCPlanarMaster*)master_)->getGraph()), GraphAttributes::edgeStyle |
			GraphAttributes::edgeDoubleWeight | GraphAttributes::edgeStyle |
			GraphAttributes::edgeGraphics);

		for (int i = 0; i < nVar(); ++i)
		{
			EdgeVar *e = (EdgeVar*)variable(i);
			if (e->theEdgeType() == EdgeVar::EdgeType::Original)
			{
				GA.doubleWeight(e->theEdge()) = xVal(i);
				GA.strokeWidth(e->theEdge()) = 10.0*xVal(i);
				if (xVal(i) == 1.0) GA.strokeColor(e->theEdge()) = "#FF0000";
			}
		}

		std::ofstream out("WeightedIntermediateGraph.gml");
		GraphIO::writeGML(GA, out);
	}
#endif

	List< Constraint* > criticalConstraints;
	if (master()->pricing())
	{
#if 0
		SetBranchRule* srule = (SetBranchRule*)(rule);
#endif
		SetBranchRule* srule;
		srule = dynamic_cast<SetBranchRule*>(rule);
		OGDF_ASSERT( srule ); // hopefully no other branching stuff...
		//Branching by setting a variable to 0 may
		//result in infeasibility of the current system
		//because connectivity constraints may not be feasible
		//with the current set of variables
		if(!srule->setToUpperBound()) { // 0-branching
			int varidx = srule->variable();
			EdgeVar* var = static_cast<EdgeVar*>(variable(varidx));

			Logger::slout() << "FIXING VAR: ";
			var->printMe(Logger::slout());
			Logger::slout() << "\n";

			for(int i = nCon(); i-- > 0;) {
				Constraint* con = constraint(i);
				double coeff = con->coeff(var);
				if(con->sense()->sense()==CSense::Greater && coeff>0.99) {
					// check: yVal gives the slack and is always negative or 0
					double slk;
#if 0
					slk = yVal(i);
#endif
					slk = con->slack(actVar(),xVal_);
					//quick hack using ABACUS value, needs to be corrected
					if (slk > 0.0 && slk < minViolation)
					{
						slk = 0.0;
#ifdef OGDF_DEBUG
						std::cout << "Set slack to 0.0\n";
#endif
					}
					if(slk > 0.0) {
						Logger::slout() << "ohoh..." << slk << " "; var->printMe(Logger::slout()); Logger::slout()<<std::flush;
					}
					OGDF_ASSERT(slk <= 0.0);
					double zeroSlack = slk+xVal(varidx)*coeff;
					if(zeroSlack > 0.0001) { // setting might introduce infeasibility
#if 0
						// TODO: is the code below valid (in terms of theory) ???
						// "es reicht wenn noch irgendeine nicht-auf-0-fixierte variable im constraint existiert, die das rettet
						// mögliches problem: was wenn alle kanten bis auf die aktive kante in einem kuratowski constraint
						// auf 1 fixiert sind (zB grosse teile wegen planaritätstest-modus, und ein paar andere wg. branching).
						bool good = false; // does there exist another good variable?
						for(int j = nVar(); j-- > 0;) {
							if(con->coeff(variable[j])>0.99 && VARIABLE[j]-NOT-FIXED-TO-0) {
								good = true;
								break;
							}
						}
						if(!good)
#endif
						criticalConstraints.pushBack(con);
					}
				}
			}
		}
	}

	return new MaxCPlanarSub(master_, this, rule, criticalConstraints);
}


int MaxCPlanarSub::selectBranchingVariable(int &variable) {
#if 0
	dualBound_ = realDualBound;
#endif

	return Sub::selectBranchingVariable(variable);

	/// the stuff below does NOTHING!

	int variableABA;
	int found = Sub::selectBranchingVariable(variableABA);
	if (found == 0) {
#if 0
		Edge *e = (Edge*)(this->variable(variableABA));
		std::cout << "Branching variable is: " << (e->theEdgeType()==Original ? "Original" : "Connect") << " Edge (";
		std::cout << e->sourceNode()->index() << "," << e->targetNode()->index() << ") having value: ";
		std::cout << xVal(variableABA) << " and coefficient " << ((Edge*)this->variable(variableABA))->objCoeff() << std::endl;
#endif
		variable = variableABA;
		return 0;
	}
	return 1;
}


int MaxCPlanarSub::selectBranchingVariableCandidates(ArrayBuffer<int> &candidates) {
#if 0
	if(master()->m_checkCPlanar)
		return Sub::selectBranchingVariableCandidates(candidates);
#endif

	ArrayBuffer<int> candidatesABA(1,false);
	int found = Sub::selectBranchingVariableCandidates(candidatesABA);

	if (found == 1) return 1;
	else {
		int i = candidatesABA.popRet();
		EdgeVar *e = static_cast<EdgeVar*>(variable(i));
		if (e->theEdgeType() == EdgeVar::EdgeType::Original) {
			OGDF_ASSERT(!master()->m_checkCPlanar);
			candidates.push(i);
			return 0;
		} else {

			// Checking if a more appropriate o-edge can be found according to the global parameter
			// \a branchingOEdgeSelectGap. Candidates are stored in list \a oEdgeCandits.
			List<int> oEdgeCandits;
			for (int j=0; j<nVar(); ++j) {
				EdgeVar *eVar = static_cast<EdgeVar*>(variable(j));
				if (eVar->theEdgeType() == EdgeVar::EdgeType::Original
				 && xVal(j) >= (0.5 - static_cast<MaxCPlanarMaster*>(master_)->branchingOEdgeSelectGap())
				 && xVal(j) <= (0.5 + static_cast<MaxCPlanarMaster*>(master_)->branchingOEdgeSelectGap())) {
					oEdgeCandits.pushBack(j);
				}
			}
			if (oEdgeCandits.empty()) {
				candidates.push(i);
				return 0;
			} else {

				// Choose one of those edges randomly.
				int random = randomNumber(0,oEdgeCandits.size()-1);
				int index = random;
				ListConstIterator<int> it = oEdgeCandits.get(index);
				candidates.push(*it);
				return 0;
			}
		}
	}
}


void MaxCPlanarSub::updateSolution() {

	List<NodePair> originalOneEdges;
	List<NodePair> connectionOneEdges;
	List<edge> deletedEdges;
	NodePair np;
#if 0
	for (int i=0; i<((MaxCPlanarMaster*)master_)->nVars(); ++i) {
#else
	for (int i=0; i<nVar(); ++i) {
#endif
		if (xVal(i) >= 1.0-(master_->eps())) {

			EdgeVar *e = static_cast<EdgeVar*>(variable(i));
			np.source = e->sourceNode();
			np.target = e->targetNode();
			if (e->theEdgeType() == EdgeVar::EdgeType::Original) originalOneEdges.pushBack(np);
			else connectionOneEdges.pushBack(np);
		}
		else {

			EdgeVar *e = static_cast<EdgeVar*>(variable(i));
			if (e->theEdgeType() == EdgeVar::EdgeType::Original) {
				deletedEdges.pushBack(e->theEdge());
			}
		}
	}
#ifdef OGDF_DEBUG
	static_cast<MaxCPlanarMaster*>(master_)->m_solByHeuristic = false;
#endif
	static_cast<MaxCPlanarMaster*>(master_)->updateBestSubGraph(originalOneEdges, connectionOneEdges, deletedEdges);
}


double MaxCPlanarSub::subdivisionLefthandSide(SListConstIterator<KuratowskiWrapper> kw, GraphCopy *gc) {

	double lefthandSide = 0.0;
	for (int i = 0; i < nVar(); ++i) {
		EdgeVar *e = static_cast<EdgeVar*>(variable(i));
		node v = e->sourceNode();
		node w = e->targetNode();
		for (edge ei : (*kw).edgeList) {
			if ((ei->source() == gc->copy(v) && ei->target() == gc->copy(w)) ||
				(ei->source() == gc->copy(w) && ei->target() == gc->copy(v))) {
				lefthandSide += xVal(i);
			}
		}
	}
	return lefthandSide;
}


int MaxCPlanarSub::getArrayIndex(double lpValue) {
	int index = 0;
	double x = 1.0;
	double listRange = (1.0 / static_cast<MaxCPlanarMaster*>(master_)->numberOfHeuristicPermutationLists());
	while (x >= lpValue) {
		x -= listRange;
		if (lpValue >= x) return index;
		index++;
	}
	return index;
}


void MaxCPlanarSub::childClusterSpanningTree(
	GraphCopy &GC,
	List<edgeValue> &clusterEdges,
	List<NodePair> &MSTEdges)
{
	// Testing and adding of edges in \p clusterEdges is performed randomized.
	// Dividing edges into original and connection 1-edges and fractional edges.
	// Tree is built using edges in this order.
	List<edgeValue> oneOEdges;
	List<edgeValue> oneToFracBoundOEdges;
	List<edgeValue> leftoverEdges;

	for (const edgeValue &ev : clusterEdges)
	{
		if (ev.lpValue >= (1.0 - master_->eps())) {
			if (ev.original) oneOEdges.pushBack(ev);
			else leftoverEdges.pushBack(ev);
		}
		else if (ev.lpValue >= static_cast<MaxCPlanarMaster*>(master_)->getHeuristicFractionalBound()) {
			if (ev.original)
				oneToFracBoundOEdges.pushBack(ev);
			else
				leftoverEdges.pushBack(ev);
		}
		else
			leftoverEdges.pushBack(ev);
	}

	// Try to create spanning tree with original 1-edges.
	if(oneOEdges.size() > 1) oneOEdges.permute();
	edge newEdge;
	node v,w;
	NodePair np;
	for (const edgeValue &ev : oneOEdges) {
		v = ev.src;
		w = ev.trg;
		newEdge = GC.newEdge(GC.copy(v),GC.copy(w));
		//Union-Find could make this faster...
		if (!isAcyclicUndirected(GC))
			GC.delEdge(newEdge);
		else {
			np.source = v; np.target = w;
			MSTEdges.pushBack(np);
		}
		if (GC.numberOfEdges() == GC.numberOfNodes()-1)
			return;
	}
	//is there a case this if would return without having returned above?
	if (isConnected(GC)) return;

	// Create two Arrays of lists containing nodePairs that have "similar" fractional value.
	// "Similar" is defined by the parameter \a Master->nPermutationLists.
	double listRange = (1.0 / static_cast<MaxCPlanarMaster*>(master_)->numberOfHeuristicPermutationLists());
	double range = 0.0;
	int indexCount = 0;
	while ((1.0 - static_cast<MaxCPlanarMaster*>(master_)->getHeuristicFractionalBound()) > range) {
		indexCount++;
		range += listRange;
	}
	Array<List<edgeValue> > oEdgePermLists(0,indexCount);
	Array<List<edgeValue> > leftoverPermLists(0, static_cast<MaxCPlanarMaster*>(master_)->numberOfHeuristicPermutationLists());

	// Distributing edges in \a oneToFracBoundOEdges and \p leftoverEdges among the permutation lists.
	int index;
	for (const edgeValue &ev : oneToFracBoundOEdges) {
		index = getArrayIndex(ev.lpValue);
		oEdgePermLists[index].pushBack(ev);
	}
	for (const edgeValue &ev : leftoverEdges) {
		index = getArrayIndex(ev.lpValue);
		leftoverPermLists[index].pushBack(ev);
	}


	for (auto &oEdgePermList : oEdgePermLists) {
		if (oEdgePermList.size() > 1) oEdgePermList.permute();

		for (const edgeValue &ev : oEdgePermList) {
			v = ev.src;
			w = ev.trg;
			newEdge = GC.newEdge(GC.copy(v),GC.copy(w));
			if (!isAcyclicUndirected(GC)) {
				GC.delEdge(newEdge);
			}
			else {
				np.source = v; np.target = w;
				MSTEdges.pushBack(np);
			}
			if (GC.numberOfEdges() == GC.numberOfNodes()-1) return;
		}

		if (isConnected(GC)) return;
	}

	for (auto &leftoverPermList : leftoverPermLists) {
		if (leftoverPermList.size() > 1) leftoverPermList.permute();

		for (const edgeValue &ev : leftoverPermList) {
			v = ev.src;
			w = ev.trg;
			newEdge = GC.newEdge(GC.copy(v),GC.copy(w));
			if (!isAcyclicUndirected(GC)) {
				GC.delEdge(newEdge);
			}
			else {
				np.source = v; np.target = w;
				MSTEdges.pushBack(np);
			}
			if (GC.numberOfEdges() == GC.numberOfNodes()-1) return;
		}

		if (isConnected(GC)) return;
	}

	//Todo: What is happening here? Do we have to abort?
	if (!isConnected(GC)) std::cerr << "Error. For some reason no spanning tree could be computed" << std::endl << std::flush;
	return;
}


void MaxCPlanarSub::clusterSpanningTree(
	ClusterGraph &C,
	cluster c,
	ClusterArray<List<NodePair> > &treeEdges,
	ClusterArray<List<edgeValue> > &clusterEdges)
{
	//looks like a minspantree problem with some weights based
	//on edge status and LP-value, right?
	GraphCopy *GC;
	if (c->cCount() == 0) { // Cluster is a leaf, so MST can be computed.

		// Create a cluster induced GraphCopy of \a G and delete all edges.
		GC = new GraphCopy(C.constGraph());
		node v = GC->firstNode();
		node v_succ;
		while (v!=nullptr) {
			v_succ = v->succ();
			if (C.clusterOf(GC->original(v)) != c) {
				GC->delNode(v);
			}
			v = v_succ;
		}
		edge e = GC->firstEdge();
		edge e_succ;
		while (e!=nullptr) {
			e_succ = e->succ();
			GC->delEdge(e);
			e = e_succ;
		}
		childClusterSpanningTree(*GC,clusterEdges[c],treeEdges[c]);
		delete GC;
		return;
	}

	// If cluster \p c has further children, they have to be processed first.
	for (cluster ci : c->children) {

		clusterSpanningTree(C,ci,treeEdges,clusterEdges);

		// Computed treeEdges for the child clusters have to be added to \p treeEdges for current cluster.
		for (const NodePair &np : treeEdges[ci]) {
			treeEdges[c].pushBack(np);
		}
	}

	// A spanning tree has been computed for all children of cluster \p c.
	// Thus, a spanning tree for \p c can now be computed.
	// \p treeEdges[\p c] now contains all edges that have been previously added
	// during the computation of the trees for its child clusters.

	// Create GraphCopy induced by nodes in \a nodes.
	GC = new GraphCopy(C.constGraph());
	NodeArray<bool> isInCluster(*GC, false);
	List<node> clusterNodes;
	c->getClusterNodes(clusterNodes);
	for (node u : clusterNodes) {
		isInCluster[GC->copy(u)] = true;
	}
	node v = GC->firstNode();
	node v_succ;
	while (v != nullptr) {
		v_succ = v->succ();
		if (!isInCluster[v]) {
			GC->delNode(v);
		}
		v = v_succ;
	}
	edge e = GC->firstEdge();
	edge e_succ;
	while (e!=nullptr) {
		e_succ = e->succ();
		GC->delEdge(e);
		e = e_succ;
	}
	// Edges that have been added in child clusters by computing a spanning tree
	// have to be added to the GraphCopy.
	for (const NodePair &np : treeEdges[c]) {
		node cv = GC->copy(np.source);
		node cw = GC->copy(np.target);
		GC->newEdge(cv,cw);
	}

	// Compute relevant nodePairs, i.e. all nodePairs induced by cluster c
	// leaving out already added ones.
	List<edgeValue> clusterNodePairs;
	for (const edgeValue &ev : clusterEdges[c]) {
		node cv = GC->copy(ev.src);
		node cw = GC->copy(ev.trg);
		//TODO in liste speichern ob kante vorhanden dann kein searchedge
		if (!GC->searchEdge(cv,cw))
			clusterNodePairs.pushBack(ev);
	}

	childClusterSpanningTree(*GC,clusterNodePairs,treeEdges[c]);
	delete GC;
	return;
}


double MaxCPlanarSub::heuristicImprovePrimalBound(
	List<NodePair> &origEdges,
	List<NodePair> &conEdges,
	List<edge> &delEdges)
{

	origEdges.clear(); conEdges.clear(); delEdges.clear();

	double oEdgeObjValue = 0.0;
#if 0
	double cEdgeObjValue = 0.0;
#endif
	int originalEdgeCounter = 0;

	// A copy of the Clustergraph has to be created.
	// To be able to have access to the original nodes after the heuristic has been performed,
	// we maintain the Arrays \a originalClusterTable and \a originalNodeTable.
	Graph G;
	ClusterArray<cluster> originalClusterTable(*(static_cast<MaxCPlanarMaster*>(master_)->getClusterGraph()));
	NodeArray<node> originalNodeTable(*(static_cast<MaxCPlanarMaster*>(master_)->getGraph()));
	ClusterGraph CC(*(static_cast<MaxCPlanarMaster*>(master_)->getClusterGraph()), G, originalClusterTable, originalNodeTable);

	//NodeArray \a reverseNodeTable is indexized by \p G and contains the corresponding original nodes
	NodeArray<node> reverseNodeTable(G);
	for (node v : (static_cast<MaxCPlanarMaster*>(master_)->getGraph())->nodes) {
		node w = originalNodeTable[v];
		reverseNodeTable[w] = v;
	}

	// First, nodePairs have to be sorted in increasing order of their LP-value.
	// Therefore a Binary Heap is built and read once to obtain a sorted list of the nodePairs.
	List<edgeValue> globalNodePairList;
	PrioritizedQueue<edgeValue, double> BH_all;
	edgeValue ev;
	double lpValue;
	for (int i = 0; i < nVar(); ++i) {
		node ov = static_cast<EdgeVar*>(variable(i))->sourceNode();
		node ow = static_cast<EdgeVar*>(variable(i))->targetNode();
		ev.src = originalNodeTable[ov]; //the node copies in G
		ev.trg = originalNodeTable[ow];
		ev.e = static_cast<EdgeVar*>(variable(i))->theEdge(); //the original edge
		lpValue = 1.0 - xVal(i);
		ev.lpValue = xVal(i);
		ev.original = static_cast<EdgeVar*>(variable(i))->theEdgeType() == EdgeVar::EdgeType::Original;
		BH_all.push(ev, lpValue);
	}

	// ClusterArray \a clusterEdges contains for each cluster all corresponding edgeValues
	// in increasing order of their LP-values.
	ClusterArray<List<edgeValue> > clusterEdges(CC);
	for (int i=0; i<nVar(); ++i) {
		ev = BH_all.topElement();
		BH_all.pop();
		cluster lca = CC.commonCluster(ev.src,ev.trg);
		clusterEdges[lca].pushBack(ev);
		globalNodePairList.pushBack(ev);
	}

	// For each cluster \a spanningTreeNodePairs contains the computed nodePairs, edgeValues respectivly.
	ClusterArray<List<NodePair> > spanningTreesNodePairs(CC);
	clusterSpanningTree(
		CC,
		originalClusterTable[(static_cast<MaxCPlanarMaster*>(master_)->getClusterGraph())->rootCluster()],
		spanningTreesNodePairs,
		clusterEdges);
	// \a spanningTreesNodePairs[CC->rootCluster] now contains the edges of the computed tree.

	// Create the induced ClusterGraph.
	List<edge> delAll;
	G.allEdges(delAll);
	for (edge e: delAll) {
		G.delEdge(e);
	}
	for (const NodePair &np : spanningTreesNodePairs[CC.rootCluster()]) {
		G.newEdge(np.source,np.target);
	}

	// Creating two lists \a cEdgeNodePairs and \a oEdgeNodePairs in increasing order of LP-values.
	int nOEdges = 0;
	List<edgeValue> oEdgeNodePairs;
	PrioritizedQueue<edgeValue, double> BH_oEdges;
	node cv,cw;
	NodePair np;
	for (int i=0; i<nVar(); ++i) {

		cv = originalNodeTable[static_cast<EdgeVar*>(variable(i))->sourceNode()];
		cw = originalNodeTable[static_cast<EdgeVar*>(variable(i))->targetNode()];
		//todo searchedge?
		if (!G.searchEdge(cv,cw)) {
			ev.src = cv; ev.trg = cw;
			lpValue = 1.0-xVal(i);
			ev.lpValue = xVal(i);
			if (static_cast<EdgeVar*>(variable(i))->theEdgeType() == EdgeVar::EdgeType::Original) {
				ev.e = static_cast<EdgeVar*>(variable(i))->theEdge(); //the original edge
				BH_oEdges.push(ev,lpValue);
				nOEdges++;
			}
		} else { // Edge is contained in G.
			np.source = static_cast<EdgeVar*>(variable(i))->sourceNode();
			np.target = static_cast<EdgeVar*>(variable(i))->targetNode();

			if (static_cast<EdgeVar*>(variable(i))->theEdgeType() == EdgeVar::EdgeType::Original) {
				originalEdgeCounter++;
				oEdgeObjValue += variable(i)->obj();
				// Since edges that have been added are not deleted in further steps,
				// list \p origEdges may be updated in this step.
				origEdges.pushBack(np);

			} else {
				// Since edges that have been added are not deleted in further steps,
				// list \p conEdges may be updated in this step.
				conEdges.pushBack(np);
			}
		}
	}

	for (int i=1; i<=nOEdges; ++i) {
		oEdgeNodePairs.pushBack(BH_oEdges.topElement());
		BH_oEdges.pop();
	}

	//INSERTING LEFTOVER NODEPAIRS IN INCREASING ORDER OF LP_VALUE AND CHECKING FOR C-PLANARITY

	List<edgeValue> oneOEdges;
	List<edgeValue> fracEdges;
	for(const edgeValue &eVal : oEdgeNodePairs) {
		if (eVal.lpValue >= (1.0-master_->eps())) {
			oneOEdges.pushBack(eVal);
		} else {
			fracEdges.pushBack(eVal);
		}
	}

	const EdgeArray<double> *pCost = static_cast<MaxCPlanarMaster*>(master_)->m_pCost;

	// Randomly permute the edges in \a oneOEdges.
	oneOEdges.permute();
	edge addEdge;
	bool cPlanar;
	CconnectClusterPlanar cccp;
	for (const edgeValue &eVal : oneOEdges)
	{
		addEdge = G.newEdge(eVal.src,eVal.trg);
		cPlanar = cccp.call(CC);
		if (!cPlanar) {
			G.delEdge(addEdge);
			// Since edges that have been added are not deleted in further steps,
			// list \p origEdges may be updated in this step.
			np.source = reverseNodeTable[eVal.src];
			np.target = reverseNodeTable[eVal.trg];
			//Hier aus oneOEdges
			delEdges.pushBack(eVal.e);
		} else {
			originalEdgeCounter++;
			oEdgeObjValue += (eVal.original && pCost != nullptr) ? (*pCost)[eVal.e] : 1;
			// Since edges that have been added are not deleted in further steps,
			// list \p origEdges may be updated in this step.
			np.source = reverseNodeTable[eVal.src];
			np.target = reverseNodeTable[eVal.trg];
			origEdges.pushBack(np);
		}
	}


	Array<List<edgeValue> > leftoverPermLists(0, static_cast<MaxCPlanarMaster*>(master_)->numberOfHeuristicPermutationLists());

	// Distributing edges in \a fracEdges among the permutation lists.
	for (const edgeValue &eVal : fracEdges) {
		int index = getArrayIndex(eVal.lpValue);
		leftoverPermLists[index].pushBack(eVal);
	}

	for (auto &leftoverPermList : leftoverPermLists) {
		// Testing of fractional values is also performed randomized.
		leftoverPermList.permute();

		for (const edgeValue &eVal : leftoverPermList) {
			node u = eVal.src;
			node v = eVal.trg;
			addEdge = G.newEdge(u,v);
			cPlanar = cccp.call(CC);
			if (!cPlanar) {
				G.delEdge(addEdge);
				// Since edges that have been added are not deleted in further steps,
				// list \p origEdges may be updated in this step.
				np.source = reverseNodeTable[eVal.src];
				np.target = reverseNodeTable[eVal.trg];
				//Hier aus fracedges
				delEdges.pushBack(eVal.e);
			} else {
				originalEdgeCounter++;
				oEdgeObjValue += (eVal.original && pCost != nullptr) ? (*pCost)[eVal.e] : 1;
				// Since edges that have been added are not deleted in further steps,
				// list \p origEdges may be updated in this step.
				np.source = reverseNodeTable[eVal.src];
				np.target = reverseNodeTable[eVal.trg];
				origEdges.pushBack(np);
			}
		}
	}

	// If the Graph created so far contains all original edges, the Graph is c-planar.
	// Todo: And we are finished...
	if (originalEdgeCounter == static_cast<MaxCPlanarMaster*>(master_)->getGraph()->numberOfEdges()) {

#ifdef OGDF_DEBUG
		static_cast<MaxCPlanarMaster*>(master_)->m_solByHeuristic = true;
#endif

		static_cast<MaxCPlanarMaster*>(master_)->updateBestSubGraph(origEdges, conEdges, delEdges);

		master_->primalBound(oEdgeObjValue+0.79);
	}

	return oEdgeObjValue + 0.79;
}


#if 0
// OLD HEURISTIC
void MaxCPlanarSub::minimumSpanningTree(
	GraphCopy &GC,
	List<NodePair> &clusterEdges,
	List<NodePair> &MSTEdges)
{

	//list \a clusterEdges contains nodePairs of \a G in increasing order of LP-value
	//\a GC is a GraphCopy of G and contains previously added edges of the child clusters.

	NodePair np;
	node cv,cw;
	edge newEdge;
	ListConstIterator<NodePair> it;
	//function valid() returns true, even if the list clusterEdges is empty. WHY???
	//maybe because the loop doesn't "recognise" that elements are removed from list
	//\clusterEdges, but works on the original list the whole time
	for (it=clusterEdges.begin(); it.valid(); ++it) {
		if (clusterEdges.size() == 0) break;

		np = clusterEdges.front();
		cv = GC.copy(np.v1);
		cw = GC.copy(np.v2);
		newEdge = GC.newEdge(cv,cw);

		if (!isAcyclicUndirected(GC)) {
			GC.delEdge(newEdge);
		} else {
			MSTEdges.pushBack(np);
		}
		clusterEdges.popFront();

		//if the number of edges of \a GC is one less than the number of nodes,
		//the search can be stopped, because a tree has been computed and no further edges can be added
		if (GC.numberOfEdges() == GC.numberOfNodes()-1) break;
	}
}


void MaxCPlanarSub::recursiveMinimumSpanningTree(
	ClusterGraph &C,
	cluster c,
	ClusterArray<List<NodePair> > &treeEdges,
	List<NodePair> &edgesByIncLPValue,
	List<node> &clusterNodes)
{

	//node forwarding
	//nodes corresponding to cluster \a c are added to given list \a clusterNodes
	//necessary, to have quick access to the relevant nodes for building up the GraphCopy for the cluster
	for (node v : c->nodes) {
		clusterNodes.pushBack(v);
	}

	GraphCopy *cG;
	if (c->cCount() == 0) { //cluster is a leaf, so MST can be computed

		//Create a cluster induced GraphCopy of \a G and delete all edges
		cG = new GraphCopy(C.getGraph());
		node v = cG->firstNode();
		node v_succ;
		while (v!=0) {
			v_succ = v->succ();
			if (C.clusterOf(cG->original(v)) != c) {
				cG->delNode(v);
			}
			v = v_succ;
		}
		edge e = cG->firstEdge();
		edge e_succ;
		while (e!=0) {
			e_succ = e->succ();
			cG->delEdge(e);
			e = e_succ;
		}

		//Determining the relevant nodePairs of the cluster-induced GraphCopy
		//in increasing order of LP-value
		//performance should be improved, maybe by using a more sophisticated data structure
		List<NodePair> clusterNodePairs;
		ListConstIterator<NodePair> it;
		for (it=edgesByIncLPValue.begin(); it.valid(); ++it) {
			if (C.clusterOf((*it).v1) == c && C.clusterOf((*it).v2) == c) {
				clusterNodePairs.pushBack(*it);
			}
		}

		minimumSpanningTree(*cG,clusterNodePairs,treeEdges[c]);
		delete cG;
		return;
	}

	//If cluster \a c has further children, they have to be processed first
	List<node> nodes;
	for (auto child : c->children) {
		recursiveMinimumSpanningTree(C, child, treeEdges, edgesByIncLPValue, nodes);

		//computed treeEdges for the child clusters have to be added to treeEdges for current cluster
		for (auto treeEdge : treeEdges[child].begin()) {
			treeEdges[c].pushBack(treeEdge);
		}
	}
	//The MSTs of all children of cluster \a c have been computed
	//So MST for \a c can now be computed

	//updating node lists
	for (auto v : nodes) {
		clusterNodes.pushBack(v);
	}
	for (auto v : c->nodes) {
		nodes.pushBack(v);
	}
	//now list \a nodes contains all nodes belonging to cluster \a c

	//create GraphCopy induced by nodes in \a nodes
	cG = new GraphCopy(C.getGraph());
	NodeArray<bool> isInCluster(*cG,false);
	for (node v : nodes) {
		isInCluster[cG->copy(v)] = true;
	}
	node v = cG->firstNode();
	node v_succ;
	while (v != nullptr) {
		v_succ = v->succ();
		if (!isInCluster[v]) {
			cG->delNode(v);
		}
		v = v_succ;
	}
	edge e = cG->firstEdge();
	edge e_succ;
	while (e != nullptr) {
		e_succ = e->succ();
		cG->delEdge(e);
		e = e_succ;
	}
	//edges that have been added in child clusters by computing an MST have to be added to the Graphcopy
	ListConstIterator<NodePair> it2;
	node cGv,cGw;
	for (it2=treeEdges[c].begin(); it2.valid(); ++it2) {
		cGv = cG->copy((*it2).v1);
		cGw = cG->copy((*it2).v2);
		cG->newEdge(cGv,cGw);
	}

	//compute relevant nodePairs, i.e. all nodePairs induced by cluster leaving out already added ones
	List<NodePair> clusterNodePairs;
	ListConstIterator<NodePair> it3;
	for (it3=edgesByIncLPValue.begin(); it3.valid(); ++it3) {
		cGv = cG->copy((*it3).v1);
		cGw = cG->copy((*it3).v2);
		//todo remove searchedge
		if (isInCluster[cGv] && isInCluster[cGw] && !cG->searchEdge(cGv,cGw)) clusterNodePairs.pushBack(*it3);
	}

	minimumSpanningTree(*cG,clusterNodePairs,treeEdges[c]);
	delete cG;
	return;
}


double MaxCPlanarSub::heuristicImprovePrimalBoundDet(
	List<NodePair> &origEdges,
	List<NodePair> &conEdges,
	List<NodePair> &delEdges)
{

	origEdges.clear(); conEdges.clear(); delEdges.clear();

	//the primal value of the heuristically computed ILP-solution
	double oEdgeObjValue = 0.0;
	double cEdgeObjValue = 0.0;
	int originalEdgeCounter = 0;

	//a copy of the Clustergraph has to be created.
	//to be able to have access to the original nodes after the heuristic has been performed,
	//we maintain the Arrays \a originalClusterTable and \a originalNodeTable
	Graph G;
	ClusterArray<cluster> originalClusterTable(*(((MaxCPlanarMaster*)master_)->getClusterGraph()));
	NodeArray<node> originalNodeTable(*(((MaxCPlanarMaster*)master_)->getGraph()));
	ClusterGraph CC( *(((MaxCPlanarMaster*)master_)->getClusterGraph()),G,originalClusterTable,originalNodeTable );

	//NodeArray \a reverseNodeTable is indexized by \a G and contains the corresponding original nodes
	NodeArray<node> reverseNodeTable(G);
	for(node v : (((MaxCPlanarMaster*)master_)->getGraph())->nodes) {
		node w = originalNodeTable[v];
		reverseNodeTable[w] = v;
	}

	//first, nodePairs have to be sorted in increasing order of their LP-value
	//therefore a Binary Heap is build and read once, to obtain a sorted list of the nodePairs
	List<NodePair> globalNodePairList;
	BinaryHeap2<double,NodePair> BH_all(nVar());
	NodePair np;
	node ov,ow;
	double lpValue;
	for (int i=0; i<nVar(); ++i) {
		ov = ((Edge*)variable(i))->sourceNode();
		ow = ((Edge*)variable(i))->targetNode();
		np.v1 = originalNodeTable[ov];
		np.v2 = originalNodeTable[ow];
		lpValue = 1.0-xVal(i);
		BH_all.insert(np,lpValue);
	}
	for (int i=0; i<nVar(); ++i) {
		np = BH_all.extractMin();
		ov = reverseNodeTable[np.v1]; ow = reverseNodeTable[np.v2];
		globalNodePairList.pushBack(np);
	}

	//for each cluster \a spanningTreeNodePairs contains the MST nodePairs (of original Graph)
	ClusterArray<List<NodePair> > spanningTreesNodePairs(CC);
	List<node> nodes;
	recursiveMinimumSpanningTree(
		CC,
		originalClusterTable[(((MaxCPlanarMaster*)master_)->getClusterGraph())->rootCluster()],
		spanningTreesNodePairs,
		globalNodePairList,
		nodes);
	//\a spanningTreesNodePairs[CC->rootCluster] now contains the edges of the computed Tree

	//create the induced ClusterGraph
	edge e = G.firstEdge();
	edge e_succ;
	while (e!=0) {
		e_succ = e->succ();
		G.delEdge(e);
		e = e_succ;
	}
	ListConstIterator<NodePair> it2;
	for (it2=spanningTreesNodePairs[CC.rootCluster()].begin(); it2.valid(); ++it2) {
		G.newEdge((*it2).v1,(*it2).v2);
	}

	//creating two lists \a cEdgeNodePairs and \a oEdgeNodePairs in increasing order of LP-values.
	int nOEdges = 0;
	int nCEdges = 0;
	List<NodePair> cEdgeNodePairs;
	List<NodePair> oEdgeNodePairs;
	BinaryHeap2<double,NodePair> BH_cEdges(nVar());
	BinaryHeap2<double,NodePair> BH_oEdges(nVar());
	node cv,cw;
	for (int i=0; i<nVar(); ++i) {

		cv = originalNodeTable[ ((Edge*)variable(i))->sourceNode() ];
		cw = originalNodeTable[ ((Edge*)variable(i))->targetNode() ];
		if ((G.searchEdge(cv,cw))==nullptr) {
			np.v1 = cv; np.v2 = cw;
			lpValue = 1.0-xVal(i);
			if ( ((Edge*)variable(i))->theEdgeType() == EdgeType::Original ) {
				BH_oEdges.insert(np,lpValue);
				nOEdges++;
			} else {
				BH_cEdges.insert(np,lpValue);
				nCEdges++;
			}
		} else { //edge is contained in G
			np.v1 = ((Edge*)variable(i))->sourceNode();
			np.v2 = ((Edge*)variable(i))->targetNode();

			if ( ((Edge*)variable(i))->theEdgeType() == EdgeType::Original ) {
				originalEdgeCounter++;
				oEdgeObjValue += 1.0;
				//since edges that have been added are not deleted in further steps,
				//list \a origEdges may be updated in this step.
				origEdges.pushBack(np);

			} else {
#if 0
				cEdgeObjValue -= ((MaxCPlanarMaster*)master_)->epsilon();
#endif
				//since edges that have been added are not deleted in further steps,
				//list \a conEdges may be updated in this step.
				conEdges.pushBack(np);
			}
		}
	}

	for (int i=1; i<=nCEdges; ++i) {
		cEdgeNodePairs.pushBack(BH_cEdges.extractMin());
	}
	for (int i=1; i<=nOEdges; ++i) {
		oEdgeNodePairs.pushBack(BH_oEdges.extractMin());
	}


	//INSERTING LEFTOVER NODEPAIRS IN INCREASING ORDER OF LP_VALUE AND CHECKING FOR C-PLANARITY
	//first, O-Edges are testet and added.

	ListConstIterator<NodePair> it3;
	edge addEdge;
	bool cPlanar;
	CconnectClusterPlanar cccp;
	for (it3=oEdgeNodePairs.begin(); it3.valid(); ++it3) {

		addEdge = G.newEdge((*it3).v1,(*it3).v2);
		cPlanar = cccp.call(CC);
		if (!cPlanar) {
			G.delEdge(addEdge);
			//since edges that have been added are not deleted in further steps,
			//list \a origEdges may be updated in this step.
			np.v1 = reverseNodeTable[(*it3).v1];
			np.v2 = reverseNodeTable[(*it3).v2];
			delEdges.pushBack(np);
		} else {
			originalEdgeCounter++;
			oEdgeObjValue += 1.0;
			//since edges that have been added are not deleted in further steps,
			//list \a origEdges may be updated in this step.
			np.v1 = reverseNodeTable[(*it3).v1];
			np.v2 = reverseNodeTable[(*it3).v2];
			origEdges.pushBack(np);
		}
	}

	//if the Graph created so far contains all original edges, the Graph is c-planar.
	if (originalEdgeCounter == ((MaxCPlanarMaster*)master_)->getGraph()->numberOfEdges()) {
#if 0
		std::cout << "Graph is c-planar! Heuristic has computed a solution that contains all original edges" << std::endl;
#endif
		((MaxCPlanarMaster*)master_)->updateBestSubGraph(origEdges,conEdges,delEdges);
#if 0
		std::cout << "value of solution is: " << oEdgeObjValue << std::endl;
#endif
		master_->primalBound(oEdgeObjValue+0.79);
	}


	std::cout << "the objective function value of heuristically computed ILP-solution is: " << (oEdgeObjValue + cEdgeObjValue) << std::endl;

	return (oEdgeObjValue+0.79);
}
#endif


int MaxCPlanarSub::improve(double &primalValue) {
	if (static_cast<MaxCPlanarMaster*>(master_)->getHeuristicLevel() == 0) return 0;

	// If \a heuristicLevel is set to value 1, the heuristic is only run,
	// if current solution is fractional and no further constraints have been found.
	if (static_cast<MaxCPlanarMaster*>(master_)->getHeuristicLevel() == 1) {
		if (!integerFeasible() && !m_constraintsFound) {

			List<NodePair> origEdges;
			List<NodePair> conEdges;
			List<edge> delEdges;

			for (int i = static_cast<MaxCPlanarMaster*>(master_)->getHeuristicRuns(); i>0; i--) {

				origEdges.clear(); conEdges.clear(); delEdges.clear();
				int heuristic = (int)heuristicImprovePrimalBound(origEdges,conEdges,delEdges);

				// \a heuristic contains now the objective function value (primal value)
				// of the heuristically computed ILP-solution.
				// We have to check if this solution is better than the currently best primal solution.
				if(master_->betterPrimal(heuristic)) {
#ifdef OGDF_DEBUG
					static_cast<MaxCPlanarMaster*>(master_)->m_solByHeuristic = true;
#endif
					// Best primal solution has to be updated.
					static_cast<MaxCPlanarMaster*>(master_)->updateBestSubGraph(origEdges, conEdges, delEdges);
					primalValue = heuristic;
					return 1;
				}
			}
			return 0;
		}

	// If \a heuristicLevel is set to value 2, the heuristic is run after each
	// LP-optimization step, i.e. after each iteration.
	}
	else if (static_cast<MaxCPlanarMaster*>(master_)->getHeuristicLevel() == 2) {
		List<NodePair> origEdges;
		List<NodePair> conEdges;
		List<edge> delEdges;

		double heuristic = heuristicImprovePrimalBound(origEdges,conEdges,delEdges);

		// \a heuristic contains now the objective function value (primal value)
		// of the heuristically computed ILP-solution.
		// We have to check if this solution is better than the currently best primal solution.
		if(master_->betterPrimal(heuristic)) {
#ifdef OGDF_DEBUG
			static_cast<MaxCPlanarMaster*>(master_)->m_solByHeuristic = true;
#endif
			// Best primal solution has to be updated
			static_cast<MaxCPlanarMaster*>(master_)->updateBestSubGraph(origEdges, conEdges, delEdges);
			primalValue = heuristic;
			return 1;
		}
		return 0;
	}

	// For any other value of #m_heuristicLevel the function returns 0.
	return 0;
}

//! Computes the number of bags within the given cluster \p c
//! (non recursive)
//! A bag is a set of chunks within the cluster that are connected
//! via subclusters
int MaxCPlanarSub::clusterBags(ClusterGraph &CG, cluster c)
{
	const Graph& G = CG.constGraph();
	if (G.numberOfNodes() == 0) return 0;
	int numChunks = 0; //number of chunks (CCs) within cluster c
	int numBags;       //number of bags (Constructs consisting of CCs connected by subclusters)

	//stores the nodes belonging to c
	List<node> nodesInCluster;
	//stores the corresponding interator to the list element for each node
	NodeArray<ListIterator<node> > listPointer(G);

	NodeArray<bool> isVisited(G, false);
	NodeArray<bool> inCluster(G, false);
	NodeArray<node> parent(G); //parent for path to representative in bag gathering

	//saves status of all nodes in hierarchy subtree at c
	c->getClusterNodes(nodesInCluster);
	int num = nodesInCluster.size();
	if (num == 0) return 0;

#if 0
	std::cout << "#Starting clusterBags with cluster of size " << num << "\n";
#endif

	//now we store the  iterators
	ListIterator<node> it = nodesInCluster.begin();
	while (it.valid())
	{
		listPointer[*it] = it;
		inCluster[*it] = true;
		++it;
	}

	int count = 0;

	//now we make a traversal through the induced subgraph,
	//jumping between the chunks
	while (count < num)
	{
		numChunks++;
		node start = nodesInCluster.popFrontRet();

		//do a BFS and del all visited nodes in nodesInCluster using listPointer
		Queue<node> activeNodes; //could use arraybuffer
		activeNodes.append(start);
		isVisited[start] = true;
		while (!activeNodes.empty())
		{
			node v = activeNodes.pop(); //running node
			parent[v] = start; //representative points to itself
#if 0
			std::cout << "Setting parent of " << v->index() << "  to " << start->index() << "\n";
#endif
			count++;

			for(adjEntry adj : v->adjEntries) {
				node w = adj->twinNode();

				if (v == w) continue; // ignore self-loops

				if (inCluster[w] && !isVisited[w])
				{
					//use for further traversal
					activeNodes.append(w);
					isVisited[w] = true;
					//remove the node from the candidate list
					nodesInCluster.del(listPointer[w]);
				}
			}
		}
	}

#if 0
	std::cout << "Number of chunks: " << numChunks << "\n";
#endif
	//Now all node parents point to the representative of their chunk (start node in search)
	numBags = numChunks; //We count backwards if chunks are connected by subcluster

	//Now we use an idea similar to UNION FIND to gather the bags
	//out of the chunks. Each node has a parent pointer, leading to
	//a representative. Initially, it points to the rep of the chunk,
	//but each time we encounter a subcluster connecting multiple
	//chunks, we let all of them point to the same representative.
	for (cluster cc : c->children)
	{
		List<node> nodesInChild;
		cc->getClusterNodes(nodesInChild);
		std::cout << nodesInChild.size() << "\n";
		ListConstIterator<node> itN = nodesInChild.begin();
		node bagRep = nullptr; //stores the representative for the whole bag
		if (itN.valid()) bagRep = getRepresentative(*itN, parent);
#if 0
		std::cout << " bagRep is " << bagRep->index() << "\n";
#endif
		while (itN.valid())
		{
			node w = getRepresentative(*itN, parent);
#if 0
			std::cout << "  Rep is: " << w->index() << "\n";
#endif
			if (w != bagRep)
			{
				numBags--; //found nodes with different representative, merge
				parent[w] = bagRep;
				parent[*itN] = bagRep; //shorten search path
#if 0
				std::cout << "  Found new node with different rep, setting numBags to " << numBags << "\n";
#endif
			}
			++itN;
		}
	}

	return numBags;
#if 0
	std::cout << "#Number of bags: " << numBags << "\n";
#endif
}


//! returns connectivity status for complete connectivity
//! returns 1 in this case, 0 otherwise
// New version using arrays to check cluster affiliation during graph traversal,
// old version used graph copies

// For complete connectivity also the whole graph needs to
// be connected (root cluster). It therefore does not speed up
// the check to test connectivity of the graph in advance.
// Note that then a cluster induced graph always has to be
// connected to the complement, besides one of the two is empty.

// Uses an array that keeps the information on the cluster
// affiliation and bfs to traverse the graph.
//we rely on the fact that support is a graphcopy of the underlying graph
//with some edges added or removed
bool MaxCPlanarSub::checkCConnectivity(const GraphCopy& support)
{
	const ClusterGraph &CG = *static_cast<MaxCPlanarMaster*>(master_)->getClusterGraph();
	const Graph& G = CG.constGraph();
	//if there are no nodes, there is nothing to check
	if (G.numberOfNodes() < 2) return true;

	//there is always at least the root cluster
	for(cluster c : CG.clusters)
	{
		// For each cluster, the induced graph partitions the graph into two sets.
		// When the cluster is empty, we still check the complement and vice versa.
		bool set1Connected = false;

		//this initialization can be done faster by using the
		//knowledge of the cluster hierarchy and only
		//constructing the NA once for the graph (bottom up tree traversal)
		NodeArray<bool> inCluster(G, false);
		NodeArray<bool> isVisited(G, false);

		//saves status of all nodes in hierarchy subtree at c
		int num = c->getClusterNodes(inCluster);

		int count = 0;
		//search in graph should reach num and V-num nodes
		node complementStart = nullptr;

		//we start with a non-empty set
		node start = G.firstNode();
		bool startState = inCluster[start];

		Queue<node> activeNodes; //could use arraybuffer
		activeNodes.append(start);
		isVisited[start] = true;

		//could do a shortcut here for case |c| = 1, but
		//this would make the code more complicated without large benefit
		while (!activeNodes.empty())
		{
			node v = activeNodes.pop(); //running node
			count++;
			node u = support.copy(v);


			for(adjEntry adj : u->adjEntries) {
				node w = support.original(adj->twinNode());

				if (v == w) continue; // ignore self-loops

				if (inCluster[w] != startState) complementStart = w;
				else if (!isVisited[w])
				{
					activeNodes.append(w);
					isVisited[w] = true;
				}
			}
		}
		//check if we reached all nodes
		//we assume that the graph is connected, otherwise check
		//fails for root cluster anyway
		//(we could have a connected cluster and a connected complement)

		//condition depends on the checked set, cluster or complement
		set1Connected = startState ? count == num : count == G.numberOfNodes() - num;
#if 0
		std::cout << "Set 1 connected: " << set1Connected << " Cluster? " << startState << "\n";
#endif

		if (!set1Connected) return false;
		//check if the complement of set1 is also connected
		//two cases: complement empty: ok
		//           complement not empty,
		//           but no complementStart found: error
		//In case of the root cluster, this always triggers,
		//therefore we have to continue
		if (G.numberOfNodes() == count)
			continue;
		OGDF_ASSERT(complementStart != nullptr);

		activeNodes.append(complementStart);
		isVisited[complementStart] = true;
		startState = ! startState;
		int ccount = 0;
		while (!activeNodes.empty())
		{
			node v = activeNodes.pop(); //running node
			ccount++;
			node u = support.copy(v);

			for(adjEntry adj : u->adjEntries) {
				node w = support.original(adj->twinNode());

				if (v == w) continue; // ignore self-loops

				if (!isVisited[w])
				{
					activeNodes.append(w);
					isVisited[w] = true;
				}
			}
		}
		//Check if we reached all nodes
		if (ccount + count != G.numberOfNodes())
			return false;
	}
	return true;
}

//only left over for experimental evaluation of speedups
bool MaxCPlanarSub::checkCConnectivityOld(const GraphCopy& support)
{
	//Todo: It seems to me that this is not always necessary:
	//For two clusters, we could stop even if support is not connected
	if (isConnected(support)) {

		GraphCopy *cSupport;
		cluster c = static_cast<MaxCPlanarMaster*>(master_)->getClusterGraph()->firstCluster();

		while (c != nullptr) {
			// Determining the nodes of current cluster
			List<node> clusterNodes;
			c->getClusterNodes(clusterNodes);

			// Step1: checking the restgraph for connectivity
			GraphCopy cSupportRest((const Graph&)support);

			for (node v : clusterNodes) {
				node cv1 = support.copy(v);
				node cv2 = cSupportRest.copy(cv1);
				cSupportRest.delNode(cv2);
			}

			// Checking \a cSupportRest for connectivity
			if (!isConnected(cSupportRest)) {
				return false;
			}

			// Step2: checking the cluster induced subgraph for connectivity
			cSupport = new GraphCopy((const Graph&)support);
			NodeArray<bool> inCluster(*static_cast<MaxCPlanarMaster*>(master_)->getGraph());
			inCluster.fill(false);

			for (node v : clusterNodes) {
				inCluster[v] = true;
			}

			node v = static_cast<MaxCPlanarMaster*>(master_)->getGraph()->firstNode();
			node succ;
			while (v!=nullptr) {
				succ = v->succ();
				if (!inCluster[v]) {
					node cv1 = support.copy(v);
					node cv2 = cSupport->copy(cv1);
					cSupport->delNode(cv2);
				}
				v = succ;
			}
			if (!isConnected(*cSupport)) {
				return false;
			}
			delete cSupport;

			// Next cluster
			c = c->succ();
		}

	} else {
		return false;
	}
	return true;

}

bool MaxCPlanarSub::feasible() {
#if 0
	std::cout << "Checking feasibility\n";
#endif

	if (!integerFeasible()) {
		return false;
	}
	else {

		// Checking if the solution induced graph is completely connected.
		GraphCopy support(*static_cast<MaxCPlanarMaster*>(master_)->getGraph());
		intSolutionInducedGraph(support);

		//introduced merely for debug checks
		bool cc = checkCConnectivity(support);
		bool ccOld = checkCConnectivityOld(support);
		if (cc != ccOld)
		{
			std::cout << "CC: "<<cc<<" CCOLD: "<<ccOld<<"\n";

		}
		OGDF_ASSERT (cc == ccOld);
		if (!cc) return false;

		// Checking if the solution induced graph is planar.

		if (BoyerMyrvold().isPlanarDestructive(support)) {

			// Current solution is integer feasible, completely connected and planar.
			// Checking, if the objective function value of this subproblem is > than
			// the current optimal primal solution.
			// If so, the solution induced graph is updated.
			double primalBoundValue = (double)(floor(lp_->value()) + 0.79);
			if (master_->betterPrimal(primalBoundValue)) {
				master_->primalBound(primalBoundValue);
				updateSolution();
			}
			return true;

		} else {
			return false;
		}
	}
}

#if 0
static void dfsIsConnected(node v, NodeArray<bool> &visited, int &count)
{
	count++;
	visited[v] = true;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		node w = e->opposite(v);
		if (!visited[w]) dfsIsConnected(w,visited,count);
	}
}

bool MaxCPlanarSub::fastfeasible() {

	if (!integerFeasible()) {
		return false;
	}
	else {

		// Checking if the solution induced Graph is completely connected.
		GraphCopy support(*((MaxCPlanarMaster*)master_)->getGraph());
		intSolutionInducedGraph(support);

		//Todo: It seems to me that this is not always necessary:
		//For two clusters, we could stop even if support is not connected
		//we also do not need the root cluster
		if (isConnected(support)) {

			GraphCopy *cSupport;
			cluster c = ((MaxCPlanarMaster*)master_)->getClusterGraph()->firstCluster();

			while (c != nullptr)
			{
				if (c == ((MaxCPlanarMaster*)master_)->getClusterGraph().rootCluster())
				{
					//attention: does the rest of the code rely on the fact
					//that the root is connected
					// Next cluster
					c = c->succ();
					continue;
				}
				// Determining the nodes of current cluster
				List<node> clusterNodes;
				c->getClusterNodes(clusterNodes);

				int count = 0;
				NodeArray<bool> blocked(support, false);
				// Step1: checking the restgraph for connectivity
				ListIterator<node> it;
				node cv1, cv2;

				for (it=clusterNodes.begin(); it.valid(); ++it)
				{
					blocked[*it] = true;

				}

				// Checking \a cSupportRest for connectivity

				if (clusterNodes.size() < support.numberOfNodes())
				{
					//search for a node outside c
					//should be done more efficiently, rewrite this
					node runv = support->firstNode();
					while (blocked[runv]) {runv = runv->succ();}

					dfsIsConnected(runv,blocked,count);
					if (count != support.numberOfNodes()-clusterNodes.size())
						return false;
				}


				// Step2: checking the cluster induced subgraph for connectivity
#if 0
				NodeArray<bool> inCluster(*((MaxCPlanarMaster*)master_)->getGraph());
				inCluster.fill(false);
#endif
				blocked.init(support, true);

				for (it=clusterNodes.begin(); it.valid(); ++it) {
					blocked[*it] = false;
				}
				node v = ((MaxCPlanarMaster*)master_)->getGraph()->firstNode();
				node succ;
				while (v!=0) {
					succ = v->succ();
					if (!inCluster[v]) {
						cv1 = support.copy(v);
						cv2 = cSupport->copy(cv1);
						cSupport->delNode(cv2);
					}
					v = succ;
				}
				if (!isConnected(*cSupport)) {
					return false;
				}
				delete cSupport;

				// Next cluster
				c = c->succ();
			}

		} else {
			return false;
		}

		// Checking for planarity

		BoyerMyrvold bm;
		bool planar = bm.planarDestructive(support);
		if (planar) {

			// Current solution is integer feasible, completely connected and planar.
			// Checking, if the objective function value of this subproblem is > than
			// the current optimal primal solution.
			// If so, the solution induced graph is updated.
			double primalBoundValue = (double)(floor(lp_->value()) + 0.79);
			if (master_->betterPrimal(primalBoundValue)) {
				master_->primalBound(primalBoundValue);
				updateSolution();
			}
			return true;

		} else {
			return false;
		}
	}
}
#endif

void MaxCPlanarSub::intSolutionInducedGraph(GraphCopy &support) {
	edge e, ce;
	node v, w, cv, cw;
	for (int i=0; i<nVar(); ++i) {
		if ( xVal(i) >= 1.0-(master_->eps()) ) {

			if (static_cast<EdgeVar*>(variable(i))->theEdgeType() == EdgeVar::EdgeType::Connect) {

				// If Connection-variables have value == 1.0 they have to be ADDED to the support graph.
				v = static_cast<EdgeVar*>(variable(i))->sourceNode();
				w = static_cast<EdgeVar*>(variable(i))->targetNode();
				cv = support.copy(v);
				cw = support.copy(w);
				support.newEdge(cv,cw);
			}
		} else {

			// If Original-variables have value == 0.0 they have to be DELETED from the support graph.
			if (static_cast<EdgeVar*>(variable(i))->theEdgeType() == EdgeVar::EdgeType::Original) {
				e = static_cast<EdgeVar*>(variable(i))->theEdge();
				ce = support.copy(e);
				support.delEdge(ce);
			}
		}
	}
}


void MaxCPlanarSub::kuratowskiSupportGraph(GraphCopy &support, double low, double high) {

	edge e, ce;
	node v, w, cv, cw;
	for (int i=0; i<nVar(); ++i) {
		if (xVal(i) >= high) {
			// If variables have value >= \p high and are of type Connect
			// they are ADDED to the support graph.
			if (static_cast<EdgeVar*>(variable(i))->theEdgeType() == EdgeVar::EdgeType::Connect) {
				v = static_cast<EdgeVar*>(variable(i))->sourceNode();
				w = static_cast<EdgeVar*>(variable(i))->targetNode();
				cv = support.copy(v);
				cw = support.copy(w);
				support.newEdge(cv,cw);
			} else continue;
		} else if (xVal(i) <= low) {
			// If variables have value <= \p low and are of type Original
			// they are DELETED from the support graph.
			if (static_cast<EdgeVar*>(variable(i))->theEdgeType() == EdgeVar::EdgeType::Original) {
				e = static_cast<EdgeVar*>(variable(i))->theEdge();
				ce = support.copy(e);
				support.delEdge(ce);
			} else continue;
		}
		else {	// Value of current variable lies between \p low and \p high.
			// Variable is added/deleted randomized according to its current value.
			// Variable of type Original is deleted with probability 1-xVal(i).
			if (static_cast<EdgeVar*>(variable(i))->theEdgeType() == EdgeVar::EdgeType::Original) {
				double ranVal = randomDouble(0.0,1.0);
				if (ranVal > xVal(i)) {
					e = static_cast<EdgeVar*>(variable(i))->theEdge();
					ce = support.copy(e);
					support.delEdge(ce);
				}
			} else {
				// Variable of type Connect is added with probability of xVal(i).
				double ranVal = randomDouble(0.0,1.0);
				if (ranVal < xVal(i)) {
					v = static_cast<EdgeVar*>(variable(i))->sourceNode();
					w = static_cast<EdgeVar*>(variable(i))->targetNode();
					cv = support.copy(v);
					cw = support.copy(w);
					// searchEdge ist hier wohl ueberfluessig... (assertion)
					if (!support.searchEdge(cv,cw)) support.newEdge(cv,cw);
				}
			}
		}
	}
}


void MaxCPlanarSub::connectivitySupportGraph(GraphCopy &support, EdgeArray<double> &weight) {

	// Step 1+2: Create the support graph & Determine edge weights and fill the EdgeArray \p weight.
	// MCh: warning: modified by unifying both steps. performance was otherwise weak.
	edge ce;
	node v, w, cv, cw;
	//initializes weight array to original graph (values undefined)
	weight.init(support);
	for (int i=0; i<nVar(); ++i) {
		EdgeVar* var = static_cast<EdgeVar*>(variable(i));
		double val = xVal(i);
		//weight array entry is set for all nonzero values
		if (val > master()->eps()) {
			// Connection edges have to be added.
			if (var->theEdgeType() == EdgeVar::EdgeType::Connect) {
				v = var->sourceNode();
				w = var->targetNode();
				cv = support.copy(v);
				cw = support.copy(w);
				weight[ support.newEdge(cv,cw) ] = val;
			} else
				weight[ support.chain(var->theEdge()).front() ] = val;
		} else {
		// Original edges have to be deleted if their current value equals 0.0.
#if 0
			if (val <= master()->eps())
#endif
			if (var->theEdgeType() == EdgeVar::EdgeType::Original) {
				ce = support.copy( var->theEdge() );
				support.delEdge(ce);
			}
		}
	}
	//TODO: KK: Removed this (think it is safe), test!
	// Step 2:
#if 0
	for (int i=0; i<nVar(); ++i)
	{
		v = ((EdgeVar*)variable(i))->sourceNode();
		w = ((EdgeVar*)variable(i))->targetNode();
		//TODO: Inefficient! Speed search up
		e = support.searchEdge(support.copy(v),support.copy(w));
		if (e) weight[e] = xVal(i);
	}
#endif
}

// Implementation and usage of separation algorithmns
// for the Kuratowski- and the Connectivity- constraints

int MaxCPlanarSub::separateReal(double minViolate) {

	// The number of generated and added constraints.
	// Each time a constraint is created and added to the buffer, the variable \a count is incremented.
	// When adding the created constraints \a nGenerated and \a count are checked for equality.
	int nGenerated = 0;
	int count = 0;
	m_constraintsFound = false;

	if(master()->m_useDefaultCutPool)
		nGenerated = constraintPoolSeparation(0,nullptr,minViolate);
	if(nGenerated>0) return nGenerated;

	// CUT SEPARATION

	// We first try to regenerate cuts from our cutpools
	nGenerated = separateConnPool(minViolate);
	if (nGenerated > 0)
	{
#ifdef OGDF_DEBUG
	Logger::slout()<<"con-regeneration.";
#endif
		return nGenerated;
		//TODO: Check if/how we can proceed here, i.e. should we have this else?
	}
	else
	{
#ifdef OGDF_DEBUG
#if 0
	std::cout<<"Connectivity Regeneration did not work\n";
#endif
#endif
		GraphCopy support(*static_cast<MaxCPlanarMaster*>(master())->getGraph());
		EdgeArray<double> w;
		connectivitySupportGraph(support,w);

#if 0
		// Buffer for the constraints
		int nClusters = (((MaxCPlanarMaster*)master_)->getClusterGraph())->numberOfClusters();
		ArrayBuffer<Constraint *> cConstraints(master_,2*nClusters,false);
#endif

		GraphCopy *c_support;
		EdgeArray<double> c_w;

		// INTER-CLUSTER CONNECTIVITY

		for (cluster c : static_cast<MaxCPlanarMaster*>(master_)->getClusterGraph()->clusters) {

			c_support = new GraphCopy((const Graph&)support);
			c_w.init(*c_support);

			// Copying edge weights to \a c_w.
			List<double> weights;
			for(edge e : support.edges) {
				weights.pushBack(w[e]);
			}
			ListConstIterator<double> wIt = weights.begin();
			for(edge c_e : c_support->edges) {
				if (wIt.valid()) c_w[c_e] = (*wIt);
				++wIt;
			}

			// Residue graph is determined and stored in \a c_support.
			List<node> clusterNodes;
			c->getClusterNodes(clusterNodes);
			for (node v : clusterNodes) {
				node cCopy1 = support.copy(v);
				node cCopy2 = c_support->copy(cCopy1);
				c_support->delNode(cCopy2);
			}

			// Checking if Graph is connected.
			if (isConnected(*c_support)) {

				MinCut mc(*c_support,c_w);
				List<edge> cutEdges;
				double mincutV = mc.minimumCut();
				if (mincutV < 1.0-master()->eps()-minViolate) {

					mc.cutEdges(cutEdges,*c_support);

					List<NodePair> cutNodePairs;
					NodePair np;
					for (edge cutEdge : cutEdges) {
						node cv = c_support->original(cutEdge->source());
						node cw = c_support->original(cutEdge->target());
						node ccv = support.original(cv);
						node ccw = support.original(cw);
						np.source = ccv;
						np.target = ccw;
						cutNodePairs.pushBack(np);
					}

					// Create constraint
					bufferedForCreation.push(new CutConstraint(static_cast<MaxCPlanarMaster*>(master()), this, cutNodePairs));
					count++;
				}
			} else {
				NodeArray<int> comp(*c_support);
				connectedComponents(*c_support,comp);
				List<node> partition;
				NodeArray<bool> isInPartition(*c_support);
				isInPartition.fill(false);
				for(node v : c_support->nodes) {
					if (comp[v] == 0) {
						partition.pushBack(v);
						isInPartition[v] = true;
					}
				}

				// Computing nodePairs defining the cut.
				List<NodePair> cutEdges;
				NodePair np;
				for (node v : partition) {
					node cv, cw;
					for (node wSupp : c_support->nodes) {
						if ((wSupp != v) && !isInPartition[wSupp]) {
							cw = c_support->original(wSupp);
							cv = c_support->original(v);
							np.source = support.original(cw);
							np.target = support.original(cv);
							cutEdges.pushBack(np);
						}
					}
				}

				// Create cut-constraint
				bufferedForCreation.push(new CutConstraint(static_cast<MaxCPlanarMaster*>(master()), this, cutEdges)); // always violated enough
				count++;
			}
			delete c_support;

		}

		// INTRA-CLUSTER CONNECTIVITY

		// The initial constraints can not guarantee the connectivity of a cluster.
		// Thus, for each cluster we have to check, if the induced Graph is connected.
		// If so, we compute the mincut and create a corresponding constraint.
		// Otherwise a constraint is created in the same way as above.

		for (cluster c : static_cast<MaxCPlanarMaster*>(master_)->getClusterGraph()->clusters) {

			c_support = new GraphCopy((const Graph&)support);
			c_w.init(*c_support);

			List<double> weights;
			for(edge e : support.edges) {
				weights.pushBack(w[e]);
			}
			ListConstIterator<double> wIt = weights.begin();
			for(edge c_e : c_support->edges) {
				if (wIt.valid()) c_w[c_e] = (*wIt);
				++wIt;
			}

			// Cluster induced Graph is determined and stored in \a c_support.
			List<node> clusterNodes;
			c->getClusterNodes(clusterNodes);
			NodeArray<bool> isInCluster(*c_support);
			isInCluster.fill(false);
			for (node v : clusterNodes) {
				node cv = support.copy(v);
				isInCluster[c_support->copy(cv)] = true;
			}
			for(node v = c_support->firstNode(); v; ) {
				node succ = v->succ();
				if (!isInCluster[v]) {
					c_support->delNode(v);
				}
				v = succ;
			}

			// Checking if Graph is connected.
			if (isConnected(*c_support)) {

				MinCut mc(*c_support,c_w);
				List<edge> cutEdges;
				double x = mc.minimumCut();
				if (x < 1.0-master()->eps()-minViolate) {
					mc.cutEdges(cutEdges,*c_support);
					List<NodePair> cutNodePairs;
					NodePair np;
					for (edge cutEdge : cutEdges) {
						node cv = c_support->original(cutEdge->source());
						node cw = c_support->original(cutEdge->target());
						node ccv = support.original(cv);
						node ccw = support.original(cw);
						np.source = ccv;
						np.target = ccw;
						cutNodePairs.pushBack(np);
					}

					// Create constraint
					bufferedForCreation.push(new CutConstraint(static_cast<MaxCPlanarMaster*>(master()), this, cutNodePairs));
					count++;
				}
			}

			else {
				NodeArray<int> comp(*c_support);
				connectedComponents(*c_support,comp);
				List<node> partition;
				NodeArray<bool> isInPartition(*c_support);
				isInPartition.fill(false);
				for(node v : c_support->nodes) {
					if (comp[v] == 0) {
						partition.pushBack(v);
						isInPartition[v] = true;
					}
				}

				List<NodePair> cutEdges;
				NodePair np;
				for (node vi : partition) {
					node cv, cw;
					for (node wSupp : c_support->nodes) {
						if ((wSupp != vi) && !isInPartition[wSupp]) {
							cw = c_support->original(wSupp);
							cv = c_support->original(vi);
							np.source = support.original(cw);
							np.target = support.original(cv);
							cutEdges.pushBack(np);
						}
					}
				}

				// Create Cut-constraint
				bufferedForCreation.push(new CutConstraint(static_cast<MaxCPlanarMaster*>(master()), this, cutEdges)); // always violated enough.

				count++;
			}
			delete c_support;
		}

		// Adding constraints
		if(count>0) {
			if(master()->pricing())
				nGenerated = createVariablesForBufferedConstraints();
			if(nGenerated==0) {
				ArrayBuffer<Constraint*> cons(count,false);
				while(!bufferedForCreation.empty()) {
					Logger::slout() <<"\n"; ((CutConstraint*&)bufferedForCreation.top())->printMe(Logger::slout());
					cons.push( bufferedForCreation.popRet() );
				}
				OGDF_ASSERT( bufferedForCreation.size()==0 );
				nGenerated = addCutCons(cons);
				OGDF_ASSERT( nGenerated == count );
				master()->updateAddedCCons(nGenerated);
			}
			m_constraintsFound = true;
			return nGenerated;
		}
	}

	// KURATOWSKI SEPARATION

	// We first try to regenerate cuts from our cutpools
	nGenerated = separateKuraPool(minViolate);
	if (nGenerated > 0) {
		Logger::slout()<<"kura-regeneration.";
		return nGenerated; //TODO: Check if/how we can proceed here
	}
	// Since the Kuratowski support graph is computed from fractional values, an extracted
	// Kuratowski subdivision might not be violated by the current solution.
	// Thus, the separation algorithm is run several times, each time checking if the first
	// extracted subdivision is violated.
	// If no violated subdivisions have been extracted after \a nKuratowskiIterations iterations,
	// the algorithm behaves like "no constraints have been found".

	GraphCopy *kSupport;
	SList<KuratowskiWrapper> kuratowskis;
#if 0
	BoyerMyrvold *bm1;
#endif
	BoyerMyrvold *bm2;
	bool violatedFound = false;

	// The Kuratowski support graph is created randomized  with probability xVal (1-xVal) to 0 (1).
	// Because of this, Kuratowski-constraints might not be found in the current support graph.
	// Thus, up to #m_nKSupportGraphs are computed and checked for planarity.

	for (int i = 0; i < static_cast<MaxCPlanarMaster*>(master_)->getNKuratowskiSupportGraphs(); ++i) {

		// If a violated constraint has been found, no more iterations have to be performed.
		if (violatedFound) break;

		kSupport = new GraphCopy(*static_cast<MaxCPlanarMaster*>(master())->getGraph());
		kuratowskiSupportGraph(*kSupport, static_cast<MaxCPlanarMaster*>(master_)->getKBoundLow(), static_cast<MaxCPlanarMaster*>(master_)->getKBoundHigh());

		if (isPlanar(*kSupport)) {
			delete kSupport;
			continue;
		}

		int iteration = 1;
		while (static_cast<MaxCPlanarMaster*>(master_)->getKIterations() >= iteration) {

			// Testing support graph for planarity.
			bm2 = new BoyerMyrvold();
			/* bool planar = */ bm2->planarEmbedDestructive(*kSupport, kuratowskis, static_cast<MaxCPlanarMaster*>(master_)->getNSubdivisions(), false, false, true);
			delete bm2;

			// Checking if first subdivision is violated by current solution
			// Performance should be improved somehow!!!
			SListConstIterator<KuratowskiWrapper> kw = kuratowskis.begin();
			double leftHandSide = subdivisionLefthandSide(kw,kSupport);

			// Only violated constraints are created and added
			// if \a leftHandSide is greater than the number of edges in subdivision -1, the constraint is violated by current solution.
			if (leftHandSide > (*kw).edgeList.size()-(1-master()->eps()-minViolate)) {

				violatedFound = true;

				// Buffer for new Kuratowski constraints
				ArrayBuffer<Constraint *> kConstraints(kuratowskis.size(),false);

				SListPure<edge> subdiv;
				SListPure<NodePair> subdivOrig;
				NodePair np;
				node v,w;

				for (edge ei : (*kw).edgeList) {
					subdiv.pushBack(ei);
				}
				for (edge si : subdiv) {
					v = si->source();
					w = si->target();
					np.source = kSupport->original(v);
					np.target = kSupport->original(w);
					subdivOrig.pushBack(np);
				}

				// Adding first Kuratowski constraint to the buffer.
				kConstraints.push(new ClusterKuratowskiConstraint(static_cast<MaxCPlanarMaster*>(master()), subdivOrig.size(), subdivOrig));
				count++;
				subdiv.clear();
				subdivOrig.clear();

				// Checking further extracted subdivisions for violation.
				++kw;
				while(kw.valid()) {
					leftHandSide = subdivisionLefthandSide(kw,kSupport);
					if (leftHandSide > (*kw).edgeList.size()-(1-master()->eps()-minViolate)) {

						for (edge ei : (*kw).edgeList) {
							subdiv.pushBack(ei);
						}
						for (edge si : subdiv) {
							v = si->source();
							w = si->target();
							np.source = kSupport->original(v);
							np.target = kSupport->original(w);
							subdivOrig.pushBack(np);
						}

						// Adding Kuratowski constraint to the buffer.
						kConstraints.push(new ClusterKuratowskiConstraint(static_cast<MaxCPlanarMaster*>(master()), subdivOrig.size(), subdivOrig));
						count++;
						subdiv.clear();
						subdivOrig.clear();
					}
					++kw;
				}

				// Adding constraints to the pool.
				for(auto &kConstraint : kConstraints) {
					Logger::slout() <<"\n"; ((ClusterKuratowskiConstraint*&)kConstraint)->printMe(Logger::slout());
				}
				nGenerated += addKuraCons(kConstraints);
				if (nGenerated != count)
				std::cerr << "Number of added constraints doesn't match number of created constraints" << std::endl;
				break;

			} else {
				kuratowskis.clear();
				iteration++;
			}
		}
		delete kSupport;
	}

	if (nGenerated > 0) {
		static_cast<MaxCPlanarMaster*>(master_)->updateAddedKCons(nGenerated);
		m_constraintsFound = true;
	}
	return nGenerated;
}

int MaxCPlanarSub::createVariablesForBufferedConstraints() {
	List<Constraint*> crit;
	for(int i = bufferedForCreation.size(); i-- > 0;) {
#if 0
		((CutConstraint*)bufferedForCreation[i])->printMe(); Logger::slout() << ": ";
#endif
		for(int j = nVar(); j-- > 0;) {
#if 0
			((EdgeVar*)variable(j))->printMe();
			Logger::slout() << "=" << bufferedForCreation[i]->coeff(variable(j)) << "/ ";
#endif
			if(bufferedForCreation[i]->coeff(variable(j))!=0.0) {
#if 0
				Logger::slout() << "!";
#endif
				goto nope;
			}
		}
		crit.pushBack(bufferedForCreation[i]);
		nope:;
	}
	if(crit.size()==0) return 0;
	ArrayBuffer<ListIterator<NodePair> > creationBuffer(crit.size());
	for (ListIterator<NodePair> npit = master()->m_inactiveVariables.begin(); npit.valid(); ++npit) {
		bool select = false;
		ListIterator<Constraint*> ccit = crit.begin();
		while(ccit.valid()) {
			if(((BaseConstraint*)(*ccit))->coeff(*npit)) {
				ListIterator<Constraint*> delme = ccit;
				++ccit;
				crit.del(delme);
				select = true;
			} else
				++ccit;
		}
		if(select) creationBuffer.push(npit);
		if(crit.size()==0) break;
	}
	if( crit.size() ) { // something remained here...
		for(int i = bufferedForCreation.size(); i-- > 0;) {
			delete bufferedForCreation[i];
		}
		detectedInfeasibility = true;
		return 0; // a positive value denotes infeasibility
	}
	OGDF_ASSERT(crit.size()==0);
	ArrayBuffer<Variable*> vars(creationBuffer.size(),false);
	master()->m_varsCut += creationBuffer.size();
	int gen = creationBuffer.size();
	for(int j = gen; j-- > 0;) {
		vars.push( master()->createVariable( creationBuffer[j] ) );
	}
	myAddVars(vars);
	return -gen;
}


#if 0
// !! This function is incorrect (due to uninitialized usage of variable rc)       !!
// !! and cannot work correctly (seems to be a placeholder for further development) !!
// Therefore it has been commented out
int MaxCPlanarSub::pricingReal(double minViolate) {
	if(!master()->pricing()) return 0; // no pricing
	Top10Heap<Prioritized<ListIterator<NodePair> > > goodVar(master()->m_numAddVariables);
	for(ListIterator<NodePair> it = master()->m_inactiveVariables.begin(); it.valid(); ++it) {
		double rc;
		EdgeVar v(master(), -master()->m_epsilon, EdgeVar::EdgeType::Connect, (*it).v1, (*it).v2);
		if(v.violated(rc) && rc>=minViolate) {
			Prioritized<ListIterator<NodePair> > entry(it,rc);
			goodVar.pushBlind( entry );
		}
	}

	int nv = goodVar.size();
	if(nv > 0) {
		ArrayBuffer<Variable*> vars(nv,false);
		for(int i = nv; i-- > 0;) {
			ListIterator<NodePair> it = goodVar[i].item();
			vars.push( master()->createVariable(it) );
		}
		myAddVars(vars);
	}
	return nv;
}
#endif

int MaxCPlanarSub::repair() {
	//warning. internal abacus stuff BEGIN
	bInvRow_ = new double[nCon()];
	lp_->getInfeas(infeasCon_, infeasVar_, bInvRow_);
	//warning. internal abacus stuff END

	// only output begin
	Logger::slout() << "lpInfeasCon=" << lp_->infeasCon()->size()
		<< " var="<< infeasVar_
		<< " con="<< infeasCon_<< "\n";
	for(int i=0; i<nCon(); ++i)
		Logger::slout() << bInvRow_[i] << " " << std::flush;
	Logger::slout() << "\n" << std::flush;
	for(int i=0; i<nCon(); ++i) {
		if(bInvRow_[i]!=0) {
			Logger::slout() << bInvRow_[i] << ": " << std::flush;
			ChunkConnection* chc = dynamic_cast<ChunkConnection*>(constraint(i));
			if(chc) chc->printMe(Logger::slout());
			CutConstraint* cuc = dynamic_cast<CutConstraint*>(constraint(i));
			if(cuc) cuc->printMe(Logger::slout());
			ClusterKuratowskiConstraint* kc = dynamic_cast<ClusterKuratowskiConstraint*>(constraint(i));
			if(kc) kc->printMe(Logger::slout());
			Logger::slout() << "\n" << std::flush;
		}
	}
	// only output end

	int added = 0;
	ArrayBuffer<Variable*> nv(1, false);
	for (int i = 0; i < nCon(); ++i) {
		if (bInvRow_[i] < 0) { // negativ: infeasible cut or chunk constraint, or oversatisfies kura
			BaseConstraint* b = dynamic_cast<BaseConstraint*>(constraint(i));
			if (!b) continue; // was: oversatisfied kura. nothing we can do here
			OGDF_ASSERT(b);
			for (ListIterator<NodePair> it = master()->m_inactiveVariables.begin(); it.valid(); ++it) {
				if (b->coeff(*it)) {
					Logger::slout() << "\tFeasibility Pricing: ";
					nv.push(master()->createVariable(it));
					Logger::slout() << "\n";
					myAddVars(nv);
					added = 1;
					goto done;
				}
			}
		}
	}
done:
	//warning. internal abacus stuff BEGIN
	delete[] bInvRow_;
	//warning. internal abacus stuff END
	master()->m_varsKura += added;
	return added;
}

int MaxCPlanarSub::solveLp() {
	m_reportCreation = 0;
	const double minViolation = 0.001; // value fixed by abacus...

	Logger::slout() << "SolveLp\tNode=" << this->id() << "\titeration=" << this->nIter_ << "\n";


	if(master()->pricing() && id()>1 && nIter_==1) { // ensure that global variables are really added...
		StandardPool<Variable, Constraint>* vp = master()->varPool();
		int addMe = vp->number() - nVar();
		OGDF_ASSERT(addMe >=0 );
		if(addMe) {
			Logger::slout() << "ARRRGGGG!!!! ABACUS SUCKS!!\n";
			Logger::slout() << nVar() << " variables of " << vp->number() << " in model. Fetching " << addMe << ".\n" << std::flush;
#if 0
			master()->activeVars->loadIndices(this); // current indexing scheme
#endif
			m_reportCreation = 0;
			for(int i=0; i<vp->size(); ++i ) {
				PoolSlot<Variable, Constraint> * slot = vp->slot(i);
				Variable* v = slot->conVar();
				if(v && !v->active()) {
					addVarBuffer_->insert(slot,true);
					--m_reportCreation;
				}
			}
			OGDF_ASSERT(m_reportCreation == -addMe);
			return 0; // rerun;
		}
	}


	if(master()->m_checkCPlanar && master()->feasibleFound()) {
		Logger::slout() << "Feasible Solution Found. That's good enough! C-PLANAR\n";
		master()->clearActiveRepairs();
		return 1;
	}

	if(bufferedForCreation.size()) {
		m_reportCreation = bufferedForCreation.size();
		ArrayBuffer<Constraint*> cons(bufferedForCreation.size(),false);
		while(!bufferedForCreation.empty()) {
			((CutConstraint*&)bufferedForCreation.top())->printMe(Logger::slout());Logger::slout() <<"\n";
			cons.push( bufferedForCreation.popRet() );
		}
		OGDF_ASSERT( bufferedForCreation.size()==0 );
		addCutCons(cons);
		master()->updateAddedCCons(m_reportCreation);
		master()->clearActiveRepairs();
		return 0;
	}

	inOrigSolveLp = true;
	++(master()->m_solvesLP);
	int ret = Sub::solveLp();
	inOrigSolveLp = false;
	if(ret) {
		if(!(master()->m_checkCPlanar))
			return ret;
		if(master()->pricing()) {
			if(criticalSinceBranching.size()) {
				ListIterator<NodePair> best;
				Array<ListIterator<Constraint*> > bestKickout;
				int bestCCnt = 0;
				for (ListIterator<NodePair> nit = master()->m_inactiveVariables.begin(); nit.valid(); ++nit) {
					ArrayBuffer<ListIterator<Constraint*> > kickout(criticalSinceBranching.size());
					for (ListIterator<Constraint*> cit = criticalSinceBranching.begin(); cit.valid(); ++cit) {
						BaseConstraint* bc = dynamic_cast<BaseConstraint*>(*cit);
						OGDF_ASSERT(bc);
						if( bc->coeff(*nit) > 0.99) {
							kickout.push(cit);
						}
					}
					if(kickout.size() > bestCCnt) {
						bestCCnt = kickout.size();
						best = nit;
						kickout.compactMemcpy(bestKickout);
					}
				}
				if(bestCCnt>0) {
					ArrayBuffer<Variable*> vars(1,false);
					vars.push( master()->createVariable(best) );
					myAddVars(vars);
					for(auto elem : bestKickout) {
						criticalSinceBranching.del(elem);
					}
					m_reportCreation = -1;
					++(master()->m_varsBranch);
					master()->clearActiveRepairs();
					return 0;
				}
				criticalSinceBranching.clear(); // nothing helped... resorting to full repair
#if 0
				master()->clearActiveRepairs();
				return 0;
#endif
			}
#if 0
			else {
#endif
				m_reportCreation = -repair();
				if(m_reportCreation<0) {
					++(master()->m_activeRepairs);
					return 0;
				}
#if 0
			}
#endif
		}
		master()->clearActiveRepairs();
		dualBound_ = -master()->infinity();


#ifdef OGDF_DEBUG
		for (const NodePair &p : master()->m_inactiveVariables) {
			int t = p.source->index();
			if (t == 0) {
				if (p.target->index() == 35)
					Logger::slout() << "VAR MISSING: 0-35\n";
			}
			else {
				if (t % 6 == 0) continue;
				if (p.target->index() == t + 5)
					Logger::slout() << "VAR MISSING: " << t << "-" << (t + 5) << "\n";
			}
		}
		for (int t = 0; t < nVar(); ++t) {
			EdgeVar* v = static_cast<EdgeVar*>(variable(t));
			if ((v->sourceNode()->index() == 27 && v->targetNode()->index() == 32) ||
				(v->sourceNode()->index() == 32 && v->targetNode()->index() == 27)) {
				Logger::slout() << "VAR 27-32: " << xVal(t) << "(" << lBound(t) << "," << uBound(t) << ")\n";
			}
		}
		for (int t = 0; t < nVar(); ++t) {
			EdgeVar* v = static_cast<EdgeVar*>(variable(t));
			if (v->theEdgeType() == EdgeVar::EdgeType::Connect
			 && lBound(t) == uBound(t)) {
				Logger::slout() << "VAR FIXED: ";
				v->printMe(Logger::slout());
				Logger::slout() << " TO " << lBound(t) << "\n";
			}
		}
#endif //OGDF_DEBUG

#if 0
		infeasibleSub(); // great! a virtual function that is private...
#endif
		Logger::slout() << "\tInfeasible\n";
		return 1; // report any errors
	}
	master()->clearActiveRepairs();
	OGDF_ASSERT( !lp_->infeasible() );
	//is set here for pricing only
	if(master()->m_checkCPlanar) // was: master()->pricing()
		dualBound_=master()->infinity();//666
	Logger::slout() << "\t\tLP-relaxation: " <<  lp_->value() << "\n";
	Logger::slout() << "\t\tLocal/Global dual bound: " << dualBound() << "/" << master_->dualBound() << std::endl;
	realDualBound = lp_->value();

#if 0
	if(master()->m_checkCPlanar2 && dualBound()<master()->m_G->numberOfEdges()-0.79) {
		dualBound(-master()->infinity());
		return 1;
	}
#endif


	if(!master()->pricing()) {
		m_reportCreation = separateReal(minViolation);//use ...O for output
	} else {
		// Pricing-code has been disabled since it is currently incorrect!
		// See MaxCPlanarSub::pricingReal() above for more details.
		OGDF_THROW(AlgorithmFailureException);

#if 0
		m_sepFirst = !m_sepFirst;
		if(m_sepFirst) {
			if( (m_reportCreation = separateRealO(master()->m_strongConstraintViolation)) ) return 0;
			if( detectedInfeasibility ) { Logger::slout() << "Infeasibility detected (a)"<< std::endl; return 1; }
			if( (m_reportCreation = -pricingRealO(master()->m_strongVariableViolation)) ) return 0;
			if( (m_reportCreation = separateRealO(minViolation)) ) return 0;
			if( detectedInfeasibility ) { Logger::slout() << "Infeasibility detected (b)"<< std::endl; return 1; }
			m_reportCreation = -pricingRealO(minViolation);
		} else {
			if( (m_reportCreation = -pricingRealO(master()->m_strongVariableViolation)) ) return 0;
			if( (m_reportCreation = separateRealO(master()->m_strongConstraintViolation)) ) return 0;
			if( detectedInfeasibility ) { Logger::slout() << "Infeasibility detected (c)"<< std::endl; return 1; }
			if( (m_reportCreation = -pricingRealO(minViolation)) ) return 0;
			m_reportCreation = separateRealO(minViolation);
			if( detectedInfeasibility ) { Logger::slout() << "Infeasibility detected (d)"<< std::endl; return 1; }
		}
#endif
	}
	return 0;
}
