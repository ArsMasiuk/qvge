/** \file
 * \brief Implementation of algorithms as templates working with
 *        different list types
 *
 * \author Carsten Gutwenger, Tilo Wiedera
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

#include <ogdf/basic/Array.h>
#include <functional>

namespace ogdf {

/**
 * Calls (possibly destructive) \p func for each element of \p container
 *
 * "Destructive" means that the current iterator of \p container may
 * be deleted during the processing of \p func. It works by saving
 * the successor of the current element before calling \p func.
 */
template<typename CONTAINER>
inline void safeForEach(CONTAINER &container, std::function<void(typename CONTAINER::value_type)> func) {
	for (auto it = container.begin(); it != container.end();) {
		auto prev = it++;
		func(*prev);
	}
}

/**
 * Like ogdf::safeForEach() but aborts if \p func returns \c false
 *
 * @return false if \p func returns false,
 *         true if the whole \p container was processed
 */
template<typename CONTAINER>
inline bool safeTestForEach(CONTAINER &container, std::function<bool(typename CONTAINER::value_type)> func) {
	for (auto it = container.begin(); it != container.end();) {
		auto prev = it++;
		if (!func(*prev)) {
			return false;
		}
	}
	return true;
}

// sorts list L using quicksort
template<class LIST>
void quicksortTemplate(LIST &L)
{
	auto comparer = StdComparer<typename LIST::value_type>();
	quicksortTemplate(L, comparer);
}


// sorts list L using quicksort and compare element comp
template<class LIST, class COMPARER>
void quicksortTemplate(LIST &L, const COMPARER &comp)
{
	const int n = L.size();
	Array<typename LIST::value_type> A(n);

	int i = 0;
	for (const typename LIST::value_type &x : L)
		A[i++] = x;

	A.quicksort(comp);

	i = 0;
	for (typename LIST::value_type &x : L)
		x = A[i++];
}

namespace internal {

/**
 * @see chooseIteratorFrom
 *
 * Don't allocate additional space but count the number of feasible elements instead.
 */
template<typename CONTAINER, typename TYPE, typename ITERATOR>
ITERATOR chooseIteratorByFastTest(
		CONTAINER &container,
		std::function<bool(const TYPE &)> includeElement) {
	int nElements = 0;

	for(const auto &e : container) {
		nElements += includeElement(e) ? 1 : 0;
	}

	ITERATOR result = container.end();

	if(nElements > 0) {
		int chosenElement = randomNumber(1, nElements);
		int elemCounter = 0;

		for (ITERATOR it = container.begin(); result == container.end(); it++) {
			if(includeElement(*it)) {
				elemCounter++;

				if(elemCounter == chosenElement) {
					result = it;
				}
			}
		}
	}

	return result;
};

/**
 * @see chooseIteratorFrom
 *
 * Store elements in permuted order and call includeElement at most once per element.
 */
template<typename CONTAINER, typename TYPE, typename ITERATOR>
ITERATOR chooseIteratorBySlowTest(
		CONTAINER &container,
		std::function<bool(const TYPE &)> includeElement,
		int size) {
	Array<ITERATOR> other(size);

	int i = 0;
	for (ITERATOR it = container.begin(); it != container.end(); it++) {
		other[i] = it;
		i++;
	}

	other.permute();

	ITERATOR result = container.end();

	for (auto it : other) {
		if (includeElement(*it)) {
			result = it;
			break;
		}
	}

	return result;
};


/**
 * Returns an iterator to a random element in the \p container.
 *
 * Takes linear time (given that \p includeElement runs in constant time).
 * An invalid iterator is returned iff no feasible element exists.
 * When \p includeElement has a non-constant runtime it is recommended to set \p isFastTest to \c false.
 *
 * @tparam CONTAINER Type of the container.
 *                   Any iterable container that implements \c size() is applicable.
 * @tparam TYPE Type of elements returned by the iterator of the container.
 * @param container The container that we want to pick an element from.
 * @param includeElement Specifies for each element whether it is feasible to be chosen.
 *                       Defaults to all elements being feasible.
 *                       Must return the same value when called twice with the same element.
 * @param isFastTest Should be set to false to prevent querying the same element multiple times for feasibility.
 *                   Note that this will result in additional space allocated linear in the size of the container.
 * @return An iterator to the picked element or an invalid iterator if no such element exists.
 */
template<typename CONTAINER, typename TYPE, typename ITERATOR>
ITERATOR chooseIteratorFrom(
		CONTAINER &container,
		std::function<bool(const TYPE &)> includeElement,
		bool isFastTest) {
	ITERATOR result = container.begin();
	int size = container.size();

	if (size > 0) {
		// let's try to pick *any* element
		int index = randomNumber(0, size - 1);

		for (int i = 0; i < index; i++) {
			result++;
		}

		// the initially chosen element is not feasible?
		if (!includeElement(*result)) {
			if(isFastTest) {
				result = chooseIteratorByFastTest<CONTAINER, TYPE, ITERATOR>(container, includeElement);
			} else {
				result = chooseIteratorBySlowTest<CONTAINER, TYPE, ITERATOR>(container, includeElement, size);
			}
		}
	}

	return result;
}


}

//! @copydoc ogdf::internal::chooseIteratorFrom
template<typename CONTAINER, typename TYPE>
typename CONTAINER::iterator chooseIteratorFrom(
		CONTAINER &container,
		std::function<bool(const TYPE&)> includeElement = [](const TYPE&) { return true; },
		bool isFastTest = true) {
	return internal::chooseIteratorFrom
			<CONTAINER, TYPE, typename CONTAINER::iterator>(container, includeElement, isFastTest);
}

//! @copydoc ogdf::internal::chooseIteratorFrom
template<typename CONTAINER, typename TYPE>
typename CONTAINER::const_iterator chooseIteratorFrom(
		const CONTAINER &container,
		std::function<bool(const TYPE&)> includeElement = [](const TYPE&) { return true; },
		bool isFastTest = true) {
	return internal::chooseIteratorFrom
			<const CONTAINER, TYPE, typename CONTAINER::const_iterator>(container, includeElement, isFastTest);
}

}
