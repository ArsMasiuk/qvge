/** \file
 * \brief Declaration of a Mixed-Model crossings beautifier
 * that uses grid doubling.
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

#include <ogdf/planarlayout/MMCBBase.h>


namespace ogdf {


/**
 * \brief Crossings beautifier using grid doubling.
 */
class OGDF_EXPORT MMCBDoubleGrid : public MMCBBase
{
public:
	//! Creates an instance of the crossings beautifier module.
	MMCBDoubleGrid() { }

	~MMCBDoubleGrid() { }

protected:
	//! Implements the module call.
	virtual void doCall(const PlanRep &PG, GridLayout &gl, const List<node> &L) override;
};

}
