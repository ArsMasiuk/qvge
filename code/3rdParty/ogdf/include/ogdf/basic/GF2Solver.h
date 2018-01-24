/** \file
 * \brief Defines class GF2Solver, which represents a solver for
 *        linear equation systems over GF(2).
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

#include <ogdf/basic/List.h>
#include <iomanip>


namespace ogdf {

class GF2Solver {

	static constexpr int chunkSize = 13;
	static constexpr int chunkSize2 = 9;

	template<int Dim, typename Next>
	struct ChunkBase {
		int m_x[Dim];
		int m_max;
		Next* m_next;

		bool full() const {
			return m_max == Dim - 1;
		}

		ChunkBase() {
			m_max = -1;
			m_next = nullptr;
			for (int i = 0; i < Dim; i++) {
				m_x[i] = 0;
			}
		}

		OGDF_NEW_DELETE
	};

	struct Chunk : public ChunkBase<chunkSize, Chunk> {
		Chunk() : ChunkBase<chunkSize, Chunk>() {}

		void add(int x) {
			m_x[++m_max] = x;
		}

		OGDF_NEW_DELETE
	};

	struct Chunk2 : public ChunkBase<chunkSize2, Chunk2> {
		ListIterator<int> m_it[chunkSize2];

		Chunk2() : ChunkBase<chunkSize2, Chunk2>() { }

		void add(int x, ListIterator<int> it) {
			++m_max;
			m_x [m_max] = x;
			m_it[m_max] = it;
		}

		OGDF_NEW_DELETE
	};

#if 0
	struct Row {
		Chunk *m_pHead;
		Chunk *m_pTail;

		Row() {
			m_pHead = m_pTail = nullptr;
		}

		void addChunk(Chunk *p) {
			if(m_pHead == nullptr)
				m_pHead = m_pTail = p;
			else {
				m_pTail->m_next = p;
				m_pTail = p;
			}
		}
	};
#endif

	struct Row2 {
		Chunk2 *m_pHead;
		Chunk2 *m_pTail;

		Row2() {
			m_pHead = m_pTail = nullptr;
		}

		void addChunk(Chunk2 *p) {
			if(m_pHead == nullptr)
				m_pHead = m_pTail = p;
			else {
				m_pTail->m_next = p;
				m_pTail = p;
			}
		}
	};

	Chunk *m_freelist;
	Chunk2 *m_freelist2;

#if 0
	Chunk *getChunk() {
		if(m_freelist != nullptr) {
			Chunk *p = m_freelist;
			m_freelist = p->m_next;
			p->m_next = nullptr;
			p->m_max = -1;
			return p;
		}
		return new Chunk;
	}
#endif

	Chunk2 *getChunk2() {
		if(m_freelist2 != nullptr) {
			Chunk2 *p = m_freelist2;
			m_freelist2 = p->m_next;
			p->m_next = nullptr;
			p->m_max = -1;
			return p;
		}
		return new Chunk2;
	}

#if 0
	void freeChunk(Chunk *p) {
		p->m_next = m_freelist;
		m_freelist = p;
	}
#endif

	void freeChunk2(Chunk2 *p) {
		p->m_next = m_freelist2;
		m_freelist2 = p;
	}

#if 0
	void freeChunks(Chunk *pHead, Chunk *pTail) {
		pTail->m_next = m_freelist;
		m_freelist = pHead;
	}
#endif

	void freeChunks2(Chunk2 *pHead, Chunk2 *pTail) {
		pTail->m_next = m_freelist2;
		m_freelist2 = pHead;
	}

#if 0
	bool contains(const Row &r, int x) const;

	void symDiff(Row &r, const Row &other);
#endif
	void symDiff2(int r1, int r2, Array<Row2> &rows, Array<List<int>> &cols);

public:

	class Equation {

		List<int> m_objects;

	public:
		Equation() { }

		void print() {
			std::cout << m_objects << std::endl;
		}

		ListConstIterator<int> begin() const { return m_objects.begin(); }
		ListConstIterator<int> end() const { return m_objects.end(); }

#if 0
		bool contains(OBJ obj) const {
			for(OBJ x : m_objects) {
				if(x == obj)
					return true;
				else if(x > obj)
					return false;
			}
			return false;
		}
#endif

		int size() const {
			return m_objects.size();
		}

		Equation &operator|=(int obj) {
			ListIterator<int> it = m_objects.begin();
			while(it.valid() && *it < obj)
				++it;
			if(it.valid()) {
				if(*it != obj)
					m_objects.insertBefore(obj,it);
			} else
				m_objects.pushBack(obj);

			return *this;
		}

#if 0
		Equation &operator^=(const Equation &other) {
			ListConstIterator<OBJ> itOther = other.m_objects.begin();
			ListIterator<OBJ> it = m_objects.begin();

			while(itOther.valid())
			{
				if(!it.valid()) {
					m_objects.pushBack(*itOther);
					++itOther;

				} else if(*it == *itOther) {
					ListIterator<OBJ> itDel = it;
					++it; ++itOther;
					m_objects.del(itDel);

				} else if(*itOther < *it) {
					m_objects.insertBefore(*itOther, it);
					++itOther;

				} else {
					++it;
				}
			}

			return *this;
		}
#endif

		OGDF_NEW_DELETE
	};

	class Matrix {

		Array<Equation*> m_equations;
		int m_numRows;
		int m_numCols;

	public:
		Matrix() : m_equations(0, 255, nullptr), m_numRows(0), m_numCols(0) { }

		~Matrix() {
			for(int i = 0; i < m_numRows; ++i)
				delete m_equations[i];
		}

		Equation &operator[](int i) {
			OGDF_ASSERT(i >= 0);
			OGDF_ASSERT(i < m_numRows);
			return *m_equations[i];
		}

		const Equation &operator[](int i) const {
			OGDF_ASSERT(i >= 0);
			OGDF_ASSERT(i < m_numRows);
			return *m_equations[i];
		}

		int numRows() const { return m_numRows; }
		int numColumns() const { return m_numCols; }

		int addRow() {
			int i = m_numRows++;
			if(i == m_equations.size())
				m_equations.grow(m_equations.size(), nullptr);
			m_equations[i] = new Equation;

			return i;
		}

		int addColumn() {
			return m_numCols++;
		}

		void clear() {
			for(int i = 0; i < m_numRows; ++i)
				delete m_equations[i];

			m_equations.init(0, 255, nullptr);
			m_numRows = m_numCols = 0;
		}

		void print() const {
			for(int i = 0; i < m_numRows; ++i) {
				std::cout << std::setw(4) << i << ": ";
				m_equations[i]->print();
			}
		}
	};


	explicit GF2Solver(GF2Solver::Matrix &Mx)
	  : m_freelist(nullptr)
	  , m_freelist2(nullptr)
	  , m_matrix(Mx)
	{
	}

	~GF2Solver();

	bool solve();
	bool solve2();


private:
	Matrix &m_matrix;
};

}
