/** \file
 * \brief Declares the base class ShellingOrderModule for modules
 *        that compute a shelling order of a graph.
 *
 * \author Carsten Gutwenger
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

#include <ogdf/planarlayout/ShellingOrder.h>


namespace ogdf {


/**
 * \brief Base class for modules that compute a shelling order of a graph.
 *
 */
class OGDF_EXPORT ShellingOrderModule
{
public:
	//! Computes a shelling order of an embedded graph G such that \p adj lies on the external face.
	/**
	 * @param G is the input graph; \p G must represent a combinatorial embedding.
	 * @param order is assigned the shelling order.
	 * @param adj is an adjacency entry on the external face; if \p adj is 0, a suitable
	 *        external face is chosen.
	 */
	void call(const Graph &G, ShellingOrder &order, adjEntry adj = nullptr);

	//! Computes a lefmost shelling order of an embedded graph G such that \p adj lies on the external face.
	/**
	 * @param G is the input graph; \p G must represent a combinatorial embedding.
	 * @param order is assigned the shelling order.
	 * @param adj is an adjacency entry on the external face; if \p adj is 0, a suitable
	 *        external face is chosen.
	 */
	void callLeftmost(const Graph &G, ShellingOrder &order, adjEntry adj = nullptr);

	//! Sets the option <i>base ratio</i> to \p x.
	void baseRatio(double x) {m_baseRatio = x;}

	//! Returns the current setting of the option <b>base ratio</b>.
	double baseRatio() const {return m_baseRatio;}

	virtual ~ShellingOrderModule() { }

protected:
	//! This pure virtual function does the actual computation.
	/**
	 * A derived class must implement this method. It is called with the embedded graph
	 * and an adjacency entry describing the external face, and must return the
	 * computed order in \p partition.
	 * @param G is the embedded input graph.
	 * @param adj is an adjacency entry on the external face.
	 * @param partition returns the coputed shelling order.
	 */
	virtual void doCall(const Graph &G,
		adjEntry adj,
		List<ShellingOrderSet> &partition) = 0;

	double m_baseRatio; //! The option <i>base ratio</i>.
};

}
