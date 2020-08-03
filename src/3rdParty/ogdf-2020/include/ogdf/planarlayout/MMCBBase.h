/** \file
 * \brief Declaration of base class for certain Mixed-Model
 * crossings beautifier.
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

#include <ogdf/planarlayout/MixedModelCrossingsBeautifierModule.h>

namespace ogdf {

/**
 * \brief common base class for MMCBDoubleGrid and MMCBLocalStretch.
 *
 * MMCBBase contains the common functionality shared by the two Mixed-Model
 * crossings beautifier MMCBDoubleGrid and MMCBLocalStretch.
 */
class OGDF_EXPORT MMCBBase : public MixedModelCrossingsBeautifierModule
{
public:
	//! Constructor (does nothing).
	MMCBBase() { }

	~MMCBBase() { }

protected:
	static void insertBend(GridLayout &gl, edge e, node v, int x, int y);
	static void copyOn(int old_a[] , int new_a[]);
	static int workOn(GridLayout &gl, node v);
};

}
