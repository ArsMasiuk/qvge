/** \file
 * \brief Offers variety of possible SimDraw creations.
 *
 * \author Michael Schulz and Daniel Lueckerath
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

#include <ogdf/simultaneous/SimDrawManipulatorModule.h>

namespace ogdf
{
//! Creates variety of possible SimDraw creations
/**
 * This class is used for creating simdraw instances.
 * Possible features include reading a graph, randomly modifying
 * or clearing the edgeSubgraph value and changing the subGraphBits.
 */

class OGDF_EXPORT SimDrawCreator : public SimDrawManipulatorModule
{

public:
	//! constructor
	explicit SimDrawCreator(SimDraw &SD) : SimDrawManipulatorModule(SD) {}

	//! returns SubGraphBits from edge e
	uint32_t &SubGraphBits(edge e) { return m_GA->subGraphBits(e); }

	//! returns SubGraphBits from edge e
	uint32_t &SubGraphBits(edge e) const { return m_GA->subGraphBits(e); }

	//! reads a Graph
	void readGraph(const Graph &G) { *m_G = G; }

	//! randomly chose edgeSubGraphs value for two graphs
	/**
	* Assigns random edgeSubGraphs values to all edges to create
	* a SimDraw instance consisting of two basic graphs.
	* Each edge in m_G has a chance of \p doubleESGProbability (in Percent)
	* to belong to two SubGraphs.
	* Otherwise it has equal chance to belong to either basic graph.
	*/
	void randomESG2(int doubleESGProbability = 50);

	//! randomly chose edgeSubGraphs value for three graphs
	/**
	* Assigns random edgeSubGraphs values to all edges to create
	* a SimDraw instance consisting of three basic graphs.
	* Each edge in m_G has a chance of \p doubleESGProbabilit (in Percent)
	* to belong to two basic graphs and a chance of \p tripleESGProbability
	* (in Percent) to belong to three basic graphs.
	*/
	void randomESG3(int doubleESGProbability = 50, int tripleESGProbability = 25);

	//! randomly chose edgeSubGraphs value for graphNumber graphs
	/**
	* Assigns random edgeSubGraphs values to all edges to create
	* a SimDraw instance consisting of \p graphNumber basic graphs.
	* Each edge has an equal chance for each SubGraphBit - value.
	*/
	void randomESG(int graphNumber);

	//! clears edgeSubGraphs value
	/**
	* This method clears all SubGraph values from m_G.
	* After this function all edges belong to no basic graph.
	* CAUTION: All edges need to be reset their edgeSubGraphs value
	* for maintaining consistency.
	*/
	void clearESG();

	//! randomly creates a simdraw instance
	/**
	* This method creates a random graph with \p numberOfNodes nodes,
	* \p numberOfEdges edges. It is transfered into a simdraw instance with
	* \p numberOfBasicGraphs basic graphs.
	*
	* randomSimpleGraph from graph_generators.h is used to
	* create a random graph. Furthermore randomESG is used on this graph
	* to generate \p numberOfBasicGraphs basic graphs.
	*/
	void createRandom(int numberOfNodes, int numberOfEdges, int numberOfBasicGraphs);

};

}
