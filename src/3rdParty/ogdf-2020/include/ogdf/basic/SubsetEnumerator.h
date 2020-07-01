/** \file
 * \brief A class that allows to enumerate k-subsets.
 *
 * \author Stephan Beyer
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

#include <ogdf/basic/List.h>

namespace ogdf {

//! Enumerator for k-subsets of a given type.
/**
 * <H3>Usage examples</H3>
 * <ul>
 *   <li> Enumerate all subsets of edges with cardinality 3:
 *    \code
 *     List<edge> edges;
 *
 *     do_something_eg_fill_edges();
 *
 *     SubsetEnumerator<edge> subset(edges);
 *
 *     for (subset.begin(3); subset.valid(); subset.next()) {
 *       do_something_with(subset[0], subset[1], subset[2]);
 *     }
 *    \endcode
 *   </li>
 *   <li> Enumerate all subsets of edges:
 *    \code
 *     SubsetEnumerator<edge> subset(edges);
 *
 *     for (subset.begin(); subset.valid(); subset.next()) {
 *       for (int i = 0; i < subset.size(); ++i) {
 *         do_something_with(subset[i]);
 *       }
 *       do_stuff();
 *     }
 *    \endcode
 *   </li>
 *   <li> Do something with member lists and complement lists of all 2-, 3-, and 4-element subsets
 *    \code
 *     SubsetEnumerator<edge> subset(edges);
 *
 *     for (subset.begin(2, 4); subset.valid(); subset.next()) {
 *       List<edge> list1, list2;
 *       subset.list(list1, list2);
 *       // if subset = { 1, 3, 4 } of { 1, 2, 3, 4, 5 },
 *       // then list1 = 1 3 4 and list2 = 2 5
 *       do_something_with(list1);
 *       do_another_things_with(list2);
 *     }
 *    \endcode
 *   </li>
 * </ul>
 *
 * Please note that the internal data structures of SubsetEnumerator do not use references of the type T.
 * Hence, T should either be a simple type or a pointer to a complex type (which is also only sane for Lists, too).
 * Otherwise the data structure will slow down due to extensive implicit copying.
 */
template<typename T>
class SubsetEnumerator {
protected:
	bool m_valid;
	int m_maxCard;
	ArrayBuffer<T> m_subset;
	Array<int> m_index;

	void initSubset(int card)
	{
		if (card >= 0 && card <= m_subset.size()) {
			m_index.init(card);
			for (int i = 0; i < card; ++i) {
				m_index[i] = i;
			}
			m_valid = true;
		}
	}

public:
	/**
	 * Constructor.
	 *
	 * @param set The container of elements we want to enumerate subsets for.
	 */
	template<typename ContainerType>
	explicit SubsetEnumerator(const ContainerType &set)
	  : m_valid(false)
	  , m_maxCard(-1)
	  , m_subset(set.size())
	{
		for (auto x : set) {
			m_subset.push(x);
		}
	}

	//! Initializes the SubsetEnumerator to enumerate subsets of cardinalities from low to high.
	void begin(int low, int high)
	{
		if (high >= low) {
			m_maxCard = high;
			m_maxCard = min(m_maxCard, m_subset.size());
			initSubset(low);
		} else {
			m_valid = false;
		}
	}

	//! Initializes the SubsetEnumerator to enumerate subsets of given cardinality.
	void begin(int card)
	{
		begin(card, card);
	}

	//! Initializes the SubsetEnumerator to enumerate all subsets.
	void begin()
	{
		begin(0, m_subset.size());
	}

	//! Returns the cardinality of the subset.
	int size() const
	{
		return m_index.size();
	}

	//! Returns the cardinality of the (super-)set.
	//! This is the maximum size that can be used for a subset.
	int numberOfMembersAndNonmembers() const
	{
		return m_subset.size();
	}

	//! Checks if the current subset is valid.
	//! If not, the subset is either not initialized or all subsets have already been enumerated.
	bool valid() const
	{
		return m_valid;
	}

