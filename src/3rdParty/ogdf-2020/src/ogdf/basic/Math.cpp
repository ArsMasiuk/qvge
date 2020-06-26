/** \file
 * \brief Implementation of mathematical constants, functions.
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

#include <ogdf/basic/Math.h>

namespace ogdf {
namespace Math {

int binomial(int n, int k)
{
	if(k>n/2) k = n-k;
	if(k == 0) return 1;
	int r = n;
	for(int i = 2; i<=k; ++i)
		r = (r * (n+1-i))/i;
	return r;
}

double binomial_d(int n, int k)
{
	if(k>n/2) k = n-k;
	if(k == 0) return 1.0;
	double r = n;
	for(int i = 2; i<=k; ++i)
		r = (r * (n+1-i))/i;
	return r;
}

constexpr double compiletimeHarmonic(unsigned n)
{
	return n <= 1 ? 1.0 : (compiletimeHarmonic(n-1) + 1.0 / n);
}

template<unsigned... Is>
struct seq { };
// rec_seq<3>{} : rec_seq<2,2>{} : rec_seq<1,1,2>{} : rec_seq<0,0,1,2> : seq<0,1,2>
template<unsigned N, unsigned... Is>
struct rec_seq : rec_seq<N-1, N-1, Is...> { };
template<unsigned... Is>
struct rec_seq<0, Is...> : seq<Is...> { };

constexpr unsigned compiletimeLimit = 128;

struct compiletimeTable {
	double value[compiletimeLimit];
};

template<unsigned... Is>
constexpr compiletimeTable generateCompiletimeHarmonics(seq<Is...>)
{
	return {{compiletimeHarmonic(Is)...}};
}

double harmonic(unsigned n)
{
	if (n < compiletimeLimit) {
		return generateCompiletimeHarmonics(rec_seq<compiletimeLimit>{}).value[n];
	}

	const double n_recip = 1.0 / n;
	const double n2_recip = n_recip * n_recip;
	const double n4_recip = n2_recip * n2_recip;
	const double n6_recip = n4_recip * n2_recip;
	const double n8_recip = n4_recip * n4_recip;
	const double n8_term = n8_recip / 240;
	const double n6_term = n6_recip / 252;
	const double n4_term = n4_recip / 120;
	const double n2_term = n2_recip / 12;
	const double n_term = n_recip / 2;
	return n8_term - n6_term + n4_term - n2_term + n_term + gamma + std::log(n);
}

}}
