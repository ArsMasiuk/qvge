/** \file
 * \brief Declares ClusterGraphCopyAttributes, which manages access
 *  on copy of an attributed clustered graph.
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

#include <ogdf/layered/ExtendedNestingGraph.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>


namespace ogdf {

/**
 * \brief Manages access on copy of an attributed clustered graph.
 *
 * @ingroup gd-helper
 */
class OGDF_EXPORT ClusterGraphCopyAttributes {

	const ExtendedNestingGraph *m_pH;
	ClusterGraphAttributes     *m_pACG;
	NodeArray<double> m_x, m_y;

public:
	//! Initializes instance of class ClusterGraphCopyAttributes.
	ClusterGraphCopyAttributes(
		const ExtendedNestingGraph &H,
		ClusterGraphAttributes &ACG) :
		m_pH(&H), m_pACG(&ACG), m_x(H,0), m_y(H,0) { }

	~ClusterGraphCopyAttributes() { }

	//! Returns corresponding ClusterGraphAttributes.
	const ClusterGraphAttributes &getClusterGraphAttributes() const { return *m_pACG; }

	//! Returns width of node v.
	double getWidth(node v) const {
		node vOrig = m_pH->origNode(v);
		return (vOrig == nullptr) ? 0.0 : m_pACG->width(vOrig);
	}

	//! Returns height of node v.
	double getHeight(node v) const {
		node vOrig = m_pH->origNode(v);
		return (vOrig == nullptr) ? 0.0 : m_pACG->height(vOrig);
	}

	//! Returns reference to x-coord. of node v.
	const double &x(node v) const {
		return m_x[v];
	}

	//! Returns reference to x-coord. of node v.
	double &x(node v) {
		return m_x[v];
	}

	//! Returns reference to y-coord. of node v.
	const double &y(node v) const {
		return m_y[v];
	}

	//! Returns reference to y-coord. of node v.
	double &y(node v) {
		return m_y[v];
	}

	//! Returns coordinate of upper cluster boundary of original cluster \p cOrig.
	double top(cluster cOrig) const {
		return m_pACG->y(cOrig);
	}
	//! Returns coordinate of lower cluster boundary of original cluster \p cOrig.
	double bottom(cluster cOrig) const {
		return m_pACG->y(cOrig) + m_pACG->height(cOrig);
	}

	//! Sets the position of the cluster rectangle for original cluster \p cOrig.
	void setClusterRect(
		cluster cOrig,
		double left,
		double right,
		double top,
		double bottom)
	{
		m_pACG->x  (cOrig) = left;
		m_pACG->y  (cOrig) = top;
		m_pACG->width (cOrig) = right-left;
		m_pACG->height(cOrig) = bottom-top;
	}

	void setClusterLeftRight(
		cluster cOrig,
		double left,
		double right)
	{
		m_pACG->x  (cOrig) = left;
		m_pACG->width (cOrig) = right-left;
	}

	void setClusterTopBottom(
		cluster cOrig,
		double top,
		double bottom)
	{
		m_pACG->y  (cOrig) = top;
		m_pACG->height(cOrig) = bottom-top;
	}

	//! Sets attributes for the original graph in attributed graph.
	void transform();
};

}
