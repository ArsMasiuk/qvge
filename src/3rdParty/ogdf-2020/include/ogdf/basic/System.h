/** \file
 * \brief Decalration of System class which provides unified
 *        access to system information.
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

#pragma once

#include <ogdf/basic/basic.h>

#if defined(OGDF_SYSTEM_OSX)
#include <stdlib.h>
#elif defined(OGDF_SYSTEM_UNIX) || defined(__MINGW32__)
#include <malloc.h>
#endif

// detect processor architecture we're compiling for
//
// OGDF_ARCH_X86       Intel / AMD x86 32-bit processors
// OGDF_ARCH_X64       Intel / AMD x86 64-bit processors
// OGDF_ARCH_IA64      Intel Itanium
// OGDF_ARCH_PPC       PowerPC
// OGDF_ARCH_SPARC     SUN SPARC
// OGDF_ARCH_SPARC_V9  SUN SPARC V9

#if defined(_M_X64) || defined(__x86_64__)
#define OGDF_ARCH_X64
#elif defined(_M_IX86) || defined(__i386__)
#define OGDF_ARCH_X86
#elif defined(_M_IA64) || defined(__ia64__)
#define OGDF_ARCH_IA64
#elif defined(_M_MPPC) || defined(_M_PPC) || defined(__powerpc__)
#define OGDF_ARCH_PPC
#elif defined(__sparc__)
#define OGDF_ARCH_SPARC
#elif defined(__sparc_v9__)
#define OGDF_ARCH_SPARC_V9
#endif

namespace ogdf {

//! Special features supported by a x86/x64 CPU.
/**
 * @ingroup system
 *
 * This enumeration is used to specify spcial additional features that
 * are supported by the CPU, in particular extended instruction sets
 * such as SSE.
 */
enum class CPUFeature {
	MMX,    //!< Intel MMX Technology
	SSE,    //!< Streaming SIMD Extensions (SSE)
	SSE2,   //!< Streaming SIMD Extensions 2 (SSE2)
	SSE3,   //!< Streaming SIMD Extensions 3 (SSE3)
	SSSE3,  //!< Supplemental Streaming SIMD Extensions 3 (SSSE3)
	SSE4_1, //!< Streaming SIMD Extensions 4.1 (SSE4.1)
	SSE4_2, //!< Streaming SIMD Extensions 4.2 (SSE4.2)
	VMX,    //!< Virtual Machine Extensions
	SMX,    //!< Safer Mode Extensions
	EST,    //!< Enhanced Intel SpeedStep Technology
	MONITOR //!< Processor supports MONITOR/MWAIT instructions
};

//! Bit mask for CPU features.
/**
 * @ingroup system
 */
enum class CPUFeatureMask : unsigned int {
	MMX     = 1 << static_cast<int>(CPUFeature::MMX),    //!< Intel MMX Technology
	SSE     = 1 << static_cast<int>(CPUFeature::SSE),    //!< Streaming SIMD Extensions (SSE)
	SSE2    = 1 << static_cast<int>(CPUFeature::SSE2),   //!< Streaming SIMD Extensions 2 (SSE2)
	SSE3    = 1 << static_cast<int>(CPUFeature::SSE3),   //!< Streaming SIMD Extensions 3 (SSE3)
	SSSE3   = 1 << static_cast<int>(CPUFeature::SSSE3),  //!< Supplemental Streaming SIMD Extensions 3 (SSSE3)
	SSE4_1  = 1 << static_cast<int>(CPUFeature::SSE4_1), //!< Streaming SIMD Extensions 4.1 (SSE4.1)
	SSE4_2  = 1 << static_cast<int>(CPUFeature::SSE4_2), //!< Streaming SIMD Extensions 4.2 (SSE4.2)
	VMX     = 1 << static_cast<int>(CPUFeature::VMX),    //!< Virtual Machine Extensions
	SMX     = 1 << static_cast<int>(CPUFeature::SMX),    //!< Safer Mode Extensions
	EST     = 1 << static_cast<int>(CPUFeature::EST),    //!< Enhanced Intel SpeedStep Technology
	MONITOR = 1 << static_cast<int>(CPUFeature::MONITOR) //!< Processor supports MONITOR/MWAIT instructions
};

OGDF_EXPORT unsigned int operator|=(unsigned int &i, CPUFeatureMask fm);

//! %System specific functionality.
/**
 * @ingroup system
 *
 * The class System encapsulates system specific functions
 * providing unified access across different operating systems.
 * The provided functionality includes:
 *   - Query memory usage.
 *   - Access to high-perfomance counter under Windows and Cygwin.
 *   - Query CPU specific information.
 */
class OGDF_EXPORT System {

public:
	/**
	 * @name Memory
	 * These methods allow to allocate aligned memory and to query the amount of memory used.
	 */
	//@{

	static void *alignedMemoryAlloc16(size_t size) {
#ifdef OGDF_SYSTEM_WINDOWS
# ifdef __MINGW64__
		return __mingw_aligned_malloc(size, 16);
# else
		return _aligned_malloc(size, 16);
# endif
#elif defined(OGDF_SYSTEM_OSX)
		// malloc returns 16 byte aligned memory on OS X.
		return malloc(size);
#else
		return memalign(16, size);
#endif
	}

