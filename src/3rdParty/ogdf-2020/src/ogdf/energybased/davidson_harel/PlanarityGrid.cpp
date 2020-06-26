/** \file
 * \brief Implementation of class PlanarityGrid
 *
 * The PlanarityGrid energy function counts the number of
 * crossings. It contains two UniformGrids: One for the
 * current layout and one for the candidate layout.
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


#include <ogdf/energybased/davidson_harel/PlanarityGrid.h>

namespace ogdf {
namespace davidson_harel {

PlanarityGrid::~PlanarityGrid()
{
	delete m_currentGrid;
	delete m_candidateGrid;
}


// intialize m_currentLayout and m_candidateLayout
PlanarityGrid::PlanarityGrid(GraphAttributes &AG):
EnergyFunction("PlanarityGrid",AG), m_layout(AG)
{
	m_currentGrid = new UniformGrid(AG);
	m_candidateGrid = nullptr;
}


// computes energy of layout, stores it and sets the crossingMatrix
void PlanarityGrid::computeEnergy()
{
	m_energy = m_currentGrid->numberOfCrossings();
}


// computes the energy if the node returned by testNode() is moved
// to position testPos().
void PlanarityGrid::compCandEnergy()
{
	delete m_candidateGrid;
	node v = testNode();
	const DPoint& newPos = testPos();
	if(m_currentGrid->newGridNecessary(v,newPos))
		m_candidateGrid = new UniformGrid(m_layout,v,newPos);
	else
		m_candidateGrid = new UniformGrid(*m_currentGrid,v,newPos);
	m_candidateEnergy = m_candidateGrid->numberOfCrossings();
}


// this functions sets the currentGrid to the candidateGrid
void PlanarityGrid::internalCandidateTaken() {
	delete m_currentGrid;
	m_currentGrid = m_candidateGrid;
	m_candidateGrid = nullptr;
}


#ifdef OGDF_DEBUG
void PlanarityGrid::printInternalData() const {
	std::cout << "\nCurrent grid: " << *m_currentGrid;
	std::cout << "\nCandidate grid: ";
	if(m_candidateGrid != nullptr)
		std::cout << *m_candidateGrid;
	else std::cout << "empty.";
}
#endif

}}
