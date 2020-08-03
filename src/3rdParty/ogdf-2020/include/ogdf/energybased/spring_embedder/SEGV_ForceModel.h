/** \file
 * \brief Declarations of force-models for Spring-Embedder algorithm
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

#include <ogdf/energybased/SpringEmbedderGridVariant.h>
#include <ogdf/energybased/spring_embedder/common.h>

namespace ogdf {

class SpringEmbedderGridVariant::ForceModelBase : public spring_embedder::CommonForceModelBase<NodeInfo>
{
public:
	ForceModelBase(const Array<NodeInfo> &vInfo, const Array<int> &adjLists, const Array2D<ListPure<int>> &gridCell, double idealEdgeLength)
	  : spring_embedder::CommonForceModelBase<NodeInfo>(vInfo, adjLists, idealEdgeLength)
	  , m_gridCell(gridCell)
	{
	}

	virtual ~ForceModelBase() { }

	virtual DPoint computeDisplacement(int j, double boxLength) const = 0;

protected:
	const Array2D<ListPure<int>> &m_gridCell;

	DPoint computeRepulsiveForce(int j, double boxLength, int idealExponent, int normExponent = 1) const;
	DPoint computeMixedForcesDisplacement(int j, int boxLength, std::function<DPoint(double, const DPoint &)> attractiveChange, std::function<double()> attractiveFinal) const;
};


class SpringEmbedderGridVariant::ForceModelFR : public ForceModelBase
{
public:
	ForceModelFR(const Array<NodeInfo> &vInfo, const Array<int> &adjLists, const Array2D<ListPure<int>> &gridCell, double idealEdgeLength)
		: ForceModelBase(vInfo, adjLists, gridCell, idealEdgeLength) { }

	DPoint computeDisplacement(int j, double boxLength) const override;
};

class SpringEmbedderGridVariant::ForceModelFRModAttr : public ForceModelBase
{
public:
	ForceModelFRModAttr(const Array<NodeInfo> &vInfo, const Array<int> &adjLists, const Array2D<ListPure<int>> &gridCell, double idealEdgeLength)
		: ForceModelBase(vInfo, adjLists, gridCell, idealEdgeLength) { }

	DPoint computeDisplacement(int j, double boxLength) const override;
};

class SpringEmbedderGridVariant::ForceModelFRModRep : public ForceModelBase
{
public:
	ForceModelFRModRep(const Array<NodeInfo> &vInfo, const Array<int> &adjLists, const Array2D<ListPure<int>> &gridCell, double idealEdgeLength)
		: ForceModelBase(vInfo, adjLists, gridCell, idealEdgeLength) { }

	DPoint computeDisplacement(int j, double boxLength) const override;
};


class SpringEmbedderGridVariant::ForceModelEades : public ForceModelBase
{
public:
	ForceModelEades(const Array<NodeInfo> &vInfo, const Array<int> &adjLists, const Array2D<ListPure<int>> &gridCell, double idealEdgeLength)
		: ForceModelBase(vInfo, adjLists, gridCell, idealEdgeLength) { }

	DPoint computeDisplacement(int j, double boxLength) const override;
};


class SpringEmbedderGridVariant::ForceModelHachul : public ForceModelBase
{
public:
	ForceModelHachul(const Array<NodeInfo> &vInfo, const Array<int> &adjLists, const Array2D<ListPure<int>> &gridCell, double idealEdgeLength)
		: ForceModelBase(vInfo, adjLists, gridCell, idealEdgeLength) { }

	DPoint computeDisplacement(int j, double boxLength) const override;
};


class SpringEmbedderGridVariant::ForceModelGronemann : public ForceModelBase
{
public:
	ForceModelGronemann(const Array<NodeInfo> &vInfo, const Array<int> &adjLists, const Array2D<ListPure<int>> &gridCell, double idealEdgeLength)
		: ForceModelBase(vInfo, adjLists, gridCell, idealEdgeLength) { }

	DPoint computeDisplacement(int j, double boxLength) const override;
};

}
