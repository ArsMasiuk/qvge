/** \file
 * \brief Implementation of the Reverse class for the reverse iteration of
 * containers.
 *
 * \author Tilo Wiedera, Max Ilsen
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

#include <utility>

namespace ogdf {

//! A wrapper class to easily iterate through a container in reverse.
/**
 * @ingroup container-functions
 *
 * @tparam T denotes the container type.
 */
template<typename T>
class Reverse {
	//! The container for which reverse iterators should be provided.
	T &m_container;

public:
	//! Creates a reverse iteration wrapper for \p container.
	explicit Reverse(T &container) : m_container(container) { }

	//! Provides a reverse iterator disguised a normal iterator.
	using iterator = typename std::conditional<std::is_const<T>::value,
	                 typename T::const_reverse_iterator,
	                 typename T::reverse_iterator>::type;

	//! Returns a reverse iterator to the last element of #m_container.
	iterator begin() { return m_container.rbegin(); }

	//! Returns a reverse iterator to the one-before-first element of #m_container.
	iterator end() { return m_container.rend(); }
};

//! Provides iterators for \p container to make it easily iterable in reverse.
/**
 * @ingroup container-functions
 *
 * @code for (auto elem : reverse(container)) { ... } @endcode
 * @tparam T denotes the type of \p container.
 * @param container is the container to be reversed.
 */
template<typename T>
Reverse<T> reverse(T &container) {
	return Reverse<T>(container);
}

}
