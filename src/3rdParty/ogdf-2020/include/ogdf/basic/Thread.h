/** \file
 * \brief Declaration of Thread class representing threads.
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
#include <ogdf/basic/memory.h>
#include <thread>


namespace ogdf {

//! Threads supporting OGDF's memory management.
/**
 * @ingroup threads
 *
 * This class derives from std::thread and extends the constructor in such a way
 * that worker functions correctly call thread-specific initialization and clean-up
 * functions for OGDF's memory management.
 *
 * If you use OGDF data structures in your threads you have to use the Thread class
 * (instead of just using std::thread), or you need to call the flushPool()
 * function of the memory allocator manually (see Thread's constructor for an example).
 */
class Thread : public std::thread
{
public:
	Thread() : thread() { }
	Thread(Thread &&other) : thread(std::move((thread&&)other)) { }

	// Visual C++ 2013 Preview cannot compile that combination of variadic templates and lambda function (though it should).
	// Therefore we use the version without function arguments for MSVC untils this works.
#ifdef _MSC_VER
	template<class Function>
	explicit Thread(Function && f) : thread([&]{
		f();
		OGDF_ALLOCATOR::flushPool();
	}) { }

#else
	template<class Function, class ... Args>
	explicit Thread(Function && f, Args && ... args) : thread([&](Args && ... tArgs){
		f(std::forward<Args>(tArgs)...);
		OGDF_ALLOCATOR::flushPool();
	}, std::forward<Args>(args)...) { }
#endif

	Thread &operator=(Thread &&other) {
		thread::operator=(std::move((thread&&)other));
		return *this;
	}
};

}
