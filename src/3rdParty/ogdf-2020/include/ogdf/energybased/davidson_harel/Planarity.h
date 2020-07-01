/** \file
 * \brief Declaration of class Planarity which implements an
 *        energy function where the energy of a layout depends
 *        on the number of crossings.
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

#include <ogdf/energybased/davidson_harel/EnergyFunction.h>
#include <ogdf/basic/Array2D.h>

namespace ogdf {
namespace davidson_harel {

class Planarity: public EnergyFunction {
public:
	//! Initializes data structures to speed up later computations.
	explicit Planarity(GraphAttributes &AG);

	~Planarity();

	//! Computes energy of initial layout and stores it in #m_energy.
	void computeEnergy() override;

private:
	struct ChangedCrossing {
		int edgeNum1;
		int edgeNum2;
		bool cross;
	};

	//! Returns 1 if edges cross else 0.
	bool intersect(const edge, const edge) const;

	//! Computes energy of candidate.
	void compCandEnergy() override;

	//! Changes internal data if candidate is taken.
	void internalCandidateTaken() override;

	//! Tests if two lines given by four points intersect.
	bool lowLevelIntersect(const DPoint&, const DPoint&, const DPoint&,
	                       const DPoint&) const;

#ifdef OGDF_DEBUG
	virtual void printInternalData() const override;
#endif

	EdgeArray<int> *m_edgeNums; //!< numbers of edges
	Array2D<bool> *m_crossingMatrix; //!< stores for each pair of edges if they cross

	/**
	 * stores for all edges incident to the test node
	 * an array with the crossings that change if the candidate position is chosen
	 */
	List<ChangedCrossing> m_crossingChanges;

	List<edge> m_nonSelfLoops; //!< list of edges that are not slef loops
};

}}
