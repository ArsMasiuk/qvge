/** \file
 * \brief Declaration of class FMEMultipoleKernel.
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

#include <ogdf/energybased/fast_multipole_embedder/FMEKernel.h>
#include <ogdf/energybased/fast_multipole_embedder/FMEFunc.h>

namespace ogdf {
namespace fast_multipole_embedder {

struct ArrayPartition
{
	uint32_t begin;
	uint32_t end;

	template<typename Func>
	void for_loop(Func& func)
	{
		for(uint32_t i=begin; i<=end; i++) func(i);
	}
};


class FMEMultipoleKernel : public FMEKernel
{
public:
	explicit FMEMultipoleKernel(FMEThread* pThread) : FMEKernel(pThread) { }

	//! allocate the global and local contexts used by an instance of this kernel
	static FMEGlobalContext* allocateContext(ArrayGraph* pGraph, FMEGlobalOptions* pOptions, uint32_t numThreads);

	//! free the global and local context
	static void deallocateContext(FMEGlobalContext* globalContext);

	//! sub procedure for quadtree construction
	void quadtreeConstruction(ArrayPartition& nodePointPartition);

	//! the single threaded version without fences
	void multipoleApproxSingleThreaded(ArrayPartition& nodePointPartition);

	//! the original algorithm which runs the WSPD completely single threaded
	void multipoleApproxSingleWSPD(ArrayPartition& nodePointPartition);

	//! new but slower method, parallel wspd computation without using the wspd structure
	void multipoleApproxNoWSPDStructure(ArrayPartition& nodePointPartition);

	//! the final version, the wspd structure is only used for the top of the tree
	void multipoleApproxFinal(ArrayPartition& nodePointPartition);

	//! main function of the kernel
	void operator()(FMEGlobalContext* globalContext);

	//! creates an array partition with a default chunksize of 16
	inline ArrayPartition arrayPartition(uint32_t n)
	{
		return arrayPartition(n, threadNr(), numThreads(), 16);
	}

	//! returns an array partition for the given threadNr and thread count
	inline ArrayPartition arrayPartition(uint32_t n, uint32_t threadNr, uint32_t numThreads, uint32_t chunkSize)
	{
		ArrayPartition result;
		if (!n)
		{
			result.begin = 1;
			result.end = 0;
			return result;
		}
		if (n>=numThreads*chunkSize)
		{
			uint32_t s = n/(numThreads*chunkSize);
			uint32_t o = s*chunkSize*threadNr;
			if (threadNr == numThreads-1)
			{
				result.begin = o;
				result.end = n-1;
			}
			else
			{
				result.begin = o;
				result.end = o+s*chunkSize-1;
			}
		} else
		{
			if (threadNr == 0)
			{
				result.begin = 0;
				result.end = n-1;
			} else
			{
				result.begin = 1;
				result.end = 0;
			}
		}
		return result;
	}

	//! for loop on a partition
	template<typename F>
	inline void for_loop(const ArrayPartition& partition, F func)
	{
		if (partition.begin > partition.end)
			return;
		for (uint32_t i=partition.begin; i<= partition.end; i++) func(i);
	}

	//! for loop on the tree partition
	template<typename F>
	inline void for_tree_partition(F functor)
	{
		for (LinearQuadtree::NodeID id : m_pLocalContext->treePartition.nodes)
		{
			functor(id);
		}
	}

	//! sorting single threaded
	template<typename T, typename C>
	inline void sort_single(T* ptr, uint32_t n, C comparer)
	{
		if (isMainThread())
		{
			std::sort(ptr, ptr + n, comparer);
		}
	}

	//! lazy parallel sorting for num_threads = power of two
	template<typename T, typename C>
	inline void sort_parallel(T* ptr, uint32_t n, C comparer)
	{
		if (n < numThreads() * 1000 || numThreads() == 1)
			sort_single(ptr, n, comparer);
		else
			sort_parallel(ptr, n, comparer, 0, numThreads());
	}

	//! lazy parallel sorting for num_threads = power of two
	template<typename T, typename C>
	inline void sort_parallel(T* ptr, uint32_t n, C comparer, uint32_t threadNrBegin, uint32_t numThreads)
	{
		if (n <= 1) return;
		if (numThreads == 1)
		{
			std::sort(ptr, ptr + n, comparer);
		}
		else
		{
			uint32_t half = n >> 1;
			uint32_t halfThreads = numThreads >> 1;
			if (this->threadNr() < threadNrBegin + halfThreads)
				sort_parallel(ptr, half, comparer, threadNrBegin, halfThreads);
			else
				sort_parallel(ptr + half, n - half, comparer, threadNrBegin + halfThreads, halfThreads);

			// wait until all threads are ready.
			sync();
			if (this->threadNr() == threadNrBegin)
				std::inplace_merge(ptr, ptr + half, ptr + n, comparer);
		}
	}

private:
	FMEGlobalContext* m_pGlobalContext = nullptr;
	FMELocalContext* m_pLocalContext = nullptr;
};

}
}
