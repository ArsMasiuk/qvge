/** \file
 * \brief Declaration and definition of ogdf::spring_embedder::WorkerBase.
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

#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/Barrier.h>
#include <ogdf/energybased/spring_embedder/SpringEmbedderBase.h>
#include <ogdf/basic/Math.h>

namespace ogdf {
namespace spring_embedder {

//! Base class for ogdf::SpringEmbedderGridVariant::Worker.
template<class Master, class NodeInfo>
class WorkerBase {
public:
	WorkerBase(unsigned int id, Master &master, int vStartIndex, int vStopIndex, node vStart, node vStop)
			: m_id(id), m_master(master), m_vStartIndex(vStartIndex), m_vStopIndex(vStopIndex), m_vStart(vStart), m_vStop(vStop) { }

	virtual ~WorkerBase() = default;

	virtual void operator()() = 0;

protected:
	unsigned int m_id;
	Master &m_master;

	int m_vStartIndex;
	int m_vStopIndex;
	node m_vStart;
	node m_vStop;

	double m_wsum;
	double m_hsum;
	double m_xmin;
	double m_xmax;
	double m_ymin;
	double m_ymax;

	double m_sumForces;
	double m_maxForce;
	double m_sumLengths;

	void finalScaling(Array<NodeInfo> &vInfo, const Array<int> &adjLists) {
		m_sumLengths = sumUpLengths(vInfo, adjLists);

		m_master.syncThreads();

		if(m_id == 0)
			m_master.scaleLayout(m_sumLengths);

		m_master.syncThreads();

		double s = m_master.scaleFactor();

		const GraphCopy &gc = m_master.getGraph();
		GraphAttributes &ga = m_master.getAttributes();

		double xmin = std::numeric_limits<double>::max(), xmax = std::numeric_limits<double>::lowest();
		double ymin = std::numeric_limits<double>::max(), ymax = std::numeric_limits<double>::lowest();

		node v = m_vStart;
		for(int j = m_vStartIndex; j < m_vStopIndex; ++j) {
			node vOrig = gc.original(v);
			NodeInfo &vj = vInfo[j];

			double xv = s * vj.m_pos.m_x;
			double yv = s * vj.m_pos.m_y;
			vj.m_pos.m_x = xv;
			vj.m_pos.m_y = yv;

			double wv = ga.width(vOrig);
			double hv = ga.height(vOrig);

			Math::updateMin(xmin, xv-0.5*wv);
			Math::updateMax(xmax, xv+0.5*wv);
			Math::updateMin(ymin, yv-0.5*hv);
			Math::updateMax(ymax, yv+0.5*hv);
		}

		m_xmin = xmin; m_xmax = xmax;
		m_ymin = ymin; m_ymax = ymax;

		m_master.syncThreads();
	}

	void scaling(Array<NodeInfo> &vInfo, const Array<int> &adjLists) {
		m_sumLengths = sumUpLengths(vInfo, adjLists);

		m_master.syncThreads();

		if(m_id == 0)
			m_master.scaleLayout(m_sumLengths);

		m_master.syncThreads();

		double s = m_master.scaleFactor();
		for(int j = m_vStartIndex; j < m_vStopIndex; ++j)
			vInfo[j].m_pos *= s;

		if(m_id == 0)
			m_master.initImprovementPhase();

		m_master.syncThreads();
	}

	double sumUpLengths(Array<NodeInfo> &vInfo, const Array<int> &adjLists) {
		double sumLengths = 0.0;
		for(int j = m_vStartIndex; j < m_vStopIndex; ++j) {
			const NodeInfo &vj = vInfo[j];
			for(int k = vj.m_adjBegin; k != vj.m_adjStop; ++k) {
				int u = adjLists[k];
				if(u < j) {
					DPoint dist = vj.m_pos - vInfo[u].m_pos;
					sumLengths += dist.norm();
				}
			}
		}

		return sumLengths;
	}
};

}
}
