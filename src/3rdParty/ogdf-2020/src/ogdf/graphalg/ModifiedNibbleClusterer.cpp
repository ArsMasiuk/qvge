/** \file
 * \brief Implementation of a fast and simple clustering algorithm, Modified Nibble Clusterer
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

#include <ogdf/graphalg/ModifiedNibbleClusterer.h>
#include <ogdf/basic/HashArray.h>

using namespace ogdf;

// This could either be done internally in the single level run, avoiding duplication of runs,
// but having conditional code, or just by calling the single level call and then postProcess
long ModifiedNibbleClusterer::call(Graph &G, NodeArray<long> &clusterNum, NodeArray<long> &topLevelNum)
{
	call(G,clusterNum);
	// Will keep the relations between clusters on the lower level as computed in single level call
	Graph clusterStructure;
	HashArray<long, node> cNode(nullptr);
	for(node v : G.nodes) {
		if (cNode[clusterNum[v]] == nullptr)
			cNode[clusterNum[v]] = clusterStructure.newNode();
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			node w = e->opposite(v);
			if (clusterNum[v] != clusterNum[w]) {
				if (cNode[clusterNum[w]] == nullptr)
					cNode[clusterNum[w]] = clusterStructure.newNode();
				// Also allows multi edges to model connection strenghts!
				// This is simpler for the random walk than storing weights...
				clusterStructure.newEdge(cNode[clusterNum[v]],cNode[clusterNum[w]]);
			}
		}
	}
	int storeClusterThreshold = m_clusterThreshold;
	int storeMaxClusterNum = maxClusterNum;
	maxClusterNum = 25;
	m_clusterThreshold = 4; // Some magic is in every code. Below 4 there is not much
	//justification for a cluster created be nibble, above we might join too much.
	// Now the clusterStructure graph should model the connection structure between
	// the clusters on G computed by the single level call.
	// Note that this graph does not need to be connected in case G is not connected
	NodeArray<long> tln(clusterStructure);
	long topNum = call(clusterStructure, tln);
	for(node v : G.nodes) {
		topLevelNum[v] = tln[cNode[clusterNum[v]]];
	}

	m_clusterThreshold = storeClusterThreshold;
	maxClusterNum = storeMaxClusterNum;
	return topNum;
}

long ModifiedNibbleClusterer::call(Graph &G, NodeArray<long> &clusterNum)
{
	m_pG = &G;
	m_pGC = new GraphCopy(G);

	std::vector< std::vector<node>* > clusters;
	initialize();
	while (m_pGC->numberOfNodes() > 0) {
		m_startNode = selectStartNode();
		std::vector<node>* cluster = new std::vector<node>;
		modifiedNibble(m_startNode, *cluster);
		for (auto &elem : *cluster) {
			m_pGC->delNode(m_pGC->copy(elem));
		}
		clusters.push_back(cluster);
	}
	// Now CCs for non assigned nodes are assigned as clusters
	// And small clusters are dissolved and assigned to their neighbors
#if 0
	postProcess();
#endif
	// Delete cluster vectors
	int cnum = (int)clusters.size();
	for (int i = 0; i < (int)clusters.size(); ++i) {
		std::vector<node> &cluster = *(clusters.at(i));
#if 0
		if (cluster.size() < m_clusterThreshold)
#endif
		// Assign to neighbor(s)
		for (auto &elem : cluster) {
			clusterNum[elem] = i;
		}
		delete clusters.at(i);
	}
	// And small clusters are dissolved and assigned to their neighbors
#if 0
	postProcess(clusterNum);
#endif
	// Delete GraphCopy
	delete m_pGC;
	std::cout << "Created " << cnum << "clusters\n";
	return cnum;
}

//! Initialize before FIRST step
void ModifiedNibbleClusterer::initialize()
{
	// We are not trying to do a perfect partitioning, but also
	// want to avoid unbalanced clusters.
	m_maxClusterSize = min((unsigned int)m_pGC->numberOfNodes(), 2*((unsigned int)m_pGC->numberOfNodes() / maxClusterNum + 1));
}

// Returns conductance of best cluster
double ModifiedNibbleClusterer::findBestCluster(NodeArray<bool> &isActive, std::vector<node> &activeNodes, std::vector<node> &cluster)
{
	ArrayBuffer< Prioritized<int> > sortedPairs((int)activeNodes.size());
	cluster.clear();

	// collect pairs and sort them
	for (int i = 0; i < (int)activeNodes.size(); ++i) {
		const node v = activeNodes.at(i);
		sortedPairs.push(Prioritized<int>(i, - m_prob[v] / v->degree()));
	}
	sortedPairs.quicksort();
	int maxSize = min(static_cast<int>(activeNodes.size()), static_cast<int>(m_maxClusterSize));
	// Now we search for the best cluster in the list (paper description is ambiguous)
	NodeArray<bool> inCluster(*m_pGC, false); // costs dearly but hard to avoid without knowing sizes
	// Save list entry in frontier list of current set to delete nodes that are added to the list
	NodeArray< ListIterator<node> > frontierEntry(*m_pGC, nullptr);
	List<node> frontier; //frontier of current set (next node in list doesn't need to be a member!)
	// Save nodes that become abandoned, i.e. only have neighbors in a current cluster set.
	// These will never again become non-abandoned, i.e. can be collected, but could become
	// part of the cluster, i.e. we need to take care as there might be duplicates in cluster plus
	// abandoned list (but deleting would make it impossible to simply store the index for the solution).
	std::vector<node> abandoned;
	NodeArray<bool> wasAbandoned(*m_pGC, false);
	// As the cluster and the list of abandoned nodes might overlap, we cannot simply add their size
	long numRealAband = 0;
	int volume = 0;
	long cutSize = 0;
	long bestindex = 0;
	double bestConductance = std::numeric_limits<double>::max();
	long bestindexaband = -1;
	for (int run = 0; run < maxSize; ++run) {
		//Check the conductance of the current set
		node next = activeNodes.at(sortedPairs[run].item());
		inCluster[next] = true;

		OGDF_ASSERT(numRealAband >= 0);
		//in case the node was in our frontier make sure we won't consider it later
		if (!(((ListIterator<node>)frontierEntry[next]) == (ListIterator<node>)nullptr)){
			frontier.del(frontierEntry[next]);
			frontierEntry[next] = (ListIterator<node>)nullptr; //will never be used again
		}
		//volume changes by degree of node if not already taken into account
		// as previously abandoned node
		if (wasAbandoned[next]) numRealAband--;
		else volume += next->degree();
		//cutsize changes according to new nodes adjacency
		for(adjEntry adj : next->adjEntries){
			node w = adj->theEdge()->opposite(next);
			if (inCluster[w]) {
				cutSize--;
			}
			else {
				cutSize++;
				frontierEntry[w] = frontier.pushBack(w);
			}
		}

		// We add the abandoned nodes here, i.e. nodes that only have
		// neighbors in the current set left.
		ListIterator<node> itn = frontier.begin();
		while (itn.valid()) {
			node t = (*itn);
			if (wasAbandoned[t]) {
				++itn;
				continue;
			}
			bool aband = true;
			for(adjEntry adj : t->adjEntries){
				if (!inCluster[adj->theEdge()->opposite(t)]) {
					aband = false;
					break;
				}
			}
			if (aband) {
				wasAbandoned[t] = true;
				abandoned.push_back(t);
				numRealAband++;
				// Abandoned nodes are part of the prospective cluster
				// Adding them does not change the cut, but the volume
				volume += t->degree();
			}
			++itn;
		}


		OGDF_ASSERT(cutSize >= 0);
		if (run + numRealAband > m_maxClusterSize) break; // Can only get bigger now
		// Calculate conductance
		double conductance = static_cast<double>(cutSize) / static_cast<double>(min(volume, max(1, 2*m_pGC->numberOfEdges() - volume)));
		if (conductance < bestConductance) {
			bestConductance = conductance;
			bestindex = run;
			bestindexaband = (long)(abandoned.size()-1);
		}
	}
	// Put together our result
	for (int run = 0; run <= bestindex; ++run) {
		node next = activeNodes.at(sortedPairs[run].item());
		if (!wasAbandoned[next]) cluster.push_back(next);
	}
	for (int run = 0; run <= bestindexaband; ++run) {
		cluster.push_back(abandoned.at(run));
	}
#if 0
	std::cout << "Cluster found "<<cluster.size()<< " " << bestConductance<<"\n";
#endif
#ifdef OGDF_DEBUG
	NodeArray<bool> test(*m_pGC, false);
	for (node v : cluster) {
		OGDF_ASSERT(!test[v]);
		test[v] = true;
	}
#endif
	return bestConductance;
}
// run modified nibble starting at snode
void ModifiedNibbleClusterer::modifiedNibble(node snode, std::vector<node> & bestCluster) {
	if (m_pGC->numberOfNodes() < m_clusterThreshold) {
		for(node v : m_pGC->nodes) {
			bestCluster.push_back(m_pGC->original(v));
		}
		return;
	}
	// Initialize current run
	m_prob.init(*m_pGC);
	m_prob[snode] = 1.0;
	m_steps = 0; //Total steps
	long maxSteps = maxClusterSize();
	NodeArray<double> probUpdate(*m_pGC, 0.0); //Needed to avoid serial update
	NodeArray<bool> isActive (*m_pGC, false); //quick check if node is i list active nodes
#if 0
	std::vector<node> bestCluster; // the best cluster found;
#endif

	// Active nodes visited along the walks
	std::vector<node> activeNodes;
	activeNodes.push_back(m_startNode);
	// We never remove nodes from this list again
	isActive[m_startNode] = true;
	int batchi = 0;
	bool finished = false;
	double bestCon = std::numeric_limits<double>::max();
	while (!finished) {
		int t_i = aPGP(batchi);
		long batchsteps = ((t_i > maxSteps ? maxSteps : t_i) - m_steps);
		for (int i = 0; i < batchsteps; ++i) {
			// spread the word, i.e. current m_prob values
			spreadValues(isActive, activeNodes, probUpdate);
			// Check if we reached our active node bound
			// If yes we can either stop or iterate only within the active node set
			// TODO implement the second option
			if ((long)activeNodes.size() > m_maxActiveNodes)
				break;
		}
		std::vector<node> cluster;
		double curCon = findBestCluster(isActive, activeNodes, cluster);
		if (curCon < bestCon) {
			bestCon = curCon;
			bestCluster.clear();
			for (auto &elem : cluster) {
				bestCluster.push_back(m_pGC->original(elem));
			}
			//bestCluster = cluster;
			if (t_i >= maxSteps) finished = true;
			else m_steps = t_i;
		} else {
			// if we could not improve in a larger active node set, we (can safely) stop (?)!
			finished = true;
		}
	}
	std::cout << "Final cluster "<< bestCluster.size() << "  " << bestCon <<"\n";
}

void ModifiedNibbleClusterer::spreadValues(NodeArray<bool> &isActive, std::vector<node> &activeNodes, NodeArray<double> &probUpdate) {
	OGDF_ASSERT(m_spreadProbability > 0.0);
	std::vector<node> affected;
	// spread values at node it, but don't do serial distribution
	for(node v : activeNodes) {
		// distribute evenly
		double spread = m_spreadProbability*m_prob[v] / v->degree();
		m_prob[v] -= m_spreadProbability*m_prob[v];
		for(adjEntry adj : v->adjEntries) {
			node opp = adj->theEdge()->opposite(v);
			//we assume that nodes never run dry, i.e. spread > 0.0
			if (!isActive[opp]) {
				affected.push_back(opp);
				isActive[opp] = true;
			}
			probUpdate[opp] += spread;
		}
	}
	// Now acculumate spread and update
	// Active nodes will always get some spread
	for(node v : activeNodes) {
		m_prob[v] += probUpdate[v];//accumulate
		probUpdate[v] = 0.0; // reinit for following steps
	}
	// Newly affected nodes get spread and become active
	for(node v : affected) {
		activeNodes.push_back(v);
		m_prob[v] = probUpdate[v]; //first time, first value
		probUpdate[v] = 0.0; //reinit for following steps
	}
#ifdef OGDF_DEBUG
	testSpreadSum();
#endif
}

void ModifiedNibbleClusterer::postProcess() {
//TODO
}
node ModifiedNibbleClusterer::selectStartNode() {
	OGDF_ASSERT(m_pGC->numberOfNodes()>0);
	if (m_sns == StartNodeStrategy::Random) return m_pGC->chooseNode();
	node start = m_pGC->firstNode();
	for(node v : m_pGC->nodes) {
		switch (m_sns) {
			case StartNodeStrategy::MaxDeg: if (v->degree() > start->degree()) start = v; break;
			case StartNodeStrategy::MinDeg: if (v->degree() < start->degree()) start = v; break;
			case StartNodeStrategy::Random: std::cerr << "Unknown strategy\n";
		}
	}
	return start;
}
