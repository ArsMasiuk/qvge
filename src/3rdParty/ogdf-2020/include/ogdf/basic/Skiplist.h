/** \file
 * \brief Declaration of class Skiplist.
 *
 * \author Markus Chimani
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
#include <ogdf/basic/memory.h>

namespace ogdf {

template<class X> class SkiplistIterator;

//! A randomized skiplist
/**
 * The elements height is computed using the traditional coin-flip method, using a
 * 50-50 chance to stop growing. The given running times of the methods below are therefore
 * only expected running times.
 *
 * \warning The code expects the type \a X to be a pointer! If \a X is not a pointer,
 * compiler errors will occur!
 */
template<class X> class Skiplist {
	friend class SkiplistIterator<X>;
	friend class Element;

	//! Internal structure to hold the items and internal forward pointers of the skiplist
	class Element {
		friend class Skiplist<X>;
		friend class SkiplistIterator<X>;

		X entry; // content
		Element** next; // successor elements

		// construction
		Element(const X &item, int height) :
		entry(item) {
			next = (Element**)malloc(height*sizeof(Element*));
		}

		~Element() {
			free(next);
		}

		OGDF_NEW_DELETE
	};

public:

	//! Construct an initially empty skiplist
	Skiplist() : m_lSize(0) {
		srand((unsigned int)time(nullptr));
		m_realheight = 5;
		m_height = 1;
		m_start = (Element**)malloc(m_realheight*sizeof(Element*));
		m_start[0] = nullptr;
	}

	~Skiplist() {
		clear();
		free(m_start);
	}

	//! Returns true if the item \p item is contained in the skiplist [O'(log n)]
	bool isElement(X item) const {
		int h = m_height - 1;
		Element** cur = m_start; // wheeha!
		while(true)	{
			if( cur[h] && *(cur[h]->entry) < *item ) //nxt != nullptr
				cur = cur[h]->next;
			else if(--h < 0)
				return cur[0] && *(cur[0]->entry) == *item;
		}
	}

	//! Adds the item \p item into the skiplist [O'(log n)]
	void add(X item) {
		m_lSize++;

		int nh = random_height();
		Element* n = new Element(item, nh);
		if(nh > m_height)
			grow(nh);

		int h = m_height - 1;
		Element** cur = m_start; // wheeha!
		while(true)	 {
			if( cur[h] && *(cur[h]->entry) < *item ) //nxt != nullptr
				cur = cur[h]->next;
			else {
				if(h < nh) { // add only if new element is high enough
					n->next[h] = cur[h];
					cur[h] = n;
				}
				if(--h < 0)
					return;
			}
		}
	}

	//! Returns the current size of the skiplist, i.e., the number of elements
	int size() const { return m_lSize; }

	//! Returns true if the skiplist contains no elements
	bool empty() const { return m_lSize == 0; }

	//! Clears the current skiplist
	/**
	* If \p killData is true, the items of the Skiplist (which are stored as
	* pointers) are automatically deleted.
	*/
	void clear(bool killData = false) {
		Element* item = m_start[0];
		while(item) {
			Element* old = item;
			item = item->next[0];
			if(killData)
				delete old->entry;
			delete old;
		}
		m_lSize = 0;
		m_height = 1;
		m_start[0] = nullptr;
	}

	//! returns an (forward) iterator for the skiplist
	const SkiplistIterator<X> begin() const { return m_start[0]; }

	//! returns an invalid iterator
	const SkiplistIterator<X> end() const { return nullptr; }

private:
	int m_lSize;
	Element** m_start;
	int m_height;
	int m_realheight;

	int random_height() {
		int h = 1;
		while(rand() > RAND_MAX/2) h++;
		return h;
	}

	void grow(int newheight) {
		if(newheight > m_realheight) {
			m_realheight = newheight;
			Element** newStart = static_cast<Element**>(realloc(m_start, m_realheight*sizeof(Element*)));
			if (newStart == nullptr) {
				free(m_start);
			} else {
				m_start = newStart;
			}
		}
		for(int i = newheight; i-- > m_height;) {
			m_start[i] = nullptr;
		}
		m_height = newheight;
	}

};

//! Forward-Iterator for Skiplists
template<class X> class SkiplistIterator {
	friend class Skiplist<X>;

	const typename Skiplist<X>::Element *el;

	SkiplistIterator(const typename Skiplist<X>::Element *e) { el = e; }

public:

	//! Returns the item to which the iterator points
	const X &operator*() const { return el->entry; }

	bool valid() const { return el != nullptr; }

	//! Move the iterator one item forward (prefix notation)
	SkiplistIterator<X> &operator++() {
		el = el->next[0];
		return *this;
	}
	//! Move the iterator one item forward (prefix notation)
	SkiplistIterator<X> operator++(int) {
		SkiplistIterator<X> it = *this;
		el = el->next[0];
		return it;
	}

	bool operator==(const SkiplistIterator<X> other) const {
		return el == other.el;
	}

	bool operator!=(const SkiplistIterator<X> other) const {
		return !operator==(other);
	}
};

}
