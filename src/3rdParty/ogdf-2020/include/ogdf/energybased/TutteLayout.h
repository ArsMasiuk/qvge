/** \file
 * \brief Declaration of ogdf::TutteLayout.
 *
 * \author David Alberts and Andrea Wagner
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

#include <ogdf/basic/LayoutModule.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/external/coin.h>

#include <coin/CoinPackedMatrix.hpp>

namespace ogdf {

//! Tutte's layout algorithm.
/**
 * @ingroup gd-energy
 *
 * This algorithm draws a planar graph straight-line
 * without crossings.
 *
 * The idea of the algorithm is to place every vertex into the
 * center of gravity by its neighbours.
 *
 * See "How to draw a graph" by W. T. Tutte (1962) for details.
 *
 * \pre Input graphs need to be triconnected.
 */
class OGDF_EXPORT TutteLayout : public LayoutModule
{
public:
	TutteLayout();

	DRect bbox () const {
		return m_bbox;
	}

	void bbox (const DRect &bb) {
		m_bbox = bb;
	}

	virtual void call(GraphAttributes &AG) override;
	void call(GraphAttributes &AG, const List<node> &givenNodes);

private:
	static bool solveLP(
		int cols,
		const CoinPackedMatrix &Matrix,
		const Array<double> &rightHandSide,
		Array<double> &x);

	void setFixedNodes(const Graph &G, List<node> &nodes,
		List<DPoint> &pos, double radius = 1.0);

	/*! sets the positions of the nodes in a largest face of $G$ in the
	*  form of a regular $k$-gon with the prescribed radius. The
	*  corresponding nodes and their positions are stored in nodes
	*  and pos, respectively. $G$ does not have to be planar!
	*/
	void setFixedNodes(const Graph &G, List<node> &nodes, const List<node> &givenNodes,
		List<DPoint> &pos, double radius = 1.0);

	bool doCall(GraphAttributes &AG,
		const List<node> &fixedNodes,
		List<DPoint> &fixedPositions);

	DRect m_bbox;
};

}
