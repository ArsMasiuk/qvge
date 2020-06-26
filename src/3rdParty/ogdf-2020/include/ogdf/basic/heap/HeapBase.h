/** \file
 * \brief Interface for heap implementations.
 *
 * \author Tilo Wiedera
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

#include <stdexcept>

#include <ogdf/basic/basic.h>

namespace ogdf {

/**
 * Common interface for all heap classes.
 *
 * @tparam IMPL The type of heap.
 * @tparam H The type of handle to use.
 *         Such a handle will be given to the user for each pushed value.
 * @tparam T The type of values to be stored.
 * @tparam C The comparator used to order the stored values.
 */
template<
  typename IMPL,
  typename H,
  typename T,
  typename C
>
class HeapBase
{

	C m_comp;

public:

	/**
	 * The type of handle used to identify stored values.
	 * The handle type accessible from outside of the heap will always be a pointer.
	 */
	using Handle = H*;

	explicit HeapBase(C const &comp = C()) : m_comp(comp) {}

	/**
	 * Returns the comparator used to sort the values in the heap.
	 *
	 * @return The comparator for sorting the heaps values
	 */
	virtual const C &comparator() const { return m_comp; }

	/**
	 * Returns the topmost value in the heap.
	 *
	 * @return the topmost value
	 */
	virtual const T &top() const = 0;

	/**
	 * Inserts a value into the heap.
	 *
	 * @param value The value to be inserted
	 * @return A handle to access and modify the value
	 */
	virtual Handle push(const T &value) = 0;

	/**
	 * Removes the topmost value from the heap.
	 */
	virtual void pop() = 0;

	/**
	 * Decreases a single value.
	 *
	 * @param handle The handle of the value to be decreased
	 * @param value The decreased value. This must be less than the former value
	 */
	virtual void decrease(Handle handle, const T &value) = 0;

	/**
	 * Returns the value of that handle.
	 *
	 * @param handle The handle
	 * @return The value
	 */
	virtual const T &value(const Handle handle) const = 0;

	/**
	 * Merges in values of \p other heap.
	 *
	 * After merge \p other heap becomes empty and is valid for further usage.
	 *
	 * @param other A heap to be merged in.
	 */
	virtual void merge(IMPL &other);

};


template<
  typename IMPL,
  typename H,
  typename T,
  typename C
>
void HeapBase<IMPL, H, T, C>::merge(IMPL &/*other*/) {
	throw std::runtime_error("Merging two binary heaps is not supported");
}

}
