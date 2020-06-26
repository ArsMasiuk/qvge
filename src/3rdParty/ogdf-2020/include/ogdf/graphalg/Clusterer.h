/** \file
 * \brief Declaration of Clusterer class that computes a clustering
 *        for a given graph based on the local neighborhood
 *        structure of each edge. Uses the criteria by
 *        Auber, Chiricota, Melancon for small-world graphs to
 *        compute clustering index and edge strength.
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

#include <ogdf/graphalg/ClustererModule.h>

namespace ogdf {

/**
 * @ingroup ga-clustering
 *
 * Clustering is determined based on the threshold values (connectivity
 * thresholds determine edges to be deleted) and stopped if average
 * clustering index drops below m_stopIndex.
 *
 * \pre Input graph has to be connected
 */
class OGDF_EXPORT Clusterer : public ClustererModule
{
public:
	//! Constructor taking a graph G to be clustered
	explicit Clusterer(const Graph &G);

	/**Default constructor allowing to cluster multiple
	*graphs with the same instance of the Clusterer
	*graphs */
	Clusterer();

	virtual ~Clusterer() {}

	//The clustering can be done recursively (use single threshold
	//on component to delete weak edges (recompute strengths)) or
	//by applying a set of thresholds, set the behaviour in
	//function setRecursive
	virtual void computeClustering(SList<SimpleCluster*> &sl) override;

	//set the thresholds defining the hierarchy assignment decision
	//should be dependent on the used metrics
	void setClusteringThresholds(const List<double> &threshs);

	//thresholds are computed from edge strengths to split off
	//at least some edges as long as there is a difference between
	//min and max strength (progressive clustering)
	//set this value to 0 to use your own or the default values
	void setAutomaticThresholds(int numValues)
	{
		m_autoThreshNum = numValues;
	}

	//for recursive clustering, only the first threshold is used
	void setRecursive(bool b) { m_recursive = b; }

	//preliminary
	void computeEdgeStrengths(EdgeArray<double> & strength);
	void computeEdgeStrengths(const Graph &G, EdgeArray<double> & strength);

	virtual void createClusterGraph(ClusterGraph &C) override;

	void setStopIndex(double stop) { m_stopIndex = stop; }

	//compute a clustering index for node v
	//number of connections in neighborhood compared to clique
	virtual double computeCIndex(node v) override
	{
		return computeCIndex(*m_pGraph, v);
	}

	virtual double computeCIndex(const Graph &G, node v) override
	{
		OGDF_ASSERT(v->graphOf() == &G);
		if (v->degree() < 2) return 1.0;
		int conns = 0; //connections, without v
		NodeArray<bool> neighbor(G, false);
		for (adjEntry adjE : v->adjEntries)
		{
			neighbor[adjE->twinNode()] = true;
		}
		for (adjEntry adjE : v->adjEntries)
		{
			for (adjEntry adjEE : adjE->twinNode()->adjEntries)
			{
				if (neighbor[adjEE->twinNode()])
					conns++;
			}
		}
		//connections were counted twice
		double index = conns / 2.0;
		return index / (v->degree()*(v->degree() - 1));
	}

protected:
	EdgeArray<double> m_edgeValue; //strength value for edge clustering index
	NodeArray<double> m_vertexValue; //clustering index for vertices
	List<double> m_thresholds; //clustering level thresholds
	List<double> m_autoThresholds; //automatically generated values (dep. on graph instance)
	List<double> m_defaultThresholds; //some default values
	double m_stopIndex; //average clustering index when recursive clustering stops
	//between 0 and 1
	bool m_recursive; //recursive clustering or list of tresholds
#if 0
	bool m_autoThresholds; //compute thresholds according to edge strengths
#endif
	int m_autoThreshNum; //number of thresholds to be computed
};

}
