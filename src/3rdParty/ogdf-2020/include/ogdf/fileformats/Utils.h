/** \file
 * \brief Declaration of useful methods for processing various fileformats.
 *
 * \author Łukasz Hanuszczak
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

#include <map>

#include <ogdf/basic/basic.h>

namespace ogdf {

// Provides a nicer syntax for reading formatted input through streams, e.g.
// `stream >> a >> ';' >> y`.
class TokenIgnorer {
private:
	char m_c;

public:
	explicit TokenIgnorer(const char c): m_c(c) {};

	friend std::istream &operator >>(std::istream &is, TokenIgnorer c);
};


std::istream &operator >>(std::istream &is, TokenIgnorer token);

template <typename E>
static inline E toEnum(
	const std::string &str, // A string we want to convert.
	std::string toString(const E&),
	const E first, const E last, const E def) // Enum informations.
{
	static std::map<std::string, E> map; // A map to be lazily evaluated.
	if(map.empty()) {
		// Iterating over enums is potentially unsafe... (fixable in C++11).
		for(int it = static_cast<int>(last); it >= static_cast<int>(first); it--) {
			const E e = static_cast<E>(it);
			map[toString(e)] = e;
		}
	}

	return map.find(str) == map.end() ? def : map[str];
}

}
