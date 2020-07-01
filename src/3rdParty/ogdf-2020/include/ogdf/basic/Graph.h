/** \file
 * \brief Includes declaration of graph class.
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

#include <ogdf/basic/List.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>

namespace ogdf {

//! Output operator for nodes; prints node index (or "nil").
OGDF_EXPORT std::ostream &operator<<(std::ostream &os, ogdf::node v);

//! Output operator for edges; prints source and target indices (or "nil").
OGDF_EXPORT std::ostream &operator<<(std::ostream &os, ogdf::edge e);

//! Output operator for adjacency entries; prints node and twin indices (or "nil").
OGDF_EXPORT std::ostream &operator<<(std::ostream &os, ogdf::adjEntry adj);

}
