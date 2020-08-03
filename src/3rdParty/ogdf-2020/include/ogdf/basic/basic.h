/** \file
 * \brief Basic declarations, included by all source files.
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

#include <ogdf/basic/internal/config.h>

//! @name Assertions (only active in debug builds)
//! @{

//! Assert condition \p expr. See doc/build.md for more information.
//! @ingroup macros
#define OGDF_ASSERT(expr)
//! Assert condition \p expr when using heavy debugging. See OGDF_ASSERT.
//! @ingroup macros
#define OGDF_HEAVY_ASSERT(expr)

#ifdef OGDF_DEBUG
# ifdef OGDF_HEAVY_DEBUG
#  undef OGDF_HEAVY_ASSERT
#  define OGDF_HEAVY_ASSERT(expr) OGDF_ASSERT(expr)
# endif
# undef OGDF_ASSERT
# ifndef OGDF_USE_ASSERT_EXCEPTIONS
#  include <cassert>
#  define OGDF_ASSERT(expr) assert(expr)
# else
#  include <stdexcept>
#  include <sstream>

namespace ogdf {
/**
 * A trivial exception for failed assertions.
 * Only available if the macro OGDF_USE_ASSERT_EXCEPTIONS is defined.
 */
class AssertionFailed : public std::runtime_error {
	using std::runtime_error::runtime_error;
};
}

#  define OGDF_ASSERT(expr) do { \
	if (!(expr)) { \
		std::stringstream ogdf_assert_ss; \
		ogdf_assert_ss \
		 << "OGDF assertion `" #expr "' failed at " __FILE__ ":" \
		 << __LINE__ \
		 << "(" << OGDF_FUNCTION_NAME << ")"; \
		ogdf::get_stacktrace(ogdf_assert_ss); \
		throw ogdf::AssertionFailed(ogdf_assert_ss.str()); \
	} } while (false)
# endif
#endif

//! @}

#include <cstdint>
#include <cmath>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <limits>

//! The namespace for all OGDF objects.
namespace ogdf {

//! Set to true iff debug mode is used during compilation of the OGDF
OGDF_EXPORT extern bool debugMode;

// generally used <algorithm> members
using std::min;
using std::max;

/**
 *  The class Initialization is used for initializing global variables.
 *  You should never create instances of it!
*/
class OGDF_EXPORT Initialization {
public:
	Initialization();
	~Initialization();
};

// OGDF_INSTALL is defined only when compiling a dynamic library.
// Whenever this library is used we can safely assume that this header
// file is included beforehand.
// Thus, there will be a static initialization object in the program that uses the library.
#ifndef OGDF_INSTALL
// This has to be in the header file. Being in the cpp file does not
// guarantee that it is constructed (with all linkers).
static Initialization s_ogdfInitializer;
#endif

#ifdef OGDF_USE_ASSERT_EXCEPTIONS
//! Output a mangled stack backtrace of the caller function to stream
extern void get_stacktrace(std::ostream &);
#endif

enum class Direction { before, after };

//! @addtogroup random
//! @{

//! Returns a random value suitable as initial seed for a random number engine.
/**
 * <H3>Thread Safety</H3>
 * This functions is thread-safe.
 */
OGDF_EXPORT long unsigned int randomSeed();

//! Sets the seed for functions like randomSeed(), randomNumber(), randomDouble().
OGDF_EXPORT void setSeed(int val);

//! Returns random integer between low and high (including).
/**
 * <H3>Thread Safety</H3>
 * This functions is thread-safe.
 */
OGDF_EXPORT int randomNumber(int low, int high);

//! Returns a random double value from the interval [\p low, \p high).
/**
 * <H3>Thread Safety</H3>
 * This functions is thread-safe.
 */
OGDF_EXPORT double randomDouble(double low, double high);

//! Returns a random double value from the normal distribution
//! with mean m and standard deviation sd
inline double randomDoubleNormal(double m, double sd)
{
	double x1, y1, w;

	do {
		double rndVal = randomDouble(0,1);
		x1 = 2.0 * rndVal - 1.0;
		rndVal = randomDouble(0,1);
		double x2 = 2.0 * rndVal - 1.0;
		w = x1*x1 + x2*x2;
	} while (w >= 1.0);

	w = sqrt((-2.0 * log(w))/w) ;
	y1 = x1*w;

	return m + y1 * sd;
}

//! @}

//! Returns used CPU time from T to current time and assigns current time to T.
/**
 * @ingroup date-time
 */
OGDF_EXPORT double usedTime(double& T);

//! Removes trailing space, horizontal and vertical tab, feed, newline, and carriage return  from \p str.
OGDF_EXPORT void removeTrailingWhitespace(string &str);

//! Compares the two strings \p str1 and \p str2, ignoring the case of characters.
OGDF_EXPORT bool equalIgnoreCase(const string &str1, const string &str2);

//! Tests if \p prefix is a prefix of \p str, ignoring the case of characters.
OGDF_EXPORT bool prefixIgnoreCase(const string &prefix, const string &str);

//! @addtogroup container-functions
//! @{

//! Searches for the position of \p x in container \p C; returns -1 if not found.
/**
 * Positions are number 0, 1, 2, ... The function uses the equality operator for comparing elements.
 *
 * \param C is a container.
 * \param x is the element to search for.
 * \tparam T is the type of the elements in \p C.
 * \return the position of the first occurrence of \p x in \p C (positions start with 0), or -1 if
 *         \p x is not in \p C.
 */
template< typename CONTAINER, typename T>
int searchPos(const CONTAINER &C, const T &x)
{
	int pos = 0;
	for (const T &y : C) {
		if (x == y)
			return pos;
		++pos;
	}

	return -1;
}

//! @}

//! Abstract base class for bucket functions.
/**
 * The parameterized class BucketFunc<E> is an abstract base class
 * for bucket functions. Derived classes have to implement #getBucket().
 * Bucket functions are used by bucket sort functions for container types.
 */
template<class E> class BucketFunc
{
public:
	virtual ~BucketFunc() { }

	//! Returns the bucket of \p x.
	virtual int getBucket(const E &x) = 0;
};

}
