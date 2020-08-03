/** \file
 * \brief Declaration of classes GridSifting and GlobalSifting.
 *
 * \author Paweł Schmidt
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

#include <ogdf/layered/SugiyamaLayout.h>
#include <ogdf/layered/BlockOrder.h>
#include <ogdf/layered/LayeredCrossMinModule.h>

namespace ogdf {

/**
 * \brief The global sifting heuristic for crossing minimization.
 *
 * @ingroup gd-layered-crossmin
 *
 * Implementation of the global sifting heuristic based on
 * C. Bachmaier, F. J. Brandenburg, W. Brunner, F. Hübner,
 * <i>Global k-Level Crossing Reduction</i>, J. Graph Algorithms and
 * Appl. 15(5), 2011, pp. 631-659.
 * This class implements the interface LayeredCrossMinModule and should be
 * used as a part of the Sugiyama algorithm for drawing layered graphs.
 */
class GlobalSifting : public LayeredCrossMinModule {
public:
	/**
	 * \brief Returns the current setting of option nRepeats.
	 *
	 * This option determines, how many times the global sifting is repeated.
	 * Each repetition starts from permutation returned by the previous one.
	 * The first repetition starts from random permutation.
	 */
	int nRepeats() { return m_nRepeats; }

	//! Sets the option nRepeats to \p num.
	void nRepeats( int num ) { m_nRepeats = num; }

	//! Implementation of interface LateredCrossMinModule.
	const HierarchyLevelsBase *reduceCrossings(const SugiyamaLayout &sugi, Hierarchy &H, int &nCrossings) {
		BlockOrder *pBlockOrder = new BlockOrder(H,true);
		pBlockOrder -> globalSifting( sugi.runs(), m_nRepeats, &nCrossings );

		return pBlockOrder;
	}

private:
	int m_nRepeats = 10;
};

/**
 * \brief The grid sifting heuristic for crossing minimization.
 *
 * @ingroup gd-layered-crossmin
 *
 * Implementation of the grid sifting heuristic based on
 * C. Bachmaier, W. Brunner, A. Gleißner, <i>Grid Sifting: Leveling
 * and Crossing Reduction</i>, Technical Report MIP-1103, University
 * of Passau, 2011.

 * This class implements the interface LayeredCrossMinModule and should be
 * used as a part of the Sugiyama algorithm for drawing layered graphs.
 *
 *
 *
 */
class GridSifting : public LayeredCrossMinModule {
public:
	/**
	 * @copydoc ogdf::LayeredCrossMinModule::reduceCrossings
	 *
	 * \warning \p nCrossings is not set by this implementation!
	 */
	const HierarchyLevelsBase *reduceCrossings(const SugiyamaLayout &sugi, Hierarchy &H, int &nCrossings) override {
		BlockOrder *pBlockOrder = new BlockOrder(H,false);
		pBlockOrder -> m_verticalStepsBound = m_verticalStepsBound;
		pBlockOrder -> gridSifting( sugi.runs() );

		return pBlockOrder;
	}

	/**
	 * \brief Returns the current setting of option verticalStepsBound.
	 *
	 * This option determines, how many levels can be traversed in
	 * vertical step of the grid sifting algorithm.
	 */
	int verticalStepsBound() { return m_verticalStepsBound; }

	//! Sets the option verticalStepsBound to \p b.
	void verticalStepsBound( int b ) { m_verticalStepsBound = b; }

private:
	int m_verticalStepsBound = 10;
};

}