	//! Checks in O(subset cardinality) whether \p element is a member of the subset.
	bool hasMember(const T &element) const
	{
		for (int index : m_index) {
			if (element == m_subset[index]) {
				return true;
			}
		}
		return false;
	}

	//! Gets a member of subset by index (starting from 0).
	T operator[](int i) const
	{
		OGDF_ASSERT(i >= 0);
		OGDF_ASSERT(i < m_index.size());
		return m_subset[m_index[i]];
	}

	//! Obtains the next subset if possible. The result should be checked using the #valid() method.
	void next()
	{
		if (m_valid) {
			const int t = m_index.size();
			if (t == 0) { // last (empty) subset has been found
				if (t < m_maxCard) {
					initSubset(t + 1);
				} else {
					m_valid = false;
				}
				return;
			}
			const int n = m_subset.size();
			int i;
			for (i = t - 1; m_index[i] == i + n - t; --i) {
				if (i == 0) { // the last subset of this cardinality has been found
					if (t < m_maxCard) {
						initSubset(t + 1);
					} else {
						m_valid = false;
					}
					return;
				}
			}
			for (++m_index[i]; i < t - 1; ++i) {
				m_index[i + 1] = m_index[i] + 1;
			}
		}
	}

	//! Calls \p func for each member in the subset.
	void forEachMember(std::function<void(const T &)> func) const
	{
		for (int index : m_index) {
			func(m_subset[index]);
		}
	}

	//! Obtains (appends) a list of the subset members.
	void list(List<T> &subset) const
	{
		forEachMember([&](const T &member) {
			subset.pushBack(member);
		});
	}

	//! Obtains an array of the subset members.
	void array(Array<T> &array) const
	{
		array.init(m_index.size());
		for (int i = 0; i < m_index.size(); ++i) {
			array[i] = m_subset[m_index[i]];
		}
	}

	//! Calls \p funcIn for each subset member and \p funcNotIn for each other element of the set.
	void forEachMemberAndNonmember(std::function<void(const T &)> funcIn, std::function<void(const T &)> funcNotIn) const
	{
		for (int i = 0, j = 0; i < m_subset.size(); ++i) {
			if (j < m_index.size() && m_index[j] == i) {
				funcIn(m_subset[i]);
				++j;
			} else {
				funcNotIn(m_subset[i]);
			}
		}
	}

	//! Obtains a container of the subset members and a container of the other elements of the set.
	template<typename ContainerType>
	void getSubsetAndComplement(ContainerType &subset, ContainerType &complement, std::function<void(ContainerType &, T)> func) const
	{
		forEachMemberAndNonmember([&](const T &member) {
			func(subset, member);
		}, [&](const T &nonmember) {
			func(complement, nonmember);
		});
	}

	//! Obtains (appends) a list of the subset members and a list of the other elements of the set.
	void list(List<T> &subset, List<T> &complement) const
	{
		getSubsetAndComplement<List<T>>(subset, complement, [](List<T> &lc, T element) {
			lc.pushBack(element);
		});
	}

	//! Tests \p predicate for all subset members.
	bool testForAll(std::function<bool(const T&)> predicate) const
	{
		for (int index : m_index) {
			if (!predicate(m_subset[index])) {
				return false;
			}
		}
		return true;
	}

	//! Prints subset to output stream \p os using delimiter \p delim
	void print(std::ostream &os, string delim = " ") const
	{
		if (valid()) {
			if (size() > 0) {
				os << m_subset[m_index[0]];
				for (int i = 1; i < size(); ++i) {
					os << delim << m_subset[m_index[i]];
				}
			}
		} else {
			os << "<<invalid subset>>";
		}
	}
};

template<class T>
std::ostream &operator<<(std::ostream &os, const SubsetEnumerator<T> &subset)
{
	subset.print(os);
	return os;
}

}
