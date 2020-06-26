/** \file
 * \brief Declaration and definition of ogdf::SpringEmbedderBase.
 *
 * \author Carsten Gutwenger, Tilo Wiedera
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

#include <ogdf/basic/LayoutModule.h>
#include <ogdf/energybased/SpringForceModel.h>
#include <ogdf/packing/TileToRowsCCPacker.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/GraphCopy.h>

namespace ogdf {
namespace spring_embedder {

//! Common base class for ogdf::SpringEmbedderBase and ogdf::SpringEmbedderGridVariant.
class SpringEmbedderBase : public LayoutModule {
public:
	//! The scaling method used by the algorithm.
	enum class Scaling {
		input,             //!< bounding box of input is used.
		userBoundingBox,   //!< bounding box set by userBoundingBox() is used.
		scaleFunction,     //!< automatic scaling is used with parameter set by scaleFunctionFactor() (larger factor, larger b-box).
		useIdealEdgeLength //!< use the given ideal edge length to scale the layout suitably.
	};

	//! Constructor
	SpringEmbedderBase() {
		// default parameters
		m_iterations        = 400;
		m_iterationsImprove = 200;
		m_coolDownFactor    = 0.999;
		m_forceLimitStep    = 0.25;

		m_boundingBox = DRect(0, 0, 250, 250);
		m_noise = true;

		m_forceModel        = SpringForceModel::FruchtermanReingold;
		m_forceModelImprove = SpringForceModel::FruchtermanReingoldModRep;

		m_avgConvergenceFactor = 0.1;
		m_maxConvergenceFactor = 0.2;

		m_scaling = Scaling::scaleFunction;
		m_scaleFactor = 4.0;

		m_userBoundingBox = DRect(0, 0, 100, 100);

		m_minDistCC = LayoutStandards::defaultCCSeparation();
		m_pageRatio = 1.0;

		m_maxThreads = System::numberOfProcessors();

		double def_nw = LayoutStandards::defaultNodeWidth();
		double def_nh = LayoutStandards::defaultNodeHeight();
		m_idealEdgeLength = LayoutStandards::defaultNodeSeparation() + sqrt(def_nw*def_nw + def_nh*def_nh);
	}

	virtual void call(GraphAttributes &GA) override {
		const Graph &G = GA.constGraph();
		if(G.empty())
			return;

		// all edges straight-line
		GA.clearAllBends();

		GraphCopy GC;
		GC.createEmpty(G);

		// compute connected component of G
		NodeArray<int> component(G);
		int numCC = connectedComponents(G,component);

		// intialize the array of lists of nodes contained in a CC
		Array<List<node> > nodesInCC(numCC);

		for(node v : G.nodes)
			nodesInCC[component[v]].pushBack(v);

		EdgeArray<edge> auxCopy(G);
		Array<DPoint> boundingBox(numCC);

		for(int i = 0; i < numCC; ++i)
		{
			GC.initByNodes(nodesInCC[i],auxCopy);
			makeSimpleUndirected(GC);

			const int n = GC.numberOfNodes();

			// special case: just one node
			if(n == 1) {
				node vOrig = GC.original(GC.firstNode());
				GA.x(vOrig) = GA.y(vOrig) = 0;
				boundingBox[i] = DPoint(0,0);
				continue;
			}

			callMaster(GC, GA, boundingBox[i]);
			//std::cout << "avg. force: " << master.avgDisplacement() << std::endl;
			//std::cout << "max. force: " << master.maxDisplacement() << std::endl;
		}

		Array<DPoint> offset(numCC);
		TileToRowsCCPacker packer;
		packer.call(boundingBox,offset,m_pageRatio);

		// The arrangement is given by offset to the origin of the coordinate
		// system. We still have to shift each node and edge by the offset
		// of its connected component.

		for(int i = 0; i < numCC; ++i)
		{
			const List<node> &nodes = nodesInCC[i];

			const double dx = offset[i].m_x;
			const double dy = offset[i].m_y;

			// iterate over all nodes in ith CC
			ListConstIterator<node> it;
			for(node v : nodes) {
				GA.x(v) += dx;
				GA.y(v) += dy;
			}
		}
	}

	//! Returns the currently used force model.
	SpringForceModel forceModel() const {
		return m_forceModel;
	}

	//! Sets the used force model to \p fm.
	void forceModel(SpringForceModel fm) {
		m_forceModel = fm;
	}

	//! Returns the currently used force model for the improvement step.
	SpringForceModel forceModelImprove() const {
		return m_forceModelImprove;
	}

	//! Sets the used force model for the improvement step to \p fm.
	void forceModelImprove(SpringForceModel fm) {
		m_forceModelImprove = fm;
	}

	//! Returns the currently used <i>average convergence factor</i>.
	/**
	 * This factor is used for detecting convergence of the energy system.
	 * With respect to the average displacement of a node in a single step, we assume
	 * to have convergence if it is at most #m_avgConvergenceFactor * #m_idealEdgeLength.
	 */
	double avgConvergenceFactor() const {
		return m_avgConvergenceFactor;
	}

	//! Sets the <i>average convergence factor</i> to \p f.
	void avgConvergenceFactor(double f) {
		if(f >= 0)
			m_avgConvergenceFactor = f;
	}

	//! Returns the currently used <i>maximum</i> convergence factor.
	/**
	 * This factor is used for detecting convergence of the energy system.
	 * With respect to the maximum displacement of a node in a single step, we assume
	 * to have convergence if it is at most #m_maxConvergenceFactor * #m_idealEdgeLength.
	 */
	double maxConvergenceFactor() const {
		return m_maxConvergenceFactor;
	}

	//! Sets the <i>maximum</i> convergence factor to \p f.
	void maxConvergenceFactor(double f) {
		if(f >= 0)
			m_maxConvergenceFactor = f;
	}

	//! Returns the current setting of iterations.
	/**
	 * This setting limits the number of optimization rounds. If convergence (with respect to node displacement)
	 * is detected, the optimization process is immediately finished.
	 */
	int iterations() const {
		return m_iterations;
	}

	//! Sets the number of iterations to \p i.
	void iterations(int i) {
		if (i >= 0)
			m_iterations = i;
	}

	//! Returns the current setting of iterations for the improvement phase.
	int iterationsImprove() const {
		return m_iterationsImprove;
	}

	//! Sets the number of iterations for the improvement phase to \p i.
	void iterationsImprove(int i) {
		if (i >= 0)
			m_iterationsImprove = i;
	}

	double coolDownFactor() const {
		return m_coolDownFactor;
	}

	double forceLimitStep() const {
		return m_forceLimitStep;
	}

	//! Returns the current setting of ideal edge length.
	double idealEdgeLength() const {
		return m_idealEdgeLength;
	}

	//! Sets the ideal edge length to \p len.
	/**
	 * Edge lengths are measured between the centers of the two nodes, i.e.,
	 * node sizes are not taken into account.
	 */
	void idealEdgeLength(double len) {
		m_idealEdgeLength = len;
	}

	//! Returns the current setting of noise.
	bool noise() const {
		return m_noise;
	}

	//! Sets the parameter noise to \p on.
	void noise(bool on) {
		m_noise = on;
	}

	//! Returns the minimum distance between connected components.
	double minDistCC() const { return m_minDistCC; }

	//! Sets the minimum distance between connected components to \p x.
	void minDistCC(double x) { m_minDistCC = x; }

	//! Returns the page ratio.
	double pageRatio() { return m_pageRatio; }

	//! Sets the page ration to \p x.
	void pageRatio(double x) { m_pageRatio = x; }

	//! Returns the current scaling method.
	Scaling scaling() const {
		return m_scaling;
	}

	//! Sets the method for scaling the inital layout to \p sc.
	void scaling(Scaling sc) {
		m_scaling = sc;
	}

	//! Returns the current scale function factor.
	double scaleFunctionFactor() const {
		return m_scaleFactor;
	}

	//! Sets the scale function factor to \p f.
	void scaleFunctionFactor(double f) {
		m_scaleFactor = f;
	}

	//! Sets the user bounding box (used if scaling method is scUserBoundingBox).
	void userBoundingBox(double xmin, double ymin, double xmax, double ymax) {
		m_userBoundingBox = DRect(xmin, ymin, xmax, ymax);
	}

	//! Gets the user bounding box.
	DRect userBoundingBox() const {
		return m_userBoundingBox;
	}

	//! Returns the maximal number of used threads.
	unsigned int maxThreads() const { return m_maxThreads; }

	//! Sets the maximal number of used threads to \p n.
	void maxThreads(unsigned int n) { m_maxThreads = n; }

protected:
	virtual void callMaster(const GraphCopy& copy, GraphAttributes& attr, DPoint& box) = 0;

	int    m_iterations;         //!< The number of iterations.
	int    m_iterationsImprove;  //!< The number of iterations for the improvement phase.
	double m_idealEdgeLength;    //!< The ideal edge length.
	double m_coolDownFactor;
	double m_forceLimitStep;

	DRect m_boundingBox;

	SpringForceModel m_forceModel; //! The used force model.
	SpringForceModel m_forceModelImprove; //! The used force model for the improvement phase.
	bool m_noise;            //!< Perform random perturbations?

	Scaling m_scaling;    //!< The scaling method.
	double m_scaleFactor; //!< The factor used if scaling type is scScaleFunction.

	DRect m_userBoundingBox;

	double m_minDistCC; //!< The minimal distance between connected components.
	double m_pageRatio; //!< The page ratio.

	double m_avgConvergenceFactor; //!< convergence if avg. displacement is at most this factor times ideal edge length
	double m_maxConvergenceFactor; //!< convergence if max. displacement is at most this factor times ideal edge length

	unsigned int m_maxThreads; //!< The maximal number of used threads.
};

}}
