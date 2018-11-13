/** \file
 * \brief Definition of MDMFLengthAttribute
 *
 * \author Thorsten Kerkhof
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
namespace embedder {

/**
 * Auxiliary length attribute
 *
 * It contains two components (\a a, \a b) and a linear order is defined by:
 * (\a a, \a b) > (\a a', \a b') iff \a a > \a a' or (\a a = \a a' and \a b > \a b')
 */
class MDMFLengthAttribute
{
public:
	//! Default constructor for (0, 0)
	MDMFLengthAttribute() { a = b = 0; }

	//! Converting constructor from int, default second is 0
	MDMFLengthAttribute(int _a, int _b = 0) : a(_a), b(_b) { }

	//! Copy constructor
	MDMFLengthAttribute(const MDMFLengthAttribute& x) : a(x.a), b(x.b) { }

	//! Destructor
	~MDMFLengthAttribute() { }

	MDMFLengthAttribute &operator=(const MDMFLengthAttribute& x) {
		a = x.a;
		b = x.b;
		return *this;
	}

	MDMFLengthAttribute &operator+=(const MDMFLengthAttribute& x) {
		a += x.a;
		b += x.b;
		return *this;
	}

	MDMFLengthAttribute &operator-=(const MDMFLengthAttribute& x) {
		a -= x.a;
		b -= x.b;
		return *this;
	}

	//the two components:
	int a;
	int b;
};

inline bool operator==(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y) {
	return x.a == y.a && x.b == y.b;
}

inline bool operator!=(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y) {
	return !(x == y);
}

inline bool operator<(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y) {
	return x.a < y.a || (x.a == y.a && x.b < y.b);
}

inline bool operator>(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y) {
	return y < x;
}

inline bool operator>=(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y) {
	return !(x < y);
}

inline bool operator<=(const MDMFLengthAttribute& x, const MDMFLengthAttribute& y) {
	return !(y < x);
}

inline MDMFLengthAttribute operator+(MDMFLengthAttribute x, const MDMFLengthAttribute& y) {
	x += y;
	return x;
}

inline MDMFLengthAttribute operator-(MDMFLengthAttribute x, const MDMFLengthAttribute& y) {
	x -= y;
	return x;
}

inline std::ostream& operator<<(std::ostream& s, const MDMFLengthAttribute& x) {
	s << x.a << ", " << x.b;
	return s;
}

}
}
