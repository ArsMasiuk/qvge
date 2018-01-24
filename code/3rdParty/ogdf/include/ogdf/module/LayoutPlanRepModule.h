/** \file
 * \brief Declaration of interface for planar layout algorithms
 *        (used in planarization approach).
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

#include <ogdf/planarity/PlanRep.h>
#include <ogdf/basic/Layout.h>


namespace ogdf {


//! Interface for planar layout algorithms (used in the planarization approach).
/**
 * \see PlanarizationLayout
 */
class OGDF_EXPORT LayoutPlanRepModule {
public:
	//! Initializes a planar layout module.
	LayoutPlanRepModule() { }

	//! Destructor.
	virtual ~LayoutPlanRepModule() { }

	//! Computes a planar layout of \p PG in \p drawing.
	/**
	 * Must be overridden by derived classes. The implementation must also set
	 * m_boundingBox, which gives the bounding box of the computed layout.
	 *
	 * @param PG          is the input planarized representation which may be modified.
	 * @param adjExternal is an adjacenty entry on the external face.
	 * @param drawing     is the computed layout of \p PG.
	 */
	virtual void call(PlanRep &PG,
		adjEntry adjExternal,
		Layout &drawing) = 0;

	//! Computes a planar layout of \p PG in \p drawing.
	void operator()(PlanRep &PG, adjEntry adjExternal, Layout &drawing) {
		call(PG,adjExternal,drawing);
	}

	//! Returns the bounding box of the computed layout.
	const DPoint &getBoundingBox() const {
		return m_boundingBox;
	}

	//! Returns the minimal allowed distance between edges and vertices.
	virtual double separation() const = 0;

	//! Sets the minimal allowed distance between edges and vertices to \p sep.
	virtual void separation(double sep) = 0;

protected:
	//! Stores the bounding box of the computed layout.
	/**
	 * <b>Must be set by derived algorithms!</b>
	 */
	DPoint m_boundingBox;

	//! Computes and sets the bounding box variable #m_boundingBox.
	/**
	 * An algorithm can call setBoundingBox() for setting the
	 * m_boundingBox variable if no faster implementation is available.
	 */
	void setBoundingBox(PlanRep &PG, Layout &drawing) {
		m_boundingBox = drawing.computeBoundingBox(PG);
	}

	OGDF_MALLOC_NEW_DELETE
};

}
