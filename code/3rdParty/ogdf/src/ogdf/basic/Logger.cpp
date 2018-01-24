/** \file
 * \brief Logging functionality
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


#include <ogdf/basic/Logger.h>

namespace ogdf {

// initializers
#ifdef OGDF_DEBUG
Logger::Level Logger::m_globalloglevel = Logger::Level::Default;
Logger::Level Logger::m_minimumloglevel = Logger::Level::Minor;
Logger::Level Logger::m_globallibraryloglevel = Logger::Level::Default;
#else // RELEASE
Logger::Level Logger::m_globalloglevel = Logger::Level::Alarm; // forbid anything except alarms and forced writes -> logging is off
Logger::Level Logger::m_globallibraryloglevel = Logger::Level::Alarm;
Logger::Level Logger::m_minimumloglevel = Logger::Level::Medium;
#endif

std::ostream* Logger::world = &std::cout;
std::ostream Logger::nirvana(nullptr);

bool Logger::m_globalstatisticmode = false;

}
