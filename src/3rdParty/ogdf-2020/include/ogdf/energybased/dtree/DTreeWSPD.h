/** \file
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

#include <ogdf/basic/basic.h>
#include <ogdf/energybased/dtree/DTree.h>

namespace ogdf {
namespace energybased {
namespace dtree {

class IWSPD
{
public:
	//! called by the WSPD for a pair that is well separated
	virtual void onWellSeparatedPair(int aIndex, int bIndex) = 0;
};

template<int Dim>
class DTreeWSPD
{
public:
	using IntType = unsigned int;
	using Tree = DTree<IntType, Dim>;

	//! constructs a new WSPD for numPoints
	explicit DTreeWSPD(int numPoints);

	//! destructor
	~DTreeWSPD();

	//! geometry for the quadtree nodes
	struct NodeData {
		//! center of cell circle
		double x[Dim];

		//! bounding box min coord
		double min_x[Dim];

		//! bounding box min coord
		double max_x[Dim];

		//! radius of the cell
		double radius_sq;
	};

	//! world coordinates of the points
	struct PointData {
		//! coords of the point
		double x[Dim];
	};

	//! call this when the point set has been updated.
	void update();

	//! returns the corresponding Dtree
	const Tree& tree() const { return *m_pTree; };

	//! returns the data for a quadtree
	const NodeData& node(int i) const { return m_nodeData[i]; };

	//! sets the point to the given coords
	void setPoint(int i, int d, double coord);

	//! return ith point
	const PointData& point(int i) const { return m_pointData[i]; };

	// temp function for
	void computeWSPD(IWSPD* m_pIWSPD);

	//! returns the parameter s of the WSPD (default is 1.0)
	double separationFactor() const { return m_wspdSeparationFactor; };

	//! sets the parameter s of the WSPD (default is 1.0)
	void setSeparationFactor(double s) { m_wspdSeparationFactor = s; };

protected:
	IWSPD* m_pIWSPD = nullptr;

	//! returns the data for a quadtree
	NodeData& node(int i) { return m_nodeData[i]; };

	//! updates the bounding box by iterating over all points
	void updateBoundingBox();

	//! updates the integer grid points in the quadtree
	void updateTreeGridPoints();

	//! updates the geometry of the quadtree nodes
	void updateTreeNodeGeometry();

	//! the recursive function of the above
	void updateTreeNodeGeometry(int curr);

	//! the unary recursive function generating the binary calls
	void wspdRecursive(int a);

	//! the binary recursive function to separate the subtree a and b
	void wspdRecursive(int a, int b);

	//! predicate for determining if cells are well-separated
	bool areWellSeparated(int a, int b) const;

	//! allocate mem
	void allocate();

	//! free mem
	void deallocate();

	//! number of points
	int m_numPoints;

	//! the separation factor for the ws predicate
	double m_wspdSeparationFactor;

	//! a cached value for the ws test
	double m_wspdSeparationFactorPlus2Squared_cached;

	//! the quadtree this wspd is working on
	Tree* m_pTree = nullptr;

	//! geometry for the quadtree nodes
	NodeData* m_nodeData = nullptr;

	//! point data
	PointData* m_pointData = nullptr;

	//! the bounding box min coord of the point set
	double m_bboxMin[Dim];

	//! the bounding box max coord of the point set
	double m_bboxMax[Dim];
};

//! constructs a new WSPD for numPoints
template<int Dim>
DTreeWSPD<Dim>::DTreeWSPD(int numPoints)
: m_numPoints(numPoints)
, m_wspdSeparationFactor(1.0)
, m_wspdSeparationFactorPlus2Squared_cached(9.0)
{
	// get the memory
	allocate();

	// init arrays
	for (int i = 0; i < Dim; i++) {
		m_bboxMin[i] = m_bboxMax[i] = 0.0;
	}
}

//! destructor
template<int Dim>
DTreeWSPD<Dim>::~DTreeWSPD()
{
	// free the mem
	deallocate();
}

template<int Dim>
void DTreeWSPD<Dim>::allocate()
{
	m_pTree = new Tree(m_numPoints);
	m_nodeData = new NodeData[m_pTree->maxNumNodes()];
	m_pointData = new PointData[m_numPoints];
}

template<int Dim>
void DTreeWSPD<Dim>::deallocate()
{
	delete m_pTree;
	delete[] m_nodeData;
	delete[] m_pointData;
}

template<int Dim>
void DTreeWSPD<Dim>::update()
{
	// update the bounding box of the point set
	updateBoundingBox();

	// update grid points inside the quadtree
	updateTreeGridPoints();

	// rebuild the tree
	m_pTree->build();

	// compute center, radius and bbox for each node
	updateTreeNodeGeometry();
}

template<int Dim>
void DTreeWSPD<Dim>::computeWSPD(IWSPD* pIWSPD)
{
	m_pIWSPD = pIWSPD;

	// update this cached value for the well-sep test
	const double s = (m_wspdSeparationFactor + 2.0);

	// precompute this
	m_wspdSeparationFactorPlus2Squared_cached = s * s;

	// go ahead with the decomposition
	wspdRecursive(m_pTree->rootIndex());
}


// the unary recursive function generating the binary calls
template<int Dim>
void DTreeWSPD<Dim>::wspdRecursive(int curr)
{
	// iterate over all ordered pairs of children
	for (int i = 0; i < m_pTree->numChilds(curr); i++) {
		// the first child index
		const int first_child = m_pTree->child(curr, i);

		// the second loop for the pair
		for (int j = i+1; j < m_pTree->numChilds(curr); j++) {
			// the second child index
			const int second_child = m_pTree->child(curr, j);

			// call for each ordered pair the binary function
			wspdRecursive(first_child, second_child);
		}

		// now do all this for every child
		wspdRecursive(first_child);
	}
}

template<int Dim>
void DTreeWSPD<Dim>::wspdRecursive(int a, int b)
{
	if (areWellSeparated(a, b)) {
		// far enough away => approx
		if (m_pIWSPD)
			m_pIWSPD->onWellSeparatedPair(a, b);
	} else {
		// two cells are too close
		int small_node = a;
		int large_node = b;

		// make sure the small one is not the bigger one
		if (node(small_node).radius_sq > node(large_node).radius_sq) {
			std::swap(small_node, large_node);
		}

		// split the bigger one
		for (int i = 0; i < tree().numChilds(large_node); ++i) {
			// recurse on the child
			wspdRecursive(small_node, tree().child(large_node, i));
		}
	}
}

template<int Dim>
bool DTreeWSPD<Dim>::areWellSeparated(int a, int b) const
{
	// take the bigger radius
	double r_max_sq = std::max(node(a).radius_sq, node(b).radius_sq);

	// compute the square distance
	double dist_sq = 0.0;

	// the d^2 loop
	for (int d = 0; d < Dim; d++) {
		double dx = node(a).x[d] - node(b).x[d];
		dist_sq += dx * dx;
	}

	// two circles are well separated iff d - 2r > sr
	// where d is the distance between the two centers
	// (not the distance of circles!)
	// this ws <=> d > (s + 2)r
	// more efficient: d^2 > (s + 2)^2 r^2
	// d_sq > (s^2 + 4s + 4) * r_max
#if 0
# if 0
	const double s = (m_wspdSeparationFactor + 2.0);
	return dist_sq > (m_wspdSeparationFactor * m_wspdSeparationFactor + 4.0 * m_wspdSeparationFactor + 4) * r_max * r_max;
# else
	return dist_sq > s * s * r_max_sq;
# endif
#else
	return dist_sq > m_wspdSeparationFactorPlus2Squared_cached * r_max_sq;
#endif
}

template<>
bool DTreeWSPD<2>::areWellSeparated(int a, int b) const
{
	// take the bigger radius
	double r_max_sq = std::max(node(a).radius_sq, node(b).radius_sq);

	// compute the square distance
	double dx_0 = node(a).x[0] - node(b).x[0];
	double dx_1 = node(a).x[1] - node(b).x[1];
	double dist_sq = dx_0 * dx_0 + dx_1 * dx_1;

	// compare the sq distance
	return dist_sq > m_wspdSeparationFactorPlus2Squared_cached * r_max_sq;
}

template<>
bool DTreeWSPD<3>::areWellSeparated(int a, int b) const
{
	// take the bigger radius
	double r_max_sq = std::max(node(a).radius_sq, node(b).radius_sq);

	// compute the square distance
	double dx_0 = node(a).x[0] - node(b).x[0];
	double dx_1 = node(a).x[1] - node(b).x[1];
	double dx_2 = node(a).x[2] - node(b).x[2];
	double dist_sq = dx_0 * dx_0 + dx_1 * dx_1 + dx_2 * dx_2;

	// compare the sq distance
	return dist_sq > m_wspdSeparationFactorPlus2Squared_cached * r_max_sq;
}


//! sets the point to the given coords
template<int Dim>
void DTreeWSPD<Dim>::setPoint(int i, int d, double coord)
{
	m_pointData[i].x[d] = coord;
}

template<int Dim>
void DTreeWSPD<Dim>::updateBoundingBox()
{
	if (!m_numPoints)
		return;

	// initial values
	for (int d = 0; d < Dim; d++) {
		m_bboxMin[d] = m_bboxMax[d] = point(0).x[d];
	}

	// no magic here
	for (int i = 1; i < m_numPoints; i++) {
		for (int d = 0; d < Dim; d++) {
			m_bboxMin[d] = std::min(m_bboxMin[d], point(i).x[d]);
			m_bboxMax[d] = std::max(m_bboxMax[d], point(i).x[d]);
		}
	}
}

// updates the integer grid points in the quadtree
template<int Dim>
void DTreeWSPD<Dim>::updateTreeGridPoints()
{
	for (int d = 0; d < Dim; d++) {
		double noise_max = (m_bboxMax[d] - m_bboxMin[d]) * 0.25;
		m_bboxMax[d] += randomDouble(0.0, noise_max);
		m_bboxMin[d] -= randomDouble(0.0, noise_max);
	}
	// lets assume the bbox is updated. find the longest side
	double quad_size = m_bboxMax[0] - m_bboxMin[0];
	for (int d = 1; d < Dim; d++) {
		quad_size = std::max(quad_size, m_bboxMax[d] - m_bboxMin[d]);
	}

	const double factor = (double)(0xffffffff);

	double scale = factor / quad_size;
	// iterate over all points
	for (int i = 0; i < m_numPoints; i++) {
		for (int d = 0; d < Dim; d++) {
			// put it in the bounding square
#if 0
			double nx = ((point(i).x[d] - m_bboxMin[d] + 0.01) / quad_size + 0.02);
#endif
			double nx = ((point(i).x[d] - m_bboxMin[d]) * scale);

			// dirty put on grid here
			unsigned int ix = static_cast<unsigned int>(nx); //nx * (double)(unsigned int)(0x1fffffff);

			// set the point coord
			m_pTree->setPoint(i, d, ix);
		}
	}
}

template<int Dim>
void DTreeWSPD<Dim>::updateTreeNodeGeometry()
{
	updateTreeNodeGeometry(m_pTree->rootIndex());
}

// updates the geometry of the quadtree nodes
template<int Dim>
void DTreeWSPD<Dim>::updateTreeNodeGeometry(int curr)
{
	// if this is an inner node
	if (m_pTree->numChilds(curr)) {
		// init with the bbox of the first child

		// iterate over the rest of the children
		for (int i = 0; i < m_pTree->numChilds(curr); i++) {
			// the child index
			const int child = m_pTree->child(curr, i);

			// compute the size of the subtree
			updateTreeNodeGeometry(child);

			// lazy set the first
			if (!i) {
				for (int d = 0; d < Dim; d++) {
					node(curr).min_x[d] = node(child).min_x[d];
					node(curr).max_x[d] = node(child).max_x[d];
				}
			} else {
				for (int d = 0; d < Dim; d++) {
					node(curr).min_x[d] = std::min(node(curr).min_x[d], node(child).min_x[d]);
					node(curr).max_x[d] = std::max(node(curr).max_x[d], node(child).max_x[d]);
				}
			}
		};
	} else { // else it is a leaf
		// init from first point
		for (int d = 0; d < Dim; d++) {
			node(curr).min_x[d] = node(curr).max_x[d] = point(tree().point(curr,0)).x[d];
		}

		// and the remaining points
		for (int i = 1; i < tree().numPoints(curr); ++i) {
			for (int d = 0; d < Dim; d++) {
				node(curr).min_x[d] = std::min(node(curr).min_x[d], point(tree().point(curr,i)).x[d]);
				node(curr).max_x[d] = std::max(node(curr).max_x[d], point(tree().point(curr,i)).x[d]);
			}
		}
	}

	// init the radius
	node(curr).radius_sq = 0.0;

	// compute the center and radius of the rect
	for (int d = 0; d < Dim; d++) {
		node(curr).x[d] = (node(curr).min_x[d] + node(curr).max_x[d]) * 0.5;
		double s_x = (node(curr).max_x[d] - node(curr).min_x[d]);
		node(curr).radius_sq += s_x * s_x;
	}

	// and the smallest enclosing circle radius squared
	node(curr).radius_sq *= 0.25; // sqrt(node(curr).radius) * 0.5 squared;
}

}}}
