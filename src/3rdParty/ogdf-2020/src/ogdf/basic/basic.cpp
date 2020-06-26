/** \file
 * \brief Implementation of basic functionality
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

#include <random>

#include <ogdf/basic/basic.h>
#include <ogdf/basic/memory.h>

#ifdef OGDF_SYSTEM_WINDOWS
# define WIN32_EXTRA_LEAN
# define WIN32_LEAN_AND_MEAN
# undef NOMINMAX
# define NOMINMAX
# include <windows.h>
#endif
#ifdef OGDF_SYSTEM_UNIX
# include <unistd.h>
# include <sys/times.h>
#endif

// When OGDF_DLL is not set, we use the static initializer object
// that is instantiated in basic.h. Because it is instantiated in
// the header file, it becomes instantiated (a lot) more than once.
// We cannot simply instantiate it in the source file because
// this does not guarantee that it is instantiated at all (depends
// on the linker and different other things).
// Defining it in basic.h, which is always one of the first used
// header files, assures that the initializer instantiation takes
// place before any other static object is instantiated.
//
// The following counter is necessary to make sure that init is
// called when the constructor is called the first time, and that
// deinit is called when the destructor is called the *last* time.
//
// The latter is important. Otherwise, it could happen that static
// objects' destructors deallocate memory in the pool manager after
// the pool manager is already deinitialized.
static int initializerCount = 0;

static void initializeOGDF()
{
	if (initializerCount++ == 0) {
		ogdf::System::init();
	}
}

static void deinitializeOGDF()
{
	if (--initializerCount == 0) {
		ogdf::PoolMemoryAllocator::cleanup();
	}
}

namespace ogdf {

#ifdef OGDF_DEBUG
bool debugMode = true;
#else
bool debugMode = false;
#endif

Initialization::Initialization()
{
	initializeOGDF();
}

Initialization::~Initialization()
{
	deinitializeOGDF();
}

inline bool charCompareIgnoreCase(char a, char b)
{
	return toupper(a) == toupper(b);
}

void removeTrailingWhitespace(std::string &str)
{
	std::size_t found = str.find_last_not_of(" \t\v\f\n\r");
	if (found != std::string::npos) {
		str.erase(found+1);
	} else { // string consists only of whitespacae
		str.clear();
	}
}

bool equalIgnoreCase(const string &str1, const string &str2)
{
	return str1.size() == str2.size()
	    && std::equal(str1.begin(), str1.end(), str2.begin(), charCompareIgnoreCase);
}

bool prefixIgnoreCase(const string &prefix, const string &str)
{
	string::size_type len = prefix.length();
	return str.size() >= len
	    && std::equal(prefix.begin(), prefix.end(), str.begin(), charCompareIgnoreCase);
}

static std::mt19937 s_random;

#ifndef OGDF_MEMORY_POOL_NTS
static std::mutex s_randomMutex;
#endif

long unsigned int randomSeed()
{
#ifndef OGDF_MEMORY_POOL_NTS
	std::lock_guard<std::mutex> guard(s_randomMutex);
#endif
	return 7*s_random()+3;  // do not directly return seed, add a bit of variation
}

void setSeed(int val)
{
	s_random.seed(val);
}

int randomNumber(int low, int high)
{
	OGDF_ASSERT(low <= high);

	std::uniform_int_distribution<> dist(low,high);

#ifndef OGDF_MEMORY_POOL_NTS
	std::lock_guard<std::mutex> guard(s_randomMutex);
#endif
	return dist(s_random);
}

double randomDouble(double low, double high)
{
	OGDF_ASSERT(low <= high);

	std::uniform_real_distribution<> dist(low,high);

#ifndef OGDF_MEMORY_POOL_NTS
	std::lock_guard<std::mutex> guard(s_randomMutex);
#endif
	return dist(s_random);
}

double usedTime(double& T)
{
	double t = T;

#ifdef OGDF_SYSTEM_WINDOWS
	FILETIME creationTime;
	FILETIME exitTime;
	FILETIME kernelTime;
	FILETIME userTime;

	GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime);
	ULARGE_INTEGER user;
	user.LowPart = userTime.dwLowDateTime;
	user.HighPart = userTime.dwHighDateTime;
	T = double(user.QuadPart) * 0.0000001;

#else
	struct tms now;
	times (&now);
	T = (double)now.tms_utime / (double)sysconf(_SC_CLK_TCK);
#endif

	return T - t;
}

}
