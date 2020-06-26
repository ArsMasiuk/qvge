/** \file
 * \brief Declaration of stopwatch classes
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


namespace ogdf {

//! Realizes a stopwatch for measuring elapsed time.
/**
 * @ingroup date-time
 */
class OGDF_EXPORT Stopwatch {

	int64_t m_startTime;  //!< The start time of the timer in milliseconds.
	int64_t m_totalTime;  //!< The total time in milliseconds.
	bool    m_running;    //!< true, if the timer is running.

public:
	//! Initializes a stop watch with total time 0.
	/**
	 * After creation the stopwatch is not running, i.e., it has to be started explicitly
	 * for measuring time.
	 */
	Stopwatch() : m_startTime(0), m_totalTime(0), m_running(false) { }

	//! Initializes a stopwatch and sets its total time to \p milliSecs.
	/**
	 * After creation the stopwatch is not running, i.e., it has to be started explicitly
	 * for measuring time.
	 *
	 * \param milliSecs The intial value of the total time in milliseconds.
	 */
	explicit Stopwatch(int64_t milliSecs) :  m_startTime(0), m_totalTime(milliSecs), m_running(false) { }


	virtual ~Stopwatch() { }


	//! Starts the stopwatch.
	/**
	 * For safety reasons starting a running timer is an error.
	 *
	 * \param reset If this flag is set to true, the stopwatch is reset before it is started.
	 */
	void start(bool reset = false);

	//! Stops the stopwatch and adds the difference between the current time and the starting time to the total time.
	/**
	 * Stopping a non-running stopwatch is an error.
	 */
	void stop();

	//! Stops the stopwatch and sets its total time to 0.
	void reset() {
		m_running   = false;
		m_totalTime = 0;
	}


	//! Returns true if the stopwatch is running, false otherwise.
	bool running() const { return m_running; }


	//! Returns the currently elapsed time in milliseconds.
	/**
	 * It is not necessary to stop the timer to get the correct time.
	 */
	int64_t milliSeconds() const {
		return m_running ? m_totalTime + theTime() - m_startTime : m_totalTime;
	}

	//! Returns the currently elapsed time in 1/100-seconds.
	/**
	 * It is not necessary to stop the timer to get the correct time.
	 */
	int64_t centiSeconds() const { return milliSeconds()/10; }

	//! Returns the currently elapsed time in seconds.
	/**
	 * It is not necessary to stop the timer to get the correct time.
	 * The result is rounded down to the next integer value.
	 */
	int64_t seconds() const { return milliSeconds()/1000; }

	//! Returns the currently elapsed time in minutes.
	/**
	 * It is not necessary to stop the timer to get the correct time.
	 * The result is rounded down to the next integer value.
	 */
	int64_t minutes() const { return seconds()/60; }

	//! Returns the currently elapsed time in hours.
	/**
	 * It is not necessary to stop the timer to get the correct time.
	 * The result is rounded down to the next integer value.
	 */
	int64_t hours() const { return seconds()/3600; }


	//! Returns true iff the currently elapsed time exceeds \p maxSeconds.
	bool exceeds(int64_t maxSeconds) const {
		return seconds() >= maxSeconds;
	}

	//! Adds \p centiSeconds to total time.
	/**
	 * \param centiSeconds The number of centiseconds to be added.
	 */
	void addCentiSeconds(int64_t centiSeconds) {
		m_totalTime += 10*centiSeconds;
	}


	//! Writes the currently elapsed time in the format <tt>hh:mm:ss.sec/100</tt> to output stream \p os.
	/**
	 * \param os        The output stream.
	 * \param stopwatch The stopwatch whose elapsed time shall be written.
	 * \return A reference to the output stream \p os.
	 */
	friend OGDF_EXPORT std::ostream& operator<<(std::ostream& os, const Stopwatch &stopwatch);

protected:

	//! Returns the current time in milliseconds (from some fixed starting point).
	/**
	 * This pure virtual function is used for measuring time differences.
	 * It must be implemented in derived classes.
	 *
	 * \return The time since some starting point (e.g., the program start) in milliseconds.
	 */
	virtual int64_t theTime() const = 0;

};


//! Implements a stopwatch measuring CPU time.
class OGDF_EXPORT StopwatchCPU : public Stopwatch {
public:
	//! Creates a stopwatch for measuring CPU time with total time 0.
	/**
	 * After creation the stopwatch is not running, i.e., it has to be started explicitly
	 * for measuring time.
	 */
	StopwatchCPU() : Stopwatch() { }

	//! Creates a stopwatch for measuring CPU time and sets its total time to \p milliSecs.
	/**
	 * After creation the stopwatch is not running, i.e., it has to be started explicitly
	 * for measuring time.
	 *
	 * \param milliSecs The intial value of the total time in milliseconds.
	 */
	explicit StopwatchCPU(int64_t milliSecs) : Stopwatch(milliSecs) { }

	virtual ~StopwatchCPU() { }

private:
	//! Returns the current CPU time in milliseconds (from some fixed starting point).
	virtual int64_t theTime() const override;
};


//! Implements a stopwatch measuring wall-clock time.
class OGDF_EXPORT StopwatchWallClock : public Stopwatch {
public:
	//! Creates a stopwatch for measuring wall-clock time with total time 0.
	/**
	 * After creation the stopwatch is not running, i.e., it has to be started explicitly
	 * for measuring time.
	 */
	StopwatchWallClock() : Stopwatch() { }

	//! Creates a stopwatch for measuring wall-clock time and sets its total time to \p milliSecs.
	/**
	 * After creation the stopwatch is not running, i.e., it has to be started explicitly
	 * for measuring time.
	 *
	 * \param milliSecs The intial value of the total time in milliseconds.
	 */
	explicit StopwatchWallClock(int64_t milliSecs) : Stopwatch(milliSecs) { }

	virtual ~StopwatchWallClock() { }

private:
	//! Returns the current wall-clock time in milliseconds (from some fixed starting point).
	virtual int64_t theTime() const override;
};

}
