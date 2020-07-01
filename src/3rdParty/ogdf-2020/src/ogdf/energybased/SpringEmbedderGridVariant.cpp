/** \file
 * \brief Implementation of ogdf::SpringEmbedderGridVariant.
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

#include <ogdf/energybased/spring_embedder/SEGV_ForceModel.h>
#include <ogdf/energybased/spring_embedder/MasterBase.h>
#include <ogdf/energybased/spring_embedder/WorkerBase.h>

#include <ogdf/packing/TileToRowsCCPacker.h>
#include <ogdf/basic/Math.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Thread.h>
#include <ogdf/fileformats/GraphIO.h>

using std::minstd_rand;
using std::uniform_real_distribution;
using ogdf::Math::updateMax;
using ogdf::Math::updateMin;

namespace ogdf {

class SpringEmbedderGridVariant::Master : public spring_embedder::MasterBase<NodeInfo, ForceModelBase> {
	Array<Worker*> m_worker;
	Array2D<ListPure<int>> m_gridCell;

	double m_k2;

	double m_xmin;
	double m_xmax;
	double m_ymin;
	double m_ymax;

public:
	Master(const SpringEmbedderGridVariant &spring, const GraphCopy &gc, GraphAttributes &ga, DPoint &boundingBox);

	double xmin() const { return m_xmin; }
	double ymin() const { return m_ymin; }

	double boxLength() const { return m_k2; }
	Array2D<ListPure<int>> &gridCell() { return m_gridCell; }

	void initialize(double wsum, double hsum, double xmin, double xmax, double ymin, double ymax);
	void updateGridAndMoveNodes();
	void scaleLayout(double sumLengths);
	void computeFinalBB();
};


class SpringEmbedderGridVariant::Worker : spring_embedder::WorkerBase<Master, NodeInfo> {
	friend class SpringEmbedderGridVariant;
private:
	int m_eStartIndex;

public:
	Worker(unsigned int id, Master &master, int vStartIndex, int vStopIndex, node vStart, node vStop, int eStartIndex)
		: WorkerBase(id, master, vStartIndex, vStopIndex, vStart, vStop), m_eStartIndex(eStartIndex) {}
	void operator()() override;
};

SpringEmbedderGridVariant::Master::Master(const SpringEmbedderGridVariant &spring, const GraphCopy &gc, GraphAttributes &ga, DPoint &boundingBox)
  : spring_embedder::MasterBase<NodeInfo, ForceModelBase>(spring, gc, ga, boundingBox)
{
	const unsigned int minNodesPerThread = 64;
	const unsigned int n = gc.numberOfNodes();

	unsigned int nThreads = max( 1u, min(spring.m_maxThreads, (n/4) / (minNodesPerThread/4)) );
	m_worker.init(nThreads);

	if(nThreads == 1) {
		int nextIndex = 0;
		for(node v : gc.nodes)
			m_index[v] = nextIndex++;

		m_worker[0] = new Worker(0, *this, 0, n, gc.firstNode(), nullptr, 0);
		(*m_worker[0])();
	} else {
		unsigned int nodesPerThread = 4*((n/4) / nThreads);

		Array<node> startNode(nThreads+1);
		Array<int> startIndex(nThreads+1);
		Array<int> eStartIndex(nThreads+1);

		int nextIndex = 0, j = 0, t = 0;
		for(node v : gc.nodes) {
			if(nextIndex % nodesPerThread == 0) {
				startNode [t] = v;
				startIndex[t] = nextIndex;
				eStartIndex [t] = j;
				++t;
			}
			m_index[v] = nextIndex++;
			j += v->degree();
		}

		startNode [nThreads] = nullptr;
		startIndex[nThreads] = gc.numberOfNodes();

		m_barrier = new Barrier(nThreads);
		Array<Thread> thread(nThreads-1);

		for(unsigned int i = 1; i < nThreads; ++i) {
			m_worker[i] =
				new Worker(i, *this, startIndex[i], startIndex[i+1], startNode[i], startNode[i+1], eStartIndex[i]);
			thread [i-1] = Thread(*m_worker[i]);
		}

		m_worker[0] = new Worker(0, *this, startIndex[0], startIndex[1], startNode[0], startNode[1], eStartIndex[0]);
		(*m_worker[0])();

		for(unsigned int i = 1; i < nThreads; ++i) {
			thread[i-1].join();
			delete m_worker[i];
		}
	}

	delete m_worker[0];
}

void SpringEmbedderGridVariant::Master::initialize(double wsum, double hsum, double xmin, double xmax, double ymin, double ymax) {
	const int n = m_gc.numberOfNodes();

	for(int t = 1; t <= m_worker.high(); ++t) {
		updateMin(xmin, m_worker[t]->m_xmin);
		updateMax(xmax, m_worker[t]->m_xmax);
		updateMin(ymin, m_worker[t]->m_ymin);
		updateMax(ymax, m_worker[t]->m_ymax);
		wsum += m_worker[t]->m_wsum;
		hsum += m_worker[t]->m_hsum;
	}

	Scaling scaling = m_spring.scaling();
	m_idealEdgeLength = m_spring.idealEdgeLength();

	// handle special case of zero area bounding box
	if(xmin == xmax || ymin == ymax) {
		if(scaling == Scaling::userBoundingBox) {
			const DRect& box = m_spring.userBoundingBox();
			m_xmin = box.p1().m_x;
			m_xmax = box.p2().m_x;
			m_ymin = box.p1().m_y;
			m_ymax = box.p2().m_y;
		} else {
			m_idealEdgeLength = max(1e-3, m_spring.idealEdgeLength());
			m_xmin = m_ymin = 0;
			m_xmax = m_ymax = m_idealEdgeLength * sqrt(double(n));
		}

		minstd_rand rng(randomSeed());
		uniform_real_distribution<> rand_x(m_xmin ,m_xmax);
		uniform_real_distribution<> rand_y(m_ymin,m_ymax);

		for(int j = 0; j < n; ++j) {
			m_vInfo[j].m_pos.m_x = rand_x(rng);
			m_vInfo[j].m_pos.m_y = rand_y(rng);
		}

	} else {
		double scaleFactor = m_spring.scaleFunctionFactor();

		switch(scaling) {
		case Scaling::input:
			m_xmin = xmin;
			m_xmax = xmax;
			m_ymin = ymin;
			m_ymax = ymax;
			break;

		case Scaling::userBoundingBox:
		case Scaling::scaleFunction:
		case Scaling::useIdealEdgeLength:

			if(scaling == Scaling::userBoundingBox) {
				const DRect& box = m_spring.userBoundingBox();
				m_xmin = box.p1().m_x;
				m_xmax = box.p2().m_x;
				m_ymin = box.p1().m_y;
				m_ymax = box.p2().m_y;
			} else if(scaling == Scaling::scaleFunction) {
				double sqrt_n = sqrt((double)n);
				m_xmin = m_ymin = 0;
				m_xmax = (wsum > 0) ? scaleFactor * wsum / sqrt_n : 1;
				m_ymax = (hsum > 0) ? scaleFactor * hsum / sqrt_n : 1;
			} else {
				m_idealEdgeLength = max(1e-3, m_spring.idealEdgeLength());
				double w = xmax - xmin;
				double h = ymax - ymin;
				double r = (w > 0) ? h / w : 1.0;
				m_xmin = m_ymin = 0;
				m_xmax = m_idealEdgeLength * sqrt(double(n) / r);
				m_ymax = r * m_xmax;
			}
			// Compute scaling such that layout coordinates fit into used bounding box
			double fx = (xmax == xmin) ? 1.0 : m_xmax / (xmax - xmin);
			double fy = (ymax == ymin) ? 1.0 : m_ymax / (ymax - ymin);

			// Adjust coordinates accordingly
			for(int j = 0; j < n; ++j) {
				m_vInfo[j].m_pos.m_x = m_xmin + (m_vInfo[j].m_pos.m_x - xmin) * fx;
				m_vInfo[j].m_pos.m_y = m_ymin + (m_vInfo[j].m_pos.m_y - ymin) * fy;
			}
		}
	}

	double width = m_xmax - m_xmin;
	double height = m_ymax - m_ymin;

	OGDF_ASSERT(width >= 0);
	OGDF_ASSERT(height >= 0);

	initUnfoldPhase();

	double k = sqrt(width*height / n);
	m_k2 = max(1e-3, 2*k);

	if(scaling != Scaling::useIdealEdgeLength)
		m_idealEdgeLength = k;

	switch(m_spring.forceModel()) {
	case SpringForceModel::FruchtermanReingold:
		m_forceModel = new ForceModelFR(m_vInfo, m_adjLists, m_gridCell, m_idealEdgeLength);
		break;
	case SpringForceModel::FruchtermanReingoldModAttr:
		m_forceModel = new ForceModelFRModAttr(m_vInfo, m_adjLists, m_gridCell, m_idealEdgeLength);
		break;
	case SpringForceModel::FruchtermanReingoldModRep:
		m_forceModel = new ForceModelFRModRep(m_vInfo, m_adjLists, m_gridCell, m_idealEdgeLength);
		break;
	case SpringForceModel::Eades:
		m_forceModel = new ForceModelEades(m_vInfo, m_adjLists, m_gridCell, m_idealEdgeLength);
		break;
	case SpringForceModel::Hachul:
		m_forceModel = new ForceModelHachul(m_vInfo, m_adjLists, m_gridCell, m_idealEdgeLength);
		break;
	case SpringForceModel::Gronemann:
		m_forceModel = new ForceModelGronemann(m_vInfo, m_adjLists, m_gridCell, m_idealEdgeLength);
		break;
	}

	switch(m_spring.forceModelImprove()) {
	case SpringForceModel::FruchtermanReingold:
		m_forceModelImprove = new ForceModelFR(m_vInfo, m_adjLists, m_gridCell, m_idealEdgeLength);
		break;
	case SpringForceModel::FruchtermanReingoldModAttr:
		m_forceModelImprove = new ForceModelFRModAttr(m_vInfo, m_adjLists, m_gridCell, m_idealEdgeLength);
		break;
	case SpringForceModel::FruchtermanReingoldModRep:
		m_forceModelImprove = new ForceModelFRModRep(m_vInfo, m_adjLists, m_gridCell, m_idealEdgeLength);
		break;
	case SpringForceModel::Eades:
		m_forceModelImprove = new ForceModelEades(m_vInfo, m_adjLists, m_gridCell, m_idealEdgeLength);
		break;
	case SpringForceModel::Hachul:
		m_forceModelImprove = new ForceModelHachul(m_vInfo, m_adjLists, m_gridCell, m_idealEdgeLength);
		break;
	case SpringForceModel::Gronemann:
		m_forceModelImprove = new ForceModelGronemann(m_vInfo, m_adjLists, m_gridCell, m_idealEdgeLength);
		break;
	}

	// build grid cells
	int xA = int(width / m_k2 + 2);
	int yA = int(height / m_k2 + 2);
	m_gridCell.init(-1,xA,-1,yA);

	for(int j = 0; j < n; ++j) {
		NodeInfo &vj = m_vInfo[j];

		vj.m_gridX = int((vj.m_pos.m_x - m_xmin ) / m_k2);
		vj.m_gridY = int((vj.m_pos.m_y - m_ymin) / m_k2);

		OGDF_ASSERT(vj.m_gridX < xA);
		OGDF_ASSERT(vj.m_gridX > -1);
		OGDF_ASSERT(vj.m_gridY < yA);
		OGDF_ASSERT(vj.m_gridY > -1);

		m_vInfo[j].m_lit = m_gridCell(vj.m_gridX,vj.m_gridY).pushFront(j);
	}
}

void SpringEmbedderGridVariant::Master::updateGridAndMoveNodes() {
	const Worker &worker = *m_worker[0];

	double xmin = worker.m_xmin;
	double xmax = worker.m_xmax;
	double ymin = worker.m_ymin;
	double ymax = worker.m_ymax;
	double maxForce = worker.m_maxForce;
	double sumForces = worker.m_sumForces;

	for(int t = 1; t <= m_worker.high(); ++t) {
		updateMin(xmin, m_worker[t]->m_xmin);
		updateMax(xmax, m_worker[t]->m_xmax);
		updateMin(ymin, m_worker[t]->m_ymin);
		updateMax(ymax, m_worker[t]->m_ymax);
		updateMax(maxForce, m_worker[t]->m_maxForce);
		sumForces += m_worker[t]->m_sumForces;
	}

	m_avgDisplacement = sumForces / numberOfNodes();

	const int xA = m_gridCell.high1();
	const int yA = m_gridCell.high2();

	// prevent drawing area from getting too small
	double hMargin = 0.5 * max(0.0, m_idealEdgeLength * xA - (xmax-xmin));
	double vMargin = 0.5 * max(0.0, m_idealEdgeLength * yA - (ymax-ymin));

	// set new values
	m_xmin = xmin - hMargin;
	m_xmax = xmax + hMargin;
	m_ymin = ymin - vMargin;
	m_ymax = ymax + vMargin;

	m_k2 = max( (m_xmax-m_xmin) / (xA-1), (m_ymax-m_ymin) / (yA-1) );

	// move nodes
	for(int j = 0; j <= m_vInfo.high(); ++j) {
		NodeInfo &vj = m_vInfo[j];

		// new position
		vj.m_pos += m_disp[j];

		// new cell
		int grid_x = int((vj.m_pos.m_x - m_xmin ) / m_k2);
		int grid_y = int((vj.m_pos.m_y - m_ymin) / m_k2);

		OGDF_ASSERT(grid_x >= 0);
		OGDF_ASSERT(grid_x < m_gridCell.high1());
		OGDF_ASSERT(grid_y >= 0);
		OGDF_ASSERT(grid_y < m_gridCell.high2());

		// move to different cell?
		if( (grid_x != vj.m_gridX) || (grid_y != vj.m_gridY) ) {
			m_gridCell(vj.m_gridX,vj.m_gridY).moveToFront(vj.m_lit, m_gridCell(grid_x,grid_y));
			vj.m_gridX = grid_x;
			vj.m_gridY = grid_y;
		}
	}
}

void SpringEmbedderGridVariant::Master::computeFinalBB() {
	double xmin = m_worker[0]->m_xmin;
	double xmax = m_worker[0]->m_xmax;
	double ymin = m_worker[0]->m_ymin;
	double ymax = m_worker[0]->m_ymax;

	for(int t = 1; t <= m_worker.high(); ++t) {
		updateMin(xmin, m_worker[t]->m_xmin);
		updateMax(xmax, m_worker[t]->m_xmax);
		updateMin(ymin, m_worker[t]->m_ymin);
		updateMax(ymax, m_worker[t]->m_ymax);
	}

	xmin -= m_spring.minDistCC();
	ymin -= m_spring.minDistCC();
	m_boundingBox = DPoint(xmax - xmin, ymax - ymin);

	m_xmin = xmin;
	m_ymin = ymin;
}

void SpringEmbedderGridVariant::Master::scaleLayout(double sumLengths) {
	for(int t = 1; t <= m_worker.high(); ++t)
		sumLengths += m_worker[t]->m_sumLengths;

	const int m = m_gc.numberOfEdges();
	m_scaleFactor = m_idealEdgeLength / sumLengths * m;

	// set new values
	m_xmin *= m_scaleFactor;
	m_xmax *= m_scaleFactor;
	m_ymin *= m_scaleFactor;
	m_ymax *= m_scaleFactor;

	m_k2 = max( (m_xmax-m_xmin) / (m_gridCell.high1()-1), (m_ymax-m_ymin) / (m_gridCell.high2()-1) );
}

void SpringEmbedderGridVariant::callMaster(const GraphCopy& copy, GraphAttributes& attr, DPoint& box) {
	Master(*this, copy, attr, box);
}

void SpringEmbedderGridVariant::Worker::operator()() {
	const double forceScaleFactor = 0.1; //0.05;

	const GraphCopy &gc = m_master.getGraph();
	GraphAttributes &ga = m_master.getAttributes();

	const NodeArray<int> &index = m_master.index();
	Array<NodeInfo> &vInfo = m_master.vInfo();
	Array<int> &adjLists = m_master.adjLists();

	// Initialize

	double wsum = 0, hsum = 0;
	double xmin = std::numeric_limits<double>::max(), xmax = std::numeric_limits<double>::lowest();
	double ymin = std::numeric_limits<double>::max(), ymax = std::numeric_limits<double>::lowest();

	int adjCounter = m_eStartIndex;
	int runnerIndex = m_vStartIndex;
	for(node v = m_vStart; v != m_vStop; v = v->succ(), ++runnerIndex) {
		node vOrig = gc.original(v);

		double x = ga.x(vOrig), y = ga.y(vOrig);
		wsum += ga.width(vOrig);
		hsum += ga.height(vOrig);

		vInfo[runnerIndex].m_pos.m_x = x;
		vInfo[runnerIndex].m_pos.m_y = y;
		if(x < xmin) xmin = x;
		if(x > xmax) xmax = x;
		if(y < ymin) ymin = y;
		if(y > ymax) ymax = y;

		vInfo[runnerIndex].m_adjBegin = adjCounter;
		for(adjEntry adj : v->adjEntries)
			adjLists[adjCounter++] = index[adj->twinNode()];
		vInfo[runnerIndex].m_adjStop = adjCounter;
	}

	m_xmin = xmin; m_xmax = xmax;
	m_ymin = ymin; m_ymax = ymax;
	m_wsum = wsum; m_hsum = hsum;

	m_master.syncThreads();

	if(m_id == 0)
		m_master.initialize(wsum, hsum, xmin, xmax, ymin, ymax);

	m_master.syncThreads();

	// Main step

	const bool noise = m_master.noise();
	Array<DPoint> &disp = m_master.disp();

	// random number generator for adding noise
	minstd_rand rng(randomSeed());
	uniform_real_distribution<> rand(0.75,1.25);

	// --- Unfold Phase ---

	const ForceModelBase &forceModel = m_master.forceModel();

	const int numIter = m_master.numberOfIterations();
	for(int iter = 1; !m_master.hasConverged() && iter <= numIter; ++iter) {
		double boxLength = m_master.boxLength();

		xmin = std::numeric_limits<double>::max(); xmax = std::numeric_limits<double>::lowest();
		ymin = std::numeric_limits<double>::max(); ymax = std::numeric_limits<double>::lowest();

		double sumForces = 0.0;
		double maxForce = 0.0;

		double t = m_master.maxForceLength();
		double f = m_master.coolingFactor() * forceScaleFactor;

		for(int j = m_vStartIndex; j < m_vStopIndex; ++j) {
			NodeInfo &vj = vInfo[j];

			DPoint dp = forceModel.computeDisplacement(j, boxLength);

			// noise
			if(noise) {
				dp.m_x *= rand(rng);
				dp.m_y *= rand(rng);
			}

			double length = dp.norm();
			sumForces += length;
			updateMax(maxForce, length);

			//double s = (length <= t) ? forceScaleFactor : forceScaleFactor * t / length;
			double s = (length <= t) ? f : f * t / length;
			dp *= s;

			DPoint newPos = vj.m_pos + dp;

			// update new bounding box
			updateMin(xmin, newPos.m_x);
			updateMax(xmax, newPos.m_x);
			updateMin(ymin, newPos.m_y);
			updateMax(ymax, newPos.m_y);

			// store displacement
			disp[j] = dp;
		}

		m_xmin = xmin; m_xmax = xmax;
		m_ymin = ymin; m_ymax = ymax;
		m_sumForces = sumForces;
		m_maxForce = maxForce;

		m_master.syncThreads();

		if(m_id == 0) {
			m_master.updateGridAndMoveNodes();
			m_master.coolDown();
		}

		m_master.syncThreads();
	}


	// --- Improvement Phase ---

	const int numIterImp = m_master.numberOfIterationsImprove();
	if(numIterImp > 0) {
		// scaling to ideal edge length
		scaling(vInfo,adjLists);

		const ForceModelBase &forceModelImprove = m_master.forceModelImprove();

		for(int iter = 1; !m_master.hasConverged() && iter <= numIterImp; ++iter) {
			double boxLength = m_master.boxLength();

			xmin = std::numeric_limits<double>::max(); xmax = std::numeric_limits<double>::lowest();
			ymin = std::numeric_limits<double>::max(); ymax = std::numeric_limits<double>::lowest();

			double sumForces = 0.0;
			double maxForce = 0.0;

			double t = m_master.maxForceLength();
			double f = m_master.coolingFactor() * forceScaleFactor;

			for(int j = m_vStartIndex; j < m_vStopIndex; ++j) {
				NodeInfo &vj = vInfo[j];

				DPoint dp = forceModelImprove.computeDisplacement(j, boxLength);

				// noise
				if(noise) {
					dp.m_x *= rand(rng);
					dp.m_y *= rand(rng);
				}

				double length = dp.norm();
				sumForces += length;
				updateMax(maxForce, length);

				//double s = (length <= t) ? forceScaleFactor : forceScaleFactor * t / length;
				double s = (length <= t) ? f : f * t / length;
				dp *= s;

				DPoint newPos = vj.m_pos + dp;

				// update new bounding box
				updateMin(xmin, newPos.m_x);
				updateMax(xmax, newPos.m_x);
				updateMin(ymin, newPos.m_y);
				updateMax(ymax, newPos.m_y);

				// store displacement
				disp[j] = dp;
			}

			m_xmin = xmin; m_xmax = xmax;
			m_ymin = ymin; m_ymax = ymax;
			m_sumForces = sumForces;
			m_maxForce = maxForce;

			m_master.syncThreads();

			// last iteration?
			if(iter == numIterImp) {

				for(int j = m_vStartIndex; j < m_vStopIndex; ++j) {
					NodeInfo &vj = vInfo[j];
					vj.m_pos += disp[j];
				}

			} else if(m_id == 0) {
				m_master.updateGridAndMoveNodes();
				m_master.coolDown();
			}

			m_master.syncThreads();
		}
	}


	// Compute final layout

	// scale layout to ideal edge length and compute bounding box
	finalScaling(vInfo, adjLists);

	if(m_id == 0)
		m_master.computeFinalBB();

	m_master.syncThreads();

	xmin = m_master.xmin();
	ymin = m_master.ymin();

	node v = m_vStart;
	for(int j = m_vStartIndex; j < m_vStopIndex; v = v->succ(), ++j) {
		node vOrig = gc.original(v);

		ga.x(vOrig) = vInfo[j].m_pos.m_x - xmin;
		ga.y(vOrig) = vInfo[j].m_pos.m_y - ymin;
	}
}

}
