/** \file
 * \brief Definition of the FullComponentGeneratorCaller class template
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

#include <ogdf/graphalg/MinSteinerTreeModule.h>
#include <ogdf/graphalg/steiner_tree/FullComponentDecisions.h>

namespace ogdf {
namespace steiner_tree {

template<typename T>
class FullComponentGeneratorCaller {
public:
	//! Computes distance and predecessor matrix
	static void computeDistanceMatrix(NodeArray<NodeArray<T>>& distance, NodeArray<NodeArray<edge>>& pred,
			const EdgeWeightedGraph<T>& graph,
			const List<node>& terminals,
			const NodeArray<bool>& isTerminal,
			int restricted);
};

template<typename T>
void FullComponentGeneratorCaller<T>::computeDistanceMatrix(NodeArray<NodeArray<T>>& distance, NodeArray<NodeArray<edge>>& pred,
		const EdgeWeightedGraph<T>& graph,
		const List<node>& terminals,
		const NodeArray<bool>& isTerminal,
		int restricted) {
	if (steiner_tree::FullComponentDecisions::shouldUseDijkstra(restricted, graph.numberOfNodes(), graph.numberOfEdges(), terminals.size())) {
		if (restricted <= 3) {
			// for 2- and 3-restricted computations, it is ok to use SSSP from all terminals
			MinSteinerTreeModule<T>::allTerminalShortestPaths(graph, terminals, isTerminal, distance, pred);
		} else {
			MinSteinerTreeModule<T>::allNodeShortestPaths(graph, terminals, isTerminal, distance, pred);
		}
	} else {
		MinSteinerTreeModule<T>::allPairShortestPaths(graph, isTerminal, distance, pred);
	}
}

}
}
