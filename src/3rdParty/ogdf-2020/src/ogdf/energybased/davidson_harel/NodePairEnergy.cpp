/** \file
 * \brief Implementation of class NodePairEnergy
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


#include <ogdf/energybased/davidson_harel/NodePairEnergy.h>

namespace ogdf {
namespace davidson_harel {

NodePairEnergy::NodePairEnergy(const string energyname, GraphAttributes &AG) :
	EnergyFunction(energyname, AG),
	m_candPairEnergy(m_G),
	m_shape(m_G),
	m_adjacentOracle(m_G)
{
	for (node v : m_G.nodes) { //saving the shapes of the nodes in m_shape
		DPoint center(AG.x(v), AG.y(v));
		m_shape[v] = DIntersectableRect(center, AG.width(v), AG.height(v));
	}
	m_G.allNodes(m_nonIsolated);
	ListIterator<node> it, itSucc;
	for (it = m_nonIsolated.begin(); it.valid(); it = itSucc) {
		itSucc = it.succ();
		if ((*it)->degree() == 0)
			m_nonIsolated.del(it);
	}
	m_nodeNums = new NodeArray<int>(m_G, 0);
	int n_num = 1;
	for (node v : m_nonIsolated) {
		(*m_nodeNums)[v] = n_num;
		n_num++;
	}
	n_num--;
	m_pairEnergy = new Array2D<double>(1, n_num, 1, n_num);
}


void NodePairEnergy::computeEnergy()
{
	int n_num = m_nonIsolated.size();
	double energySum = 0.0;
	Array<node> numNodes(1,n_num);

	for(node v : m_nonIsolated) {
		numNodes[(*m_nodeNums)[v]] = v;
	}
	for(int i = 1; i <= n_num-1 ; i++) {
		for(int j = i+1; j <= n_num; j++) {
			double E = computePairEnergy(numNodes[i],numNodes[j]);
			(*m_pairEnergy)(i,j) = E;
			energySum += E;
		}
	}
	m_energy = energySum;
}


double NodePairEnergy::computePairEnergy(const node v, const node w) const {
	return computeCoordEnergy(v,w,currentPos(v),currentPos(w));
}


void NodePairEnergy::internalCandidateTaken() {
	node v = testNode();
	int candNum = (*m_nodeNums)[v];
	for(node u :  m_nonIsolated) {
		if(u != v) {
			int numit = (*m_nodeNums)[u];
			(*m_pairEnergy)(min(numit,candNum),max(numit,candNum)) = m_candPairEnergy[u];
			m_candPairEnergy[u] = 0.0;
		}
	}
}


void NodePairEnergy::compCandEnergy()
{
	node v = testNode();
	int numv = (*m_nodeNums)[v];
	m_candidateEnergy = energy();
	for (node u : m_nonIsolated) {
		if (u != v) {
			int j = (*m_nodeNums)[u];
			m_candidateEnergy -= (*m_pairEnergy)(min(j, numv), max(j, numv));
			m_candPairEnergy[u] = computeCoordEnergy(v, u, testPos(), currentPos(u));
			m_candidateEnergy += m_candPairEnergy[u];
			if (m_candidateEnergy < 0.0) {
				OGDF_ASSERT(m_candidateEnergy > -0.00001);
				m_candidateEnergy = 0.0;
			}
		}
		else m_candPairEnergy[u] = 0.0;
	}
	OGDF_ASSERT(m_candidateEnergy >= -0.0001);
}


#ifdef OGDF_DEBUG
void NodePairEnergy::printInternalData() const
{
	for (node v : m_nonIsolated) {
		std::cout << "\nNode: " << (*m_nodeNums)[v];
		std::cout << " CandidatePairEnergy: " << m_candPairEnergy[v];
	}
	std::cout << "\nPair energies:";
	for (int i = 1; i < m_nonIsolated.size(); i++)
	for (int j = i + 1; j <= m_nonIsolated.size(); j++)
	if ((*m_pairEnergy)(i, j) != 0.0)
		std::cout << "\nEnergy(" << i << ',' << j << ") = " << (*m_pairEnergy)(i, j);
}
#endif

}}
