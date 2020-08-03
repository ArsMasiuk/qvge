/** \file
 * \brief Implements class EnergyFunction.
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


#include <ogdf/energybased/davidson_harel/EnergyFunction.h>

namespace ogdf {
namespace davidson_harel {

EnergyFunction::EnergyFunction(const string &funcname, GraphAttributes &AG) :
	m_G(AG.constGraph()),
	m_name(funcname),
	m_candidateEnergy(0),
	m_energy(0),
	m_AG(AG),
	m_testNode(nullptr),
	m_testPos(0.0,0.0) { }


void EnergyFunction::candidateTaken()
{
	m_energy=m_candidateEnergy;
	m_candidateEnergy = 0.0;
	m_AG.x(m_testNode)=m_testPos.m_x;
	m_AG.y(m_testNode)=m_testPos.m_y;
	m_testPos = DPoint(0.0,0.0);
	internalCandidateTaken();
	m_testNode=nullptr;
}


double EnergyFunction::computeCandidateEnergy(const node v, const DPoint &testPos)
{
	m_testPos = testPos;
	m_testNode = v;
	compCandEnergy();
	OGDF_ASSERT(m_candidateEnergy >= 0.0);
	return m_candidateEnergy;
}


#ifdef OGDF_DEBUG
void EnergyFunction::printStatus() const{
	std::cout << "\nEnergy function name: " << m_name;
	std::cout << "\nCurrent energy: " << m_energy;
	std::cout << "\nPosition of nodes in current solution:";
	NodeArray<int> num(m_G);
	int count = 1;
	for (node v : m_G.nodes)
		num[v] = count++;
	for (node v : m_G.nodes) {
		std::cout << "\nNode: " << num[v] << " Position: " << currentPos(v);
	}
	std::cout << "\nTest Node: " << m_testNode << " New coordinates: " << m_testPos;
	std::cout << "\nCandidate energy: " << m_candidateEnergy;
	printInternalData();
}
#endif

}}
