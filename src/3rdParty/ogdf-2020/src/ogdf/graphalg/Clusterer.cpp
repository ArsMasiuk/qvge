/** \file
 * \brief Implementation of a clustering algorithm (by Auber, Chiricota,
 * Melancon).
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

#include <ogdf/graphalg/Clusterer.h>
#include <ogdf/basic/GraphCopy.h>


namespace ogdf {

Clusterer::Clusterer(const Graph &G) :
	ClustererModule(G),
	m_recursive(true),
	m_autoThreshNum(0)
{
	//use automatic thresholds
#if 0
	m_thresholds.pushFront(3.0);
	m_thresholds.pushFront(1.4);
#endif
	m_defaultThresholds.pushFront(1.6);
	m_defaultThresholds.pushBack(3.2);
	m_defaultThresholds.pushBack(4.5);
	m_stopIndex = 0.7;
}


//computes a list of SimpleCluster pointers, where each SimpleCluster
//models a cluster and has a parent (0 if root)
//list of cluster structures will be parameter
void Clusterer::computeClustering(SList<SimpleCluster*> &clustering)
{
	//save to which cluster the vertices belong
	//0 is root cluster
	SimpleCluster *root = new SimpleCluster();
	root->m_size = m_pGraph->numberOfNodes();

	//we push root to the front afterwards
#if 0
	clustering.pushFront(root);
#endif
	NodeArray<SimpleCluster*> vCluster(*m_pGraph, root);

	//we delete edges and therefore work on a copy
	GraphCopy GC(*m_pGraph);
	//multiedges are not used for the computation
	makeSimple(GC);
	//based on strengths, we compute a clustering
	//Case 1:with depth one (or #thresholds)
	//Case 2: Recursively

	ListIterator<double> it;

	if (m_thresholds.size() > 0)
		it = m_thresholds.begin();
	else
	{
		OGDF_ASSERT(m_defaultThresholds.size() > 0);
		it = m_defaultThresholds.begin();
	}

	//are there more cases than (not) rec.?
	int fall = (m_recursive ? 2 : 1);
#if 0
	int fall = 2;
#endif
	if (fall == 2)
	{
		//we just use the first value of auto/default/threshs
#if 0
		OGDF_ASSERT(m_thresholds.size() == 1);
#endif
		//Case2:
		//we compute the edge strengths on the components recursively
		//we therefore only need a single threshold

		//we compute the edge strengths
		EdgeArray<double> strengths(GC,0.0);

		//we need a criterion to stop the process
		//we stop if there are no edges deleted or
		//the average node clustering index in our copy rises
		//above m_stopIndex
		bool edgesDeleted = true;

		while (edgesDeleted && (averageCIndex(GC)<m_stopIndex))
		{
			computeEdgeStrengths(GC, strengths);
			if (m_autoThreshNum > 0)
			{
				OGDF_ASSERT(m_autoThresholds.size() > 0);
				it = m_autoThresholds.begin();
			}
			if (it.valid())
			{
				//collect edges that will be deleted
				List<edge> le;
				for(edge e : GC.edges)
				{
					if (strengths[e] < *it)
					{
						le.pushFront(e);
					}
				}
				//stop criterion
				if (le.size() == 0)
				{
					edgesDeleted = false;
					continue;
				}
				//delete edges
				for(edge e : le)
					GC.delEdge(e);

				//gather cluster information
				//vertices within a connected component are always part
				//of the same cluster which then will be parent of the new clusters

				ArrayBuffer<node> S;
				NodeArray<bool> done(GC, false);
				for(node v : GC.nodes)
				{
					if (done[v]) continue;
					done[v] = true;
					S.push(v);
					SimpleCluster* parent = vCluster[GC.original(v)];
					SList<node> theCluster;

					List<node> compNodes; //nodes in a component

					while(!S.empty())
					{
						node w = S.popRet();
						compNodes.pushFront(w);
						for(adjEntry adj : w->adjEntries) {
							node x = adj->twinNode();
							if (!(done[x]))
							{
								done[x] = true;
								S.push(x);
							}
						}
					}
					//run over nodes and set cluster info
					//do not construct trivial clusters
					if ( (parent->m_size > compNodes.size()) &&
						(compNodes.size()>2))
					{
						SimpleCluster* s = new SimpleCluster();
						s->setParent(parent);

						parent->pushBackChild(s);
						clustering.pushFront(s);
						for (node vi : compNodes)
						{
							vCluster[GC.original(vi)] = s;
							//vertex leaves parent to s
							parent->m_size--;
							s->m_size++;
						}
					}
				}
			}
		}
	} else { // case 2
		//we compute the edge strengths
		EdgeArray<double> strengths(*m_pGraph,0.0);
		computeEdgeStrengths(strengths);
		if (m_autoThreshNum > 0)
		{
			OGDF_ASSERT(m_autoThresholds.size() > 0);
			it = m_autoThresholds.begin();
		}
		//Case1:

		while (it.valid())
		{
			//collect edges that will be deleted
			List<edge> le;
			for(edge e : GC.edges)
			{
				if (strengths[e] < *it)
				{
					le.pushFront(e);
				}
			}
			//delete edges
			for(edge e : le)
				GC.delEdge(e);

			//gather cluster information
			//vertices within a connected component are always part
			//of the same cluster which then will be parent of the new clusters

			ArrayBuffer<node> S;
			NodeArray<bool> done(GC, false);
			for(node v : GC.nodes)
			{
				if (done[v]) continue;
				done[v] = true;
				S.push(v);
				SimpleCluster* parent = vCluster[GC.original(v)];
				SList<node> theCluster;

				List<node> compNodes; //nodes in a component

				//do not immediately construct a cluster, first gather and store
				//vertices, then test if the cluster fits some constraints, e.g. size
				while(!S.empty())
				{
					node w = S.popRet();
					compNodes.pushFront(w);
					for(adjEntry adj : w->adjEntries) {
						node x = adj->twinNode();
						if (!(done[x]))
						{
							done[x] = true;
							S.push(x);
						}
					}
				}

				//run over nodes and set cluster info
				//do not construct trivial clusters
				if ( (parent->m_size > compNodes.size()) &&
					(compNodes.size()>2))
				{
					SimpleCluster* s = new SimpleCluster;
					s->setParent(parent);
#if 0
					s->m_index = -1;
#endif
					parent->pushBackChild(s);
					clustering.pushFront(s);
					for(node vi : compNodes)
					{
						vCluster[GC.original(vi)] = s;
						//vertex leaves parent to s
						parent->m_size--;
						s->m_size++;
					}
				}
			}

			++it;
		}
	}

	clustering.pushFront(root);
	//we now have the clustering and know for each vertex, to which cluster
	//it was assigned in the last iteration (vCluster)
	//we update the lists of children
	for(node v : m_pGraph->nodes)
	{
		vCluster[v]->pushBackVertex(v);
	}
}

void Clusterer::computeEdgeStrengths(EdgeArray<double> &strength)
{
	const Graph &G = *m_pGraph;
	computeEdgeStrengths(G, strength);
}

void Clusterer::computeEdgeStrengths(const Graph &G, EdgeArray<double> &strength)
{
	strength.init(G, 0.0);
	double minStrength = 5.0, maxStrength = 0.0; //used to derive automatic thresholds
	//5 is the maximum possible value (sum of five values 0-1)

	//A Kompromiss: Entweder immer Nachbarn der Nachbarn oder einmal berechnen und speichern (gut
	//wenn haeufig benoetigt (hoher Grad), braucht aber viel Platz
	//B Was ist schneller: Listen nachher nochmal durchlaufen und loeschen, wenn Doppelnachbar oder
	//gleich beim Auftreten loeschen ueber Iterator

	//First, compute the sets Mw, Mv, Wwv. Then check their connectivity.
	//Use a list for the vertices that are solely connected to w
	for(edge e : G.edges)
	{
		List<node> vNb;
		List<node> wNb;
		List<node> bNb; //neighbour to both vertices
		EdgeArray<bool> processed(G, false); //edge was processed
		NodeArray<int> nba(G, 0); //neighbours of v and w: 1v, 2both, 3w

		node v = e->source();
		node w = e->target();

		//neighbourhood sizes
		int sizeMv = 0;
		int sizeMw = 0;
		int sizeWvw = 0;
		//neighbourhood links
#if 0
		int rMv = 0; //within MV
		int rMw = 0;
#endif
		int rWvw = 0;

		int rMvMw = 0; //from Mv to Mw
		int rMvWvw = 0;
		int rMwWvw = 0;

		//Compute neighbourhood
		//Muss man selfloops gesondert beruecksichtigen
		for(adjEntry adjE : v->adjEntries)
		{
			node u = adjE->twinNode();
			if (u == v) continue;
			if ( u != w )
			{
				nba[u] = 1;
			}
		}
		for(adjEntry adjE : w->adjEntries)
		{
			node u = adjE->twinNode();
			if (u == w) continue;
			if ( u != v )
			{
				if (nba[u] == 1)
				{
					nba[u] = 2;
				}
				else
				{
					if (nba[u] != 2) nba[u] = 3;

					sizeMw++;
					wNb.pushFront(u);
				}
			}
		}

		//Problem in der Laufzeit ist die paarweise Bewertung der Nachbarschaft
		//ohne Nutzung vorheriger Informationen

		//We know the neighbourhood of v and w and have to compute the connectivity
		for(adjEntry adjE : v->adjEntries)
		{
			node u = adjE->twinNode();

			if ( u != w )
			{
				//check if u is in Mv
				if (nba[u] == 1)
				{
					//vertex in Mv
					sizeMv++;
					//check links within Mv, to Mw and Wvw
					for(adjEntry adjE2 : u->adjEntries)
					{
						processed[adjE2->theEdge()] = true;
						node t = adjE2->twinNode();
						//test links to other sets
						switch (nba[t]) {
#if 0
						case 1: rMv++; break;
#endif
						case 2: rMvWvw++; break;
						case 3: rMvMw++; break;
						}
					}
				} else {
					//vertex in Wvw, nba == 2
					OGDF_ASSERT(nba[u] == 2);
					sizeWvw++;
					for(adjEntry adjE2 : u->adjEntries)
					{
						node t = adjE2->twinNode();
						//processed testen?
						//test links to other sets
						switch (nba[t]) {
#if 0
						case 1: rMv++; break;
#endif
						case 2: rWvw++; break;
						case 3: rMwWvw++; break;
						}
					}
				}
			}
		}

		//Now compute the ratio of existing edges to maximal number
		//(complete graph)

		double sMvWvw = 0.0;
		double sMwWvw = 0.0;
		double sWvw = 0.0;
		double sMvMw = 0.0;
		//we have to cope with special cases
		int smult = sizeMv*sizeWvw;
		if (smult != 0) sMvWvw = (double)rMvWvw/smult;
		smult = sizeMw*sizeWvw;
		if (smult != 0) sMwWvw = (double)rMwWvw/smult;
		smult = (sizeMv*sizeMw);
		if (smult != 0) sMvMw = (double)rMvMw/smult;


		if (sizeWvw > 1)
			sWvw   = 2.0*rWvw/(sizeWvw*(sizeWvw-1));
		else if (sizeWvw == 1) sWvw = 1.0;
		//Ratio of cycles of size 3 and 4
		double cycleProportion = ((double)sizeWvw/(sizeWvw+sizeMv+sizeMw));
		double edgeStrength = sMvWvw+sMwWvw+sWvw+sMvMw+cycleProportion;

		if (m_autoThreshNum > 0)
		{
			if (minStrength > edgeStrength) minStrength = edgeStrength;
			if (maxStrength < edgeStrength) maxStrength = edgeStrength;
		}

#if 0
		std::cout<<"sWerte: "<<sMvWvw<<"/"<<sMwWvw<<"/"<<sWvw<<"/"<<sMvMw<<"\n";
		std::cout << "CycleProportion "<<cycleProportion<<"\n";
		std::cout << "EdgeStrength "<<edgeStrength<<"\n";
#endif
		strength[e] = edgeStrength;

		//true neighbours are not adjacent to v
#if 0
		std::cout << "Checking true neighbours of w\n";
#endif

	}
	if (m_autoThreshNum > 0)
	{
		if (m_autoThresholds.size()>0) m_autoThresholds.clear();
		if (maxStrength > minStrength)
		{
#if 0
			std::cout << "Max: "<<maxStrength<< " Min: "<<minStrength<<"\n";
#endif
			double step = (maxStrength-minStrength)/((double)m_autoThreshNum+1.0);
			double val = minStrength+step;
			for (int i = 0; i<m_autoThreshNum; i++)
			{
				m_autoThresholds.pushBack(val);
				val += step;
			}
		} else {
			m_autoThresholds.pushBack(maxStrength); // Stops computation
		}
	}
}

//computes clustering and translates the computed structure into C
void Clusterer::createClusterGraph(ClusterGraph &C)
{
	OGDF_ASSERT(&(C.constGraph()) == m_pGraph);
	//clear existing entries
	C.clear();

	//we compute the edge strengths
	EdgeArray<double> strengths(*m_pGraph,0.0);
	computeEdgeStrengths(strengths);

	//based on strengths, we compute a clustering
	//Case 1:with depth one (or #thresholds)
	//Case 2: Recursively

	//Case1:
	GraphCopy GC(*m_pGraph);
	ListIterator<double> it = m_thresholds.begin();

	while (it.valid())
	{
		//collect edges that will be deleted
		List<edge> le;
		for(edge e : GC.edges)
		{
			if (strengths[e] < *it)
			{
				le.pushFront(e);
			}
		}
		//delete edges
		for (edge e : le)
			GC.delEdge(e);

		//gather cluster information
		//vertices within a connected component are always part
		//of the same cluster which then will be parent of the new clusters

		ArrayBuffer<node> S;
		NodeArray<bool> done(GC, false);
		for(node v : GC.nodes)
		{
			if (done[v]) continue;
			done[v] = true;
			S.push(v);
			cluster parent = C.clusterOf(GC.original(v));
			SList<node> theCluster;

			while(!S.empty())
			{
				node w = S.popRet();
				theCluster.pushFront(GC.original(w));
				for(adjEntry adj : w->adjEntries) {
					node x = adj->twinNode();
					if (!(done[x]))
					{
						done[x] = true;
						S.push(x);
					}
				}
			}
			//create the cluster
			C.createCluster(theCluster, parent);
		}

		++it;
	}
}

void Clusterer::setClusteringThresholds(const List<double> &threshs)
{
	//we copy the values, should be a low number
	m_thresholds.clear();

	for (double x : threshs)
		m_thresholds.pushFront(x);
}

}
