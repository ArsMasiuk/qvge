/** \file
 * \brief implementation of initial cut-constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * A feasible ILP solution has to imply a completely connected, planar Sub-Clustergraph.
 * For each cluster that is not connected, additional connection edges have to be inserted
 * between the chunks of the cluster, to obtain c-connectivity.
 * Thus, initial constraints are added that guarantee this behaviour, if the number of chunks
 * is at most 3. If some cluster consists of more than 3 chunks, additional constraints
 * have to be separated during the algorithm.
 *
 * \author Mathias Jansen
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

#include <ogdf/basic/basic.h>

#include <ogdf/cluster/internal/ChunkConnection.h>

using namespace ogdf;
using namespace ogdf::cluster_planarity;
using namespace abacus;

ChunkConnection::ChunkConnection(Master *master, const ArrayBuffer<node>& chunk, const ArrayBuffer<node>& cochunk) :
	BaseConstraint(master, nullptr, CSense::Greater, 1.0, false, false, true)
{
	chunk.compactMemcpy(m_chunk);
	cochunk.compactMemcpy(m_cochunk);
}


ChunkConnection::~ChunkConnection() {}


int ChunkConnection::coeff(node n1, node n2) const {
	//TODO: speedup
	for(node v : m_chunk) {
		if(v == n1) {
			for(node w : m_cochunk) {
				if(w == n2) {
					return 1;
				}
			}
			return 0;
		} else if(v == n2) {
			for(node w : m_cochunk) {
				if(w == n1) {
					return 1;
				}
			}
			return 0;
		}
	}
	return 0;
}