	static void alignedMemoryFree(void *p) {
#ifdef OGDF_SYSTEM_WINDOWS
# ifdef __MINGW64__
		__mingw_aligned_free(p);
# else
		_aligned_free(p);
# endif
#else
		free(p);
#endif
	}

	//! Returns the page size of virtual memory (in bytes).
	static int pageSize() { return s_pageSize; }

	//! Returns the total size of physical memory (in bytes).
	static long long physicalMemory();

	//! Returns the size of available (free) physical memory (in bytes).
	static long long availablePhysicalMemory();

	//! Returns the amount of memory (in bytes) allocated by the process.
	static size_t memoryUsedByProcess();

#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
	//! Returns the maximal amount of memory (in bytes) used by the process (Windows/Cygwin only).
	static size_t peakMemoryUsedByProcess();
#endif

	//! Returns the amount of memory (in bytes) allocated by OGDF's memory manager.
	/**
	 * The memory manager allocates blocks of a fixed size from the system (via malloc())
	 * and makes it available in its free lists (for allocating small pieces of memory.
	 * The returned value is the total amount of memory allocated from the system;
	 * the amount of memory currently allocated from the user is
	 * memoryAllocatedByMemoryManager() - memoryInFreelistOfMemoryManager().
	 *
	 * Keep in mind that the memory manager never releases memory to the system before
	 * its destruction.
	 */
	static size_t memoryAllocatedByMemoryManager();

	//! Returns the amount of memory (in bytes) contained in the global free list of OGDF's memory manager.
	static size_t memoryInGlobalFreeListOfMemoryManager();

	//! Returns the amount of memory (in bytes) contained in the thread's free list of OGDF's memory manager.
	static size_t memoryInThreadFreeListOfMemoryManager();

	//! Returns the amount of memory (in bytes) allocated on the heap (e.g., with malloc).
	/**
	 * This refers to dynamically allocated memory, e.g., memory allocated with malloc()
	 * or new.
	 */
	static size_t memoryAllocatedByMalloc();

	//! Returns the amount of memory (in bytes) contained in free chunks on the heap.
	/**
	 * This refers to memory that has been deallocated with free() or delete, but has not
	 * yet been returned to the operating system.
	 */
	static size_t memoryInFreelistOfMalloc();

#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
	//@}
	/**
	 * @name Measuring time
	 * These methods provide various ways to measure time. The high-performance
	 * counter (Windows and Cygwin only) can be used to measure real time
	 * periods with a better resolution than the standard system time function.
	 */
	//@{

	//! Returns the current value of the high-performance counter in \p counter.
	static void getHPCounter(int64_t &counter);

	//! Returns the elapsed time (in seconds) between \p startCounter and \p endCounter.
	static double elapsedSeconds(
		const int64_t &startCounter,
		const int64_t &endCounter);
#endif

	//! Returns the elapsed time (in milliseconds) between \p t and now.
	/**
	 * The functions sets \p t to to the current time. Usually, you first call
	 * usedRealTime(\p t) to query the start time \p t, and determine the elapsed time
	 * after performing some computation by calling usedRealTime(t) again; this time
	 * the return value gives you the elapsed time in milliseconds.
	 */
	static int64_t usedRealTime(int64_t &t);

	//! Returns the current time point of the real time wall clock.
	/**
	 * The start point of time points is system specific. The differences of two time points
	 * returned by this function represent elapsed real time in milliseconds.
	 */
	static int64_t realTime();


	//@}
	/**
	 * @name Process information
	 */
	//@{

	//! Returns the process ID of the current process.
	static int getProcessID();

	//@}
	/**
	 * @name Processor information
	 * These methods allow to query information about the current processor such as
	 * supported instruction sets (e.g., SSE extensions), cache size, and number of
	 * installed processors.
	 */
	//@{

	//! Returns the bit vector describing the CPU features supported on current system.
	static int cpuFeatures() { return s_cpuFeatures; }

	//! Returns true if the CPU supports \p feature.
	static bool cpuSupports(CPUFeature feature) {
		return (s_cpuFeatures & (1 << static_cast<int>(feature))) != 0;
	}

	//! Returns the L2-cache size (in KBytes).
	static int cacheSizeKBytes() { return s_cacheSize; }

	//! Returns the number of bytes in a cache line.
	static int cacheLineBytes() { return s_cacheLine; }

	//! Returns the number of processors (cores) available on the current system.
	static int numberOfProcessors() { return s_numberOfProcessors; }

	//@}

private:
	static unsigned int s_cpuFeatures; //!< Supported CPU features.
	static int          s_cacheSize;   //!< Cache size in KBytes.
	static int          s_cacheLine;   //!< Bytes in a cache line.
	static int          s_numberOfProcessors; //!< Number of processors (cores) available.
	static int          s_pageSize;    //!< The page size of virtual memory.

#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
	static int64_t s_HPCounterFrequency; //!< Frequency of high-performance counter.
#endif

public:
	//! Static initilization routine (automatically called).
	static void init();
};

}
