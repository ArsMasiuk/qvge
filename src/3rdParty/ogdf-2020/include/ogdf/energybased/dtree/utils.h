/** \file
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

#include <cstddef>
#include <type_traits>

namespace ogdf {
namespace energybased {
namespace dtree {

template<typename IntType, int Dim>
inline typename std::enable_if<Dim != 1 && Dim != 2, bool>::type
mortonComparerEqual(const IntType a[Dim], const IntType b[Dim]) {
	// loop over the dimension
	for (int d = Dim - 1; d >= 0; d--) {
		// if the block is different
		if (a[d] != b[d]) {
			return false;
		}
	}
	return true;
}

// special tuned version for unsigned int and dim = 1
template<typename IntType, int Dim>
inline typename std::enable_if<Dim == 1, bool>::type
mortonComparerEqual(const IntType a[Dim], const IntType b[Dim]) {
	return a[0] == b[0];
}

// special tuned version for unsigned int and dim = 2
template<typename IntType, int Dim>
inline typename std::enable_if<Dim == 2, bool>::type
mortonComparerEqual(const IntType a[Dim], const IntType b[Dim]) {
	return a[0] == b[0] && a[1] == b[1];
}

template<typename IntType, int Dim>
inline typename std::enable_if<Dim != 1 && Dim != 2, bool>::type
mortonComparerLess(const IntType a[Dim], const IntType b[Dim]) {
	// loop over the dimension
	for (int d = Dim - 1; d >= 0; d--) {
		// if the block is different
		if (a[d] != b[d]) {
			return a[d] < b[d];
		}
	}
	return false;
}

// special tuned version for unsigned int and dim = 1
template<typename IntType, int Dim>
inline typename std::enable_if<Dim == 1, bool>::type
mortonComparerLess(const unsigned int a[Dim], const unsigned int b[Dim]) {
	return a[0] < b[0];
}

// special tuned version for unsigned int and dim = 2
template<typename IntType, int Dim>
inline typename std::enable_if<Dim == 2, bool>::type
mortonComparerLess(const unsigned int a[Dim], const unsigned int b[Dim]) {
	return (a[1] == b[1]) ? a[0] < b[0] : a[1] < b[1];
}

template<typename IntType, int Dim>
inline typename std::enable_if<Dim != 1 && Dim != 2, void>::type
interleaveBits(const IntType coords[Dim], IntType mnr[Dim]) {
	// number of bits of the grid coord type
	const int BitLength = sizeof(IntType) << 3;

	// loop over the dimension
	for (int d = 0; d < Dim; d++) {
		// reset the mnr
		mnr[d] = 0x0;
	}

	// counter for the res bit
	int k = 0;

	// now loop over all bits
	for (int i = 0; i < BitLength; ++i) {
		// loop over the dimension
		for (int d = 0; d < Dim; d++) {
			// set the k-th bit in the correct block at the k % position
			mnr[k / BitLength] |= ((coords[d] >> i) & 0x1) << ( k % BitLength);

			// stupid increment
			k++;
		}
	};
}

template<typename IntType, int Dim>
inline typename std::enable_if<Dim == 1, void>::type
interleaveBits(const unsigned int coords[Dim], unsigned int mnr[Dim]) {
	mnr[0] = coords[0];
}

template<typename IntType, int Dim>
inline typename std::enable_if<Dim == 2, void>::type
interleaveBits(const unsigned int coords[Dim], unsigned int mnr[Dim]) {
	// half the bit length = #bytes * 4
	const size_t HalfBitLength  = sizeof(unsigned int) << 2;

	// reset the mnr
	mnr[0] = 0x0;
	mnr[1] = 0x0;

	// this variable is used to generate an alternating pattern for the
	// lower half bits of both coords (the higher half will be shifted out later)
	unsigned int x_lo[2] = { coords[0], coords[1] };

	// this one is used for the higher half, thus, we shift them to right
	unsigned int x_hi[2] = { coords[0] >> HalfBitLength, coords[1] >> HalfBitLength };

	// a mask full of 1's
	unsigned int mask = ~0x0;

	for (unsigned int i = (HalfBitLength);i > 0; i = i >> 1) {
		// increase frequency
		// generates step by step: ..., 11110000, 11001100, 10101010
		mask = mask ^ (mask << i);

		// create an alternating 0x0x0x0x0x pattern for the lower bits
		x_lo[0] = (x_lo[0] | (x_lo[0] << i)) & mask;
		x_lo[1] = (x_lo[1] | (x_lo[1] << i)) & mask;
		// and for the higher bits too
		x_hi[0] = (x_hi[0] | (x_hi[0] << i)) & mask;
		x_hi[1] = (x_hi[1] | (x_hi[1] << i)) & mask;
	};

	// save the lower bits alternating in the first block
	mnr[0] = x_lo[0] | (x_lo[1] << 1);

	// the higher go into the second block
	mnr[1] = x_hi[0] | (x_hi[1] << 1);
}


template<typename IntType>
inline int mostSignificantBit(IntType x)
{
	// number of bits of the int type
	const size_t BitLength = sizeof(IntType) << 3;

	// the index  0... BitLength - 1 of the msb
	int result = 0;

	// binary search on the bits of x
	for (unsigned int i = (BitLength >> 1); i > 0; i = i >> 1) {
		// check if anything left of i - 1 is set
		if ( x >> i) {
			// it is msb must be in that half
			x = x >> i;

			// msb > i
			result += i;
		}
	}

	// return the result
	return result;
}

template<typename IntType, int Dim>
inline typename std::enable_if<Dim != 1, int>::type
lowestCommonAncestorLevel(const IntType a[Dim], const IntType b[Dim]) {
	// number of bits of the grid coord type
	const size_t BitLength = sizeof(IntType) << 3;

	// loop over the dimension
	for (int d = Dim - 1; d >= 0; d--) {
		// if the block is different
		if (a[d] != b[d]) {
			int msb = (mostSignificantBit<IntType>(a[d] ^ b[d]) + (d * BitLength));

			// the lowest common ancestor level is msb / num coords
			return msb / Dim;
		}
	}

	return 0;
}

template<typename IntType, int Dim>
inline typename std::enable_if<Dim == 1, int>::type
lowestCommonAncestorLevel(const unsigned int a[Dim], const unsigned int b[Dim]) {
	return mostSignificantBit<unsigned int>(a[0] ^ b[0]);
}

}}}
