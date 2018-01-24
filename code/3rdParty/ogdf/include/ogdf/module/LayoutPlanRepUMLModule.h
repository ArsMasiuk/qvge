/** \file
 * \brief Declaration of interface for planar layout algorithms for
 *        UML diagrams (used in planarization approach).
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

#include <ogdf/uml/PlanRepUML.h>
#include <ogdf/basic/Layout.h>



namespace ogdf {


/**
 * \brief Interface for planar UML layout algorithms.
 *
 * \see PlanarizationLayout
 */
class OGDF_EXPORT LayoutPlanRepUMLModule {
public:
	//! Initializes a UML planar layout module.
	LayoutPlanRepUMLModule() { }

	virtual ~LayoutPlanRepUMLModule() { }

	/** \brief Computes a planar layout of \p PG in \p drawing.
	 *
	 * Must be overridden by derived classes.
	 * @param PG is the input planarized representation which may be modified.
	 * @param adjExternal is an adjacenty entry on the external face.
	 * @param drawing is the computed layout of \p PG.
	 */
	virtual void call(PlanRepUML &PG,
		adjEntry adjExternal,
		Layout &drawing) = 0;

	//! Computes a planar layout of \p PG in \p drawing.
	void operator()(PlanRepUML &PG, adjEntry adjExternal, Layout &drawing) {
		call(PG,adjExternal,drawing);
	}

	//! Returns the bounding box of the computed layout.
	const DPoint &getBoundingBox() const {
		return m_boundingBox;
	}

	//! Sets the (generic) options; derived classes have to cope with the interpretation)
	virtual void setOptions(int /* optionField */) { }
	// don't make it abstract!

	//! Returns the (generic) options.
	virtual int getOptions() { return 0; }
	// don't make it abstract!

	//! Returns the minimal allowed distance between edges and vertices.
	virtual double separation() const = 0;

	//! Sets the minimal allowed distance between edges and vertices to \p sep.
	virtual void separation(double sep) = 0;

protected:
	/**
	 * \brief Stores the bounding box of the computed layout.
	 * <b>Must be set by derived algorithms!</b>
	 */
	DPoint m_boundingBox;

	/**
	 * \brief Computes and sets the bounding box variable #m_boundingBox.
	 * An algorithm can call setBoundingBox() for setting the
	 * m_boundingBox variable if no faster implementation is available.
	 */
	void setBoundingBox(PlanRep &PG, Layout &drawing) {
		m_boundingBox = drawing.computeBoundingBox(PG);
	}

	OGDF_MALLOC_NEW_DELETE
};

}
