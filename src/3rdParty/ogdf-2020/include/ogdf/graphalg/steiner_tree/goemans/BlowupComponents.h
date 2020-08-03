/** \file
 * \brief Definition of the ogdf::steiner_tree::goemans::BlowupComponents class template
 *
 * \author Stephan Beyer
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

#include <ogdf/graphalg/steiner_tree/goemans/BlowupGraph.h>

//#define OGDF_STEINER_TREE_GOEMANS_BLOWUP_COMPONENTS_LOGGING

namespace ogdf {
namespace steiner_tree {
namespace goemans {

//! Obtain and provides information about components in a given blowup graph
template<typename T>
class BlowupComponents
{
protected:
	// To represent Gamma(X) [the set of all components in the blowup graph], we give
	//  - terminals, source and target the component id 0
	//  - all other nodes a component id > 0. Nodes with same id belong to the same component.
	// Note that this is also fine for 2-components with only one edge, since this
	// is a core edge and hence a dummy node is inserted.
	NodeArray<int> componentId;
	// We also save lists of terminals for each component in the specified array buffer.
	ArrayBuffer<ArrayBuffer<node>> componentTerminals;
	// To efficiently traverse the found component, we save the edge from the root of the component
	ArrayBuffer<edge> componentRootEdge;
	// Finally a component -> cost array.
	ArrayBuffer<T> componentCost;

	int maxId; // == the size of the arraybuffers

	//! Initialize all information about the component starting with \p rootEdge in the blowup graph.
	void initializeComponent(edge rootEdge, const BlowupGraph<T> &blowupGraph)
	{
		List<node> queue;
		const node start = rootEdge->target();
		queue.pushBack(start);
		componentRootEdge.push(rootEdge);
		componentTerminals.push(ArrayBuffer<node>());
		auto &terms = componentTerminals[maxId];
		if (!blowupGraph.getOriginal(start)) { // start node is a core edge
			componentCost.push(blowupGraph.getCost(rootEdge));
		} else {
			componentCost.push(0);
		}
		T &cost = componentCost[maxId];
		++maxId;
		while (!queue.empty()) {
			const node v = queue.popBackRet();
			componentId[v] = maxId;
			for (adjEntry adj : v->adjEntries) {
				const node w = adj->twinNode();
				if (componentId[w] < 0) {
					// count coreEdge cost only once
					if (blowupGraph.getOriginal(v)) { // v is no core edge
						cost += blowupGraph.getCost(adj->theEdge());
					}
					if (blowupGraph.isTerminal(w)) {
						terms.push(w);
					} else {
						queue.pushBack(w);
					}
				}
			}
		}
#ifdef OGDF_STEINER_TREE_GOEMANS_BLOWUP_COMPONENTS_LOGGING
		std::cout
		  << " * component with terminals " << terms
		  << " starting with edge " << rootEdge
		  << " having cost " << cost
		  << " and capacity " << blowupGraph.getCapacity(rootEdge) << std::endl;
#endif
	}

public:
	//! Find all components in the blowup graph and initialize information them
	BlowupComponents(const BlowupGraph<T> &blowupGraph)
	  : componentId(blowupGraph.getGraph(), -1)
	  , maxId(0)
	{
		componentId[blowupGraph.getSource()] = 0;
		componentId[blowupGraph.getPseudotarget()] = 0;
		componentId[blowupGraph.getTarget()] = 0;

#ifdef OGDF_STEINER_TREE_GOEMANS_BLOWUP_COMPONENTS_LOGGING
		std::cout << "Finding components in blowup graph:" << std::endl;
#endif
		for (node t : blowupGraph.terminals()) {
			for (adjEntry rootAdj : t->adjEntries) {
				const edge rootEdge = rootAdj->theEdge();
				if (rootEdge->source() != t) {
					continue;
				}
				const node r = rootAdj->twinNode();
				OGDF_ASSERT(r == rootEdge->target());
				if (componentId[r] < 0) {
					initializeComponent(rootEdge, blowupGraph);
				}
			}
		}

		// now set terminals to id 0   [XXX: why dont we keep -1?]
		for (node t : blowupGraph.terminals()) {
			componentId[t] = 0;
		}
	}

	//! Return list of terminals for a given component
	const ArrayBuffer<node> &terminals(int id) const
	{
		OGDF_ASSERT(id > 0);
		return componentTerminals[id-1];
	}

	//! Return the component id a given node is contained in
	int id(node v) const
	{
		return componentId[v];
	}

	//! Return total cost of a given component
	const T &cost(int id) const
	{
		OGDF_ASSERT(id > 0);
		return componentCost[id-1];
	}

	//! Return number of components
	int size() const
	{
		return maxId;
	}

	//! Return the edge coming from the root of a given component
	edge rootEdge(int id) const
	{
		OGDF_ASSERT(id > 0);
		return componentRootEdge[id-1];
	}

	//! Set the edge coming from the root for a given component
	void setRootEdge(int id, edge e) // beware of using!
	{
		OGDF_ASSERT(id > 0);
		componentRootEdge[id-1] = e;
		OGDF_ASSERT(componentTerminals[id-1].linearSearch(e->source()) != -1);
	}
};

}
}
}
