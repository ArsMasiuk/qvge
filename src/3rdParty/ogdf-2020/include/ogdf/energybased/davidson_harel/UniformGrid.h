/** \file
 * \brief Declaration of class UniformGrid.
 *
 * This is a class implementing a 2-dimensional Hash array.
 * It uses templates for the keys and the data of the objects
 * stored in it.
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

#pragma once

#include<ogdf/basic/geometry.h>
#include<ogdf/basic/SList.h>
#include<ogdf/basic/Array2D.h>
#include<ogdf/basic/GraphAttributes.h>
#include<ogdf/basic/HashArray2D.h>

namespace ogdf {
namespace davidson_harel {

class UniformGrid {

public:
	// This constructor takes a GraphAttributes and computes a grid for the given layout.
	explicit UniformGrid(const GraphAttributes &);

	// This constructor gets the current layout, the node that may be
	// moved and its new position and computes the data for the
	// modified layout.
	UniformGrid(const GraphAttributes &,const node, const DPoint&);

	// Takes a UniformGrid and produces a new grid for the updated layout
	UniformGrid(const UniformGrid &, const node, const DPoint&);

	int numberOfCrossings() const {return m_crossNum;}

	bool newGridNecessary(const node v, const DPoint& p) {
		bool resize = false;
		DIntersectableRect ir;
		computeGridGeometry(v,p,ir);
		double size = max(ir.width(), ir.height());
		size /= m_edgeMultiplier*(m_graph).numberOfEdges();
		if(size <= m_CellSize/2.0 || size >= m_CellSize*2.0) resize = true;
		return resize;
	}

private:
	void ModifiedBresenham(const IPoint &, const IPoint &, SList<IPoint> &) const;

	// This takes two DPoints with and computes a list of points
	// that are the lower left corners of the cells that may possibly contain points
	// of the straight line segment connecting the two points
	void DoubleModifiedBresenham(const DPoint &, const DPoint &, SList<IPoint> &) const;

	// this function computes the grid coordinate of a point that depends on the
	// coordiantes of the point, the lower left corner of the bounding rectangle
	// and the size of a cell
	IPoint computeGridPoint(const DPoint &dp) const {
		double x = floor(dp.m_x / m_CellSize);
		OGDF_ASSERT(isInt(x));
		double y = floor(dp.m_y / m_CellSize);
		OGDF_ASSERT(isInt(y));
		return IPoint(int(x),int(y));
	}

	// computes for a grid point the corresponding DPoint
	DPoint computeRealPoint(const IPoint &ip) const {
		DPoint p;
		p.m_x = ip.m_x*m_CellSize;
		p.m_y = ip.m_y*m_CellSize;
		return p;
	}

	// checks if a double number is an integer
	bool isInt(double d) const {
		if(d - floor(d) > 0) return false;
		if(d < std::numeric_limits<int>::min() || d > std::numeric_limits<int>::max())
			return false;
		return true;
	}

	// computes the crossings of the given edges for the given layout
	// with the node moved to the position given as argument
	void computeCrossings(const List<edge>&, const node, const DPoint&);

	// computes the geometry of the grid if the node is moved
	// to the position given by the point
	void computeGridGeometry(const node, const DPoint&, DIntersectableRect&) const;

	// Checks if two edges cross inside the given cell.
	// The node and the point are the moved node and its/new position
	bool crossingTest(
		const edge,
		const edge,
		const node,
		const DPoint&,
		const IPoint&);

#ifdef OGDF_DEBUG
	void markCells(SList<IPoint> &, Array2D<bool> &) const;

	template<typename TPoint, typename TNum>
	bool crossesCell(TPoint &A, TPoint &B,
	                 TNum xlow, TNum xhigh, TNum ylow, TNum yhigh,
	                 const IPoint &CellAdr) const
	{
		bool crosses;
		if (A.m_x == B.m_x) { // line segment is vertical
			crosses = A.m_x >= xlow
			       && A.m_x < xhigh
			       && intervalIntersect(A.m_y, B.m_y, ylow, yhigh);
		} else { // line segment not vertical
			if (A.m_x > B.m_x) {
				std::swap(A, B);
			}
			double m = (B.m_y - A.m_y) / (B.m_x - A.m_x);
			double c = A.m_y - A.m_x * m;
			double y1 = m * xlow + c;
			double y2 = m * xhigh + c;
			crosses = intervalIntersect(A.m_x, B.m_x, xlow, xhigh)
			       && intervalIntersect(min(A.m_y, B.m_y),
			                            max(A.m_y, B.m_y),
			                            ylow, yhigh)
			       && intervalIntersect(y1, y2, ylow, yhigh);
		}
		return crosses;
	}

	bool crossesCell(IPoint, IPoint, const IPoint &) const;
	bool crossesCell(DPoint, DPoint, const IPoint &) const;

	void checkBresenham(DPoint, DPoint) const;
	void checkBresenham(IPoint, IPoint) const;
	bool intervalIntersect(double,double,double,double) const;
	friend std::ostream& operator<<(std::ostream &,const UniformGrid&);
	int m_crossingTests;
	int m_maxEdgesPerCell;
	double m_time;
#endif
	const GraphAttributes &m_layout; // the layout
	const Graph &m_graph;
	HashArray2D<int,int,List<edge> > m_grid; // stores for each grid cell
	                                         // the Array of edges that cross that cell
	EdgeArray<List<edge> > m_crossings; // stores for each edge the edges
	                                    //its crossings in the current layout
	EdgeArray<List<IPoint> > m_cells; //Contains for each edge the list of cells it crosses
	double m_CellSize; // Sidelength of one cell
	const static double m_epsilon; // tolerance fo double computation
	const static double m_edgeMultiplier; // this controls the gridsize
	int m_crossNum; // number of crossings

	UniformGrid& operator=(const UniformGrid& ug);
};

}}
