/** \file
 * \brief Implements class GF2Solver, which represents a solver for
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

#include <ogdf/basic/GF2Solver.h>

namespace ogdf {

GF2Solver::~GF2Solver()
{
#if 0
	for(Chunk *p = m_freelist; p != nullptr; ) {
		Chunk *pNext = p->m_next;
		delete p;
		p = pNext;
	}
#endif

	for(Chunk2 *p = m_freelist2; p != nullptr; ) {
		Chunk2 *pNext = p->m_next;
		delete p;
		p = pNext;
	}
}

#if 0
bool GF2Solver::solve()
{
	const int n = m_matrix.numRows();
	const int maxCol = m_matrix.numColumns()-1;

	Array<Row> rows(n);
	for(int i = 0; i < n; ++i) {
		Row &r = rows[i];
		r.addChunk(getChunk());
		for(int x : m_matrix[i]) {
			if(r.m_pTail->full())
				r.addChunk(getChunk());
			r.m_pTail->add(x);
		}
	}

	Array<bool> diagonal(0, n, false);

#if 0
	int count = 0, R;
	for(int r = 0; r < n; ++r) {
		if(contains(rows[r],maxCol)) {
			count++; R = r;
		}
	}
	std::cout << "Rows with maxCol: " << count << std::endl;

	{
		int nReqRows = 0, nReqVars = 0;

		Array<bool> reqVar(0, maxCol, false);
		Array<bool> reqRow(0, n-1, false);

		Queue<int> Q;
		Q.append(R);

		while(!Q.empty()) {
			int r = Q.pop();

			if(reqRow[r])
				continue;

			reqRow[r] = true;
			nReqRows++;

			const Equation &eq = m_matrix[r];
			for(int c : eq)
				if(!reqVar[c]) {
					reqVar[c] = true;
					nReqVars++;
					for(int i = 0; i < n; ++i)
						if(!reqRow[i] && contains(rows[i], c))
							Q.append(i);
				}
		}

		std::cout << "required: " << nReqRows << " rows, " << nReqVars << " vars" << std::endl;
	}
#endif

	for(int c = 0; c < maxCol; ++c)
	{
		bool foundPivot = false;
		int pivot = -1;
		for(int r = 0; r < n; ++r) {
			if(diagonal[r]) continue;

			if(contains(rows[r], c)) {
				foundPivot = true;
				pivot = r;
				break;
			}
		}

		if(foundPivot) {
			for(int r = 0; r < n; ++r) {
				if(r != pivot && contains(rows[r],c))
					symDiff(rows[r], rows[pivot]);
			}
			diagonal[pivot] = true;
		}
	}

#if 0
	count = 0;
#endif
	bool result = true;
	for(int r = 0; r < n; ++r) {
#if 0
		if(contains(rows[r],maxCol))
			count++;
#endif

		if(diagonal[r]) {
			continue;
		}

		if(contains(rows[r],maxCol)) {
#if 0
			std::cout << "Rows with maxCol: " << count << std::endl;
#endif
			result = false;
			break;
		}
	}
#if 0
	std::cout << "Rows with maxCol: " << count << std::endl;
#endif

	for(int i = 0; i < n; ++i) {
		Row &r = rows[i];
		if(r.m_pHead != nullptr)
			freeChunks(r.m_pHead,r.m_pTail);
	}

	return result;
}
#endif

bool GF2Solver::solve2()
{
	const int n = m_matrix.numRows();
	const int m = m_matrix.numColumns();
	const int maxCol = m-1;

	Array<Row2> rows(n);
	Array<List<int>> cols(m);

	for(int i = 0; i < n; ++i) {
		Row2 &r = rows[i];
		r.addChunk(getChunk2());
		for(int x : m_matrix[i]) {
			if(r.m_pTail->full())
				r.addChunk(getChunk2());
			r.m_pTail->add(x, cols[x].pushBack(i));
		}
	}

	Array<bool> diagonal(0, n, false);

	for(int c = 0; c < maxCol; ++c)
	{
		bool foundPivot = false;
		int pivot = -1;
		for(int r : cols[c]) {
			if(diagonal[r]) continue;

			foundPivot = true;
			pivot = r;
			break;
		}

		if(foundPivot) {
			ListIterator<int> it, itSucc;
			for(it = cols[c].begin(); it.valid(); it = itSucc) {
				itSucc = it.succ();

				if(*it != pivot)
					symDiff2(*it, pivot, rows, cols);
			}

			diagonal[pivot] = true;
		}
	}

	bool result = true;
	for(int r : cols[maxCol]) {
		if(!diagonal[r]) {
			result = false;
			break;
		}
	}

	for(int i = 0; i < n; ++i) {
		Row2 &r = rows[i];
		if(r.m_pHead != nullptr)
			freeChunks2(r.m_pHead,r.m_pTail);
	}

	return result;
}

#if 0
bool GF2Solver::contains(const Row &r, int x) const
{
	for(Chunk *p = r.m_pHead; p != nullptr; p = p->m_next)
		if(x <= p->m_x[p->m_max]) {
			int i = 0;
			while(p->m_x[i] < x)
				++i;
			return (p->m_x[i] == x);
		}
	return false;
}


void GF2Solver::symDiff(Row &r, const Row &other)
{
	Chunk *p1 = r.m_pHead;
	const Chunk *p2 = other.m_pHead;
	int i1 = 0, i2 = 0;

	Chunk *pHead = getChunk();
	Chunk *p = pHead;

	while( p1 != nullptr || p2 != nullptr )
	{
		if(p1 != nullptr && p2 != nullptr && p1->m_x[i1] == p2->m_x[i2]) {
			if(++i1 > p1->m_max) { i1 = 0; p1 = p1->m_next; }
			if(++i2 > p2->m_max) { i2 = 0; p2 = p2->m_next; }

		} else {
			if(p->full())
				p = p->m_next = getChunk();

			if(p2 == nullptr || (p1 != nullptr && p1->m_x[i1] < p2->m_x[i2])) {
				p->add(p1->m_x[i1]);
				if(++i1 > p1->m_max) { i1 = 0; p1 = p1->m_next; }

			} else {
				p->add(p2->m_x[i2]);
				if(++i2 > p2->m_max) { i2 = 0; p2 = p2->m_next; }
			}
		}
	}

	freeChunks(r.m_pHead,r.m_pTail);

	if(pHead == p && p->m_max == -1) {
		freeChunk(pHead);
		r.m_pHead = r.m_pTail = nullptr;
	} else {
		r.m_pHead = pHead;
		r.m_pTail = p;
	}
}
#endif


void GF2Solver::symDiff2(int r1, int r2, Array<Row2> &rows, Array<List<int>> &cols)
{
	Row2 &row = rows[r1];

	Chunk2 *p1 = row.m_pHead;
	const Chunk2 *p2 = rows[r2].m_pHead;
	int i1 = 0, i2 = 0;

	Chunk2 *pHead = getChunk2();
	Chunk2 *p = pHead;

	while( p1 != nullptr || p2 != nullptr )
	{
		if(p1 != nullptr && p2 != nullptr && p1->m_x[i1] == p2->m_x[i2]) {
			cols[p1->m_x[i1]].del(p1->m_it[i1]);

			if(++i1 > p1->m_max) { i1 = 0; p1 = p1->m_next; }
			if(++i2 > p2->m_max) { i2 = 0; p2 = p2->m_next; }

		} else {
			if(p->full())
				p = p->m_next = getChunk2();

			if(p2 == nullptr || (p1 != nullptr && p1->m_x[i1] < p2->m_x[i2])) {
				p->add(p1->m_x[i1], p1->m_it[i1]);
				if(++i1 > p1->m_max) { i1 = 0; p1 = p1->m_next; }

			} else {
				p->add(p2->m_x[i2], cols[p2->m_x[i2]].pushBack(r1));
				if(++i2 > p2->m_max) { i2 = 0; p2 = p2->m_next; }
			}
		}
	}

	freeChunks2(row.m_pHead, row.m_pTail);

	if(pHead == p && p->m_max == -1) {
		freeChunk2(pHead);
		row.m_pHead = row.m_pTail = nullptr;
	} else {
		row.m_pHead = pHead;
		row.m_pTail = p;
	}
}

}
