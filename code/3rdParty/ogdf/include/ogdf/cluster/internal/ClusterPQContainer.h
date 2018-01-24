/** \file
 * \brief Declaration of ClusterPQContainer.
 *
 * Stores information for a biconnected component
 * of a cluster for embedding the cluster in the
 * top down traversal
 *
 * \author Sebastian Leipert
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

#include <ogdf/planarity/booth_lueker/EmbedPQTree.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>

namespace ogdf {

class CconnectClusterPlanarEmbed;

namespace cluster_planarity {

class ClusterPQContainer {
	friend class ogdf::CconnectClusterPlanarEmbed;
	using InfoLeafPtr = booth_lueker::PlanarLeafKey<booth_lueker::IndInfo *> *;

	// Definition
	// incoming edge of v: an edge e = (v,w) with number(v) < number(w)

	// Stores for every node v the keys corresponding to the incoming edges of v
	NodeArray<SListPure<InfoLeafPtr>> *m_inLeaves;

	// Stores for every node v the keys corresponding to the outgoing edges of v
	NodeArray<SListPure<InfoLeafPtr>> *m_outLeaves;

	// Stores for every node v the sequence of incoming edges of v according
	// to the embedding
	NodeArray<SListPure<edge> >* m_frontier;

	// Stores for every node v the nodes corresponding to the
	// opposed sink indicators found in the frontier of v.
	NodeArray<SListPure<node> >* m_opposed;

	// Stores for every node v the nodes corresponding to the
	// non opposed sink indicators found in the frontier of v.
	NodeArray<SListPure<node> >* m_nonOpposed;

	// Table to acces for every edge its corresponding key in the PQTree
	EdgeArray<InfoLeafPtr> *m_edge2Key;

	// Stores for every node its st-number
	NodeArray<int> *m_numbering;

	// Stores for every st-number the node
	Array<node> *m_tableNumber2Node;

	node m_superSink;

	// the subgraph that contains the biconnected component
	// NOT THE COPY OF THE BICONNECTED COMPONENT THAT WAS CONSTRUCTED
	// DURING PLANARITY TESTING. THIS HAS BEEN DELETED.
	Graph					*m_subGraph;
	// corresponding PQTree
	booth_lueker::EmbedPQTree *m_T;
	// The leaf correpsonding to the edge (s,t).
	InfoLeafPtr m_stEdgeLeaf;

public:

	ClusterPQContainer():
		m_inLeaves(nullptr),m_outLeaves(nullptr),m_frontier(nullptr),
		m_opposed(nullptr),m_nonOpposed(nullptr),m_edge2Key(nullptr),
		m_numbering(nullptr),m_tableNumber2Node(nullptr),
		m_superSink(nullptr),m_subGraph(nullptr),m_T(nullptr), m_stEdgeLeaf(nullptr) { }

	~ClusterPQContainer() { }

	void init(Graph *subGraph){
		m_subGraph = subGraph;
		m_inLeaves
			= new NodeArray<SListPure<InfoLeafPtr>>(*subGraph);

		m_outLeaves
			= new NodeArray<SListPure<InfoLeafPtr>>(*subGraph);

		m_frontier
			= new NodeArray<SListPure<edge>>(*subGraph);

		m_opposed
			= new NodeArray<SListPure<node>>(*subGraph);

		m_nonOpposed
			= new NodeArray<SListPure<node>>(*subGraph);

		m_edge2Key
			= new EdgeArray<InfoLeafPtr>(*subGraph);

		m_numbering
			= new NodeArray<int >(*subGraph);

		m_tableNumber2Node
			= new Array<node>(subGraph->numberOfNodes()+1);
	}

	void Cleanup() {
		delete m_inLeaves;
		if (m_outLeaves)
		{
			for(node v : m_subGraph->nodes)
			{
				while (!(*m_outLeaves)[v].empty())
				{
					InfoLeafPtr L = (*m_outLeaves)[v].popFrontRet();
					delete L;
				}
			}
			delete m_outLeaves;
		}
		delete m_frontier;
		delete m_opposed;
		delete m_nonOpposed;
		delete m_edge2Key;
		if (m_T)
		{
			m_T->emptyAllPertinentNodes();
			delete m_T;
		}
		delete m_numbering;
		delete m_tableNumber2Node;

	}
};

}
}
