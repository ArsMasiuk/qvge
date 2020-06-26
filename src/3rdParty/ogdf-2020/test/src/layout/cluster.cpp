/** \file
 * \brief Tests for layout algorithms for cluster graphs.
 *
 * \author Tilo Wiedera
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

#include <ogdf/basic/LayoutModule.h>
#include <ogdf/cluster/ClusterPlanarizationLayout.h>
#include <ogdf/cluster/ClusterOrthoLayout.h>

#include "layout_helpers.h"

//! Looks like a regular LayoutModule but creates a ClusterGraph that is handed to
//! a ClusterPlanarizationLayout instead.
class CPLMock : public LayoutModule {
	ClusterPlanarizationLayout clusterPlanarizationLayout;

public:
	virtual void call(GraphAttributes &attr) override {
		GraphCopy G(attr.constGraph());
		ClusterGraph C(G);
		ClusterGraphAttributes cAttr(C);
		bool originalEmpty = G.numberOfNodes() == 0;

		// add clique cluster
		SList<node> nodes;
		node nodeInClique = G.newNode();
		nodes.pushBack(nodeInClique);

		for(int i = 0; i < 10; i++) {
			node w = G.newNode();
			for(node v : nodes) {
				G.newEdge(v, w);
			}
			nodes.pushBack(w);
		}
		C.createCluster(nodes, C.firstCluster());

		// add path cluster
		nodes.clear();
		nodes.pushBack(G.newNode());
		for(int i = 0; i < 10; i++) {
			node w = G.newNode();
			G.newEdge(nodes.back(), w);
			nodes.pushBack(w);
		}

		// connect it all
		G.newEdge(nodeInClique, nodes.front());
		G.newEdge(nodeInClique, nodes.back());
		if (!originalEmpty) {
			G.newEdge(nodeInClique, G.firstNode());
		}

		clusterPlanarizationLayout.call(G, cAttr, C);

		for (node v : G.nodes) {
			if(!G.isDummy(v)) {
				attr.x(G.original(v)) = cAttr.x(v);
				attr.y(G.original(v)) = cAttr.y(v);
			}
		}

		for (edge e : G.edges) {
			if(!G.isDummy(e)) {
				attr.bends(G.original(e)) = cAttr.bends(e);
			}
		}
	}
};

go_bandit([] {
	describeLayout<CPLMock>("ClusterPlanarizationLayout", 0, {GraphProperty::connected, GraphProperty::sparse, GraphProperty::simple}, true, GraphSizes(16, 32, 16));
});
