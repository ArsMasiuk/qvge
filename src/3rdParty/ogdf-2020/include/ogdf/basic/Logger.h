/** \file
 * \brief Contains logging functionality
 *
 * \author Markus Chimani
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

#include <ogdf/basic/internal/config.h>
#include <algorithm>

namespace ogdf {

//! Centralized global and local logging facility working on streams like \c std::cout.
/**
 * The Logger class is a centralized logging environment with 2x2 different use-cases working together.
 * All generated output is sent into the \a world-stream, i.e., \c std::cout, if not set otherwise.
 *
 * \b Logging \b vs. \b Statistic:
 * The Logger differentiates between \a logging and \a statistic mode.
 * When in logging mode, only the output written via the lout()/slout() commands is
 * written to the world stream (according to loglevels). When in statistic mode,
 * only the output of the sout()/ssout() commands is written.
 * (Sidenote: there is also a \a forced output fout()/sfout() which is written independent on the current mode).
 *
 * The idea of these two modi is that one can augment the code with output which is
 * interesting in the normal computation mode via lout, but the same algorithm can also be run given tabular
 * statistic-lines when e.g. batch-testing the algorithm on a set of benchmark instances.
 *
 * \b Global \b vs. \b Local:
 * You can choose to use the Logging facilities globally via the static outputs (slout(), ssout(), sfout()).
 * Thereby the global log-level and statistic-mode settings are applied.
 * Alternatively you can create your own Logging object with its own parameters only for your algorithm,
 * and use the object methods lout(), sout(), and fout(). This allows you to turn output on for your own
 * (new) algorithm, but keep the rest of the library silent.
 *
 * \b Global \b Settings:
 * The slout command takes an (optional) parameter given the importance of the output (aka. loglevel).
 * The output is written only if the globalLogLevel is not higher. The method globalStatisticMode
 * turn the statistic-mode on and off (thereby disabling or enabling the logging mode).
 *
 * Furthermore, we have a globalMinimumLogLevel. This is used to globally forbid any output
 * with too low importance written by any Logger-objects.
 *
 * \b Local \b Settings:
 * A Logger-object has its own set of settings, i.e., its own localLogLevel and an own localLogMode,
 * which can be any of the enumerators in Logger::LogMode.
 *
 * \b Typical \b Usage:<br>
 * The simplest but restricted and verbose usage is to write <br>
 * <code><br>
 *    Logger::slout() << "1+2=" << (1+2) << std::endl;
 * </code>
 *
 * The conceptually easiest and cleanest approach is to augment your algorithm class with a Logger.
 * Multiple inheritance allows this pretty straightforwardly:<br>
 * <code><br>
 *    class MyAlgorithm : public MyBaseClass, protected Logger {<br>
 *    &nbsp;&nbsp;int myMethod();<br>
 *    }<br>
 *    <br>
 *    MyAlgorithm::myMethod() {<br>
 *    &nbsp;&nbsp;lout() << "1+2=" << (1+2) << std::endl;<br>
 *    }<br>
 * </code>
 *
 *
 *  \b Internal \b Library \b Logging:
 *  Internal Libaries (as, e.g., Abacus) can use their own, global, set of logging functions.
 *  Its LogLevels are parallel and independent of the global LogLevel, but its logging is automatically
 *  turned off whenever the (global/static) Logger is in statistics-mode.
 */

class Logger {

public:
	//! supported log-levels from lowest to highest importance
	enum class Level {
		Minor,
		Medium,
		Default,
		High,
		Alarm,
		Force
	};
	//! Local log-modes
	enum class LogMode {
		//! the object is in the same mode as the static Logger-class (i.e., global settings)
		Global,
		//! the object is in logging mode, but uses the globalLogLevel
		GlobalLog,
		//! the object is in logging mode, using its own localLogLevel
		Log,
		//! the object is in statistic mode
		Statistic
	};

	//! creates a new Logger-object with LogMode::Global and local log-level equal globalLogLevel
	Logger() : Logger(LogMode::Global, m_globalloglevel) {}
	//! creates a new Logger-object with given log-mode and local log-level equal globalLogLevel
	explicit Logger(LogMode m) : Logger(m, m_globalloglevel) {}
	//! creates a new Logger-object with LogMode::Global and given local log-level
	explicit Logger(Level level) : Logger(LogMode::Global, level) {}
	//! creates a new Logger-object with given log-mode and given local log-level
	Logger(LogMode m, Level level) :
		m_loglevel(level), m_logmode(m) {}

	//! \name Usage
	//! @{

	//! returns true if such an lout command will result in text being printed
	bool is_lout(Level level = Level::Default) const {
		bool globalNotStatistic = !m_globalstatisticmode && m_logmode == LogMode::Global;
		if (globalNotStatistic || m_logmode==LogMode::GlobalLog) {
			return level >= m_globalloglevel;
		} else {
			return m_logmode==LogMode::Log
			    && level >= std::max(m_loglevel, m_minimumloglevel);
		}
	}
	//! stream for logging-output (local)
	std::ostream& lout(Level level = Level::Default) const {
		return is_lout(level) ? *world : nirvana;
	}
	//! stream for statistic-output (local)
	std::ostream& sout() const {
		return (m_globalstatisticmode && m_logmode == LogMode::Global) || m_logmode == LogMode::Statistic ? *world : nirvana;
	}
	//! stream for forced output (local)
	std::ostream& fout() const {
		return sfout();
	}

