/** \file
 * \brief Declaration of class LinearQuadtreeExpansion.
 *
 * \author Martin Gronemann
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

#include <ogdf/energybased/fast_multipole_embedder/LinearQuadtree.h>

namespace ogdf {
namespace fast_multipole_embedder {

class LinearQuadtreeExpansion
{
public:
	//! constructor
	LinearQuadtreeExpansion(uint32_t precision, const LinearQuadtree& tree);

	//! destructor
	~LinearQuadtreeExpansion(void);

	//! adds a point with the given charge to the receiver expansion
	void P2M(uint32_t point, uint32_t receiver);

	//! shifts the source multipole coefficient to the center of the receiver and adds them
	void M2M(uint32_t source, uint32_t receiver);

	//! converts the source multipole coefficient in to a local coefficients at the center of the receiver and adds them
	void M2L(uint32_t source, uint32_t receiver);

	//! shifts the source local coefficient to the center of the receiver and adds them
	void L2L(uint32_t source, uint32_t receiver);

	//! evaluates the derivate of the local expansion at the point and adds the forces to fx fy
	void L2P(uint32_t source, uint32_t point, float& fx, float& fy);

	//! returns the size in bytes
	uint32_t sizeInBytes() const { return m_numExp*m_numCoeff*(uint32_t)sizeof(double)*4; }

	//! returns the array with multipole coefficients
	inline double* multiExp() const { return m_multiExp; }

	//! returns the array with local coefficients
	inline double* localExp() const { return m_localExp; }

	//! number of coefficients per expansions
	inline uint32_t numCoeff() const { return m_numCoeff; }

	//! the quadtree
	const LinearQuadtree& tree() { return m_tree; }
private:

	//! allocates the space for the coeffs
	void allocate();

	//! releases the memory for the coeffs
	void deallocate();

	//! the Quadtree reference
	const LinearQuadtree& m_tree;
public:
	//! the big multipole expansione coeff array
	double* m_multiExp;

	//! the big local expansion coeff array
	double* m_localExp;

public:
	//! the number of multipole (locale) expansions
	uint32_t m_numExp;

	//! the number of coeff per expansions
	uint32_t m_numCoeff;

	BinCoeff<double> binCoef;
};

}
}
