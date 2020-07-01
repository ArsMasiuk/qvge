/** \file
 * \brief Implementation of a thread barrier.
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

#include <mutex>
#include <condition_variable>


namespace ogdf {

//! Representation of a barrier.
/**
 * @ingroup threads
 *
 * A barrier is used for synchronizing threads. A barrier for a group of threads means
 * that all threads in the group must have reached the barrier before any of the threads
 * may proceed executing code after the barrier.
 */
class Barrier {

	std::condition_variable m_allThreadsReachedSync;
	std::mutex m_numThreadsReachedSyncLock;

	uint32_t m_threadCount; //!< the number of threads in the group.
	uint32_t m_numThreadsReachedSync; //!< number of htreads that reached current synchronization point.
	uint32_t m_syncNumber; //!< number of current synchronization point.

public:

	//! Creates a barrier for a group of \p numThreads threads.
	explicit Barrier(uint32_t numThreads) : m_threadCount(numThreads) {
		m_numThreadsReachedSync = 0;
		m_syncNumber = 0;
	}

	//! Synchronizes the threads in the group.
	/**
	 * Each thread proceeds only after all threads in the group have reached the barrier.
	 * A barrier may be used for several synchronization points.
	 */
	void threadSync() {
		std::unique_lock<std::mutex> lk(m_numThreadsReachedSyncLock);

		uint32_t syncNr = m_syncNumber;
		m_numThreadsReachedSync++;
		if (m_numThreadsReachedSync == m_threadCount) {
			m_syncNumber++;
			m_allThreadsReachedSync.notify_all();
			m_numThreadsReachedSync = 0;

		} else {
			m_allThreadsReachedSync.wait(lk, [syncNr,this]{return syncNr != m_syncNumber; });
		}
	}

};

}
