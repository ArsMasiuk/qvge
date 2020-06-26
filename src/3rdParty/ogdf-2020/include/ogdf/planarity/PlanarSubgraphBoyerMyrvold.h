/** \file
 * \brief Declaration of the subgraph wrapper class of the Boyer-Myrvold planarity test
 *
 * \author Tilo Wiedera
 *
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

#include <random>
#include <ogdf/planarity/PlanarSubgraphModule.h>
#include <ogdf/planarity/boyer_myrvold/BoyerMyrvoldPlanar.h>
#include <ogdf/planarity/BoyerMyrvold.h>
#include <ogdf/planarity/BoothLueker.h>

namespace ogdf {

//! Maximum planar subgraph heuristic based on the Boyer-Myrvold planarity test.
/**
 * Computes a (possibly non-maximal) planar subgraph using the ogdf::BoyerMyrvold planarity test.
 * Runs in linear time.
 *
 * @ingroup ga-plansub
 */
class OGDF_EXPORT PlanarSubgraphBoyerMyrvold : public PlanarSubgraphModule<int>
{
private:
	int m_runs;
	double m_randomness;
	BoothLueker m_planModule;
	std::minstd_rand m_rand;

public:
	/**
	 * Creates a new Boyer-Myrvold subgraph module.
	 *
	 * @param runs The number of times to start the algorithm, the greatest found subgraph is returned
	 * @param randomness If randomness equals 1, each edge is chosen with the same probability.
	 *                   A randomness of zero chooses each edge according to its cost.
	 *                   Any value between 0 and 1 is allowed and will result in a specific random influence.
	 *                   When performing multiple runs, a randomness greater zero should be chosen.
	 */
	explicit PlanarSubgraphBoyerMyrvold(int runs = 1, double randomness = 0) :
	    m_runs(runs),
	    m_randomness(randomness),
	    m_rand(rand())
	{};

	~PlanarSubgraphBoyerMyrvold() {};

	virtual PlanarSubgraphBoyerMyrvold *clone() const override {
		return new PlanarSubgraphBoyerMyrvold(m_runs);
	};

	//! Seeds the random generator for performing a random DFS.
	//! If this method is never called the random generator will be seeded by a value
	//! extracted from the global random generator.
	void seed(std::minstd_rand rand) {
		m_rand = rand;
	}

protected:
	/**
	 * Constructs a planar subgraph according to the options supplied to the constructor.
	 *
	 * @param graph the Graph to be planarized
	 * @param preferedEdges ignored
	 * @param delEdges will contain the list of edges that can be deleted to achieve planarity
	 * @param pCost the costs for removing each edge
	 * @param preferedImplyPlanar ignored
	 */
	virtual ReturnType doCall(
		const Graph &graph,
		const List<edge> &preferedEdges,
		List<edge> &delEdges,
		const EdgeArray<int> *pCost,
		bool preferedImplyPlanar) override;

	/**
	 * Returns true iff this edge could not be embedded.
	 */
	bool isRemoved(const GraphCopy &copy, const edge e) {
		return copy.copy(e) == nullptr || copy.copy(e)->source() != copy.copy(e->source()) || copy.copy(e)->target() != copy.copy(e->target());
	}
};

}
