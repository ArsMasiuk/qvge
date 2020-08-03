/** \file
 * \brief Declaration of interface for two-layer crossing
 *        minimization algorithms.
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

#include <ogdf/layered/Hierarchy.h>
#include <ogdf/layered/HierarchyLevels.h>
#include <ogdf/layered/LayeredCrossMinModule.h>

namespace ogdf {

class SugiyamaLayout;

/**
 * \brief Interface of two-layer crossing minimization algorithms.
 *
 * @ingroup gd-layered-crossmin
 *
 * The interface of a two-layer crossing minimization algorithm consists of
 * four methods:
 *   -# init(const Hierarchy & H) must be called first. This initializes the module
 *      for operating on hierarchy \a H.
 *   -# call(Level &L) (or operator()(Level &L)) performs two-layer crossing minimization,
 *      where \a L is the permutable level and the neighbor level of \a L (fixed
 *      level) is determined by the hierarchy (see documentation of class Hierarchy).
 *      Any number of call's may be performed once init() has been executed.
 *   -# cleanup() has to be called last and performs some final clean-up work.
 */
class OGDF_EXPORT LayerByLayerSweep : public LayeredCrossMinModule {
public:

	virtual const HierarchyLevels *reduceCrossings(const SugiyamaLayout &sugi, const Hierarchy &H, int &nCrossings);

	//! Template method implementation of reduceCrossings from LayeredCrossMinModule.
	virtual const HierarchyLevels *reduceCrossings(const SugiyamaLayout &sugi, Hierarchy &H, int &nCrossings) override
	{
		const Hierarchy &constH = H;
		return reduceCrossings( sugi, constH, nCrossings );
	}

	//! Initializes a two-layer crossing minimization module.
	LayerByLayerSweep() { }

	virtual ~LayerByLayerSweep() { }

	//! Returns a new instance of the two-layer crossing minimization module with the same option settings.
	virtual LayerByLayerSweep *clone() const = 0;

	/**
	 * \brief Initializes the crossing minimization module for hierarchy levels \p levels.
	 *
	 * @param levels is the hierarchy on which the module shall operate.
	 */
	virtual void init(const HierarchyLevels &levels) { }

	/**
	 * \brief Performs crossing minimization for level \p L.
	 *
	 * @param L is the level in the hierarchy on which nodes are permuted; the
	 *        neighbor level (fixed level) is determined by the hierarchy.
	 */
	virtual void call(Level &L) = 0;

	/**
	 * \brief Performs crossing minimization for level \p L.
	 *
	 * @param L is the level in the hierarchy on which nodes are permuted; the
	 *        neighbor level (fixed level) is determined by the hierarchy.
	 */
	void operator()(Level &L) {
		call(L);
	}

	//! Performs clean-up.
	virtual void cleanup() override { }


	class CrossMinMaster;
	class CrossMinWorker;

	OGDF_MALLOC_NEW_DELETE
};

}
