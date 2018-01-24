/** \file
 * \brief Declaration of memory manager for allocating small
 *        pieces of memory
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

#include <ogdf/basic/System.h>

#ifndef OGDF_MEMORY_POOL_NTS
# include <mutex>
#endif

namespace ogdf {

//! Allocates memory in large chunks for better runtime
/**
 * Possibly allocates more memory than required.
 * Newly allocated chunks contain #BLOCK_SIZE many bytes.
 * Can allocate at most #TABLE_SIZE bytes per invocation of #allocate.
 *
 * For each requested memory segment of \c n bytes,
 * \c OGDF_POINTER_SIZE bytes are allocated in addition to store a pointer to another segment of memory.
 *
 * This allows to store memory that is requested to be deallocated in a single linked list,
 * and re-distribute it upon later allocation requests, instead of actually decallocating it.
 */
class PoolMemoryAllocator {
	//! Basic memory element used to realize a linked list of deallocated memory segments
	struct MemElem {
		MemElem *m_next;
	};

	using MemElemPtr = MemElem*;

	struct PoolElement;
	struct BlockChain;

	static constexpr size_t MIN_BYTES = sizeof(MemElemPtr);
	static constexpr size_t TABLE_SIZE = 256;
	static constexpr size_t BLOCK_SIZE = 8192;

public:
	PoolMemoryAllocator() { }
	~PoolMemoryAllocator() { }

	//! Frees all allocated memory
	static OGDF_EXPORT void cleanup();

	//! Returns true iff #allocate can be invoked with \c nBytes
	static OGDF_EXPORT bool checkSize(size_t nBytes) {
		return nBytes < TABLE_SIZE;
	}

	//! Allocates memory of size \c nBytes.
	static OGDF_EXPORT void *allocate(size_t nBytes);

	//! Deallocates memory at address \c p which is of size \c nBytes.
	static OGDF_EXPORT void deallocate(size_t nBytes, void *p);

	//! Deallocate a complete list starting at \c pHead and ending at \c pTail.
	/**
	 * The elements are assumed to be chained using the first word of each element and
	 * elements are of size \c nBytes.
	 * In contrasting to linear-time element-wise deallocation this method takes constant time.
	 */
	static OGDF_EXPORT void deallocateList(size_t nBytes, void *pHead, void *pTail);

	//! Flushes all free but allocated bytes (#s_tp) to the thread-global list (#s_pool) of allocated bytes.
	static OGDF_EXPORT void flushPool();

	//! Returns the total amount of memory (in bytes) allocated from the system.
	static OGDF_EXPORT size_t memoryAllocatedInBlocks();

	//! Returns the total amount of memory (in bytes) available in the global free lists.
	static OGDF_EXPORT size_t memoryInGlobalFreeList();

	//! Returns the total amount of memory (in bytes) available in the thread's free lists.
	static OGDF_EXPORT size_t memoryInThreadFreeList();

	/**
	 * Defragments the global free lists.
	 *
	 * This methods sorts the global free lists, so that successive elements come after each
	 * other. This can improve perfomance for data structure that allocate many elements from
	 * the pool like lists and graphs.
	 */
	static OGDF_EXPORT void defrag();

private:
	static inline void enterCS() {
#ifndef OGDF_MEMORY_POOL_NTS
		s_mutex.lock();
#endif
	}

	static inline void leaveCS() {
#ifndef OGDF_MEMORY_POOL_NTS
		s_mutex.unlock();
#endif
	}

	static int slicesPerBlock(uint16_t nBytes) {
		int nWords;
		return slicesPerBlock(nBytes,nWords);
	}

	static int slicesPerBlock(uint16_t nBytes, int &nWords) {
		nWords = (nBytes + OGDF_SIZEOF_POINTER - 1) / OGDF_SIZEOF_POINTER;
		return (BLOCK_SIZE - OGDF_SIZEOF_POINTER) / (nWords * OGDF_SIZEOF_POINTER);
	}

	static void *fillPool(MemElemPtr &pFreeBytes, uint16_t nBytes);

	static MemElemPtr allocateBlock();
	static void makeSlices(MemElemPtr p, int nWords, int nSlices);

	static size_t unguardedMemGlobalFreelist();

	//! Contains allocated but free memory that may be used by all threads.
	//! Filled upon exiting a thread that allocated memory that was later freed.
	static PoolElement s_pool[TABLE_SIZE];

	//! Holds all allocated memory independently of whether it is cleared in chunks of size #BLOCK_SIZE.
	static BlockChain *s_blocks;

#ifdef OGDF_DEBUG
	//! Holds the number of globally allocated bytes for debugging.
	static long long s_globallyAllocatedBytes;
	//! Holds the number of thread-locally allocated bytes for debugging.
	static thread_local long long s_locallyAllocatedBytes;
#endif

#ifdef OGDF_MEMORY_POOL_NTS
	static MemElemPtr s_tp[TABLE_SIZE];
#else
	static std::mutex s_mutex;
	//! Contains the allocated but free memory for a single thread.
	static thread_local MemElemPtr s_tp[TABLE_SIZE];
#endif

};

}
