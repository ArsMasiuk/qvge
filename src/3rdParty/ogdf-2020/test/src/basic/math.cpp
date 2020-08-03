/** \file
 * \brief Tests for Math.h
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

#include <typeinfo>

#include <ogdf/basic/Math.h>
#include <ogdf/basic/EpsilonTest.h>

#include <testing.h>

template<typename T>
static void testGcdAndLcm(const char *type)
{
	it("computes gcd of large numbers of type " + string(type), [] {
		T big = std::numeric_limits<T>::max();
		AssertThat(Math::gcd(big, big), Equals(big));
	});
	it("computes lcm of large numbers of type " + string(type), [] {
		T big = std::numeric_limits<T>::max();
		AssertThat(Math::lcm(big, big), Equals(big));
	});
}

static void testHarmonic()
{
	it("computes harmonic numbers correctly", []() {
		EpsilonTest eps;
		AssertThat(eps.equal(Math::harmonic(0), 1.0), IsTrue());
		AssertThat(eps.equal(Math::harmonic(1), 1.0), IsTrue());
		AssertThat(eps.equal(Math::harmonic(2), 1.5), IsTrue());
		AssertThat(eps.equal(Math::harmonic(3), 1.5 + 1/3.0), IsTrue());
		AssertThat(Math::harmonic(10), IsLessThan(3));
		AssertThat(Math::harmonic(11), IsGreaterThan(3));
		AssertThat(Math::harmonic(30), IsLessThan(4));
		AssertThat(Math::harmonic(31), IsGreaterThan(4));
		AssertThat(Math::harmonic(82), IsLessThan(5));
		AssertThat(Math::harmonic(83), IsGreaterThan(5));
		AssertThat(Math::harmonic(12366), IsLessThan(10));
		AssertThat(Math::harmonic(12367), IsGreaterThan(10));
	});
	it("computes huge harmonic numbers correctly", []() {
		unsigned i = 2012783313;
		double result;
		while ((result = Math::harmonic(i)) < 22.0) {
			i++;
		}
		AssertThat(i, Equals(2012783315u));
	});
}

namespace next_power_2 {

template<typename T>
void testSingle(T input, T expected) {
	AssertThat(Math::nextPower2(input), Equals(expected));
}

template<typename T>
void testJump(int exponent) {
	const T value = static_cast<T>(1) << exponent;

	testSingle<T>(value - 1, value);
	testSingle<T>(value, value);
	testSingle<T>(value + 1, value * 2);
}

template<typename T>
void test(const string &name, int maxExponent = sizeof(T)*8 - 1) {
	it("works with " + name, [&] {
		testSingle<T>(0, 0);
		testSingle<T>(1, 1);
		testSingle<T>(2, 2);

		for (int i = 2; i < maxExponent; i++) {
			testJump<T>(i);
		}
	});
}

template<typename T>
void testUnSigned(string name) {
	test<typename std::make_signed<T>::type>(name, sizeof(T)* 8 - 2);
	test<typename std::make_unsigned<T>::type>("unsigned " + name);
}

}

go_bandit([]() {
	describe("Math.h", []() {
		it("computes gcd with two arguments", []() {
			AssertThat(Math::gcd(5,7), Equals(1));
			AssertThat(Math::gcd(5,15), Equals(5));
			AssertThat(Math::gcd(6,9), Equals(3));
		});
		it("computes gcd with array of arguments", []() {
			AssertThat(Math::gcd(Array<int>({5,7,11})), Equals(1));
			AssertThat(Math::gcd(Array<int>({6,12,45})), Equals(3));
		});
		testGcdAndLcm<int>("int");
		testGcdAndLcm<unsigned int>("unsigned int");
		testGcdAndLcm<long>("long");
		testGcdAndLcm<unsigned long>("unsigned long");
		testGcdAndLcm<long long>("long long");
		testGcdAndLcm<unsigned long long>("unsigned long long");

		testHarmonic();

		describe("nextPower2", [] {
			next_power_2::test<char>("char");
			next_power_2::testUnSigned<short>("short");
			next_power_2::testUnSigned<int>("int");
			next_power_2::testUnSigned<long>("long");
			next_power_2::testUnSigned<long long>("long long");
		});
	});
});
