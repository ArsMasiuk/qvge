/** \file
 * \brief Offers simple SimDraw creations.
 *
 * \author Michael Schulz and Daniel Lueckerath
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

#include <ogdf/simultaneous/SimDrawCreator.h>

namespace ogdf
{
//! Offers predefined SimDraw creations.
/**
 * This class offers some predefined SimDraw creations, such as
 * an instance of two outerplanar graphs from Brass et al. (WADS'03)
 * or an instance of a path and a planar graph from Erten and Kobourov
 * (GD'04).
 */

class OGDF_EXPORT SimDrawCreatorSimple : public SimDrawCreator
{

public:
	//! constructor
	explicit SimDrawCreatorSimple(SimDraw &SD) : SimDrawCreator(SD) {}

	//! creates pair-of-tree instance from Geyer, Kaufmann, Vrto (GD'05)
	void createTrees_GKV05(int n);

	//! creates instance of a path and a planar graph from Erten and Kobourov (GD'04)
	void createPathPlanar_EK04();

	//! creates K5 instance from Erten and Kobourov (GD'04)
	void createK5_EK04();

	//!creates K5 instance from Gassner et al. (WG'06)
	void createK5_GJPSS06();

	//!creates instance of two outerplanar graphs from Brass et al. (WADS'03)
	void createOuterplanar_BCDEEIKLM03();

	//!creates instance from Kratochvil (GD'98)
	void createKrat98(int N, int nodeNumber);

	//! creates instance with numberofBasic*2 outer,
	//! numberOfParallels*numberOfBasic inner Nodes and one Root.
	void createWheel(int numberOfParallels, int numberOfbasic);

	//! creates simultaneously planar simultaneous graph with n+1 basic graphs.
	void createExpo(int n);
};

}
