/** \file
 * \brief Implementation of class FMEThread.
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

#include <ogdf/energybased/fast_multipole_embedder/FMEThread.h>

#ifdef OGDF_HAS_LINUX_CPU_MACROS
#include <sched.h>
#endif

namespace ogdf {
namespace fast_multipole_embedder {

#ifdef OGDF_HAS_LINUX_CPU_MACROS
//! helper function for setting the affinity on the test machine.
void CPU_SET_ordered_dual_quad(int cpu, cpu_set_t* set)
{
	int cpuOrdered = cpu;
	switch ( cpu )
	{
	case 0 : cpuOrdered = 0; break;
	case 1 : cpuOrdered = 2; break;
	case 2 : cpuOrdered = 4; break;
	case 3 : cpuOrdered = 6; break;

	case 4 : cpuOrdered = 1; break;
	case 5 : cpuOrdered = 3; break;
	case 6 : cpuOrdered = 5; break;
	case 7 : cpuOrdered = 7; break;
	default: cpuOrdered = 0; break;
	}
	CPU_SET(cpuOrdered, set);
}

void FMEThread::unixSetAffinity()
{
	cpu_set_t mask;
	CPU_ZERO( &mask );
	CPU_SET_ordered_dual_quad(m_threadNr*(System::numberOfProcessors()/m_numThreads), &mask);
	sched_setaffinity( 0, sizeof(mask), &mask );
}
#endif


FMEThread::FMEThread(FMEThreadPool* pThreadPool, uint32_t threadNr) : m_threadNr(threadNr), m_pThreadPool(pThreadPool)
{
	m_numThreads = m_pThreadPool->numThreads();
}


void FMEThread::sync()
{
	if (m_numThreads>1)
		m_pThreadPool->syncBarrier()->threadSync();
}



FMEThreadPool::FMEThreadPool(uint32_t numThreads) : m_numThreads(numThreads)
{
	allocate();
}

FMEThreadPool::~FMEThreadPool()
{
	deallocate();
}

//! runs one iteration. This call blocks the main thread
void FMEThreadPool::runThreads()
{
	uint32_t nThreads = numThreads();

	Array<Thread> threads(1, nThreads);

	for (uint32_t i=1; i < nThreads; i++)
	{
		threads[i] = Thread(*thread(i));
	}

	thread(0)->operator()();

	for (uint32_t i=1; i < nThreads; i++)
	{
		threads[i].join();
	}
}


void FMEThreadPool::allocate()
{
	using FMEThreadPtr = FMEThread*;

	m_pSyncBarrier = new Barrier(m_numThreads);
	m_pThreads = new FMEThreadPtr[m_numThreads];
	for (uint32_t i=0; i < m_numThreads; i++)
	{
		m_pThreads[i] = new FMEThread(this, i);
#if 0
		m_pThreads[i]->cpuAffinity(1 << i);
#endif
	}
}

void FMEThreadPool::deallocate()
{
	for (uint32_t i=0; i < numThreads(); i++)
	{
		delete m_pThreads[i];
	}
	delete[] m_pThreads;
	delete m_pSyncBarrier;
}

}
}
