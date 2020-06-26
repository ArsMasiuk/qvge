/** \file
 * \brief Declaration and implementation of class Tuple2, Tuple3
 *        and Tuple4.
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

#include <ogdf/basic/basic.h>
#include <ogdf/basic/Hashing.h>


namespace ogdf {

//! Tuples of two elements (2-tuples).
/**
 * @tparam E1 is the data type for the first element.
 * @tparam E2 is the data type for the second element.
 */
template<class E1, class E2> class Tuple2 {
public:
	E1 m_x1; //!< The first element.
	E2 m_x2; //!< The second element.

	//! Constructs a 2-tuple using default constructors.
	Tuple2() { }
	//! Constructs a 2-tuple for given values.
	Tuple2(const E1 &y1, const E2 &y2) : m_x1(y1), m_x2(y2) { }
	//! Constructs a 2-tuple that is a copy of \p t2.
	Tuple2(const Tuple2<E1,E2> &t2) : m_x1(t2.m_x1), m_x2(t2.m_x2) { }

	//! Returns a reference the first element.
	const E1 &x1() const { return m_x1; }
	//! Returns a reference the second element.
	const E2 &x2() const { return m_x2; }

	//! Returns a reference the first element.
	E1 &x1() { return m_x1; }
	//! Returns a reference the second element.
	E2 &x2() { return m_x2; }

	// default assignment operator
	Tuple2& operator=(const Tuple2<E1,E2>&) = default;

	OGDF_NEW_DELETE
};

//! Equality operator for 2-tuples
template<class E1, class E2>
bool operator==(const Tuple2<E1,E2> &t1, const Tuple2<E1,E2> &t2)
{
	return t1.x1() == t2.x1() && t1.x2() == t2.x2();
}

//! Inequality operator for 2-tuples
template<class E1, class E2>
bool operator!=(const Tuple2<E1,E2> &t1, const Tuple2<E1,E2> &t2)
{
	return t1.x1() != t2.x1() || t1.x2() != t2.x2();
}

//! Output operator for 2-tuples.
template<class E1, class E2>
std::ostream &operator<<(std::ostream &os, const Tuple2<E1,E2> &t2)
{
	os << "(" << t2.x1() << " " << t2.x2() << ")";
	return os;
}



template<typename K1_, typename K2_,
	typename Hash1_ = DefHashFunc<K1_>,
	typename Hash2_ = DefHashFunc<K2_> >
class HashFuncTuple
{
public:
	HashFuncTuple() { }

	HashFuncTuple(const Hash1_ &hash1, const Hash2_ &hash2)
		: m_hash1(hash1), m_hash2(hash2) { }

	size_t hash(const Tuple2<K1_,K2_> &key) const {
		return 23*m_hash1.hash(key.x1()) + 443*m_hash2.hash(key.x2());
	}

private:
	Hash1_ m_hash1;
	Hash2_ m_hash2;
};

}
