/** \file
 * \brief Mathematical Helpers
 *
 * \author Markus Chimani, Ivo Hedtke
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

#include <numeric>

#include <ogdf/basic/basic.h>
#include <ogdf/basic/Array.h>


namespace ogdf {
namespace Math {
namespace internal {

template<typename T, int size>
inline typename std::enable_if<size == 1, T>::type
nextPower2(T x) {
	return x;
}

//! Efficiently computes the next power of 2 without branching.
//! See "Hacker's Delight" 2nd Edition, by Henry S. Warren, Fig. 3.3
template<typename T, int size>
inline typename std::enable_if<size != 1, T>::type
nextPower2(T x) {
	x = nextPower2<T, size/2>(x);
	return x | (x >> size/2);
}

}

//! The constant \f$\pi\f$.
constexpr double pi = 3.14159265358979323846;

//! The constant \f$\frac{\pi}{2}\f$.
constexpr double pi_2 = 1.57079632679489661923;

//! The constant \f$\frac{\pi}{180}\f$.
constexpr double pi_180 = 0.01745329251994329576;

//! The constant \f$\frac{180}{\pi}\f$.
constexpr double one_rad = 57.29577951308232087679;

//! The constant log(4.0).
const double log_of_4 = log(4.0);

//! The Euler-Mascheroni constant gamma
constexpr double gamma = 0.57721566490153286061;

//! Returns the smallest power of 2 that is no less than the given (integral) argument.
template<typename T>
inline T nextPower2(T x) {
	return internal::nextPower2<T, sizeof(T)*8>(x - 1) + 1;
}

//! Returns the smallest power of 2 that is no less than the given (integral) arguments.
template<typename T, typename... Args>
inline static T nextPower2(T arg1, T arg2, Args... args) {
	return nextPower2(std::max(arg1, arg2, args...));
}

//! Stores the maximum of \p max and \p newValue in \p max.
template<typename T>
inline void updateMax(T& max, const T& newValue) {
	if(max < newValue) {
		max = newValue;
	}
}

//! Stores the minimum of \p min and \p newValue in \p min.
template<typename T>
inline void updateMin(T& min, const T& newValue) {
	if(min > newValue) {
		min = newValue;
	}
}

//! Returns the logarithm of \p x to the base 2.
template<typename T>
OGDF_DEPRECATED("Use std::log2(x).")
inline T log2(T x) {
	OGDF_ASSERT(x > 0);
	return std::log2(x);
}

//! Returns the logarithm of \p x to the base 4.
inline double log4(double x) {
	OGDF_ASSERT(x > 0);
	return log(x) / log_of_4;
}

//! Returns +1 for val > 0, 0 for val = 0, and -1 for val < 0
template <typename T>
inline int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

//! Converts an angle from degrees to radians.
inline double degreesToRadians(const double &angleInDegrees) {
	return angleInDegrees * Math::pi_180;
}

//! Converts an angle from radians to degrees.
inline double radiansToDegrees(const double &angleInRadians) {
	return angleInRadians * Math::one_rad;
}

//! Returns \f$n \choose k\f$.
OGDF_EXPORT int binomial(int n, int k);

//! Returns \f$n \choose k\f$.
OGDF_EXPORT double binomial_d(int n, int k);

//! Returns \p n!.
OGDF_DEPRECATED("Use std::tgamma(n+1).")
inline int factorial(int n) {
	return (int) std::tgamma(n+1);
}

//! Returns \p n!.
OGDF_DEPRECATED("Use std::tgamma(n+1).")
inline double factorial_d(int n) {
	return std::tgamma(n+1);
}

//! Returns the \p n-th harmonic number or 1.0 if \p n < 1.
OGDF_EXPORT double harmonic(unsigned n);

/*!
 * \brief A method to obtain the rounded down binary logarithm of \p v.
 *
 * @param v The number of which the binary logarithm is to be determined
 * @return The rounded down logarithm base 2 if v is positive, -1 otherwise
 */
OGDF_DEPRECATED("Use std::ilogb(v).")
inline int floorLog2(int v) {
	if (v <= 0) {
		return -1;
	} else {
		return std::ilogb(v);
	}
}

//! Returns the greatest common divisor of two numbers.
template<typename T>
inline T gcd(T a, T b) {
	// If b > a, they will be swapped in the first iteration.
	do {
		T c = a % b;
		a = b;
		b = c;
	} while (b > 0);
	return a;
}

//! Returns the greatest common divisor of a list of numbers.
template<class T, class INDEX = int>
inline T gcd(const Array<T, INDEX> &numbers) {
	T current_gcd = numbers[numbers.low()];
	for (INDEX i = numbers.low()+1; i <= numbers.high(); i++) {
		current_gcd = gcd(current_gcd, numbers[i]);
	}
	return current_gcd;
}

//! Returns the least common multipler of two numbers.
template<typename T>
inline T lcm(T a, T b) {
	T g = gcd(a,b);
	OGDF_ASSERT(g != 0);
	return (a / g) * b;
}

//! Converts a double to a fraction.
inline void getFraction(double d, int &num, int &denom, const double epsilon = 5e-10, const int count = 10) {
	ArrayBuffer<int> continuedFrac;

	// build continued fraction
	int z((int)d);
	continuedFrac.push(z);
	d = d - z;
	int i = 0;
	while (d > epsilon && i++ < count) {
		d = 1 / d;
		z = (int)d;
		continuedFrac.push(z);
		d = d - z;
	}

	// simplify continued fraction to simple fraction
	num = 1;
	denom = 0;
	while (!continuedFrac.empty()) {
		int last = continuedFrac.popRet();
		std::swap(num, denom);
		num += last * denom;
	}
}

//! Returns the minimum of an iterable container of given \p values.
template<class Container>
inline typename Container::value_type minValue(const Container &values) {
	OGDF_ASSERT(!values.empty());
	return *std::min_element(values.begin(), values.end());
}

//! Returns the maximum of an iterable container of given \p values.
template<class Container>
inline typename Container::value_type maxValue(const Container &values) {
	OGDF_ASSERT(!values.empty());
	return *std::max_element(values.begin(), values.end());
}

//! Returns the sum of an iterable container of given \p values.
template<class Container>
inline typename Container::value_type sum(const Container &values) {
	return std::accumulate(values.begin(), values.end(), static_cast<typename Container::value_type>(0));
}

//! Returns the mean of an iterable container of given \p values.
template<class Container>
inline double mean(const Container &values) {
	OGDF_ASSERT(!values.empty());
	return sum(values) / static_cast<double>(values.size());
}

//! @copydoc standardDeviation(const Container&)
//! The given \p mean is used instead of computing a new one.
template<class Container>
inline double standardDeviation(const Container &values, double mean) {
	OGDF_ASSERT(!values.empty());
	double sum = 0;
	for (auto value : values) {
		double d = value - mean;
		sum += d*d;
	}
	return sqrt(sum / values.size());
}

//! Returns the standard deviation of an iterable container of given \p values.
template<class Container>
inline double standardDeviation(const Container &values) {
	return standardDeviation(values, mean(values));
}

}}
