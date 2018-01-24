/** \file
 * \brief Declaration of interface for layout algorithms (class
 *        LayoutModule)
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

#include <ogdf/basic/GraphAttributes.h>

namespace ogdf {


/**
 * \brief Interface of general layout algorithms.
 *
 */
class OGDF_EXPORT LayoutModule {
public:
	//! Initializes a layout module.
	LayoutModule() { }

	virtual ~LayoutModule() { }

	/**
	 * \brief Computes a layout of graph \p GA.
	 *
	 * This method is the actual algorithm call and must be implemented by
	 * derived classes.
	 * @param GA is the input graph and will also be assigned the layout information.
	 */
	virtual void call(GraphAttributes &GA) = 0;

	/**
	 * \brief Computes a layout of graph \p GA.
	 *
	 * @param GA is the input graph and will also be assigned the layout information.
	 */
	void operator()(GraphAttributes &GA) { call(GA); }

	OGDF_MALLOC_NEW_DELETE
};

}
