/** \file
 * \brief Declaration of class Layout
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

#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/GraphCopy.h>

namespace ogdf {

class PlanRep;

/**
 * \brief Stores a layout of a graph (coordinates of nodes, bend points of edges).
 *
 * @ingroup graph-drawing
 */
class OGDF_EXPORT Layout
{
public:
	/** @{
	 * \brief Creates a layout associated with no graph.
	 */
	Layout() { }

	/**
	 * \brief Creates a layout associated with graph \p G.
	 *
	 * The layout is initialized such that all node positions are (0,0)
	 * and all bend point lists of edges are empty.
	 *
	 * @param G is the corresponding graph .
	 */
	explicit Layout(const Graph &G) : m_x(G,0), m_y(G,0), m_bends(G) { }


	/** @} @{
	 * \brief Returns a reference to the array storing x-coordinates of nodes.
	 */
	const NodeArray<double> &x() const { return m_x; }

	/**
	 * \brief Returns a reference to the array storing x-coordinates of nodes.
	 */
	NodeArray<double> &x() { return m_x; }

	/** @} @{
	 * \brief Returns a reference to the array storing y-coordinates of nodes.
	 */
	const NodeArray<double> &y() const { return m_y; }

	/**
	 * \brief Returns a reference to the array storing y-coordinates of nodes.
	 */
	NodeArray<double> &y() { return m_y; }


	/** @} @{
	 * \brief Returns the x-coordinate of node \p v.
	 */
	const double &x(node v) const { return m_x[v]; }

	/**
	 * \brief Returns the x-coordinate of node \p v.
	 */
	double &x(node v) { return m_x[v]; }

	/** @} @{
	 * \brief Returns the y-coordinate of node \p v.
	 */
	const double &y(node v) const { return m_y[v]; }

	/**
	 * \brief Returns the y-coordinate of node \p v.
	 */
	double &y(node v) { return m_y[v]; }

	/** @} @{
	 * \brief Returns the bend point list of edge \p e.
	 */
	const DPolyline &bends(edge e) const { return m_bends[e]; }

	/**
	 * \brief Returns the bend point list of edge \p e.
	 */
	DPolyline &bends(edge e) { return m_bends[e]; }


	/** @} @{
	 * \brief Returns the polyline of edge \p eOrig in \p dpl.
	 *
	 * @param GC is the input graph copy; \p GC must also be the associated graph.
	 * @param eOrig is an edge in the original graph of \p GC.
	 * @param dpl is assigned the poyline of \p eOrig.
	 */
	void computePolyline(GraphCopy &GC, edge eOrig, DPolyline &dpl) const;

	/**
	 * \brief Returns the polyline of edge \p eOrig in \p dpl and clears the
	 *        bend points of the copies.
	 *
	 * The bend point lists of all edges in the edge path corresponding to \p eOrig are
	 * empty afterwards! This is a faster version of computePolyline().
	 *
	 * @param PG is the input graph copy; \p PG must also be the associated graph.
	 *        of this layout.
	 * @param eOrig is an edge in the original graph of \p PG.
	 * @param dpl is assigned the poyline of \p eOrig.
	 */
	void computePolylineClear(PlanRep &PG, edge eOrig, DPolyline &dpl);

	//! Computes the bounding box of the layout, which is a drawing of \p PG.
	/**
	 * @param PG must be the planarized representation associated with this layout.
	 * @return a point representing the with and height of this layout, respecting the sizes
	 *         of nodes as stored in \p PG.
	 */
	DPoint computeBoundingBox(PlanRep &PG) const;

	/** @} */

private:
	NodeArray<double> m_x;        //!< The x-coordinates of nodes.
	NodeArray<double> m_y;        //!< The y-coordinates of nodes.
	EdgeArray<DPolyline> m_bends; //!< The bend points of edges.

	OGDF_MALLOC_NEW_DELETE
};

}
