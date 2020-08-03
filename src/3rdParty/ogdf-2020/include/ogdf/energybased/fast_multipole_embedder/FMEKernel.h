/** \file
 * \brief Declaration of FME kernel.
 *
 * \author Martin Gronemann
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
#include <ogdf/energybased/fast_multipole_embedder/FastUtils.h>
#include <ogdf/energybased/fast_multipole_embedder/ArrayGraph.h>
#include <ogdf/energybased/fast_multipole_embedder/FMEThread.h>
#include <list>

namespace ogdf {
namespace fast_multipole_embedder {

class FMEKernel
{
public:
	explicit FMEKernel(FMEThread* pThread) : m_pThread(pThread) { }

	inline void sync() { m_pThread->sync(); }

	//! returns the index of the thread ( 0.. numThreads()-1 )
	inline uint32_t threadNr() const { return m_pThread->threadNr(); }

	//! returns the total number of threads in the pool
	inline uint32_t numThreads() const { return m_pThread->numThreads(); }

	//! returns true if this is the main thread ( the main thread is always the first thread )
	inline bool isMainThread() const { return m_pThread->isMainThread(); }

	//! returns true if this run only uses one thread )
	inline bool isSingleThreaded() const { return m_pThread->numThreads() == 1; };

private:
	FMEThread* m_pThread;
};


#define OGDF_FME_KERNEL_USE_OLD

#define OGDF_FME_KERNEL_COMPUTE_FORCE_PROTECTION_FACTOR 0.25f
// makro for force computation via SSE   s / max(s*0.5, (dx*dx + dy*dy))
#define OGDF_FME_KERNEL_MM_COMPUTE_FORCE(dx,dy,s) \
	_mm_div_ps((s), \
	           _mm_max_ps(_mm_mul_ps((s), \
	                                 _mm_set1_ps(OGDF_FME_KERNEL_COMPUTE_FORCE_PROTECTION_FACTOR)), \
	                      _mm_add_ps(_mm_mul_ps((dx),(dx)), \
	                                 _mm_mul_ps((dy),(dy)))))
#define OGDF_FME_KERNEL_COMPUTE_FORCE(dx,dy,s) \
	(s/(max<float>(s*OGDF_FME_KERNEL_COMPUTE_FORCE_PROTECTION_FACTOR, (dx)*(dx) + (dy)*(dy))))

inline double move_nodes(float* x, float* y, const uint32_t begin, const uint32_t end, const float* fx, const float* fy, const float t)
{
	double dsq_max = 0.0;
	for (uint32_t i=begin; i <= end; i++)
	{
		double dsq = fx[i]*fx[i] + fy[i]*fy[i];
		x[i] += fx[i]*t;
		y[i] += fy[i]*t;
		Math::updateMax(dsq_max, dsq);
	}
	return dsq_max;
}


inline void eval_edges(const ArrayGraph& graph, const uint32_t begin, const uint32_t end, float* fx, float* fy)
{
	const float* x = graph.nodeXPos();
	const float* y = graph.nodeYPos();
	const float* e = graph.desiredEdgeLength();
	for (uint32_t i=begin; i <= end; i++)
	{
		const auto& e_info = graph.edgeInfo(i);
		const uint32_t a = e_info.a;
		const uint32_t b = e_info.b;
		const auto& a_info = graph.nodeInfo(a);
		const auto& b_info = graph.nodeInfo(b);

		float dx = x[a] - x[b];
		float dy = y[a] - y[b];
		float dsq = dx*dx + dy*dy;
		float f = dsq == 0 ? 0 : (logf(dsq)*0.5f-logf(e[i])) * 0.25f;
		float fa = (float)(f/((float)a_info.degree));
		float fb = (float)(f/((float)b_info.degree));
		fx[a] -= dx*fa;
		fy[a] -= dy*fa;
		fx[b] += dx*fb;
		fy[b] += dy*fb;
	}
}


//! kernel function to evaluate forces between n points with coords x, y directly. result is stored in fx, fy
inline void eval_direct(float* x, float* y, float* s, float* fx, float* fy, size_t n)
{
	for (uint32_t i=0; i < n; i++)
	{
		for (uint32_t j=i+1; j < n; j++)
		{
			float dx = x[i] - x[j];
			float dy = y[i] - y[j];
#ifdef OGDF_FME_KERNEL_USE_OLD
			float s_sum = s[i]+s[j];
#else
			float s_sum = s[i]*s[j];
#endif
			float f = OGDF_FME_KERNEL_COMPUTE_FORCE(dx, dy, s_sum);
			fx[i] += dx*f;
			fy[i] += dy*f;
			fx[j] -= dx*f;
			fy[j] -= dy*f;
		}
	}
}


//! kernel function to evaluate forces between two sets of points with coords x1, y1 (x2, y2) directly. result is stored in fx1, fy1 (fx2, fy2
inline void eval_direct(float* x1, float* y1, float* s1, float* fx1, float* fy1, size_t n1,
						float* x2, float* y2, float* s2, float* fx2, float* fy2, size_t n2)
{
	for (uint32_t i=0; i < n1; i++)
	{
		for (uint32_t j=0; j < n2; j++)
		{
			float dx = x1[i] - x2[j];
			float dy = y1[i] - y2[j];
#ifdef OGDF_FME_KERNEL_USE_OLD
			float s_sum = s1[i]+s2[j];
#else
			float s_sum = s1[i]*s2[j];
#endif
			float f = OGDF_FME_KERNEL_COMPUTE_FORCE(dx, dy, s_sum);
			fx1[i] += dx*f;
			fy1[i] += dy*f;
			fx2[j] -= dx*f;
			fy2[j] -= dy*f;
		}
	}
}


#ifndef OGDF_FME_KERNEL_USE_SSE_DIRECT
//! kernel function to evaluate forces between n points with coords x, y directly. result is stored in fx, fy
inline void eval_direct_fast(float* x, float* y, float* s, float* fx, float* fy, size_t n)
{
	eval_direct(x, y, s, fx, fy, n);
}

//! kernel function to evaluate forces between two sets of points with coords x1, y1 (x2, y2) directly. result is stored in fx1, fy1 (fx2, fy2
inline void eval_direct_fast(float* x1, float* y1, float* s1, float* fx1, float* fy1, size_t n1,
                             float* x2, float* y2, float* s2, float* fx2, float* fy2, size_t n2)
{
	eval_direct(x1, y1, s1, fx1, fy1, n1,
	            x2, y2, s2, fx2, fy2, n2);
}

#else
//! kernel function to evaluate forces between n points with coords x, y directly. result is stored in fx, fy
void eval_direct_fast(float* x, float* y, float* s, float* fx, float* fy, size_t n);
//! kernel function to evaluate forces between two sets of points with coords x1, y1 (x2, y2) directly. result is stored in fx1, fy1 (fx2, fy2
void eval_direct_fast(
	float* x1, float* y1, float* s1, float* fx1, float* fy1, size_t n1,
	float* x2, float* y2, float* s2, float* fx2, float* fy2, size_t n2);
#endif

//! kernel function to evalute a local expansion at point x,y result is added to fx, fy
void fast_multipole_l2p(double* localCoeffiecients, uint32_t numCoeffiecients, double centerX, double centerY,
									float x, float y, float q, float& fx, float &fy);

void fast_multipole_p2m(double* mulitCoeffiecients, uint32_t numCoeffiecients, double centerX, double centerY,
									float x, float y, float q);

class FMEBasicKernel
{
public:
	inline void edgeForces(const ArrayGraph& graph, float* fx, float* fy)
	{
		eval_edges(graph, 0, graph.numEdges()-1, fx, fy);
	}

	inline void repForces(ArrayGraph& graph, float* fx, float* fy)
	{
		eval_direct_fast(graph.nodeXPos(), graph.nodeYPos(), graph.nodeSize(), fx, fy,  graph.numNodes());
	}

	inline double moveNodes(ArrayGraph& graph, float* fx, float* fy, float timeStep)
	{
		return move_nodes(graph.nodeXPos(), graph.nodeYPos(), 0, graph.numNodes()-1, fx, fy, timeStep);
	}

	inline double simpleIteration(ArrayGraph& graph, float* fx, float* fy, float timeStep)
	{
		repForces(graph, fx, fy);
		edgeForces(graph, fx, fy);
		return moveNodes(graph, fx, fy, timeStep);
	}


	inline double simpleEdgeIteration(ArrayGraph& graph, float* fx, float* fy, float timeStep)
	{
		edgeForces(graph, fx, fy);
		return moveNodes(graph, fx, fy, timeStep);
	}


	inline void simpleForceDirected(ArrayGraph& graph, float timeStep, uint32_t minIt, uint32_t maxIt, uint32_t preProcIt, double threshold)
	{
		bool earlyExit = false;
		float* fx = (float*)OGDF_MALLOC_16(sizeof(float)*graph.numNodes());
		float* fy = (float*)OGDF_MALLOC_16(sizeof(float)*graph.numNodes());

		for (uint32_t i = 0; i<preProcIt; i++)
		{
			for (uint32_t j = 0; j<graph.numNodes(); j++)
			{
				fx[j] = 0.0f;
				fy[j] = 0.0f;
			}
			simpleEdgeIteration(graph, fx, fy, timeStep);
		}
		for(uint32_t i = 0; i < maxIt && !earlyExit; i++)
		{
			for (uint32_t j = 0; j<graph.numNodes(); j++)
			{
				fx[j] = 0.0f;
				fy[j] = 0.0f;
			}
			double dsq = simpleIteration(graph, fx, fy, timeStep);
			if (dsq < threshold && i>(minIt))
				earlyExit = true;
		}

		OGDF_FREE_16(fx);
		OGDF_FREE_16(fy);
	}

private:
};


class FMESingleKernel : FMEBasicKernel
{
public:
#if 0
	FMESingleKernel(FMEThread* pThread) : FMEKernel(pThread) {};
#endif

	void operator()(ArrayGraph& graph, float timeStep, uint32_t minIt, uint32_t maxIt, double threshold)
	{
		simpleForceDirected(graph, timeStep, minIt, maxIt, 20, threshold);
	}
};

}
}
