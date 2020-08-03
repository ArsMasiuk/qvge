/** \file
 * \brief Implements graph generator for random geographical threshold graphs
 *
 * \author Jöran Schierbaum
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

namespace ogdf {

/**
 * @addtogroup graph-generators
 * @{
 */

//! @name Randomized graph generators
//! @{

//! Creates a random geometric graph where edges are created based on their distance and the weight of nodes
/**
 * Geographical threshold graphs with small-world and scale-free properties
 * Naoki Masuda, Hiroyoshi Miwa, Norio Konno
 * https://arxiv.org/abs/cond-mat/0409378
 *
 * Distribute vertices using an exponential distribution in a d-dimensional Euclidean space.
 * Then a pair of vertices with weights \a w,\a w' and Euclidean distance \f$r:=||w-w'||\f$ are connected iff
 * for the heuristic function \p h holds:
 *  \f$(w+w')*h(r) < \mathrm{threshold}\f$.
 *
 * @tparam D the random distribution to use (see \p dist).
 * @param G is assigned the generated graph.
 * @param weights has the weights for all nodes in the graph.
 * @param dist is a random number distribution, e.g. \c std::uniform_int_distribution<>.
 *             It should likely generate values in roughly the same order of magnitude as \p h(\p threshold).
 * @param threshold is the threshold for edge insertion.
 * @param h is a function that should be decreasing in the distance supplied to it.
 * @param dimension is the dimension the nodes are laid out in.
 */
template<typename D>
void randomGeographicalThresholdGraph(Graph &G, Array<int> &weights, D &dist, double threshold, std::function<double(double)> h, int dimension = 2)
{
	OGDF_ASSERT(dimension >= 1);
	OGDF_ASSERT(threshold >= 0.0);

	G.clear();
	Array<node> nodes(weights.size());

	// Randomly distribute nodes in a d-dimensional area
	NodeArray<int> nodeWeights = NodeArray<int>(G);
	NodeArray<Array<double>> cord(G, Array<double>(dimension));
	std::minstd_rand rng(randomSeed());
	for (int i = 0; i < weights.size(); i++) {
		nodes[i] = G.newNode();
		nodeWeights[nodes[i]] = weights[i];
		for (int j = 0; j < dimension; j++){
			cord[nodes[i]][j] = dist(rng);
		}
	}

	for (node v : nodes) {
		for (node w = v->succ(); w; w = w->succ()) {
			double distance = 0.0;
			for (int i = 0; i < dimension; i++) {
				distance += (cord[v][i] - cord[w][i]) * (cord[v][i] - cord[w][i]);
			}
			distance = sqrt(distance);

			if ((nodeWeights[v] + nodeWeights[w]) * h(distance) > threshold) {
				G.newEdge(v, w);
			}
		}
	}
}

/**
 * @copybrief ::randomGeographicalThresholdGraph(Graph &G, Array<int> &weights, D &dist, double threshold, std::function<double(double)> h, int dimension)
 *
 * This generator uses \f$r^{-\alpha}\f$ for the given \p alpha as heuristic function.
 *
 * @see ::randomGeographicalThresholdGraph(Graph &G, Array<int> &weights, D &dist, double threshold, std::function<double(double)> h, int dimension)
 *      for detailed description.
 *
 * @tparam D the random distribution to use (see \p dist).
 * @param G is assigned the generated graph.
 * @param weights has the weights for all nodes in the graph.
 * @param dist is a random number distribution, e.g. \c std::uniform_int_distribution<>.
 *             It should likely generate values in roughly the same order of magnitude as \c 1/\p threshold.
 * @param threshold is the threshold for edge insertion.
 * @param alpha is the constant in the heuristic function.
 * @param dimension is the dimension the nodes are laid out in.
 */
template<typename D>
void randomGeographicalThresholdGraph(Graph &G, Array<int> &weights, D &dist, double threshold, int alpha = 2, int dimension = 2)
{
	randomGeographicalThresholdGraph<D>(G, weights, dist, threshold, [alpha](double d){ return 1/pow(d, alpha); }, dimension);
}

//! @}

/** @} */

}
