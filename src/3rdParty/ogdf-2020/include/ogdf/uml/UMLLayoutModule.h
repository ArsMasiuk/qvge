/** \file
 * \brief Declaration of interface for layout algorithms for
 *        UML diagrams.
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

#include <ogdf/uml/UMLGraph.h>


namespace ogdf {


/**
 * \brief Interface of UML layout algorithms.
 */
class OGDF_EXPORT UMLLayoutModule
{
public:
	//! Initializes a UML layout module.
	UMLLayoutModule() { }

	virtual ~UMLLayoutModule() { }

	/**
	 * \brief Computes a layout of UML graph \p umlGraph
	 *
	 * Must be implemented by derived classes.
	 * @param umlGraph is the input UML graph and has to be assigned the UML layout.
	 */
	virtual void call(UMLGraph &umlGraph) = 0;

	/**
	 * \brief Computes a layout of UML graph \p umlGraph
	 *
	 * @param umlGraph is the input UML graph and has to be assigned the UML layout.
	 */
	void operator()(UMLGraph &umlGraph) { call(umlGraph); }

	OGDF_MALLOC_NEW_DELETE
};

}
