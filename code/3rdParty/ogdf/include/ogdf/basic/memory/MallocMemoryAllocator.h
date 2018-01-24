/** \file
 * \brief Declaration of memory manager for allocating small
 *        pieces of memory
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

#include <ogdf/basic/Logger.h>
#include <ogdf/basic/exceptions.h>

namespace ogdf {

//! Implements a simple memory manager using \c malloc() and \c free().
class OGDF_EXPORT MallocMemoryAllocator
{
	struct MemElem { MemElem *m_next; };
	using MemElemPtr = MemElem*;

public:

	MallocMemoryAllocator() { }
	~MallocMemoryAllocator() { }

	static void cleanup() { }

	//! Allocates memory of size \p nBytes.
	static inline void *allocate(size_t nBytes, const char *, int) {
		return allocate(nBytes);
	}

	//! Allocates memory of size \p nBytes.
	static inline void *allocate(size_t nBytes)
	{
		void *p = malloc(nBytes);
		if (OGDF_UNLIKELY(p == nullptr)) OGDF_THROW(ogdf::InsufficientMemoryException);
		return p;
	}


	//! Deallocates memory at address \p p.
	//! We do not keep track of the size of the deallocated object.
	static inline void deallocate(size_t, void *p) {
		free(p);
	}

	//! Deallocate a complete list starting at \p pHead and ending at \p pTail.
	/**
	 * The elements are assumed to be chained using the first word of each element.
	 */
	static void deallocateList(size_t /* nBytes */, void *pHead, void *pTail)
	{
		MemElemPtr q, pStop = MemElemPtr(pTail)->m_next;
		while (pHead != pStop) {
			q = MemElemPtr(pHead)->m_next;
			free(pHead);
			pHead = q;
		}
	}

	static void flushPool() { }
	static void flushPool(uint16_t /* nBytes */) { }

	//! Always returns true since we simply trust malloc().
	static constexpr bool checkSize(size_t) {
		return true;
	}

	//! Always returns 0, since no blocks are allocated.
	static constexpr size_t memoryAllocatedInBlocks() {
		return 0;
	}

	//! Always returns 0, since no blocks are allocated.
	static constexpr size_t memoryInFreelist() {
		return 0;
	}

	//! Always returns 0, since no blocks are allocated.
	static constexpr size_t memoryInGlobalFreeList() {
		return 0;
	}

	//! Always returns 0, since no blocks are allocated.
	static constexpr size_t memoryInThreadFreeList() {
		return 0;
	}
};

}
