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

#include <ogdf/energybased/dtree/DTreeForce.h>
#include <ogdf/energybased/dtree/GalaxyLevel.h>

namespace ogdf {
namespace energybased {
namespace dtree {

template<int Dim>
class DTreeEmbedder
{
public:
	//! constructor with a given graph, allocates memory and does initialization
	explicit DTreeEmbedder(const Graph& graph);

	//! destructor
	virtual ~DTreeEmbedder();

	//! returns the d-th coordinate of node v
	double position(node v, int d) const;

	//! sets the d-th coordinate of node v to coord
	void setPosition(node v, int d, double coord);

	//! returns the mass of node v
	double mass(node v) const;

	//! sets the mass of a node v
	void setMass(node v, double mass);

	//! returns the edge weight
	double edgeWeight(edge e) const;

	//! sets the weight of an edge
	void setEdgeWeight(edge e, double weight);

	//! sets the forces of all nodes to 0
	void resetForces();

	//! computes the repulsive forces
	template<typename ForceFunc, bool UseForcePrime>
	void computeRepForces(ForceFunc forceFunc);

	//! computes the repulsive forces for one iteration in O(n^2)
	template<typename ForceFunc, bool UseForcePrime>
	void computeRepForcesExact(ForceFunc forceFunc);

	//! uses the tree code to approximate the repulsive forces in O(nlogn) for one iteration
	template<typename ForceFunc, bool UseForcePrime>
	void computeRepForcesApprox(ForceFunc forceFunc);

	//! computes the edge forces for one iteration
	template<typename AttrForceFunc, bool UseForcePrime>
	void computeEdgeForces(AttrForceFunc attrForceFunc);

#if 0
	//! computes the edge forces for one iteration
	void computeEdgeForcesSq();
#endif

	//! moves the nodes by the computed force vector
	double moveNodes(double timeStep);
	double moveNodesByForcePrime();

#if 0
	//! does a complete iteration with the given repulsive force function
	template<typename RepForceFunc>
	double doIteration(RepForceFunc repForceFunc);

	//! does a complete iteration using the default inverse distance function
	double doIteration() { return doIteration(RepForceFunctionInvDist<Dim>); };

	//! does multiple iterations using the given repulsive force function
	template<typename RepForceFunc>
	void doIterations(int numIterations, double epsilon, RepForceFunc repForceFunc);

	//! does multiple iterations using the given repulsive force function
	template<typename RepForceFunc>
	void doIterations(int numIterations, double epsilon);
#endif

	//! does multiple iterations using the given repulsive force function
	template<typename RepForceFunc, typename AttrForceFunc, bool UseForcePrime>
	void doIterationsTempl(int numIterations, double epsilon, RepForceFunc repForceFunc, AttrForceFunc attrForceFunc);

	template<typename RepForceFunc, typename AttrForceFunc>
	void doIterationsStandard(int numIterations, double epsilon, RepForceFunc repForceFunc, AttrForceFunc attrForceFunc);

	template<typename RepForceFunc, typename AttrForceFunc>
	void doIterationsNewton(int numIterations, double epsilon, RepForceFunc repForceFunc, AttrForceFunc attrForceFunc);

#if 0
	template<typename RepForceFunc>
	void doIterationsAdaptive(int numIterations, double epsilon, RepForceFunc repForceFunc);
#endif

	//! returns the graph
	const Graph& graph() const;

	//! computes the bounding box and all nodes are translated such that bbox center is at centerBBox
	void centerNodesAt(double centerBBox[Dim]);

	//! changes the position of nodes according to a given scale factor
	void scaleNodes(double scaleFactor);

private:
	//! node state
	struct NodeInfo {
		//! position of a node
		double position[Dim];

		//! the forces on a node
		double force[Dim];

		//! sum of the derivs
		double force_prime;

		//! the mass of this node
		double mass;
	};

	//! the graph
	const Graph& m_graph;

	//! node states of all nodes
	NodeArray<NodeInfo> m_nodeInfo;

