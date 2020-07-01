/** \file
 * \brief This class is a simple layout that places nodes next to each other
 * and draws edges as bows above the nodes.
 * The user may decide whether to use a custom permutation or use the ordering
 * given by the nodes indices.
 *
 * \author Sebastian Semper
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

#include <ogdf/basic/basic.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/basic/LayoutModule.h>
#include <cmath>
#include <math.h>
#include <ogdf/basic/Math.h>

namespace ogdf {

/**
 * %Layout the graph with nodes next to each other with natural or custom
 * order and draw the edges as semicircular bows above them.
 */
class OGDF_EXPORT LinearLayout : public LayoutModule {
private:
	//! If true a custom order stored in #m_nodeOrder will be used
	bool m_customOrder;

	//! Contains a custom ordering for putting the graphs next to each other
	ListPure<node> m_nodeOrder;
	double m_outWidth;
public:

	/**
	 * Constructor that takes a desired width and a custom ordering
	 *
	 * @param w Width of the output
	 * @param o custom order
	 */
	LinearLayout(
		double w,
		ListPure<node> o
	);

	//! Constructor that uses a standard width and no custom order of the nodes
	LinearLayout();

	//! Standard destructor
	virtual ~LinearLayout();

	virtual void call(GraphAttributes& GA) override;

	//! Interface function to toggle custom ordering
	virtual void setCustomOrder(bool o);
};

}
