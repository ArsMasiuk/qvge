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

#include <ogdf/energybased/dtree/DTreeWSPD.h>
#include <ogdf/energybased/dtree/DTreeForceTypes.h>

namespace ogdf {
namespace energybased {
namespace dtree {

template<int Dim, typename ForceFunc, bool UseForcePrime>
class DTreeWSPDCallback;

template<int Dim>
class DTreeForce
{
public:
	template<int _Dim, typename ForceFunc, bool UseForcePrime>
	friend class DTreeWSPDCallback;

	using WSPD = DTreeWSPD<Dim>;
	using Tree = typename WSPD::Tree;

	//! constructs a new WSPD (well-separated pair decomposition) for numPoints
	explicit DTreeForce(int numPoints);

	//! destructor
	virtual ~DTreeForce();

	//! returns d-th coordinate of the i-th point
	double position(int i , int d) const;

	//! sets the d-th coordinate of the i-th point
	void setPosition(int i, int d, double c);

	//! returns the mass of the i-th point
	double mass(int i) const;

	//! sets the mass of the i-th point
	void setMass(int i, double m);

	//! returns d-th coordinate of the i-th force vector
	double force(int i , int d) const;

	//! returns derivation of the d-th coordinate of the i-th force vector
	double force_prime(int i ) const;

	//! main call
	template<typename ForceFunc, bool UseForcePrime>
	void computeForces(ForceFunc forceFunc);

	//! returns the number of points
	int numPoints() const { return m_numPoints; };

	//! returns a const ref to the wspd
	const WSPD& wspd() const;

	//! returns a const reference to the tree
	const Tree& tree() const;
private:
	//! the point data
	struct PointData {
		//! mass of this point
		double mass;

		//! the force
		double force[Dim];

		double force_prime;
	};

	//! the node data
	struct NodeData {
		//! mass of this node
		double mass;

		//! center of mass of the subtree
		double centerOfMass[Dim];

		//! the force
		double force[Dim];

		//! the first derivation of the distance based force function
		double force_prime;
	};

	//! returns the wspd
	WSPD& wspd();

	//! internal for allocating mem
	void allocate();

	//! internal for allocating mem
	void deallocate();

	//! internal function for computing the mass and center of mass of quadtree nodes
	void bottomUpPhase(int curr);

	//! internal function for accumulating forces in the leaves
	void topDownPhase(int curr);

	//! reset the point forces
	void resetPointForces();

	//! reset the tree nodes
	void resetTreeNodes();

	//! array for the point related data
	PointData* m_pointData;

	//! array for the node related data
	NodeData* m_nodeData;

	//! the number of points
	int m_numPoints;

	//! the WSPD instance
	WSPD* m_pWSPD;
};

template<int Dim, typename ForceFunc, bool UseForcePrime>
class DTreeWSPDCallback : public IWSPD
{
public:
	DTreeWSPDCallback(DTreeForce<Dim>& treeForce, ForceFunc forceFunc)
		: m_treeForce(treeForce), m_forceFunc(forceFunc)
	{
	}

	//! called by the WSPD for well separated pair
	virtual void onWellSeparatedPair(int a, int b)
	{
		// force vector
		double delta[Dim];
		double force;
		double force_prime;

		double dist = computeDeltaAndDistance<Dim>(m_treeForce.m_nodeData[a].centerOfMass,
				m_treeForce.m_nodeData[b].centerOfMass,
				delta);
		m_forceFunc(dist, force, force_prime);
#if 0
		m_forceFunc(m_treeForce.m_nodeData[a].centerOfMass, m_treeForce.m_nodeData[b].centerOfMass, force, force_prime);
#endif

		// compute the force vector for each dim
		for (int d = 0; d < Dim; d++) {
			m_treeForce.m_nodeData[a].force[d] += force * delta[d] / dist * m_treeForce.m_nodeData[b].mass;
			m_treeForce.m_nodeData[b].force[d] -= force * delta[d] / dist * m_treeForce.m_nodeData[a].mass;
		};

		if (UseForcePrime) {
			m_treeForce.m_nodeData[a].force_prime += force_prime * m_treeForce.m_nodeData[b].mass;
			m_treeForce.m_nodeData[b].force_prime += force_prime * m_treeForce.m_nodeData[a].mass;
		}


	}