	//! the weight of the edges
	EdgeArray<double> m_edgeWeight;

	//! the tree force approx
	DTreeForce<Dim>* m_pTreeForce;

	int m_maxNumNodesExactRepForces;

	double m_defaultTimeStep;
};

using DTreeEmbedder2D = DTreeEmbedder<2>;
using DTreeEmbedder3D = DTreeEmbedder<3>;

template<int Dim>
DTreeEmbedder<Dim>::DTreeEmbedder(const Graph& graph) : m_graph(graph)
{
	m_pTreeForce = new DTreeForce<Dim>(m_graph.numberOfNodes());
	m_nodeInfo.init(m_graph);
	for (node v = m_graph.firstNode(); v; v = v->succ()) {
		m_nodeInfo[v].mass = 1.0;
	}

	m_edgeWeight.init(m_graph, 1.0);
	m_maxNumNodesExactRepForces = 50;
	m_defaultTimeStep = 0.125;
}

template<int Dim>
DTreeEmbedder<Dim>::~DTreeEmbedder()
{
	delete m_pTreeForce;
}

template<int Dim>
template<typename ForceFunc, bool UseForcePrime>
void DTreeEmbedder<Dim>::computeRepForcesExact(ForceFunc forceFunc)
{
	double delta[Dim];
	double force;
	double force_prime;
	for (node s = m_graph.firstNode(); s; s = s->succ()) {
		for (node t = s->succ(); t; t = t->succ()) {
			double dist = computeDeltaAndDistance<Dim>(m_nodeInfo[s].position, m_nodeInfo[t].position, delta);

			// evaluate the force function
			forceFunc(dist, force, force_prime);

			force = force / dist * mass(s) * mass(t);

			// add the force to the existing
			for (int d = 0; d < Dim; d++) {
				m_nodeInfo[s].force[d] += force * delta[d];
				m_nodeInfo[t].force[d] -= force * delta[d];
			}

			if (UseForcePrime) {
				force_prime = force_prime * mass(s) * mass(t);
				m_nodeInfo[s].force_prime += force_prime;
				m_nodeInfo[t].force_prime += force_prime;
			}
		}
	}
}

#if 0
template<int Dim>
template<typename ForceFunc>
void DTreeEmbedder<Dim>::computeRepForcesExact(ForceFunc forceFunc)
{
	double delta[Dim];
	double force;
	for (node s = m_graph.firstNode(); s; s = s->succ()) {
		for (node t = s->succ(); t; t = t->succ()) {
			double dist = computeDeltaAndDistance<Dim>(m_nodeInfo[s].position, m_nodeInfo[t].position, delta);

			// evaluate the force function
			forceFunc(dist, force);

			force = force / dist * mass(s) * mass(t);

			// add the force to the existing
			for (int d = 0; d < Dim; d++) {
				m_nodeInfo[s].force[d] += force * delta[d];
				m_nodeInfo[s].force[d] += force * delta[d];
			}
		}
	}
}
#endif

template<int Dim>
template<typename ForceFunc, bool UseForcePrime>
void DTreeEmbedder<Dim>::computeRepForcesApprox(ForceFunc forceFunc)
{
	int currIndex = 0;
	// loop over all nodes
	for (node v = m_graph.firstNode(); v; v = v->succ()) {
		// update the node pos in the tree data structure
		for (int d = 0; d < Dim; d++) {
			m_pTreeForce->setPosition(currIndex, d, m_nodeInfo[v].position[d]);
		}

		// update the mass
		m_pTreeForce->setMass(currIndex, m_nodeInfo[v].mass);
		currIndex++;
	}

	// run the approximation
	m_pTreeForce->template computeForces<ForceFunc, UseForcePrime>(forceFunc);

	// reset the index
	currIndex = 0;

	// loop over all nodes
	for (node v = m_graph.firstNode(); v; v = v->succ()) {
		// read back the forces
		for (int d = 0; d < Dim; d++) {
			m_nodeInfo[v].force[d] += m_pTreeForce->force(currIndex, d);
		}

		if (UseForcePrime) {
			m_nodeInfo[v].force_prime += m_pTreeForce->force_prime(currIndex);
		}

		currIndex++;
	}
}

#if 0
template<int Dim>
template<typename ForceFunc>
void DTreeEmbedder<Dim>::computeRepForcesApproxNewton(ForceFunc forceFunc)
{
	int currIndex = 0;
	// loop over all nodes
	for (node v = m_graph.firstNode(); v; v = v->succ()) {
		// update the node pos in the tree data structure
		for (int d = 0; d < Dim; d++) {
			m_pTreeForce->setPosition(currIndex, d, m_nodeInfo[v].position[d]);
		}

		// update the mass
		m_pTreeForce->setMass(currIndex, m_nodeInfo[v].mass);
		currIndex++;
	}

	// run the approximation
	m_pTreeForce->template computeForces<ForceFunc, true>(forceFunc);

	// reset the index
	currIndex = 0;

	// loop over all nodes
	for (node v = m_graph.firstNode(); v; v = v->succ()) {
		// read back the forces
		for (int d = 0; d < Dim; d++) {
			m_nodeInfo[v].force[d] += m_pTreeForce->force(currIndex, d);
		}

		currIndex++;
	}
}
#endif

template<int Dim>
template<typename ForceFunc, bool UseForcePrime>
void DTreeEmbedder<Dim>::computeRepForces(ForceFunc forceFunc)
{
	if (m_graph.numberOfNodes() <= m_maxNumNodesExactRepForces) {
		computeRepForcesExact<ForceFunc, UseForcePrime>(forceFunc);
	} else {
		computeRepForcesApprox<ForceFunc, UseForcePrime>(forceFunc);
	}
}

#if 0
template<int Dim>
template<typename ForceFunc>
void DTreeEmbedder<Dim>::computeRepForcesNewton(ForceFunc forceFunc)
{
	if (m_graph.numberOfNodes() <= m_maxNumNodesExactRepForces) {
		computeRepForcesExactNewton(forceFunc);
	} else {
		computeRepForcesApproxNewton(forceFunc);
	}
}
#endif

template<int Dim>
template<typename AttrForceFunc, bool UseForcePrime>
void DTreeEmbedder<Dim>::computeEdgeForces(AttrForceFunc attrForceFunc)
{
	// for all edges
	for (edge e = m_graph.firstEdge(); e; e = e->succ()) {
		node s = e->source();
		node t = e->target();

		// check if this is a self loop
		if (s == t) {
			continue;
		}

		// s t delta vector
		double delta[Dim];

		// var for the squared distance of s and t
		double dist_sq = 0.0;

		// compute delta and sum up dist_sq
		for (int d = 0; d < Dim; d++) {
			delta[d] = m_nodeInfo[t].position[d] - m_nodeInfo[s].position[d];
			dist_sq += delta[d] * delta[d];
		}

		// we take the log of the squared distance here
#if 0
		double f = log(dist_sq) * 0.5;
#endif
		double dist = (sqrt(dist_sq));

		double f;
		double f_prime;

		if (UseForcePrime) {
			attrForceFunc(dist, f, f_prime);

			f_prime *= m_edgeWeight[e];

			m_nodeInfo[s].force_prime += f_prime;
			m_nodeInfo[t].force_prime += f_prime;
		} else {
			attrForceFunc(dist, f, f_prime);
		}

		f *= m_edgeWeight[e];

		// compute delta and sum up dist_sq
		for (int d = 0; d < Dim; d++) {
			m_nodeInfo[s].force[d] += f * delta[d] / dist;// * s_scale;
			m_nodeInfo[t].force[d] -= f * delta[d] / dist;// * t_scale;
		}
	}
}

#if 0
template<int Dim>
template<typename AttrForceFunc>
void DTreeEmbedder<Dim>::computeEdgeForces(AttrForceFunc attrForceFunc)
{
	// for all edges
	for (edge e = m_graph.firstEdge(); e; e = e->succ()) {
		node s = e->source();
		node t = e->target();

		// check if this is a self loop
		if (s == t) {
			continue;
		}

		// s t delta vector
		double delta[Dim];

		// var for the squared distance of s and t
		double dist_sq = 0.0;

		// compute delta and sum up dist_sq
		for (int d = 0; d < Dim; d++) {
			delta[d] = m_nodeInfo[t].position[d] - m_nodeInfo[s].position[d];
			dist_sq += delta[d] * delta[d];
		}

		// we take the log of the squared distance here
#if 0
		double f = log(dist_sq) * 0.5;
#endif
		double dist = (sqrt(dist_sq));
		double f  = (dist) * (dist) * m_edgeWeight[e];
		double f_prime = 2.0 * dist * m_edgeWeight[e];
		// for scaling the force accordingly
#if 0
		double s_scale = 1.0/(double)s->degree();
		double t_scale = 1.0/(double)t->degree();
#endif

		m_nodeInfo[s].force_prime += f_prime;
		m_nodeInfo[t].force_prime += f_prime;

		// compute delta and sum up dist_sq
		for (int d = 0; d < Dim; d++) {
			m_nodeInfo[s].force[d] += f * delta[d] / dist;// * s_scale;
			m_nodeInfo[t].force[d] -= f * delta[d] / dist;// * t_scale;
		}
	}
}

template<int Dim>
void DTreeEmbedder<Dim>::computeEdgeForcesSq()
{
	for (node s = m_graph.firstNode(); s; s = s->succ()) {
		double sum_f[Dim];
		double sum_df[Dim];

		double sum_ddf = 0.0;
		// compute delta and sum up dist_sq
		for (int d = 0; d < Dim; d++) {
			sum_f[d] = 0.0;
			sum_df[d] = 0.0;
		}

		for (adjEntry adj = s->firstAdj(); adj; adj = adj->succ()) {
			node t = adj->twinNode();

			// check if this is a self loop
			if (s == t)
				continue;

			// s t delta vector
			double delta[Dim];

			// var for the squared distance of s and t
			double dist_sq = 0.0;

			// compute delta and sum up dist_sq
			for (int d = 0; d < Dim; d++) {
				delta[d] = m_nodeInfo[t].position[d] - m_nodeInfo[s].position[d];
				dist_sq += delta[d] * delta[d];
			}

			double dist = sqrt(dist_sq);

			double f = 1.0;
			double df = 1.0;
			// compute delta and sum up dist_sq
			for (int d = 0; d < Dim; d++) {
				sum_f[d]  +=  f * delta[d] / dist;

			}
			sum_ddf += df;
		}
		for (int d = 0; d < Dim; d++) {
			m_nodeInfo[s].force[d] = m_nodeInfo[s].force[d] + sum_f[d] / sum_ddf;
		}
	}
}
#endif

template<int Dim>
double DTreeEmbedder<Dim>::moveNodesByForcePrime()
{
	// the maximum displacement
	double maxDispl = 0.0;

	// loop over all nodes
	for (node v = m_graph.firstNode(); v; v = v->succ()) {
		double displ_sq = 0.0;
		// update the position by the force vec
		for (int d = 0; d < Dim; d++) {
			double displ = m_nodeInfo[v].force[d] / m_nodeInfo[v].force_prime ;
			m_nodeInfo[v].position[d] += displ;
			displ_sq += displ * displ;
		}

		// we compare the square of the distance to save the sqrt here
		if (maxDispl < displ_sq) {
			maxDispl = displ_sq;
		}
	}
	Logger::slout() << "sqrt(maxDispl)=" << sqrt(maxDispl) << std::endl;
	return sqrt(maxDispl);
}

template<int Dim>
double DTreeEmbedder<Dim>::moveNodes(double timeStep)
{
	// the maximum displacement
	double maxDispl = 0.0;

	// loop over all nodes
	for (node v = m_graph.firstNode(); v; v = v->succ()) {
		double displ_sq = 0.0;
		// update the position by the force vec
		for (int d = 0; d < Dim; d++) {
			double displ = m_nodeInfo[v].force[d] * timeStep;
			m_nodeInfo[v].position[d] += displ  ;
			displ_sq += displ * displ;
		}

		// we compare the square of the distance to save the sqrt here
		if (maxDispl < displ_sq) {
			maxDispl = displ_sq;
		}
	}

	Logger::slout() << "sqrt(maxDispl)=" << sqrt(maxDispl) << std::endl;
	return sqrt(maxDispl);
}

template<int Dim>
void DTreeEmbedder<Dim>::resetForces()
{
	// loop over all nodes
	for (node v = m_graph.firstNode(); v; v = v->succ()) {
		// reset the force vector
		for (int d = 0; d < Dim; d++) {
			m_nodeInfo[v].force[d] = 0.0;
		}
		m_nodeInfo[v].force_prime = 0.0;
	}
}

#if 0
template<int Dim>
template<typename RepForceFunc, bool>
double DTreeEmbedder<Dim>::doIteration(RepForceFunc repForceFunc)
{
	// reset forces
	resetForces();

	// the repulsive forces
	computeRepForces(repForceFunc);

	// edge forces
	computeEdgeForces();

	// move the nodes
	return moveNodes(m_defaultTimeStep);
}

template<int Dim>
template<typename RepForceFunc>
void DTreeEmbedder<Dim>::doIterations(int numIterations, double epsilon, RepForceFunc repForceFunc)
{
	// init the error with epsilon
	double maxDisplacement = epsilon;

	// while error too big and we have iterations left
	for (int i = 0; i < numIterations && maxDisplacement >= epsilon; ++i) {
		// run it
		maxDisplacement = doIteration(repForceFunc);
	}
}

template<int Dim>
template<typename RepForceFunc>
void DTreeEmbedder<Dim>::doIterationsAdaptive(int numIterations, double epsilon, RepForceFunc repForceFunc)
{
	int iterationsUsed = 0;
	// the current timeStep

	double maxTimeStep = 1.0;
	double timeStep = m_defaultTimeStep;

	int progress = 0;
	// scaling factor for the adaptive timeStep
	double t = 0.99;

	// init the error with epsilon
	double maxDisplacement = 100000000.0;

	// value that keeps track of the displacement
	double lastMaxDisplacement = 0.0;

	// while error too big and we have iterations left
	for (int i = 0; i < numIterations && maxDisplacement >= epsilon; ++i) {
		// reset forces
		resetForces();

		// the repulsive forces
		computeRepForces(repForceFunc);

		// edge forces
		computeEdgeForces();

		// move the nodes
		maxDisplacement = moveNodes(timeStep);

		// if this is not the first iteration
		if (i > 0) {
			if (maxDisplacement < lastMaxDisplacement * t) {
				progress++;
				timeStep = std::min(timeStep / t, maxTimeStep);
			} else if (maxDisplacement > lastMaxDisplacement / t) {
				timeStep = timeStep * t;
			}
		}
		// save the maxDisplacement
		lastMaxDisplacement = maxDisplacement;
		iterationsUsed++;
#if 0
		Logger::slout() << maxDisplacement << std::endl;
#endif
	}
	Logger::slout() << "IterationsUsed: " << iterationsUsed << " of " << numIterations << " energy: " << lastMaxDisplacement << std::endl;
}

template<int Dim>
struct MyVec
{
	double x[Dim];
};
#endif

template<int Dim>
template<typename RepForceFunc, typename AttrForceFunc, bool UseForcePrime>
void DTreeEmbedder<Dim>::doIterationsTempl(int numIterations,
                                           double epsilon,
                                           RepForceFunc repForceFunc,
                                           AttrForceFunc attrForceFunc)
{
	if (m_graph.numberOfNodes() < 2) {
		return;
	}

	int numIterationsUsed = 0;
	Logger::slout() << "doIterationsNewton: V = " << m_graph.numberOfNodes() << " E = " << m_graph.numberOfEdges() << " Iterations " << numIterations << std::endl;

	// init the error with epsilon
	double maxDisplacement = 10000.0;

	// while error too big and we have iterations left
	for (int i = 0; i < numIterations && maxDisplacement > epsilon; ++i) {
		numIterationsUsed++;
		// reset forces
		resetForces();

		// the repulsive forces
		computeRepForces<RepForceFunc, UseForcePrime>(repForceFunc);

		// edge forces
		computeEdgeForces<AttrForceFunc, UseForcePrime>(attrForceFunc);

		// move the nodes
		if (UseForcePrime) {
			maxDisplacement =  moveNodesByForcePrime();
		} else {
			maxDisplacement =  moveNodes(0.1);
		}
	}

	Logger::slout() << "Needed " << numIterationsUsed << " of " << numIterations << std::endl;
}

template<int Dim>
template<typename RepForceFunc, typename AttrForceFunc>
void DTreeEmbedder<Dim>::doIterationsStandard(int numIterations, double epsilon, RepForceFunc repForceFunc, AttrForceFunc attrForceFunc)
{
	doIterationsTempl<RepForceFunc, AttrForceFunc, false>(numIterations, epsilon, repForceFunc, attrForceFunc);
}

template<int Dim>
template<typename RepForceFunc, typename AttrForceFunc>
void DTreeEmbedder<Dim>::doIterationsNewton(int numIterations, double epsilon, RepForceFunc repForceFunc, AttrForceFunc attrForceFunc)
{
	doIterationsTempl<RepForceFunc, AttrForceFunc, true>(numIterations, epsilon, repForceFunc, attrForceFunc);
}

template<int Dim>
void DTreeEmbedder<Dim>::centerNodesAt(double centerBBox[Dim])
{
	double bboxMin[Dim];
	double bboxMax[Dim];

	// initial values
	for (int d = 0; d < Dim; d++) {
		bboxMin[d] = bboxMax[d] = position(m_graph.firstEdge(), d);
	}

	// no magic here
	for (node v = m_graph.firstNode(); v; v = v->succ()) {
		for (int d = 0; d < Dim; d++) {
			bboxMin[d] = std::min(bboxMin[d], position(v, d));
			bboxMax[d] = std::max(bboxMax[d], position(v, d));
		}
	}

	double delta[Dim];
	for (int d = 0; d < Dim; d++) {
		delta[d] = -(bboxMin[d] + bboxMax[d]) * 0.5;
	}

	for (node v = m_graph.firstNode(); v; v = v->succ()) {
		for (int d = 0; d < Dim; d++) {
			setPosition(v, d, delta[d]);
		}
	}
}

template<int Dim>
void DTreeEmbedder<Dim>::scaleNodes(double scaleFactor)
{
	for (node v = m_graph.firstNode(); v; v = v->succ()) {
		for (int d = 0; d < Dim; d++) {
			setPosition(v, d, position(v, d) * scaleFactor);
		}
	}
}

template<int Dim>
double DTreeEmbedder<Dim>::position(node v, int d) const
{
	return m_nodeInfo[v].position[d];
}

template<int Dim>
void DTreeEmbedder<Dim>::setPosition(node v, int d, double coord)
{
	m_nodeInfo[v].position[d] = coord;
}

template<int Dim>
double DTreeEmbedder<Dim>::mass(node v) const
{
	return m_nodeInfo[v].mass;
}

template<int Dim>
void DTreeEmbedder<Dim>::setMass(node v, double mass)
{
	m_nodeInfo[v].mass = mass;
}

template<int Dim>
const Graph& DTreeEmbedder<Dim>::graph() const
{
	return m_graph;
}

template<int Dim>
void DTreeEmbedder<Dim>::setEdgeWeight(edge e, double weight)
{
	m_edgeWeight[e] = weight;
}

template<int Dim>
double DTreeEmbedder<Dim>::edgeWeight(edge e) const
{
	return m_edgeWeight[e];
}

}}}
