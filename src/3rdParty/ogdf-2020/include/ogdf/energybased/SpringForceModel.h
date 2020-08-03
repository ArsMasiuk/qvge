/** \file
 * \brief Declaration of SpringForceModel enumeration.
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

#include <ogdf/basic/basic.h>

namespace ogdf {

//! The force model used for computing forces on nodes.
//! @ingroup gd-energy
enum class SpringForceModel {
	FruchtermanReingold, //!< the force model proposed by Fruchterman and Reingold.
	FruchtermanReingoldModAttr,
	FruchtermanReingoldModRep,
	Eades, //!< the force model proposed by Eades for the original spring embedder.
	Hachul, //!< the force model proposed by Hachul (FMMMLayout)
	Gronemann //!< the force model proposed by Gronemann (FastMultipoleEmbedder).
};

}
