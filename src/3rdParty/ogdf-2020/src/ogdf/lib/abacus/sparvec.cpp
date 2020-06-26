/*!\file
 * \author Matthias Elf
 *
 * \par License:
 * This file is part of ABACUS - A Branch And CUt System
 * Copyright (C) 1995 - 2003
 * University of Cologne, Germany
 *
 * \par
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * \par
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * \see http://www.gnu.org/copyleft/gpl.html
 */

#include <ogdf/lib/abacus/sparvec.h>

namespace abacus {


SparVec::SparVec(
	AbacusGlobal *glob,
	int size,
	double reallocFac)
	:
glob_(glob),
	size_(size),
	nnz_(0),
	reallocFac_(reallocFac)
{
	if (size) {
		support_ = new int[size];
		coeff_   = new double[size];
	}
	else {
		support_ = nullptr;
		coeff_   = nullptr;
	}
}


SparVec::SparVec(
	AbacusGlobal *glob,
	int size,
	const Array<int> &s,
	const Array<double> &c,
	double reallocFac)
	:
glob_(glob),
	size_(size),
	reallocFac_(reallocFac)
{
	if (size ) {
		support_ = new int[size];
		coeff_   = new double[size];
	}
	else {
		support_ = nullptr;
		coeff_   = nullptr;
	}

	nnz_ = size < s.size() ? size : s.size();

	for(int i = 0; i < nnz_; i++) {
		support_[i] = s[i];
		coeff_[i]   = c[i];
	}
}


SparVec::SparVec(
	AbacusGlobal *glob,
	int nnz,
	int *s,
	double *c,
	double reallocFac)
	:
glob_(glob),
	size_(nnz),
	nnz_(nnz),
	reallocFac_(reallocFac)
{
	if (nnz) {
		support_ = new int[nnz];
		coeff_   = new double[nnz];
	}
	else {
		support_ = nullptr;
		coeff_   = nullptr;
	}

	for(int i = 0; i < nnz; i++) {
		support_[i] = s[i];
		coeff_[i]   = c[i];
	}
}


SparVec::SparVec(const SparVec& rhs)
	:
	glob_(rhs.glob_),
	size_(rhs.size_),
	nnz_(rhs.nnz_),
	reallocFac_(rhs.reallocFac_)
{
	if (size_) {
		support_ = new int[size_];
		coeff_   = new double[size_];

		for(int i = 0; i < nnz_; i++) {
			support_[i] = rhs.support_[i];
			coeff_[i]   = rhs.coeff_[i];
		}
	}
	else {
		support_ = nullptr;
		coeff_   = nullptr;
	}
}


SparVec::~SparVec()
{
	delete [] support_;
	delete [] coeff_;
}


SparVec& SparVec::operator=(const SparVec& rhs)
{
	if (this == &rhs)
		return *this;

	if(size_ != rhs.size_) {
		Logger::ilout() << "SparVec::operator= : length of operands are different (" << size_ << " != " << rhs.size_ << " ).\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::SparVec);
	}

	for(int i = 0; i < rhs.nnz_; i++) {
		support_[i] = rhs.support_[i];
		coeff_[i]   = rhs.coeff_[i];
	}

	nnz_ = rhs.nnz_;
	glob_ = rhs.glob_;
	return *this;
}


std::ostream& operator<<(std::ostream &out, const SparVec &rhs)
{
	for (int i = 0; i < rhs.nnz_; i++)
		out << rhs.support_[i] << " " << rhs.coeff_[i] << std::endl;
	return out;
}


double SparVec::origCoeff(int i) const
{
#ifdef TEST_SPARVEC_ORDER
	{ int k;
	for (k = 1; k < nnz_; k++)
		if (support_[k-1]>support_[k])
			break;
	if (k==nnz_)
		std::cout << "SparVec::origCoeff vector is in order.\n";
	else
		std::cout << "SparVec::origCoeff vector is not in order.\n";
	}
#endif
	for (int k = 0; k < nnz_; k++)
		if (support_[k] == i) return coeff_[k];
	return 0.0;
}


void SparVec::leftShift(ArrayBuffer<int> &del)
{
	const int nDel = del.size();

	if (nDel == 0) return;

	int i,j;
	int current = del[0];

	// shift all elements left of the last deleted element to the left side
	/* All elements in the arrays between the removed elements \a del[i] and
	*   del[i+1] are shifted left in the inner loop.
	*/
	for (i = 0; i < nDel - 1; i++) {

#ifdef ABACUSSAFE
		if(del[i] < 0 || del[i] >= nnz_) {
			Logger::ilout() << "ArrayBuffer:leftShift(): shift index " << i << " not valid.\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::afcSparVec);
		}
#endif

		const int last = del[i+1];

		for(j = del[i]+1; j < last; j++) {
			support_[current] = support_[j];
			coeff_[current]   = coeff_[j];
			++current;
		}
	}

	// shift all elements right of the last deleted element to the left side
#ifdef ABACUSSAFE
	if(del[nDel -1] < 0 || del[nDel - 1] >= nnz_) {
		Logger::ilout() << "ArrayBuffer:leftShift(): shift index " << i << " not valid.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::afcSparVec);
	}
#endif

	for (j = del[nDel - 1] + 1; j < nnz_; j++) {
		support_[current] = support_[j];
		coeff_[current]   = coeff_[j];
		++current;
	}


	nnz_ -= nDel;
}


void SparVec::copy(const SparVec &vec)
{
	if(size_ < vec.nnz())
		realloc(vec.nnz());

	nnz_ = vec.nnz();

	for(int i = 0; i < nnz_; i++) {
		support_[i] = vec.support(i);
		coeff_[i]   = vec.coeff(i);
	}
}


void SparVec::rename(Array<int> &newName)
{
	for(int i = 0; i < nnz_; i++)
		support_[i] = newName[support_[i]];
}


double SparVec::norm()
{
	double v = 0.0;

	for (int i = 0; i < nnz_; i++)
		v += coeff_[i]*coeff_[i];

	return sqrt(v);
}


void SparVec::realloc()
{
	realloc((int) ((1.0 + reallocFac_/100.0) * size_) + 1);
}


void SparVec::realloc(int newSize)
{
	if (newSize < nnz_) {
		Logger::ilout() << "SparVec::realloc(" << newSize << "):\nlength of vector becomes less than number of nonzeros " << nnz_ << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::SparVec);
	}

	int    *newSupport = new int[newSize];
	double *newCoeff    = new double[newSize];

	for (int i = 0; i < nnz_; i++) {
		newSupport[i] = support_[i];
		newCoeff[i]   = coeff_[i];
	}

	delete [] support_;
	delete [] coeff_;

	support_ = newSupport;
	coeff_   = newCoeff;
	size_    = newSize;
}


void SparVec::rangeCheck(int i) const
{
	if (i < 0 || i >= nnz_) {
		Logger::ilout() << "SparVec::rangeCheck(): index " << i << "\nout of ranges of nonzeros [0," << nnz_ << "-1 ].\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::SparVec);
	}
}
}
