/** \file
 * \brief Compare floating point numbers with epsilons and integral numbers
 * with normal compare operators.
 *
 * \author Ivo Hedtke
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
#include <type_traits>

namespace ogdf {

class EpsilonTest {
private:
	const double eps; //!< Epsilon for floating point comparisons.

public:
	//! Constructs an EpsilonTest with a given epsilon (double) for comparisons.
	explicit EpsilonTest(double epsilon = 1.0e-8) : eps(epsilon) {}

	// LESS: integral and floating_point
	//! Compare if x is LESS than y for integral types.
	/**
	 * @param x is the left parameter of the operator LESS
	 * @param y is the right parameter of the operator LESS
	 * \pre x and y are of integral type
	 */
	template<typename T>
	inline typename std::enable_if<std::is_integral<T>::value, bool>::type
		less(const T &x, const T &y) const
	{
			return x < y;
	}

	//! Compare if x is LESS than y for floating point types, using the given
	//! epsilon.
	/**
	 * @param x is the left parameter of the operator LESS
	 * @param y is the right parameter of the operator LESS
	 * \pre x and y are of floating point type
	 */
	template<typename T>
	inline typename std::enable_if<std::is_floating_point<T>::value, bool>::type
		less(const T &x, const T &y) const
	{
			return x < (y - eps);
	}

	// LEQ: integral and floating_point
	//! Compare if x is LEQ than y for integral types.
	/**
	 * @param x is the left parameter of the operator LEQ
	 * @param y is the right parameter of the operator LEQ
	 * \pre x and y are of integral type
	 */
	template<typename T>
	inline typename std::enable_if<std::is_integral<T>::value, bool>::type
		leq(const T &x, const T &y) const
	{
			return x <= y;
	}

	//! Compare if x is LEQ than y for floating point types, using the given
	//! epsilon.
	/**
	 * @param x is the left parameter of the operator LEQ
	 * @param y is the right parameter of the operator LEQ
	 * \pre x and y are of floating point type
	 */
	template<typename T>
	inline typename std::enable_if<std::is_floating_point<T>::value, bool>::type
		leq(const T &x, const T &y) const
	{
			return x < (y + eps);
	}

	// EQUAL: integral and floating_point
	//! Compare if x is EQUAL to y for integral types.
	/**
	 * @param x is the left parameter of the operator EQUAL
	 * @param y is the right parameter of the operator EQUAL
	 * \pre x and y are of integral type
	 */
	template<typename T>
	inline typename std::enable_if<std::is_integral<T>::value, bool>::type
		equal(const T &x, const T &y) const
	{
			return x == y;
	}

	//! Compare if x is EQUAL to y for floating point types, using the given
	//! epsilon.
	/**
	 * @param x is the left parameter of the operator EQUAL
	 * @param y is the right parameter of the operator EQUAL
	 * \pre x and y are of floating point type
	 */
	template<typename T>
	inline typename std::enable_if<std::is_floating_point<T>::value, bool>::type
		equal(const T &x, const T &y) const
	{
			return leq(x, y) && geq(x, y);
	}

	// GEQ: integral and floating_point
	//! Compare if x is GEQ to y for integral types.
	/**
	 * @param x is the left parameter of the operator GEQ
	 * @param y is the right parameter of the operator GEQ
	 * \pre x and y are of integral type
	 */
	template<typename T>
	inline typename std::enable_if<std::is_integral<T>::value, bool>::type
		geq(const T &x, const T &y) const
	{
			return x >= y;
	}

	//! Compare if x is GEQ to y for floating point types, using the given
	//! epsilon.
	/**
	 * @param x is the left parameter of the operator GEQ
	 * @param y is the right parameter of the operator GEQ
	 * \pre x and y are of floating point type
	 */
	template<typename T>
	inline typename std::enable_if<std::is_floating_point<T>::value, bool>::type
		geq(const T &x, const T &y) const
	{
			return x > (y - eps);
	}

	// GREATER: integral and floating_point
	//! Compare if x is GREATER than y for integral types.
	/**
	 * @param x is the left parameter of the operator GREATER
	 * @param y is the right parameter of the operator GREATER
	 * \pre x and y are of integral type
	 */
	template<typename T>
	inline typename std::enable_if<std::is_integral<T>::value, bool>::type
		greater(const T &x, const T &y) const
	{
			return x > y;
	}

	//! Compare if x is GREATER than y for floating point types, using the given
	//! epsilon.
	/**
	 * @param x is the left parameter of the operator GREATER
	 * @param y is the right parameter of the operator GREATER
	 * \pre x and y are of floating point type
	 */
	template<typename T>
	inline typename std::enable_if<std::is_floating_point<T>::value, bool>::type
		greater(const T &x, const T &y) const
	{
			return x > (y + eps);
	}
};

}
