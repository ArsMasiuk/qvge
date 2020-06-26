/** \file
 * \brief Declares class LayoutStatistics which provides various
 *        functions for computing statistical measures of a layout.
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


namespace ogdf {


//! Computes statistical information about a layout.
/**
 * @ingroup graph-drawing
 */
class OGDF_EXPORT LayoutStatistics
{
public:
	//! Computes the edge length for each edge in the layout \p ga.
	/**
	 * \param ga                Input layout.
	 * \param considerSelfLoops Determines whether the lengths of self-loops are considered.
	 * \return                  The edge length for each edge.
	 */
	static ArrayBuffer<double> edgeLengths(
		const GraphAttributes &ga,
		bool considerSelfLoops = false);


	//! Computes the number of bends (i.e. bend-points) for each edge in the layout \p ga.
	/**
	 * \param ga                Input layout.
	 * \param considerSelfLoops Determines whether the bends of self-loops are considered.
	 * \return                  The number of bends for each edge.
	 */
	static ArrayBuffer<int> numberOfBends(
		const GraphAttributes &ga,
		bool considerSelfLoops = false);


	//! Computes the angle for each pair of adjacent edge segments of the layout \p ga.
	/**
	 * Angles are given in radians.
	 *
	 * \param ga            Input layout.
	 * \param considerBends Determines whether bend points of edges shall be considered.
	 * \return              The angle for each two adjacent edge segments.
	 */
	static ArrayBuffer<double> angles(
		const GraphAttributes &ga,
		bool considerBends = true);


	//! Computes the number of edge crossings for each edge in the layout \p ga.
	/**
	 * If several edge segments cross in the same point, this is counted as if
	 * all of these segments would cross pairwise. E.g., if three edge segments
	 * cross in a common points, this counts as two crossings for each of the
	 * edges.
	 *
	 * \warning The same warning as for #intersectionGraph applies.
	 * \warning The sum of all returned values is twice the number of crossings
	 * as each crossing involves two edges.
	 *
	 * \param ga Input layout. If it contains bend points, each segment of an edge's polyline is considered as a line segment.
	             Otherwise, a straight-line drawing is assumed.
	 * \return   The number of crossings for each edge.
	 */
	static ArrayBuffer<int> numberOfCrossings(const GraphAttributes &ga);


	//! Computes the number of crossings through a non-incident node for each
	//! edge in the layout \p ga.
	/**
	 * If several edge segments cross a node in the same point, one crossing per
	 * edge segment is counted. E.g., if three edge segments cross a node in a
	 * common point, this counts as three node crossings.
	 * Each node is treated as if it had the shape of the rectangle with the
	 * corresponding width and height given by \p ga.
	 *
	 * \param ga Input layout. If it contains bend points, each segment of an edge's polyline is considered as a line segment.
	             Otherwise, a straight-line drawing is assumed.
	 * \return   The number of node crossings for each edge.
	 */
	static ArrayBuffer<int> numberOfNodeCrossings(const GraphAttributes &ga);


	//! Computes the number of node overlaps for each node in the layout \p ga.
	/**
	 * Each node is treated as if it had the shape of the rectangle with the
	 * corresponding width and height given by \p ga.
	 *
	 * \warning The sum of all returned values is twice the number of node
	 * overlaps as each node overlap involves two nodes.
	 *
	 * \param ga Input layout.
	 * \return   The number of node overlaps for each node.
	 */
	static ArrayBuffer<int> numberOfNodeOverlaps(const GraphAttributes &ga);


	//! Computes the intersection graph \p H of the line segments in the layout given by \p ga.
	/**
	 * The nodes of the intersection graph are all endpoints of segments in \p ga plus all intersection points.
	 * The edges corrsepond to edges in the input layout: If an edge connecting points \a v and \a w in \p H corresponds
	 * to an edge \a e in the input graph, then \a e contains a line segment \a s such that both \a v and \a w are
	 * endpoints or intersection points of \a s.
	 *
	 * To put it more simple, we obtain graph \p H from \p ga by putting a dummy vertex on each crossing
	 * and bend point, and joining all nodes representing the same point in the plane.
	 *
	 * \warning Do not call this algorithm on drawings with arbitrarily close curves (e.g., curves overlapping on an interval).
	 *
	 * \param ga        Input layout. If it contains bend points, each segment of an edge's polyline is considered as a line segment.
	                    Otherwise, a straight-line drawing is assumed.
	 * \param H         Is assigned the intersection graph.
	 * \param points    Maps nodes in \p H to their geometric position in the layout.
	 * \param origNode  Maps nodes in \p H to nodes in \p ga's graph. Points that are only intersection points of segments are mapped to \c nullptr.
	 * \param origEdge  Maps edges in \p H to the corresponding edges in \p ga's graph.
	 */
	static void intersectionGraph(const GraphAttributes &ga, Graph &H, NodeArray<DPoint> &points, NodeArray<node> &origNode, EdgeArray<edge> &origEdge);
};

}