	DTreeForce<Dim>& m_treeForce;
	ForceFunc m_forceFunc;
};

template<int Dim>
DTreeForce<Dim>::DTreeForce(int numPoints) : m_numPoints(numPoints)
{
	// get the memory
	allocate();

	m_pWSPD->setSeparationFactor(1.0);
}

template<int Dim>
DTreeForce<Dim>::~DTreeForce()
{
	// free the mem
	deallocate();
}

template<int Dim>
void DTreeForce<Dim>::allocate()
{
	m_pWSPD = new WSPD(m_numPoints);
	m_nodeData = new NodeData[tree().maxNumNodes()];
	m_pointData = new PointData[m_numPoints];

	// initial mass setting for a point
	for (int i = 0; i < m_numPoints; i++) {
		m_pointData[i].mass = 1.0;
	}
}

template<int Dim>
void DTreeForce<Dim>::deallocate()
{
	delete m_pWSPD;
	delete[] m_nodeData;
	delete[] m_pointData;
}

template<int Dim>
const typename DTreeForce<Dim>::WSPD& DTreeForce<Dim>::wspd() const
{
	return *m_pWSPD;
}

template<int Dim>
typename DTreeForce<Dim>::WSPD& DTreeForce<Dim>::wspd()
{
	return *m_pWSPD;
}

template<int Dim>
const typename DTreeForce<Dim>::Tree& DTreeForce<Dim>::tree() const
{
	return wspd().tree();
}

template<int Dim>
double DTreeForce<Dim>::position(int i , int d) const
{
	return wspd().point(i).x[d];
}

template<int Dim>
void DTreeForce<Dim>::setPosition(int i, int d, double c)
{
	wspd().setPoint(i, d, c);
}

template<int Dim>
double DTreeForce<Dim>::mass(int i) const
{
	return m_pointData[i].mass;
}

template<int Dim>
void DTreeForce<Dim>::setMass(int i, double m)
{
	m_pointData[i].mass = m;
}

template<int Dim>
double DTreeForce<Dim>::force(int i , int d) const
{
	return m_pointData[i].force[d];
}

template<int Dim>
double DTreeForce<Dim>::force_prime(int i) const
{
	return m_pointData[i].force_prime;
}

template<int Dim>
template<typename ForceFunc, bool UseForcePrime>
void DTreeForce<Dim>::computeForces(ForceFunc forceFunc)
{
	// reset all point forces
	resetPointForces();

	if (numPoints() <= 1)
		return;

	// first let the WSPD instance update the quadtre
	wspd().update();

	// now do the bottom up phase
	bottomUpPhase(tree().rootIndex());

	DTreeWSPDCallback<Dim, ForceFunc, UseForcePrime> wspdCallBack(*this, forceFunc);

	// run the WSPD cell cell interaction
	wspd().computeWSPD(&wspdCallBack);

	// finally, push down the forces
	topDownPhase(tree().rootIndex());
}


template<int Dim>
void DTreeForce<Dim>::bottomUpPhase(int curr)
{
	// reset the force and the center of mass
	for (int d = 0; d < Dim; d++) {
		m_nodeData[curr].force[d] = 0.0;
		m_nodeData[curr].force_prime = 0.0;
		m_nodeData[curr].centerOfMass[d] = 0.0;
	}

	// reset the mass
	m_nodeData[curr].mass = 0.0;

	// if this is an inner node
	if (tree().numChilds(curr)) {
		// iterate over the children
		for (int i = 0; i < tree().numChilds(curr); i++) {
			// the child index
			const int child = tree().child(curr, i);

			// compute the size of the subtree
			bottomUpPhase(child);

			// node curr
			for (int d = 0; d < Dim; d++) {
				// sum up the center of mass coordinates
				m_nodeData[curr].centerOfMass[d] += m_nodeData[child].centerOfMass[d] * m_nodeData[child].mass;
			}

			// add the mass
			m_nodeData[curr].mass += m_nodeData[child].mass;
		};
	}	// else it is a leaf
	else {
		// and the remaining points
		for (int i = 0; i < tree().numPoints(curr); ++i) {
			int pointIndex = tree().point(curr, i);
			// node curr
			for (int d = 0; d < Dim; d++) {
				m_nodeData[curr].centerOfMass[d] += position(pointIndex, d) * mass(pointIndex);
			}

			m_nodeData[curr].mass += mass(pointIndex);
		}
	}
	// node curr
	for (int d = 0; d < Dim; d++) {
		m_nodeData[curr].centerOfMass[d] /= m_nodeData[curr].mass;
	}
}

template<int Dim>
void DTreeForce<Dim>::topDownPhase(int curr)
{
	// if this is an inner node
	if (tree().numChilds(curr)) {
		// iterate over the children
		for (int i = 0; i < tree().numChilds(curr); i++) {
			// the child index
			const int child = tree().child(curr, i);

			// node curr
			for (int d = 0; d < Dim; d++) {
				// push the force down to the child
				m_nodeData[child].force[d] += m_nodeData[curr].force[d];
			}

			m_nodeData[child].force_prime += m_nodeData[curr].force_prime;

			// compute the size of the subtree
			topDownPhase(child);
		};
	}	// else it is a leaf
	else {
		// and the remaining points
		for (int i = 0; i < tree().numPoints(curr); ++i) {
			// node curr
			int pointIndex = tree().point(curr, i);

			// node curr
			for (int d = 0; d < Dim; d++) {
				// push the force down to the child
				m_pointData[pointIndex].force[d] = m_nodeData[curr].force[d] * mass(pointIndex);
			}

			m_pointData[pointIndex].force_prime = m_nodeData[curr].force_prime * mass(pointIndex);
		}
	}
}

template<int Dim>
void DTreeForce<Dim>::resetPointForces()
{
	for (int i = 0; i < m_numPoints; i++) {
		for (int d = 0; d < Dim; d++) {
			m_pointData[i].force[d] = 0.0;
		}
		m_pointData[i].force_prime = 0.0;
	}
}

}}}
