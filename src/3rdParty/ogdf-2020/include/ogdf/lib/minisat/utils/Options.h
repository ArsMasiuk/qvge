/***************************************************************************************[Options.h]
Copyright (c) 2008-2010, Niklas Sorensson

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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <ogdf/lib/minisat/mtl/IntTypes.h>
#include <ogdf/lib/minisat/mtl/Vec.h>
#include <ogdf/lib/minisat/utils/ParseUtils.h>

namespace Minisat {
namespace Internal {

//==================================================================================================
// Range classes with specialization for floating types:


struct IntRange {
    int begin;
    int end;
    IntRange(int b, int e) : begin(b), end(e) {}
};

struct DoubleRange {
    double begin;
    double end;
    bool  begin_inclusive;
    bool  end_inclusive;
    DoubleRange(double b, bool binc, double e, bool einc) : begin(b), end(e), begin_inclusive(binc), end_inclusive(einc) {}
};


//==================================================================================================
// Double options:


class DoubleOption
{
 protected:
    DoubleRange range;
    double      value;

 public:
    DoubleOption(const char* d, double def = double(), DoubleRange r = DoubleRange(-HUGE_VAL, false, HUGE_VAL, false))
        : range(r), value(def) {
    }

    operator      double   (void) const { return value; }
    operator      double&  (void)       { return value; }
    DoubleOption& operator=(double x)   { value = x; return *this; }
};


//==================================================================================================
// Int options:


class IntOption
{
 protected:
    IntRange range;
    int32_t  value;

 public:
    IntOption(const char* d, int32_t def = int32_t(), IntRange r = IntRange(INT32_MIN, INT32_MAX))
        : range(r), value(def) {}

    operator   int32_t   (void) const { return value; }
    operator   int32_t&  (void)       { return value; }
    IntOption& operator= (int32_t x)  { value = x; return *this; }
};



//==================================================================================================
// Bool option:


class BoolOption
{
    bool value;

 public:
    BoolOption(const char* d, bool v)
        : value(v) {}

    operator    bool     (void) const { return value; }
    operator    bool&    (void)       { return value; }
    BoolOption& operator=(bool b)     { value = b; return *this; }
};

//=================================================================================================
} // namespace Internal
} // namespace Minisat
