/** \file
 * \brief Implementation of stopwatch classes
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

#include <ogdf/basic/Stopwatch.h>
#include <ogdf/basic/System.h>
#include <ogdf/basic/exceptions.h>

namespace ogdf {

std::ostream& operator<<(std::ostream& os, const Stopwatch &stopwatch)
{
	int64_t centiSeconds = stopwatch.centiSeconds();

	int64_t sec  = centiSeconds/100;
	int64_t mSec = centiSeconds - 100*sec;
	int64_t rSec = sec%60;
	int64_t min  = sec/60;
	int64_t rMin = min%60;

	os << min/60 << ":";
	if(rMin < 10) os << '0';
	os << rMin << ':';
	if(rSec < 10) os << '0';
	os << rSec << '.';
	if (mSec < 10) os << '0';
	os << mSec;
	return os;
}


void Stopwatch::start(bool reset)
{
	if (reset)
		m_totalTime = 0;

	else if (m_running) {
		Logger::ifout() << "Stopwatch::start(): you cannot start a running stopwatch.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Timer);
	}

	m_running   = true;
	m_startTime = theTime();
}


void Stopwatch::stop()
{
	if(!m_running) {
		Logger::ifout() << "Stopwatch::stop(): you cannot stop a non-running stopwatch.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Timer);
	}

	m_totalTime += theTime() - m_startTime;
	m_running   = false;
}


int64_t StopwatchCPU::theTime() const
{
	double t;
	ogdf::usedTime(t);

	return (int64_t)(1000.0*t);
}


int64_t StopwatchWallClock::theTime() const
{
	int64_t t;
	ogdf::System::usedRealTime(t);

	return t;
}

 }
