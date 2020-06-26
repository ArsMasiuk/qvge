/** \file
 * \brief Declaration and definition of ogdf::spring_embedder::MasterBase.
 *
 * \author Carsten Gutwenger, Stephan Beyer
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

#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/Barrier.h>
#include <ogdf/energybased/spring_embedder/SpringEmbedderBase.h>

namespace ogdf {
namespace spring_embedder {

//! Base class for ogdf::SpringEmbedderGridVariant::Master.
template<typename NodeInfo, typename ForceModelBase>
class MasterBase {
protected:
	const SpringEmbedderBase &m_spring;
	const GraphCopy &m_gc;
	GraphAttributes &m_ga;
	DPoint &m_boundingBox;

	NodeArray<int> m_index;
	Array<NodeInfo> m_vInfo;
	Array<DPoint> m_disp;
	Array<int> m_adjLists;

	ForceModelBase *m_forceModel;
	ForceModelBase *m_forceModelImprove;

	Barrier        *m_barrier;

	double m_idealEdgeLength;

	double m_tNull;
	double m_cF;
	double m_t;
	double m_coolingFactor;

	double m_avgDisplacement;
	double m_maxDisplacement;
	double m_scaleFactor;

public:
	MasterBase(const SpringEmbedderBase &spring, const GraphCopy &gc, GraphAttributes &ga, DPoint &boundingBox)
	  : m_spring(spring)
	  , m_gc(gc)
	  , m_ga(ga)
	  , m_boundingBox(boundingBox)
	  , m_index(gc)
	  , m_vInfo(gc.numberOfNodes())
	  , m_disp(gc.numberOfNodes())
	  , m_adjLists(2*gc.numberOfEdges())
	  , m_forceModel(nullptr)
	  , m_forceModelImprove(nullptr)
	  , m_barrier(nullptr)
	  , m_avgDisplacement(std::numeric_limits<double>::max())
	  , m_maxDisplacement(std::numeric_limits<double>::max())
	{
	}

	~MasterBase()
	{
		delete m_barrier;
		delete m_forceModel;
		delete m_forceModelImprove;
	}

	int numberOfNodes() const { return m_vInfo.size(); }
	int numberOfIterations() const { return m_spring.iterations(); }
	int numberOfIterationsImprove() const { return m_spring.iterationsImprove(); }

	void initUnfoldPhase()
	{
		// cool down
		m_t = m_tNull = 0.25 * m_idealEdgeLength * sqrt(numberOfNodes());
		m_cF = 2.0;
		m_coolingFactor = m_spring.coolDownFactor();

		// convergence
		m_avgDisplacement = std::numeric_limits<double>::max();
		m_maxDisplacement = std::numeric_limits<double>::max();
	}

	void initImprovementPhase()
	{
		// cool down
		m_t  = m_tNull;
		m_cF = 2.0;
		m_coolingFactor = m_spring.coolDownFactor();

		// convergence
		m_avgDisplacement = std::numeric_limits<double>::max();
		m_maxDisplacement = std::numeric_limits<double>::max();
	}

	void coolDown()
	{
		m_cF += m_spring.forceLimitStep();
		m_t = m_tNull / std::log2(m_cF);

		m_coolingFactor *= m_spring.coolDownFactor();
	}

	double maxForceLength() const { return m_t; }
	double coolingFactor() const { return m_coolingFactor; }

	double idealEdgeLength() const { return m_idealEdgeLength; }
	bool noise() const { return m_spring.noise(); }

	const GraphCopy &getGraph() const { return m_gc; }
	GraphAttributes &getAttributes() { return m_ga; }

	const NodeArray<int> &index() const { return m_index; }
	Array<NodeInfo> &vInfo() { return m_vInfo; }
	Array<DPoint> &disp() { return m_disp; }
	Array<int> &adjLists() { return m_adjLists; }

	const ForceModelBase &forceModel() const { return *m_forceModel; }
	const ForceModelBase &forceModelImprove() const { return *m_forceModelImprove; }

	void syncThreads() {
		if(m_barrier)
			m_barrier->threadSync();
	}

	double scaleFactor() const { return m_scaleFactor; }

	bool hasConverged() const {
		return m_avgDisplacement <= m_spring.avgConvergenceFactor() * m_idealEdgeLength
		    && m_maxDisplacement <= m_spring.maxConvergenceFactor() * m_idealEdgeLength;
	}

	double avgDisplacement() const { return m_avgDisplacement; }
	double maxDisplacement() const { return m_maxDisplacement; }
};

}
}
