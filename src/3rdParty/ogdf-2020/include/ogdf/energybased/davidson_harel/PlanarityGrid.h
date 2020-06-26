/** \file
 * \brief Declaration of class PlanarityGrid which implements an
 *        energy function where the energy of a layout depends
 *        on the number of crossings.
 *
 * Uses the UniformGris Class to compute the number of crossings.
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
#include <ogdf/energybased/davidson_harel/UniformGrid.h>

namespace ogdf {
namespace davidson_harel {

class PlanarityGrid: public EnergyFunction {
public:
	//initializes data structures to speed up later computations
	explicit PlanarityGrid(GraphAttributes &AG);
	~PlanarityGrid();
	// computes energy of initial layout and stores it in m_energy
	void computeEnergy() override;
private:
	// computes energy of candidate
	void compCandEnergy() override;
	// changes internal data if candidate is taken
	void internalCandidateTaken() override;
#ifdef OGDF_DEBUG
		virtual void printInternalData() const override;
#endif
	const GraphAttributes &m_layout; //The current layout
	UniformGrid *m_currentGrid; //stores grid for current layout
	UniformGrid *m_candidateGrid; //stores grid for candidate layout
};

}}
