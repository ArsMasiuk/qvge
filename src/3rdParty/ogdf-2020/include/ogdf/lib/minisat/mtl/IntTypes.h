/**************************************************************************************[IntTypes.h]
Copyright (c) 2009-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#pragma once

#ifdef __sun
    // Not sure if there are newer versions that support C99 headers. The
    // needed features are implemented in the headers below though:

#   include <sys/int_types.h>
#   include <sys/int_fmtio.h>
#   include <sys/int_limits.h>

#elif _MSC_VER

#   include <stdint.h>

#else

#   include <stdint.h>
#   include <inttypes.h>
#	include <limits.h>

#endif

#include <limits>

	#ifndef INT32_MIN
		#define INT32_MIN std::numeric_limits<int>::min()
	#endif
	#ifndef INT32_MAX
		#define INT32_MAX std::numeric_limits<int>::max()
	#endif
	#ifndef INT64_MIN
		#define INT64_MIN std::numeric_limits<long>::min()
	#endif
	#ifndef INT64_MAX
		#define INT64_MAX std::numeric_limits<long>::max()
	#endif
	#ifndef UINT32_MIN
		#define UINT32_MIN std::numeric_limits<unsigned int>::min()
	#endif
	#ifndef UINT32_MAX
		#define UINT32_MAX std::numeric_limits<unsigned int>::max()
	#endif
	#ifndef UINT64_MIN
		#define UINT64_MIN std::numeric_limits<unsigned long>::min()
	#endif
	#ifndef UINT64_MAX
		#define UINT64_MAX std::numeric_limits<unsigned long>::max()
	#endif


//=================================================================================================