	//! @}
	//! \name Static usage
	//! @{

	//! returns true if such an slout command will result in text being printed
	static bool is_slout(Level level = Level::Default) {
		return !m_globalstatisticmode && level >= m_globalloglevel;
	}
	//! stream for logging-output (global)
	static std::ostream& slout(Level level = Level::Default) {
		return is_slout(level) ? *world : nirvana;
	}
	//! stream for statistic-output (global)
	static std::ostream& ssout() {
		return m_globalstatisticmode ? *world : nirvana;
	}
	//! stream for forced output (global)
	static std::ostream& sfout() {
		return *world;
	}

	//! @}
	//! \name Static internal library usage
	//! @{

	//! stream for logging-output (global; used by internal libraries, e.g. Abacus)
	//! returns true if such an ilout command will result in text being printed
	static bool is_ilout(Level level = Level::Default) {
		return !m_globalstatisticmode && level >= m_globallibraryloglevel;
	}
	static std::ostream& ilout(Level level = Level::Default) {
		return is_ilout(level) ? *world : nirvana;
	}

	//! stream for forced output (global; used by internal libraries, e.g. Abacus)
	static std::ostream& ifout() {
		return *world;
	}

	//! @}
	//! \name Local
	//! @{

	//! gives the local log-level
	Level localLogLevel() const {
		return m_loglevel;
	}
	//! sets the local log-level
	void localLogLevel(Level level) {
		m_loglevel = level;
	}
	//! gives the local log-mode
	LogMode localLogMode() const {
		return m_logmode;
	}
	//! sets the local log-mode
	void localLogMode(LogMode m) {
		m_logmode = m;
	}

	//! @}
	//! \name Global
	//! @{

	//! gives the global log-level
	static Level globalLogLevel() { return m_globalloglevel; }
	//! sets the global log-level
	static void globalLogLevel(Level level) {
		m_globalloglevel = level;
		if( m_globalloglevel < m_minimumloglevel )
			m_minimumloglevel = m_globalloglevel;
	}

	//! gives the internal-library log-level
	static Level globalInternalLibraryLogLevel() { return m_globallibraryloglevel; }
	//! sets the internal-library log-level
	static void globalInternalLibraryLogLevel(Level level) { m_globallibraryloglevel = level; }

	//! gives the globally minimally required log-level
	static Level globalMinimumLogLevel() { return m_minimumloglevel; }
	//! sets the globally minimally required log-level
	static void globalMinimumLogLevel(Level level) {
		m_minimumloglevel = level;
		if( m_globalloglevel < m_minimumloglevel )
			m_globalloglevel = m_minimumloglevel;
	}

	//! returns true if we are globally in statistic mode
	static bool globalStatisticMode() { return m_globalstatisticmode; }
	//! sets whether we are globally in statistic mode
	static void globalStatisticMode(bool s) { m_globalstatisticmode = s; }

	//! change the stream to which allowed output is written (by default: \c std::cout)
	static void setWorldStream(std::ostream& o) { world = &o; }

	//! @}
	//! \name Effective
	//! @{

	//! obtain the effective log-level for the Logger-object (i.e., resolve the dependencies on the global settings)
	Level effectiveLogLevel() const {
		if(m_logmode==LogMode::Global || m_logmode==LogMode::GlobalLog)
			return m_globalloglevel;
		else
			return (m_loglevel > m_minimumloglevel) ? m_loglevel : m_minimumloglevel;
	}

	//! returns true if the Logger-object is effectively in statistic-mode (as this might be depending on the global settings)
	bool effectiveStatisticMode() const {
		return m_logmode==LogMode::Statistic || (m_logmode==LogMode::Global && m_globalstatisticmode);
	}

	//! @}

private:
	static OGDF_EXPORT std::ostream nirvana;
	static OGDF_EXPORT std::ostream* world;

	static OGDF_EXPORT Level m_globalloglevel;
	static OGDF_EXPORT Level m_globallibraryloglevel;
	static OGDF_EXPORT Level m_minimumloglevel;
	static OGDF_EXPORT bool m_globalstatisticmode;

	Level m_loglevel;
	LogMode m_logmode;
};

inline std::ostream &operator<<(std::ostream &os, Logger::Level level) {
	switch (level) {
	case Logger::Level::Minor:
		os << "Minor";
		break;
	case Logger::Level::Medium:
		os << "Medium";
		break;
	case Logger::Level::Default:
		os << "Default";
		break;
	case Logger::Level::High:
		os << "High";
		break;
	case Logger::Level::Alarm:
		os << "Alarm";
		break;
	case Logger::Level::Force:
		os << "Force";
		break;
	}
	return os;
}

}
