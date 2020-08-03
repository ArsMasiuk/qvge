/** \file
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

#include <ogdf/energybased/dtree/DTreeEmbedder.h>
#include <ogdf/energybased/dtree/GalaxyLevel.h>
#include <ogdf/basic/LayoutModule.h>
#include <ogdf/basic/simple_graph_alg.h>

namespace ogdf {

template<int Dim>
class DTreeMultilevelEmbedder
{
public:
	struct NodeCoords {
		double coords[Dim];
	};

	//! constructor with a given graph, allocates memory and does initialization
	DTreeMultilevelEmbedder() {
		m_useMultilevelWeights = true;
		m_levelMaxNumNodes = 10;

		if (m_useMultilevelWeights) {
			m_scaleFactorPerLevel = 1.71;
		} else {
			m_scaleFactorPerLevel = 3.71;//3.71;//3.71;
		}

		m_maxIterationsPerLevel = 1000;
		m_minIterationsPerLevel = 50;
#if 0
		m_numIterationsFinestLevel = 200;
#endif
		m_numIterationsFinestLevel = 50;
		m_numIterationsFactorPerLevel = 1.0;

		m_thresholdFinestLevel = 0.0002;
		m_thresholdFactorPerLevel = 0.8;

		m_numIterationsCoarsestLevel = 1000;
		m_thresholdCoarsestLevel = 0.0;
	}

	//! call the multilevel embedder layout for graph, the result is stored in coords
	void call(const Graph& graph, NodeArray<NodeCoords>& coords);

private:
	int m_maxIterationsPerLevel;
	int m_minIterationsPerLevel;
	bool m_useMultilevelWeights;
	int m_numIterationsFinestLevel;
	double m_numIterationsFactorPerLevel;
	double m_thresholdFinestLevel;
	double m_thresholdFactorPerLevel;
	int m_numIterationsCoarsestLevel;
	double m_thresholdCoarsestLevel;

	int m_levelMaxNumNodes;
	double m_scaleFactorPerLevel;
};

class DTreeMultilevelEmbedder2D : public DTreeMultilevelEmbedder<2>, public LayoutModule
{
public:
	void call(GraphAttributes& GA) override
	{
		// the graph
		const Graph& G = GA.constGraph();

		// the input for the general d dim class
		NodeArray<DTreeMultilevelEmbedder<2>::NodeCoords> coords;

		// call it
		DTreeMultilevelEmbedder<2>::call(GA.constGraph(), coords);

		// copy the coords back to graph attributes
		for (node v = G.firstNode(); v; v = v->succ()) {
			GA.x(v) = coords[v].coords[0];
			GA.y(v) = coords[v].coords[1];
		}
	}
};

class DTreeMultilevelEmbedder3D : public DTreeMultilevelEmbedder<3>, public LayoutModule
{
public:
	void call(GraphAttributes& GA) override
	{
		// assert 3d
		OGDF_ASSERT(GA.has(GraphAttributes::threeD));

		// the graph
		const Graph& G = GA.constGraph();

		// the input for the general d dim class
		NodeArray<DTreeMultilevelEmbedder<3>::NodeCoords> coords;

		// call it
		DTreeMultilevelEmbedder<3>::call(GA.constGraph(), coords);

		// copy the coords back to graph attributes
		for (node v = G.firstNode(); v; v = v->succ()) {
			GA.x(v) = coords[v].coords[0];
			GA.y(v) = coords[v].coords[1];
			GA.z(v) = coords[v].coords[2];
		}
	}
};


template<int Dim>
void DTreeMultilevelEmbedder<Dim>::call(const Graph& graph, NodeArray<NodeCoords>& resultCoords)
{
	using namespace energybased::dtree;
	using Embedder = DTreeEmbedder<Dim>;

	// we call this just in case
	resultCoords.init(graph);

	// make sure the graph is connected
	OGDF_ASSERT(isConnected(graph));

	// setup the multilevel step
	GalaxyLevel levelBegin(graph);

	// this is the coarsest level with at most m_levelMaxNumNodes
	GalaxyLevel* pLevelEnd = levelBegin.buildLevelsUntil(m_levelMaxNumNodes);

	// this array will hold the layout information of the parent node in the coarser level.
	// furthermore this will keep the final result.
	NodeArray<NodeCoords>& parentPosition = resultCoords;

	double currNumIterations = m_numIterationsFinestLevel;
	double currThreshold = m_thresholdFinestLevel;

	int numLevels = 0;
	// loop from the coarsest to the finest level
	for (GalaxyLevel* pCurrLevel = &levelBegin; pCurrLevel != nullptr; pCurrLevel = pCurrLevel->nextCoarser()) {
		numLevels++;
		currNumIterations *= m_numIterationsFactorPerLevel;
		currThreshold *= m_thresholdFactorPerLevel;
	}

	int currLevelIndex = numLevels - 1;
	// now we loop from the coarsest to the finest level
	for (GalaxyLevel* pCurrLevel = pLevelEnd; pCurrLevel; pCurrLevel = pCurrLevel->nextFiner()) {
		currNumIterations /= m_numIterationsFactorPerLevel;
		currThreshold /= m_thresholdFactorPerLevel;

		// new embedder instance for the current level
		Embedder embedder(pCurrLevel->graph());

		// if this is coarsest one
		if (pCurrLevel->isCoarsestLevel()) {
			// we cannot init from the parent level, do it random
			for (node v = pCurrLevel->graph().firstNode(); v; v = v->succ()) {
				// for all dims
				for (int d = 0; d < Dim; d++) {
					// set the position to some random value
					embedder.setPosition(v, d, randomDouble(-1.0, 1.0));
				}
			}
		} else { // if we have a parent level
			// iterate over all nodes
			for (node v = pCurrLevel->graph().firstNode(); v; v = v->succ()) {
				// this is a node on the coarser level we already processed
				node v_parent = pCurrLevel->parent(v);

				// now init the position from the parent.
				for (int d = 0; d < Dim; d++) {
					// get the position of the parent
					double offset = parentPosition[v_parent].coords[d] * m_scaleFactorPerLevel;

					// set v's position to the parents pos with some random
					embedder.setPosition(v, d, offset + randomDouble(-1.0, 1.0));
				}
			}
		}
		// we have some proper initial coordinates for the nodes

		if (m_useMultilevelWeights) {
			// we cannot init from the parent level, do it random
			for (node v = pCurrLevel->graph().firstNode(); v; v = v->succ()) {
				embedder.setMass(v, pCurrLevel->weight(v));
			}

			for (edge e = pCurrLevel->graph().firstEdge(); e; e = e->succ()) {
				embedder.setEdgeWeight(e, pCurrLevel->edgeWeight(e));
			}
		}

		const int numIterationsMaybe = pCurrLevel->isCoarsestLevel() ? m_numIterationsCoarsestLevel : currNumIterations;
		const int numIterations = std::min(std::max(m_minIterationsPerLevel, numIterationsMaybe), m_maxIterationsPerLevel);

		embedder.scaleNodes(3.0);
		embedder.doIterationsNewton(numIterations, currThreshold, RepForceFunctionNewton<Dim, 1>, AttrForceFunctionPow<Dim, 2>);
		embedder.scaleNodes(1.0 / 3.0);
		embedder.doIterationsNewton(numIterations, currThreshold, RepForceFunctionNewton<Dim, 2>, AttrForceFunctionPow<Dim, 2>);
		// run the layout

		// we now have to backup the positions before getting rid of the embedder
		parentPosition.init(pCurrLevel->graph());

		// iterate over all nodes
		for (node v = pCurrLevel->graph().firstNode(); v; v = v->succ()) {
			// for all coords
			for (int d = 0; d < Dim; d++) {
				parentPosition[v].coords[d] = embedder.position(v, d);
			}
		}

		currLevelIndex--;
	}

	// we are done with the layout. It is saved now in the parentposition nodearray.
	// which is a reference to the result array
}

}
