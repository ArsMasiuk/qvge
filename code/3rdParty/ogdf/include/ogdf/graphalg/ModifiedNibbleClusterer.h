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

#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/geometry.h>
#include <vector>

namespace ogdf {

//! The modified nibble clustering algorithm.
/**
 * @ingroup ga-clustering
 *
 * Modified Nibble Clustering Algorithm (as given in Graph Clustering for Keyword Search.
 * R. Catherine, S. Sudarshan).
 * The algorithm is very fast (and thus suited for huge graphs) and simple to implement, however not very accurate.
 *
 * State: Experimental - Only use when you know what you are doing
 *
 * To be used in remainders of graph decomposition for clustering
 * (Remove trees first, then use BC, SPQR,)
 */
class ModifiedNibbleClusterer {

public:
	ModifiedNibbleClusterer()
	  : m_clusterThreshold(3)
	  , m_maxClusterSize(100)
	  , maxClusterNum(100)
	  , m_spreadProbability(0.8)
	  , m_pG(nullptr)
	  , m_pGC(nullptr)
	  , m_sns(StartNodeStrategy::MaxDeg)
	{
	}

	~ModifiedNibbleClusterer()
	{
#if 0
		delete m_pGC;
#endif
	}

	//TODO add a call with GraphAttributes and store in intweight

	//! Call method: Creates a clustering of G
	//! Returns number of created clusters and sets cluster index for each node in clusterNum
	long call(Graph &G, NodeArray<long> &clusterNum);

	//! A convenience method. Due to the characteristics of the algorithm (not very accurate,
	//! fast for large graphs), we could have a medium number (several hundreds) of clusters,
	//! and could need a further level of clustering. On the other hand, fully recursive
	//! clustering does not make much sense as after a second level there will be not to many
	//! clusters left.
	//! topLevelNum keeps a cluster number in the top level of the two level cluster hierarchy
	long call(Graph &G, NodeArray<long> &clusterNum, NodeArray<long> &topLevelNum);

	//! Call method: Creates a clustering of G in C
	//! Returns number of created clusters
	// TODO long call(ClusterGraph &C, Graph & G) {}
	void setMaxClusterNum(int i) {maxClusterNum = i;}
	void setMaxClusterSize(long i) {m_maxClusterSize = i;}
	// Smaller clusters are joint with a neighbor (non-recursive) as a postprocessing
	void setClusterSizeThreshold(int threshold)
	{
		if (threshold > 0) {
			m_clusterThreshold = threshold;
		}
	}

	enum class StartNodeStrategy {MinDeg, MaxDeg, Random};

protected:
	void initialize(); //1< Initialize values for calculation before first step
	node selectStartNode(); //!< select start node according to some strategy
	void modifiedNibble(node snode, std::vector<node> &bestCluster); //!< main step with walks starting from snode
	double findBestCluster(NodeArray<bool> &isActive, std::vector<node> &activeNodes, std::vector<node> &cluster);
	void spreadValues(NodeArray<bool> &isActive, std::vector<node> &activeNodes, NodeArray<double> & probUpdate);

	inline long maxClusterSize() {
		return m_maxClusterSize;
	}

	// Arithmetic plus Geometric Progression
	int aPGP(int i)
	{
		const int a = 2;
		const int d = 7;
		const double r = 1.5;
		return static_cast<int>(ceil(a * pot(r, i))) + i*d + a;
	}

	// TODO we expect i to be very small, but do this efficiently anyway
	double pot(double r, long i)
	{
		return pow(r, static_cast<double>(i));
	}

	long activeNodeBound()
	{
		const int factor = 25; //!< f in publication Rose Catherine K., S. Sudarshan
		//does not make sense to set it to 500 as they did, as we want less clusters.
		return factor*m_maxClusterSize;
	}

	void postProcess();

private:
	int m_steps;
	int m_clusterThreshold; // Below that size all remaining nodes are just packed in a cluster
	long m_maxClusterSize;
	long m_maxActiveNodes; // Bound on number of nodes in active set
	int maxClusterNum;
	double m_spreadProbability; //how much is spread, i.e. 1-val is the prob to stay at the node
	node m_startNode; // Node to start a walk from
	NodeArray<double> m_prob; // Probability of a node along the walk
	Graph* m_pG;
	GraphCopy* m_pGC;
	StartNodeStrategy m_sns;

	// Tests
	bool testSpreadSum()
	{
		double sum = 0.0;
		for(node v : m_pGC->nodes) {
			sum += m_prob[v];
		}
		return OGDF_GEOM_ET.equal(sum, 1.0);
	}
};

}
