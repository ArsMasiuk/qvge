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
	//! Computes the total edge length in the layout \p ga.
	/**
	 * One may assign \c nullptr to any of the output parameter if the respective value isn't of interest.
	 *
	 * \param ga                Input layout.
	 * \param pMinLength        Is assigned the minimum edge length.
	 * \param pMaxLength        Is assigned the maximum edge length.
	 * \param pAvgLength        Is assigned the average edge length.
	 * \param pStdDeviation     Is assigned the standard deviation of edge lengths.
	 * \param considerSelfLoops Determines whether the lengths of self-loops are considered.
	 * \return                  Sum of all edge lengths.
	 */
	static double edgeLengths(
		const GraphAttributes &ga,
		double *pMinLength        = nullptr,
		double *pMaxLength        = nullptr,
		double *pAvgLength        = nullptr,
		double *pStdDeviation     = nullptr,
		bool    considerSelfLoops = false);


	//! Computes the total number of bends (i.e., bend-points) in the layout \p ga.
	/**
	 * One may assign \c nullptr to any of the output parameter if the respective value isn't of interest.

	 * \param ga                Input layout.
	 * \param pMinBendsPerEdge  Is assigned the minimum number of bends for any edge.
	 * \param pMaxBendsPerEdge  Is assigned the maximum number of bends for any edge.
	 * \param pAvgBendsPerEdge  Is assigned the average number of bends per edge.
	 * \param pStdDeviation     Is assigned the standard deviation of edge bends.
	 * \param considerSelfLoops Determines whether the bends of self-loops are considered.
	 * \return                  Total number of bend points on all edges.
	 */
	static int numberOfBends(
		const GraphAttributes &ga,
		int    *pMinBendsPerEdge  = nullptr,
		int    *pMaxBendsPerEdge  = nullptr,
		double *pAvgBendsPerEdge  = nullptr,
		double *pStdDeviation     = nullptr,
		bool    considerSelfLoops = false);


	//! Computes the angular resolution of the layout \p ga.
	/**
	 * The angular resolution of a layout is the smallest angle formed by any two edge segments that meet in a common endpoint or bend point.
	 * Angles are given in radians.

	 * One may assign \c nullptr to any of the output parameter if the respective value isn't of interest.
	 *
	 * \param ga            Input layout.
	 * \param pMaxAngle     Is assigned the maximum angle for any two tangent segments.
	 * \param pAvgAngle     Is assigned the average angle for any two tangent segments.
	 * \param pStdDeviation Is assigned the standard deviation for angles of tangent segments.
	 * \param considerBends Determines whether bend points of edges shall be considered.
	 * \return              Angular resolution (smallest angle) of the layout.
	 */
	static double angularResolution(
		const GraphAttributes &ga,
		double *pMaxAngle     = nullptr,
		double *pAvgAngle     = nullptr,
		double *pStdDeviation = nullptr,
		bool    considerBends = true);


	//! Computes the number of edge crossings in the layout \p ga.
	/**
	 * If several edge segments cross in the same point, this is counted as if all of these segments
	 * would cross pairwise. E.g., if three edge segments cross in a common points, this counts as
	 * three crossings.
	 *
	 * \warning The same warning as for #intersectionGraph applies.
	 *
	 * \param ga Input layout. If it contains bend points, each segement of an edge's polyline is considered as a line segment.
	             Otherwise, a straight-line drawing is assumed.
	 * \return Crossing number of the drawing.
	 */
	static int numberOfCrossings(const GraphAttributes &ga);


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
	 * \param ga        Input layout. If it contains bend points, each segement of an edge's polyline is considered as a line segment.
	                    Otherwise, a straight-line drawing is assumed.
	 * \param H         Is assigned the intersection graph.
	 * \param points    Maps nodes in \p H to their geometric position in the layout.
	 * \param origNode  Maps nodes in \p H to nodes in \p ga's graph. Points that are only intersection points of segments are mapped to \c nullptr.
	 * \param origEdge  Maps edges in \p H to the corresponding edges in \p ga's graph.
	 */
	static void intersectionGraph(const GraphAttributes &ga, Graph &H, NodeArray<DPoint> &points, NodeArray<node> &origNode, EdgeArray<edge> &origEdge);
};

}
