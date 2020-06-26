/** \file
 * \brief Declares class NodePairEnergy which implements an energy
 *        function where the energy of a layout depends on the
 *        each pair of nodes.
 *
 * \author Rene Weiskircher
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

#include <ogdf/basic/Array2D.h>
#include <ogdf/basic/AdjacencyOracle.h>
#include <ogdf/energybased/davidson_harel/EnergyFunction.h>

namespace ogdf {
namespace davidson_harel {

class NodePairEnergy: public EnergyFunction {
public:
	//Initializes data dtructures to speed up later computations
	NodePairEnergy(const string energyname, GraphAttributes &AG);

	virtual ~NodePairEnergy() {
		delete m_nodeNums;
		delete m_pairEnergy;
	}

	//computes the energy of the initial layout
	void computeEnergy() override;

protected:
	//! Computes the energy stored by a pair of vertices at the given positions.
	virtual double computeCoordEnergy(node, node, const DPoint&, const DPoint&) const = 0;

	//! Returns the internal number given to each vertex.
	int nodeNum(node v) const { return (*m_nodeNums)[v]; }

	//! returns true in constant time if two vertices are adjacent.
	bool adjacent(const node v, const node w) const { return m_adjacentOracle.adjacent(v,w); }

	//! Returns the shape of a vertex \p v as a DIntersectableRect.
	const DIntersectableRect& shape(const node v) const { return m_shape[v]; }

#ifdef OGDF_DEBUG
	virtual void printInternalData() const override;
#endif

private:
	NodeArray<int> *m_nodeNums;//stores internal number of each vertex
	Array2D<double> *m_pairEnergy;//stores for each pair of vertices its energy
	NodeArray<double> m_candPairEnergy;//stores for each vertex its pair energy with
	//respect to the vertex to be moved if its new position is chosen
	NodeArray<DIntersectableRect> m_shape;//stores the shape of each vertex as
	//a DIntersectableRect
	List<node> m_nonIsolated;//list of vertices with degree greater zero
	const AdjacencyOracle m_adjacentOracle;//structure for constant time adjacency queries

	//function computes energy stored in a certain pair of vertices
	double computePairEnergy(const node v, const node w) const;

	//computes energy of whole layout if new position of the candidate vertex is chosen
	void compCandEnergy() override;

	//If a candidate change is chosen as the new position, this function sets the
	//internal data accordingly
	void internalCandidateTaken() override;
};

}}
